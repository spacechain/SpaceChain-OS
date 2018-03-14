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

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <ctype.h>
#include "pcm.h"
#include "control.h"
#include "local.h"

/*
 */
static int snd_pcm_hw_mmap_status(snd_pcm_t *pcm);
static void snd_pcm_hw_munmap_status(snd_pcm_t *pcm);

/*
 * open/close
 */

static int get_pcm_subdev(int fd)
{
	snd_pcm_info_t info;
	memzero_valgrind(&info, sizeof(info));
	if (ioctl(fd, SNDRV_PCM_IOCTL_INFO, &info) < 0)
		return -1;
	return info.subdevice;
}

#if SALSA_CHECK_ABI
int _snd_pcm_open(snd_pcm_t **pcmp, const char *name, 
		  snd_pcm_stream_t stream, int mode, unsigned int magic)
#else
int snd_pcm_open(snd_pcm_t **pcmp, const char *name, 
		 snd_pcm_stream_t stream, int mode)
#endif
{
	char filename[sizeof(SALSA_DEVPATH) + 24];
	int card, dev, subdev;
	int fd, err, fmode, ver;
	snd_pcm_t *pcm = NULL;

	check_incompatible_abi(magic, SALSA_PCM_MAGIC);

	*pcmp = NULL;

	err = _snd_dev_get_device(name, &card, &dev, &subdev);
	if (err < 0)
		return err;

	snprintf(filename, sizeof(filename), "%s/pcmC%dD%d%c",
		 SALSA_DEVPATH, card, dev,
		 (stream == SND_PCM_STREAM_PLAYBACK ? 'p' : 'c'));
	fmode = O_RDWR | O_NONBLOCK;
	if (mode & SND_PCM_ASYNC)
		fmode |= O_ASYNC;

	if (subdev >= 0) {
		fd = _snd_open_subdev(filename, fmode, card, subdev,
				      SNDRV_CTL_IOCTL_PCM_PREFER_SUBDEVICE);
		if (fd < 0)
			return fd;
		if (get_pcm_subdev(fd) != subdev) {
			close(fd);
			return -EBUSY;
		}
	} else {
		fd = open(filename, fmode);
		if (fd < 0)
			return -errno;
		subdev = get_pcm_subdev(fd);
		if (subdev < 0) {
			close(fd);
			return -EBUSY;
		}
	}
	if (!(mode & SND_PCM_NONBLOCK)) {
		fmode &= ~O_NONBLOCK;
		fcntl(fd, F_SETFL, fmode);
	}

	if (ioctl(fd, SNDRV_PCM_IOCTL_PVERSION, &ver) < 0) {
		err = -errno;
		goto error;
	}

	pcm = calloc(1, sizeof(*pcm));
	if (!pcm) {
		err = -ENOMEM;
		goto error;
	}

	pcm->card = card;
	pcm->stream = stream;
	pcm->device = dev;
	pcm->subdevice = subdev;
	pcm->protocol = ver;
	pcm->fd = fd;
	pcm->pollfd.fd = fd;
	pcm->pollfd.events =
		(stream == SND_PCM_STREAM_PLAYBACK ? POLLOUT : POLLIN)
		| POLLERR | POLLNVAL;

	err = snd_pcm_hw_mmap_status(pcm);
	if (err < 0)
		goto error;

	*pcmp = pcm;
	return 0;

 error:
	free(pcm);
	close(fd);
	return err;
}

int snd_pcm_close(snd_pcm_t *pcm)
{
	if (pcm->setup) {
		snd_pcm_drop(pcm);
		snd_pcm_hw_free(pcm);
	}
	_snd_pcm_munmap(pcm);
	snd_pcm_hw_munmap_status(pcm);
#if SALSA_HAS_ASYNC_SUPPORT
	if (pcm->async)
		snd_async_del_handler(pcm->async);
#endif
	close(pcm->fd);
	free(pcm);
	return 0;
}	


/*
 * READ/WRITE
 */

static int correct_pcm_error(snd_pcm_t *pcm, int err)
{
	switch (snd_pcm_state(pcm)) {
	case SND_PCM_STATE_XRUN:
		return -EPIPE;
	case SND_PCM_STATE_SUSPENDED:
		return -ESTRPIPE;
	case SND_PCM_STATE_DISCONNECTED:
		return -ENODEV;
	default:
		return err;
	}
}

static inline int snd_pcm_check_error(snd_pcm_t *pcm, int err)
{
	if (err == -EINTR)
		return correct_pcm_error(pcm, err);
	return err;
}

snd_pcm_sframes_t snd_pcm_writei(snd_pcm_t *pcm, const void *buffer,
				 snd_pcm_uframes_t size)
{
	struct snd_xferi xferi;

#ifdef DELIGHT_VALGRIND
	xferi.result = 0;
#endif
	xferi.buf = (char*) buffer;
	xferi.frames = size;
	if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_WRITEI_FRAMES, &xferi) < 0)
		return snd_pcm_check_error(pcm, -errno);
	_snd_pcm_sync_ptr(pcm, SNDRV_PCM_SYNC_PTR_APPL);
	return xferi.result;
}

snd_pcm_sframes_t snd_pcm_writen(snd_pcm_t *pcm, void **bufs,
				 snd_pcm_uframes_t size)
{
	struct snd_xfern xfern;

#ifdef DELIGHT_VALGRIND
	xfern.result = 0;
#endif
	xfern.bufs = bufs;
	xfern.frames = size;
	if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_WRITEN_FRAMES, &xfern) < 0)
		return snd_pcm_check_error(pcm, -errno);
	_snd_pcm_sync_ptr(pcm, SNDRV_PCM_SYNC_PTR_APPL);
	return xfern.result;
}

snd_pcm_sframes_t snd_pcm_readi(snd_pcm_t *pcm, void *buffer,
				snd_pcm_uframes_t size)
{
	struct snd_xferi xferi;

#ifdef DELIGHT_VALGRIND
	xferi.result = 0;
#endif
	xferi.buf = buffer;
	xferi.frames = size;
	if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_READI_FRAMES, &xferi) < 0)
		return snd_pcm_check_error(pcm, -errno);
	_snd_pcm_sync_ptr(pcm, SNDRV_PCM_SYNC_PTR_APPL);
	return xferi.result;
}

snd_pcm_sframes_t snd_pcm_readn(snd_pcm_t *pcm, void **bufs,
				snd_pcm_uframes_t size)
{
	struct snd_xfern xfern;

#ifdef DELIGHT_VALGRIND
	xfern.result = 0;
#endif
	xfern.bufs = bufs;
	xfern.frames = size;
	if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_READN_FRAMES, &xfern) < 0)
		return snd_pcm_check_error(pcm, -errno);
	_snd_pcm_sync_ptr(pcm, SNDRV_PCM_SYNC_PTR_APPL);
	return xfern.result;
}


