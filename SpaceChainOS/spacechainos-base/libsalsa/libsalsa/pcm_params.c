/*
 *  SALSA-Lib - PCM HW/SW PARAMS
 *
 *  Copyright (c) 2007-2012 by Takashi Iwai <tiwai@suse.de>
 *
 *   This library is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as
 *   published by the Free Software Foundation; either version 2.1 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <limits.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "pcm.h"
#include "control.h"
#include "local.h"


/*
 * non-inlined mask operations
 */
#define MASK_SIZE (SND_MASK_MAX / 32)
#define MASK_OFS(i)	((i) >> 5)
#define MASK_BIT(i)	(1U << ((i) & 31))

#define mask_set_any(m)		_snd_mask_any(m)
#define mask_get(m, val)	_snd_mask_test(m, val)
#define mask_set(m, val)	_snd_mask_set(m, val)
#define mask_reset(m, val)	_snd_mask_reset(m, val)
#define mask_is_empty(m)	_snd_mask_empty(m)
#define mask_clear(m)		_snd_mask_none(m)

static int mask_is_single(const snd_mask_t *mask)
{
	int i, c = 0;
	for (i = 0; i < MASK_SIZE; i++) {
		if (!mask->bits[i])
			continue;
		if (mask->bits[i] & (mask->bits[i] - 1))
			return 0;
		if (c)
			return 0;
		c++;
	}
	return 1;
}

static void mask_leave_single(snd_mask_t *mask, unsigned int val)
{
	unsigned int v;
	v = mask_get(mask, val);
	mask_clear(mask);
	mask->bits[MASK_OFS(val)] = v;
}

static void mask_reset_range(snd_mask_t *mask, unsigned int from,
			     unsigned int to)
{
	for (; from <= to; from++)
		mask_reset(mask, from);
}

static unsigned int ld2(u_int32_t v)
{
	unsigned int r, bits;
	for (r = 0, bits = 16; v > 1; bits >>= 1) {
		if (v >= (1U << bits)) {
			v >>= bits;
			r += bits;
		}
	}
	return r;
}

static unsigned int mask_get_max(const snd_mask_t *mask)
{
	int i;
	for (i = MASK_SIZE - 1; i >= 0; i--) {
		if (mask->bits[i])
			return ld2(mask->bits[i]) + (i << 5);
	}
	return 0;
}

static unsigned int mask_get_min(const snd_mask_t *mask)
{
	int i;
	for (i = 0; i < MASK_SIZE; i++) {
		if (mask->bits[i])
			return ffs(mask->bits[i]) - 1 + (i << 5);
	}
	return 0;
}

#define mask_get_final(mask)	mask_get_min(mask)

static int mask_is_full(const snd_mask_t *mask)
{
	int i;
	for (i = 0; i < MASK_SIZE; i++)
		if (mask->bits[i] != (unsigned int)-1)
			return 0;
	return 1;
}

static int snd_mask_refine(snd_mask_t *mask, const snd_mask_t *v)
{
	int i;
	snd_mask_t old = *mask;
	for (i = 0; i < MASK_SIZE; i++)
		mask->bits[i] &= v->bits[i];
	if (mask_is_empty(mask))
		return -EINVAL;
	return memcmp(mask, &old, sizeof(old));
}

static int snd_mask_refine_first(snd_mask_t *mask)
{
	if (mask_is_empty(mask))
		return -ENOENT;
	if (mask_is_single(mask))
		return 0;
	mask_leave_single(mask, mask_get_min(mask));
	return 1;
}

static int snd_mask_refine_last(snd_mask_t *mask)
{
	if (mask_is_empty(mask))
		return -ENOENT;
	if (mask_is_single(mask))
		return 0;
	mask_leave_single(mask, mask_get_max(mask));
	return 1;
}

static int snd_mask_refine_min(snd_mask_t *mask, unsigned int val)
{
	if (mask_is_empty(mask))
		return -ENOENT;
	if (mask_get_min(mask) >= val)
		return 0;
	mask_reset_range(mask, 0, val - 1);
	if (mask_is_empty(mask))
		return -EINVAL;
	return 1;
}

static int snd_mask_refine_max(snd_mask_t *mask, unsigned int val)
{
	if (mask_is_empty(mask))
		return -ENOENT;
	if (mask_get_max(mask) <= val)
		return 0;
	mask_reset_range(mask, val + 1, SND_MASK_MAX - 1);
	if (mask_is_empty(mask))
		return -EINVAL;
	return 1;
}

