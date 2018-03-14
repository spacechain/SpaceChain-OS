/*
 * SALSA-Lib - High-level Control Interface
 *
 * h-control privates and macros
 */

#ifndef __ALSA_HCTL_MACROS_H
#define __ALSA_HCTL_MACROS_H

#include "asound.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/poll.h>

struct _snd_hctl {
	snd_ctl_t *ctl;
	snd_hctl_callback_t callback;
	void *callback_private;
	snd_hctl_elem_t *first_elem;
	snd_hctl_elem_t *last_elem;
	unsigned int count;
};

struct _snd_hctl_elem {
	snd_ctl_elem_id_t id;
	snd_hctl_t *hctl;
	void *private_data;
	snd_hctl_elem_callback_t callback;
	void *callback_private;
	snd_hctl_elem_t *prev;
	snd_hctl_elem_t *next;
};


#if SALSA_CHECK_ABI
#define SALSA_HCTL_MAGIC \
	((sizeof(struct _snd_hctl) << 8) | sizeof(struct _snd_hctl_elem))
__SALSA_EXPORT_FUNC
int snd_hctl_open(snd_hctl_t **hctl, const char *name, int mode)
{
	return _snd_hctl_open(hctl, name, mode, SALSA_HCTL_MAGIC);
}
#endif

__SALSA_EXPORT_FUNC
int snd_hctl_wait(snd_hctl_t *hctl, int timeout)
{
	int err;
	do {
		err = snd_ctl_wait(hctl->ctl, timeout);
	} while (err == -EINTR);
	return err;
}

__SALSA_EXPORT_FUNC
const char *snd_hctl_name(snd_hctl_t *hctl)
{
	return snd_ctl_name(hctl->ctl);
}

__SALSA_EXPORT_FUNC
int snd_hctl_nonblock(snd_hctl_t *hctl, int nonblock)
{
	return snd_ctl_nonblock(hctl->ctl, nonblock);
}

__SALSA_EXPORT_FUNC
int snd_hctl_set_compare(snd_hctl_t *hctl, snd_hctl_compare_t compare)
{
	return 0;
}

__SALSA_EXPORT_FUNC
int snd_hctl_poll_descriptors_count(snd_hctl_t *hctl)
{
	return snd_ctl_poll_descriptors_count(hctl->ctl);
}

__SALSA_EXPORT_FUNC
int snd_hctl_poll_descriptors(snd_hctl_t *hctl, struct pollfd *pfds, unsigned int space)
{
	return snd_ctl_poll_descriptors(hctl->ctl, pfds, space);
}

__SALSA_EXPORT_FUNC
int snd_hctl_poll_descriptors_revents(snd_hctl_t *hctl, struct pollfd *pfds, unsigned int nfds, unsigned short *revents)
{
	return snd_ctl_poll_descriptors_revents(hctl->ctl, pfds, nfds, revents);
}

__SALSA_EXPORT_FUNC
void snd_hctl_set_callback(snd_hctl_t *hctl, snd_hctl_callback_t callback)
{
	hctl->callback = callback;
}

__SALSA_EXPORT_FUNC
void snd_hctl_set_callback_private(snd_hctl_t *hctl, void *callback_private)
{
	hctl->callback_private = callback_private;
}

__SALSA_EXPORT_FUNC
void *snd_hctl_get_callback_private(snd_hctl_t *hctl)
{
	return hctl->callback_private;
}

__SALSA_EXPORT_FUNC
unsigned int snd_hctl_get_count(snd_hctl_t *hctl)
{
	return hctl->count;
}

__SALSA_EXPORT_FUNC
snd_ctl_t *snd_hctl_ctl(snd_hctl_t *hctl)
{
	return hctl->ctl;
}

__SALSA_EXPORT_FUNC
int snd_hctl_elem_info(snd_hctl_elem_t *elem, snd_ctl_elem_info_t *info)
{
	info->id = elem->id;
	return snd_ctl_elem_info(elem->hctl->ctl, info);
}

__SALSA_EXPORT_FUNC
int snd_hctl_elem_read(snd_hctl_elem_t *elem, snd_ctl_elem_value_t *value)
{
	value->id = elem->id;
	return snd_ctl_elem_read(elem->hctl->ctl, value);
}

__SALSA_EXPORT_FUNC
int snd_hctl_elem_write(snd_hctl_elem_t *elem, snd_ctl_elem_value_t *value)
{
	value->id = elem->id;
	return snd_ctl_elem_write(elem->hctl->ctl, value);
}

