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

/**
   @enum FormatTokenType
   @brief Specifies the type of a token.
 */
typedef enum
{
	/*! The token is invalid. */
	FORMAT_TOKEN_INVALID,
	/*! The token is a string. */
	FORMAT_TOKEN_STRING,
	/*! The token is a flag. */
	FORMAT_TOKEN_FLAG,
	/*! The token is a number. */
	FORMAT_TOKEN_NUMBER,
	/*! The token is a file attribute. */
	FORMAT_TOKEN_ATTRIBUTE,
	/*! The token is a date attribute. */
	FORMAT_TOKEN_DATE_ATTRIBUTE,
	/*! The token is a date format string. */
	FORMAT_TOKEN_DATE_FORMAT,
	/*! The token is an escape sequence. */
	FORMAT_TOKEN_ESCAPE_SEQ
} FormatTokenType;

/**
   @struct FormatToken
   @brief A found format token.
 */
typedef struct
{
	/*! Type of the token. */
	FormatTokenType type_id;
	/*! Text of the found token (not terminated by '\0'). */
	const char *text;
	/*! Lenght of the text. */
	size_t len;
} FormatToken;

/*! Allowed maximum length of a format string. */
#define FORMAT_TEXT_BUFFER_MAX 4096

/**
   @struct FormatLexerResult
   @brief Lexer state and result.
 */
typedef struct
{
	/*! true if format string has been processed without any failure. */
	bool success;

	struct
	{
		/*! Current state of the lexer. */
		Stack state;
		/*! Length of the format string to scan. */
		size_t len;
		/*! Format string to scan. */
		char *fmt;
		/*! Pointer to first processed character of a token. */
		char *start;
		/*! Pointer to last processed character if a token. */
		char *tail;
		/*! Custom allocator for lexer states. */
		Allocator *alloc;
		/*! A list of found tokens. */
		SList token;
	} ctx;
} FormatLexerResult;

/**
   @param format format string to scan
   @return a newly-allocated FormatLexerResult instance

   Scans a format string.
 */
FormatLexerResult *format_lexer_scan(const char *format);

/**
   @param result FormatLexerResult instance to free

   Frees a FormatLexerResult instance.
 */
void format_lexer_result_destroy(FormatLexerResult *result);

/**
   @param result FormatLexerResult instance
   @return a list item

   Gets first found token from a FormatLexerResult instance.
 */
SListItem *format_lexer_result_first_token(FormatLexerResult *result);

#endif

