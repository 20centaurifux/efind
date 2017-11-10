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
   @file format-fields.c
   @brief Substitute format characters for field names.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "format-fields.h"
#include "utils.h"
#include "log.h"

static const char *_FIELD_CHARS = "AbCDfFgGhHiklmMnpsSTuU";

static const char *_FIELD_NAMES[] =
{
	"atime", "blocks", "ctime", "device", "filename", "filesystem",
	"group", "gid", "directory", "starting-point", "inode", "kb", "link",
	"permission-bits", "permissions", "hardlinks", "path", "bytes",
	"sparseness", "mtime", "username", "uid"
};

char *
format_substitute(const char *str)
{
	char *fmt = NULL;

	assert(str != NULL);

	DEBUG("format", "Replacing field names.");

	if(str)
	{
		TRACEF("format", "Processing format string: \"%s\"", str);

		fmt = utils_strdup(str);
		char *ptr = fmt;

		while(*ptr)
		{
			for(size_t i = 0; i < sizeof(_FIELD_NAMES) / sizeof(char *); i++)
			{
				char name[32] = {'\0'};

				snprintf(name, sizeof(name) - 1, "{%s}", _FIELD_NAMES[i]);

				if(utils_startswith(ptr, name))
				{
					TRACEF("format", "Replacing field name \"%s\" with '%c'", name, _FIELD_CHARS[i]);

					size_t len = strlen(name);

					*ptr = _FIELD_CHARS[i];
					strcpy(ptr + 1, ptr + len);
				}
			}

			ptr++;
		}
	}

	return fmt;
}

char
format_map_field_name(const char *name)
{
	assert(name != NULL);

	if(name)
	{
		for(size_t i = 0; i < sizeof(_FIELD_NAMES) / sizeof(char *); i++)
		{
			if(!strcmp(_FIELD_NAMES[i], name))
			{
				return _FIELD_CHARS[i];
			}
		}
	}

	return '\0';
}

