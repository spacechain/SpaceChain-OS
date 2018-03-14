/*
 * SALSA-Lib - Control Interface
 *
 * Control privates and macros
 */

#ifndef __ALSA_CTL_MACROS_H
#define __ALSA_CTL_MACROS_H

#include "asound.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/poll.h>

struct _snd_ctl {
	char *name;
	snd_ctl_type_t type;
	int nonblock;
	int fd;
	int card;
	int protocol;
	struct pollfd pollfd;
#if SALSA_HAS_ASYNC_SUPPORT
	snd_async_handler_t *async;
#endif
};


#if SALSA_CHECK_ABI
#define SALSA_CTL_MAGIC		sizeof(struct _snd_ctl)
__SALSA_EXPORT_FUNC
int snd_ctl_open(snd_ctl_t **ctl, const char *name, int mode)
{
	return _snd_ctl_open(ctl, name, mode, SALSA_CTL_MAGIC);
}
#endif

__SALSA_EXPORT_FUNC
int snd_ctl_nonblock(snd_ctl_t *ctl, int nonblock)
{
	return _snd_set_nonblock(ctl->fd, nonblock);
}

__SALSA_EXPORT_FUNC
const char *snd_ctl_name(snd_ctl_t *ctl)
{
	return ctl->name;
}

__SALSA_EXPORT_FUNC
snd_ctl_type_t snd_ctl_type(snd_ctl_t *ctl)
{
	return ctl->type;
}

__SALSA_EXPORT_FUNC
int snd_ctl_poll_descriptors_count(snd_ctl_t *ctl)
{
	return 1;
}

__SALSA_EXPORT_FUNC
int snd_ctl_poll_descriptors(snd_ctl_t *ctl, struct pollfd *pfds,
			     unsigned int space)
{
	*pfds = ctl->pollfd;
	return 1;
}

__SALSA_EXPORT_FUNC
int snd_ctl_poll_descriptors_revents(snd_ctl_t *ctl, struct pollfd *pfds,
				     unsigned int nfds, unsigned short *revents)
{
	*revents = pfds->revents;
	return 0;
}

__SALSA_EXPORT_FUNC
int snd_ctl_subscribe_events(snd_ctl_t *ctl, int subscribe)
{
	if (ioctl(ctl->fd, SNDRV_CTL_IOCTL_SUBSCRIBE_EVENTS, &subscribe) < 0)
		return -errno;
	return 0;
}

__SALSA_EXPORT_FUNC
int snd_ctl_card_info(snd_ctl_t *ctl, snd_ctl_card_info_t *info)
{
	if (ioctl(ctl->fd, SNDRV_CTL_IOCTL_CARD_INFO, info) < 0)
		return -errno;
	return 0;
}

__SALSA_EXPORT_FUNC
int snd_ctl_elem_list(snd_ctl_t *ctl, snd_ctl_elem_list_t *list)
{
	if (ioctl(ctl->fd, SNDRV_CTL_IOCTL_ELEM_LIST, list) < 0)
		return -errno;
	return 0;
}

__SALSA_EXPORT_FUNC
int snd_ctl_elem_info(snd_ctl_t *ctl, snd_ctl_elem_info_t *info)
{
	if (ioctl(ctl->fd, SNDRV_CTL_IOCTL_ELEM_INFO, info) < 0)
		return -errno;
	return 0;
}

__SALSA_EXPORT_FUNC
int snd_ctl_elem_add(snd_ctl_t *ctl, snd_ctl_elem_info_t *info)
{
#if SALSA_HAS_USER_ELEM_SUPPORT
	if (ioctl(ctl->fd, SNDRV_CTL_IOCTL_ELEM_ADD, info) < 0)
		return -errno;
	return 0;
#else
	return -ENXIO;
#endif
}

__SALSA_EXPORT_FUNC
int snd_ctl_elem_remove(snd_ctl_t *ctl, snd_ctl_elem_id_t *id)
{
	if (ioctl(ctl->fd, SNDRV_CTL_IOCTL_ELEM_REMOVE, id) < 0)
		return -errno;
	return 0;
}

__SALSA_EXPORT_FUNC
int snd_ctl_elem_read(snd_ctl_t *ctl, snd_ctl_elem_value_t *control)
{
	if (ioctl(ctl->fd, SNDRV_CTL_IOCTL_ELEM_READ, control) < 0)
		return -errno;
	return 0;
}

__SALSA_EXPORT_FUNC
int snd_ctl_elem_write(snd_ctl_t *ctl, snd_ctl_elem_value_t *control)
{
	if (ioctl(ctl->fd, SNDRV_CTL_IOCTL_ELEM_WRITE, control) < 0)
		return -errno;
	return 0;
}

