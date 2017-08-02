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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "sort.h"
#include "utils.h"

#define FIELDS "bfgGhiklmMnpsSuUyYpPHFD"

int
sort_string_test(const char *str)
{
	int result = -1;

	if(str && *str)
	{
		const char *ptr = str;

		result = 0;

		while(result >= 0 && *ptr)
		{
			if(*ptr == '-')
			{
				++ptr;
			}

			if(*ptr)
			{
				if(strchr(FIELDS, *ptr))
				{
					if(result <= INT_MAX)
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
	}

	return result;
}

const char
*sort_string_pop(const char *str, char *field, bool *asc)
{
	const char *ptr = str;

	assert(field != NULL);
	assert(asc != NULL);

	*field = '\0';
	*asc = true;

	if(ptr && *ptr)
	{
		if(*ptr == '-')
		{
			*asc = false;
			++ptr;
		}

		if(*ptr && strchr(FIELDS, *ptr))
		{
			*field = *ptr;
			++ptr;
		}

		if(!*ptr)
		{
			ptr = NULL;
		}
	}
	else
	{
		ptr = NULL;
	}

	return ptr;
}

void
file_list_init(FileList *list, const char *cli, const char *sortby)
{
	assert(list != NULL);
	assert(cli != NULL);

	memset(list, 0, sizeof(FileList));

	list->cli = utils_strdup(cli);
	list->entries = (FileListEntry **)utils_malloc(sizeof(FileListEntry *) * 8);
	list->size = 8;

	if(sortby)
	{
		int count = sort_string_test(sortby);

		if(count > 0)
		{
			size_t offset = 0;

			list->fields = (char *)utils_malloc(sizeof(char) * count);
			list->fields_asc = (bool *)utils_malloc(sizeof(bool) * count);
			list->fields_n = count;

			const char *rest = sort_string_pop(sortby, list->fields, list->fields_asc);

			while(rest)
			{
				++offset;
				rest = sort_string_pop(rest, &list->fields[offset], &list->fields_asc[offset]);
			}
		}
		else
		{
			fprintf(stderr, "Couldn't parse orderby string.\n");
		}
	}
}

static void
_file_list_entry_destroy(FileListEntry *entry)
{
	if(entry)
	{
		for(int i = 0; i < entry->filesp->fields_n; ++i)
		{
			file_attr_free(&entry->attrs[i]);
		}

		free(entry->attrs);
		free(entry->path);
		file_info_clear(entry->info);
		free(entry->info);
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

	if(list->fields_n > 0)
	{
		file_info_init(&info);

		if(file_info_get(&info, list->cli, path))
		{
			entry = _file_list_entry_new(list);
			entry->info = file_info_dup(&info);

			bool success = true;
			int count = 0;

			entry->attrs = (FileAttr *)utils_malloc(sizeof(FileAttr) * list->fields_n);

			for(int i = 0; success && i < list->fields_n; ++i)
			{
				if((success = file_info_get_attr(entry->info, &entry->attrs[i], list->fields[i])))
				{
					++count;
				}
			}

			if(success)
			{
				entry->path = utils_strdup(path);
			}
			else
			{
				for(int i = 0; i < count; ++i)
				{
					file_attr_free(&entry->attrs[i]);
				}

				free(entry->attrs);
				free(entry);
				entry = NULL;
			}
		}

		file_info_clear(&info);
	}
	else
	{
			entry = _file_list_entry_new(list);
			entry->path = utils_strdup(path);
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
		result = file_attr_compare(&first->attrs[i], &second->attrs[i]);

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