/*
 * STRINGS, DUMP
 */
#define PCMTYPE(v) [SND_PCM_TYPE_##v] = #v
#define STATE(v) [SND_PCM_STATE_##v] = #v
#define STREAM(v) [SND_PCM_STREAM_##v] = #v
#define READY(v) [SND_PCM_READY_##v] = #v
#define XRUN(v) [SND_PCM_XRUN_##v] = #v
#define SILENCE(v) [SND_PCM_SILENCE_##v] = #v
#define TSTAMP(v) [SND_PCM_TSTAMP_##v] = #v
#define ACCESS(v) [SND_PCM_ACCESS_##v] = #v
#define START(v) [SND_PCM_START_##v] = #v
#define HW_PARAM(v) [SND_PCM_HW_PARAM_##v] = #v
#define SW_PARAM(v) [SND_PCM_SW_PARAM_##v] = #v
#define FORMAT(v) [SND_PCM_FORMAT_##v] = #v
#define SUBFORMAT(v) [SND_PCM_SUBFORMAT_##v] = #v 

#define FORMATD(v, d) [SND_PCM_FORMAT_##v] = d
#define SUBFORMATD(v, d) [SND_PCM_SUBFORMAT_##v] = d 


const char * const _snd_pcm_stream_names[SND_PCM_STREAM_LAST + 1] = {
	STREAM(PLAYBACK),
	STREAM(CAPTURE),
};

const char * const _snd_pcm_state_names[SND_PCM_STATE_LAST + 1] = {
	STATE(OPEN),
	STATE(SETUP),
	STATE(PREPARED),
	STATE(RUNNING),
	STATE(XRUN),
	STATE(DRAINING),
	STATE(PAUSED),
	STATE(SUSPENDED),
	STATE(DISCONNECTED),
};

const char * const _snd_pcm_access_names[SND_PCM_ACCESS_LAST + 1] = {
	ACCESS(MMAP_INTERLEAVED), 
	ACCESS(MMAP_NONINTERLEAVED),
	ACCESS(MMAP_COMPLEX),
	ACCESS(RW_INTERLEAVED),
	ACCESS(RW_NONINTERLEAVED),
};

const char * const _snd_pcm_format_names[SND_PCM_FORMAT_LAST + 1] = {
	FORMAT(S8),
	FORMAT(U8),
	FORMAT(S16_LE),
	FORMAT(S16_BE),
	FORMAT(U16_LE),
	FORMAT(U16_BE),
	FORMAT(S24_LE),
	FORMAT(S24_BE),
	FORMAT(U24_LE),
	FORMAT(U24_BE),
	FORMAT(S32_LE),
	FORMAT(S32_BE),
	FORMAT(U32_LE),
	FORMAT(U32_BE),
	FORMAT(FLOAT_LE),
	FORMAT(FLOAT_BE),
	FORMAT(FLOAT64_LE),
	FORMAT(FLOAT64_BE),
	FORMAT(IEC958_SUBFRAME_LE),
	FORMAT(IEC958_SUBFRAME_BE),
	FORMAT(MU_LAW),
	FORMAT(A_LAW),
	FORMAT(IMA_ADPCM),
	FORMAT(MPEG),
	FORMAT(GSM),
	FORMAT(SPECIAL),
	FORMAT(S24_3LE),
	FORMAT(S24_3BE),
	FORMAT(U24_3LE),
	FORMAT(U24_3BE),
	FORMAT(S20_3LE),
	FORMAT(S20_3BE),
	FORMAT(U20_3LE),
	FORMAT(U20_3BE),
	FORMAT(S18_3LE),
	FORMAT(S18_3BE),
	FORMAT(U18_3LE),
	FORMAT(U18_3BE),
	FORMAT(G723_24),
	FORMAT(G723_24_1B),
	FORMAT(G723_40),
	FORMAT(G723_40_1B),
	FORMAT(DSD_U8),
	FORMAT(DSD_U16_LE),
	FORMAT(DSD_U32_LE),
	FORMAT(DSD_U16_BE),
	FORMAT(DSD_U32_BE),
};

static const char * const _snd_pcm_format_aliases[SND_PCM_FORMAT_LAST + 1] = {
	FORMAT(S16),
	FORMAT(U16),
	FORMAT(S24),
	FORMAT(U24),
	FORMAT(S32),
	FORMAT(U32),
	FORMAT(FLOAT),
	FORMAT(FLOAT64),
	FORMAT(IEC958_SUBFRAME),
};

