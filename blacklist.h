/***************************************************************************
    begin........: May 2015
    copyright....: Sebastian Fedrau
    email........: sebastian.fedrau@gmail.com
 ***************************************************************************/

/***************************************************************************
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License v3 as published by
    the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License v3 for more details.
 ***************************************************************************/
/**
   @file blacklist.h
   @brief List containing blacklisted files.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#ifndef BLACKLIST_H
#define BLACKLIST_H

#include <datatypes.h>

/**
   @typedef Blacklist
   @brief Blacklisted files.
 */
typedef List Blacklist;

/**
   @return new Blacklist

   Creates a Blacklist.
 */
Blacklist *blacklist_new(void);

/**
   @param blacklist Blacklist to free

   Frees a Blacklist.
 */
void blacklist_destroy(Blacklist *blacklist);

/**
   @param blacklist Blacklist
   @param pattern glob pattern
   @return number of added files

   Adds files matching the given pattern to the blacklist.
 */
size_t blacklist_glob(Blacklist *blacklist, const char *pattern);

/**
   @param blacklist a Blacklist
   @param filename filename to test
   @return true if specified filename matches a blacklisted path

   Tests if a filename matches a blacklisted path.
 */
bool blacklist_matches(Blacklist *blacklist, const char *filename);

/**
   @param blacklist a Blacklist
   @param filename name of a file containing patterns
   @return number of files added to the blacklist

   Loads patterns from a file (one pattern per line).
 */
size_t blacklist_load(Blacklist *blacklist, const char *filename);

/**
   @param blacklist a Blacklist
   @return number of files added to the blacklist

   Loads patterns from the default blacklist file (~/.efind/blacklist).
 */
size_t blacklist_load_default(Blacklist *blacklist);

/**
   @param blacklist a Blacklist
   @return head of the list
  
   Gets the head of the list.
 */
#define blacklist_head(blacklist) list_head(blacklist)

#endif

