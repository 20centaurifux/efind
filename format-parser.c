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
   @file format-parser.c
   @brief Parse format strings.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#include <ctype.h>
#include <assert.h>

#include "format.h"
#include "log.h"
#include "format-lexer.h"
#include "format-parser.h"
#include "utils.h"
#include "gettext.h"

/*! @cond INTERNAL */
typedef enum
{
	FORMAT_PARSER_STATE_NONE,
	FORMAT_PARSER_STATE_TEXT,
	FORMAT_PARSER_STATE_FLAG,
	FORMAT_PARSER_STATE_WIDTH,
	FORMAT_PARSER_STATE_ATTR,
	FORMAT_PARSER_STATE_DATE_ATTR
} FormatParserState;

typedef enum
{
	FORMAT_PARSER_STEP_RESULT_NEXT     = 0,
	FORMAT_PARSER_STEP_RESULT_ABORT    = 1,
	FORMAT_PARSER_STEP_RESULT_CONTINUE = 2
} FormatParserStepResult;

typedef struct
{
	Stack state;
	int32_t flags;
	ssize_t width;
	ssize_t precision;
	char buffer[FORMAT_TEXT_BUFFER_MAX];
	char *tail;
	char format[FORMAT_FMT_BUFFER_MAX];
	char attr;
	Pool *pool;
	SList *nodes;
} FormatParserCtx;
/*! @endcond */

static void
_format_node_init(FormatNodeBase *node, FormatNodeType type_id, int32_t flags, ssize_t width, ssize_t precision)
{
	assert(node != NULL);
	assert(flags >= 0);

	node->type_id = type_id;
	node->flags = flags;
	node->width = width;
	node->precision = precision;
}

static FormatNodeBase *
_format_text_node_new(FormatParserCtx *ctx)
{
	FormatTextNode *node;

	assert(ctx != NULL);

	TRACEF("format", "new FormatTextNode(flags=%#x, width=%ld, precision=%ld, len=%d)", ctx->flags, ctx->width, ctx->precision, strlen(ctx->buffer));

	node = utils_new(1, FormatTextNode);

	_format_node_init((FormatNodeBase *)node, FORMAT_NODE_TEXT, ctx->flags, ctx->width, ctx->precision);

	strcpy(node->text, ctx->buffer);

	return (FormatNodeBase *)node;
}

static FormatNodeBase *
_format_attribute_node_new(FormatParserCtx *ctx)
{
	FormatAttrNode *node;

	assert(ctx != NULL);

	TRACEF("format", "new FormatAttrNode(flags=%#x, width=%ld, precision=%ld, attr=%c, format=%s)", ctx->flags, ctx->width, ctx->precision, ctx->attr, ctx->format);

	node = utils_new(1, FormatAttrNode);

	_format_node_init((FormatNodeBase *)node, FORMAT_NODE_ATTR, ctx->flags, ctx->width, ctx->precision);

	node->attr = ctx->attr;
	strcpy(node->format, ctx->format);

	return (FormatNodeBase *)node;
}

static void
_format_parser_reset_cache(FormatParserCtx *ctx)
{
	assert(ctx != NULL);

	ctx->width = -1;
	ctx->precision = -1;
	ctx->flags = 0;
	memset(ctx->buffer, 0, FORMAT_TEXT_BUFFER_MAX);
	ctx->tail = ctx->buffer;
	memset(ctx->format, 0, FORMAT_FMT_BUFFER_MAX);
}

static void
_format_parser_found_attribute(FormatParserCtx *ctx)
{
	assert(ctx != NULL);

	slist_append(ctx->nodes, _format_attribute_node_new(ctx));
	_format_parser_reset_cache(ctx);
}

static void
_format_parser_found_date_attribute(FormatParserCtx *ctx)
{
	assert(ctx != NULL);

	slist_append(ctx->nodes, _format_attribute_node_new(ctx));
	_format_parser_reset_cache(ctx);
}

