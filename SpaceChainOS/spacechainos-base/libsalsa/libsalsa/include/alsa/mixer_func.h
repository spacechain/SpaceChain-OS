/*
 * SALSA-Lib - Mixer abstraction layer
 * Exported functions declarations
 */

#include "hcontrol.h"

#include "pcm.h"

typedef struct _snd_mixer snd_mixer_t;
typedef struct _snd_mixer_elem snd_mixer_elem_t;
#define snd_mixer_class_t	snd_mixer_t

typedef int (*snd_mixer_callback_t)(snd_mixer_t *ctl,
				    unsigned int mask,
				    snd_mixer_elem_t *elem);

typedef int (*snd_mixer_elem_callback_t)(snd_mixer_elem_t *elem,
					 unsigned int mask);

typedef int (*snd_mixer_compare_t)(const snd_mixer_elem_t *e1,
				   const snd_mixer_elem_t *e2);

typedef enum _snd_mixer_elem_type {
	SND_MIXER_ELEM_SIMPLE,
	SND_MIXER_ELEM_LAST = SND_MIXER_ELEM_SIMPLE
} snd_mixer_elem_type_t;

typedef enum _snd_mixer_selem_channel_id {
	SND_MIXER_SCHN_UNKNOWN = -1,
	SND_MIXER_SCHN_FRONT_LEFT = 0,
	SND_MIXER_SCHN_FRONT_RIGHT,
	SND_MIXER_SCHN_REAR_LEFT,
	SND_MIXER_SCHN_REAR_RIGHT,
	SND_MIXER_SCHN_FRONT_CENTER,
	SND_MIXER_SCHN_WOOFER,
	SND_MIXER_SCHN_SIDE_LEFT,
	SND_MIXER_SCHN_SIDE_RIGHT,
	SND_MIXER_SCHN_REAR_CENTER,
	SND_MIXER_SCHN_LAST = 31,
	SND_MIXER_SCHN_MONO = SND_MIXER_SCHN_FRONT_LEFT
} snd_mixer_selem_channel_id_t;

enum snd_mixer_selem_regopt_abstract {
	SND_MIXER_SABSTRACT_NONE = 0,
	SND_MIXER_SABSTRACT_BASIC,
};

struct snd_mixer_selem_regopt {
	int ver;
	enum snd_mixer_selem_regopt_abstract abstract;
	const char *device;
	snd_pcm_t *playback_pcm;
	snd_pcm_t *capture_pcm;
};

typedef struct _snd_mixer_selem_id snd_mixer_selem_id_t;

#if SALSA_CHECK_ABI
int _snd_mixer_open(snd_mixer_t **mixer, int mode, unsigned int magic);
#else
int snd_mixer_open(snd_mixer_t **mixer, int mode);
#endif
int snd_mixer_close(snd_mixer_t *mixer);
int snd_mixer_handle_events(snd_mixer_t *mixer);
int snd_mixer_attach(snd_mixer_t *mixer, const char *name);
int snd_mixer_attach_hctl(snd_mixer_t *mixer, snd_hctl_t *hctl);
int snd_mixer_detach(snd_mixer_t *mixer, const char *name);
int snd_mixer_detach_hctl(snd_mixer_t *mixer, snd_hctl_t *hctl);
int snd_mixer_load(snd_mixer_t *mixer);
void snd_mixer_free(snd_mixer_t *mixer);

int snd_mixer_selem_register(snd_mixer_t *mixer,
			     struct snd_mixer_selem_regopt *options,
			     snd_mixer_class_t **classp);
snd_mixer_elem_t *snd_mixer_find_selem(snd_mixer_t *mixer,
				       const snd_mixer_selem_id_t *id);

int snd_mixer_selem_set_playback_switch(snd_mixer_elem_t *elem,
					snd_mixer_selem_channel_id_t channel,
					int value);
int snd_mixer_selem_set_capture_switch(snd_mixer_elem_t *elem,
				       snd_mixer_selem_channel_id_t channel,
				       int value);
int snd_mixer_selem_set_playback_switch_all(snd_mixer_elem_t *elem, int value);
int snd_mixer_selem_set_capture_switch_all(snd_mixer_elem_t *elem, int value);
int snd_mixer_selem_set_playback_volume_range(snd_mixer_elem_t *elem, 
					      long min, long max);
int snd_mixer_selem_set_capture_volume_range(snd_mixer_elem_t *elem, 
					     long min, long max);

int snd_mixer_selem_set_enum_item(snd_mixer_elem_t *elem,
				  snd_mixer_selem_channel_id_t channel,
				  unsigned int idx);
int snd_mixer_selem_get_enum_item_name(snd_mixer_elem_t *elem,
				       unsigned int idx,
				       size_t maxlen, char *str);
