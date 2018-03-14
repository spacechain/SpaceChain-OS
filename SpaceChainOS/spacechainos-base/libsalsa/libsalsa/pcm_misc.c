/*
 *  SALSA-Lib - PCM Interface - misc routines
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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#ifndef SYLIXOS
#include <byteswap.h>
#endif /* !SYLIXOS */
#include "pcm.h"


const struct snd_pcm_format_data _snd_pcm_formats[SND_PCM_FORMAT_LAST+1] = {
	[SND_PCM_FORMAT_S8] = {
		.width = 8, .phys = 8, .le = -EINVAL, .signd = 1,
		.silence = {},
	},
	[SND_PCM_FORMAT_U8] = {
		.width = 8, .phys = 8, .le = -EINVAL, .signd = 0,
		.silence = { 0x80 },
	},
	[SND_PCM_FORMAT_S16_LE] = {
		.width = 16, .phys = 16, .le = 1, .signd = 1,
		.silence = {},
	},
	[SND_PCM_FORMAT_S16_BE] = {
		.width = 16, .phys = 16, .le = 0, .signd = 1,
		.silence = {},
	},
	[SND_PCM_FORMAT_U16_LE] = {
		.width = 16, .phys = 16, .le = 1, .signd = 0,
		.silence = { 0x00, 0x80 },
	},
	[SND_PCM_FORMAT_U16_BE] = {
		.width = 16, .phys = 16, .le = 0, .signd = 0,
		.silence = { 0x80, 0x00 },
	},
	[SND_PCM_FORMAT_S24_LE] = {
		.width = 24, .phys = 32, .le = 1, .signd = 1,
		.silence = {},
	},
	[SND_PCM_FORMAT_S24_BE] = {
		.width = 24, .phys = 32, .le = 0, .signd = 1,
		.silence = {},
	},
	[SND_PCM_FORMAT_U24_LE] = {
		.width = 24, .phys = 32, .le = 1, .signd = 0,
		.silence = { 0x00, 0x00, 0x80, 0x00 },
	},
	[SND_PCM_FORMAT_U24_BE] = {
		.width = 24, .phys = 32, .le = 0, .signd = 0,
		.silence = { 0x00, 0x80, 0x00, 0x00 },
	},
	[SND_PCM_FORMAT_S32_LE] = {
		.width = 32, .phys = 32, .le = 1, .signd = 1,
		.silence = {},
	},
	[SND_PCM_FORMAT_S32_BE] = {
		.width = 32, .phys = 32, .le = 0, .signd = 1,
		.silence = {},
	},
	[SND_PCM_FORMAT_U32_LE] = {
		.width = 32, .phys = 32, .le = 1, .signd = 0,
		.silence = { 0x00, 0x00, 0x00, 0x80 },
	},
	[SND_PCM_FORMAT_U32_BE] = {
		.width = 32, .phys = 32, .le = 0, .signd = 0,
		.silence = { 0x80, 0x00, 0x00, 0x00 },
	},
	[SND_PCM_FORMAT_FLOAT_LE] = {
		.width = 32, .phys = 32, .le = 1, .signd = -EINVAL,
		.silence = {},
	},
	[SND_PCM_FORMAT_FLOAT_BE] = {
		.width = 32, .phys = 32, .le = 0, .signd = -EINVAL,
		.silence = {},
	},
	[SND_PCM_FORMAT_FLOAT64_LE] = {
		.width = 64, .phys = 64, .le = 1, .signd = -EINVAL,
		.silence = {},
	},
	[SND_PCM_FORMAT_FLOAT64_BE] = {
		.width = 64, .phys = 64, .le = 0, .signd = -EINVAL,
		.silence = {},
	},
	[SND_PCM_FORMAT_IEC958_SUBFRAME_LE] = {
		.width = 32, .phys = 32, .le = 1, .signd = -EINVAL,
		.silence = {},
	},
	[SND_PCM_FORMAT_IEC958_SUBFRAME_BE] = {
		.width = 32, .phys = 32, .le = 0, .signd = -EINVAL,
		.silence = {},
	},
	[SND_PCM_FORMAT_MU_LAW] = {
		.width = 8, .phys = 8, .le = -EINVAL, .signd = -EINVAL,
		.silence = { 0x7f },
	},
	[SND_PCM_FORMAT_A_LAW] = {
		.width = 8, .phys = 8, .le = -EINVAL, .signd = -EINVAL,
		.silence = { 0x55 },
	},
	[SND_PCM_FORMAT_IMA_ADPCM] = {
		.width = 4, .phys = 4, .le = -EINVAL, .signd = -EINVAL,
		.silence = {},
	},
	/* FIXME: the following three formats are not defined properly yet */
	[SND_PCM_FORMAT_MPEG...SND_PCM_FORMAT_SPECIAL] = {
		.width = -EINVAL, .phys = -EINVAL,
		.le = -EINVAL, .signd = -EINVAL,
	},
	[SND_PCM_FORMAT_S24_3LE] = {
		.width = 24, .phys = 24, .le = 1, .signd = 1,
		.silence = {},
	},
	[SND_PCM_FORMAT_S24_3BE] = {
		.width = 24, .phys = 24, .le = 0, .signd = 1,
		.silence = {},
	},
	[SND_PCM_FORMAT_U24_3LE] = {
		.width = 24, .phys = 24, .le = 1, .signd = 0,
		.silence = { 0x00, 0x00, 0x80 },
	},
	[SND_PCM_FORMAT_U24_3BE] = {
		.width = 24, .phys = 24, .le = 0, .signd = 0,
		.silence = { 0x80, 0x00, 0x00 },
	},
	[SND_PCM_FORMAT_S20_3LE] = {
		.width = 20, .phys = 24, .le = 1, .signd = 1,
		.silence = {},
	},
	[SND_PCM_FORMAT_S20_3BE] = {
		.width = 20, .phys = 24, .le = 0, .signd = 1,
		.silence = {},
	},
	[SND_PCM_FORMAT_U20_3LE] = {
		.width = 20, .phys = 24, .le = 1, .signd = 0,
		.silence = { 0x00, 0x00, 0x08 },
	},
	[SND_PCM_FORMAT_U20_3BE] = {
		.width = 20, .phys = 24, .le = 0, .signd = 0,
		.silence = { 0x08, 0x00, 0x00 },
	},
	[SND_PCM_FORMAT_S18_3LE] = {
		.width = 18, .phys = 24, .le = 1, .signd = 1,
		.silence = {},
	},
	[SND_PCM_FORMAT_S18_3BE] = {
		.width = 18, .phys = 24, .le = 0, .signd = 1,
		.silence = {},
	},
	[SND_PCM_FORMAT_U18_3LE] = {
		.width = 18, .phys = 24, .le = 1, .signd = 0,
		.silence = { 0x00, 0x00, 0x02 },
	},
	[SND_PCM_FORMAT_U18_3BE] = {
		.width = 18, .phys = 24, .le = 0, .signd = 0,
		.silence = { 0x02, 0x00, 0x00 },
	},
	[SND_PCM_FORMAT_G723_24] = {
		.width = 3, .phys = 3, .le = -1, .signd = -1,
		.silence = {},
	},
	[SND_PCM_FORMAT_G723_40] = {
		.width = 5, .phys = 5, .le = -1, .signd = -1,
		.silence = {},
	},
	[SND_PCM_FORMAT_DSD_U8] = {
		.width = 8, .phys = 8, .le = 1, .signd = 0,
		.silence = { 0x69 },
	},
	[SND_PCM_FORMAT_DSD_U16_LE] = {
		.width = 16, .phys = 16, .le = 1, .signd = 0,
		.silence = { 0x69, 0x69 },
	},
	[SND_PCM_FORMAT_DSD_U32_LE] = {
		.width = 32, .phys = 32, .le = 1, .signd = 0,
		.silence = { 0x69, 0x69, 0x69, 0x69 },
	},
	[SND_PCM_FORMAT_DSD_U16_BE] = {
		.width = 16, .phys = 16, .le = 0, .signd = 0,
		.silence = { 0x69, 0x69 },
	},
	[SND_PCM_FORMAT_DSD_U32_BE] = {
		.width = 32, .phys = 32, .le = 0, .signd = 0,
		.silence = { 0x69, 0x69, 0x69, 0x69 },
	},
};

