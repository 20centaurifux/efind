/***************************************************************************
    begin........: April 2015
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
/**
   @file buffer.c
   @brief A byte buffer.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
   @version 0.1.0
*/
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "buffer.h"
#include "utils.h"

#define RETURN_IF_INVALID(b) if(!buffer_is_valid(b)) return

#define RETURN_VAL_IF_INVALID(b, v) if(!buffer_is_valid(b)) return v

void
buffer_init(Buffer *buf)
{
	memset(buf, 0, sizeof(Buffer));

	buf->msize = 64;
	buf->data = (char *)utils_malloc(64 * sizeof(char *));
	buf->valid = true;
}

void
buffer_free(Buffer *buf)
{
	RETURN_IF_INVALID(buf);

	utils_free(buf->data);
}

void
buffer_clear(Buffer *buf)
{
	RETURN_IF_INVALID(buf);

	buf->len = 0;
}

size_t
buffer_len(const Buffer *buf)
{
	RETURN_VAL_IF_INVALID(buf, 0);

	return buf->len;
}

bool
buffer_is_valid(const Buffer *buf)
{
	return buf && buf->valid;
}

bool
buffer_is_empty(const Buffer *buf)
{
	RETURN_VAL_IF_INVALID(buf, false);

	return buf->len == 0;
}

bool
buffer_fill(Buffer *buf, const char *data, size_t len)
{
	RETURN_VAL_IF_INVALID(buf, false);

	/* test if data does fit */
	if(len > buf->msize - buf->len)
	{
		/* resize buffer */
		size_t new_size = utils_next_pow2(buf->msize + len);

		if(new_size > MAX_BUFFER_SIZE)
		{
			fprintf(stderr, "buffer exceeds maximum size");
			buf->valid = false;

			return false;
		}

		if(new_size < buf->msize)
		{
			fprintf(stderr, "overflow in buf->msize calculation");
			buf->valid = false;

			return false;
		}

		buf->msize = new_size;
		buf->data = (char *)utils_realloc(buf->data, buf->msize * sizeof(char *));
	}

	/* copy data to buffer */
	if(buf->valid)
	{
		memcpy(buf->data + buf->len, data, len);
		buf->len += len;
	}

	return buf->valid;
}

ssize_t
buffer_fill_from_fd(Buffer *buf, int fd, size_t count)
{
	ssize_t bytes;
	char data[count];

	RETURN_VAL_IF_INVALID(buf, 0);

	if((bytes = read(fd, data, count)) > 0)
	{
		if(!buffer_fill(buf, data, bytes))
		{
			bytes = 0;
		}
	}

	return bytes;
}

static void
_buffer_copy_to(Buffer *buf, size_t count, char **dst, size_t *len)
{
	if(count + 1 > *len)
	{
		*len = count;
		*dst = (char *)utils_realloc(*dst, count + 1);
	}

	memcpy(*dst, buf->data, count);
	(*dst)[count] = '\0';
}

bool
buffer_read_line(Buffer *buf, char **dst, size_t *len)
{
	char *ptr;

	RETURN_VAL_IF_INVALID(buf, false);

	/* find line-break */
	if((ptr = memchr(buf->data, '\n', buf->len)))
	{
		size_t slen = ptr - buf->data;

		/* copy found line to destination */
		_buffer_copy_to(buf, slen, dst, len);

		/* update buffer length & content */
		buf->len -= slen + 1;
		memmove(buf->data, ptr + 1, buf->len);

		return true;
	}

	return false;
}

bool
buffer_flush(Buffer *buf, char **dst, size_t *len)
{
	RETURN_VAL_IF_INVALID(buf, false);

	if(!buf->len)
	{
		return false;
	}

	_buffer_copy_to(buf, buf->len, dst, len);

	return true;
}

char *
buffer_to_string(Buffer *buf)
{
	RETURN_VAL_IF_INVALID(buf, NULL);

	if(buf->len)
	{
		char *str = utils_malloc(buf->len + 1);

		memcpy(str, buf->data, buf->len);
		str[buf->len] = '\0';

		return str;
	}

	return NULL;
}