__SALSA_EXPORT_FUNC
int snd_ctl_elem_lock(snd_ctl_t *ctl, snd_ctl_elem_id_t *id)
{
	if (ioctl(ctl->fd, SNDRV_CTL_IOCTL_ELEM_LOCK, id) < 0)
		return -errno;
	return 0;
}

__SALSA_EXPORT_FUNC
int snd_ctl_elem_unlock(snd_ctl_t *ctl, snd_ctl_elem_id_t *id)
{
	if (ioctl(ctl->fd, SNDRV_CTL_IOCTL_ELEM_UNLOCK, id) < 0)
		return -errno;
	return 0;
}

__SALSA_EXPORT_FUNC
int snd_ctl_hwdep_next_device(snd_ctl_t *ctl, int *device)
{
	if (ioctl(ctl->fd, SNDRV_CTL_IOCTL_HWDEP_NEXT_DEVICE, device) < 0)
		return -errno;
	return 0;
}

__SALSA_EXPORT_FUNC
int snd_ctl_hwdep_info(snd_ctl_t *ctl, snd_hwdep_info_t * info)
{
	if (ioctl(ctl->fd, SNDRV_CTL_IOCTL_HWDEP_INFO, info) < 0)
		return -errno;
	return 0;
}

__SALSA_EXPORT_FUNC
int snd_ctl_pcm_next_device(snd_ctl_t *ctl, int * device)
{
	if (ioctl(ctl->fd, SNDRV_CTL_IOCTL_PCM_NEXT_DEVICE, device) < 0)
		return -errno;
	return 0;
}

__SALSA_EXPORT_FUNC
int snd_ctl_pcm_info(snd_ctl_t *ctl, snd_pcm_info_t * info)
{
	if (ioctl(ctl->fd, SNDRV_CTL_IOCTL_PCM_INFO, info) < 0)
		return -errno;
	return 0;
}

__SALSA_EXPORT_FUNC
int snd_ctl_pcm_prefer_subdevice(snd_ctl_t *ctl, int subdev)
{
	if (ioctl(ctl->fd, SNDRV_CTL_IOCTL_PCM_PREFER_SUBDEVICE, &subdev) < 0)
		return -errno;
	return 0;
}

__SALSA_EXPORT_FUNC
int snd_ctl_rawmidi_next_device(snd_ctl_t *ctl, int * device)
{
	if (ioctl(ctl->fd, SNDRV_CTL_IOCTL_RAWMIDI_NEXT_DEVICE, device) < 0)
		return -errno;
	return 0;
}

__SALSA_EXPORT_FUNC
int snd_ctl_rawmidi_info(snd_ctl_t *ctl, snd_rawmidi_info_t * info)
{
	if (ioctl(ctl->fd, SNDRV_CTL_IOCTL_RAWMIDI_INFO, info) < 0)
		return -errno;
	return 0;
}

__SALSA_EXPORT_FUNC
int snd_ctl_rawmidi_prefer_subdevice(snd_ctl_t *ctl, int subdev)
{
	if (ioctl(ctl->fd, SNDRV_CTL_IOCTL_RAWMIDI_PREFER_SUBDEVICE, &subdev) < 0)
		return -errno;
	return 0;
}

__SALSA_EXPORT_FUNC
int snd_ctl_read(snd_ctl_t *ctl, snd_ctl_event_t *event)
{
	ssize_t res = read(ctl->fd, event, sizeof(*event));
	if (res <= 0)
		return -errno;
	return 1;
}


extern const char * const _snd_ctl_elem_type_names[];
extern const char * const _snd_ctl_elem_iface_names[];
extern const char * const _snd_ctl_event_type_names[];

#define snd_ctl_elem_type_name(type)	_snd_ctl_elem_type_names[type]
#define snd_ctl_elem_iface_name(iface)	_snd_ctl_elem_iface_names[iface]
#define snd_ctl_event_type_name(type)	_snd_ctl_event_type_names[type]

__SALSA_EXPORT_FUNC
int snd_ctl_elem_list_alloc_space(snd_ctl_elem_list_t *obj,
				  unsigned int entries)
{
	free(obj->pids);
	obj->pids = (snd_ctl_elem_id_t*)calloc(entries, sizeof(*obj->pids));
	if (!obj->pids) {
		obj->space = 0;
		return -ENOMEM;
	}
	obj->space = entries;
	return 0;
}  

__SALSA_EXPORT_FUNC
void snd_ctl_elem_list_free_space(snd_ctl_elem_list_t *obj)
{
	free(obj->pids);
	obj->pids = NULL;
	obj->space = 0;
}

__SALSA_EXPORT_FUNC
unsigned int snd_ctl_event_elem_get_mask(const snd_ctl_event_t *obj)
{
	return obj->data.elem.mask;
}

