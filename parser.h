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
 * @file parser.h
 * @brief Expression parser.
 * @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#ifndef PARSER_EXTRA_H
#define PARSER_EXTRA_H

#include <stdint.h>

#include <datatypes.h>
#include "translate.h"

/*! Maximum expression length. */
#define PARSER_MAX_EXPRESSION_LENGTH 512

/**
   @struct ParserExtra
   @brief Additional parser data used by flex.
 */
typedef struct
{
	/*! Buffer used to read strings. */
	Buffer buffer;
	/*! Memory allocator for nodes. */
	Allocator *alloc;
	/*! List containing pointers of allocated strings. */
	SList strings;
	/*! Current column. */
	int column;
	/*! Current line number. */
	int lineno;
} ParserExtra;

/**
   @struct ParserResult
   @brief The parsing result.
 */
typedef struct
{
	/*! Parser data. */
	ParserExtra data;
	/*! true if parsing was successfully. */
	bool success;
	/*! Root node of the parsed abstract syntax tree. */
	RootNode *root;
	/*! Error message. */
	char *err;
} ParserResult;

/**
   @param str string to parse
   @return a ParserResult

   Parses an expression and returns the result.
 */
ParserResult *parse_string(const char *str);

/**
   @param result ParserResult to free

   Frees a ParserResult.
  */
void parser_result_free(ParserResult *result);
#endif

