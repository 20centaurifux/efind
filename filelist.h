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
   @file filelist.h
   @brief Sortable file list.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#ifndef FILELIST_H
#define FILELIST_H

#include <stdbool.h>
#include <datatypes.h>

#include "fileinfo.h"
#include "search.h"

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
	/*! Command line arguments under which the files were found. */
	SList clis;
	/*! Fields the list should be sorted by. */
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
	/*! Pool for FileListEntries. */
	Pool *pool;
};

/**
   @param str string to validate
   @return number of found fields

   Tests if a sort string is valid.
  */
int sort_string_test(const char *str);

/**
   @param list FileList instance to initialize
   @param orderby sort string

   Initializes a FileList instance.
  */
void file_list_init(FileList *list, const char *orderby);

/**
   @param list FileList instance to free

   Frees a FileList instance.
  */
void file_list_free(FileList *list);

/**
   @param list FileList instance
   @param cli command line argument under which the files are found
   @param path path to append to the FileList
   @return true on success

   Appends a path to a FileList instance.
  */
bool file_list_append(FileList *list, const char *cli, const char *path);

/**
   @param list FileList instance

   Sorts a file list.
  */
void file_list_sort(FileList *list);

/**
   @param list FileList instance
   @return number of stored files

   Gets the number of stored files.
  */
#define file_list_count(list) list->count

/**
   @param list FileList instance
   @param offset index of the entry to get
   @return a FileListEntry

   Gets a FileListEntry from the list.
  */
#define file_list_at(list, offset) list->entries[offset]

#endif

