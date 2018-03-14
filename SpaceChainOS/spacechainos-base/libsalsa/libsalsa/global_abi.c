/*
 *  SALSA-Lib - Global defines
 *  ABI-compatible definitions
 */

#include "recipe.h"
#include <errno.h>
#include <signal.h>

/* async helpers */
typedef struct _snd_async_handler snd_async_handler_t;
typedef void (*snd_async_callback_t)(snd_async_handler_t *handler);

#if SALSA_HAS_ASYNC_SUPPORT

struct _snd_async_handler {
	int fd;
	snd_async_callback_t callback;
	void *private_data;
	void *rec;
	struct _snd_async_handler **pointer;
	struct _snd_async_handler *next;
};

int snd_async_handler_get_signo(snd_async_handler_t *h)
{
	return SIGIO;
}

int snd_async_handler_get_fd(snd_async_handler_t *h)
{
	return h->fd;
}

void *snd_async_handler_get_callback_private(snd_async_handler_t *h)
{
	return h->private_data;
}

#else /* !SALSA_HAS_ASYNC_SUPPORT */

int snd_async_add_handler(snd_async_handler_t **handler, int fd, 
			  snd_async_callback_t callback, void *private_data)
{
	return -ENXIO;
}

int snd_async_del_handler(snd_async_handler_t *handler)
{
	return -ENXIO;
}

#endif /* SALSA_HAS_ASYNC_SUPPORT */

#if !SALSA_HAS_DUMMY_CONF
int snd_config_update_free_global(void)
{
	return 0;
}
#endif /* !SALSA_HAS_DUMMY_CONF */