static void
_format_parser_found_string(FormatParserCtx *ctx)
{
	assert(ctx != NULL);

	slist_append(ctx->nodes, _format_text_node_new(ctx));
	_format_parser_reset_cache(ctx);
}

static void
_format_parser_pop(FormatParserCtx *ctx)
{
	void *state;

	assert(ctx != NULL);

	stack_pop(&ctx->state, &state);

	if((intptr_t)state == FORMAT_PARSER_STATE_TEXT)
	{
		_format_parser_found_string(ctx);
	}
	else if((intptr_t)state == FORMAT_PARSER_STATE_DATE_ATTR)
	{
		_format_parser_found_date_attribute(ctx);
	}
	else if((intptr_t)state != FORMAT_PARSER_STATE_NONE && (intptr_t)state != FORMAT_PARSER_STATE_WIDTH && (intptr_t)state != FORMAT_PARSER_STATE_FLAG)
	{
		WARNINGF("format", "Unknown parser state: %#x", (intptr_t)state);
	}
}

static FormatParserStepResult
_format_parser_begin_string(FormatParserCtx *ctx, const FormatToken *token)
{
	assert(ctx != NULL);
	assert(token != NULL);
	assert(token->type_id == FORMAT_TOKEN_STRING || token->type_id == FORMAT_TOKEN_ESCAPE_SEQ);

	stack_push(&ctx->state, (void *)FORMAT_PARSER_STATE_TEXT);

	return FORMAT_PARSER_STEP_RESULT_CONTINUE;
}

static FormatParserStepResult
_format_parser_begin_width(FormatParserCtx *ctx, const FormatToken *token)
{
	char str[128];

	assert(ctx != NULL);
	assert(token != NULL);
	assert(token->type_id == FORMAT_TOKEN_NUMBER);

	stack_push(&ctx->state, (void *)FORMAT_PARSER_STATE_WIDTH);

	memset(str, 0, 128);
	
	if(token->len < 128)
	{
		strncpy(str, token->text, token->len);

		char *offset = strchr(str, '.');

		if(offset)
		{
			ctx->precision = atoi(offset + 1);
			*offset = '\0';
			ctx->width = atoi(str);
		}
		else
		{
			ctx->width = atoi(str);
		}
	}
	else
	{
		ctx->width = 0;
		ctx->precision = 0;
	}

	return FORMAT_PARSER_STEP_RESULT_NEXT;
}

static FormatParserStepResult
_format_parser_begin_flags(FormatParserCtx *ctx, const FormatToken *token)
{
	assert(ctx != NULL);
	assert(token != NULL);

	stack_push(&ctx->state, (void *)FORMAT_PARSER_STATE_FLAG);

	return FORMAT_PARSER_STEP_RESULT_CONTINUE;
}

static FormatParserStepResult
_format_parser_begin_attribute(FormatParserCtx *ctx, const FormatToken *token)
{
	assert(ctx != NULL);
	assert(token != NULL);

	ctx->attr = *token->text;
	_format_parser_found_attribute(ctx);

	return FORMAT_PARSER_STEP_RESULT_NEXT;
}

static FormatParserStepResult
_format_parser_begin_date_attribute(FormatParserCtx *ctx, const FormatToken *token)
{
	assert(ctx != NULL);
	assert(token != NULL);

	memset(ctx->format, 0, FORMAT_FMT_BUFFER_MAX);

	ctx->attr = *token->text;

	stack_push(&ctx->state, (void *)FORMAT_PARSER_STATE_DATE_ATTR);

	return FORMAT_PARSER_STEP_RESULT_NEXT;
}

