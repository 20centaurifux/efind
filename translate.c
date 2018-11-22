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
   @file translate.c
   @brief Translate abstract syntax tree to find arguments.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <limits.h>
#include <inttypes.h>
#include <math.h>
#include <assert.h>

#include "translate.h"
#include "log.h"
#include "parser.h"
#include "utils.h"
#include "gettext.h"

/*
 *	translation context
 */

/*! @cond INTERNAL */
typedef struct
{
	Node *root;    /* root node of the tree */
	char *err;     /* error message */
	char **argv;   /* translated arguments */
	size_t argc;   /* number of translated arguments */
	size_t msize;  /* size of argv array */
	int32_t flags; /* translation flags */
} TranslationCtx;

#define QUOTE_ARGS(ctx) (ctx->flags & TRANSLATION_FLAG_QUOTE)
/*! @endcond */

static void
_translation_ctx_init(TranslationCtx *ctx, Node *root, TranslationFlags flags)
{
	assert(ctx != NULL);
	assert(root != NULL);

	memset(ctx, 0, sizeof(TranslationCtx));
	ctx->root = root;
	ctx->msize = 8;
	ctx->argv = utils_new(ctx->msize, char *);
	ctx->flags = flags;
}

static bool
_translation_ctx_append_arg(TranslationCtx *ctx, const char *arg)
{
	assert(ctx != NULL);
	assert(arg != NULL);

	if(ctx->argc >= ctx->msize)
	{
		size_t new_size = ctx->msize;

		do
		{
			new_size *= 2;

			if(new_size < ctx->msize)
			{
				FATAL("translate", "Overflow in ctx->msize calculation.");
				return false;
			}
		} while(new_size <= ctx->argc);

		ctx->msize = new_size;
		ctx->argv = utils_renew(ctx->argv, ctx->msize, char *);
	}

	ctx->argv[ctx->argc++] = utils_strdup(arg);

	return true;
}

static bool
_translation_ctx_append_args(TranslationCtx *ctx, ...)
{
	const char *arg;
	va_list ap;
	bool success = true;

	assert(ctx != NULL);

	va_start(ap, ctx);

	arg = va_arg(ap, const char *);

	while(arg && success)
	{
		success = _translation_ctx_append_arg(ctx, arg);
		arg = va_arg(ap, char *);
	}

	va_end(ap);

	return success;
}

static ssize_t
_vprintf_error(const Node *node, char *buf, size_t size, const char *format, va_list ap)
{
	const YYLTYPE *locp;
	char tmp[64];
	size_t len;
	ssize_t ret = -1;

	assert(node != NULL);
	assert(buf != NULL);
	assert(format != NULL);

	locp = &node->loc;

	memset(buf, 0, size);
	memset(tmp, 0, sizeof(tmp));

	/* write line number(s) to buffer */
	if(locp->first_line == locp->last_line)
	{
		snprintf(tmp, sizeof(tmp), _("line: %d, "), locp->first_line);
	}
	else
	{
		snprintf(tmp, sizeof(tmp), _("line: %d-%d, "), locp->first_line, locp->last_line);
	}

	len = utils_strlcat(buf, tmp, size);

	if(len > size)
	{
		goto out;
	}

	/* append column(s) to buffer */
	if(locp->first_column == locp->last_column)
	{
		snprintf(tmp, sizeof(tmp), _("column: %d: "), locp->first_column);
	}
	else
	{
		snprintf(tmp, sizeof(tmp), _("column: %d-%d: "), locp->first_column, locp->last_column);
	}

	len = utils_strlcat(buf, tmp, size);

	if(len > size)
	{
		goto out;
	}

	/* append format string to buffer */
	size_t available = size - len;

	int written = vsnprintf(buf + len, available, format, ap);

	if(written >= 0 && (size_t)written < available)
	{
		if(SIZE_MAX - written > len)
		{
			ret = len + written;
		}
	}

out:
	return ret;
}

