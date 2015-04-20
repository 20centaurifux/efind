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
   @file parser.h
   @brief Expression parser.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
   @version 0.1.0
*/
#ifndef PARSER_EXTRA_H
#define PARSER_EXTRA_H

#include <stdio.h>
#include <stdint.h>

#include "buffer.h"
#include "translate.h"

/*! Maximum expression length. */
#define PARSER_MAX_EXPRESSION_LENGTH 512

/**
   @struct ParserExtra
   @brief Additional parser data used by flex.
 */
typedef struct
{
	/*! A buffer used to read string data. */
	Buffer buffer;
} ParserExtra;

/**
   @param str string to parse
   @param TranslationFlags translation flags
   @param argc number of translated find arguments
   @param argv translated find arguments
   @param err location to store error message
   @return true on success

   Converts an expression to a find-compatible argument vector.
 */
bool parse_string(const char *str, TranslationFlags flags, size_t *argc, char ***argv, char **err);

/**
   @param out stream to write the translated expression to
   @param err stream to write failure messages to
   @flags translation flags
   @return true on success

   Converts an expression to a valid find string an writes it to the specified stream.
 */
bool parse_string_and_print(FILE *out, FILE *err, const char *str, TranslationFlags flags);
#endif


