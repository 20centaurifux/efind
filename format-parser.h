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
   @file format-parser.h
   @brief Parse format strings.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#ifndef FORMAT_PARSER_H
#define FORMAT_PARSER_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "datatypes.h"

/*! Maximum format string length. */
#define FORMAT_TEXT_BUFFER_MAX 4096
/*! Maximum date-time format string length. */
#define FORMAT_FMT_BUFFER_MAX  128

/**
   @enum FormatNodeType
   @brief Node types.
 */
typedef enum
{
	/*! Node contains text. */
	FORMAT_NODE_TEXT,
	/*! Node is a file attribute. */
	FORMAT_NODE_ATTR
} FormatNodeType;

/**
   @struct FormatNodeBase
   @brief General node information.
 */
typedef struct
{
	/*! Type of the node. */
	FormatNodeType type_id;
	/*! Format flags. */
	int32_t flags;
	/*! Text width. */
	ssize_t width;
	/*! Precision. */
	ssize_t precision;
} FormatNodeBase;

/**
   @struct FormatTextNode
   @brief A text node.
 */
typedef struct
{
	/*! Base structure. */
	FormatNodeBase padding;
	/*! Found text. */
	char text[FORMAT_TEXT_BUFFER_MAX];
} FormatTextNode;

/**
   @struct FormatAttrNode
   @brief A node representing a file attribute. */
typedef struct
{
	/*! Base structure. */
	FormatNodeBase padding;
	/*! The file attribute. */
	char attr;
	/*! Optional format string (for date-time attributes). */
	char format[FORMAT_FMT_BUFFER_MAX];
} FormatAttrNode;

/**
   @enum FormatPrintFlag
   @brief Print flags.
 */
typedef enum
{
	/*! Zero padding. */
	FORMAT_PRINT_FLAG_ZERO  = 1,
	/*! Left adjustment. */
	FORMAT_PRINT_FLAG_MINUS = 2,
	/*! Convert value to "alternate form". */
	FORMAT_PRINT_FLAG_HASH  = 4,
	/*! Print space before a positive number. */
	FORMAT_PRINT_FLAG_SPACE = 8,
	/*! Print a sign (+ or -) before a number. */
	FORMAT_PRINT_FLAG_PLUS  = 16
} FormatPrintFlag;

/**
   @struct FormatParserResult
   @brief Parser result information.
 */
typedef struct
{
	/*! true if format string has been processed without any failure. */
	bool success;
	/*! Found nodes. */
	SList *nodes;
} FormatParserResult;

/**
   @param fmt format to parse
   @return a newly-allocated FormatParserResult instance

   Parses a format string.
 */
FormatParserResult *format_parse(const char *fmt);

/**
   @param result FormatParserResult instance to free

   Frees a FormatParserResult instance.
 */
void format_parser_result_free(FormatParserResult *result);

#endif