static int snd_mask_refine_set(snd_mask_t *mask, unsigned int val)
{
	int changed;
	if (mask_is_empty(mask))
		return -ENOENT;
	changed = !mask_is_single(mask);
	mask_leave_single(mask, val);
	if (mask_is_empty(mask))
		return -EINVAL;
	return changed;
}

/*
 * Operations for interval types
 */

static void interval_set_any(snd_interval_t *i)
{
	i->min = 0;
	i->openmin = 0;
	i->max = -1;
	i->openmax = 0;
	i->integer = 0;
	i->empty = 0;
}

#define interval_is_empty(i)	(i)->empty

static inline int interval_is_full(const snd_interval_t *i)
{
	return i->min == 0 && i->openmin == 0 && 
		i->max == -1 && i->openmax == 0;
}

static inline int interval_is_single(const snd_interval_t *i)
{
	return (i->min == i->max || 
		(i->min + 1 == i->max && i->openmax));
}

#define interval_get_min(i)	(i)->min
#define interval_get_max(i)	(i)->max
#define interval_get_final(i)	interval_get_min(i)

static int interval_check_empty(snd_interval_t *i)
{
	if (i->min > i->max ||
	    (i->min == i->max && (i->openmin || i->openmax))) {
		i->empty = 1;
		return -1;
	}
	return 0;
}

static int snd_interval_refine_min(snd_interval_t *i, unsigned int min,
				   int openmin, int integer)
{
	int changed = 0;
	if (interval_is_empty(i))
		return -ENOENT;
	if (i->min < min) {
		i->min = min;
		i->openmin = openmin;
		changed = 1;
	} else if (i->min == min && !i->openmin && openmin) {
		i->openmin = 1;
		changed = 1;
	}
	if (!i->integer && integer) {
		i->integer = 1;
		changed = 1;
	}
	if (i->integer) {
		if (i->openmin) {
			i->min++;
			i->openmin = 0;
		}
	}
	if (interval_check_empty(i))
		return -EINVAL;
	return changed;
}

static int snd_interval_refine_max(snd_interval_t *i, unsigned int max,
				   int openmax, int integer)
{
	int changed = 0;
	if (interval_is_empty(i))
		return -ENOENT;
	if (i->max > max) {
		i->max = max;
		i->openmax = openmax;
		changed = 1;
	} else if (i->max == max && !i->openmax && openmax) {
		i->openmax = 1;
		changed = 1;
	}
	if (!i->integer && integer) {
		i->integer = 1;
		changed = 1;
	}
	if (i->integer) {
		if (i->openmax) {
			i->max--;
			i->openmax = 0;
		}
	}
	if (interval_check_empty(i))
		return -EINVAL;
	return changed;
}

static int snd_interval_refine(snd_interval_t *i, const snd_interval_t *v)
{
	int err, changed;
	err = snd_interval_refine_min(i, v->min, v->openmin, v->integer);
	if (err < 0)
		return err;
	changed = err;
	err = snd_interval_refine_max(i, v->max, v->openmax, v->integer);
	if (err < 0)
		return err;
	changed |= err;
	if (!i->integer && !i->openmin && !i->openmax && i->min == i->max)
		i->integer = 1;
	return changed;
}

static int snd_interval_refine_first(snd_interval_t *i)
{
	if (interval_is_empty(i))
		return -ENOENT;
	if (interval_is_single(i))
		return 0;
	i->max = i->min;
	i->openmax = i->openmin;
	if (i->openmax)
		i->max++;
	return 1;
}

static int snd_interval_refine_last(snd_interval_t *i)
{
	if (interval_is_empty(i))
		return -ENOENT;
	if (interval_is_single(i))
		return 0;
	i->min = i->max;
	i->openmin = i->openmax;
	if (i->openmin)
		i->min--;
	return 1;
}

static int snd_interval_refine_set(snd_interval_t *i, unsigned int val)
{
	snd_interval_t t;
	t.empty = 0;
	t.min = t.max = val;
	t.openmin = t.openmax = 0;
	t.integer = 1;
	return snd_interval_refine(i, &t);
}

/*
 * HANDLE HW_PARAMS, SW_PARAMS
 */

