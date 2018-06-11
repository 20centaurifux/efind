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
#include "format-fields.h"
#include "log.h"
#include "utils.h"
#include "gettext.h"

/*! @cond INTERNAL */
typedef enum
{
	FORMAT_LEXER_STATE_STRING,
	FORMAT_LEXER_STATE_ATTR,
	FORMAT_LEXER_STATE_WIDTH,
	FORMAT_LEXER_STATE_PRECISION,
	FORMAT_LEXER_STATE_DATE_ATTR,
	FORMAT_LEXER_STATE_ESCAPE_SEQ,
	FORMAT_LEXER_STATE_FIELD_NAME
} FormatLexerState;

typedef enum
{
	FORMAT_LEXER_RESULT_CONTINUE,
	FORMAT_LEXER_RESULT_ABORT,
	FORMAT_LEXER_RESULT_FINISHED
} FormatLexerStepResult;

#define ATTRIBUTES      "bfgGhiklmMnpsSuUyYpPHFDact"
#define DATE_ATTRIBUTES "ACT"
#define TIME_FIELDS     "HIklMprST+XZ"
#define DATE_FIELDS     "aAbBcdDhjmUwWxyY"
#define FLAGS           "-0# +"
#define ESCAPE_CHAR     "abfnrtv0\\"
/*! @endcond */

static bool
_format_lexer_get_current_state(FormatLexerResult *result, FormatLexerState *state)
{
	void *val;
	bool success = false;

	if(stack_head(&result->ctx.state, &val))
	{
		*state = (intptr_t)val;
		success = true;
	}

	return success;
}

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
_format_token_new(Pool *pool, FormatTokenType type_id, const char *text, size_t len)
{
	FormatToken *token;

	assert(pool != NULL);
	assert(text != NULL);
	assert(len > 0);

	TRACEF("format", "new Token(type=%#x, len=%ld)", type_id, len);

	token = pool->alloc(pool);

	token->type_id = type_id;
	token->text = text;
	token->len = len;

	return token;
}

static bool
_format_lexer_can_copy_text_token(const FormatLexerResult *result)
{
	assert(result != NULL);

	return result->ctx.start < result->ctx.tail && *result->ctx.start;
}

static bool
_format_lexer_is_text_token(FormatTokenType type_id)
{
	return type_id == FORMAT_TOKEN_STRING ||
	       type_id == FORMAT_TOKEN_ESCAPE_SEQ ||
	       type_id == FORMAT_TOKEN_NUMBER ||
	       type_id == FORMAT_TOKEN_DATE_FORMAT;
}

static bool
_format_lexer_is_char_token(FormatTokenType type_id)
{
	return type_id == FORMAT_TOKEN_FLAG ||
	       type_id == FORMAT_TOKEN_ATTRIBUTE ||
	       type_id == FORMAT_TOKEN_DATE_ATTRIBUTE;
}

static void
_format_lexer_found_token(FormatLexerResult *result, FormatTokenType type_id)
{
	assert(result != NULL);

	if(_format_lexer_is_text_token(type_id))
	{
		if(_format_lexer_can_copy_text_token(result))
		{
			slist_append(&result->ctx.token,
			             _format_token_new(result->ctx.pool, type_id, result->ctx.start, result->ctx.tail - result->ctx.start));
		}
	}
	else if(_format_lexer_is_char_token(type_id))
	{
		slist_append(&result->ctx.token,
		             _format_token_new(result->ctx.pool, type_id, result->ctx.tail, 1));
	}
	else
	{
		abort();
		FATALF("format", "Unknown token type: %#x", type_id);
	}
}

static bool
_format_lexer_date_attr_char_is_valid(const FormatLexerResult *result)
{
	assert(result != NULL);

	return *result->ctx.tail && (strchr(DATE_FIELDS, *result->ctx.tail) || strchr(TIME_FIELDS, *result->ctx.tail));
}

