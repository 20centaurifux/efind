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
   @file fileinfo.c
   @brief Read file attributes.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
/*! @cond INTERNAL */
#define _GNU_SOURCE
/*! @endcond */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>

#include "fileinfo.h"
#include "log.h"
#include "linux.h"
#include "utils.h"
#include "gettext.h"

/*! Shared FSMap instance to get the filesystem of a file. */
static FSMap *_fs_map = NULL;

static void
_fs_map_free(void)
{
	if(_fs_map)
	{
		fs_map_destroy(_fs_map);
	}
}

static char *
_file_info_get_dirname(const char *filename)
{
	assert(filename != NULL);

	char *dirname = NULL;
	size_t len = 0;

	if(filename)
	{
		len = strlen(filename);
	}

	if(len > 0 && len < PATH_MAX)
	{
		dirname = utils_strdup(filename);

		char *offset = strrchr(dirname, '/');

		if(offset)
		{
			*offset = '\0';
		}
		else
		{
			strcpy(dirname, ".");
		}
	}

	return dirname;
}

static char *
_file_info_readlink(const char *filename)
{
	assert(filename != NULL);

	ssize_t size;
	char *buffer = utils_new(PATH_MAX, char);

	size = readlink(filename, buffer, PATH_MAX);

	if(size > 0 && size < PATH_MAX)
	{
		buffer[size] = '\0';
	}
	else
	{
		*buffer = '\0';

		if(size == -1)
		{
			perror("readlink()");
		}
	}

	return buffer;
}

static char *
_file_info_permissions(mode_t mode)
{
	static const char *rwx[] = {"---", "--x", "-w-", "-wx", "r--", "r-x", "rw-", "rwx"};
	char *bits = utils_new(11, char);

	switch(mode & S_IFMT)
	{
		case S_IFSOCK:
			*bits = 's';
			break;

		case S_IFLNK:
			*bits = 'l';
			break;

		case S_IFBLK:
			*bits = 'b';
			break;

		case S_IFDIR:
			*bits = 'd';
			break;

		case S_IFCHR:
			*bits = 'c';
			break;

		case S_IFIFO:
			*bits = 'p';
			break;

		default:
			*bits = '-';
	}

	strcpy(&bits[1], rwx[(mode >> 6) & 7]);
	strcpy(&bits[4], rwx[(mode >> 3) & 7]);
	strcpy(&bits[7], rwx[(mode & 7)]);

	if(mode & S_ISUID)
	{
        	bits[3] = (mode & S_IXUSR) ? 's' : 'S';
	}

	if(mode & S_ISGID)
	{
        	bits[6] = (mode & S_IXGRP) ? 's' : 'l';
	}

	if(mode & S_ISVTX)
	{
        	bits[9] = (mode & S_IXOTH) ? 't' : 'T';
	}

	return bits;
}

static const char *
_file_info_remove_cli(const char *cli, const char *path)
{
	assert(cli != NULL);
	assert(path != NULL);

	size_t len;

	len = strlen(cli);

	if(cli[len - 1] != '/' && path[len])
	{
		++len;
	}

	return path + len;
}

static double
#ifdef _LARGEFILE64_SOURCE
_file_info_calc_sparseness(blksize_t blksize, blkcnt_t blocks, off64_t size)
#else
_file_info_calc_sparseness(blksize_t blksize, blkcnt_t blocks, off_t size)
#endif
{
	double sparseness = 0.0;

	if(size > 0)
	{
		sparseness = (double)(blksize / 8) * blocks / size;

		if(isnan(sparseness) || isinf(sparseness))
		{
			sparseness = 0.0;
		}
	}
	else if(blocks)
	{
		sparseness = 1.0;
	}

	return sparseness;
}

void
file_info_init(FileInfo *info)
{
	assert(info != NULL);

	memset(info, 0, sizeof(FileInfo));
}

void
file_info_clear(FileInfo *info)
{
	if(info)
	{
		free(info->path);

		if(info->dup_cli)
		{
			free(info->cli);
		}

		file_info_init(info);
	}
}

bool
file_info_get(FileInfo *info, const char *cli, bool dup_cli, const char *path)
{
	bool success = false;

	assert(info != NULL);
	assert(cli != NULL);
	assert(path != NULL);

	#ifdef _LARGEFILE64_SOURCE
	struct stat64 sb;
	int rc = lstat64(path, &sb);
	#else
	struct stat sb;
	int rc = lstat(path, &sb);
	#endif

	if(!rc)
	{
		if(strlen(cli) < PATH_MAX && strlen(path) < PATH_MAX)
		{
			info->cli = dup_cli ? utils_strdup(cli) : (char *)cli;
			info->dup_cli = dup_cli;
			info->path = utils_strdup(path);
			info->sb = sb;
			success = true;
		}
		else
		{
			ERRORF("misc", "Couldn't validate paths: cli=%s, path=%s", cli, path);
		}
	}
	else
	{
		ERRORF("misc", "Couldn't retrieve information about %s: '`lstat64' failed.", path);
		fprintf(stderr, _("Couldn't stat file: %s\n"), path);
		#ifdef _LARGEFILE64_SOURCE
		perror("lstat64()");
		#else
		perror("lstat()");
		#endif
	}

	return success;
}