/* set up default sw_params values */
static int snd_pcm_sw_params_default(snd_pcm_t *pcm,
				     snd_pcm_sw_params_t *params)
{
	params->tstamp_mode = SND_PCM_TSTAMP_NONE;
	params->tstamp_type = pcm->sw_params.tstamp_type;
	params->period_step = 1;
	params->sleep_min = 0;
	params->avail_min = pcm->period_size;
	params->xfer_align = pcm->period_size;
	params->start_threshold = 1;
	params->stop_threshold = pcm->buffer_size;
	params->silence_threshold = 0;
	params->silence_size = 0;
	params->boundary = pcm->buffer_size;
	while (params->boundary * 2 <= LONG_MAX - pcm->buffer_size)
		params->boundary *= 2;
	return 0;
}

static inline int hw_is_mask(int var)
{
	return var >= SNDRV_PCM_HW_PARAM_FIRST_MASK &&
		var <= SNDRV_PCM_HW_PARAM_LAST_MASK;
}

static inline int hw_is_interval(int var)
{
	return var >= SNDRV_PCM_HW_PARAM_FIRST_INTERVAL &&
		var <= SNDRV_PCM_HW_PARAM_LAST_INTERVAL;
}

static inline
snd_mask_t *hw_param_mask(snd_pcm_hw_params_t *params,
			  int var)
{
	return (snd_mask_t*)&params->masks[var - SNDRV_PCM_HW_PARAM_FIRST_MASK];
}

static inline
snd_interval_t *hw_param_interval(snd_pcm_hw_params_t *params,
				  int var)
{
	return &params->intervals[var - SNDRV_PCM_HW_PARAM_FIRST_INTERVAL];
}

static inline
const snd_mask_t *hw_param_mask_c(const snd_pcm_hw_params_t *params,
				  int var)
{
	return (const snd_mask_t *)hw_param_mask((snd_pcm_hw_params_t*) params, var);
}

static inline
const snd_interval_t *hw_param_interval_c(const snd_pcm_hw_params_t *params,
					  int var)
{
	return (const snd_interval_t *)hw_param_interval((snd_pcm_hw_params_t*) params, var);
}

/*
 */
static void _snd_pcm_hw_param_any(snd_pcm_hw_params_t *params,
				  int var)
{
	if (hw_is_mask(var))
		mask_set_any(hw_param_mask(params, var));
	else
		interval_set_any(hw_param_interval(params, var));
	params->cmask |= 1 << var;
	params->rmask |= 1 << var;
}

static int snd_pcm_hw_param_empty(const snd_pcm_hw_params_t *params,
				  int var)
{
	if (hw_is_mask(var))
		return mask_is_empty(hw_param_mask_c(params, var));
	else
		return interval_is_empty(hw_param_interval_c(params, var));
}

static void _snd_pcm_hw_params_any(snd_pcm_hw_params_t *params)
{
	unsigned int k;
	memset(params, 0, sizeof(*params));
	for (k = SNDRV_PCM_HW_PARAM_FIRST_MASK;
	     k <= SNDRV_PCM_HW_PARAM_LAST_MASK; k++)
		_snd_pcm_hw_param_any(params, k);
	for (k = SNDRV_PCM_HW_PARAM_FIRST_INTERVAL;
	     k <= SNDRV_PCM_HW_PARAM_LAST_INTERVAL; k++)
		_snd_pcm_hw_param_any(params, k);
	params->rmask = ~0U;
	params->cmask = 0;
	params->info = ~0U;
	params->flags = SNDRV_PCM_HW_PARAMS_NORESAMPLE;
}

int snd_pcm_hw_params_any(snd_pcm_t *pcm, snd_pcm_hw_params_t *params)
{
	_snd_pcm_hw_params_any(params);
	return snd_pcm_hw_refine(pcm, params);
}

int _snd_pcm_hw_param_get(const snd_pcm_hw_params_t *params,
			  int var, unsigned int *val, int *dir)
{
	if (hw_is_mask(var)) {
		const snd_mask_t *mask = hw_param_mask_c(params, var);
		if (mask_is_empty(mask) || !mask_is_single(mask))
			return -EINVAL;
		if (dir)
			*dir = 0;
		if (val)
			*val = mask_get_final(mask);
		return 0;
	} else if (hw_is_interval(var)) {
		const snd_interval_t *i = hw_param_interval_c(params, var);
		if (interval_is_empty(i) || !interval_is_single(i))
			return -EINVAL;
		if (dir)
			*dir = i->openmin;
		if (val)
			*val = interval_get_final(i);
		return 0;
	}
	return -EINVAL;
}