const char * const _snd_pcm_format_descriptions[SND_PCM_FORMAT_LAST + 1] = {
	FORMATD(S8, "Signed 8 bit"), 
	FORMATD(U8, "Unsigned 8 bit"),
	FORMATD(S16_LE, "Signed 16 bit Little Endian"),
	FORMATD(S16_BE, "Signed 16 bit Big Endian"),
	FORMATD(U16_LE, "Unsigned 16 bit Little Endian"),
	FORMATD(U16_BE, "Unsigned 16 bit Big Endian"),
	FORMATD(S24_LE, "Signed 24 bit Little Endian"),
	FORMATD(S24_BE, "Signed 24 bit Big Endian"),
	FORMATD(U24_LE, "Unsigned 24 bit Little Endian"),
	FORMATD(U24_BE, "Unsigned 24 bit Big Endian"),
	FORMATD(S32_LE, "Signed 32 bit Little Endian"),
	FORMATD(S32_BE, "Signed 32 bit Big Endian"),
	FORMATD(U32_LE, "Unsigned 32 bit Little Endian"),
	FORMATD(U32_BE, "Unsigned 32 bit Big Endian"),
	FORMATD(FLOAT_LE, "Float 32 bit Little Endian"),
	FORMATD(FLOAT_BE, "Float 32 bit Big Endian"),
	FORMATD(FLOAT64_LE, "Float 64 bit Little Endian"),
	FORMATD(FLOAT64_BE, "Float 64 bit Big Endian"),
	FORMATD(IEC958_SUBFRAME_LE, "IEC-958 Little Endian"),
	FORMATD(IEC958_SUBFRAME_BE, "IEC-958 Big Endian"),
	FORMATD(MU_LAW, "Mu-Law"),
	FORMATD(A_LAW, "A-Law"),
	FORMATD(IMA_ADPCM, "Ima-ADPCM"),
	FORMATD(MPEG, "MPEG"),
	FORMATD(GSM, "GSM"),
	FORMATD(SPECIAL, "Special"),
	FORMATD(S24_3LE, "Signed 24 bit Little Endian in 3bytes"),
	FORMATD(S24_3BE, "Signed 24 bit Big Endian in 3bytes"),
	FORMATD(U24_3LE, "Unsigned 24 bit Little Endian in 3bytes"),
	FORMATD(U24_3BE, "Unsigned 24 bit Big Endian in 3bytes"),
	FORMATD(S20_3LE, "Signed 20 bit Little Endian in 3bytes"),
	FORMATD(S20_3BE, "Signed 20 bit Big Endian in 3bytes"),
	FORMATD(U20_3LE, "Unsigned 20 bit Little Endian in 3bytes"),
	FORMATD(U20_3BE, "Unsigned 20 bit Big Endian in 3bytes"),
	FORMATD(S18_3LE, "Signed 18 bit Little Endian in 3bytes"),
	FORMATD(S18_3BE, "Signed 18 bit Big Endian in 3bytes"),
	FORMATD(U18_3LE, "Unsigned 18 bit Little Endian in 3bytes"),
	FORMATD(U18_3BE, "Unsigned 18 bit Big Endian in 3bytes"),
	FORMATD(G723_24, "G.723 (ADPCM) 24 kbit/s, 8 samples in 3 bytes"),
	FORMATD(G723_24_1B, "G.723 (ADPCM) 24 kbit/s, 1 sample in 1 byte"),
	FORMATD(G723_40, "G.723 (ADPCM) 40 kbit/s, 8 samples in 3 bytes"),
	FORMATD(G723_40_1B, "G.723 (ADPCM) 40 kbit/s, 1 sample in 1 byte"),
	FORMATD(DSD_U8, "Direct Stream Digital (DSD) in 1-byte samples (x8)"),
	FORMATD(DSD_U16_LE, "Direct Stream Digital (DSD) in 2-byte samples (x16)"),
	FORMATD(DSD_U32_LE, "Direct Stream Digital (DSD) in 4-byte samples (x32)"),
	FORMATD(DSD_U16_BE, "Direct Stream Digital (DSD) in 2-byte samples (x16)"),
	FORMATD(DSD_U32_BE, "Direct Stream Digital (DSD) in 4-byte samples (x32)"),
};

const char * const _snd_pcm_type_names[] = {
	PCMTYPE(HW), 
	PCMTYPE(HOOKS), 
	PCMTYPE(MULTI), 
	PCMTYPE(FILE), 
	PCMTYPE(NULL), 
	PCMTYPE(SHM), 
	PCMTYPE(INET), 
	PCMTYPE(COPY), 
	PCMTYPE(LINEAR), 
	PCMTYPE(ALAW), 
	PCMTYPE(MULAW), 
	PCMTYPE(ADPCM), 
	PCMTYPE(RATE), 
	PCMTYPE(ROUTE), 
	PCMTYPE(PLUG), 
	PCMTYPE(SHARE), 
	PCMTYPE(METER), 
	PCMTYPE(MIX), 
	PCMTYPE(DROUTE), 
	PCMTYPE(LBSERVER), 
	PCMTYPE(LINEAR_FLOAT), 
	PCMTYPE(LADSPA), 
	PCMTYPE(DMIX), 
	PCMTYPE(JACK),
        PCMTYPE(DSNOOP),
        PCMTYPE(IEC958),
	PCMTYPE(SOFTVOL),
        PCMTYPE(IOPLUG),
        PCMTYPE(EXTPLUG),
};

const char * const _snd_pcm_subformat_names[SND_PCM_SUBFORMAT_LAST + 1] = {
	SUBFORMAT(STD), 
};

const char * const _snd_pcm_subformat_descriptions[SND_PCM_SUBFORMAT_LAST + 1] = {
	SUBFORMATD(STD, "Standard"), 
};

const char * const _snd_pcm_tstamp_mode_names[SND_PCM_TSTAMP_LAST + 1] = {
	TSTAMP(NONE),
	TSTAMP(MMAP),
};

snd_pcm_format_t snd_pcm_format_value(const char* name)
{
	snd_pcm_format_t format;
	for (format = 0; format <= SND_PCM_FORMAT_LAST; format++) {
		if (_snd_pcm_format_names[format] &&
		    strcasecmp(name, _snd_pcm_format_names[format]) == 0) {
			return format;
		}
		if (_snd_pcm_format_aliases[format] &&
		    strcasecmp(name, _snd_pcm_format_aliases[format]) == 0) {
			return format;
		}
	}
	for (format = 0; format <= SND_PCM_FORMAT_LAST; format++) {
		if (_snd_pcm_format_descriptions[format] &&
		    strcasecmp(name, _snd_pcm_format_descriptions[format]) == 0) {
			return format;
		}
	}
	return SND_PCM_FORMAT_UNKNOWN;
}


int snd_pcm_dump_hw_setup(snd_pcm_t *pcm, snd_output_t *out)
{
	snd_output_printf(out, "  stream       : %s\n",
			  snd_pcm_stream_name(pcm->stream));
	snd_output_printf(out, "  access       : %s\n",
			  snd_pcm_access_name(pcm->_access));
	snd_output_printf(out, "  format       : %s\n",
			  snd_pcm_format_name(pcm->format));
	snd_output_printf(out, "  subformat    : %s\n",
			  snd_pcm_subformat_name(pcm->subformat));
	snd_output_printf(out, "  channels     : %u\n", pcm->channels);
	snd_output_printf(out, "  rate         : %u\n", pcm->rate);
	snd_output_printf(out, "  exact rate   : %g (%u/%u)\n",
			  (pcm->hw_params.rate_den ?
			   ((double) pcm->hw_params.rate_num / pcm->hw_params.rate_den) : 0.0),
			  pcm->hw_params.rate_num, pcm->hw_params.rate_den);
	snd_output_printf(out, "  msbits       : %u\n", pcm->hw_params.msbits);
	snd_output_printf(out, "  buffer_size  : %lu\n", pcm->buffer_size);
	snd_output_printf(out, "  period_size  : %lu\n", pcm->period_size);
	snd_output_printf(out, "  period_time  : %u\n", pcm->period_time);
#if 0 /* deprecated */
	snd_output_printf(out, "  tick_time    : %u\n", pcm->tick_time);
#endif
	return 0;
}

