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
   @file linux.h
   @brief Linux utilities.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#ifndef LINUX_H
#define LINUX_H

#include <sys/types.h>

char *linux_map_gid(gid_t gid);

char *linux_map_uid(uid_t uid);

#endif

