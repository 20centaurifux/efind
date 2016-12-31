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
	int channels;
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
	static ImageProperties cache = { .filename = "" };
	ImageProperties *properties = NULL;

	assert(filename != NULL);

	if(strcmp(filename, cache.filename))
	{
		GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(filename, NULL);

		memset(&cache, 0, sizeof(ImageProperties));

		if(pixbuf)
		{
			g_object_get(G_OBJECT(pixbuf),
			             "width", &cache.width,
			             "height", &cache.height,
			             "has-alpha", &cache.has_alpha,
			             "bits-per-sample", &cache.bits_per_sample,
			             "n-channels", &cache.channels,
			             NULL); 
			g_object_unref(pixbuf);

			strncpy(cache.filename, filename, PATH_MAX);
			properties = &cache;
		}
	}
	else
	{
		properties = &cache;
	}

	return properties;
}

#define GET_IMAGE_PROPERTY(field) \
int \
image_##field(const char *filename, int argc, void *argv[]) \
{ \
	ImageProperties *props; \
\
	if((props = _read_properties(filename))) \
	{ \
		return props->field; \
	} \
\
	return 0; \
}

GET_IMAGE_PROPERTY(width)
GET_IMAGE_PROPERTY(height)
GET_IMAGE_PROPERTY(has_alpha)
GET_IMAGE_PROPERTY(bits_per_sample)
GET_IMAGE_PROPERTY(channels)

