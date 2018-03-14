/*
 *  SALSA-Lib - PCM Interface
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

#ifndef __ALSA_PCM_H_INC
#define __ALSA_PCM_H_INC

#include "recipe.h"
#include "pcm_func.h"

#include "pcm_macros.h"

#define snd_pcm_info_alloca(ptr)	__snd_alloca(ptr, snd_pcm_info)
#define snd_pcm_hw_params_alloca(ptr)	__snd_alloca(ptr, snd_pcm_hw_params)
#define snd_pcm_sw_params_alloca(ptr)	__snd_alloca(ptr, snd_pcm_sw_params)
#define snd_pcm_access_mask_alloca(ptr)	__snd_alloca(ptr, snd_pcm_access_mask)
#define snd_pcm_format_mask_alloca(ptr)	__snd_alloca(ptr, snd_pcm_format_mask)
#define snd_pcm_subformat_mask_alloca(ptr) \
	__snd_alloca(ptr, snd_pcm_subformat_mask)
#define snd_pcm_status_alloca(ptr)	__snd_alloca(ptr, snd_pcm_status)

#endif /* __ALSA_PCM_H_INC */
