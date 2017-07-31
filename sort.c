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
   @file sort.c
   @brief Sort found files.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#include <string.h>
#include <assert.h>

#include "sort.h"

#define FIELDS "bfgGhiklmMnpsSuUyYpPHFD"

bool
sort_string_test(const char *str)
{
	bool success = true;

	if((success = str && *str))
	{
		const char *ptr = str;

		while(success && *ptr)
		{
			if(*ptr == '-')
			{
				++ptr;
			}

			if(*ptr)
			{
				success = strchr(FIELDS, *ptr);
				++ptr;
			}
			else
			{
				success = false;
			}
		}
	}

	return success;
}

const char
*sort_string_pop(const char *str, char *field, bool *asc)
{
	const char *ptr = str;

	assert(field != NULL);
	assert(asc != NULL);

	*field = '\0';
	*asc = true;

	if(ptr && *ptr)
	{
		if(*ptr == '-')
		{
			*asc = false;
			++ptr;
		}

		if(*ptr && strchr(FIELDS, *ptr))
		{
			*field = *ptr;
			++ptr;
		}
		else
		{
			ptr = NULL;
		}
	}
	else
	{
		ptr = NULL;
	}

	return ptr;
}