static void
_set_error(TranslationCtx *ctx, Node *node, const char *fmt, ...)
{
	char msg[4096];
	va_list ap;

	assert(ctx != NULL);
	assert(fmt != NULL);

	if(ctx->err)
	{
		WARNING("translate", "Error message already set.");
		return;
	}

	va_start(ap, fmt);

	if(node)
	{
		ssize_t written = _vprintf_error(node, msg, sizeof(msg), fmt, ap);

		if(written != -1)
		{
			ctx->err = strdup(msg);
		}
		else
		{
			WARNING("translate", "Couldn't set error message, `_vprintf_error' failed.");
		}
	}
	else
	{
		int written = vsnprintf(msg, sizeof(msg), fmt, ap);

		if(written > 0 && (size_t)written < sizeof(msg))
		{
			ctx->err = strdup(msg);
		}
		else
		{
			WARNING("translate", "Couldn't set error message, `vsnprintf' failed.");
		}
	}

	va_end(ap);
}

/*
 *	convert property ids & flags:
 */
static const char *
_property_to_str(PropertyId id)
{
	const char *name = NULL;

	switch(id)
	{
		case PROP_NAME:
			name = "name";
			break;

		case PROP_INAME:
			name = "iname";
			break;

		case PROP_REGEX:
			name = "regex";
			break;

		case PROP_IREGEX:
			name = "iregex";
			break;

		case PROP_ATIME:
			name = "atime";
			break;

		case PROP_CTIME:
			name = "ctime";
			break;

		case PROP_MTIME:
			name = "mtime";
			break;

		case PROP_GROUP:
			name = "group";
			break;

		case PROP_GROUP_ID:
			name = "gid";
			break;

		case PROP_USER:
			name = "user";
			break;

		case PROP_USER_ID:
			name = "uid";
			break;

		case PROP_SIZE:
			name = "size";
			break;

		case PROP_TYPE:
			name = "type";
			break;

		case PROP_FILESYSTEM:
			name = "filesystem";
			break;

		default:
			FATALF("translate", "Invalid property id: %#x", id);
	}

	return name;
}

static const char *
_property_to_arg(PropertyId id, int arg)
{
	const char *name = NULL;

	switch(id)
	{
		case PROP_NAME:
			name = "-name";
			break;

		case PROP_INAME:
			name = "-iname";
			break;

		case PROP_REGEX:
			name = "-regex";
			break;

		case PROP_IREGEX:
			name = "-iregex";
			break;

		case PROP_ATIME:
			name = (arg == TIME_DAYS) ? "-atime" : "-amin";
			break;

		case PROP_CTIME:
			name = (arg == TIME_DAYS) ? "-ctime" : "-cmin";
			break;

		case PROP_MTIME:
			name = (arg == TIME_DAYS) ? "-mtime" : "-mmin";
			break;

		case PROP_GROUP:
			name = "-group";
			break;

		case PROP_GROUP_ID:
			name = "-gid";
			break;

		case PROP_USER:
			name = "-user";
			break;

		case PROP_USER_ID:
			name = "-uid";
			break;

		case PROP_SIZE:
			name = "-size";
			break;

		case PROP_TYPE:
			name = "-type";
			break;

		case PROP_FILESYSTEM:
			name = "-fstype";
			break;

		default:
			FATALF("translate", "Invalid property id: %#x", id);
	}

	return name;
}

static const char *
_flag_to_arg(FileFlag id)
{
	const char *name = NULL;

	switch(id)
	{
		case FILE_FLAG_READABLE:
			name = "-readable";
			break;

		case FILE_FLAG_WRITABLE:
			name = "-writable";
			break;

		case FILE_FLAG_EXECUTABLE:
			name = "-executable";
			break;

		case FILE_FLAG_EMPTY:
			name = "-empty";
			break;

		default:
			FATALF("translate", "Invalid flag: %#x", id);
	}

	return name;
}

static const char *
_cmp_to_str(CompareType cmp)
{
	const char *name = NULL;

	switch(cmp)
	{
		case CMP_LT:
			name = "<";
			break;

		case CMP_LT_EQ:
			name = "<=";
			break;

		case CMP_EQ:
			name = "=";
			break;

		case CMP_GT:
			name = ">";
			break;

		case CMP_GT_EQ:
			name = ">=";
			break;

		default:
			FATALF("translate", "Invalid compare operator: %#x", cmp);
	}

	return name;
}
/*
 *	validation:
 */
static bool
_property_supports_number(PropertyId prop)
{
	return prop == PROP_ATIME || prop == PROP_CTIME || prop == PROP_MTIME ||
	               prop == PROP_SIZE || prop == PROP_GROUP_ID || prop == PROP_USER_ID;
}

static bool
_property_supports_time(PropertyId prop)
{
	return prop == PROP_ATIME || prop == PROP_CTIME || prop == PROP_MTIME;
}

