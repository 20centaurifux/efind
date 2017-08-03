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
   @file format-lexer.c
   @brief Scan format strings.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "format-lexer.h"
#include "log.h"
#include "utils.h"

/*! @cond INTERNAL */
typedef enum
{
	FORMAT_LEXER_STATE_STRING,
	FORMAT_LEXER_STATE_ATTR,
	FORMAT_LEXER_STATE_WIDTH,
	FORMAT_LEXER_STATE_PRECISION,
	FORMAT_LEXER_STATE_DATE_ATTR,
	FORMAT_LEXER_STATE_ESCAPE_SEQ
} FormatLexerState;

typedef enum
{
	FORMAT_LEXER_RESULT_CONTINUE,
	FORMAT_LEXER_RESULT_ABORT,
	FORMAT_LEXER_RESULT_FINISHED
} FormatLexerStepResult;
/*! @endcond */

static bool
_format_lexer_init(FormatLexerResult *result, const char *format)
{
	bool success = false;
	size_t item_size = sizeof(SListItem);

	assert(result != NULL);

	memset(result, 0, sizeof(FormatLexerResult));

	if(format && strlen(format) < FORMAT_TEXT_BUFFER_MAX)
	{
		if(sizeof(FormatToken) > item_size)
		{
			item_size = sizeof(FormatToken);
		}
		
		result->ctx.alloc = (Allocator *)chunk_allocator_new(item_size, 32);

		stack_init(&result->ctx.state, &direct_compare, NULL, result->ctx.alloc);
		slist_init(&result->ctx.token, &direct_compare, NULL, result->ctx.alloc);

		result->ctx.fmt = format;
		result->ctx.tail = format;
		result->ctx.start = format;
		result->ctx.len = strlen(format);

		success = true;
	}

	return success;
}

/*! @cond INTERNAL */
#define ATTRIBUTES      "bfgGhiklmMnpsSuUyYpPHFD"
#define DATE_ATTRIBUTES "ACT"
#define TIME_FIELDS     "HIklMprST+XZ"
#define DATE_FIELDS     "aAbBcdDhjmUwWxyY"
#define FLAGS           "-0# +"
#define ESCAPE_CHAR     "abfnrtv0\\"
/*! @endcond */

static void
_format_lexer_pop(FormatLexerResult *result)
{
	void *state;

	assert(result != NULL);

	stack_pop(&result->ctx.state, &state);
	result->ctx.start = result->ctx.tail;
}

static void
_format_lexer_push(FormatLexerResult *result, FormatLexerState state, size_t offset)
{
	assert(result != NULL);
	assert(offset > 0);

	stack_push(&result->ctx.state, (void *)state);
	result->ctx.start = result->ctx.tail;
	result->ctx.tail += offset;
}

static FormatToken *
_format_token_new(Allocator *alloc, FormatTokenType type_id, const char *text, size_t len)
{
	FormatToken *token;

	assert(alloc != NULL);
	assert(text != NULL);
	assert(len > 0);

	TRACEF("format", "new Token(type=%#x, len=%ld)", type_id, len);

	token = alloc->alloc(alloc);

	token->type_id = type_id;
	token->text = text;
	token->len = len;

	return token;
}

static void
_format_lexer_found_token(FormatLexerResult *result, FormatTokenType type_id)
{
	assert(result != NULL);

	if(type_id == FORMAT_TOKEN_STRING || type_id == FORMAT_TOKEN_ESCAPE_SEQ || type_id == FORMAT_TOKEN_NUMBER || type_id == FORMAT_TOKEN_DATE_FORMAT)
	{
		if(result->ctx.start < result->ctx.tail && *result->ctx.start)
		{
			slist_append(&result->ctx.token, _format_token_new(result->ctx.alloc, type_id, result->ctx.start, result->ctx.tail - result->ctx.start));
		}
	}
	else if(type_id == FORMAT_TOKEN_FLAG || type_id == FORMAT_TOKEN_ATTRIBUTE || type_id == FORMAT_TOKEN_DATE_ATTRIBUTE)
	{
		slist_append(&result->ctx.token, _format_token_new(result->ctx.alloc, type_id, result->ctx.tail, 1));
	}
	else
	{
		FATALF("format", "Unknown token type: %#x", type_id);
	}
}