__SALSA_EXPORT_FUNC
void snd_ctl_event_elem_get_id(const snd_ctl_event_t *obj,
			       snd_ctl_elem_id_t *ptr)
{
	*ptr = obj->data.elem.id;
}

__SALSA_EXPORT_FUNC
unsigned int snd_ctl_event_elem_get_numid(const snd_ctl_event_t *obj)
{
	return obj->data.elem.id.numid;
}

__SALSA_EXPORT_FUNC
snd_ctl_elem_iface_t snd_ctl_event_elem_get_interface(const snd_ctl_event_t *obj)
{
	return (snd_ctl_elem_iface_t)obj->data.elem.id.iface;
}

__SALSA_EXPORT_FUNC
unsigned int snd_ctl_event_elem_get_device(const snd_ctl_event_t *obj)
{
	return obj->data.elem.id.device;
}

__SALSA_EXPORT_FUNC
unsigned int snd_ctl_event_elem_get_subdevice(const snd_ctl_event_t *obj)
{
	return obj->data.elem.id.subdevice;
}

__SALSA_EXPORT_FUNC
const char *snd_ctl_event_elem_get_name(const snd_ctl_event_t *obj)
{
	return (const char *)obj->data.elem.id.name;
}

__SALSA_EXPORT_FUNC
unsigned int snd_ctl_event_elem_get_index(const snd_ctl_event_t *obj)
{
	return obj->data.elem.id.index;
}

__snd_define_type(snd_ctl_elem_id);

__SALSA_EXPORT_FUNC
unsigned int snd_ctl_elem_id_get_numid(const snd_ctl_elem_id_t *obj)
{
	return obj->numid;
}

__SALSA_EXPORT_FUNC
snd_ctl_elem_iface_t snd_ctl_elem_id_get_interface(const snd_ctl_elem_id_t *obj)
{
	return (snd_ctl_elem_iface_t)obj->iface;
}

__SALSA_EXPORT_FUNC
unsigned int snd_ctl_elem_id_get_device(const snd_ctl_elem_id_t *obj)
{
	return obj->device;
}

__SALSA_EXPORT_FUNC
unsigned int snd_ctl_elem_id_get_subdevice(const snd_ctl_elem_id_t *obj)
{
	return obj->subdevice;
}

__SALSA_EXPORT_FUNC
const char *snd_ctl_elem_id_get_name(const snd_ctl_elem_id_t *obj)
{
	return (const char *)obj->name;
}

__SALSA_EXPORT_FUNC
unsigned int snd_ctl_elem_id_get_index(const snd_ctl_elem_id_t *obj)
{
	return obj->index;
}

__SALSA_EXPORT_FUNC
void snd_ctl_elem_id_set_numid(snd_ctl_elem_id_t *obj, unsigned int val)
{
	obj->numid = val;
}

__SALSA_EXPORT_FUNC
void snd_ctl_elem_id_set_interface(snd_ctl_elem_id_t *obj,
				   snd_ctl_elem_iface_t val)
{
	obj->iface = val;
}

__SALSA_EXPORT_FUNC
void snd_ctl_elem_id_set_device(snd_ctl_elem_id_t *obj, unsigned int val)
{
	obj->device = val;
}

__SALSA_EXPORT_FUNC
void snd_ctl_elem_id_set_subdevice(snd_ctl_elem_id_t *obj, unsigned int val)
{
	obj->subdevice = val;
}

__SALSA_EXPORT_FUNC
void snd_ctl_elem_id_set_name(snd_ctl_elem_id_t *obj, const char *val)
{
	strncpy((char *)obj->name, val, sizeof(obj->name));
}

__SALSA_EXPORT_FUNC
void snd_ctl_elem_id_set_index(snd_ctl_elem_id_t *obj, unsigned int val)
{
	obj->index = val;
}

__snd_define_type(snd_ctl_card_info);

__SALSA_EXPORT_FUNC
int snd_ctl_card_info_get_card(const snd_ctl_card_info_t *obj)
{
	return obj->card;
}

__SALSA_EXPORT_FUNC
const char *snd_ctl_card_info_get_id(const snd_ctl_card_info_t *obj)
{
	return (const char *)obj->id;
}

__SALSA_EXPORT_FUNC
const char *snd_ctl_card_info_get_driver(const snd_ctl_card_info_t *obj)
{
	return (const char *)obj->driver;
}

__SALSA_EXPORT_FUNC
const char *snd_ctl_card_info_get_name(const snd_ctl_card_info_t *obj)
{
	return (const char *)obj->name;
}

__SALSA_EXPORT_FUNC
const char *snd_ctl_card_info_get_longname(const snd_ctl_card_info_t *obj)
{
	return (const char *)obj->longname;
}

__SALSA_EXPORT_FUNC
const char *snd_ctl_card_info_get_mixername(const snd_ctl_card_info_t *obj)
{
	return (const char *)obj->mixername;
}