static FormatParserStepResult
_format_parser_step_none(FormatParserCtx *ctx, const FormatToken *token)
{
	FormatParserStepResult result = FORMAT_PARSER_STEP_RESULT_NEXT;

	assert(ctx != NULL);
	assert(token != NULL);

	if(token->type_id == FORMAT_TOKEN_STRING || token->type_id == FORMAT_TOKEN_ESCAPE_SEQ)
	{
		result = _format_parser_begin_string(ctx, token);
	}
	else if(token->type_id == FORMAT_TOKEN_FLAG)
	{
		result = _format_parser_begin_flags(ctx, token);
	}
	else if(token->type_id == FORMAT_TOKEN_NUMBER)
	{
		result = _format_parser_begin_width(ctx, token);
	}
	else if(token->type_id == FORMAT_TOKEN_ATTRIBUTE)
	{
		result = _format_parser_begin_attribute(ctx, token);
	}
	else if(token->type_id == FORMAT_TOKEN_DATE_ATTRIBUTE)
	{
		result = _format_parser_begin_date_attribute(ctx, token);
	}
	else
	{
		result = FORMAT_PARSER_STEP_RESULT_ABORT;
	}

	return result;
}

static void
_format_parser_process_ascii_escape_seq(FormatParserCtx *ctx, const FormatToken *token)
{
	char buffer[4];
	int base = 8;

	memset(buffer, 0, 4);

	if(token->text[1] == 'x')
	{
		base *= 2;
	}

	strncpy(buffer, token->text + (base / 8), token->len);
	*ctx->tail = (char)strtol(buffer, NULL, base);
	++ctx->tail;
}

static void
_format_parser_process_non_ascii_escape_seq(FormatParserCtx *ctx, const FormatToken *token)
{
	assert(ctx != NULL);
	assert(token != NULL);

	char c = *(token->text + 1);

	switch(c)
	{
		case 'a':
			c = '\a';
			break;

		case 'b':
			c = '\b';
			break;

		case 'f':
			c = '\f';
			break;

		case 'n':
			c = '\n';
			break;

		case 'r':
			c = '\r';
			break;

		case 't':
			c = '\t';
			break;

		case 'v':
			c = '\v';
			break;

		case '0':
			c = '\0';
			break;

		default:
			break;
	}

	*ctx->tail = c;
	++ctx->tail;
}

static FormatParserStepResult
_format_parser_process_escape_seq(FormatParserCtx *ctx, const FormatToken *token)
{
	FormatParserStepResult result = FORMAT_PARSER_STEP_RESULT_ABORT;

	assert(ctx != NULL);
	assert(token != NULL);

	if(ctx->tail - ctx->buffer + 1 < FORMAT_TEXT_BUFFER_MAX)
	{
		result = FORMAT_PARSER_STEP_RESULT_NEXT;

		assert(token->len >= 2 && *token->text == '\\');

		if(token->len == 2 && !isdigit(token->text[1]))
		{
			_format_parser_process_non_ascii_escape_seq(ctx, token);
		}
		else if(token->len <= 4)
		{
			_format_parser_process_ascii_escape_seq(ctx, token);
		}
		else
		{
			fprintf(stderr, _("Unsupported escape sequence.\n"));
		}
	}
	else
	{
		fprintf(stderr, _("String exceeds allowed maximum length.\n"));
		result = FORMAT_PARSER_STEP_RESULT_ABORT;
	}

	return result;
}

static FormatParserStepResult
_format_parser_process_string(FormatParserCtx *ctx, const FormatToken *token)
{
	FormatParserStepResult result = FORMAT_PARSER_STEP_RESULT_ABORT;

	assert(ctx != NULL);
	assert(token != NULL);

	if(ctx->tail - ctx->buffer + token->len < FORMAT_TEXT_BUFFER_MAX) 
	{
		strncat(ctx->tail, token->text, token->len);
		ctx->tail += token->len;
		result = FORMAT_PARSER_STEP_RESULT_NEXT;
	}
	else
	{
		fprintf(stderr, _("String exceeds allowed maximum length.\n"));
		result = FORMAT_PARSER_STEP_RESULT_ABORT;
	}

	return result;
}

static FormatParserStepResult
_format_parser_step_string(FormatParserCtx *ctx, const FormatToken *token)
{
	FormatParserStepResult result = FORMAT_PARSER_STEP_RESULT_CONTINUE;

	assert(ctx != NULL);
	assert(token != NULL);

	if(token->type_id == FORMAT_TOKEN_STRING)
	{
		result = _format_parser_process_string(ctx, token);
	}
	else if(token->type_id == FORMAT_TOKEN_ESCAPE_SEQ)
	{
		result = _format_parser_process_escape_seq(ctx, token);
	}
	else
	{
		_format_parser_pop(ctx);
	}

	return result;
}