static bool
_format_lexer_step_string(FormatLexerResult *result)
{
	bool success = true;

	assert(result != NULL);

	if(*result->ctx.tail == '\0')
	{
		_format_lexer_found_token(result, FORMAT_TOKEN_STRING);
		result->ctx.start = result->ctx.tail;
	}
	else if(*result->ctx.tail == '%')
	{
		if(result->ctx.len - (result->ctx.tail - result->ctx.fmt) >= 2)
		{
			if(*(result->ctx.tail + 1) == '%')
			{
				_format_lexer_found_token(result, FORMAT_TOKEN_STRING);

				result->ctx.start = result->ctx.tail + 1;
				result->ctx.tail += 2;

				_format_lexer_found_token(result, FORMAT_TOKEN_STRING);

				++result->ctx.start;
			}
			else
			{
				_format_lexer_found_token(result, FORMAT_TOKEN_STRING);
				_format_lexer_push(result, FORMAT_LEXER_STATE_ATTR, 1);
			}

			success = true;
		}
		else
		{
			fprintf(stderr, "Missing text after '%%'.\n");
			success = false;
		}
	}
	else if(*result->ctx.tail == '\\')
	{
		_format_lexer_found_token(result, FORMAT_TOKEN_STRING);
		result->ctx.start = result->ctx.tail;

		if(result->ctx.len - (result->ctx.tail - result->ctx.fmt) >= 4 && (*(result->ctx.tail + 1)  == 'x' || isdigit(*(result->ctx.tail + 1)))
		   && isdigit(*(result->ctx.tail + 2)) && isdigit(*(result->ctx.tail + 3)))
		{
			result->ctx.tail += 4;
			_format_lexer_found_token(result, FORMAT_TOKEN_ESCAPE_SEQ);
			result->ctx.start = result->ctx.tail;
		}
		else if(result->ctx.len - (result->ctx.tail - result->ctx.fmt) >= 3 && (*(result->ctx.tail + 1) == 'x' || isdigit(*(result->ctx.tail + 1)))
		        && isdigit(*(result->ctx.tail + 2)))
		{
			result->ctx.tail += 3;
			_format_lexer_found_token(result, FORMAT_TOKEN_ESCAPE_SEQ);
			result->ctx.start = result->ctx.tail;
		}
		else if(result->ctx.len - (result->ctx.tail - result->ctx.fmt) >= 2 && isdigit(*(result->ctx.tail + 1)))
		{
			result->ctx.tail += 2;
			_format_lexer_found_token(result, FORMAT_TOKEN_ESCAPE_SEQ);
			result->ctx.start = result->ctx.tail;
		}
		else if(result->ctx.len - (result->ctx.tail - result->ctx.fmt) >= 2)
		{
			result->ctx.tail += 2;
			_format_lexer_found_token(result, FORMAT_TOKEN_ESCAPE_SEQ);
			result->ctx.start = result->ctx.tail;
		}
		else
		{
			fprintf(stderr, "Invalid escape sequence.\n");
			success = false;
		}
	}
	else
	{
		++result->ctx.tail;
	}

	return success;
}

static bool
_format_lexer_step_field(FormatLexerResult *result)
{
	bool success = true;

	assert(result != NULL);

	if(strchr(FLAGS, *result->ctx.tail))
	{
		_format_lexer_found_token(result, FORMAT_TOKEN_FLAG);
		++result->ctx.tail;
	}
	else if(strchr(ATTRIBUTES, *result->ctx.tail))
	{
		_format_lexer_found_token(result, FORMAT_TOKEN_ATTRIBUTE);
		++result->ctx.tail;
		_format_lexer_pop(result);
	}
	else if(strchr(DATE_ATTRIBUTES, *result->ctx.tail))
	{
		_format_lexer_found_token(result, FORMAT_TOKEN_DATE_ATTRIBUTE);
		_format_lexer_pop(result);
		_format_lexer_push(result, FORMAT_LEXER_STATE_DATE_ATTR, 1);
		++result->ctx.start;
	}
	else if(isdigit(*result->ctx.tail) && *result->ctx.tail != '0')
	{
		_format_lexer_push(result, FORMAT_LEXER_STATE_WIDTH, 1);
	}
	else
	{
		FATALF("format", "Unexpected character: '%c'", *result->ctx.tail);
		success = false;
	}

	return success;
}