__SALSA_EXPORT_FUNC
const char *snd_ctl_card_info_get_components(const snd_ctl_card_info_t *obj)
{
	return (const char *)obj->components;
}

__snd_define_type(snd_ctl_event);

__SALSA_EXPORT_FUNC
snd_ctl_event_type_t snd_ctl_event_get_type(const snd_ctl_event_t *obj)
{
	return (snd_ctl_event_type_t) obj->type;
}

__snd_define_type(snd_ctl_elem_list);

__SALSA_EXPORT_FUNC
void snd_ctl_elem_list_set_offset(snd_ctl_elem_list_t *obj, unsigned int val)
{
	obj->offset = val;
}

__SALSA_EXPORT_FUNC
unsigned int snd_ctl_elem_list_get_used(const snd_ctl_elem_list_t *obj)
{
	return obj->used;
}

__SALSA_EXPORT_FUNC
unsigned int snd_ctl_elem_list_get_count(const snd_ctl_elem_list_t *obj)
{
	return obj->count;
}

__SALSA_EXPORT_FUNC
void snd_ctl_elem_list_get_id(const snd_ctl_elem_list_t *obj,
			      unsigned int idx, snd_ctl_elem_id_t *ptr)
{
	*ptr = obj->pids[idx];
}

__SALSA_EXPORT_FUNC
unsigned int snd_ctl_elem_list_get_numid(const snd_ctl_elem_list_t *obj,
					 unsigned int idx)
{
	return obj->pids[idx].numid;
}

__SALSA_EXPORT_FUNC
snd_ctl_elem_iface_t snd_ctl_elem_list_get_interface(const snd_ctl_elem_list_t *obj,
						     unsigned int idx)
{
	return (snd_ctl_elem_iface_t) obj->pids[idx].iface;
}

__SALSA_EXPORT_FUNC
unsigned int snd_ctl_elem_list_get_device(const snd_ctl_elem_list_t *obj,
					  unsigned int idx)
{
	return obj->pids[idx].device;
}

__SALSA_EXPORT_FUNC
unsigned int snd_ctl_elem_list_get_subdevice(const snd_ctl_elem_list_t *obj,
					     unsigned int idx)
{
	return obj->pids[idx].subdevice;
}

__SALSA_EXPORT_FUNC
const char *snd_ctl_elem_list_get_name(const snd_ctl_elem_list_t *obj,
				       unsigned int idx)
{
	return (const char *)obj->pids[idx].name;
}

__SALSA_EXPORT_FUNC
unsigned int snd_ctl_elem_list_get_index(const snd_ctl_elem_list_t *obj,
					 unsigned int idx)
{
	return obj->pids[idx].index;
}

__snd_define_type(snd_ctl_elem_info);

__SALSA_EXPORT_FUNC
snd_ctl_elem_type_t snd_ctl_elem_info_get_type(const snd_ctl_elem_info_t *obj)
{
	return (snd_ctl_elem_type_t) obj->type;
}

__SALSA_EXPORT_FUNC
int snd_ctl_elem_info_is_readable(const snd_ctl_elem_info_t *obj)
{
	return !!(obj->_access & SNDRV_CTL_ELEM_ACCESS_READ);
}

__SALSA_EXPORT_FUNC
int snd_ctl_elem_info_is_writable(const snd_ctl_elem_info_t *obj)
{
	return !!(obj->_access & SNDRV_CTL_ELEM_ACCESS_WRITE);
}

__SALSA_EXPORT_FUNC
int snd_ctl_elem_info_is_volatile(const snd_ctl_elem_info_t *obj)
{
	return !!(obj->_access & SNDRV_CTL_ELEM_ACCESS_VOLATILE);
}

__SALSA_EXPORT_FUNC
int snd_ctl_elem_info_is_inactive(const snd_ctl_elem_info_t *obj)
{
	return !!(obj->_access & SNDRV_CTL_ELEM_ACCESS_INACTIVE);
}

__SALSA_EXPORT_FUNC
int snd_ctl_elem_info_is_locked(const snd_ctl_elem_info_t *obj)
{
	return !!(obj->_access & SNDRV_CTL_ELEM_ACCESS_LOCK);
}

__SALSA_EXPORT_FUNC
int snd_ctl_elem_info_is_owner(const snd_ctl_elem_info_t *obj)
{
	return !!(obj->_access & SNDRV_CTL_ELEM_ACCESS_OWNER);
}

__SALSA_EXPORT_FUNC
int snd_ctl_elem_info_is_user(const snd_ctl_elem_info_t *obj)
{
	return !!(obj->_access & SNDRV_CTL_ELEM_ACCESS_USER);
}