#if SALSA_HAS_TLV_SUPPORT
__SALSA_EXPORT_FUNC
int snd_hctl_elem_tlv_read(snd_hctl_elem_t *elem, unsigned int *tlv,
			   unsigned int tlv_size)
{
	return snd_ctl_elem_tlv_read(elem->hctl->ctl, &elem->id, tlv, tlv_size);
}

__SALSA_EXPORT_FUNC
int snd_hctl_elem_tlv_write(snd_hctl_elem_t *elem, const unsigned int *tlv)
{
	return snd_ctl_elem_tlv_write(elem->hctl->ctl, &elem->id, tlv);
}

__SALSA_EXPORT_FUNC
int snd_hctl_elem_tlv_command(snd_hctl_elem_t *elem, const unsigned int *tlv)
{
	return snd_ctl_elem_tlv_command(elem->hctl->ctl, &elem->id, tlv);
}

#else /* SALSA_HAS_TLV_SUPPORT */

__SALSA_EXPORT_FUNC __SALSA_NOT_IMPLEMENTED
int snd_hctl_elem_tlv_read(snd_hctl_elem_t *elem, unsigned int *tlv,
			   unsigned int tlv_size)
{
	return -ENXIO;
}

__SALSA_EXPORT_FUNC __SALSA_NOT_IMPLEMENTED
int snd_hctl_elem_tlv_write(snd_hctl_elem_t *elem, const unsigned int *tlv)
{
	return -ENXIO;
}

__SALSA_EXPORT_FUNC __SALSA_NOT_IMPLEMENTED
int snd_hctl_elem_tlv_command(snd_hctl_elem_t *elem, const unsigned int *tlv)
{
	return -ENXIO;
}
#endif /* SALSA_HAS_TLV_SUPPORT */

__SALSA_EXPORT_FUNC
snd_hctl_t *snd_hctl_elem_get_hctl(snd_hctl_elem_t *elem)
{
	return elem->hctl;
}

__SALSA_EXPORT_FUNC
void snd_hctl_elem_get_id(const snd_hctl_elem_t *obj, snd_ctl_elem_id_t *ptr)
{
	*ptr = obj->id;
}

__SALSA_EXPORT_FUNC
unsigned int snd_hctl_elem_get_numid(const snd_hctl_elem_t *obj)
{
	return obj->id.numid;
}

__SALSA_EXPORT_FUNC
snd_ctl_elem_iface_t snd_hctl_elem_get_interface(const snd_hctl_elem_t *obj)
{
	return (snd_ctl_elem_iface_t) obj->id.iface;
}

__SALSA_EXPORT_FUNC
unsigned int snd_hctl_elem_get_device(const snd_hctl_elem_t *obj)
{
	return obj->id.device;
}

__SALSA_EXPORT_FUNC
unsigned int snd_hctl_elem_get_subdevice(const snd_hctl_elem_t *obj)
{
	return obj->id.subdevice;
}

__SALSA_EXPORT_FUNC
const char *snd_hctl_elem_get_name(const snd_hctl_elem_t *obj)
{
	return (const char *)obj->id.name;
}

__SALSA_EXPORT_FUNC
unsigned int snd_hctl_elem_get_index(const snd_hctl_elem_t *obj)
{
	return obj->id.index;
}

__SALSA_EXPORT_FUNC
void snd_hctl_elem_set_callback(snd_hctl_elem_t *obj,
				snd_hctl_elem_callback_t val)
{
	obj->callback = val;
}

__SALSA_EXPORT_FUNC
void snd_hctl_elem_set_callback_private(snd_hctl_elem_t *obj, void * val)
{
	obj->callback_private = val;
}

__SALSA_EXPORT_FUNC
void *snd_hctl_elem_get_callback_private(const snd_hctl_elem_t *obj)
{
	return obj->callback_private;
}
__SALSA_EXPORT_FUNC
snd_hctl_elem_t *snd_hctl_first_elem(snd_hctl_t *hctl)
{
	return hctl->first_elem;
}

__SALSA_EXPORT_FUNC
snd_hctl_elem_t *snd_hctl_last_elem(snd_hctl_t *hctl)
{
	return hctl->last_elem;
}

__SALSA_EXPORT_FUNC
snd_hctl_elem_t *snd_hctl_elem_next(snd_hctl_elem_t *elem)
{
	return elem->next;
}

__SALSA_EXPORT_FUNC
snd_hctl_elem_t *snd_hctl_elem_prev(snd_hctl_elem_t *elem)
{
	return elem->prev;
}


/*
 * not implemented yet
 */

__SALSA_EXPORT_FUNC __SALSA_NOT_IMPLEMENTED
int snd_hctl_async(snd_hctl_t *hctl, int sig, pid_t pid)
{
	return -ENXIO;
}

#endif /* __ALSA_HCTL_MACROS_H */
