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
 */
#ifndef SEARCH_H
#define SEARCH_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "translate.h"

/**
   @struct SearchOptions
   @brief Search options.
 */
typedef struct
{
	/*! Directory search level limitation. */
	int32_t max_depth;
	/*! Dereference symbolic links. */
	bool follow;
	/*! Regular expression type. */
	char *regex_type;
} SearchOptions;

/**
   @param opts SearchOptions to free

   Frees resources allocated by the SearchOptions structure.
 */
void search_options_free(SearchOptions *opts);

/**
   @typedef Callback
   @brief A function called for each found file or error message.
          If the callback returns true the search aborts.
 */
typedef bool (*Callback)(const char *str, void *user_data);

/**
   @param path directory to search in
   @param expr expression
   @param flags translation flags
   @param opts search options
   @param found_file function called for each found file
   @param err_message function called for each failure message
   @param user_data user data
   @return number of found files

   Translates an expression and executes GNU find. If specified the result is filtered
   by a post-processing expression.
 */
int search_files(const char *path, const char *expr, TranslationFlags flags, const SearchOptions *opts, Callback found_file, Callback err_message, void *user_data);

/**
   @param out stream to write the translated expression to
   @param err stream to write failure messages to
   @param path directory to search in
   @param expr expression to parse
   @param flags translation flags
   @param opts search options
   @return true on success

   Converts an expression to GNU find arguments an writes the result to the desired stream.
 */
bool search_debug(FILE *out, FILE *err, const char *path, const char *expr, TranslationFlags flags, const SearchOptions *opts);
#endif