static void
_format_lexer_step_date_attr(FormatLexerResult *result)
{
	assert(result != NULL);

	if(_format_lexer_date_attr_char_is_valid(result))
	{
		++result->ctx.tail;
	}
	else
	{
		_format_lexer_found_token(result, FORMAT_TOKEN_DATE_FORMAT);
		_format_lexer_pop(result);
	}
}

static void
_format_lexer_process_precision(FormatLexerResult *result)
{
	assert(result != NULL);

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

static void
_format_lexer_process_width(FormatLexerResult *result)
{
	assert(result != NULL);

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

static bool
_format_lexer_step_number(FormatLexerResult *result)
{
	FormatLexerState state;
	bool success = true;

	assert(result != NULL);

	if(_format_lexer_get_current_state(result, &state))
	{
		if(state == FORMAT_LEXER_STATE_WIDTH)
		{
			_format_lexer_process_width(result);
		}
		else if(state == FORMAT_LEXER_STATE_PRECISION)
		{
			_format_lexer_process_precision(result);
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
_format_lexer_copy_found_field_name(const FormatLexerResult *result, char name[FORMAT_TEXT_BUFFER_MAX])
{
	assert(result != NULL);

	memset(name, 0, FORMAT_TEXT_BUFFER_MAX);
	memcpy(name, result->ctx.start + 1, result->ctx.tail - result->ctx.start - 1);
}

static bool
_format_lexer_substitute_current_field_char(FormatLexerResult *result)
{
	bool success = false;
	char name[FORMAT_TEXT_BUFFER_MAX];

	assert(result != NULL);

	_format_lexer_copy_found_field_name(result, name);

	char field = format_map_field_name(name);

	TRACEF("format", "Found long field name: %s, mapped to field '%c'", name, field);

	if(field)
	{
		result->ctx.len -= result->ctx.tail - result->ctx.start;
		memmove(result->ctx.start, result->ctx.tail, result->ctx.len - (result->ctx.start - result->ctx.fmt) + 1);

		*result->ctx.start = field;

		result->ctx.tail = result->ctx.start;
		--result->ctx.start;

		TRACEF("format",
		       "Substituted '%c' for \"{%s}\", fmt=%s, len=%ld, start=%s, tail=%s",
		       field,
		       name,
		       result->ctx.fmt,
		       result->ctx.len,
		       result->ctx.start,
		       result->ctx.tail);

		_format_lexer_pop(result);
		success = true;
	}
	else
	{
		FATALF("format", "Couldn't map field name: %s", name);
	}

	return success;
}

static bool
_format_lexer_field_name_ends(const FormatLexerResult *result)
{
	assert(result != NULL);

	return *result->ctx.tail == '}';
}

static bool
_format_lexer_field_name_char_is_valid(const FormatLexerResult *result)
{
	assert(result != NULL);

	return isalpha(*result->ctx.tail) || *result->ctx.tail == '-';
}

static bool
_format_lexer_step_field_name(FormatLexerResult *result)
{
	bool success = true;

	assert(result != NULL);

	if(_format_lexer_field_name_char_is_valid(result))
	{
		++result->ctx.tail;
	}
	else if(_format_lexer_field_name_ends(result))
	{
		success = _format_lexer_substitute_current_field_char(result);
	}
	else
	{
		FATALF("format", "Unexpected character: '%c'", *result->ctx.tail);

		success = false;
		_format_lexer_pop(result);
	}

	return success;
}

static bool
_format_lexer_found_flag_in_field(const FormatLexerResult *result)
{
	assert(result != NULL);

	return *result->ctx.tail && strchr(FLAGS, *result->ctx.tail);
}

static bool
_format_lexer_found_field_name_in_field(const FormatLexerResult *result)
{
	assert(result != NULL);

	return *result->ctx.tail == '{';
}

static bool
_format_lexer_found_attribute_in_field(const FormatLexerResult *result)
{
	assert(result != NULL);

	return *result->ctx.tail && strchr(ATTRIBUTES, *result->ctx.tail);
}

static bool
_format_lexer_found_date_attribute_in_field(const FormatLexerResult *result)
{
	assert(result != NULL);

	return *result->ctx.tail && strchr(DATE_ATTRIBUTES, *result->ctx.tail);
}

static bool
_format_lexer_found_width_in_field(const FormatLexerResult *result)
{
	assert(result != NULL);

	return *result->ctx.tail && isdigit(*result->ctx.tail) && *result->ctx.tail != '0';
}

static bool
_format_lexer_step_field(FormatLexerResult *result)
{
	bool success = true;

	assert(result != NULL);

	if(_format_lexer_found_flag_in_field(result))
	{
		_format_lexer_found_token(result, FORMAT_TOKEN_FLAG);
		++result->ctx.tail;
	}
	else if(_format_lexer_found_field_name_in_field(result))
	{
		_format_lexer_push(result, FORMAT_LEXER_STATE_FIELD_NAME, 1);
	}
	else if(_format_lexer_found_attribute_in_field(result))
	{
		_format_lexer_found_token(result, FORMAT_TOKEN_ATTRIBUTE);
		++result->ctx.tail;
		_format_lexer_pop(result);
	}
	else if(_format_lexer_found_date_attribute_in_field(result))
	{
		_format_lexer_found_token(result, FORMAT_TOKEN_DATE_ATTRIBUTE);
		_format_lexer_pop(result);
		_format_lexer_push(result, FORMAT_LEXER_STATE_DATE_ATTR, 1);
		++result->ctx.start;
	}
	else if(_format_lexer_found_width_in_field(result))
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
_format_lexer_process_percent_in_string(FormatLexerResult *result)
{
	bool success = true;

	assert(result != NULL);

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
		fprintf(stderr, _("Missing text after '%%'.\n"));
		success = false;
	}

	return success;
}

static bool
_format_lexer_found_3_digit_ascii_code_in_string(const FormatLexerResult *result)
{
	assert(result != NULL);

	return result->ctx.len - (result->ctx.tail - result->ctx.fmt) >= 4 &&
	       (*(result->ctx.tail + 1)  == 'x' || isdigit(*(result->ctx.tail + 1))) &&
	       isdigit(*(result->ctx.tail + 2)) &&
	       isdigit(*(result->ctx.tail + 3));
}

static bool
_format_lexer_found_2_digit_ascii_code_in_string(const FormatLexerResult *result)
{
	assert(result != NULL);

	return result->ctx.len - (result->ctx.tail - result->ctx.fmt) >= 3 &&
	       (*(result->ctx.tail + 1) == 'x' || isdigit(*(result->ctx.tail + 1))) &&
	       isdigit(*(result->ctx.tail + 2));
}

static bool
_format_lexer_found_1_digit_ascii_code_in_string(const FormatLexerResult *result)
{
	assert(result != NULL);

	return result->ctx.len - (result->ctx.tail - result->ctx.fmt) >= 2 && isdigit(*(result->ctx.tail + 1));
}

static bool
_format_lexer_found_escape_sequence_in_string(const FormatLexerResult *result)
{
	assert(result != NULL);

	return result->ctx.len - (result->ctx.tail - result->ctx.fmt) >= 2;
}

static bool
_format_lexer_process_blackslash_in_string(FormatLexerResult *result)
{
	bool success = true;

	assert(result != NULL);

	_format_lexer_found_token(result, FORMAT_TOKEN_STRING);
	result->ctx.start = result->ctx.tail;

	if(_format_lexer_found_3_digit_ascii_code_in_string(result))
	{
		result->ctx.tail += 4;
		_format_lexer_found_token(result, FORMAT_TOKEN_ESCAPE_SEQ);
		result->ctx.start = result->ctx.tail;
	}
	else if(_format_lexer_found_2_digit_ascii_code_in_string(result))
	{
		result->ctx.tail += 3;
		_format_lexer_found_token(result, FORMAT_TOKEN_ESCAPE_SEQ);
		result->ctx.start = result->ctx.tail;
	}
	else if(_format_lexer_found_1_digit_ascii_code_in_string(result))
	{
		result->ctx.tail += 2;
		_format_lexer_found_token(result, FORMAT_TOKEN_ESCAPE_SEQ);
		result->ctx.start = result->ctx.tail;
	}
	else if(_format_lexer_found_escape_sequence_in_string(result))
	{
		result->ctx.tail += 2;
		_format_lexer_found_token(result, FORMAT_TOKEN_ESCAPE_SEQ);
		result->ctx.start = result->ctx.tail;
	}
	else
	{
		fprintf(stderr, _("Invalid escape sequence.\n"));
		success = false;
	}

	return success;
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
		success = _format_lexer_process_percent_in_string(result);
	}
	else if(*result->ctx.tail == '\\')
	{
		success = _format_lexer_process_blackslash_in_string(result);
	}
	else
	{
		++result->ctx.tail;
	}

	return success;
}

static bool
_format_lexer_no_characters_left(const FormatLexerResult *result)
{
	return !(result->ctx.len - (result->ctx.tail - result->ctx.fmt)) && result->ctx.start == result->ctx.tail;
}

static FormatLexerStepResult
_format_lexer_step(FormatLexerResult *result)
{
	FormatLexerState state;
	bool success = true;
	FormatLexerStepResult next_step = FORMAT_LEXER_RESULT_CONTINUE;

	assert(result != NULL);

	if(!_format_lexer_get_current_state(result, &state))
	{
		return FORMAT_LEXER_RESULT_ABORT;	
	}

	if(_format_lexer_no_characters_left(result))
	{
		return FORMAT_LEXER_RESULT_FINISHED;
	}

	switch(state)
	{
		case FORMAT_LEXER_STATE_STRING:
			success = _format_lexer_step_string(result);
			break;

		case FORMAT_LEXER_STATE_ATTR:
			success = _format_lexer_step_field(result);

			break;
		case FORMAT_LEXER_STATE_FIELD_NAME:
			success = _format_lexer_step_field_name(result);
			break;

		case FORMAT_LEXER_STATE_WIDTH:
		case FORMAT_LEXER_STATE_PRECISION:
			success = _format_lexer_step_number(result);
			break;

		case FORMAT_LEXER_STATE_DATE_ATTR:
			_format_lexer_step_date_attr(result);
			break;

		default:
			FATALF("format", "Invalid state: %#x", state);
			success = false;
	}

	if(!success)
	{
		next_step = FORMAT_LEXER_RESULT_ABORT;
	}

	return next_step;
}

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
		
		result->ctx.pool = (Pool *)memory_pool_new(item_size, 32);

		stack_init(&result->ctx.state, &direct_compare, NULL, result->ctx.pool);
		slist_init(&result->ctx.token, &direct_compare, NULL, result->ctx.pool);

		result->ctx.fmt = utils_strdup(format);
		result->ctx.tail = result->ctx.fmt;
		result->ctx.start = result->ctx.tail;
		result->ctx.len = strlen(result->ctx.fmt);

		success = true;
	}

	return success;
}

FormatLexerResult *
format_lexer_scan(const char *format)
{
	FormatLexerResult *result;

	assert(format != NULL);

	TRACEF("format", "Scanning format string: %s", format);

	result = utils_new(1, FormatLexerResult);

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
	assert(result != NULL);

	if(result)
	{
		stack_free(&result->ctx.state);
		slist_free(&result->ctx.token);

		if(result->ctx.pool)
		{
			memory_pool_destroy((MemoryPool *)result->ctx.pool);
		}

		free(result->ctx.fmt);
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

