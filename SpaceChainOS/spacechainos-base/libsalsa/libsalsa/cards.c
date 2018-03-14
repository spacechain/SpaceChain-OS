/*
 *  SALSA-Lib - Global card functions
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "control.h"
#include "local.h"

/* common helper function to set nonblock mode
 * called from various *_macros.h
 */
int _snd_set_nonblock(int fd, int nonblock)
{
	long flags;
	flags = fcntl(fd, F_GETFL);
	if (flags < 0)
		return -errno;
	if (nonblock)
		flags |= O_NONBLOCK;
	else
		flags &= ~O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flags) < 0)
		return -errno;
	return 0;
}

#define SND_FILE_CONTROL	SALSA_DEVPATH "/controlC%i"

#define fill_control_name(name, card)		\
	sprintf(name, SND_FILE_CONTROL, card);

int snd_card_load(int card)
{
	char control[sizeof(SND_FILE_CONTROL) + 10];
	fill_control_name(control, card);
	return !access(control, R_OK);
}

int snd_card_next(int *rcard)
{
	int card;
	
	if (rcard == NULL)
		return -EINVAL;
	card = *rcard;
	card = card < 0 ? 0 : card + 1;
	for (; card < SALSA_MAX_CARDS; card++) {
		if (snd_card_load(card)) {
			*rcard = card;
			return 0;
		}
	}
	*rcard = -1;
	return 0;
}

/* open the substream with the given subdevice number */
int _snd_open_subdev(const char *filename, int fmode,
		     int card, int subdev, unsigned int prefer_ioctl)
{
	char control[sizeof(SND_FILE_CONTROL) + 10];
	int ctl, fd;

	fill_control_name(control, card);
	ctl = open(control, O_RDWR);
	if (ctl < 0)
		return -errno;
	if (ioctl(ctl, prefer_ioctl, &subdev) >= 0)
		fd = -1;
	else
		fd = open(filename, fmode);
	close(ctl);
	return fd < 0 ? -errno : fd;
}

static int load_card_info(const char *control, snd_ctl_card_info_t *info)
{
	int open_dev;

	memzero_valgrind(info, sizeof(*info));
	open_dev = open(control, O_RDONLY);
	if (open_dev < 0)
		return -errno;
	if (ioctl(open_dev, SNDRV_CTL_IOCTL_CARD_INFO, info) < 0) {
		close(open_dev);
		return -errno;
	}
	close(open_dev);
	return 0;
}

static int snd_card_load2(const char *control)
{
	snd_ctl_card_info_t info;
	int err = load_card_info(control, &info);
	if (err < 0)
		return err;
	return info.card;
}

static int get_card_info(int card, snd_ctl_card_info_t *info)
{
	char control[sizeof(SND_FILE_CONTROL) + 10];
	fill_control_name(control, card);
	return load_card_info(control, info);
}

int snd_card_get_index(const char *string)
{
	int card;
	snd_ctl_card_info_t info;

	if (!string || *string == '\0')
		return -EINVAL;
	if (*string == '/') /* device name */
		return snd_card_load2(string);
	if (sscanf(string, "%i", &card) == 1) {
		if (card < 0 || card >= SALSA_MAX_CARDS)
			return -EINVAL;
		if (snd_card_load(card))
			return card;
		return -ENODEV;
	}
	for (card = 0; card < SALSA_MAX_CARDS; card++) {
		if (get_card_info(card, &info) < 0)
			continue;
		if (!strcmp((const char *)info.id, string))
			return card;
	}
	return -ENODEV;
}

int snd_card_get_name(int card, char **name)
{
	snd_ctl_card_info_t info;
	int err;
	
	err = get_card_info(card, &info);
	if (err < 0)
		return err;
	*name = strdup((const char *)info.name);
	if (*name == NULL)
		return -ENOMEM;
	return 0;
}

int snd_card_get_longname(int card, char **name)
{
	snd_ctl_card_info_t info;
	int err;
	
	err = get_card_info(card, &info);
	if (err < 0)
		return err;
	*name = strdup((const char *)info.longname);
	if (*name == NULL)
		return -ENOMEM;
	return 0;
}

/* very simple device name parsing; only limited prefix are allowed:
 * "hw" and "default"
 */
int _snd_dev_get_device(const char *name, int *cardp, int *devp, int *subdevp)
{
	int card;
	*cardp = 0;
	if (devp)
		*devp = 0;
	if (subdevp)
		*subdevp = -1;
	if (!strcmp(name, "hw") || !strcmp(name, "default") ||
	    !strcmp(name, "sysdefault"))
		return 0;
	if (!strncmp(name, "hw:", 3))
		name += 3;
	else if (!strncmp(name, "default:", 8))
		name += 8;
	else if (!strncmp(name, "sysdefault:", 11))
		name += 11;
	else
		return -EINVAL;
	card = snd_card_get_index(name);
	if (card < 0)
		return card;
	*cardp = card;
	if (devp && subdevp) {
		/* parse the secondary and third arguments (if any) */
		name = strchr(name, ',');
		if (name) {
			sscanf(name, ",%d,%d",  devp, subdevp);
			if (*devp < 0 || *devp >= SALSA_MAX_DEVICES)
				return -EINVAL;
		}
	}
	return 0;
}

snd_config_t *snd_config; /* placeholder */