static bool
_property_supports_string(PropertyId prop)
{
	return prop == PROP_NAME || prop == PROP_INAME || prop == PROP_REGEX ||
	               prop == PROP_IREGEX || prop == PROP_GROUP || prop == PROP_USER ||
	               prop == PROP_FILESYSTEM;
}

static bool
_property_supports_size(PropertyId prop)
{
	return prop == PROP_SIZE;
}

static bool
_property_supports_type(PropertyId prop)
{
	return prop == PROP_TYPE;
}

static bool
_property_supports_numeric_operators(PropertyId prop)
{
	return prop == PROP_ATIME || prop == PROP_CTIME || prop == PROP_MTIME || prop == PROP_SIZE;
}

static bool
_test_property(TranslationCtx *ctx, const ConditionNode *node, bool (*test_property)(PropertyId id), const char *type_desc)
{
	assert(ctx != NULL);
	assert(test_property != NULL);
	assert(type_desc != NULL);

	if(!test_property(node->prop))
	{
		_set_error(ctx, (Node *)node, _("Cannot compare a value of type \"%s\" value to property \"%s\"."), type_desc, _property_to_str(node->prop));

		return false;
	}

	if(node->cmp != CMP_EQ && !_property_supports_numeric_operators(node->prop))
	{
		_set_error(ctx, (Node *)node, _("Values of type \"%s\" don't support the \"%s\" operator."), type_desc, _cmp_to_str(node->cmp));

		return false;
	}

	return true;
}

static bool
_append_numeric_cond_arg(TranslationCtx *ctx, const char *arg, CompareType cmp, int64_t val, const char *suffix)
{
	char v0[64];
	char v1[64];
	char *lparen, *rparen;
	bool success = true;

	assert(ctx != NULL);
	assert(arg != NULL);

	if(suffix == NULL)
	{
		suffix = "";
	}

	lparen = QUOTE_ARGS(ctx) ? "\\(" : "(";
	rparen = QUOTE_ARGS(ctx) ? "\\)" : ")";

	switch(cmp)
	{
		case CMP_LT_EQ:
			snprintf(v0, 64, "%" PRId64 "%s", val, suffix);
			snprintf(v1, 64, "-%" PRId64 "%s", val, suffix);
			success = _translation_ctx_append_args(ctx, lparen, arg, v0, "-o", arg, v1, rparen, NULL);
			break;

		case CMP_GT_EQ:
			snprintf(v0, 64, "%" PRId64 "%s", val, suffix);
			snprintf(v1, 64, "+%" PRId64 "%s", val, suffix);
			success = _translation_ctx_append_args(ctx, lparen, arg, v0, "-o", arg, v1, rparen, NULL);
			break;

		case CMP_EQ:
			snprintf(v0, 64, "%" PRId64 "%s", val, suffix);
			success = _translation_ctx_append_args(ctx, arg, v0, NULL);
			break;

		case CMP_LT:
			snprintf(v0, 64, "-%" PRId64 "%s", val, suffix);
			success = _translation_ctx_append_args(ctx, arg, v0, NULL);
			break;

		case CMP_GT:
			snprintf(v0, 64, "+%" PRId64 "%s", val, suffix);
			success = _translation_ctx_append_args(ctx, arg, v0, NULL);
			break;

		default:
			FATALF("translate", "Unsupported compare id: %#x", cmp);
			success = false;
	}

	return success;
}

static bool
_append_time_cond(TranslationCtx *ctx, PropertyId prop, CompareType cmp, int val, TimeInterval unit)
{
	bool success = true;

	assert(ctx != NULL);

	if(unit == TIME_HOURS)
	{
		int64_t val64 = val * 60;

		if(val64 > INT32_MAX || val64 < INT32_MIN)
		{
			FATAL("translate", "Integer overflow.");
			success = false;
		}
		else
		{
			val = (int)val64;
		}
	}

	if(success)
	{
		success = _append_numeric_cond_arg(ctx, _property_to_arg(prop, unit), cmp, val, NULL);
	}

	return success;
}

