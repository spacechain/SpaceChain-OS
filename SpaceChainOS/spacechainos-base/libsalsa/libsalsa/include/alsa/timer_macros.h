/*
 * timer privates and macros
 */

#ifndef __ALSA_TIMER_MACROS_H
#define __ALSA_TIMER_MACROS_H

#include "asound.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/poll.h>

struct _snd_timer {
	char *name;
	int type;
	int version;
	int mode;
	int fd;
	struct pollfd pollfd;
};

/*
 */

__SALSA_EXPORT_FUNC
int snd_timer_nonblock(snd_timer_t *timer, int nonblock)
{
	int err = _snd_set_nonblock(timer->fd, nonblock);
	if (err < 0)
		return err;
	if (nonblock)
		timer->mode |= SND_TIMER_OPEN_NONBLOCK;
	else
		timer->mode &= ~SND_TIMER_OPEN_NONBLOCK;
	return 0;
}

__SALSA_EXPORT_FUNC
int snd_timer_poll_descriptors_count(snd_timer_t *handle)
{
	return 1;
}

__SALSA_EXPORT_FUNC
int snd_timer_poll_descriptors(snd_timer_t *handle, struct pollfd *pfds, unsigned int space)
{
	*pfds = handle->pollfd;
	return 1;
}

__SALSA_EXPORT_FUNC
int snd_timer_poll_descriptors_revents(snd_timer_t *timer, struct pollfd *pfds, unsigned int nfds, unsigned short *revents)
{
	*revents = pfds->events;
	return 0;
}

__SALSA_EXPORT_FUNC
int snd_timer_info(snd_timer_t *handle, snd_timer_info_t *info)
{
	if (ioctl(handle->fd, SNDRV_TIMER_IOCTL_INFO, info) < 0)
		return -errno;
	return 0;
}

__SALSA_EXPORT_FUNC
int snd_timer_params(snd_timer_t *handle, snd_timer_params_t *params)
{
	if (ioctl(handle->fd, SNDRV_TIMER_IOCTL_PARAMS, params) < 0)
		return -errno;
	return 0;
}

__SALSA_EXPORT_FUNC
int snd_timer_status(snd_timer_t *handle, snd_timer_status_t *status)
{
	if (ioctl(handle->fd, SNDRV_TIMER_IOCTL_STATUS, status) < 0)
		return -errno;
	return 0;
}

__SALSA_EXPORT_FUNC
int snd_timer_start(snd_timer_t *handle)
{
	if (ioctl(handle->fd, SNDRV_TIMER_IOCTL_START) < 0)
		return -errno;
	return 0;
}

__SALSA_EXPORT_FUNC
int snd_timer_stop(snd_timer_t *handle)
{
	if (ioctl(handle->fd, SNDRV_TIMER_IOCTL_STOP) < 0)
		return -errno;
	return 0;
}

__SALSA_EXPORT_FUNC
int snd_timer_continue(snd_timer_t *handle)
{
	if (ioctl(handle->fd, SNDRV_TIMER_IOCTL_CONTINUE) < 0)
		return -errno;
	return 0;
}

__SALSA_EXPORT_FUNC
ssize_t snd_timer_read(snd_timer_t *handle, void *buffer, size_t size)
{
	ssize_t result;
	result = read(handle->fd, buffer, size);
	if (result < 0)
		return -errno;
	return result;
}

__snd_define_type(snd_timer_id);

__SALSA_EXPORT_FUNC
void snd_timer_id_set_class(snd_timer_id_t *id, int dev_class)
{
	id->dev_class = dev_class;
}

__SALSA_EXPORT_FUNC
int snd_timer_id_get_class(snd_timer_id_t *id)
{
	return id->dev_class;
}

__SALSA_EXPORT_FUNC
void snd_timer_id_set_sclass(snd_timer_id_t *id, int dev_sclass)
{
	id->dev_sclass = dev_sclass;
}

__SALSA_EXPORT_FUNC
int snd_timer_id_get_sclass(snd_timer_id_t *id)
{
	return id->dev_sclass;
}

__SALSA_EXPORT_FUNC
void snd_timer_id_set_card(snd_timer_id_t *id, int card)
{
	id->card = card;
}

__SALSA_EXPORT_FUNC
int snd_timer_id_get_card(snd_timer_id_t *id)
{
	return id->card;
}

__SALSA_EXPORT_FUNC
void snd_timer_id_set_device(snd_timer_id_t *id, int device)
{
	id->device = device;
}

