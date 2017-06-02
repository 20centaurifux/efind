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
   @file fs.h
   @brief File-system related functions.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#ifndef FS_H
#define FS_H

#include <sys/types.h>
#include <limits.h>

#define FS_NAME_MAX 16

typedef struct
{
	char fs[FS_NAME_MAX];
	char path[PATH_MAX];
} MountPoint;

typedef struct
{
	MountPoint **mps;
	size_t size;
	size_t len;
} FSMap;

FSMap *fs_map_load(void);
void fs_map_destroy(FSMap *map);
const char *fs_map_path(FSMap *map, const char *path);

#endif