int _snd_pcm_hw_param_get_min(const snd_pcm_hw_params_t *params,
			      int var, unsigned int *val, int *dir)
{
	if (hw_is_mask(var)) {
		const snd_mask_t *m = hw_param_mask_c(params, var);
		if (dir)
			*dir = 0;
		if (val)
			*val = mask_get_min(m);
		return 0;
	} else if (hw_is_interval(var)) {
		const snd_interval_t *i = hw_param_interval_c(params, var);
		if (dir)
			*dir = i->openmin;
		if (val)
			*val = interval_get_min(i);
		return 0;
	}
	return 0;
}

int _snd_pcm_hw_param_get_max(const snd_pcm_hw_params_t *params,
			      int var, unsigned int *val, int *dir)
{
	if (hw_is_mask(var)) {
		const snd_mask_t *m = hw_param_mask_c(params, var);
		if (dir)
			*dir = 0;
		if (val)
			*val = mask_get_max(m);
		return 0;
	} else if (hw_is_interval(var)) {
		const snd_interval_t *i = hw_param_interval_c(params, var);
		if (dir)
			*dir = -(int)i->openmax;
		if (val)
			*val = interval_get_max(i);
		return 0;
	}
	return 0;
}

static int hw_param_set_min(snd_pcm_hw_params_t *params,
			    int var, unsigned int val, int dir)
{
	int openmin;

	if (hw_is_mask(var))
		return snd_mask_refine_min(hw_param_mask(params, var), val);

	openmin = 0;
	if (dir > 0)
		openmin = 1;
	else if (dir < 0 && val > 0) {
		openmin = 1;
		val--;
	}
	return snd_interval_refine_min(hw_param_interval(params, var),
				       val, openmin, 0);
}

static int hw_param_update_var(snd_pcm_t *pcm, snd_pcm_hw_params_t *params,
			       int var, int changed)
{
	if (changed < 0)
		return changed;
	if (changed) {
		params->cmask |= 1 << var;
		params->rmask |= 1 << var;
	}
	if (params->rmask) {
		changed = snd_pcm_hw_refine(pcm, params);
		if (changed < 0)
			return changed;
		if (snd_pcm_hw_param_empty(params, var))
			return -ENOENT;
	}
	return 0;
}

int _snd_pcm_hw_param_set_min(snd_pcm_t *pcm, snd_pcm_hw_params_t *params,
			      int var, unsigned int *val, int *dir)
{
	snd_pcm_hw_params_t save;
	int err;

	save = *params;
	err = hw_param_set_min(params, var, *val, dir ? *dir : 0);
	err = hw_param_update_var(pcm, params, var, err);
	if (!err)
		return _snd_pcm_hw_param_get_min(params, var, val, dir);
	*params = save;
	return err;
}

static int hw_param_set_max(snd_pcm_hw_params_t *params,
			    int var, unsigned int val, int dir)
{
	int openmax;

	if (hw_is_mask(var))
		return snd_mask_refine_max(hw_param_mask(params, var), val);

	openmax = 0;
	if (dir) {
		openmax = 1;
		if (dir > 0)
			val++;
	}
	return snd_interval_refine_max(hw_param_interval(params, var),
				       val, openmax, 0);
}

int _snd_pcm_hw_param_set_max(snd_pcm_t *pcm, snd_pcm_hw_params_t *params,
			      int var, unsigned int *val, int *dir)
{
	snd_pcm_hw_params_t save;
	int err;

	save = *params;
	err = hw_param_set_max(params, var, *val, dir ? *dir : 0);
	err = hw_param_update_var(pcm, params, var, err);
	if (!err)
		return _snd_pcm_hw_param_get_max(params, var, val, dir);
	*params = save;
	return err;
}

static int hw_param_set_minmax(snd_pcm_hw_params_t *params,
			       int var,
			       unsigned int min, int mindir,
			       unsigned int max, int maxdir)
{
	int c1, c2;
	c1 = hw_param_set_min(params, var, min, mindir);
	if (c1 < 0)
		return c1;
	c2 = hw_param_set_max(params, var, max, maxdir);
	if (c2 < 0)
		return c2;
	return c1 || c2;
}