__SALSA_EXPORT_FUNC
int snd_timer_id_get_device(snd_timer_id_t *id)
{
	return id->device;
}

__SALSA_EXPORT_FUNC
void snd_timer_id_set_subdevice(snd_timer_id_t *id, int subdevice)
{
	id->subdevice = subdevice;
}

__SALSA_EXPORT_FUNC
int snd_timer_id_get_subdevice(snd_timer_id_t *id)
{
	return id->subdevice;
}

__snd_define_type(snd_timer_ginfo);

__SALSA_EXPORT_FUNC
int snd_timer_ginfo_set_tid(snd_timer_ginfo_t *obj, snd_timer_id_t *tid)
{
	obj->tid = *tid;
	return 0;
}

__SALSA_EXPORT_FUNC
snd_timer_id_t *snd_timer_ginfo_get_tid(snd_timer_ginfo_t *obj)
{
	return &obj->tid;
}

__SALSA_EXPORT_FUNC
unsigned int snd_timer_ginfo_get_flags(snd_timer_ginfo_t *obj)
{
	return obj->flags;
}

__SALSA_EXPORT_FUNC
int snd_timer_ginfo_get_card(snd_timer_ginfo_t *obj)
{
	return obj->card;
}

__SALSA_EXPORT_FUNC
char *snd_timer_ginfo_get_id(snd_timer_ginfo_t *obj)
{
	return (char *)obj->id;
}

__SALSA_EXPORT_FUNC
char *snd_timer_ginfo_get_name(snd_timer_ginfo_t *obj)
{
	return (char *)obj->name;
}

__SALSA_EXPORT_FUNC
unsigned long snd_timer_ginfo_get_resolution(snd_timer_ginfo_t *obj)
{
	return obj->resolution;
}

__SALSA_EXPORT_FUNC
unsigned long snd_timer_ginfo_get_resolution_min(snd_timer_ginfo_t *obj)
{
	return obj->resolution_min;
}

__SALSA_EXPORT_FUNC
unsigned long snd_timer_ginfo_get_resolution_max(snd_timer_ginfo_t *obj)
{
	return obj->resolution_max;
}

__SALSA_EXPORT_FUNC
unsigned int snd_timer_ginfo_get_clients(snd_timer_ginfo_t *obj)
{
	return obj->clients;
}

__snd_define_type(snd_timer_info);

__SALSA_EXPORT_FUNC
int snd_timer_info_is_slave(snd_timer_info_t * info)
{
	return info->flags & SNDRV_TIMER_FLG_SLAVE ? 1 : 0;
}

__SALSA_EXPORT_FUNC
int snd_timer_info_get_card(snd_timer_info_t * info)
{
	return info->card;
}

__SALSA_EXPORT_FUNC
const char *snd_timer_info_get_id(snd_timer_info_t * info)
{
	return (const char *)info->id;
}

__SALSA_EXPORT_FUNC
const char *snd_timer_info_get_name(snd_timer_info_t * info)
{
	return (const char *)info->name;
}

__SALSA_EXPORT_FUNC
long snd_timer_info_get_resolution(snd_timer_info_t * info)
{
	return info->resolution;
}

__snd_define_type(snd_timer_params);

__SALSA_EXPORT_FUNC
int snd_timer_params_set_auto_start(snd_timer_params_t * params,
				    int auto_start)
{
	if (auto_start)
		params->flags |= SNDRV_TIMER_PSFLG_AUTO;
	else
		params->flags &= ~SNDRV_TIMER_PSFLG_AUTO;
	return 0;
}

__SALSA_EXPORT_FUNC
int snd_timer_params_get_auto_start(snd_timer_params_t * params)
{
	return params->flags & SNDRV_TIMER_PSFLG_AUTO ? 1 : 0;
}

__SALSA_EXPORT_FUNC
int snd_timer_params_set_exclusive(snd_timer_params_t * params, int exclusive)
{
	if (exclusive)
		params->flags |= SNDRV_TIMER_PSFLG_EXCLUSIVE;
	else
		params->flags &= ~SNDRV_TIMER_PSFLG_EXCLUSIVE;
	return 0;
}

__SALSA_EXPORT_FUNC
int snd_timer_params_get_exclusive(snd_timer_params_t * params)
{
	return params->flags & SNDRV_TIMER_PSFLG_EXCLUSIVE ? 1 : 0;
}

__SALSA_EXPORT_FUNC
int snd_timer_params_set_early_event(snd_timer_params_t * params,
				     int early_event)
{
	if (early_event)
		params->flags |= SNDRV_TIMER_PSFLG_EARLY_EVENT;
	else
		params->flags &= ~SNDRV_TIMER_PSFLG_EARLY_EVENT;
	return 0;
}

