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
   @file search.h
   @brief A find-wrapper.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
   @version 0.1.0
*/
#ifndef SEARCH_H
#define SEARCH_H

#include <stdbool.h>
#include <stdint.h>

#include "translate.h"

/**
   @typedef Callback
   @brief A callback function.
 */
typedef void (*Callback)(const char *str, void *user_data);

/**
   @param path directory to search in
   @param expr find expression
   @param flags translation flags
   @param found_file function called for each found file
   @param err_message function called for each failure
   @param user_data user data
   @return number of found files

   Translates expr and executes find with the translated arguments.
 */
int search_files_expr(const char *path, const char *expr, TranslationFlags flags, Callback found_file, Callback err_message, void *user_data);

#endif

