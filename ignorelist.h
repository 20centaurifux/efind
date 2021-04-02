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
   @file ignorelist.h
   @brief List containing ignored files.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#ifndef BLACKLIST_H
#define BLACKLIST_H

#include <datatypes.h>

/**
   @typedef Ignorelist
   @brief Ignored files.
 */
typedef List Ignorelist;

/**
   @return new Ignorelist

   Creates an Ignorelist.
 */
Ignorelist *ignorelist_new(void);

/**
   @param ignorelist Ignorelist to free

   Frees an Ignorelist.
 */
void ignorelist_destroy(Ignorelist *ignorelist);

/**
   @param ignorelist Ignorelist
   @param pattern glob pattern
   @return number of added files

   Adds files matching the given pattern to the ignore-list.
 */
size_t ignorelist_glob(Ignorelist *ignorelist, const char *pattern);

/**
   @param ignorelist an Ignorelist
   @param filename filename to test
   @return true if specified filename matches an ignored path

   Tests if a filename matches an ignored path.
 */
bool ignorelist_matches(Ignorelist *ignorelist, const char *filename);

/**
   @param ignorelist an Ignorelist
   @param filename name of a file containing patterns
   @return number of files added to the ignore-list

   Loads patterns from a file (one pattern per line).
 */
size_t ignorelist_load(Ignorelist *ignorelist, const char *filename);

/**
   @param ignorelist an Ignorelist
   @return number of files added to the ignore-list

   Loads patterns from the default ignore-list file (~/.efind/ignore-list).
 */
size_t ignorelist_load_default(Ignorelist *ignorelist);

/**
   @param ignorelist an Ignorelist
   @return head of the list
  
   Gets the head of the list.
 */
#define ignorelist_head(ignorelist) list_head(ignorelist)

#endif