int snd_pcm_dump_sw_setup(snd_pcm_t *pcm, snd_output_t *out)
{
	snd_output_printf(out, "  tstamp_mode  : %s\n",
			  snd_pcm_tstamp_mode_name(pcm->sw_params.tstamp_mode));
	snd_output_printf(out, "  period_step  : %d\n",
			  pcm->sw_params.period_step);
#if 0 /* deprecated */
	snd_output_printf(out, "  sleep_min    : %d\n",
			  pcm->sw_params.sleep_min);
#endif
	snd_output_printf(out, "  avail_min    : %ld\n",
			  pcm->sw_params.avail_min);
#if 0 /* deprecated */
	snd_output_printf(out, "  xfer_align   : %ld\n",
			  pcm->sw_params.xfer_align);
#endif
	snd_output_printf(out, "  start_threshold  : %ld\n",
			  pcm->sw_params.start_threshold);
	snd_output_printf(out, "  stop_threshold   : %ld\n",
			  pcm->sw_params.stop_threshold);
	snd_output_printf(out, "  silence_threshold: %ld\n",
			  pcm->sw_params.silence_threshold);
	snd_output_printf(out, "  silence_size : %ld\n",
			  pcm->sw_params.silence_size);
	snd_output_printf(out, "  boundary     : %ld\n",
			  pcm->sw_params.boundary);
	return 0;
}

int snd_pcm_dump_setup(snd_pcm_t *pcm, snd_output_t *out)
{
	snd_pcm_dump_hw_setup(pcm, out);
	snd_pcm_dump_sw_setup(pcm, out);
	return 0;
}

int snd_pcm_status_dump(snd_pcm_status_t *status, snd_output_t *out)
{
	snd_output_printf(out, "  state       : %s\n",
			  snd_pcm_state_name((snd_pcm_state_t) status->state));
	snd_output_printf(out, "  trigger_time: %ld.%06ld\n",
		status->trigger_tstamp.tv_sec, status->trigger_tstamp.tv_nsec);
	snd_output_printf(out, "  tstamp      : %ld.%06ld\n",
		status->tstamp.tv_sec, status->tstamp.tv_nsec);
	snd_output_printf(out, "  delay       : %ld\n", (long)status->delay);
	snd_output_printf(out, "  avail       : %ld\n", (long)status->avail);
	snd_output_printf(out, "  avail_max   : %ld\n", (long)status->avail_max);
	return 0;
}

int snd_pcm_dump(snd_pcm_t *pcm, snd_output_t *out)
{
	char *name;
	int err = snd_card_get_name(pcm->card, &name);
	if (err < 0)
		return err;
	snd_output_printf(out,
			  "Hardware PCM card %d '%s' device %d subdevice %d\n",
			  pcm->card, name, pcm->device, pcm->subdevice);
	if (pcm->setup) {
		snd_output_printf(out, "Its setup is:\n");
		snd_pcm_dump_setup(pcm, out);
	}
	free(name);
	return 0;
}


/*
 * SILENCE AND COPY AREAS
 */

static inline
void *snd_pcm_channel_area_addr(const snd_pcm_channel_area_t *area,
				snd_pcm_uframes_t offset)
{
	unsigned int bitofs = area->first + area->step * offset;
	return (char *) area->addr + bitofs / 8;
}

#if SALSA_SUPPORT_4BIT_PCM
static int area_silence_4bit(const snd_pcm_channel_area_t *dst_area,
			     snd_pcm_uframes_t dst_offset,
			     unsigned int samples, snd_pcm_format_t format)
{
	char *dst = snd_pcm_channel_area_addr(dst_area, dst_offset);
	int dst_step = dst_area->step / 8;
	int dstbit = dst_area->first % 8;
	int dstbit_step = dst_area->step % 8;
	while (samples-- > 0) {
		if (dstbit)
			*dst &= 0xf0;
		else
			*dst &= 0x0f;
		dst += dst_step;
		dstbit += dstbit_step;
		if (dstbit == 8) {
			dst++;
			dstbit = 0;
		}
	}
	return 0;
}
#else
static int area_silence_4bit(const snd_pcm_channel_area_t *dst_area,
			     snd_pcm_uframes_t dst_offset,
			     unsigned int samples, snd_pcm_format_t format)
{
	return -EINVAL;
}
#endif /* SALSA_SUPPORT_4BIT_PCM */

int snd_pcm_area_silence(const snd_pcm_channel_area_t *dst_area,
			 snd_pcm_uframes_t dst_offset,
			 unsigned int samples, snd_pcm_format_t format)
{
	char *dst;
	unsigned int dst_step;
	int width;

	if (!dst_area->addr)
		return 0;
	width = snd_pcm_format_physical_width(format);
	if (width < 8)
		return area_silence_4bit(dst_area, dst_offset, samples, format);
	dst = snd_pcm_channel_area_addr(dst_area, dst_offset);
	if (dst_area->step == (unsigned int) width) {
		snd_pcm_format_set_silence(format, dst, samples);
		return 0;
	}
	dst_step = dst_area->step / 8;
	while (samples-- > 0) {
		snd_pcm_format_set_silence(format, dst, 1);
		dst += dst_step;
	}
	return 0;
}

int snd_pcm_areas_silence(const snd_pcm_channel_area_t *dst_areas,
			  snd_pcm_uframes_t dst_offset,
			  unsigned int channels, snd_pcm_uframes_t frames,
			  snd_pcm_format_t format)
{
	while (channels > 0) {
		snd_pcm_area_silence(dst_areas, dst_offset, frames, format);
		dst_areas++;
		channels--;
	}
	return 0;
}

#if SALSA_SUPPORT_4BIT_PCM
static int area_copy_4bit(const snd_pcm_channel_area_t *dst_area,
			  snd_pcm_uframes_t dst_offset,
			  const snd_pcm_channel_area_t *src_area,
			  snd_pcm_uframes_t src_offset,
			  unsigned int samples, snd_pcm_format_t format)
{
	int srcbit = src_area->first % 8;
	int srcbit_step = src_area->step % 8;
	int dstbit = dst_area->first % 8;
	int dstbit_step = dst_area->step % 8;
	const unsigned char *src = snd_pcm_channel_area_addr(src_area,
							     src_offset);
	unsigned char *dst = snd_pcm_channel_area_addr(dst_area, dst_offset);
	int src_step = src_area->step / 8;
	int dst_step = dst_area->step / 8;
	
	while (samples-- > 0) {
		unsigned char srcval;
		if (srcbit)
			srcval = *src & 0x0f;
		else
			srcval = *src & 0xf0;
		if (dstbit)
			*dst &= 0xf0;
		else
			*dst &= 0x0f;
		*dst |= srcval;
		src += src_step;
		srcbit += srcbit_step;
		if (srcbit == 8) {
			src++;
			srcbit = 0;
		}
		dst += dst_step;
		dstbit += dstbit_step;
		if (dstbit == 8) {
			dst++;
			dstbit = 0;
		}
	}
	return 0;
}
#else
static int area_copy_4bit(const snd_pcm_channel_area_t *dst_area,
			  snd_pcm_uframes_t dst_offset,
			  const snd_pcm_channel_area_t *src_area,
			  snd_pcm_uframes_t src_offset,
			  unsigned int samples, snd_pcm_format_t format)
{
	return -EINVAL;
}
#endif /* SALSA_SUPPORT_4BIT_PCM */