__SALSA_EXPORT_FUNC
int snd_ctl_elem_info_is_tlv_readable(const snd_ctl_elem_info_t *obj)
{
#if SALSA_HAS_TLV_SUPPORT
	return !!(obj->_access & SNDRV_CTL_ELEM_ACCESS_TLV_READ);
#else
	return 0;
#endif
}

__SALSA_EXPORT_FUNC
int snd_ctl_elem_info_is_tlv_writable(const snd_ctl_elem_info_t *obj)
{
#if SALSA_HAS_TLV_SUPPORT
	return !!(obj->_access & SNDRV_CTL_ELEM_ACCESS_TLV_WRITE);
#else
	return 0;
#endif
}

__SALSA_EXPORT_FUNC
int snd_ctl_elem_info_is_tlv_commandable(const snd_ctl_elem_info_t *obj)
{
#if SALSA_HAS_TLV_SUPPORT
	return !!(obj->_access & SNDRV_CTL_ELEM_ACCESS_TLV_COMMAND);
#else
	return 0;
#endif
}

__SALSA_EXPORT_FUNC
pid_t snd_ctl_elem_info_get_owner(const snd_ctl_elem_info_t *obj)
{
	return obj->owner;
}

__SALSA_EXPORT_FUNC
unsigned int snd_ctl_elem_info_get_count(const snd_ctl_elem_info_t *obj)
{
	return obj->count;
}

__SALSA_EXPORT_FUNC
long snd_ctl_elem_info_get_min(const snd_ctl_elem_info_t *obj)
{
	return obj->value.integer.min;
}

__SALSA_EXPORT_FUNC
long snd_ctl_elem_info_get_max(const snd_ctl_elem_info_t *obj)
{
	return obj->value.integer.max;
}

__SALSA_EXPORT_FUNC
long snd_ctl_elem_info_get_step(const snd_ctl_elem_info_t *obj)
{
	return obj->value.integer.step;
}

__SALSA_EXPORT_FUNC
long long snd_ctl_elem_info_get_min64(const snd_ctl_elem_info_t *obj)
{
	return obj->value.integer64.min;
}

__SALSA_EXPORT_FUNC
long long snd_ctl_elem_info_get_max64(const snd_ctl_elem_info_t *obj)
{
	return obj->value.integer64.max;
}

__SALSA_EXPORT_FUNC
long long snd_ctl_elem_info_get_step64(const snd_ctl_elem_info_t *obj)
{
	return obj->value.integer64.step;
}

__SALSA_EXPORT_FUNC
unsigned int snd_ctl_elem_info_get_items(const snd_ctl_elem_info_t *obj)
{
	return obj->value.enumerated.items;
}

__SALSA_EXPORT_FUNC
void snd_ctl_elem_info_set_item(snd_ctl_elem_info_t *obj, unsigned int val)
{
	obj->value.enumerated.item = val;
}

__SALSA_EXPORT_FUNC
const char *snd_ctl_elem_info_get_item_name(const snd_ctl_elem_info_t *obj)
{
	return obj->value.enumerated.name;
}

__SALSA_EXPORT_FUNC
int snd_ctl_elem_info_get_dimensions(const snd_ctl_elem_info_t *obj)
{
	int i;
	for (i = 3; i >= 0; i--)
		if (obj->dimen.d[i])
			break;
	return i + 1;
}

__SALSA_EXPORT_FUNC
int snd_ctl_elem_info_get_dimension(const snd_ctl_elem_info_t *obj,
				    unsigned int idx)
{
	if (idx >= 3)
		return 0;
	return obj->dimen.d[idx];
}

__SALSA_EXPORT_FUNC
void snd_ctl_elem_info_get_id(const snd_ctl_elem_info_t *obj,
			      snd_ctl_elem_id_t *ptr)
{
	*ptr = obj->id;
}

__SALSA_EXPORT_FUNC
unsigned int snd_ctl_elem_info_get_numid(const snd_ctl_elem_info_t *obj)
{
	return obj->id.numid;
}

__SALSA_EXPORT_FUNC
snd_ctl_elem_iface_t snd_ctl_elem_info_get_interface(const snd_ctl_elem_info_t *obj)
{
	return (snd_ctl_elem_iface_t) obj->id.iface;
}

__SALSA_EXPORT_FUNC
unsigned int snd_ctl_elem_info_get_device(const snd_ctl_elem_info_t *obj)
{
	return obj->id.device;
}

__SALSA_EXPORT_FUNC
unsigned int snd_ctl_elem_info_get_subdevice(const snd_ctl_elem_info_t *obj)
{
	return obj->id.subdevice;
}

__SALSA_EXPORT_FUNC
const char *snd_ctl_elem_info_get_name(const snd_ctl_elem_info_t *obj)
{
	return (const char *)obj->id.name;
}

__SALSA_EXPORT_FUNC
unsigned int snd_ctl_elem_info_get_index(const snd_ctl_elem_info_t *obj)
{
	return obj->id.index;
}

