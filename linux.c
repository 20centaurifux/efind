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
   @file linux.c
   @brief Linux utilities.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <grp.h>
#include <pwd.h>
#include <math.h>
#include <limits.h>

#include "linux.h"
#include "utils.h"

static char *
_utoa(unsigned int u)
{
	int max_len = (int)floor(log10(UINT_MAX)) + 3;

	char *str = utils_malloc(max_len);

	int written = snprintf(str, max_len, "%u", u);

	if(written >= max_len)
	{
		str[max_len - 1] = '\0';
	}
	else if(written < 0)
	{
		*str = '\0';
	}

	return str;
}

char *
linux_map_gid(gid_t gid)
{
	struct group *grp;
	char *name = NULL;

	grp = getgrgid(gid);
	
	if(grp && grp->gr_name && *grp->gr_name)
	{
		name = utils_strdup(grp->gr_name);
	}
	else
	{
		name = _utoa(gid);
	}

	return name;
}

char *
linux_map_uid(uid_t uid)
{
	struct passwd *pw;
	char *name = NULL;

	pw = getpwuid(uid);
	
	if(pw && pw->pw_name && *pw->pw_name)
	{
		name = utils_strdup(pw->pw_name);
	}
	else
	{
		name = _utoa(uid);
	}

	return name;
}

