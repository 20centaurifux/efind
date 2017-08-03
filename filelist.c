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
#include "utils.h"

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
				fprintf(stderr, "Found unexpected character in sort string: '%c'.\n", *ptr);
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
	assert(list != NULL);
	assert(cli != NULL);

	memset(list, 0, sizeof(FileList));

	list->cli = utils_strdup(cli);
	list->entries = (FileListEntry **)utils_malloc(sizeof(FileListEntry *) * 8);
	list->size = 8;

	if(orderby)
	{
		int count = sort_string_test(orderby);

		if(count != -1)
		{
			list->fields_n = count;

			if(count > 0)
			{
				size_t offset = 0;

				list->fields = (char *)utils_malloc(sizeof(char) * count);
				list->fields_asc = (bool *)utils_malloc(sizeof(bool) * count);

				const char *rest = sort_string_pop(orderby, list->fields, list->fields_asc);

				while(rest)
				{
					++offset;
					rest = sort_string_pop(rest, &list->fields[offset], &list->fields_asc[offset]);
				}
			}
		}
		else
		{
			fprintf(stderr, "Couldn't parse sort string.\n");
		}
	}
}

static void
_file_list_entry_destroy(FileListEntry *entry)
{
	if(entry)
	{
		if(entry->info)
		{
			file_info_clear(entry->info);
			free(entry->info);
		}

		free(entry);
	}
}

static FileListEntry *
_file_list_entry_new(FileList *list)
{
	FileListEntry *entry;

	assert(list != NULL);

	entry = (FileListEntry *)utils_malloc(sizeof(FileListEntry));

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
		entry->info = file_info_dup(&info);
	}

	file_info_clear(&info);

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
			_file_list_entry_destroy(list->entries[i]);
		}

		free(list->entries);
	}
}

bool
file_list_append(FileList *list, const char *path)
{
	bool success = false;

	assert(list != NULL);
	assert(path != NULL);

	/* resize entry array */
	if(list->size == list->count)
	{
		list->size *= 2;
		list->entries = (FileListEntry **)utils_realloc(list->entries, sizeof(FileListEntry *) * list->size);
	}

	if(list->size > list->count)
	{
		FileListEntry *entry = _file_list_entry_new_from_path(list, path);

		if(entry)
		{
			list->entries[list->count] = entry;
			list->count++;
			success = true;
		}
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
				fprintf(stderr, "Couldn't read file attribute from %s.\n", second->info->path);
			}

			file_attr_free(&attr_a);
		}
		else
		{
			fprintf(stderr, "Couldn't read file attribute from %s.\n", first->info->path);
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