static FormatParserStepResult
_format_parser_process_flag(FormatParserCtx *ctx, const FormatToken *token)
{
	assert(ctx != NULL);
	assert(token != NULL);
	assert(token->len == 1);

	int32_t flag = 0;

	switch(*token->text)
	{
		case '-':
			flag = FORMAT_PRINT_FLAG_MINUS;
			break;

		case '0':
			flag = FORMAT_PRINT_FLAG_ZERO;
			break;

		case '#':
			flag = FORMAT_PRINT_FLAG_HASH;
			break;

		case ' ':
			flag = FORMAT_PRINT_FLAG_SPACE;
			break;

		case '+':
			flag = FORMAT_PRINT_FLAG_PLUS;
			break;

		default:
			FATALF("format", "Invalid flag: '%c'", *token->text);
	}

	ctx->flags |= flag;

	return FORMAT_PARSER_STEP_RESULT_NEXT;
}

static FormatParserStepResult
_format_parser_try_process_number(FormatParserCtx *ctx, const FormatToken *token)
{
	FormatParserStepResult result = FORMAT_PARSER_STEP_RESULT_ABORT;

	_format_parser_pop(ctx);

	if(token->type_id == FORMAT_TOKEN_NUMBER)
	{
		result = _format_parser_begin_width(ctx, token);
	}
	else if(token->type_id == FORMAT_TOKEN_ATTRIBUTE)
	{
		result = _format_parser_begin_attribute(ctx, token);
	}
	else if(token->type_id == FORMAT_TOKEN_DATE_ATTRIBUTE)
	{
		result = _format_parser_begin_date_attribute(ctx, token);
	}
	else
	{
		result = FORMAT_PARSER_STEP_RESULT_ABORT;
	}

	return result;
}

static FormatParserStepResult
_format_parser_step_flag(FormatParserCtx *ctx, const FormatToken *token)
{
	FormatParserStepResult result = FORMAT_PARSER_STEP_RESULT_ABORT;

	assert(ctx != NULL);
	assert(token != NULL);

	if(token->type_id == FORMAT_TOKEN_FLAG)
	{
		result = _format_parser_process_flag(ctx, token);
	}
	else
	{
		result = _format_parser_try_process_number(ctx, token);
	}

	return result;
}

static FormatParserStepResult
_format_parser_step_width(FormatParserCtx *ctx, const FormatToken *token)
{
	FormatParserStepResult result = FORMAT_PARSER_STEP_RESULT_NEXT;

	assert(ctx != NULL);
	assert(token != NULL);

	_format_parser_pop(ctx);

	if(token->type_id == FORMAT_TOKEN_ATTRIBUTE)
	{
		result = _format_parser_begin_attribute(ctx, token);
	}
	else if(token->type_id == FORMAT_TOKEN_DATE_ATTRIBUTE)
	{
		result = _format_parser_begin_date_attribute(ctx, token);
	}
	else
	{
		result = FORMAT_PARSER_STEP_RESULT_ABORT;
	}

	return result;
}

static FormatParserStepResult
_format_parser_step_date_attribute(FormatParserCtx *ctx, const FormatToken *token)
{
	FormatParserStepResult result = FORMAT_PARSER_STEP_RESULT_CONTINUE;

	assert(ctx != NULL);
	assert(token != NULL);

	if(token->type_id == FORMAT_TOKEN_DATE_FORMAT)
	{
		if(token->len < FORMAT_FMT_BUFFER_MAX)
		{
			strncpy(ctx->format, token->text, token->len);
		}
		else
		{
			fprintf(stderr, _("Date-format exceeds allowed maximum length.\n"));
		}

		result = FORMAT_PARSER_STEP_RESULT_NEXT;
	}

	_format_parser_pop(ctx);

	return result;
}

