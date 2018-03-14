/*
 * SALSA-Lib - PCM Interface
 *
 * PCM declarations
 */

#include "global.h"
#include "asound.h"
#include "output.h"

#include <unistd.h>
#include <stdint.h>

typedef snd_mask_t snd_pcm_access_mask_t;
typedef snd_mask_t snd_pcm_format_mask_t;
typedef snd_mask_t snd_pcm_subformat_mask_t;

/* deprecated */
typedef enum _snd_pcm_start {
	SND_PCM_START_DATA = 0,
	SND_PCM_START_EXPLICIT,
	SND_PCM_START_LAST = SND_PCM_START_EXPLICIT
} snd_pcm_start_t;

/* deprecated */
typedef enum _snd_pcm_xrun {
	SND_PCM_XRUN_NONE = 0,
	SND_PCM_XRUN_STOP,
	SND_PCM_XRUN_LAST = SND_PCM_XRUN_STOP
} snd_pcm_xrun_t;

#define SND_PCM_NONBLOCK		0x00000001
#define SND_PCM_ASYNC			0x00000002
#define SND_PCM_NO_AUTO_RESAMPLE	0x00010000
#define SND_PCM_NO_AUTO_CHANNELS	0x00020000
#define SND_PCM_NO_AUTO_FORMAT		0x00040000
#define SND_PCM_NO_SOFTVOL		0x00080000

typedef enum _snd_pcm_type {
	SND_PCM_TYPE_HW = 0,
	/* the rest are not supported by SALSA (of course!) */
	SND_PCM_TYPE_HOOKS,
	SND_PCM_TYPE_MULTI,
	SND_PCM_TYPE_FILE,
	SND_PCM_TYPE_NULL,
	SND_PCM_TYPE_SHM,
	SND_PCM_TYPE_INET,
	SND_PCM_TYPE_COPY,
	SND_PCM_TYPE_LINEAR,
	SND_PCM_TYPE_ALAW,
	SND_PCM_TYPE_MULAW,
	SND_PCM_TYPE_ADPCM,
	SND_PCM_TYPE_RATE,
	SND_PCM_TYPE_ROUTE,
	SND_PCM_TYPE_PLUG,
	SND_PCM_TYPE_SHARE,
	SND_PCM_TYPE_METER,
	SND_PCM_TYPE_MIX,
	SND_PCM_TYPE_DROUTE,
	SND_PCM_TYPE_LBSERVER,
	SND_PCM_TYPE_LINEAR_FLOAT,
	SND_PCM_TYPE_LADSPA,
	SND_PCM_TYPE_DMIX,
	SND_PCM_TYPE_JACK,
	SND_PCM_TYPE_DSNOOP,
	SND_PCM_TYPE_DSHARE,
	SND_PCM_TYPE_IEC958,
	SND_PCM_TYPE_SOFTVOL,
	SND_PCM_TYPE_IOPLUG,
	SND_PCM_TYPE_EXTPLUG,
	SND_PCM_TYPE_LAST = SND_PCM_TYPE_EXTPLUG
} snd_pcm_type_t;

typedef struct _snd_pcm_channel_area {
	void *addr;
	unsigned int first;
	unsigned int step;
} snd_pcm_channel_area_t;

#if SALSA_CHECK_ABI
int _snd_pcm_open(snd_pcm_t **pcm, const char *name, 
		  snd_pcm_stream_t stream, int mode, unsigned int magic);
#else
int snd_pcm_open(snd_pcm_t **pcm, const char *name, 
		 snd_pcm_stream_t stream, int mode);
#endif
int snd_pcm_close(snd_pcm_t *pcm);
int snd_pcm_hw_params(snd_pcm_t *pcm, snd_pcm_hw_params_t *params);
int snd_pcm_hw_free(snd_pcm_t *pcm);
int snd_pcm_hw_params_any(snd_pcm_t *pcm, snd_pcm_hw_params_t *params);
int snd_pcm_hw_params_get_min_align(const snd_pcm_hw_params_t *params,
				    snd_pcm_uframes_t *val);
int snd_pcm_sw_params(snd_pcm_t *pcm, snd_pcm_sw_params_t *params);

snd_pcm_sframes_t snd_pcm_avail_update(snd_pcm_t *pcm);
int snd_pcm_avail_delay(snd_pcm_t *pcm, snd_pcm_sframes_t *availp,
			snd_pcm_sframes_t *delayp);