static bool
_format_lexer_step_number(FormatLexerResult *result)
{
	void *state;
	bool success = true;

	assert(result != NULL);

	if(stack_head(&result->ctx.state, &state))
	{
		if((intptr_t)state == FORMAT_LEXER_STATE_WIDTH)
		{
			if(isdigit(*result->ctx.tail))
			{
				++result->ctx.tail;
			}
			else if(*result->ctx.tail == '.')
			{
				stack_push(&result->ctx.state, (void *)FORMAT_LEXER_STATE_PRECISION);
				++result->ctx.tail;
			}
			else
			{
				_format_lexer_found_token(result, FORMAT_TOKEN_NUMBER);
				_format_lexer_pop(result);
			}
		}
		else if((intptr_t)state == FORMAT_LEXER_STATE_PRECISION)
		{
			if(isdigit(*result->ctx.tail))
			{
				++result->ctx.tail;
			}
			else
			{
				_format_lexer_found_token(result, FORMAT_TOKEN_NUMBER);
				_format_lexer_pop(result); /* pop fraction */
				_format_lexer_pop(result); /* pop integral */
			}
		}
		else
		{
			FATALF("format", "Unexpected lexer state: %#x", (intptr_t)state);
			success = false;
		}
	}

	return success;
}

static void
_format_lexer_step_date_attr(FormatLexerResult *result)
{
	assert(result != NULL);

	if(*result->ctx.tail && (strchr(DATE_FIELDS, *result->ctx.tail) || strchr(TIME_FIELDS, *result->ctx.tail)))
	{
		++result->ctx.tail;
	}
	else
	{
		_format_lexer_found_token(result, FORMAT_TOKEN_DATE_FORMAT);
		_format_lexer_pop(result);
	}
}

static FormatLexerStepResult
_format_lexer_step(FormatLexerResult *result)
{
	void *state;
	bool success = true;
	FormatLexerStepResult next_step = FORMAT_LEXER_RESULT_CONTINUE;

	assert(result != NULL);

	/* get current state */
	if(!stack_head(&result->ctx.state, &state))
	{
		return FORMAT_LEXER_RESULT_ABORT;	
	}

	/* test available characters */
	if(!(result->ctx.len - (result->ctx.tail - result->ctx.fmt)) && result->ctx.start == result->ctx.tail)
	{
		return FORMAT_LEXER_RESULT_FINISHED;
	}

	/* process format string */
	switch((intptr_t)state)
	{
		case FORMAT_LEXER_STATE_STRING:
			success = _format_lexer_step_string(result);
			break;

		case FORMAT_LEXER_STATE_ATTR:
			success = _format_lexer_step_field(result);
			break;

		case FORMAT_LEXER_STATE_WIDTH:
		case FORMAT_LEXER_STATE_PRECISION:
			success = _format_lexer_step_number(result);
			break;

		case FORMAT_LEXER_STATE_DATE_ATTR:
			_format_lexer_step_date_attr(result);
			break;

		default:
			FATALF("format", "Invalid state: %#x", (intptr_t)state);
			success = false;
	}

	if(!success)
	{
		next_step = FORMAT_LEXER_RESULT_ABORT;
	}

	return next_step;
}

FormatLexerResult *
format_lexer_scan(const char *format)
{
	FormatLexerResult *result;

	assert(format != NULL);

	TRACEF("format", "Scanning format string: %s", format);

	result = (FormatLexerResult *)utils_malloc(sizeof(FormatLexerResult));

	if(_format_lexer_init(result, format))
	{
		FormatLexerStepResult status;

		stack_push(&result->ctx.state, (void *)FORMAT_LEXER_STATE_STRING);

		do
		{
			status = _format_lexer_step(result);
		} while(status == FORMAT_LEXER_RESULT_CONTINUE);

		result->success =  (status == FORMAT_LEXER_RESULT_FINISHED);
	}

	return result;
}

void
format_lexer_result_destroy(FormatLexerResult *result)
{
	if(result)
	{
		stack_free(&result->ctx.state);
		slist_free(&result->ctx.token);

		if(result->ctx.alloc)
		{
			chunk_allocator_destroy((ChunkAllocator *)result->ctx.alloc);
		}

		free(result);
	}
}

SListItem *
format_lexer_result_first_token(FormatLexerResult *result)
{
	SListItem *head = NULL;

	assert(result != NULL);

	if(result->success)
	{
		head = slist_head(&result->ctx.token);
	}

	return head;
}

