/*
 * SALSA-Lib - Input Macros
 * ABI-compatible definitions
 */

#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>

typedef FILE snd_input_t;

int snd_input_stdio_open(snd_input_t **inputp, const char *file, const char *mode)
{
	if ((*inputp = fopen(file, mode)) == NULL)
		return -errno;
	return 0;
}

int snd_input_stdio_attach(snd_input_t **inputp, FILE *fp, int _close)
{
	*inputp = fp;
	return 0;
}
	
int snd_input_close(snd_input_t *input)
{
	return fclose(input);
}

int snd_input_scanf(snd_input_t *input, const char *fmt, ...)
{
	va_list ap;
	int ret;
	va_start(ap, fmt);
	ret = vfscanf(input, fmt, ap);
	va_end(ap);
	return ret;
}

char *snd_input_gets(snd_input_t *input, char *str, int size)
{
	return fgets(str, size, input);
}

int snd_input_getc(snd_input_t *input)
{
	return getc(input);
}

int snd_input_ungetc(snd_input_t *input, int c)
{
	return ungetc(c, input);
}

int snd_input_buffer_open(snd_input_t **inputp, const char *buffer,
			  ssize_t size)
{
	return -ENXIO;
}
