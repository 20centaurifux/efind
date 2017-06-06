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

typedef struct
{
	char cli[PATH_MAX];
	char path[PATH_MAX];
	struct stat sb;
	FSMap *fsmap;
} FileInfo;

typedef enum
{
	FILE_ATTR_FLAG_STRING  = 1,
	FILE_ATTR_FLAG_INTEGER = 2,
	FILE_ATTR_FLAG_HEAP    = 4,
	FILE_ATTR_FLAG_TIME    = 8
} FileAttrFlags;

typedef struct
{
	int32_t flags;
	union
	{
		char *str;
		int n;
		time_t time;
	} value;
} FileAttr;

void file_info_init(FileInfo *info);

void file_info_clear(FileInfo *info);

bool file_info_get(FileInfo *info, const char *cli, const char *path);

bool file_info_get_attr(FileInfo *info, FileAttr *attr, char field);

void file_attr_free(FileAttr *attr);

char *file_attr_get_string(FileAttr *attr);

int file_attr_get_integer(FileAttr *attr);

time_t file_attr_get_time(FileAttr *attr);
#endif

