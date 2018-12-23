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
   @file sort.c
   @brief Sort found files.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#include <assert.h>

#include "sort.h"
#include "fileinfo.h"
#include "filelist.h"
#include "utils.h"
#include "format-fields.h"
#include "log.h"

/*! @cond INTERNAL */
typedef struct
{
	Processor padding;
	FileList *files;
	size_t offset;
} SortProcessor;
/*! @endcond */

static const char *
_sort_processor_read(Processor *processor)
{
	assert(processor != NULL);

	SortProcessor *sort = (SortProcessor *)processor;

	processor->flags |= PROCESSOR_FLAG_READABLE;

	FileListEntry *entry = file_list_at(sort->files, sort->offset);
	const char *path = entry->info->path;

	++sort->offset;

	if(sort->offset == file_list_count(sort->files))
	{
		processor->flags &= ~PROCESSOR_FLAG_READABLE;
		processor->flags |= PROCESSOR_FLAG_CLOSED;
	}

	return path;
}

static void
_sort_processor_write(Processor *processor, const char *dir, const char *path)
{
	assert(processor != NULL);
	assert(dir != NULL);
	assert(path != NULL);

	SortProcessor *sort = (SortProcessor *)processor;

	file_list_append(sort->files, dir, path);
}

static void
_sort_processor_close(Processor *processor)
{
	assert(processor != NULL);

	SortProcessor *sort = (SortProcessor *)processor;

	if(file_list_count(sort->files))
	{
		processor->flags |= PROCESSOR_FLAG_READABLE;
		file_list_sort(sort->files);
	}
	else
	{
		processor->flags |= PROCESSOR_FLAG_CLOSED;
	}
}

static void
_sort_processor_free(Processor *processor)
{
	assert(processor != NULL);

	SortProcessor *sort = (SortProcessor *)processor;

	file_list_free(sort->files);
	free(sort->files);
}

Processor *
sort_processor_new(const char *orderby)
{
	Processor *processor = NULL;

	assert(orderby != NULL);

	DEBUGF("processor", "Preparing sort string: %s", orderby);

	char *str = format_substitute(orderby);

	DEBUGF("processor", "Testing sort string: %s", str);

	if(sort_string_test(str) != -1)
	{
		processor = (Processor *)utils_malloc(sizeof(SortProcessor));

		memset(processor, 0, sizeof(SortProcessor));

		processor->read = _sort_processor_read;
		processor->write = _sort_processor_write;
		processor->close = _sort_processor_close;
		processor->free = _sort_processor_free;

		FileList *files = utils_new(1, FileList);

		file_list_init(files, str);

		((SortProcessor *)processor)->files = files;
	}
	else
	{
		DEBUGF("processor", "Found sort string is malformed: %s", str);
	}

	free(str);

	return processor;
}