static bool
_append_size_cond(TranslationCtx *ctx, PropertyId prop, CompareType cmp, int val, UnitType unit)
{
	const char *unit_name = "bytes";
	bool success = true;
	uint64_t bytes = val, max_val = (uint64_t)INT64_MAX / 1024;
	int loops;

	assert(ctx != NULL);

	switch(unit)
	{
		case UNIT_BYTES:
			loops = 0;
			break;

		case UNIT_KB:
			unit_name = "K";
			loops = 1;
			break;

		case UNIT_MB:
			unit_name = "M";
			loops = 2;
			break;

		case UNIT_G:
			unit_name = "G";
			loops = 3;
			break;

		default:
			FATALF("translate", "Unsupported size id: %#x.", unit);
			success = false;
	}

	for(int i = 0; success && i < loops; ++i)
	{
		if(bytes > max_val)
		{
			FATALF("translate", "Integer overflow, couldn't convert %d %s to bytes.", val, unit_name);
			success = false;
			break;
		}

		bytes *= 1024;
	}

	if(success)
	{
		success = _append_numeric_cond_arg(ctx, _property_to_arg(prop, 0), cmp, bytes, "c");
	}

	return success;
}

static bool
_append_type_cond(TranslationCtx *ctx, FileType type)
{
	const char *t;
	bool success = true;

	assert(ctx != NULL);

	switch(type)
	{
		case FILE_REGULAR:
			t = "f";
			break;

		case FILE_DIRECTORY:
			t = "d";
			break;

		case FILE_PIPE:
			t = "p";
			break;

		case FILE_SOCKET:
			t = "s";
			break;

		case FILE_BLOCK:
			t = "b";
			break;

		case FILE_CHARACTER:
			t = "c";
			break;

		case FILE_SYMLINK:
			t = "l";
			break;

		default:
			FATALF("translate", "Unsupported file type: %#x", type);
			success = false;
	}

	if(success)
	{
		success = _translation_ctx_append_args(ctx, "-type", t, NULL);
	}

	return success;
}

static bool
_append_string_arg(TranslationCtx *ctx, const char *propname, const char *val)
{
	char str[PARSER_MAX_EXPRESSION_LENGTH];

	assert(ctx != NULL);

	if(!val)
	{
		val = "";
	}

	if(QUOTE_ARGS(ctx))
	{
		snprintf(str, PARSER_MAX_EXPRESSION_LENGTH, "\"%s\"", val);
	}
	else
	{
		strncpy(str, val, PARSER_MAX_EXPRESSION_LENGTH - 1);
		str[PARSER_MAX_EXPRESSION_LENGTH - 1] = '\0';
	}

	return _translation_ctx_append_args(ctx, propname, str, NULL);
}

/*
 *	process nodes:
 */
static bool _process_node(TranslationCtx *ctx, Node *root);

static bool
_open_parenthese(ExpressionNode *node, Node *child)
{
	if(node->op == OP_AND && child->type == NODE_EXPRESSION)
	{
		return ((ExpressionNode *)child)->op == OP_OR;
	}

	return false;
}

static bool
_process_expression(TranslationCtx *ctx, ExpressionNode *node)
{
	char *lparen, *rparen;
	bool open = false;
	bool success = false;

	assert(ctx != NULL);
	assert(node != NULL);
	assert(node->first != NULL);
	assert(node->second != NULL);

	lparen = QUOTE_ARGS(ctx) ? "\\(" : "(";
	rparen = QUOTE_ARGS(ctx) ? "\\)" : ")";

	if(node->op != OP_AND && node->op != OP_OR)
	{
		FATALF("translate", "Unsupported operator: %#x", node->op);
	}

	if(_open_parenthese(node, node->first))
	{
		_translation_ctx_append_arg(ctx, lparen);
		open = true;
	}

	if(_process_node(ctx, node->first))
	{
		if(open)
		{
			_translation_ctx_append_arg(ctx, rparen);
			open = false;
		}

		if(!_translation_ctx_append_arg(ctx, (node->op == OP_AND) ? "-a" : "-o"))
		{
			goto out;
		}

		if(_open_parenthese(node, node->second))
		{
			_translation_ctx_append_arg(ctx, lparen);
			open = true;
		}

		if(_process_node(ctx, node->second))
		{
			if(open)
			{
				_translation_ctx_append_arg(ctx, rparen);
			}

			success = true;
		}
	}

out:
	return success;
}