int snd_pcm_area_copy(const snd_pcm_channel_area_t *dst_area,
		      snd_pcm_uframes_t dst_offset,
		      const snd_pcm_channel_area_t *src_area,
		      snd_pcm_uframes_t src_offset,
		      unsigned int samples, snd_pcm_format_t format)
{
	const char *src;
	char *dst;
	int width;
	int src_step, dst_step;

	if (!dst_area->addr)
		return 0;
	if (dst_area == src_area && dst_offset == src_offset)
		return 0;
	if (!src_area->addr)
		return snd_pcm_area_silence(dst_area, dst_offset, samples,
					    format);
	width = snd_pcm_format_physical_width(format);
	if (width < 8)
		return area_copy_4bit(dst_area, dst_offset,
				      src_area, src_offset,
				      samples, format);
	src = snd_pcm_channel_area_addr(src_area, src_offset);
	dst = snd_pcm_channel_area_addr(dst_area, dst_offset);
	if (src_area->step == (unsigned int) width &&
	    dst_area->step == (unsigned int) width) {
		memcpy(dst, src, samples * width / 8);
		return 0;
	}
	src_step = src_area->step / 8;
	dst_step = dst_area->step / 8;
	width /= 8;
	while (samples-- > 0) {
		memcpy(dst, src, width);
		src += src_step;
		dst += dst_step;
	}
	return 0;
}

int snd_pcm_areas_copy(const snd_pcm_channel_area_t *dst_areas,
		       snd_pcm_uframes_t dst_offset,
		       const snd_pcm_channel_area_t *src_areas,
		       snd_pcm_uframes_t src_offset,
		       unsigned int channels, snd_pcm_uframes_t frames,
		       snd_pcm_format_t format)
{
	if (!channels)
		return -EINVAL;
	if (!frames)
		return -EINVAL;
	while (channels > 0) {
		snd_pcm_area_copy(dst_areas, dst_offset,
				  src_areas, src_offset,
				  frames, format);
		src_areas++;
		dst_areas++;
		channels--;
	}
	return 0;
}


/*
 * MMAP
 */

static size_t page_align(size_t size)
{
	size_t r;
	long psz = sysconf(_SC_PAGE_SIZE);
	r = size % psz;
	if (r)
		return size + psz - r;
	return size;
}

int _snd_pcm_mmap(snd_pcm_t *pcm)
{
	unsigned int c;

	/* clear first */
	_snd_pcm_munmap(pcm);

	pcm->mmap_channels = calloc(pcm->channels, sizeof(*pcm->mmap_channels));
	if (!pcm->mmap_channels)
		return -ENOMEM;
	pcm->running_areas = calloc(pcm->channels, sizeof(*pcm->running_areas));
	if (!pcm->running_areas) {
		free(pcm->mmap_channels);
		pcm->mmap_channels = NULL;
		return -ENOMEM;
	}
	for (c = 0; c < pcm->channels; ++c) {
		snd_pcm_channel_info_t *i = &pcm->mmap_channels[c];
		i->info.channel = c;
		if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_CHANNEL_INFO, &i->info) < 0)
			return -errno;
	}
	for (c = 0; c < pcm->channels; ++c) {
		snd_pcm_channel_info_t *i = &pcm->mmap_channels[c];
		snd_pcm_channel_area_t *a = &pcm->running_areas[c];
		char *ptr;
		size_t size;
		unsigned int c1;
		if (i->addr)
			goto copy;
                size = i->info.first + i->info.step * (pcm->buffer_size - 1) +
			pcm->sample_bits;
		for (c1 = c + 1; c1 < pcm->channels; ++c1) {
			snd_pcm_channel_info_t *i1 = &pcm->mmap_channels[c1];
			size_t s;
			if (i1->info.offset != i->info.offset)
				continue;
			s = i1->info.first + i1->info.step *
				(pcm->buffer_size - 1) + pcm->sample_bits;
			if (s > size)
				size = s;
		}
		size = (size + 7) / 8;
		size = page_align(size);
		ptr = mmap(NULL, size, PROT_READ|PROT_WRITE,
			   MAP_FILE|MAP_SHARED, pcm->fd, i->info.offset);
		if (ptr == MAP_FAILED)
			return -errno;
		i->addr = ptr;
		for (c1 = c + 1; c1 < pcm->channels; ++c1) {
			snd_pcm_channel_info_t *i1 = &pcm->mmap_channels[c1];
			if (i1->info.offset != i->info.offset)
				continue;
			i1->addr = i->addr;
		}
	copy:
		a->addr = i->addr;
		a->first = i->info.first;
		a->step = i->info.step;
	}
	return 0;
}

int _snd_pcm_munmap(snd_pcm_t *pcm)
{
	int err;
	unsigned int c;

	if (!pcm->mmap_channels)
		return 0;

	for (c = 0; c < pcm->channels; ++c) {
		snd_pcm_channel_info_t *i = &pcm->mmap_channels[c];
		unsigned int c1;
		size_t size;
		if (!i->addr)
			continue;
		size = i->info.first + i->info.step *
			(pcm->buffer_size - 1) + pcm->sample_bits;
		for (c1 = c + 1; c1 < pcm->channels; ++c1) {
			snd_pcm_channel_info_t *i1 = &pcm->mmap_channels[c1];
			size_t s;
			if (i1->addr != i->addr)
				continue;
			i1->addr = NULL;
			s = i1->info.first + i1->info.step *
				(pcm->buffer_size - 1) + pcm->sample_bits;
			if (s > size)
				size = s;
		}
		size = (size + 7) / 8;
		size = page_align(size);
		err = munmap(i->addr, size);
		if (err < 0)
			return -errno;
		i->addr = NULL;
	}
	free(pcm->mmap_channels);
	free(pcm->running_areas);
	pcm->mmap_channels = NULL;
	pcm->running_areas = NULL;
	return 0;
}

