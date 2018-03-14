/*
 * SALSA-Lib - Control Interface
 *
 * Declarations of control functions
 */

#include "global.h"
#include "asound.h"

#include <unistd.h>
#include <stdint.h>

typedef enum _snd_ctl_type {
	SND_CTL_TYPE_HW,
	SND_CTL_TYPE_SHM,	/* not used by SALSA */
	SND_CTL_TYPE_INET,	/* not used by SALSA */
	SND_CTL_TYPE_EXT	/* not used by SALSA */
} snd_ctl_type_t;

#define SND_CTL_NONBLOCK		0x0001
#define SND_CTL_ASYNC			0x0002
#define SND_CTL_READONLY		0x0004

int snd_card_load(int card);
int snd_card_next(int *card);
int snd_card_get_index(const char *name);
int snd_card_get_name(int card, char **name);
int snd_card_get_longname(int card, char **name);

#if SALSA_CHECK_ABI
int _snd_ctl_open(snd_ctl_t **ctl, const char *name, int mode,
		  unsigned int magic);
#else
int snd_ctl_open(snd_ctl_t **ctl, const char *name, int mode);
#endif
int snd_ctl_close(snd_ctl_t *ctl);
int snd_ctl_wait(snd_ctl_t *ctl, int timeout);

#if SALSA_HAS_TLV_SUPPORT
int snd_ctl_elem_tlv_read(snd_ctl_t *ctl, const snd_ctl_elem_id_t *id,
			  unsigned int *tlv, unsigned int tlv_size);
int snd_ctl_elem_tlv_write(snd_ctl_t *ctl, const snd_ctl_elem_id_t *id,
			   const unsigned int *tlv);
int snd_ctl_elem_tlv_command(snd_ctl_t *ctl, const snd_ctl_elem_id_t *id,
			     const unsigned int *tlv);
#endif

#if SALSA_HAS_ASYNC_SUPPORT
int snd_async_add_ctl_handler(snd_async_handler_t **handler, snd_ctl_t *ctl, 
			      snd_async_callback_t callback,
			      void *private_data);
#endif

#if SALSA_HAS_USER_ELEM_SUPPORT
int snd_ctl_elem_add_integer(snd_ctl_t *ctl, const snd_ctl_elem_id_t *id,
			     unsigned int count, long min, long max, long step);
int snd_ctl_elem_add_integer64(snd_ctl_t *ctl, const snd_ctl_elem_id_t *id,
			       unsigned int count, long long min, long long max,
			       long long step);
int snd_ctl_elem_add_boolean(snd_ctl_t *ctl, const snd_ctl_elem_id_t *id,
			     unsigned int count);
int snd_ctl_elem_add_iec958(snd_ctl_t *ctl, const snd_ctl_elem_id_t *id);
int snd_ctl_elem_add_enumerated(snd_ctl_t *ctl, const snd_ctl_elem_id_t *id,
				unsigned int count, unsigned int items,
				const char *const names[]);
#endif

#if SALSA_HAS_TLV_SUPPORT
int snd_tlv_parse_dB_info(unsigned int *tlv,
			  unsigned int tlv_size,
			  unsigned int **db_tlvp);
int snd_tlv_get_dB_range(unsigned int *tlv, long rangemin, long rangemax,
			 long *min, long *max);
int snd_tlv_convert_to_dB(unsigned int *tlv, long rangemin, long rangemax,
			  long volume, long *db_gain);
int snd_tlv_convert_from_dB(unsigned int *tlv, long rangemin, long rangemax,
			    long db_gain, long *value, int xdir);
int snd_ctl_get_dB_range(snd_ctl_t *ctl, const snd_ctl_elem_id_t *id,
			 long *min, long *max);
int snd_ctl_convert_to_dB(snd_ctl_t *ctl, const snd_ctl_elem_id_t *id,
			  long volume, long *db_gain);
int snd_ctl_convert_from_dB(snd_ctl_t *ctl, const snd_ctl_elem_id_t *id,
			    long db_gain, long *value, int xdir);
#endif

