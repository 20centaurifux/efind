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
   @file filelist.c
   @brief Sortable file list.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "filelist.h"
#include "log.h"
#include "utils.h"
#include "gettext.h"

/*! Supported fields to sort search result by. */
#define SORTABLE_FIELDS "bfgGhiklmMnpsSuUyYpPHFDaAcCtT"

int
sort_string_test(const char *str)
{
	const char *ptr = str;
	int result = 0;

	while(result >= 0 && ptr && *ptr)
	{
		if(*ptr == ' ')
		{
			++ptr;
			continue;
		}

		if(*ptr == '-')
		{
			++ptr;
		}

		if(*ptr)
		{
			if(strchr(SORTABLE_FIELDS, *ptr))
			{
				if(result < INT_MAX)
				{
					++result;
				}
			}
			else
			{
				result = -1;
			}

			++ptr;
		}
		else
		{
			result = -1;
		}
	}

	return result;
}

const char
*sort_string_pop(const char *str, char *field, bool *asc)
{
	const char *ptr = str;
	const char *rest = NULL;

	assert(field != NULL);
	assert(asc != NULL);

	*field = '\0';
	*asc = true;

	if(ptr)
	{
		/* remove whitespace */
		while(*ptr && *ptr == ' ')
		{
			++ptr;
		}

		/* read field name & sort direction */
		if(*ptr)
		{
			if(*ptr == '-')
			{
				*asc = false;
				++ptr;
			}

			if(*ptr && strchr(SORTABLE_FIELDS, *ptr))
			{
				*field = *ptr;
				++ptr;
			}
			else
			{
				fprintf(stderr, _("Found unexpected character in sort string: '%c'.\n"), *ptr);
			}

			/* find next character */
			while(*ptr && *ptr == ' ')
			{
				++ptr;
			}

			if(*ptr)
			{
				rest = ptr;
			}
		}
	}

	return rest;
}

void
file_list_init(FileList *list, const char *cli, const char *orderby)
{
	size_t item_size = sizeof(FileListEntry);

	assert(list != NULL);
	assert(cli != NULL);

	memset(list, 0, sizeof(FileList));

	list->cli = utils_strdup(cli);
	list->entries = utils_new(512, FileListEntry *);
	list->size = 512;

	if(sizeof(FileInfo) > item_size)
	{
		item_size = sizeof(FileInfo);
	}

	list->alloc = (Allocator *)chunk_allocator_new(item_size, 1024);

	if(orderby)
	{
		DEBUGF("filelist", "Testing sort string: %s", orderby);

		int count = sort_string_test(orderby);

		DEBUGF("filelist", "Found %d field(s) to sort.", count);

		if(count != -1)
		{
			list->fields_n = count;

			if(count > 0)
			{
				size_t offset = 0;

				list->fields = utils_new(count, char);
				list->fields_asc = utils_new(count, bool);

				const char *rest = sort_string_pop(orderby, list->fields, list->fields_asc);

				while(rest)
				{
					TRACEF("filelist", "Found field '%c', ascending=%d.", list->fields[offset], list->fields_asc[offset]);
					++offset;
					rest = sort_string_pop(rest, &list->fields[offset], &list->fields_asc[offset]);
				}
			}
		}
		else
		{
			fprintf(stderr, _("Couldn't parse sort string.\n"));
		}
	}
}

static void
_file_list_entry_free(FileListEntry *entry)
{
	if(entry && entry->info)
	{
		file_info_clear(entry->info);
	}
}

static FileListEntry *
_file_list_entry_new(FileList *list)
{
	FileListEntry *entry;

	assert(list != NULL);

	entry = list->alloc->alloc(list->alloc);
	memset(entry, 0, sizeof(FileListEntry));
	entry->filesp = list;

	return entry;
}

static FileListEntry *
_file_list_entry_new_from_path(FileList *list, const char *path)
{
	FileListEntry *entry = NULL;
	FileInfo info;

	assert(list != NULL);
	assert(path != NULL);

	file_info_init(&info);

	if(file_info_get(&info, list->cli, path))
	{
		entry = _file_list_entry_new(list);
		entry->info = list->alloc->alloc(list->alloc);
		memcpy(entry->info, &info, sizeof(FileInfo));
	}
	else
	{
		DEBUGF("fileinfo", "Couldn't read details of file %s.", path);;
	}

	return entry;
}

void
file_list_free(FileList *list)
{
	if(list)
	{
		free(list->cli);
		free(list->fields);
		free(list->fields_asc);

		for(size_t i = 0; i < list->count; ++i)
		{
			_file_list_entry_free(list->entries[i]);
		}

		chunk_allocator_destroy((ChunkAllocator *)list->alloc);
		free(list->entries);
	}
}

bool
file_list_append(FileList *list, const char *path)
{
	FileListEntry *entry = NULL;
	bool success = false;

	assert(list != NULL);
	assert(path != NULL);

	TRACEF("filelist", "Appending file: %s", path);

	/* resize entry array */
	if(list->size == list->count)
	{
		list->size *= 2;

		if(list->size > list->count)
		{
			list->entries = utils_renew(list->entries, list->size, FileListEntry *);
		}
		else
		{
			FATAL("filelist", "Integer overflow.");
			abort();
		}
	}

	/* append entry */
	entry = _file_list_entry_new_from_path(list, path);

	if(entry)
	{
		list->entries[list->count] = entry;
		list->count++;
		success = true;
	}
	else
	{
		DEBUG("filelist", "Couldn't create list entry.");
	}

	return success;
}

static int
_file_list_compare_entries(const void *a, const void *b)
{
	int result = 0;

	assert(a != NULL);
	assert(b != NULL);

	FileListEntry *first = (*((FileListEntry **)a));
	FileListEntry *second = (*((FileListEntry **)b));

	for(int i = 0; i < first->filesp->fields_n; ++i)
	{
		FileAttr attr_a;

		if(file_info_get_attr(first->info, &attr_a, first->filesp->fields[i]))
		{
			FileAttr attr_b;

			if(file_info_get_attr(second->info, &attr_b, second->filesp->fields[i]))
			{
				result = file_attr_compare(&attr_a, &attr_b);
				file_attr_free(&attr_b);
			}
			else
			{
				fprintf(stderr, _("Couldn't read attribute from file %s.\n"), second->info->path);
			}

			file_attr_free(&attr_a);
		}
		else
		{
			fprintf(stderr, _("Couldn't read attribute from file %s.\n"), first->info->path);
		}

		if(result)
		{
			if(!first->filesp->fields_asc[i])
			{
				result *= -1;
			}

			break;
		}
	}

	return result;
}

void
file_list_sort(FileList *list)
{
	assert(list != NULL);

	DEBUG("filelist", "Sorting file list.");

	qsort(list->entries, list->count, sizeof(FileListEntry *), &_file_list_compare_entries);
}

void
file_list_foreach(FileList *list, Callback f, void *user_data)
{
	assert(list != NULL);
	assert(f != NULL);

	for(size_t i = 0; i < list->count; ++i)
	{
		f(list->entries[i]->info->path, user_data);
	}
}