static char *
_file_info_extension(const char *filename)
{
	assert(filename != NULL);

	const char *name = basename(filename);

	return strrchr(name, '.');
}

static char *
_file_info_name_without_extension(const char *filename)
{
	assert(filename != NULL);

	char *name = utils_strdup(basename(filename));
	char *ptr = strrchr(name, '.');

	if(ptr)
	{
		*ptr = '\0';
	}

	return name;
}

bool
file_info_get_attr(FileInfo *info, FileAttr *attr, char field)
{
	bool success = true;

	assert(info != NULL);
	assert(attr != NULL);

	static const char *empty = "";

	switch(field)
	{
		case 'f': /* File's name with any leading directories removed (only the last element). */
			attr->flags = FILE_ATTR_FLAG_STRING;
			attr->value.str = basename(info->path);
			break;

		case 'P': /* File's name without the name of the command line argument under which it was found. */
			attr->flags = FILE_ATTR_FLAG_STRING;
			attr->value.str = (char *)_file_info_remove_cli(info->cli, info->path);
			break;

		case 'p': /* File's name. */
			attr->flags = FILE_ATTR_FLAG_STRING;
			attr->value.str = (char *)info->path;
			break;

		case 'h': /* Leading directories of file's name (all but the last element). */
			attr->flags = FILE_ATTR_FLAG_STRING | FILE_ATTR_FLAG_HEAP;
			attr->value.str = _file_info_get_dirname(info->path);
			break;

		case 'H': /* Command line argument under which file was found. */
			attr->flags = FILE_ATTR_FLAG_STRING;
			attr->value.str = (char *)info->cli;
			break;

		case 'g': /* File's group name, or numeric group ID if the group has no name. */
			attr->flags = FILE_ATTR_FLAG_STRING | FILE_ATTR_FLAG_HEAP;
			attr->value.str = linux_map_gid(info->sb.st_gid);
			break;

		case 'u': /* File's user name, or numeric user ID if the group has no name. */
			attr->flags = FILE_ATTR_FLAG_STRING | FILE_ATTR_FLAG_HEAP;
			attr->value.str = linux_map_uid(info->sb.st_uid);
			break;

		case 'F': /* Type of the filesystem the file is on; this value can be used for -fstype. */
			attr->flags = FILE_ATTR_FLAG_STRING;

			if(!_fs_map)
			{
				_fs_map = fs_map_load();
				atexit(_fs_map_free);
			}

			if(_fs_map)
			{
				attr->value.str = (char *)fs_map_path(_fs_map, info->path);
			}
			else
			{
				attr->value.str = (char *)empty;
			}
			break;

		case 'l': /* Object of symbolic link (empty string if file is not a symbolic link). */
			attr->flags = FILE_ATTR_FLAG_STRING;

			if(S_ISLNK(info->sb.st_mode))
			{
				attr->value.str = _file_info_readlink(info->path);
				attr->flags |= FILE_ATTR_FLAG_HEAP;
			}
			else
			{
				attr->value.str = (char *)empty;
			}
			break;

		case 'b': /* The amount of disk space used for this file in 512-byte blocks. */
			attr->flags = FILE_ATTR_FLAG_INTEGER;
			attr->value.n = info->sb.st_blocks;
			break;

		case 'D': /* The device number on which the file exists (the st_dev field of struct stat), in decimal. */
			attr->flags = FILE_ATTR_FLAG_INTEGER;
			attr->value.n = info->sb.st_dev;
			break;

		case 'G': /* File's numeric group ID. */
			attr->flags = FILE_ATTR_FLAG_INTEGER;
			attr->value.n = info->sb.st_gid;
			break;

		case 'i': /* File's inode number (in decimal). */
			attr->flags = FILE_ATTR_FLAG_INTEGER;
			attr->value.n = info->sb.st_ino;
			break;

		case 'k': /* The amount of disk space used for this file in 1K blocks. */
			attr->flags = FILE_ATTR_FLAG_LLONG;
			attr->value.llong = info->sb.st_size / 1024;
			break;

		case 'n': /* Number of hard links to file. */
			attr->flags = FILE_ATTR_FLAG_INTEGER;
			attr->value.n = info->sb.st_nlink;
			break;

		case 's': /* File's size in bytes. */
			attr->flags = FILE_ATTR_FLAG_LLONG;
			attr->value.llong = info->sb.st_size;
			break;

		case 'S': /* File's sparseness. */
			attr->flags = FILE_ATTR_FLAG_DOUBLE;
			attr->value.d = _file_info_calc_sparseness(info->sb.st_blksize, info->sb.st_blocks, info->sb.st_size);
			break;

		case 'U': /* File's numeric user ID. */
			attr->flags = FILE_ATTR_FLAG_INTEGER;
			attr->value.n = info->sb.st_uid;
			break;

		case 'A': /* File's last access time. */
			attr->flags = FILE_ATTR_FLAG_TIME;
			attr->value.time = info->sb.st_atime;
			break;

		case 'C': /* File's last status change time. */
			attr->flags = FILE_ATTR_FLAG_TIME;
			attr->value.time = info->sb.st_ctime;
			break;

		case 'T': /* File's last modification time. */
			attr->flags = FILE_ATTR_FLAG_TIME;
			attr->value.time = info->sb.st_mtime;
			break;

		case 'a': /* File last access time (numeric) */
			attr->flags = FILE_ATTR_FLAG_LLONG;
			attr->value.llong = info->sb.st_atime;
			break;

		case 'c': /* File's last status change time (numeric). */
			attr->flags = FILE_ATTR_FLAG_LLONG;
			attr->value.llong = info->sb.st_ctime;
			break;

		case 't': /* File's last modification time (numeric). */
			attr->flags = FILE_ATTR_FLAG_LLONG;
			attr->value.llong = info->sb.st_mtime;
			break;

		case 'm': /* File's  permission  bits (in octal). */
			attr->flags = FILE_ATTR_FLAG_INTEGER;
			attr->value.n = info->sb.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);
			break;

		case 'M': /* File's permissions (in symbolic form, as for ls). */
			attr->flags = FILE_ATTR_FLAG_STRING | FILE_ATTR_FLAG_HEAP;
			attr->value.str = _file_info_permissions(info->sb.st_mode);
			break;

		case 'X': /* File's extension. */
			attr->flags = FILE_ATTR_FLAG_STRING;
			attr->value.str = _file_info_extension(info->path);

			if(!attr->value.str)
			{
				attr->value.str = (char *)empty;
			}
			break;

		case 'N': /* File's name without extension. */
			attr->flags = FILE_ATTR_FLAG_STRING | FILE_ATTR_FLAG_HEAP;
			attr->value.str = _file_info_name_without_extension(info->path);
			break;

		default:
			FATALF("fileinfo", "Unexpected file attribute: '%c'", field);
			success = false;
	}

	return success;
}