static int snd_pcm_hw_mmap_status(snd_pcm_t *pcm)
{
	if (pcm->sync_ptr)
		return 0;

	pcm->mmap_status =
		mmap(NULL, page_align(sizeof(struct snd_pcm_mmap_status)),
		     PROT_READ, MAP_FILE|MAP_SHARED, 
		     pcm->fd, SNDRV_PCM_MMAP_OFFSET_STATUS);
	if (pcm->mmap_status == MAP_FAILED)
		pcm->mmap_status = NULL;
	if (!pcm->mmap_status)
		goto no_mmap;
	pcm->mmap_control =
		mmap(NULL, page_align(sizeof(struct snd_pcm_mmap_control)),
		     PROT_READ|PROT_WRITE, MAP_FILE|MAP_SHARED, 
		     pcm->fd, SNDRV_PCM_MMAP_OFFSET_CONTROL);
	if (pcm->mmap_control == MAP_FAILED)
		pcm->mmap_control = NULL;
	if (!pcm->mmap_control) {
		munmap(pcm->mmap_status,
		       page_align(sizeof(struct snd_pcm_mmap_status)));
		pcm->mmap_status = NULL;
		goto no_mmap;
	}
	pcm->mmap_control->avail_min = 1;
	return 0;

 no_mmap:
	pcm->sync_ptr = calloc(1, sizeof(*pcm->sync_ptr));
	if (!pcm->sync_ptr)
		return -ENOMEM;
	pcm->mmap_status = &pcm->sync_ptr->s.status;
	pcm->mmap_control = &pcm->sync_ptr->c.control;
	pcm->mmap_control->avail_min = 1;
	_snd_pcm_sync_ptr(pcm, 0);
	return 0;
}

static void snd_pcm_hw_munmap_status(snd_pcm_t *pcm)
{
	if (pcm->sync_ptr) {
		free(pcm->sync_ptr);
		pcm->sync_ptr = NULL;
	} else {
		munmap(pcm->mmap_status,
		       page_align(sizeof(struct snd_pcm_mmap_status)));
		munmap(pcm->mmap_control,
		       page_align(sizeof(struct snd_pcm_mmap_control)));
	}
	pcm->mmap_status = NULL;
	pcm->mmap_control = NULL;
}

#define get_hw_ptr(pcm)		((pcm)->mmap_status->hw_ptr)
#define get_appl_ptr(pcm)	((pcm)->mmap_control->appl_ptr)

static snd_pcm_uframes_t snd_pcm_mmap_playback_avail(snd_pcm_t *pcm)
{
	snd_pcm_sframes_t avail;
	avail = get_hw_ptr(pcm) + pcm->buffer_size - get_appl_ptr(pcm);
	if (avail < 0)
		avail += pcm->boundary;
	else if ((snd_pcm_uframes_t) avail >= pcm->boundary)
		avail -= pcm->boundary;
	return avail;
}

static snd_pcm_uframes_t snd_pcm_mmap_capture_avail(snd_pcm_t *pcm)
{
	snd_pcm_sframes_t avail;
	avail = get_hw_ptr(pcm) - get_appl_ptr(pcm);
	if (avail < 0)
		avail += pcm->boundary;
	return avail;
}

static snd_pcm_uframes_t snd_pcm_mmap_avail(snd_pcm_t *pcm)
{
	if (pcm->stream == SND_PCM_STREAM_PLAYBACK)
		return snd_pcm_mmap_playback_avail(pcm);
	else
		return snd_pcm_mmap_capture_avail(pcm);
}

snd_pcm_sframes_t snd_pcm_hw_forwardable(snd_pcm_t *pcm)
	__attribute__ ((alias("snd_pcm_mmap_avail")));

snd_pcm_sframes_t snd_pcm_avail_update(snd_pcm_t *pcm)
{
	snd_pcm_uframes_t avail;

	_snd_pcm_sync_ptr(pcm, 0);
	avail = snd_pcm_mmap_avail(pcm);
	switch (snd_pcm_state(pcm)) {
	case SND_PCM_STATE_RUNNING:
		if (avail >= pcm->sw_params.stop_threshold) {
			if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_XRUN) < 0)
				return -errno;
			/* everything is ok,
			 * state == SND_PCM_STATE_XRUN at the moment
			 */
			return -EPIPE;
		}
		break;
	case SND_PCM_STATE_XRUN:
		return -EPIPE;
	default:
		break;
	}
	return avail;
}

int snd_pcm_avail_delay(snd_pcm_t *pcm,
			snd_pcm_sframes_t *availp,
			snd_pcm_sframes_t *delayp)
{
	int err;
	snd_pcm_sframes_t val;

	err = snd_pcm_delay(pcm, delayp);
	if (err < 0)
		return err;
	val = snd_pcm_avail_update(pcm);
	if (val < 0)
		return (int)val;
	*availp = val;
	return 0;
}

int snd_pcm_mmap_begin(snd_pcm_t *pcm,
		       const snd_pcm_channel_area_t **areas,
		       snd_pcm_uframes_t *offset,
		       snd_pcm_uframes_t *frames)
{
	snd_pcm_uframes_t cont;
	snd_pcm_uframes_t f;
	snd_pcm_uframes_t avail;

	*areas = pcm->running_areas;
	*offset = pcm->mmap_control->appl_ptr % pcm->buffer_size;
	avail = snd_pcm_mmap_avail(pcm);
	if (avail > pcm->buffer_size)
		avail = pcm->buffer_size;
	cont = pcm->buffer_size - *offset;
	f = *frames;
	if (f > avail)
		f = avail;
	if (f > cont)
		f = cont;
	*frames = f;
	return 0;
}

snd_pcm_sframes_t snd_pcm_mmap_commit(snd_pcm_t *pcm,
				      snd_pcm_uframes_t offset,
				      snd_pcm_uframes_t frames)
{
	snd_pcm_uframes_t appl_ptr = pcm->mmap_control->appl_ptr;
	appl_ptr += frames;
	if (appl_ptr >= pcm->boundary)
		appl_ptr -= pcm->boundary;
	pcm->mmap_control->appl_ptr = appl_ptr;
	_snd_pcm_sync_ptr(pcm, 0);
	return frames;
}


#if SALSA_HAS_ASYNC_SUPPORT
/*
 * async handler
 */
int snd_async_add_pcm_handler(snd_async_handler_t **handlerp, snd_pcm_t *pcm, 
			      snd_async_callback_t callback,
			      void *private_data)
{
	int err;

	if (pcm->async)
		return -EBUSY;
	err = snd_async_add_handler(&pcm->async, pcm->fd,
				    callback, private_data);
	if (err < 0)
		return err;
	pcm->async->rec = pcm;
	pcm->async->pointer = &pcm->async;
	return 0;
}
#endif /* SALSA_HAS_ASYNC_SUPPORT */

/*
 * HELPERS
 */

