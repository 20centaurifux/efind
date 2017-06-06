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
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include "fileinfo.h"
#include "linux.h"
#include "utils.h"

static char *
_file_info_get_dirname_r(const char *filename)
{
	char *dirname = NULL;
	char *offset;

	dirname = strdup(filename);

	if(dirname)
	{
		offset = strrchr(dirname, '/');

		if(offset)
		{
			*offset = '\0';
		}
		else
		{
			strcpy(dirname, ".");
		}
	}
	else
	{
		perror("strdup()");
	}

	return dirname;
}

static char *
_file_info_readlink(const char *filename)
{
	ssize_t size;
	static char buffer[PATH_MAX];

	size = readlink(filename, buffer, PATH_MAX);

	if(size > 0 && size < PATH_MAX)
	{
		buffer[size] = '\0';
	}
	else
	{
		perror("readlink()");
	}

	return buffer;
}

static char *
_file_info_permissions(mode_t mode)
{
	static const char *rwx[] = {"---", "--x", "-w-", "-wx", "r--", "r-x", "rw-", "rwx"};
	static char bits[10];

	memset(bits, 0, sizeof(bits));

	strcpy(bits, rwx[(mode >> 6)& 7]);
	strcpy(&bits[3], rwx[(mode >> 3)& 7]);
	strcpy(&bits[6], rwx[(mode & 7)]);

	if(mode & S_ISUID)
	{
        	bits[3] = (mode & S_IXUSR) ? 's' : 'S';
	}

	if(mode & S_ISGID)
	{
        	bits[5] = (mode & S_IXGRP) ? 's' : 'l';
	}

	if(mode & S_ISVTX)
	{
        	bits[8] = (mode & S_IXOTH) ? 't' : 'T';
	}

	return bits;
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
		if(info->fsmap)
		{
			fs_map_destroy(info->fsmap);
		}

		file_info_init(info);
	}
}

bool
file_info_get(FileInfo *info, const char *cli, const char *path)
{
	struct stat sb;
	bool success = false;

	assert(cli != NULL);
	assert(path != NULL);

	if(!lstat(path, &sb) && strlen(cli) < PATH_MAX && strlen(path) < PATH_MAX)
	{
		strcpy(info->cli, cli);
		strcpy(info->path, path);
		info->sb = sb;
		success = true;
	}

	return success;
}

bool
file_info_get_attr(FileInfo *info, FileAttr *attr, char field)
{
	static const char *empty = "";
	bool success = true;

	assert(info != NULL);
	assert(attr != NULL);

	switch(field)
	{
		case 'f': /* File's name with any leading directories removed (only the last element). */
			attr->flags = FILE_ATTR_FLAG_STRING;
			attr->value.str = basename(info->path);
			break;

		case 'P': /* File's name without the name of the command line argument under which it was found. */
			attr->flags = FILE_ATTR_FLAG_STRING;
			attr->value.str = (char *)info->path + strlen(info->cli) + 1;
			break;

		case 'p': /* File's name. */
			attr->flags = FILE_ATTR_FLAG_STRING;
			attr->value.str = (char *)info->path;
			break;

		case 'h': /* Leading directories of file's name (all but the last element). */
			attr->flags = FILE_ATTR_FLAG_STRING | FILE_ATTR_FLAG_HEAP;
			attr->value.str = _file_info_get_dirname_r(info->path);
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

			if(!info->fsmap)
			{
				info->fsmap = fs_map_load();
			}

			if(info->fsmap)
			{
				attr->value.str = (char *)fs_map_path(info->fsmap, info->path);
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
			attr->flags = FILE_ATTR_FLAG_INTEGER;
			attr->value.n = info->sb.st_size / 1024;
			break;

		case 'n': /* Number of hard links to file. */
			attr->flags = FILE_ATTR_FLAG_INTEGER;
			attr->value.n = info->sb.st_nlink;
			break;

		case 's': /* File's size in bytes. */
			attr->flags = FILE_ATTR_FLAG_INTEGER;
			attr->value.n = info->sb.st_size;
			break;

		case 'S': /* File's sparseness. */
			attr->flags = FILE_ATTR_FLAG_INTEGER;
			attr->value.n = info->sb.st_blksize * info->sb.st_blocks / info->sb.st_size;
			break;

		case 'U': /* File's numeric user ID. */
			attr->flags = FILE_ATTR_FLAG_INTEGER;
			attr->value.n = info->sb.st_uid;
			break;

		case 'A': /* File's last access time. */
		case 'a':
			attr->flags = FILE_ATTR_FLAG_TIME;
			attr->value.time = info->sb.st_atime;
			break;

		case 'C': /* File's last access time. */
		case 'c':
			attr->flags = FILE_ATTR_FLAG_TIME;
			attr->value.time = info->sb.st_ctime;
			break;

		case 'T': /* File's last status change time. */
		case 't':
			attr->flags = FILE_ATTR_FLAG_TIME;
			attr->value.time = info->sb.st_mtime;
			break;

		case 'm': /* File's  permission  bits (in octal). */
			attr->flags = FILE_ATTR_FLAG_INTEGER;
			attr->value.n = info->sb.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);
			break;

		case 'M': /* File's permissions (in symbolic form, as for ls). */
			attr->flags = FILE_ATTR_FLAG_STRING;
			attr->value.str = _file_info_permissions(info->sb.st_mode);
			break;

		default:
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

time_t
file_attr_get_time(FileAttr *attr)
{
	if(attr && (attr->flags & FILE_ATTR_FLAG_TIME))
	{
		return attr->value.n;
	}

	return 0;
}

