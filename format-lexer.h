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
   @file format-lexer.h
   @brief Scan format strings.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#ifndef FORMAT_LEXER_H
#define FORMAT_LEXER_H

#include "datatypes.h"

typedef enum
{
	FORMAT_TOKEN_INVALID,
	FORMAT_TOKEN_STRING,
	FORMAT_TOKEN_FLAG,
	FORMAT_TOKEN_NUMBER,
	FORMAT_TOKEN_ATTRIBUTE,
	FORMAT_TOKEN_DATE_ATTRIBUTE,
	FORMAT_TOKEN_DATE_FORMAT,
	FORMAT_TOKEN_ESCAPE_SEQ
} FormatTokenType;

typedef struct
{
	FormatTokenType type_id;
	const char *text;
	size_t len;
} FormatToken;

#define FORMAT_TEXT_BUFFER_MAX 4096

typedef struct
{
	bool success;

	struct
	{
		SList state;
		size_t len;
		char buffer[FORMAT_TEXT_BUFFER_MAX];
		const char *fmt;
		const char *start;
		const char *tail;
		Allocator *alloc;
		SList token;
	} ctx;
} FormatLexerResult;

FormatLexerResult *format_lexer_scan(const char *format);

void format_lexer_result_destroy(FormatLexerResult *result);

SListItem *format_lexer_result_first_token(FormatLexerResult *result);

#endif

