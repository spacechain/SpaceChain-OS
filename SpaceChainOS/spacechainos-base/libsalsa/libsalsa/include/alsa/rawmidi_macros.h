/*
 * SALSA-Lib - Raw MIDI interface
 *
 * rawmidi privates and macros
 */

#ifndef __ALSA_RAWMIDI_MACROS_H
#define __ALSA_RAWMIDI_MACROS_H

#include "asound.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/poll.h>

typedef struct _snd_rawmidi_hw {
	char *name;
	int fd;
	snd_rawmidi_type_t type;
	int opened;
} snd_rawmidi_hw_t;

struct _snd_rawmidi {
	int fd;
	snd_rawmidi_stream_t stream;
	int mode;
	struct pollfd pollfd;
	snd_rawmidi_params_t params;
	snd_rawmidi_hw_t *hw;
};

/*
 */

#if SALSA_CHECK_ABI
#define SALSA_RAWMIDI_MAGIC \
	((sizeof(struct _snd_rawmidi_hw) << 8) | sizeof(struct _snd_rawmidi))
__SALSA_EXPORT_FUNC
int snd_rawmidi_open(snd_rawmidi_t **in_rmidi, snd_rawmidi_t **out_rmidi,
		     const char *name, int mode)
{
	return _snd_rawmidi_open(in_rmidi, out_rmidi, name, mode,
				 SALSA_RAWMIDI_MAGIC);
}
#endif

__SALSA_EXPORT_FUNC __SALSA_NOT_IMPLEMENTED
int snd_rawmidi_open_lconf(snd_rawmidi_t **in_rmidi, snd_rawmidi_t **out_rmidi,
			   const char *name, int mode, snd_config_t *lconf)
{
	return -ENXIO;
}

__SALSA_EXPORT_FUNC
int snd_rawmidi_nonblock(snd_rawmidi_t *rmidi, int nonblock)
{
	return _snd_set_nonblock(rmidi->fd, nonblock);
}

__SALSA_EXPORT_FUNC
int snd_rawmidi_poll_descriptors_count(snd_rawmidi_t *rmidi)
{
	return 1;
}

__SALSA_EXPORT_FUNC
int snd_rawmidi_poll_descriptors(snd_rawmidi_t *rmidi, struct pollfd *pfds,
				 unsigned int space)
{
	*pfds = rmidi->pollfd;
	return 1;
}

__SALSA_EXPORT_FUNC
int snd_rawmidi_poll_descriptors_revents(snd_rawmidi_t *rawmidi,
					 struct pollfd *pfds,
					 unsigned int nfds,
					 unsigned short *revent)
{
	*revent = pfds->events;
	return 0;
}

__snd_define_type(snd_rawmidi_info);

__SALSA_EXPORT_FUNC
unsigned int snd_rawmidi_info_get_device(const snd_rawmidi_info_t *obj)
{
	return obj->device;
}

__SALSA_EXPORT_FUNC
unsigned int snd_rawmidi_info_get_subdevice(const snd_rawmidi_info_t *obj)
{
	return obj->subdevice;
}

__SALSA_EXPORT_FUNC
snd_rawmidi_stream_t snd_rawmidi_info_get_stream(const snd_rawmidi_info_t *obj)
{
	return (snd_rawmidi_stream_t) obj->stream;
}

__SALSA_EXPORT_FUNC
int snd_rawmidi_info_get_card(const snd_rawmidi_info_t *obj)
{
	return obj->card;
}

__SALSA_EXPORT_FUNC
unsigned int snd_rawmidi_info_get_flags(const snd_rawmidi_info_t *obj)
{
	return obj->flags;
}

__SALSA_EXPORT_FUNC
const char *snd_rawmidi_info_get_id(const snd_rawmidi_info_t *obj)
{
	return (const char *)obj->id;
}

__SALSA_EXPORT_FUNC
const char *snd_rawmidi_info_get_name(const snd_rawmidi_info_t *obj)
{
	return (const char *)obj->name;
}

__SALSA_EXPORT_FUNC
const char *snd_rawmidi_info_get_subdevice_name(const snd_rawmidi_info_t *obj)
{
	return (const char *)obj->subname;
}

__SALSA_EXPORT_FUNC
unsigned int snd_rawmidi_info_get_subdevices_count(const snd_rawmidi_info_t *obj)
{
	return obj->subdevices_count;
}

__SALSA_EXPORT_FUNC
unsigned int snd_rawmidi_info_get_subdevices_avail(const snd_rawmidi_info_t *obj)
{
	return obj->subdevices_avail;
}

__SALSA_EXPORT_FUNC
void snd_rawmidi_info_set_device(snd_rawmidi_info_t *obj, unsigned int val)
{
	obj->device = val;
}

__SALSA_EXPORT_FUNC
void snd_rawmidi_info_set_subdevice(snd_rawmidi_info_t *obj, unsigned int val)
{
	obj->subdevice = val;
}

__SALSA_EXPORT_FUNC
void snd_rawmidi_info_set_stream(snd_rawmidi_info_t *obj,
				 snd_rawmidi_stream_t val)
{
	obj->stream = val;
}

