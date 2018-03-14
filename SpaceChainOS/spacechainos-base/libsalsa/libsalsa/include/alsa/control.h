/*
 *  SALSA-Lib - Control Interface
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

#ifndef __ALSA_CONTROL_H
#define __ALSA_CONTROL_H

#include "recipe.h"
#include "ctl_func.h"

#include "ctl_macros.h"

#define snd_ctl_elem_id_alloca(ptr)	__snd_alloca(ptr, snd_ctl_elem_id)
#define snd_ctl_card_info_alloca(ptr)	__snd_alloca(ptr, snd_ctl_card_info)
#define snd_ctl_event_alloca(ptr)	__snd_alloca(ptr, snd_ctl_event)
#define snd_ctl_elem_list_alloca(ptr)	__snd_alloca(ptr, snd_ctl_elem_list)
#define snd_ctl_elem_info_alloca(ptr)	__snd_alloca(ptr, snd_ctl_elem_info)
#define snd_ctl_elem_value_alloca(ptr)	__snd_alloca(ptr, snd_ctl_elem_value)

#endif /* __ALSA_CONTROL_H */