__SALSA_EXPORT_FUNC
int snd_timer_params_get_early_event(snd_timer_params_t * params)
{
	return params->flags & SNDRV_TIMER_PSFLG_EARLY_EVENT ? 1 : 0;
}

__SALSA_EXPORT_FUNC
void snd_timer_params_set_ticks(snd_timer_params_t * params, long ticks)
{
	params->ticks = ticks;
}

__SALSA_EXPORT_FUNC
long snd_timer_params_get_ticks(snd_timer_params_t * params)
{
	return params->ticks;
}

__SALSA_EXPORT_FUNC
void snd_timer_params_set_queue_size(snd_timer_params_t * params,
				     long queue_size)
{
	params->queue_size = queue_size;
}

__SALSA_EXPORT_FUNC
long snd_timer_params_get_queue_size(snd_timer_params_t * params)
{
	return params->queue_size;
}

__SALSA_EXPORT_FUNC
void snd_timer_params_set_filter(snd_timer_params_t * params,
				 unsigned int filter)
{
	params->filter = filter;
}

__SALSA_EXPORT_FUNC
unsigned int snd_timer_params_get_filter(snd_timer_params_t * params)
{
	return params->filter;
}

__snd_define_type(snd_timer_status);

__SALSA_EXPORT_FUNC
snd_htimestamp_t snd_timer_status_get_timestamp(snd_timer_status_t * status)
{
	return status->tstamp;
}

__SALSA_EXPORT_FUNC
long snd_timer_status_get_resolution(snd_timer_status_t * status)
{
	return status->resolution;
}

__SALSA_EXPORT_FUNC
long snd_timer_status_get_lost(snd_timer_status_t * status)
{
	return status->lost;
}

__SALSA_EXPORT_FUNC
long snd_timer_status_get_overrun(snd_timer_status_t * status)
{
	return status->overrun;
}

__SALSA_EXPORT_FUNC
long snd_timer_status_get_queue(snd_timer_status_t * status)
{
	return status->queue;
}

/*
 * not implemented
 */
__SALSA_EXPORT_FUNC __SALSA_NOT_IMPLEMENTED
int snd_timer_query_open(snd_timer_query_t **handle, const char *name, int mode)
{
	return -ENXIO;
}

__SALSA_EXPORT_FUNC __SALSA_NOT_IMPLEMENTED
int snd_timer_query_open_lconf(snd_timer_query_t **handle, const char *name,
			       int mode, snd_config_t *lconf)
{
	return -ENXIO;
}

__SALSA_EXPORT_FUNC __SALSA_NOT_IMPLEMENTED
int snd_timer_query_close(snd_timer_query_t *handle)
{
	return 0;
}

__SALSA_EXPORT_FUNC __SALSA_NOT_IMPLEMENTED
int snd_timer_query_next_device(snd_timer_query_t *handle, snd_timer_id_t *tid)
{
	return -ENXIO;
}

__SALSA_EXPORT_FUNC __SALSA_NOT_IMPLEMENTED
int snd_timer_query_info(snd_timer_query_t *handle, snd_timer_ginfo_t *info)
{
	return -ENXIO;
}

__SALSA_EXPORT_FUNC __SALSA_NOT_IMPLEMENTED
int snd_timer_query_params(snd_timer_query_t *handle,
			   snd_timer_gparams_t *params)
{
	return -ENXIO;
}

__SALSA_EXPORT_FUNC __SALSA_NOT_IMPLEMENTED
int snd_timer_query_status(snd_timer_query_t *handle,
			   snd_timer_gstatus_t *status)
{
	return -ENXIO;
}

__SALSA_EXPORT_FUNC __SALSA_NOT_IMPLEMENTED
int snd_timer_open_lconf(snd_timer_t **handle, const char *name, int mode,
			 snd_config_t *lconf)
{
	return -ENXIO;
}

__SALSA_EXPORT_FUNC __SALSA_NOT_IMPLEMENTED
int snd_async_add_timer_handler(snd_async_handler_t **handler,
				snd_timer_t *timer,
				snd_async_callback_t callback,
				void *private_data)
{
	return -ENXIO;
}

__SALSA_EXPORT_FUNC __SALSA_NOT_IMPLEMENTED
snd_timer_t *snd_async_handler_get_timer(snd_async_handler_t *handler)
{
	return NULL;
}

#endif /* __ALSA_TIMER_MACROS_H */
