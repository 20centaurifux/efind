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

#define FORMAT_TEXT_BUFFER_MAX 4096
#define FORMAT_FMT_BUFFER_MAX  128

typedef enum
{
	FORMAT_NODE_TEXT,
	FORMAT_NODE_ATTR
} FormatNodeType;

typedef struct
{
	FormatNodeType type_id;
	int32_t flags;
	ssize_t width;
} FormatNodeBase;

typedef struct
{
	FormatNodeBase padding;
	char text[FORMAT_TEXT_BUFFER_MAX];
} FormatTextNode;

typedef struct
{
	FormatNodeBase padding;
	char attr;
	char format[FORMAT_FMT_BUFFER_MAX];
} FormatAttrNode;

typedef enum
{
	FORMAT_PRINT_FLAG_ZERO  = 1,
	FORMAT_PRINT_FLAG_MINUS = 2,
	FORMAT_PRINT_FLAG_HASH  = 4,
	FORMAT_PRINT_FLAG_SPACE = 8,
	FORMAT_PRINT_FLAG_PLUS  = 16
} FormatPrintFlag;

typedef struct
{
	bool success;
	SList *nodes;
} FormatParserResult;

FormatParserResult *format_parse(const char *fmt);

void format_parser_result_free(FormatParserResult *result);
#endif