int snd_pcm_wait(snd_pcm_t *pcm, int timeout)
{
	struct pollfd pfd;
	int err;
	
#if 0 /* FIXME: NEEDED? */
	_snd_pcm_sync_ptr(pcm, SNDRV_PCM_SYNC_PTR_APPL);
	if (snd_pcm_mmap_avail(pcm) >= pcm->sw_params.avail_min)
		return correct_pcm_error(pcm, 1);
#endif

	pfd = pcm->pollfd;
	for (;;) {
		err = poll(&pfd, 1, timeout);
		if (err < 0) {
			if (errno == EINTR)
				continue;
			return -errno;
                }
		if (!err)
			return 0;
		if (pfd.revents & (POLLERR | POLLNVAL))
			return correct_pcm_error(pcm, -EIO);
		if (pfd.revents & (POLLIN | POLLOUT))
			return 1;
	}
}

int snd_pcm_recover(snd_pcm_t *pcm, int err, int silent)
{
	if (err > 0)
		err = -err;
	switch (err) {
	case -EINTR:
		return 0;
	case -EPIPE:
		return snd_pcm_prepare(pcm);
	case -ESTRPIPE:
		while ((err = snd_pcm_resume(pcm)) == -EAGAIN)
			sleep(1);
		if (err < 0)
			return snd_pcm_prepare(pcm);
		return 0;
        }
        return err;
}

#if SALSA_HAS_CHMAP_SUPPORT
/*
 * channel mapping API
 */

static int do_ctl_chmap(int card, int dev, int subdev, int stream,
			int (*func)(snd_ctl_t *, snd_ctl_elem_id_t *, void *),
			void *arg)
{
	char ctlname[32];
	snd_ctl_t *ctl;
	snd_ctl_elem_id_t id = {
		.iface = SND_CTL_ELEM_IFACE_PCM,
		.device = dev,
		.index = subdev,
	};
	int ret;

	sprintf(ctlname, "hw:%d", card);
	ret = snd_ctl_open(&ctl, ctlname, 0);
	if (ret < 0)
		return ret;

	if (stream == SND_PCM_STREAM_PLAYBACK)
		strcpy((char *)id.name, "Playback Channel Map");
	else
		strcpy((char *)id.name, "Capture Channel Map");
	ret = func(ctl, &id, arg);
	snd_ctl_close(ctl);
	if (ret < 0)
		return ret;
	return 0;
}

#define TLV_SIZE	256

static int do_read_tlv(snd_ctl_t *ctl, snd_ctl_elem_id_t *id, void *tlv)
{
	return snd_ctl_elem_tlv_read(ctl, id, tlv, TLV_SIZE * 4);
}

static inline int is_chmap_type(int type)
{
	return (type >= SND_CTL_TLVT_CHMAP_FIXED &&
		type <= SND_CTL_TLVT_CHMAP_PAIRED);
}

snd_pcm_chmap_query_t **
snd_pcm_query_chmaps_from_hw(int card, int dev, int subdev,
			     snd_pcm_stream_t stream)
{
	unsigned int tlv[TLV_SIZE], *start;
	snd_pcm_chmap_query_t **map;
	int i, nums;

	if (do_ctl_chmap(card, dev, subdev, stream, do_read_tlv, tlv) < 0)
		return NULL;

	if (tlv[0] != SND_CTL_TLVT_CONTAINER) {
		if (!is_chmap_type(tlv[0]))
			return NULL;
		start = tlv;
		nums = 1;
	} else {
		unsigned int *p;
		int size;
		start = tlv + 2;
		size = tlv[1];
		nums = 0;
		for (p = start; size > 0; ) {
			if (!is_chmap_type(p[0]))
				return NULL;
			nums++;
			size -= p[1] + 8;
			p += p[1] / 4 + 2;
		}
	}
	map = calloc(nums + 1, sizeof(int *));
	if (!map)
		return NULL;
	for (i = 0; i < nums; i++) {
		map[i] = malloc(start[1] + 8);
		if (!map[i]) {
			snd_pcm_free_chmaps(map);
			return NULL;
		}
		map[i]->type = start[0] - 0x100;
		map[i]->map.channels = start[1] / 4;
		memcpy(map[i]->map.pos, start + 2, start[1]);
		start += start[1] / 4 + 2;
	}
	return map;
}

void snd_pcm_free_chmaps(snd_pcm_chmap_query_t **maps)
{
	snd_pcm_chmap_query_t **p = maps;
	if (!maps)
		return;
	for (p = maps; *p; p++)
		free(*p);
	free(maps);
}

static int do_get_chmap(snd_ctl_t *ctl, snd_ctl_elem_id_t *id, void *arg)
{
	snd_ctl_elem_value_t val = { .id = *id };
	snd_pcm_chmap_t *map = arg;
	int i, ret;

	ret = snd_ctl_elem_read(ctl, &val);
	if (ret < 0)
		return ret;

	for (i = 0; i < map->channels; i++)
		map->pos[i] = snd_ctl_elem_value_get_integer(&val, i);
	return 0;
}

snd_pcm_chmap_t *snd_pcm_get_chmap(snd_pcm_t *pcm)
{
	snd_pcm_chmap_t *map;

	map = malloc(pcm->channels * sizeof(map->pos[0]) + sizeof(*map));
	if (!map)
		return NULL;
	map->channels = pcm->channels;

	if (do_ctl_chmap(pcm->card, pcm->device, pcm->subdevice, pcm->stream,
			 do_get_chmap, map) < 0) {
		free(map);
		return NULL;
	}
	return map;
}

static int do_set_chmap(snd_ctl_t *ctl, snd_ctl_elem_id_t *id, void *arg)
{
	snd_ctl_elem_value_t val = { .id = *id };
	snd_pcm_chmap_t *map = arg;
	int i;

	for (i = 0; i < map->channels; i++)
		snd_ctl_elem_value_set_integer(&val, i, map->pos[i]);
	return snd_ctl_elem_write(ctl, &val);
}

int snd_pcm_set_chmap(snd_pcm_t *pcm, const snd_pcm_chmap_t *map)
{
	int ret;

	if (map->channels > 128)
		return -EINVAL;

	ret = do_ctl_chmap(pcm->card, pcm->device, pcm->subdevice, pcm->stream,
			   do_set_chmap, (void *)map);
	if (ret == -ENOENT || ret == -EPERM )
		ret = -ENXIO;
	return ret;
}

#define _NAME(n) [SND_CHMAP_TYPE_##n] = #n
const char *_snd_chmap_type_names[SND_CHMAP_TYPE_LAST + 1] = {
	_NAME(NONE), _NAME(FIXED), _NAME(VAR), _NAME(PAIRED),
};
#undef _NAME