__SALSA_EXPORT_FUNC
void snd_ctl_elem_info_set_id(snd_ctl_elem_info_t *obj,
			      const snd_ctl_elem_id_t *ptr)
{
	obj->id = *ptr;
}

__SALSA_EXPORT_FUNC
void snd_ctl_elem_info_set_numid(snd_ctl_elem_info_t *obj, unsigned int val)
{
	obj->id.numid = val;
}

__SALSA_EXPORT_FUNC
void snd_ctl_elem_info_set_interface(snd_ctl_elem_info_t *obj,
				     snd_ctl_elem_iface_t val)
{
	obj->id.iface = val;
}

__SALSA_EXPORT_FUNC
void snd_ctl_elem_info_set_device(snd_ctl_elem_info_t *obj, unsigned int val)
{
	obj->id.device = val;
}

__SALSA_EXPORT_FUNC
void snd_ctl_elem_info_set_subdevice(snd_ctl_elem_info_t *obj,
				     unsigned int val)
{
	obj->id.subdevice = val;
}

__SALSA_EXPORT_FUNC
void snd_ctl_elem_info_set_name(snd_ctl_elem_info_t *obj, const char *val)
{
	strncpy((char *)obj->id.name, val, sizeof(obj->id.name));
}

__SALSA_EXPORT_FUNC
void snd_ctl_elem_info_set_index(snd_ctl_elem_info_t *obj, unsigned int val)
{
	obj->id.index = val;
}

__snd_define_type(snd_ctl_elem_value);
__SALSA_EXPORT_FUNC
void snd_ctl_elem_value_get_id(const snd_ctl_elem_value_t *obj,
			       snd_ctl_elem_id_t *ptr)
{
	*ptr = obj->id;
}

__SALSA_EXPORT_FUNC
unsigned int snd_ctl_elem_value_get_numid(const snd_ctl_elem_value_t *obj)
{
	return obj->id.numid;
}

__SALSA_EXPORT_FUNC
snd_ctl_elem_iface_t snd_ctl_elem_value_get_interface(const snd_ctl_elem_value_t *obj)
{
	return (snd_ctl_elem_iface_t) obj->id.iface;
}

__SALSA_EXPORT_FUNC
unsigned int snd_ctl_elem_value_get_device(const snd_ctl_elem_value_t *obj)
{
	return obj->id.device;
}

__SALSA_EXPORT_FUNC
unsigned int snd_ctl_elem_value_get_subdevice(const snd_ctl_elem_value_t *obj)
{
	return obj->id.subdevice;
}

__SALSA_EXPORT_FUNC
const char *snd_ctl_elem_value_get_name(const snd_ctl_elem_value_t *obj)
{
	return (const char *)obj->id.name;
}

__SALSA_EXPORT_FUNC
unsigned int snd_ctl_elem_value_get_index(const snd_ctl_elem_value_t *obj)
{
	return obj->id.index;
}

__SALSA_EXPORT_FUNC
void snd_ctl_elem_value_set_id(snd_ctl_elem_value_t *obj,
			       const snd_ctl_elem_id_t *ptr)
{
	obj->id = *ptr;
}

__SALSA_EXPORT_FUNC
void snd_ctl_elem_value_set_numid(snd_ctl_elem_value_t *obj, unsigned int val)
{
	obj->id.numid = val;
}

__SALSA_EXPORT_FUNC
void snd_ctl_elem_value_set_interface(snd_ctl_elem_value_t *obj,
				      snd_ctl_elem_iface_t val)
{
	obj->id.iface = val;
}

__SALSA_EXPORT_FUNC
void snd_ctl_elem_value_set_device(snd_ctl_elem_value_t *obj, unsigned int val)
{
	obj->id.device = val;
}

__SALSA_EXPORT_FUNC
void snd_ctl_elem_value_set_subdevice(snd_ctl_elem_value_t *obj,
				      unsigned int val)
{
	obj->id.subdevice = val;
}

__SALSA_EXPORT_FUNC
void snd_ctl_elem_value_set_name(snd_ctl_elem_value_t *obj, const char *val)
{
	strncpy((char *)obj->id.name, val, sizeof(obj->id.name));
}

__SALSA_EXPORT_FUNC
void snd_ctl_elem_value_set_index(snd_ctl_elem_value_t *obj, unsigned int val)
{
	obj->id.index = val;
}

__SALSA_EXPORT_FUNC
int snd_ctl_elem_value_get_boolean(const snd_ctl_elem_value_t *obj,
				   unsigned int idx)
{
	return obj->value.integer.value[idx];
}

__SALSA_EXPORT_FUNC
long snd_ctl_elem_value_get_integer(const snd_ctl_elem_value_t *obj,
				    unsigned int idx)
{
	return obj->value.integer.value[idx];
}

