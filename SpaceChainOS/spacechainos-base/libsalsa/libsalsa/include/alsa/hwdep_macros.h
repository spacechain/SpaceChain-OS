/*
 * hwdep privates and macros
 */

#ifndef __ALSA_HWDEP_MACROS_H
#define __ALSA_HWDEP_MACROS_H

#include "asound.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/poll.h>

struct _snd_hwdep {
	const char *name;
	int type;
	int mode;
	int fd;
	struct pollfd pollfd;
};


__SALSA_EXPORT_FUNC
int snd_hwdep_nonblock(snd_hwdep_t *hwdep, int nonblock)
{
	return _snd_set_nonblock(hwdep->fd, nonblock);
}

__SALSA_EXPORT_FUNC
int snd_hwdep_poll_descriptors_count(snd_rawmidi_t *rmidi)
{
	return 1;
}

__SALSA_EXPORT_FUNC
int snd_hwdep_poll_descriptors(snd_hwdep_t *hwdep, struct pollfd *pfds,
			       unsigned int space)
{
	*pfds = hwdep->pollfd;
	return 1;
}

__SALSA_EXPORT_FUNC
int snd_hwdep_poll_descriptors_revents(snd_hwdep_t *hwdep, struct pollfd *pfds,
				       unsigned int nfds,
				       unsigned short *revents)
{
	*revents = pfds->events;
	return 0;
}

__SALSA_EXPORT_FUNC
int snd_hwdep_info(snd_hwdep_t *hwdep, snd_hwdep_info_t * info)
{
	if (ioctl(hwdep->fd, SNDRV_HWDEP_IOCTL_INFO, info) < 0)
		return -errno;
	return 0;
}

__SALSA_EXPORT_FUNC
int snd_hwdep_ioctl(snd_hwdep_t *hwdep, unsigned int request, void * arg)
{
	if (ioctl(hwdep->fd, request, arg) < 0)
		return -errno;
	return 0;
}

__SALSA_EXPORT_FUNC
int snd_hwdep_dsp_status(snd_hwdep_t *hwdep, snd_hwdep_dsp_status_t *status)
{
	return snd_hwdep_ioctl(hwdep, SNDRV_HWDEP_IOCTL_DSP_STATUS, status);
}

__SALSA_EXPORT_FUNC
int snd_hwdep_dsp_load(snd_hwdep_t *hwdep, snd_hwdep_dsp_image_t *block)
{
	return snd_hwdep_ioctl(hwdep, SNDRV_HWDEP_IOCTL_DSP_LOAD, block);
}

__SALSA_EXPORT_FUNC
ssize_t snd_hwdep_write(snd_hwdep_t *hwdep, const void *buffer, size_t size)
{
	ssize_t result;
	result = write(hwdep->fd, buffer, size);
	if (result < 0)
		return -errno;
	return result;
}

__SALSA_EXPORT_FUNC
ssize_t snd_hwdep_read(snd_hwdep_t *hwdep, void *buffer, size_t size)
{
	ssize_t result;
	result = read(hwdep->fd, buffer, size);
	if (result < 0)
		return -errno;
	return result;
}

__snd_define_type(snd_hwdep_info);

__SALSA_EXPORT_FUNC
unsigned int snd_hwdep_info_get_device(const snd_hwdep_info_t *obj)
{
	return obj->device;
}

__SALSA_EXPORT_FUNC
int snd_hwdep_info_get_card(const snd_hwdep_info_t *obj)
{
	return obj->card;
}

__SALSA_EXPORT_FUNC
const char *snd_hwdep_info_get_id(const snd_hwdep_info_t *obj)
{
	return (const char *)obj->id;
}

__SALSA_EXPORT_FUNC
const char *snd_hwdep_info_get_name(const snd_hwdep_info_t *obj)
{
	return (const char *)obj->name;
}

__SALSA_EXPORT_FUNC
snd_hwdep_iface_t snd_hwdep_info_get_iface(const snd_hwdep_info_t *obj)
{
	return (snd_hwdep_iface_t) obj->iface;
}

__SALSA_EXPORT_FUNC
void snd_hwdep_info_set_device(snd_hwdep_info_t *obj, unsigned int val)
{
	obj->device = val;
}

__snd_define_type(snd_hwdep_dsp_status);

__SALSA_EXPORT_FUNC
unsigned int snd_hwdep_dsp_status_get_version(const snd_hwdep_dsp_status_t *obj)
{
	return obj->version;
}

__SALSA_EXPORT_FUNC
const char *snd_hwdep_dsp_status_get_id(const snd_hwdep_dsp_status_t *obj)
{
	return (const char *)obj->id;
}

__SALSA_EXPORT_FUNC
unsigned int snd_hwdep_dsp_status_get_num_dsps(const snd_hwdep_dsp_status_t *obj)
{
	return obj->num_dsps;
}

__SALSA_EXPORT_FUNC
unsigned int snd_hwdep_dsp_status_get_dsp_loaded(const snd_hwdep_dsp_status_t *obj)
{
	return obj->dsp_loaded;
}

__SALSA_EXPORT_FUNC
unsigned int snd_hwdep_dsp_status_get_chip_ready(const snd_hwdep_dsp_status_t *obj)
{
	return obj->chip_ready;
}

__snd_define_type(snd_hwdep_dsp_image);

__SALSA_EXPORT_FUNC
unsigned int snd_hwdep_dsp_image_get_index(const snd_hwdep_dsp_image_t *obj)
{
	return obj->index;
}

__SALSA_EXPORT_FUNC
const char *snd_hwdep_dsp_image_get_name(const snd_hwdep_dsp_image_t *obj)
{
	return (const char *)obj->name;
}

__SALSA_EXPORT_FUNC
const void *snd_hwdep_dsp_image_get_image(const snd_hwdep_dsp_image_t *obj)
{
	return (const void *)obj->image;
}

__SALSA_EXPORT_FUNC
size_t snd_hwdep_dsp_image_get_length(const snd_hwdep_dsp_image_t *obj)
{
	return obj->length;
}

__SALSA_EXPORT_FUNC
void snd_hwdep_dsp_image_set_index(snd_hwdep_dsp_image_t *obj, unsigned int _index)
{
	obj->index = _index;
}

__SALSA_EXPORT_FUNC
void snd_hwdep_dsp_image_set_name(snd_hwdep_dsp_image_t *obj, const char *name)
{
	strncpy((char *)obj->name, name, sizeof(obj->name));
	obj->name[sizeof(obj->name)-1] = 0;
}

__SALSA_EXPORT_FUNC
void snd_hwdep_dsp_image_set_image(snd_hwdep_dsp_image_t *obj, void *buffer)
{
	obj->image = (unsigned char *) buffer;
}

__SALSA_EXPORT_FUNC
void snd_hwdep_dsp_image_set_length(snd_hwdep_dsp_image_t *obj, size_t length)
{
	obj->length = length;
}

#endif /* __ALSA_HWDEP_MACROS_H */
