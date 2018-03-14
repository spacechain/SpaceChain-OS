/*
 *  SALSA-Lib - Output Macros
 *
 *  Copyright (c) 2007-2012 by Takashi Iwai <tiwai@suse.de>
 *
 *   This library is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as
 *   published by the Free Software Foundation; either version 2.1 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#ifndef __ALSA_OUTPUT_H
#define __ALSA_OUTPUT_H

#include <stdio.h>
#include <errno.h>
#include <stdarg.h>

typedef enum _snd_output_type {
	SND_OUTPUT_STDIO,
	SND_OUTPUT_BUFFER
} snd_output_type_t;


#if SALSA_SUPPORT_OUTPUT_BUFFER

typedef struct snd_output snd_output_t;

extern int snd_output_stdio_open(snd_output_t **outputp, const char *file, const char *mode);
extern int snd_output_stdio_attach(snd_output_t **outputp, FILE *fp, int _close);
extern int snd_output_close(snd_output_t *out);
extern int snd_output_printf(snd_output_t *out, const char *fmt, ...);
extern int snd_output_puts(snd_output_t *out, const char *str);
extern int snd_output_putc(snd_output_t *out, int c);
extern int snd_output_flush(snd_output_t *out);
extern int snd_output_buffer_open(snd_output_t **outputp);
extern size_t snd_output_buffer_string(snd_output_t *output, char **buf);

#else /* SALSA_SUPPORT_OUTPUT_BUFFER */

/*
 * Simplified version; don't support the string output
 */

typedef FILE snd_output_t;

#define _SALSA_BUF_OUT	(snd_output_t *)-1

__SALSA_EXPORT_FUNC
int snd_output_stdio_open(snd_output_t **outputp, const char *file, const char *mode)
{
	if ((*outputp = fopen(file, mode)) == NULL)
		return -errno;
	return 0;
}

__SALSA_EXPORT_FUNC
int snd_output_stdio_attach(snd_output_t **outputp, FILE *fp, int _close)
{
	*outputp = fp;
	return 0;
}

__SALSA_EXPORT_FUNC
int snd_output_close(snd_output_t *out)
{
	if (out != _SALSA_BUF_OUT && out != stdout && out != stderr)
		return fclose(out);
	return 0;
}

__SALSA_EXPORT_FUNC
int snd_output_printf(snd_output_t *out, const char *fmt, ...)
{
	va_list ap;
	int ret;
	if (out == _SALSA_BUF_OUT)
		return 0;
	va_start(ap, fmt);
	ret = vfprintf(out, fmt, ap);
	va_end(ap);
	return ret;
}

__SALSA_EXPORT_FUNC
int snd_output_puts(snd_output_t *out, const char *str)
{
	if (out != _SALSA_BUF_OUT)
		return fputs(str, out);
	return 0;
}

__SALSA_EXPORT_FUNC
int snd_output_putc(snd_output_t *out, int c)
{
	if (out != _SALSA_BUF_OUT)
		return putc(c, out);
	return 0;
}

__SALSA_EXPORT_FUNC
int snd_output_flush(snd_output_t *out)
{
	if (out != _SALSA_BUF_OUT)
		return fflush(out);
	return 0;
}

__SALSA_EXPORT_FUNC __SALSA_NOT_IMPLEMENTED
int snd_output_buffer_open(snd_output_t **outputp)
{
	*outputp = _SALSA_BUF_OUT;
	return 0;
}

__SALSA_EXPORT_FUNC __SALSA_NOT_IMPLEMENTED
size_t snd_output_buffer_string(snd_output_t *output, char **buf)
{
	static char tmp[1];
	*buf = tmp;
	return 0;
}

#endif /* SALSA_SUPPORT_OUTPUT_BUFFER */

#endif /* __ALSA_OUTPUT_H */