int _snd_pcm_hw_param_set_minmax(snd_pcm_t *pcm, snd_pcm_hw_params_t *params,
				 int var,
				 unsigned int *min, int *mindir,
				 unsigned int *max, int *maxdir)
{
	snd_pcm_hw_params_t save;
	int err;

	save = *params;
	err = hw_param_set_minmax(params, var, 
				  *min, mindir ? *mindir : 0,
				  *max, maxdir ? *maxdir : 0);
	err = hw_param_update_var(pcm, params, var, err);
	if (!err) {
		err = _snd_pcm_hw_param_get_min(params, var, min, mindir);
		if (err >= 0)
			return _snd_pcm_hw_param_get_max(params, var, max,
							 maxdir);
	}
	*params = save;
	return err;
}

static int hw_param_set(snd_pcm_hw_params_t *params,
			int var, unsigned int val, int dir)
{
	if (hw_is_mask(var)) {
		snd_mask_t *m = hw_param_mask(params, var);
		if (val == 0 && dir < 0) {
			mask_clear(m);
			return -EINVAL;
		}
		if (dir > 0)
			val++;
		else if (dir < 0)
			val--;
		return snd_mask_refine_set(hw_param_mask(params, var), val);
	} else {
		snd_interval_t *i = hw_param_interval(params, var);
		if (val == 0 && dir < 0) {
			i->empty = 1;
			return -EINVAL;
		}
		if (dir == 0)
			return snd_interval_refine_set(i, val);
		else {
			snd_interval_t t;
			t.openmin = 1;
			t.openmax = 1;
			t.empty = 0;
			t.integer = 0;
			if (dir < 0) {
				t.min = val - 1;
				t.max = val;
			} else {
				t.min = val;
				t.max = val + 1;
			}
			return snd_interval_refine(i, &t);
		}
	}
}

int _snd_pcm_hw_param_set(snd_pcm_t *pcm, snd_pcm_hw_params_t *params,
			  int var, unsigned int val, int dir)
{
	snd_pcm_hw_params_t save = *params;
	int err = hw_param_set(params, var, val, dir);
	err = hw_param_update_var(pcm, params, var, err);
	if (err < 0)
		*params = save;
	return err;
}

int _snd_pcm_hw_param_test(snd_pcm_t *pcm, snd_pcm_hw_params_t *params,
			   int var, unsigned int val, int *dir)
{
	snd_pcm_hw_params_t save = *params;
	int err = _snd_pcm_hw_param_set(pcm, params, var, val, dir ? *dir : 0);
	*params = save;
	return err;
}

int _snd_pcm_hw_param_set_integer(snd_pcm_t *pcm, 
				  snd_pcm_hw_params_t *params,
				  int var)
{
	snd_interval_t *i = hw_param_interval(params, var);
	snd_pcm_hw_params_t save;
	int err;

	if (i->integer)
		return 0;
	if (interval_check_empty(i))
		return -EINVAL;
	save = *params;
	i->integer = 1;
	err = hw_param_update_var(pcm, params, var, 1);
	if (err < 0)
		*params = save;
	return err;
}

int _snd_pcm_hw_param_set_mask(snd_pcm_t *pcm, snd_pcm_hw_params_t *params,
			       int var, const snd_mask_t *val)
{
	if (snd_mask_refine(hw_param_mask(params, var), val)) {
		params->cmask |= 1 << var;
		params->rmask |= 1 << var;
	}
	if (params->rmask)
		return snd_pcm_hw_refine(pcm, params);
	return 0;
}

static int hw_param_set_first(snd_pcm_hw_params_t *params, int var)
{
	if (hw_is_mask(var))
		return snd_mask_refine_first(hw_param_mask(params, var));
	else
		return snd_interval_refine_first(hw_param_interval(params, var));
}

static int hw_param_set_last(snd_pcm_hw_params_t *params, int var)
{
	if (hw_is_mask(var))
		return snd_mask_refine_last(hw_param_mask(params, var));
	else
		return snd_interval_refine_last(hw_param_interval(params, var));
}	

int _snd_pcm_hw_param_set_first(snd_pcm_t *pcm, snd_pcm_hw_params_t *params,
				int var, unsigned int *val, int *dir)
{
	int err = hw_param_set_first(params, var);
	err = hw_param_update_var(pcm, params, var, err);
	if (!err)
		return _snd_pcm_hw_param_get(params, var, val, dir);
	return err;
}

int _snd_pcm_hw_param_set_last(snd_pcm_t *pcm, snd_pcm_hw_params_t *params,
			       int var, unsigned int *val, int *dir)
{
	int err = hw_param_set_last(params, var);
	err = hw_param_update_var(pcm, params, var, err);
	if (!err)
		return _snd_pcm_hw_param_get(params, var, val, dir);
	return err;
}

