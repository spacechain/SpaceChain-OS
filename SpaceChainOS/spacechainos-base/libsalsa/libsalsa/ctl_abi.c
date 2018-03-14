/*
 *  SALSA-Lib - Control Interface
 *  ABI-compatible definitions
 */

#include "recipe.h"
#include "ctl_func.h"
#undef __SALSA_EXPORT_FUNC
#define __SALSA_EXPORT_FUNC
#include "ctl_macros.h"

#undef snd_ctl_elem_type_name
const char *snd_ctl_elem_type_name(snd_ctl_elem_type_t type)
{
	return _snd_ctl_elem_type_names[type];
}

#undef snd_ctl_elem_iface_name
const char *snd_ctl_elem_iface_name(snd_ctl_elem_iface_t iface)
{
	return _snd_ctl_elem_iface_names[iface];
}

#undef snd_ctl_event_type_name
const char *snd_ctl_event_type_name(snd_ctl_event_type_t type)
{
	return _snd_ctl_event_type_names[type];
}