u_int64_t snd_pcm_format_silence_64(snd_pcm_format_t format)
{
	const struct snd_pcm_format_data *fmt;
	int i, p, w;
	u_int64_t silence;

	fmt = &_snd_pcm_formats[format];
	if (fmt->phys <= 0)
		return 0;
	w = fmt->width / 8;
	p = 0;
	silence = 0;
	while (p < w) {
		for (i = 0; i < w; i++, p++)
			silence = (silence << 8) | fmt->silence[i];
	}
	return silence;
}

int snd_pcm_format_set_silence(snd_pcm_format_t format, void *data,
			       unsigned int samples)
{
	int width;
	unsigned char *dst;
	const unsigned char *pat;

	if (!samples)
		return 0;
	width = _snd_pcm_formats[format].phys;
	if (width <= 0)
		return -EINVAL;
	pat = _snd_pcm_formats[format].silence;
	/* signed or 1 byte data */
	if (_snd_pcm_formats[format].signd == 1 || width <= 8) {
		unsigned int bytes = samples * width / 8;
		memset(data, *pat, bytes);
		return 0;
	}
	/* non-zero samples, fill using a loop */
	width /= 8;
	dst = data;
	while (samples--) {
		memcpy(dst, pat, width);
		dst += width;
	}
	return 0;
}

