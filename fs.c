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
   @file fs.c
   @brief Filesystem related functions.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <mntent.h>
#include <assert.h>

#include "fs.h"
#include "log.h"
#include "utils.h"
#include "gettext.h"

/*! Name of unknown filesystems. */
const char *FS_UNKNOWN = "Unknown";

static int
_fs_map_compare_entries(const void *a, const void *b)
{
	assert(a != NULL);
	assert(b != NULL);

	return strcmp((*((MountPoint **)b))->path, (*((MountPoint **)a))->path);
}

FSMap *
fs_map_load(void)
{
	FSMap *map = NULL;
	FILE *fp;

	fp = setmntent("/proc/mounts", "r");

	if(fp)
	{
		/* store available mount points in array */
		map = utils_new(1, FSMap);
		map->mps = utils_new(32, MountPoint *);
		map->size = 32;
		map->len = 0;

		struct mntent *ent = getmntent(fp);

		while(ent)
		{
			if(map->len == map->size - 1)
			{
				if(map->size >= SIZE_MAX / 2)
				{
					FATAL("misc", "Integer overflow.");

					fprintf(stderr, _("Couldn't allocate memory.\n"));
					abort();
				}

				map->size *= 2;
				map->mps = utils_renew(map->mps, map->size, MountPoint *);
			}

			map->mps[map->len] = utils_new(1, MountPoint);

			strncpy(map->mps[map->len]->fs, ent->mnt_type, FS_NAME_MAX - 1);
			map->mps[map->len]->fs[FS_NAME_MAX - 1] = '\0';

			strncpy(map->mps[map->len]->path, ent->mnt_dir, PATH_MAX - 1);
			map->mps[map->len]->path[PATH_MAX - 1] = '\0';

			++map->len;
			ent = getmntent(fp);
		}

		endmntent(fp);

		/* sort array by path in descending order */
		if(map->len)
		{
			qsort(map->mps, map->len, sizeof(MountPoint *), &_fs_map_compare_entries);
		}
	}
	else
	{
		perror("setmntent()");
	}

	return map;
}

void
fs_map_destroy(FSMap *map)
{
	if(map)
	{
		for(size_t i = 0; i < map->len; ++i)
		{
			free(map->mps[i]);
		}

		free(map->mps);
		free(map);
	}
}

const char *
fs_map_path(FSMap *map, const char *path)
{
	char filename[PATH_MAX];

	assert(map != NULL);
	assert(path != NULL);

	if(realpath(path, filename))
	{
		for(size_t i = 0; i < map->len; ++i)
		{
			size_t len = strlen(map->mps[i]->path);

			if(len <= strlen(filename))
			{
				if(!strncmp(map->mps[i]->path, filename, len))
				{
					return map->mps[i]->fs;
				}
			}
		}
	}
	else
	{
		fprintf(stderr, _("Couldn't retrieve real path from file: %s\n"), path);
		perror("realpath()");
	}

	return FS_UNKNOWN;
}

