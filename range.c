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
   @file range.c
   @brief Limit output.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */

#include <assert.h>
#include <string.h>

#include "utils.h"
#include "range.h"

/*! @cond INTERNAL */
typedef struct
{
	Processor padding;
	size_t range;
	size_t count;
	const char *path;
} RangeProcessor;
/*! @endcond */

static const char *
_limit_processor_read(Processor *processor)
{
	RangeProcessor *range = (RangeProcessor *)processor;

	assert(range != NULL);

	processor->flags &= ~PROCESSOR_FLAGS_READABLE;

	if(range->count >= range->range)
	{
		processor->flags |= PROCESSOR_FLAGS_CLOSED;
	}

	return range->path;
}

static void
_limit_processor_write(Processor *processor, const char *dir, const char *path)
{
	RangeProcessor *range = (RangeProcessor *)processor;

	assert(range != NULL);
	assert(dir != NULL);
	assert(path != NULL);

	if(range->range)
	{
		processor->flags |= PROCESSOR_FLAGS_READABLE;
		++range->count;
		range->path = path;
	}
	else
	{
		processor->flags &= ~PROCESSOR_FLAGS_READABLE;
		processor->flags |= PROCESSOR_FLAGS_CLOSED;
	}
}

static const char *
_skip_processor_read(Processor *processor)
{
	assert(processor != NULL);

	processor->flags &= ~PROCESSOR_FLAGS_READABLE;

	return ((RangeProcessor *)processor)->path;
}

static void
_skip_processor_write(Processor *processor, const char *dir, const char *path)
{
	RangeProcessor *range = (RangeProcessor *)processor;

	assert(range != NULL);
	assert(dir != NULL);
	assert(path != NULL);

	if(range->count >= range->range)
	{
		processor->flags |= PROCESSOR_FLAGS_READABLE;
		range->path = path;
	}
	else
	{
		++range->count;
		processor->flags &= ~PROCESSOR_FLAGS_READABLE;
	}
}

static Processor *
_range_processor_new(const char *(*read)(Processor *processor),
                     void (*write)(struct _Processor *processor, const char *dir, const char *path),
                     size_t range)
{
	assert(write != NULL);

	Processor *processor = (Processor *)utils_malloc(sizeof(RangeProcessor));

	memset(processor, 0, sizeof(RangeProcessor));

	processor->read = read;
	processor->write = write;

	((RangeProcessor *)processor)->count = 0;
	((RangeProcessor *)processor)->range = range;

	return processor;
}

Processor *
limit_processor_new(size_t limit)
{
	return _range_processor_new(_limit_processor_read, _limit_processor_write, limit);
}

Processor *
skip_processor_new(size_t skip)
{
	return _range_processor_new(_skip_processor_read, _skip_processor_write, skip);
}