static const int linear_formats[4][2][2] = {
	{ { SND_PCM_FORMAT_S8, SND_PCM_FORMAT_S8 },
	  { SND_PCM_FORMAT_U8, SND_PCM_FORMAT_U8 } },
	{ { SND_PCM_FORMAT_S16_LE, SND_PCM_FORMAT_S16_BE },
	  { SND_PCM_FORMAT_U16_LE, SND_PCM_FORMAT_U16_BE } },
	{ { SND_PCM_FORMAT_S24_LE, SND_PCM_FORMAT_S24_BE },
	  { SND_PCM_FORMAT_U24_LE, SND_PCM_FORMAT_U24_BE } },
	{ { SND_PCM_FORMAT_S32_LE, SND_PCM_FORMAT_S32_BE },
	  { SND_PCM_FORMAT_U32_LE, SND_PCM_FORMAT_U32_BE } }
};

static const int linear24_formats[3][2][2] = {
	{ { SND_PCM_FORMAT_S24_3LE, SND_PCM_FORMAT_S24_3BE },
	  { SND_PCM_FORMAT_U24_3LE, SND_PCM_FORMAT_U24_3BE } },
	{ { SND_PCM_FORMAT_S20_3LE, SND_PCM_FORMAT_S20_3BE },
	  { SND_PCM_FORMAT_U20_3LE, SND_PCM_FORMAT_U20_3BE } },
	{ { SND_PCM_FORMAT_S18_3LE, SND_PCM_FORMAT_S18_3BE },
	  { SND_PCM_FORMAT_U18_3LE, SND_PCM_FORMAT_U18_3BE } },
};

snd_pcm_format_t snd_pcm_build_linear_format(int width, int pwidth,
					     int unsignd, int big_endian)
{
	if (pwidth == 24) {
		switch (width) {
		case 24:
			width = 0;
			break;
		case 20:
			width = 1;
			break;
		case 18:
			width = 2;
			break;
		default:
			return SND_PCM_FORMAT_UNKNOWN;
		}
		return linear24_formats[width][!!unsignd][!!big_endian];
	} else {
		switch (width) {
		case 8:
			width = 0;
			break;
		case 16:
			width = 1;
			break;
		case 24:
			width = 2;
			break;
		case 32:
			width = 3;
			break;
		default:
			return SND_PCM_FORMAT_UNKNOWN;
		}
		return linear_formats[width][!!unsignd][!!big_endian];
	}
}