/*
 */
static void boundary_sub(int a, int adir, int b, int bdir, int *c, int *cdir)
{
	adir = adir < 0 ? -1 : (adir > 0 ? 1 : 0);
	bdir = bdir < 0 ? -1 : (bdir > 0 ? 1 : 0);
	a -= b;
	*cdir = adir - bdir;
	if (*cdir == -2)
		a--;
	else if (*cdir == 2)
		a++;
	*c = a;
}

static int boundary_lt(unsigned int a, int adir, unsigned int b, int bdir)
{
	if (adir < 0) {
		a--;
		adir = 1;
	} else if (adir > 0)
		adir = 1;
	if (bdir < 0) {
		b--;
		bdir = 1;
	} else if (bdir > 0)
		bdir = 1;
	return a < b || (a == b && adir < bdir);
}

/* Return 1 if min is nearer to best than max */
static int boundary_nearer(int min, int mindir, int best, int bestdir,
			   int max, int maxdir)
{
	int dmin, dmindir;
	int dmax, dmaxdir;
	boundary_sub(best, bestdir, min, mindir, &dmin, &dmindir);
	boundary_sub(max, maxdir, best, bestdir, &dmax, &dmaxdir);
	return boundary_lt(dmin, dmindir, dmax, dmaxdir);
}

int _snd_pcm_hw_param_set_near(snd_pcm_t *pcm, snd_pcm_hw_params_t *params,
			       int var, unsigned int *val, int *dir)
{
	snd_pcm_hw_params_t save;
	int err;
	unsigned int best = *val, saved_min;
	int last = 0;
	unsigned int min, max;
	int mindir, maxdir;
	int valdir = dir ? *dir : 0;
	snd_interval_t *i;

	if (best > INT_MAX)
		best = INT_MAX;
	min = max = best;
	mindir = valdir;
	if (valdir > 0)
		maxdir = 0;
	else if (valdir == 0)
		maxdir = -1;
	else {
		maxdir = 1;
		max--;
	}
	save = *params;
	saved_min = min;
	err = _snd_pcm_hw_param_set_min(pcm, params, var, &min, &mindir);

	i = hw_param_interval(params, var);
	if (!interval_is_empty(i) && interval_is_single(i))
		return _snd_pcm_hw_param_get_min(params, var, val, dir);
	
	if (err >= 0) {
		snd_pcm_hw_params_t params1;
		if (min == saved_min && mindir == valdir)
			goto _end;
		params1 = save;
		err = _snd_pcm_hw_param_set_max(pcm, &params1, var,
						&max, &maxdir);
		if (err < 0)
			goto _end;
		if (boundary_nearer(max, maxdir, best, valdir, min, mindir)) {
			*params = params1;
			last = 1;
		}
	} else {
		*params = save;
		err = _snd_pcm_hw_param_set_max(pcm, params, var, &max, &maxdir);
		if (err < 0)
			return err;
		last = 1;
	}
 _end:
	if (last)
		err = _snd_pcm_hw_param_set_last(pcm, params, var, val, dir);
	else
		err = _snd_pcm_hw_param_set_first(pcm, params, var, val, dir);
	return err;
}

snd_mask_t *_snd_pcm_hw_param_get_mask(snd_pcm_hw_params_t *params, int var)
{
	return hw_param_mask(params, var);
}

static int snd_pcm_hw_params_choose(snd_pcm_t *pcm, snd_pcm_hw_params_t *params)
{
	struct param_choice {
		char var;
		char last;
	};
	static struct param_choice vars[] = {
		{ SNDRV_PCM_HW_PARAM_ACCESS, 0 },
		{ SNDRV_PCM_HW_PARAM_FORMAT, 0 },
		{ SNDRV_PCM_HW_PARAM_SUBFORMAT, 0 },
		{ SNDRV_PCM_HW_PARAM_CHANNELS, 0 },
		{ SNDRV_PCM_HW_PARAM_RATE, 0 },
		{ SNDRV_PCM_HW_PARAM_BUFFER_SIZE, 1 },
		{ SNDRV_PCM_HW_PARAM_PERIOD_SIZE, 0 },
		{ SNDRV_PCM_HW_PARAM_PERIOD_TIME, 0 },
		{ SNDRV_PCM_HW_PARAM_TICK_TIME, 0 },
	};
	int i, err;

	for (i = 0; i < sizeof(vars)/sizeof(vars[0]); i++) {
		if (vars[i].last)
			err = _snd_pcm_hw_param_set_last(pcm, params,
							 vars[i].var,
							 NULL, NULL);
		else
			err = _snd_pcm_hw_param_set_first(pcm, params,
							  vars[i].var,
							  NULL, NULL);
		if (err < 0)
			return err;
	}
	return 0;
}

