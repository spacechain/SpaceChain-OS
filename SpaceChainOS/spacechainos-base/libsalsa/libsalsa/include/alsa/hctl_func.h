/*
 * SALSA-Lib - High-level Control Interface
 *
 * H-control declarations
 */

#include "control.h"

typedef struct _snd_hctl_elem snd_hctl_elem_t;
typedef struct _snd_hctl snd_hctl_t;
typedef int (*snd_hctl_compare_t)(const snd_hctl_elem_t *e1,
				  const snd_hctl_elem_t *e2);
int snd_hctl_compare_fast(const snd_hctl_elem_t *c1,
			  const snd_hctl_elem_t *c2);
typedef int (*snd_hctl_callback_t)(snd_hctl_t *hctl,
				   unsigned int mask,
				   snd_hctl_elem_t *elem);
typedef int (*snd_hctl_elem_callback_t)(snd_hctl_elem_t *elem,
					unsigned int mask);

#if SALSA_CHECK_ABI
int _snd_hctl_open(snd_hctl_t **hctl, const char *name, int mode,
		   unsigned int magic);
#else
int snd_hctl_open(snd_hctl_t **hctl, const char *name, int mode);
#endif
int snd_hctl_open_ctl(snd_hctl_t **hctlp, snd_ctl_t *ctl);
int snd_hctl_close(snd_hctl_t *hctl);
snd_hctl_elem_t *snd_hctl_find_elem(snd_hctl_t *hctl, const snd_ctl_elem_id_t *id);
int snd_hctl_load(snd_hctl_t *hctl);
int snd_hctl_free(snd_hctl_t *hctl);
int snd_hctl_handle_events(snd_hctl_t *hctl);
