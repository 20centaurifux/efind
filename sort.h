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
   @file sort.h
   @brief Sort found files.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#ifndef SORT_H
#define SORT_H

#include <stdbool.h>

#include "fileinfo.h"

typedef struct _FileList FileList;

/**
   @struct FileListEntry
   @brief Stores file details.
 */
typedef struct
{
	/* A FileInfo instance used to read file details. */
	FileInfo *info;
	/*! Pointer to associated FileList instance. */
	FileList *filesp;
} FileListEntry;

/**
   @struct _FileList
   @brief A sortable file list.
 */
struct _FileList
{
	/*! Command line argument under which the file was found. */
	char *cli;
	/*! Fields the list should be filtered by. */
	char *fields;
	/*! Sort directions. */
	bool *fields_asc;
	/*! Number of fields specified in the sort string. */
	int fields_n;
	/*! Found files and attributes. */
	FileListEntry **entries;
	/*! Number of found files. */
	size_t count;
	/*! Size allocated for the entries array. */
	size_t size;
};

/**
   @param str string to validate
   @return true on success

   Tests if a sort string is valid.
  */
int sort_string_test(const char *str);

/**
   @param str sort string
   @param field location to store found field
   @param asc location to store sort direction
   @return pointer to next field or NULL
  */
const char *sort_string_pop(const char *str, char *field, bool *asc);

/**
   @param list FileList instance to initialize
   @param cli command line argument under which the files are located
   @param sortby sort string

   Initializes a FileList instance.
  */
void file_list_init(FileList *list, const char *cli, const char *sortby);

/**
   @param list FileList instance to free

   Frees a FileList instance.
  */
void file_list_free(FileList *list);

/**
   @param list FileList instance
   @param path path to append to the FileList
   @return true on success

   Appends a path to a FileList instance.
  */
bool file_list_append(FileList *list, const char *path);

/**
   @param list FileList instance

   Sorts a file list.
  */
void file_list_sort(FileList *list);

#endif