static bool
_format_parse(FormatParserCtx *ctx, FormatLexerResult *lexer)
{
	bool success = false;

	assert(ctx != NULL);
	assert(lexer != NULL);
	assert(lexer->success == true);

	void *state;

	stack_push(&ctx->state, (void *)FORMAT_PARSER_STATE_NONE);
	_format_parser_reset_cache(ctx);

	SListItem *iter = format_lexer_result_first_token(lexer);

	FormatParserStepResult result = FORMAT_PARSER_STEP_RESULT_NEXT;

	while(iter && result != FORMAT_PARSER_STEP_RESULT_ABORT)
	{
		FormatToken *token = slist_item_get_data(iter);

		if(!stack_head(&ctx->state, &state))
		{
			WARNING("format", "Parser state stack is empty.");
			break;
		}

		switch((intptr_t)state)
		{
			case FORMAT_PARSER_STATE_NONE:
				result = _format_parser_step_none(ctx, token);
				break;

			case FORMAT_PARSER_STATE_TEXT:
				result = _format_parser_step_string(ctx, token);
				break;

			case FORMAT_PARSER_STATE_FLAG:
				result = _format_parser_step_flag(ctx, token);
				break;

			case FORMAT_PARSER_STATE_WIDTH:
				result = _format_parser_step_width(ctx, token);
				break;

			case FORMAT_PARSER_STATE_DATE_ATTR:
				result = _format_parser_step_date_attribute(ctx, token);
				break;

			default:
				FATALF("format", "Invalid parser state: %#x", (intptr_t)state);
				result = FORMAT_PARSER_STEP_RESULT_ABORT;
		}

		if(result == FORMAT_PARSER_STEP_RESULT_NEXT)
		{
			iter = slist_item_next(iter);
		}
	}

	if(result == FORMAT_PARSER_STEP_RESULT_NEXT)
	{
		_format_parser_pop(ctx);
		success = true;
	}

	return success;
}

static void
_format_parser_ctx_init(FormatParserCtx *ctx)
{
	assert(ctx != NULL);

	memset(ctx, 0, sizeof(FormatParserCtx));

 	ctx->pool = (Pool *)memory_pool_new(sizeof(SListItem), 32);

	stack_init(&ctx->state, &direct_compare, NULL, ctx->pool);
	ctx->nodes = slist_new(&direct_compare, &free, NULL);
}

static SList *
_format_parser_ctx_detach_nodes(FormatParserCtx *ctx)
{
	SList *nodes;
       
	assert(ctx != NULL);

	nodes = ctx->nodes;
	ctx->nodes = NULL;

	return nodes;
}

static void
_format_parser_ctx_free(FormatParserCtx *ctx)
{
	assert(ctx != NULL);

	if(ctx)
	{
		stack_free(&ctx->state);

		if(ctx->nodes)
		{
			slist_destroy(ctx->nodes);
		}

		memory_pool_destroy((MemoryPool *)ctx->pool);
	}
}

FormatParserResult *
format_parse(const char *fmt)
{
	assert(fmt != NULL);

	DEBUGF("format", "Parsing format string: %s", fmt);

	FormatParserResult *result = utils_new(1, FormatParserResult);
	FormatLexerResult *lexer = format_lexer_scan(fmt);

	if(lexer->success)
	{
		FormatParserCtx ctx;

		_format_parser_ctx_init(&ctx);

		if(_format_parse(&ctx, lexer))
		{
			result->success = true;
			result->nodes = _format_parser_ctx_detach_nodes(&ctx);
		}

		_format_parser_ctx_free(&ctx);
	}
	else
	{
		DEBUG("format", "Scanning failed.");
	}

	format_lexer_result_destroy(lexer);

	return result;
}

void
format_parser_result_free(FormatParserResult *result)
{
	assert(result != NULL);

	if(result)
	{
		if(result->nodes)
		{
			slist_destroy(result->nodes);
		}

		free(result);
	}
}

