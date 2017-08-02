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
   @file fileinfo.h
   @brief Read file attributes.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#ifndef FILEINFO_H
#define FILEINFO_H

#include <stdbool.h>
#include <limits.h>
#include <sys/stat.h>
#include <time.h>

#include "fs.h"

/**
   @struct FileInfo
   @brief File attributes.
 */
typedef struct
{
	/*! Command line argument under which the file was found. */
	char cli[PATH_MAX];
	/*! Path of the file. */
	char path[PATH_MAX];
	/*! File information. */
	#ifdef _LARGEFILE64_SOURCE
	struct stat64 sb;
	#else
	struct stat sb;
	#endif
	/*! A FSMap instance used to get the filesystem of a file. */
	FSMap *fsmap;
} FileInfo;

/**
   @enum FileAttrFlags
   @brief Flags which can be set for a FileAttr.
 */
typedef enum
{
	/*! The FileAttr holds a string. */
	FILE_ATTR_FLAG_STRING  = 1,
	/*! The FileAttr holds an integer. */
	FILE_ATTR_FLAG_INTEGER = 2,
	/*! The FileAttr's value is allocated on the heap. */
	FILE_ATTR_FLAG_HEAP    = 4,
	/*! The FileAttr holds a time value. */
	FILE_ATTR_FLAG_TIME    = 8,
	/*! The FileAttr holds a double. */
	FILE_ATTR_FLAG_DOUBLE  = 16,
	/*! The FileAttr holds a long long. */
	FILE_ATTR_FLAG_LLONG   = 32
} FileAttrFlags;

/**
   @struct FileAttr
   @brief The FileAttr structure holds a file attribute and associated flags.
 */
typedef struct
{
	/*! Attribute flag. */
	int32_t flags;
	union
	{
		/*! A string value. */
		char *str;
		/*! An integer value. */
		int n;
		/*! A time value. */
		time_t time;
		/*! A double value. */
		double d;
		/*! A long long value. */
		long long llong;
	} value;
} FileAttr;

/**
   @param info a FileInfo instance

   Initializes a FileInfo instance.
 */
void file_info_init(FileInfo *info);

/**
   @param info a FileInfo instance

   Resets all fields of a FileInfo instance.
 */
void file_info_clear(FileInfo *info);

/**
   @param info a FileInfo instance
   @param cli command line argument under which the file was found
   @param path name of the file to read attributes from
   @return true on success

   Prepare a FileInfo instance for attribute reading.
 */
bool file_info_get(FileInfo *info, const char *cli, const char *path);

/**
   @param info a FileInfo instance
   @return a new FileInfo instance

   Copies a FileInfo instance.
 */
FileInfo *file_info_dup(const FileInfo *info);

/**
   @param info a FileInfo instance
   @param attr location to store the read attribute to
   @param field field to read (see efind's printf-syntax)
   @return true on success

   Reads a file attribute.
 */
bool file_info_get_attr(FileInfo *info, FileAttr *attr, char field);

/**
   @param attr a FileAttr instance

   Frees memory allocated by the FileAttr insance.
 */
void file_attr_free(FileAttr *attr);

/**
   @param attr a FileAttr instance
   @return a string

   Reads the string value from a FileAttr.
 */
char *file_attr_get_string(FileAttr *attr);

/**
   @param attr a FileAttr instance
   @return an integer

   Reads the integer value from a FileAttr.
 */
int file_attr_get_integer(FileAttr *attr);

/**
   @param attr a FileAttr instance
   @return a long long

   Reads the long long value from a FileAttr.
 */
long long file_attr_get_llong(FileAttr *attr);

/**
   @param attr a FileAttr instance
   @return a time_t value

   Reads the time value from a FileAttr.
 */
time_t file_attr_get_time(FileAttr *attr);

/**
   @param attr a FileAttr instance
   @return an double

   Reads the double value from a FileAttr.
 */
double file_attr_get_double(FileAttr *attr);

/*
  @param a a file attribute
  @param b a file attribute
  @return 0 if values are equal, or a positive integer if the first value comes after the second

  Compares two file attributes.
 */
int file_attr_compare(FileAttr *a, FileAttr *b);

#endif

