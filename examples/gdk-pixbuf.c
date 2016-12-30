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
#include <assert.h>

#include "extension-interface.h"

#define NAME        "GDK-PixBuf"
#define VERSION     "0.1.0"
#define DESCRIPTION "Read image data with GDK-PixBuf."

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

static int
_read_property(const char *filename, const char *prop)
{
	GdkPixbuf *pixbuf;
	int ret = -1;

	assert(filename != NULL);
	assert(prop != NULL);

	pixbuf = gdk_pixbuf_new_from_file(filename, NULL);

	if(pixbuf)
	{
		g_object_get(G_OBJECT(pixbuf), prop, &ret, NULL);
		g_object_unref(pixbuf);
	}

	return ret;
}

int
image_width(const char *filename, int argc, void *argv[])
{
	return _read_property(filename, "width");
}

int
image_height(const char *filename, int argc, void *argv[])
{
	return _read_property(filename, "height");
}

int
image_has_alpha(const char *filename, int argc, void *argv[])
{
	return _read_property(filename, "has-alpha");
}

int
image_bits_per_sample(const char *filename, int argc, void *argv[])
{
	return _read_property(filename, "bits-per-sample");
}

int
image_channels(const char *filename, int argc, void *argv[])
{
	return _read_property(filename, "n-channels");
}

