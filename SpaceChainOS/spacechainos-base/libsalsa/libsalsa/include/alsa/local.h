#ifndef __LOCAL_H_INC
#define __LOCAL_H_INC

#define SALSA_MAX_CARDS		32
#define SALSA_MAX_DEVICES	32

int _snd_dev_get_device(const char *name, int *cardp, int *devp, int *subdevp);
int _snd_open_subdev(const char *filename, int fmode,
		     int card, int subdev, unsigned int prefer_ioctl);

int _snd_pcm_mmap(snd_pcm_t *pcm);
int _snd_pcm_munmap(snd_pcm_t *pcm);

#ifdef DELIGHT_VALGRIND
#define memzero_valgrind(buf, size)	memset(buf, 0, size)
#else
#define memzero_valgrind(buf, size)
#endif

#if SALSA_CHECK_ABI
#include <assert.h>
#define check_incompatible_abi(magic, orig) assert(magic == orig)
#else
#define check_incompatible_abi(magic, orig)
#endif

#endif /* __LOCAL_H_INC */