static int _snd_pcm_hw_params(snd_pcm_t *pcm, snd_pcm_hw_params_t *params)
{
	int err;
	snd_pcm_sw_params_t sw;

	err = snd_pcm_hw_refine(pcm, params);
	if (err < 0)
		return err;
	snd_pcm_hw_params_choose(pcm, params);
	snd_pcm_hw_free(pcm);
	if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_HW_PARAMS, params) < 0)
		return -errno;
#if 0
	params->info &= ~0xf0000000;
	params->info |= (pcm->monotonic ? SND_PCM_INFO_MONOTONIC : 0);
#endif
	pcm->setup = 1;
	pcm->hw_params = *params;
	
	snd_pcm_hw_params_get_access(params, &pcm->_access);
	snd_pcm_hw_params_get_format(params, &pcm->format);
	snd_pcm_hw_params_get_subformat(params, &pcm->subformat);
	snd_pcm_hw_params_get_channels(params, &pcm->channels);
	snd_pcm_hw_params_get_rate(params, &pcm->rate, 0);
	snd_pcm_hw_params_get_period_time(params, &pcm->period_time, 0);
	snd_pcm_hw_params_get_period_size(params, &pcm->period_size, 0);
	snd_pcm_hw_params_get_buffer_size(params, &pcm->buffer_size);
	pcm->sample_bits = snd_pcm_format_physical_width(pcm->format);
	pcm->frame_bits = pcm->sample_bits * pcm->channels;

	/* Default sw params */
	memset(&sw, 0, sizeof(sw));
	snd_pcm_sw_params_default(pcm, &sw);
	snd_pcm_sw_params(pcm, &sw);

	if (pcm->_access == SND_PCM_ACCESS_MMAP_INTERLEAVED ||
	    pcm->_access == SND_PCM_ACCESS_MMAP_NONINTERLEAVED ||
	    pcm->_access == SND_PCM_ACCESS_MMAP_COMPLEX) {
		err = _snd_pcm_mmap(pcm);
		if (err < 0) {
			_snd_pcm_munmap(pcm);
			return err;
		}
	}

	_snd_pcm_sync_ptr(pcm, 0);
	return 0;
}

int snd_pcm_hw_params_get_min_align(const snd_pcm_hw_params_t *params,
				    snd_pcm_uframes_t *val)
{
	unsigned int format, channels, fb, min_align;
	int err;

	err = _snd_pcm_hw_param_get(params, SNDRV_PCM_HW_PARAM_FORMAT,
				    &format, NULL);
	if (err < 0)
		return err;
	err = _snd_pcm_hw_param_get(params, SNDRV_PCM_HW_PARAM_CHANNELS,
				    &channels, NULL);
	if (err < 0)
		return err;
	/* compute frame bits */
	fb = snd_pcm_format_physical_width(format) * channels;
        min_align = 1;
	while (fb % 8) {
		fb *= 2;
                min_align *= 2;
	}
	if (val)
		*val = min_align;
	return 0;
}

int snd_pcm_hw_params(snd_pcm_t *pcm, snd_pcm_hw_params_t *params)
{
	int err;
	err = _snd_pcm_hw_params(pcm, params);
	if (err < 0)
		return err;
	err = snd_pcm_prepare(pcm);
	return err;
}

int snd_pcm_hw_free(snd_pcm_t *pcm)
{
	if (!pcm->setup)
		return 0;
	_snd_pcm_munmap(pcm);
	if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_HW_FREE) < 0)
		return -errno;
	pcm->setup = 0;
	return 0;
}

int snd_pcm_sw_params(snd_pcm_t *pcm, snd_pcm_sw_params_t *params)
{
	if (!pcm->setup)
		return -EBADFD;
	if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_SW_PARAMS, params) < 0)
		return -errno;
	pcm->sw_params = *params;
	pcm->mmap_control->avail_min = params->avail_min;
	return 0;
}

/*
 * DUMP HW PARAMS
 */