#define _NAME(n) [SND_CHMAP_##n] = #n
const char *_snd_chmap_names[SND_CHMAP_LAST + 1] = {
	_NAME(UNKNOWN), _NAME(NA), _NAME(MONO),
	_NAME(FL), _NAME(FR),
	_NAME(RL), _NAME(RR),
	_NAME(FC), _NAME(LFE),
	_NAME(SL), _NAME(SR),
	_NAME(RC), _NAME(FLC), _NAME(FRC), _NAME(RLC), _NAME(RRC),
	_NAME(FLW), _NAME(FRW),
	_NAME(FLH), _NAME(FCH), _NAME(FRH), _NAME(TC),
	_NAME(TFL), _NAME(TFR), _NAME(TFC),
	_NAME(TRL), _NAME(TRR), _NAME(TRC),
	_NAME(TFLC), _NAME(TFRC), _NAME(TSL), _NAME(TSR),
	_NAME(LLFE), _NAME(RLFE),
	_NAME(BC), _NAME(BLC), _NAME(BRC),
};
#undef _NAME

const char *_snd_chmap_long_names[SND_CHMAP_LAST + 1] = {
	[SND_CHMAP_UNKNOWN] = "Unknown",
	[SND_CHMAP_NA] = "Unused",
	[SND_CHMAP_MONO] = "Mono",
	[SND_CHMAP_FL] = "Front Left",
	[SND_CHMAP_FR] = "Front Right",
	[SND_CHMAP_RL] = "Rear Left",
	[SND_CHMAP_RR] = "Rear Right",
	[SND_CHMAP_FC] = "Front Center",
	[SND_CHMAP_LFE] = "LFE",
	[SND_CHMAP_SL] = "Side Left",
	[SND_CHMAP_SR] = "Side Right",
	[SND_CHMAP_RC] = "Rear Center",
	[SND_CHMAP_FLC] = "Front Left Center",
	[SND_CHMAP_FRC] = "Front Right Center",
	[SND_CHMAP_RLC] = "Rear Left Center",
	[SND_CHMAP_RRC] = "Rear Right Center",
	[SND_CHMAP_FLW] = "Front Left Wide",
	[SND_CHMAP_FRW] = "Front Right Wide",
	[SND_CHMAP_FLH] = "Front Left High",
	[SND_CHMAP_FCH] = "Front Center High",
	[SND_CHMAP_FRH] = "Front Right High",
	[SND_CHMAP_TC] = "Top Center",
	[SND_CHMAP_TFL] = "Top Front Left",
	[SND_CHMAP_TFR] = "Top Front Right",
	[SND_CHMAP_TFC] = "Top Front Center",
	[SND_CHMAP_TRL] = "Top Rear Left",
	[SND_CHMAP_TRR] = "Top Rear Right",
	[SND_CHMAP_TRC] = "Top Rear Center",
	[SND_CHMAP_TFLC] = "Top Front Left Center",
	[SND_CHMAP_TFRC] = "Top Front Right Center",
	[SND_CHMAP_TSL] = "Top Side Left",
	[SND_CHMAP_TSR] = "Top Side Right",
	[SND_CHMAP_LLFE] = "Left LFE",
	[SND_CHMAP_RLFE] = "Right LFE",
	[SND_CHMAP_BC] = "Bottom Center",
	[SND_CHMAP_BLC] = "Bottom Left Center",
	[SND_CHMAP_BRC] = "Bottom Right Center",
};

int snd_pcm_chmap_print(const snd_pcm_chmap_t *map, size_t maxlen, char *buf)
{
	unsigned int i, len = 0;

	for (i = 0; i < map->channels; i++) {
		unsigned int p = map->pos[i] & SND_CHMAP_POSITION_MASK;
		if (i > 0) {
			len += snprintf(buf + len, maxlen - len, " ");
			if (len >= maxlen)
				return -ENOMEM;
		}
		if (map->pos[i] & SND_CHMAP_DRIVER_SPEC)
			len += snprintf(buf + len, maxlen - len, "%d", p);
		else {
			const char *name = _snd_chmap_names[p];
			if (name)
				len += snprintf(buf + len, maxlen - len,
						"%s", name);
			else
				len += snprintf(buf + len, maxlen - len,
						"Ch%d", p);
		}
		if (len >= maxlen)
			return -ENOMEM;
		if (map->pos[i] & SND_CHMAP_PHASE_INVERSE) {
			len += snprintf(buf + len, maxlen - len, "[INV]");
			if (len >= maxlen)
				return -ENOMEM;
		}
	}
	return len;
}

static int str_to_chmap(const char *str, int len)
{
	int val;
	unsigned long v;
	char *p;

	if (isdigit(*str)) {
		v = strtoul(str, &p, 0);
		if (v == (unsigned long)-1)
			return -1;
		val = v;
		val |= SND_CHMAP_DRIVER_SPEC;
		str = p;
	} else if (!strncasecmp(str, "ch", 2)) {
		v = strtoul(str + 2, &p, 0);
		if (v == (unsigned long)-1)
			return -1;
		val = v;
		str = p;
	} else {
		for (val = 0; val <= SND_CHMAP_LAST; val++) {
			int slen;
			slen = strlen(_snd_chmap_names[val]);
			if (slen > len)
				continue;
			if (!strncasecmp(str, _snd_chmap_names[val], slen) &&
			    !isalpha(str[slen])) {
				str += slen;
				break;
			}
		}
		if (val > SND_CHMAP_LAST)
			return -1;
	}
	if (str && !strncasecmp(str, "[INV]", 5))
		val |= SND_CHMAP_PHASE_INVERSE;
	return val;
}

unsigned int snd_pcm_chmap_from_string(const char *str)
{
	return str_to_chmap(str, strlen(str));
}

snd_pcm_chmap_t *snd_pcm_chmap_parse_string(const char *str)
{
	int i, ch = 0;
	int tmp_map[64];
	snd_pcm_chmap_t *map;

	for (;;) {
		const char *p;
		int len, val;

		if (ch >= (int)(sizeof(tmp_map) / sizeof(tmp_map[0])))
			return NULL;
		for (p = str; *p && isalnum(*p); p++)
			;
		len = p - str;
		if (!len)
			return NULL;
		val = str_to_chmap(str, len);
		if (val < 0)
			return NULL;
		str += len;
		if (*str == '[') {
			if (!strncmp(str, "[INV]", 5)) {
				val |= SND_CHMAP_PHASE_INVERSE;
				str += 5;
			}
		}
		tmp_map[ch] = val;
		ch++;
		for (; *str && !isalnum(*str); str++)
			;
		if (!*str)
			break;
	}
	map = malloc(sizeof(*map) + ch * sizeof(int));
	if (!map)
		return NULL;
	map->channels = ch;
	for (i = 0; i < ch; i++)
		map->pos[i] = tmp_map[i];
	return map;
}

#endif /* SALSA_HAS_CHMAP_SUPPORT */