__SALSA_EXPORT_FUNC
long long snd_ctl_elem_value_get_integer64(const snd_ctl_elem_value_t *obj,
					   unsigned int idx)
{
	return obj->value.integer64.value[idx];
}

__SALSA_EXPORT_FUNC
unsigned int snd_ctl_elem_value_get_enumerated(const snd_ctl_elem_value_t *obj,
					       unsigned int idx)
{
	return obj->value.enumerated.item[idx];
}

__SALSA_EXPORT_FUNC
unsigned char snd_ctl_elem_value_get_byte(const snd_ctl_elem_value_t *obj,
					  unsigned int idx)
{
	return obj->value.bytes.data[idx];
}

__SALSA_EXPORT_FUNC
void snd_ctl_elem_value_set_boolean(snd_ctl_elem_value_t *obj,
				    unsigned int idx, long val)
{
	obj->value.integer.value[idx] = val;
}

__SALSA_EXPORT_FUNC
void snd_ctl_elem_value_set_integer(snd_ctl_elem_value_t *obj,
				    unsigned int idx, long val)
{
	obj->value.integer.value[idx] = val;
}

__SALSA_EXPORT_FUNC
void snd_ctl_elem_value_set_integer64(snd_ctl_elem_value_t *obj,
				      unsigned int idx, long long val)
{
	obj->value.integer64.value[idx] = val;
}

__SALSA_EXPORT_FUNC
void snd_ctl_elem_value_set_enumerated(snd_ctl_elem_value_t *obj,
				       unsigned int idx, unsigned int val)
{
	obj->value.enumerated.item[idx] = val;
}

__SALSA_EXPORT_FUNC
void snd_ctl_elem_value_set_byte(snd_ctl_elem_value_t *obj, unsigned int idx,
				 unsigned char val)
{
	obj->value.bytes.data[idx] = val;
}

__SALSA_EXPORT_FUNC
void snd_ctl_elem_set_bytes(snd_ctl_elem_value_t *obj, void *data, size_t size)
{
	if (size < sizeof(obj->value.bytes.data))
		memcpy(obj->value.bytes.data, data, size);
}

__SALSA_EXPORT_FUNC
const void * snd_ctl_elem_value_get_bytes(const snd_ctl_elem_value_t *obj)
{
	return obj->value.bytes.data;
}

__SALSA_EXPORT_FUNC
void snd_ctl_elem_value_get_iec958(const snd_ctl_elem_value_t *obj,
				   snd_aes_iec958_t *ptr)
{
	memcpy(ptr, &obj->value.iec958, sizeof(*ptr));
}

__SALSA_EXPORT_FUNC
void snd_ctl_elem_value_set_iec958(snd_ctl_elem_value_t *obj,
				   const snd_aes_iec958_t *ptr)
{
	memcpy(&obj->value.iec958, ptr, sizeof(obj->value.iec958));
}

/*
 */
#if !SALSA_HAS_TLV_SUPPORT
__SALSA_EXPORT_FUNC __SALSA_NOT_IMPLEMENTED
int snd_ctl_elem_tlv_read(snd_ctl_t *ctl, const snd_ctl_elem_id_t *id,
			  unsigned int *tlv, unsigned int tlv_size)
{
	return -ENXIO;
}

__SALSA_EXPORT_FUNC __SALSA_NOT_IMPLEMENTED
int snd_ctl_elem_tlv_write(snd_ctl_t *ctl, const snd_ctl_elem_id_t *id,
			   const unsigned int *tlv)
{
	return -ENXIO;
}

__SALSA_EXPORT_FUNC __SALSA_NOT_IMPLEMENTED
int snd_ctl_elem_tlv_command(snd_ctl_t *ctl, const snd_ctl_elem_id_t *id,
			     const unsigned int *tlv)
{
	return -ENXIO;
}

__SALSA_EXPORT_FUNC __SALSA_NOT_IMPLEMENTED
int snd_tlv_parse_dB_info(unsigned int *tlv, unsigned int tlv_size,
			  unsigned int **db_tlvp)
{
	return -ENXIO;
}

__SALSA_EXPORT_FUNC __SALSA_NOT_IMPLEMENTED
int snd_tlv_get_dB_range(unsigned int *tlv, long rangemin, long rangemax,
			 long *min, long *max)
{
	return -ENXIO;
}

__SALSA_EXPORT_FUNC __SALSA_NOT_IMPLEMENTED
int snd_tlv_convert_to_dB(unsigned int *tlv, long rangemin, long rangemax,
			  long volume, long *db_gain)
{
	return -ENXIO;
}

__SALSA_EXPORT_FUNC __SALSA_NOT_IMPLEMENTED
int snd_tlv_convert_from_dB(unsigned int *tlv, long rangemin, long rangemax,
			    long db_gain, long *value, int xdir)
{
	return -ENXIO;
}