static void snd_mask_print(const snd_mask_t *mask, int var, snd_output_t *out)
{
	unsigned int i;
	if (mask_is_empty(mask)) {
		snd_output_puts(out, " NONE");
		return;
	} else if (mask_is_full(mask)) {
		snd_output_puts(out, " ALL");
		return;
	}
	for (i = 0; i < SND_MASK_MAX; i++) {
		const char *s;
		if (!mask_get(mask, i))
			continue;
		switch (var) {
		case SNDRV_PCM_HW_PARAM_ACCESS:
			s = snd_pcm_access_name(i);
			break;
		case SNDRV_PCM_HW_PARAM_FORMAT:
			s = snd_pcm_format_name(i);
			break;
		case SNDRV_PCM_HW_PARAM_SUBFORMAT:
			s = snd_pcm_subformat_name(i);
			break;
		default:
			continue;
		}
		snd_output_printf(out, " %s", s);
	}
}

static void snd_interval_print(const snd_interval_t *i, snd_output_t *out)
{
	if (interval_is_empty(i))
		snd_output_printf(out, "NONE");
	else if (interval_is_full(i))
		snd_output_printf(out, "ALL");
	else if (interval_is_single(i) && i->integer)
		snd_output_printf(out, "%u", interval_get_final(i));
	else
		snd_output_printf(out, "%c%u %u%c",
				i->openmin ? '(' : '[',
				i->min, i->max,
				i->openmax ? ')' : ']');
}

void snd_pcm_hw_param_dump(const snd_pcm_hw_params_t *params,
			   int var, snd_output_t *out)
{
	if (hw_is_mask(var))
		snd_mask_print(hw_param_mask_c(params, var), var, out);
	else if (hw_is_interval(var))
		snd_interval_print(hw_param_interval_c(params, var), out);
}

#define HW_PARAM(v) [SNDRV_PCM_HW_PARAM_##v] = #v

static const char * const snd_pcm_hw_param_names[] = {
	HW_PARAM(ACCESS),
	HW_PARAM(FORMAT),
	HW_PARAM(SUBFORMAT),
	HW_PARAM(SAMPLE_BITS),
	HW_PARAM(FRAME_BITS),
	HW_PARAM(CHANNELS),
	HW_PARAM(RATE),
	HW_PARAM(PERIOD_TIME),
	HW_PARAM(PERIOD_SIZE),
	HW_PARAM(PERIOD_BYTES),
	HW_PARAM(PERIODS),
	HW_PARAM(BUFFER_TIME),
	HW_PARAM(BUFFER_SIZE),
	HW_PARAM(BUFFER_BYTES),
	HW_PARAM(TICK_TIME),
};

static void dump_one_param(snd_pcm_hw_params_t *params, unsigned int k,
			   snd_output_t *out)
{
	snd_output_printf(out, "%s: ", snd_pcm_hw_param_names[k]);
	snd_pcm_hw_param_dump(params, k, out);
	snd_output_putc(out, '\n');
}

int snd_pcm_hw_params_dump(snd_pcm_hw_params_t *params, snd_output_t *out)
{
	unsigned int k;
	for (k = SNDRV_PCM_HW_PARAM_FIRST_MASK; k <= SNDRV_PCM_HW_PARAM_LAST_MASK; k++)
		dump_one_param(params, k, out);
	for (k = SNDRV_PCM_HW_PARAM_FIRST_INTERVAL; k <= SNDRV_PCM_HW_PARAM_LAST_INTERVAL; k++)
		dump_one_param(params, k, out);
	return 0;
}

int snd_pcm_sw_params_dump(snd_pcm_sw_params_t *params, snd_output_t *out)
{
	snd_output_printf(out, "tstamp_mode: %s\n",
			  snd_pcm_tstamp_mode_name(params->tstamp_mode));
	snd_output_printf(out, "period_step: %u\n",
			  params->period_step);
	snd_output_printf(out, "sleep_min: %u\n",
			  params->sleep_min);
	snd_output_printf(out, "avail_min: %lu\n",
			  params->avail_min);
	snd_output_printf(out, "xfer_align: %lu\n",
			  params->xfer_align);
	snd_output_printf(out, "silence_threshold: %lu\n",
			  params->silence_threshold);
	snd_output_printf(out, "silence_size: %lu\n",
			  params->silence_size);
	snd_output_printf(out, "boundary: %lu\n",
			  params->boundary);
	return 0;
}

