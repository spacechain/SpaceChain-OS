/*
 * SALSA-Lib - Raw MIDI interface
 * Exported functions declarations
 */

#include "global.h"

#define SND_RAWMIDI_APPEND	0x0001
#define SND_RAWMIDI_NONBLOCK	0x0002
#define SND_RAWMIDI_SYNC	0x0004

typedef enum _snd_rawmidi_type {
	SND_RAWMIDI_TYPE_HW,
	SND_RAWMIDI_TYPE_SHM,		/* not used by SALSA */
	SND_RAWMIDI_TYPE_INET,		/* not used by SALSA */
	SND_RAWMIDI_TYPE_VIRTUAL	/* not used by SALSA */
} snd_rawmidi_type_t;

#if SALSA_CHECK_ABI
int _snd_rawmidi_open(snd_rawmidi_t **in_rmidi, snd_rawmidi_t **out_rmidi,
		      const char *name, int mode, unsigned int magic);
#else
int snd_rawmidi_open(snd_rawmidi_t **in_rmidi, snd_rawmidi_t **out_rmidi,
		     const char *name, int mode);
#endif
int snd_rawmidi_close(snd_rawmidi_t *rmidi);