__SALSA_EXPORT_FUNC __SALSA_NOT_IMPLEMENTED
int snd_ctl_get_dB_range(snd_ctl_t *ctl, const snd_ctl_elem_id_t *id,
			 long *min, long *max)
{
	return -ENXIO;
}

__SALSA_EXPORT_FUNC __SALSA_NOT_IMPLEMENTED
int snd_ctl_convert_to_dB(snd_ctl_t *ctl, const snd_ctl_elem_id_t *id,
			  long volume, long *db_gain)
{
	return -ENXIO;
}

__SALSA_EXPORT_FUNC __SALSA_NOT_IMPLEMENTED
int snd_ctl_convert_from_dB(snd_ctl_t *ctl, const snd_ctl_elem_id_t *id,
			    long db_gain, long *value, int xdir)
{
	return -ENXIO;
}
#endif /* !SALSA_HAS_TLV_SUPPORT */

/*
 */
#if !SALSA_HAS_USER_ELEM_SUPPORT
__SALSA_EXPORT_FUNC __SALSA_NOT_IMPLEMENTED
int snd_ctl_elem_add_integer(snd_ctl_t *ctl, const snd_ctl_elem_id_t *id,
			     unsigned int count, long min, long max, long step)
{
	return -ENXIO;
}

__SALSA_EXPORT_FUNC __SALSA_NOT_IMPLEMENTED
int snd_ctl_elem_add_integer64(snd_ctl_t *ctl, const snd_ctl_elem_id_t *id,
			       unsigned int count, long long min, long long max,
			       long long step)
{
	return -ENXIO;
}

__SALSA_EXPORT_FUNC __SALSA_NOT_IMPLEMENTED
int snd_ctl_elem_add_boolean(snd_ctl_t *ctl, const snd_ctl_elem_id_t *id,
			     unsigned int count)
{
	return -ENXIO;
}

__SALSA_EXPORT_FUNC __SALSA_NOT_IMPLEMENTED
int snd_ctl_elem_add_iec958(snd_ctl_t *ctl, const snd_ctl_elem_id_t *id)
{
	return -ENXIO;
}

__SALSA_EXPORT_FUNC __SALSA_NOT_IMPLEMENTED
int snd_ctl_elem_add_enumerated(snd_ctl_t *ctl, const snd_ctl_elem_id_t *id,
				unsigned int count, unsigned int items,
				const char *const names[])
{
	return -ENXIO;
}
#endif /* !SALSA_HAS_USER_ELEM_SUPPORT */

/*
 * not implemented yet
 */

__SALSA_EXPORT_FUNC __SALSA_NOT_IMPLEMENTED
int snd_ctl_open_lconf(snd_ctl_t **ctlp, const char *name,
		       int mode, snd_config_t *lconf)
{
	return -ENXIO;
}
__SALSA_EXPORT_FUNC __SALSA_NOT_IMPLEMENTED
int snd_ctl_set_power_state(snd_ctl_t *ctl, unsigned int state)
{
	return -ENXIO;
}

__SALSA_EXPORT_FUNC __SALSA_NOT_IMPLEMENTED
int snd_ctl_get_power_state(snd_ctl_t *ctl, unsigned int *state)
{
	return -ENXIO;
}

__SALSA_EXPORT_FUNC __SALSA_NOT_IMPLEMENTED
int snd_device_name_hint(int card, const char *iface, void ***hints)
{
	return -ENXIO;
}

__SALSA_EXPORT_FUNC __SALSA_NOT_IMPLEMENTED
int snd_device_name_free_hint(void **hints)
{
	return 0;
}

__SALSA_EXPORT_FUNC __SALSA_NOT_IMPLEMENTED
char *snd_device_name_get_hint(const void *hint, const char *id)
{
	return NULL;
}

#if SALSA_HAS_ASYNC_SUPPORT

__SALSA_EXPORT_FUNC
snd_ctl_t *snd_async_handler_get_ctl(snd_async_handler_t *handler)
{
	return (snd_ctl_t *) handler->rec;
}

#else

__SALSA_EXPORT_FUNC __SALSA_NOT_IMPLEMENTED
int snd_async_add_ctl_handler(snd_async_handler_t **handler, snd_ctl_t *ctl, 
			      snd_async_callback_t callback,
			      void *private_data)
{
	return -ENXIO;
}

__SALSA_EXPORT_FUNC __SALSA_NOT_IMPLEMENTED
snd_ctl_t *snd_async_handler_get_ctl(snd_async_handler_t *handler)
{
	return NULL;
}

#endif /* SALSA_HAS_ASYNC_SUPPORT */

#endif /* __ALSA_CTL_MACROS_H */
