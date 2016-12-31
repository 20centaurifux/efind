/***************************************************************************
    begin........: December 2016
    copyright....: Sebastian Fedrau
    email........: sebastian.fedrau@gmail.com
 ***************************************************************************/

/***************************************************************************
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License v3 as published by
    the Free Software Foundation.

    This program is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License v3 for more details.
 ***************************************************************************/

/***************************************************************************
   This extension for efind reads image data with GDK-PixBuf.
 ***************************************************************************/

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <linux/limits.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "extension-interface.h"

#define NAME        "GDK-PixBuf"
#define VERSION     "0.1.0"
#define DESCRIPTION "Read image data with GDK-PixBuf."

typedef struct
{
	char filename[PATH_MAX];
	int width;
	int height;
	int has_alpha;
	int bits_per_sample;
	int n_channels;
} ImageProperties;

void
registration(RegistrationCtx *ctx, RegisterExtension fn)
{
	fn(ctx, NAME, VERSION, DESCRIPTION);
}

void
discover(RegistrationCtx *ctx, RegisterCallback fn)
{
	fn(ctx, "image_width", 0);
	fn(ctx, "image_height", 0);
	fn(ctx, "image_has_alpha", 0);
	fn(ctx, "image_bits_per_sample", 0);
	fn(ctx, "image_channels", 0);
}

static ImageProperties *
_read_properties(const char *filename)
{
	static ImageProperties cached_properties = { .filename = "" };
	ImageProperties *properties = NULL;

	assert(filename != NULL);

	if(strcmp(filename, cached_properties.filename))
	{
		GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(filename, NULL);

		if(pixbuf)
		{
			g_object_get(G_OBJECT(pixbuf),
			             "width", &cached_properties.width,
			             "height", &cached_properties.height,
			             "has-alpha", &cached_properties.has_alpha,
			             "bits-per-sample", &cached_properties.bits_per_sample,
			             "n-channels", &cached_properties.n_channels,
			             NULL); 
			g_object_unref(pixbuf);

			properties = &cached_properties;
		}
	}
	else
	{
		properties = &cached_properties;
	}

	return properties;
}

int
image_width(const char *filename, int argc, void *argv[])
{
	ImageProperties *props;

	if((props = _read_properties(filename)))
	{
		return props->width;
	}

	return 0;
}

int
image_height(const char *filename, int argc, void *argv[])
{
	ImageProperties *props;

	if((props = _read_properties(filename)))
	{
		return props->height;
	}

	return 0;
}

int
image_has_alpha(const char *filename, int argc, void *argv[])
{
	ImageProperties *props;

	if((props = _read_properties(filename)))
	{
		return props->has_alpha;
	}

	return 0;
}

int
image_bits_per_sample(const char *filename, int argc, void *argv[])
{
	ImageProperties *props;

	if((props = _read_properties(filename)))
	{
		return props->bits_per_sample;
	}

	return 0;
}

int
image_channels(const char *filename, int argc, void *argv[])
{
	ImageProperties *props;

	if((props = _read_properties(filename)))
	{
		return props->n_channels;
	}

	return 0;
}