__SALSA_EXPORT_FUNC
int snd_rawmidi_info(snd_rawmidi_t *rmidi, snd_rawmidi_info_t * info)
{
	info->stream = rmidi->stream;
	if (ioctl(rmidi->fd, SNDRV_RAWMIDI_IOCTL_INFO, info) < 0)
		return -errno;
	return 0;
}

__snd_define_type(snd_rawmidi_params);

__SALSA_EXPORT_FUNC
int snd_rawmidi_params_set_buffer_size(snd_rawmidi_t *rmidi,
				       snd_rawmidi_params_t *params,
				       size_t val)
{
	params->buffer_size = val;
	return 0;
}

__SALSA_EXPORT_FUNC
size_t snd_rawmidi_params_get_buffer_size(const snd_rawmidi_params_t *params)
{
	return params->buffer_size;
}

__SALSA_EXPORT_FUNC
int snd_rawmidi_params_set_avail_min(snd_rawmidi_t *rmidi,
				     snd_rawmidi_params_t *params, size_t val)
{
	params->avail_min = val;
	return 0;
}

__SALSA_EXPORT_FUNC
size_t snd_rawmidi_params_get_avail_min(const snd_rawmidi_params_t *params)
{
	return params->avail_min;
}

__SALSA_EXPORT_FUNC
int snd_rawmidi_params_set_no_active_sensing(snd_rawmidi_t *rmidi,
					     snd_rawmidi_params_t *params,
					     int val)
{
	params->no_active_sensing = val;
	return 0;
}

__SALSA_EXPORT_FUNC
int snd_rawmidi_params_get_no_active_sensing(const snd_rawmidi_params_t *params)
{
	return params->no_active_sensing;
}

__SALSA_EXPORT_FUNC
int snd_rawmidi_params(snd_rawmidi_t *rmidi, snd_rawmidi_params_t * params)
{
	params->stream = rmidi->stream;
	if (ioctl(rmidi->fd, SNDRV_RAWMIDI_IOCTL_PARAMS, params) < 0)
		return -errno;
	rmidi->params = *params;
	return 0;
}

__SALSA_EXPORT_FUNC
int snd_rawmidi_params_current(snd_rawmidi_t *rmidi,
			       snd_rawmidi_params_t *params)
{
	*params = rmidi->params;
	return 0;
}

__snd_define_type(snd_rawmidi_status);

__SALSA_EXPORT_FUNC
void snd_rawmidi_status_get_tstamp(const snd_rawmidi_status_t *obj,
				   snd_htimestamp_t *ptr)
{
	*ptr = obj->tstamp;
}

__SALSA_EXPORT_FUNC
size_t snd_rawmidi_status_get_avail(const snd_rawmidi_status_t *obj)
{
	return obj->avail;
}

__SALSA_EXPORT_FUNC
size_t snd_rawmidi_status_get_xruns(const snd_rawmidi_status_t *obj)
{
	return obj->xruns;
}

__SALSA_EXPORT_FUNC
int snd_rawmidi_status(snd_rawmidi_t *rmidi, snd_rawmidi_status_t * status)
{
	status->stream = rmidi->stream;
	if (ioctl(rmidi->fd, SNDRV_RAWMIDI_IOCTL_STATUS, status) < 0)
		return -errno;
	return 0;
}

__SALSA_EXPORT_FUNC
int snd_rawmidi_drain(snd_rawmidi_t *rmidi)
{
	int str = rmidi->stream;
	if (ioctl(rmidi->fd, SNDRV_RAWMIDI_IOCTL_DRAIN, &str) < 0)
		return -errno;
	return 0;
}

__SALSA_EXPORT_FUNC
int snd_rawmidi_drop(snd_rawmidi_t *rmidi)
{
	int str = rmidi->stream;
	if (ioctl(rmidi->fd, SNDRV_RAWMIDI_IOCTL_DROP, &str) < 0)
		return -errno;
	return 0;
}

__SALSA_EXPORT_FUNC
ssize_t snd_rawmidi_write(snd_rawmidi_t *rmidi, const void *buffer,
			  size_t size)
{
	ssize_t result;
	result = write(rmidi->fd, buffer, size);
	if (result < 0)
		return -errno;
	return result;
}

__SALSA_EXPORT_FUNC
ssize_t snd_rawmidi_read(snd_rawmidi_t *rmidi, void *buffer, size_t size)
{
	ssize_t result;
	result = read(rmidi->fd, buffer, size);
	if (result < 0)
		return -errno;
	return result;
}

__SALSA_EXPORT_FUNC
const char *snd_rawmidi_name(snd_rawmidi_t *rmidi)
{
	return rmidi->hw->name;
}

__SALSA_EXPORT_FUNC
snd_rawmidi_type_t snd_rawmidi_type(snd_rawmidi_t *rmidi)
{
	return rmidi->hw->type;
}

__SALSA_EXPORT_FUNC
snd_rawmidi_stream_t snd_rawmidi_stream(snd_rawmidi_t *rmidi)
{
	return rmidi->stream;
}

#endif /* __ALSA_RAWMIDI_MACROS_H */
