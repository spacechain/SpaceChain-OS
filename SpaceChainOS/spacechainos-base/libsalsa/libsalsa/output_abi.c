/*
 * SALSA-Lib - Output Macros
 * ABI-compatible definitions
 */

#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "recipe.h"
#include "global.h"
#undef __SALSA_EXPORT_FUNC
#define __SALSA_EXPORT_FUNC
#include "output.h"

#if SALSA_SUPPORT_OUTPUT_BUFFER

#define CHUNK 4000

struct snd_output {
	int mode;
	void *ptr;
	int len;
	int size;
};

int snd_output_stdio_attach(snd_output_t **outputp, FILE *fp, int _close)
{
	snd_output_t *out;
	out = malloc(sizeof(*out));
	if (!out)
		return -ENOMEM;
	out->mode = SND_OUTPUT_STDIO;
	out->ptr = fp;
	out->len = _close;
	*outputp = out;
	return 0;
}

int snd_output_stdio_open(snd_output_t **outputp, const char *file, const char *mode)
{
	FILE *fp = fopen(file, mode);
	if (!fp)
		return -errno;
	return snd_output_stdio_attach(outputp, fp, 1);
}

int snd_output_close(snd_output_t *out)
{
	if (out->mode == SND_OUTPUT_STDIO) {
		if (out->len)
			return fclose(out->ptr);
	} else
		free(out->ptr);
	free(out);
	return 0;
}

static int resize(snd_output_t *out, int new)
{
	int size = out->len + new + CHUNK;
	void *buf = realloc(out->ptr, size);
	if (!buf)
		return -ENOMEM;
	out->ptr = buf;
	out->size = size;
	return 0;
}

int snd_output_printf(snd_output_t *out, const char *fmt, ...)
{
	va_list ap;
	int ret;
	va_start(ap, fmt);
	if (out->mode == SND_OUTPUT_STDIO)
		ret = vfprintf(out->ptr, fmt, ap);
	else {
		ret = vsnprintf(out->ptr + out->len, out->size - out->len,
				fmt, ap);
		if (ret >= out->size - out->len) {
			if (resize(out, ret) < 0)
				return -ENOMEM;
			ret = vsnprintf(out->ptr + out->len,
					out->size - out->len,
					fmt, ap);
		}
	}
	va_end(ap);
	return ret;
}

int snd_output_puts(snd_output_t *out, const char *str)
{
	if (out->mode == SND_OUTPUT_STDIO)
		return fputs(str, out->ptr);
	else {
		int len = strlen(str);
		if (out->size - out->len <= len && resize(out, len) < 0)
			return -ENOMEM;
		strcpy(out->ptr + len, str);
		return len;
	}
}

int snd_output_putc(snd_output_t *out, int c)
{
	if (out->mode == SND_OUTPUT_STDIO)
		return putc(c, out->ptr);
	if (out->len + 1 >= out->size && resize(out, 1) < 0)
		return -ENOMEM;
	((char *)out->ptr)[out->len++] = c;
	return 0;
}

int snd_output_flush(snd_output_t *out)
{
	if (out->mode == SND_OUTPUT_STDIO)
		return fflush(out->ptr);
	else
		out->len = 0;
	return 0;
}

int snd_output_buffer_open(snd_output_t **outputp)
{
	snd_output_t *out = calloc(1, sizeof(*out));
	if (!out)
		return -ENOMEM;
	out->mode = SND_OUTPUT_BUFFER;
	*outputp = out;
	return resize(out, 0);
}

size_t snd_output_buffer_string(snd_output_t *out, char **buf)
{
	*buf = out->ptr;
	return out->len;
}

#endif /* SALSA_SUPPORT_OUTPUT_BUFFER */
