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
   @brief Filesystem related functions.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#ifndef FS_H
#define FS_H

#include <sys/types.h>
#include <limits.h>

/*! Maximum length of a filesystem (e.g. "ext4" or "btrfs"). */
#define FS_NAME_MAX 16

/**
   @struct MountPoint
   @brief Path and filesystem name of a mountpoint.
 */
typedef struct
{
	/*! Name of the filesystem. */
	char fs[FS_NAME_MAX];
	/*! The directory referring to the root of the filesystem. */
	char path[PATH_MAX];
} MountPoint;

/**
   @struct FSMap
   @brief A list of available mountpoints.
 */
typedef struct
{
	/*! Array of available mountpoints. */
	MountPoint **mps;
	/*! Size of the array. */
	size_t size;
	/*! Length of the array. */
	size_t len;
} FSMap;

/**
   @return a new FSMap instance or NULL on failure.

   Generates a list of available mountpoints.
 */
FSMap *fs_map_load(void);

/**
   @param map a FSMap instance

   Frees a FSMap instance.
 */
void fs_map_destroy(FSMap *map);

/**
   @param map a FSMap instance
   @param path a path
   @return a static string

   Gets the filesystem of the mountpoint associated to the specified path.
 */
const char *fs_map_path(FSMap *map, const char *path);

#endif