snd_pcm_sframes_t snd_pcm_writei(snd_pcm_t *pcm, const void *buffer,
				 snd_pcm_uframes_t size);
snd_pcm_sframes_t snd_pcm_readi(snd_pcm_t *pcm, void *buffer,
				snd_pcm_uframes_t size);
snd_pcm_sframes_t snd_pcm_writen(snd_pcm_t *pcm, void **bufs,
				 snd_pcm_uframes_t size);
snd_pcm_sframes_t snd_pcm_readn(snd_pcm_t *pcm, void **bufs,
				snd_pcm_uframes_t size);
int snd_pcm_wait(snd_pcm_t *pcm, int timeout);

int snd_pcm_recover(snd_pcm_t *pcm, int err, int silent);

int snd_pcm_dump(snd_pcm_t *pcm, snd_output_t *out);
int snd_pcm_dump_hw_setup(snd_pcm_t *pcm, snd_output_t *out);
int snd_pcm_dump_sw_setup(snd_pcm_t *pcm, snd_output_t *out);
int snd_pcm_dump_setup(snd_pcm_t *pcm, snd_output_t *out);
int snd_pcm_hw_params_dump(snd_pcm_hw_params_t *params, snd_output_t *out);
int snd_pcm_sw_params_dump(snd_pcm_sw_params_t *params, snd_output_t *out);
int snd_pcm_status_dump(snd_pcm_status_t *status, snd_output_t *out);

int snd_pcm_mmap_begin(snd_pcm_t *pcm,
		       const snd_pcm_channel_area_t **areas,
		       snd_pcm_uframes_t *offset,
		       snd_pcm_uframes_t *frames);
snd_pcm_sframes_t snd_pcm_mmap_commit(snd_pcm_t *pcm,
				      snd_pcm_uframes_t offset,
				      snd_pcm_uframes_t frames);

snd_pcm_format_t snd_pcm_build_linear_format(int width, int pwidth,
					     int unsignd, int big_endian);
int snd_pcm_format_set_silence(snd_pcm_format_t format, void *buf,
			       unsigned int samples);
snd_pcm_format_t snd_pcm_format_value(const char* name);

int snd_pcm_area_silence(const snd_pcm_channel_area_t *dst_channel,
			 snd_pcm_uframes_t dst_offset,
			 unsigned int samples, snd_pcm_format_t format);
int snd_pcm_areas_silence(const snd_pcm_channel_area_t *dst_channels,
			  snd_pcm_uframes_t dst_offset,
			  unsigned int channels, snd_pcm_uframes_t frames,
			  snd_pcm_format_t format);
int snd_pcm_area_copy(const snd_pcm_channel_area_t *dst_channel,
		      snd_pcm_uframes_t dst_offset,
		      const snd_pcm_channel_area_t *src_channel,
		      snd_pcm_uframes_t src_offset,
		      unsigned int samples, snd_pcm_format_t format);
int snd_pcm_areas_copy(const snd_pcm_channel_area_t *dst_channels,
		       snd_pcm_uframes_t dst_offset,
		       const snd_pcm_channel_area_t *src_channels,
		       snd_pcm_uframes_t src_offset,
		       unsigned int channels, snd_pcm_uframes_t frames,
		       snd_pcm_format_t format);

#if SALSA_HAS_ASYNC_SUPPORT
int snd_async_add_pcm_handler(snd_async_handler_t **handler, snd_pcm_t *pcm, 
			      snd_async_callback_t callback,
			      void *private_data);
#endif

#if SALSA_HAS_CHMAP_SUPPORT
void snd_pcm_free_chmaps(snd_pcm_chmap_query_t **maps);
snd_pcm_chmap_query_t **
snd_pcm_query_chmaps_from_hw(int card, int dev, int subdev,
			     snd_pcm_stream_t stream);
snd_pcm_chmap_t *snd_pcm_get_chmap(snd_pcm_t *pcm);
int snd_pcm_set_chmap(snd_pcm_t *pcm, const snd_pcm_chmap_t *map);
int snd_pcm_chmap_print(const snd_pcm_chmap_t *map, size_t maxlen, char *buf);
unsigned int snd_pcm_chmap_from_string(const char *str);
snd_pcm_chmap_t *snd_pcm_chmap_parse_string(const char *str);
#endif
