/*
 * SALSA-Lib - Hardware Dependent Interface
 * Declarations of exported functions
 */

#include "global.h"

#define SND_HWDEP_OPEN_READ		(O_RDONLY)
#define SND_HWDEP_OPEN_WRITE		(O_WRONLY)
#define SND_HWDEP_OPEN_DUPLEX		(O_RDWR)
#define SND_HWDEP_OPEN_NONBLOCK		(O_NONBLOCK)

typedef enum _snd_hwdep_type {
	SND_HWDEP_TYPE_HW,
	SND_HWDEP_TYPE_SHM,	/* not used by SALSA */
	SND_HWDEP_TYPE_INET	/* not used by SALSA */
} snd_hwdep_type_t;

int snd_hwdep_open(snd_hwdep_t **hwdep, const char *name, int mode);
int snd_hwdep_close(snd_hwdep_t *hwdep);
