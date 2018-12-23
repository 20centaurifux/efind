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
	int result = 0;

	assert(str != NULL);

	const char *ptr = str;

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

static const char *
_sort_string_pop(const char *str, char *field, bool *asc)
{
	const char *rest = NULL;

	assert(field != NULL);
	assert(asc != NULL);

	*field = '\0';
	*asc = true;

	const char *ptr = str;

	if(ptr)
	{
		/* remove whitespace */
		while(*ptr == ' ')
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
			while(*ptr == ' ')
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
file_list_init(FileList *list, const char *orderby)
{
	assert(list != NULL);

	size_t item_size = sizeof(FileListEntry);

	memset(list, 0, sizeof(FileList));

	slist_init(&list->clis, str_compare, free, NULL);

	list->entries = utils_new(512, FileListEntry *);
	list->size = 512;

	if(sizeof(FileInfo) > item_size)
	{
		item_size = sizeof(FileInfo);
	}

	list->pool = (Pool *)memory_pool_new(item_size, 1024);

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

				const char *rest = _sort_string_pop(orderby, list->fields, list->fields_asc);

				TRACEF("filelist", "Found field '%c', ascending=%d.", *list->fields, *list->fields_asc);

				while(rest)
				{
					++offset;
					rest = _sort_string_pop(rest, &list->fields[offset], &list->fields_asc[offset]);

					TRACEF("filelist", "Found field '%c', ascending=%d.", list->fields[offset], list->fields_asc[offset]);
				}
			}
		}
		else
		{
			fprintf(stderr, _("Couldn't parse sort string.\n"));
		}
	}
}

static char *
_file_list_dup_cli(FileList *list, const char *cli)
{
	char *dup;

	assert(list != NULL);

	SListItem *search = slist_find(&list->clis, NULL, cli);

	if(search)
	{
		dup = (char *)slist_item_get_data(search);
	}
	else
	{
		dup = utils_strdup(cli);
		slist_append(&list->clis, dup);
	}

	return dup;
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

	entry = list->pool->alloc(list->pool);
	memset(entry, 0, sizeof(FileListEntry));
	entry->filesp = list;

	return entry;
}

static FileListEntry *
_file_list_entry_new_from_path(FileList *list, const char *cli, const char *path)
{
	FileListEntry *entry = NULL;

	assert(list != NULL);
	assert(path != NULL);

	FileInfo info;

	file_info_init(&info);

	if(file_info_get(&info, _file_list_dup_cli(list, cli), false, path))
	{
		entry = _file_list_entry_new(list);
		entry->info = list->pool->alloc(list->pool);
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
	assert(list);

	if(list)
	{
		slist_free(&list->clis);

		free(list->fields);
		free(list->fields_asc);

		for(size_t i = 0; i < list->count; ++i)
		{
			_file_list_entry_free(list->entries[i]);
		}

		memory_pool_destroy((MemoryPool *)list->pool);
		free(list->entries);
	}
}

bool
file_list_append(FileList *list, const char *cli, const char *path)
{
	bool success = false;

	assert(list != NULL);
	assert(path != NULL);

	FileListEntry *entry = NULL;

	TRACEF("filelist", "Appending file: %s", path);

	if(list->count != SIZE_MAX)
	{
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

				fprintf(stderr, _("Couldn't allocate memory.\n"));
				abort();
			}
		}

		entry = _file_list_entry_new_from_path(list, cli, path);

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
	}
	else
	{
		FATAL("filelist", "Integer overflow.");
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

