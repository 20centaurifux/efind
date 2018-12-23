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
   @file print.c
   @brief Print found files.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "print.h"
#include "format.h"
#include "utils.h"

/*! @cond INTERNAL */
typedef struct
{
	Processor padding;
	const char *path;
} PrintProcessor;
/*! @endcond */

static const char *
_print_processor_read(Processor *processor)
{
	assert(processor != NULL);

	processor->flags &= ~PROCESSOR_FLAG_READABLE;

	return ((PrintProcessor *)processor)->path;
}

static void
_print_processor_write(Processor *processor, const char *dir, const char *path)
{
	assert(processor != NULL);
	assert(dir != NULL);
	assert(path != NULL);

	PrintProcessor *print = (PrintProcessor *)processor;

	printf("%s\n", path);

	processor->flags |= PROCESSOR_FLAG_READABLE;
	print->path = path;
}

Processor *
print_processor_new(void)
{
	Processor *processor = (Processor *)utils_malloc(sizeof(PrintProcessor));

	memset(processor, 0, sizeof(PrintProcessor));

	processor->read = _print_processor_read;
	processor->write = _print_processor_write;

	return processor;
}

/*! @cond INTERNAL */
typedef struct
{
	Processor padding;
	FormatParserResult *format;
	const char *path;
} FormatProcessor;
/*! @endcond */


static const char *
_print_format_processor_read(Processor *processor)
{
	assert(processor != NULL);

	processor->flags &= ~PROCESSOR_FLAG_READABLE;

	return ((FormatProcessor *)processor)->path;
}

static void
_print_format_processor_write(Processor *processor, const char *dir, const char *path)
{
	assert(processor != NULL);
	assert(dir != NULL);
	assert(path != NULL);

	FormatProcessor *print = (FormatProcessor *)processor;

	format_write(print->format, dir, path, stdout);

	processor->flags |= PROCESSOR_FLAG_READABLE;
	print->path = path;
}

static void
_print_format_processor_free(Processor *processor)
{
	assert(processor != NULL);

	format_parser_result_free(((FormatProcessor *)processor)->format);
}

Processor *
print_format_processor_new(const char *format)
{
	Processor *processor = NULL;

	assert(format != NULL);

	FormatParserResult *result = format_parse(format);

	if(result->success)
	{
		processor = (Processor *)utils_malloc(sizeof(FormatProcessor));

		memset(processor, 0, sizeof(FormatProcessor));

		processor->read = _print_format_processor_read;
		processor->write = _print_format_processor_write;
		processor->free = _print_format_processor_free;

		((FormatProcessor *)processor)->format = result;
	}
	else
	{
		format_parser_result_free(result);
	}

	return processor;
}

