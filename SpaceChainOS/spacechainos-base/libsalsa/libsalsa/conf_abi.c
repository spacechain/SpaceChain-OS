/*
 * Dummy snd_conf_* definitions
 * ABI-compatible definitions
 */

#include <stdio.h>
#include <unistd.h>
#include "recipe.h"
#include "global.h"
#include "input.h"
#include "output.h"
#undef __SALSA_EXPORT_FUNC
#define __SALSA_EXPORT_FUNC
#include "conf.h"