static bool
_process_condition(TranslationCtx *ctx, ConditionNode *node)
{
	bool success = false;

	assert(ctx != NULL);
	assert(node != NULL);

	switch(node->value->vtype)
	{
		case VALUE_NUMERIC:
			if((success = _test_property(ctx, node, &_property_supports_number, "numeric")))
			{
				if(_property_supports_time(node->prop))
				{
					success = _append_time_cond(ctx, node->prop, node->cmp, node->value->value.ivalue, TIME_MINUTES);
				}
				else if(_property_supports_size(node->prop))
				{
					success = _append_size_cond(ctx, node->prop, node->cmp, node->value->value.ivalue, UNIT_BYTES);
				}
				else
				{
					success = _append_numeric_cond_arg(ctx, _property_to_arg(node->prop, 0), node->cmp, node->value->value.ivalue, 0);
				}
			}
			break;

		case VALUE_TIME:
			if((success = _test_property(ctx, node, &_property_supports_time, "time")))
			{
				success = _append_time_cond(ctx, node->prop, node->cmp, node->value->value.pair.a, node->value->value.pair.b);
			}
			break;

		case VALUE_STRING:
			if((success = _test_property(ctx, node, &_property_supports_string, "string")))
			{
				success = _append_string_arg(ctx, _property_to_arg(node->prop, 0), node->value->value.svalue);
			}
			break;

		case VALUE_SIZE:
			if((success = _test_property(ctx, node, &_property_supports_size, "size")))
			{
				success = _append_size_cond(ctx, node->prop, node->cmp, node->value->value.pair.a, node->value->value.pair.b);
			}
			break;

		case VALUE_TYPE:
			if((success = _test_property(ctx, node, &_property_supports_type, "filetype")))
			{
				success = _append_type_cond(ctx, node->value->value.ivalue);
			}
			break;

		default:
			FATALF("translate", "Unsupported value type: %#x", node->value->vtype);
			break;
	}

	return success;
}

static bool
_process_flag(TranslationCtx *ctx, ValueNode *node)
{
	assert(ctx != NULL);
	assert(node != NULL);

	return _translation_ctx_append_arg(ctx, _flag_to_arg(node->value.ivalue));
}

static bool
_process_not(TranslationCtx *ctx, NotNode *node)
{
	bool success = false;

	assert(ctx != NULL);
	assert(node != NULL);

	if(_translation_ctx_append_args(ctx, "!", NULL))
	{
		char *lparen = QUOTE_ARGS(ctx) ? "\\(" : "(";
		char *rparen = QUOTE_ARGS(ctx) ? "\\)" : ")";
		bool open = false;

		if(node->expr->type == NODE_EXPRESSION)
		{
			_translation_ctx_append_arg(ctx, lparen);
			open = true;
		}

		if((success = _process_node(ctx, ((NotNode *)node)->expr)) && open)
		{
			_translation_ctx_append_arg(ctx, rparen);
		}
	}

	return success;
}

static bool
_process_node(TranslationCtx *ctx, Node *node)
{
	assert(ctx != NULL);
	assert(node != NULL);

	if(node)
	{
		if(node->type == NODE_EXPRESSION)
		{
			return _process_expression(ctx, (ExpressionNode *)node);
		}
		else if(node->type == NODE_CONDITION)
		{
			return _process_condition(ctx, (ConditionNode *)node);
		}
		else if(node->type == NODE_VALUE && ((ValueNode *)node)->vtype == VALUE_FLAG)
		{
			return _process_flag(ctx, (ValueNode *)node);
		}
		else if(node->type == NODE_NOT)
		{
			return _process_not(ctx, (NotNode *)node);
		}
		else
		{
			FATALF("translate", "Unsupported node type: %#x", node->type);
		}
	}

	return false;
}

static bool
_translate(TranslationCtx *ctx)
{
	assert(ctx != NULL);

	return _process_node(ctx, ctx->root);
}

bool
translate(Node *root, TranslationFlags flags, size_t *argc, char ***argv, char **err)
{
	TranslationCtx ctx;
	bool success;

	assert(argc != NULL);
	assert(argv != NULL);
	assert(err != NULL);

	TRACEF("translate", "Translating root node with flags %#x.", flags);

	if(root)
	{
		_translation_ctx_init(&ctx, root, flags);

		success = _translate(&ctx);

		*argc = ctx.argc;
		*argv = ctx.argv;
		*err = ctx.err;
	}
	else
	{
		TRACE("translate", "Root node not set.");
		success = true;
	}

	TRACEF("translate", "Translation completed with status %d.", success);

	return success;
}