void
file_attr_free(FileAttr *attr)
{
	if(attr && (attr->flags & FILE_ATTR_FLAG_STRING) && (attr->flags & FILE_ATTR_FLAG_HEAP))
	{
		free(attr->value.str);
	}
}

char *
file_attr_get_string(FileAttr *attr)
{
	if(attr && (attr->flags & FILE_ATTR_FLAG_STRING))
	{
		return attr->value.str;
	}

	return NULL;
}

int
file_attr_get_integer(FileAttr *attr)
{
	if(attr && (attr->flags & FILE_ATTR_FLAG_INTEGER))
	{
		return attr->value.n;
	}

	return 0;
}

long long
file_attr_get_llong(FileAttr *attr)
{
	if(attr && (attr->flags & FILE_ATTR_FLAG_LLONG))
	{
		return attr->value.llong;
	}

	return 0;
}

time_t
file_attr_get_time(FileAttr *attr)
{
	if(attr && (attr->flags & FILE_ATTR_FLAG_TIME))
	{
		return attr->value.n;
	}

	return 0;
}

double
file_attr_get_double(FileAttr *attr)
{
	if(attr && (attr->flags & FILE_ATTR_FLAG_DOUBLE))
	{
		return attr->value.d;
	}

	return 0;
}

int
file_attr_compare(FileAttr *a, FileAttr *b)
{
	int result = -1;

	assert(a != NULL);
	assert(b != NULL);
	assert(a->flags == b->flags);

	if(a->flags & FILE_ATTR_FLAG_STRING)
	{
		result = strcmp(a->value.str, b->value.str);
	}
	else if(a->flags & FILE_ATTR_FLAG_INTEGER || a->flags & FILE_ATTR_FLAG_TIME)
	{
		result = (a->value.n > b->value.n) - (a->value.n < b->value.n);
	}
	else if(a->flags & FILE_ATTR_FLAG_LLONG)
	{
		result = (a->value.llong > b->value.llong) - (a->value.llong < b->value.llong);
	}
	else if(a->flags & FILE_ATTR_FLAG_DOUBLE)
	{
		result = (a->value.d > b->value.d) - (a->value.d < b->value.d);
	}

	return result;
}

