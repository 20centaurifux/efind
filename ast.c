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
   @file ast.c
   @brief Abstract syntax tree construction.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include "ast.h"
#include "log.h"
#include "utils.h"

/*! @cond INTERNAL */
#define NODE_TYPE_IS_VALID(t) (t > NODE_UNDEFINED && t <= NODE_COMPARE)
#define FILE_FLAG_IS_VALID(f) (f > FILE_FLAG_UNDEFINED && f <= FILE_FLAG_EMPTY)
#define PROPERTY_IS_VALID(p)  (p > PROP_UNDEFINED && p <= PROP_TYPE)
#define CMP_TYPE_IS_VALID(c)  (c >= CMP_UNDEFINED && c <= CMP_GT)
#define OPERATOR_IS_VALID(op) (op >= OP_UNDEFINED && op <= OP_COMMA)
/*! @endcond */

TimeInterval
ast_str_to_interval(const char *str)
{
	TimeInterval interval = TIME_UNDEFINED;

	if(str)
	{
		switch(*str)
		{
			case 'm':
				interval = TIME_MINUTES;
				break;

			case 'h':
				interval = TIME_HOURS;
				break;

			case 'd':
				interval = TIME_DAYS;
				break;

			default:
				FATALF("parser", "Invalid time interval: %s", str);
		}
	}
		
	return interval;
}

PropertyId
ast_str_to_property_id(const char *str)
{
	PropertyId id = PROP_UNDEFINED;

	if(str)
	{
		if(!strcmp(str, "name"))
		{
			id = PROP_NAME;
		}
		else if(!strcmp(str, "iname"))
		{
			id = PROP_INAME;
		}
		else if(!strcmp(str, "atime"))
		{
			id = PROP_ATIME;
		}
		else if(!strcmp(str, "ctime"))
		{
			id = PROP_CTIME;
		}
		else if(!strcmp(str, "mtime"))
		{
			id = PROP_MTIME;
		}
		else if(!strcmp(str, "size"))
		{
			id = PROP_SIZE;
		}
		else if(!strcmp(str, "group"))
		{
			id = PROP_GROUP;
		}
		else if(!strcmp(str, "gid"))
		{
			id = PROP_GROUP_ID;
		}
		else if(!strcmp(str, "user"))
		{
			id = PROP_USER;
		}
		else if(!strcmp(str, "uid"))
		{
			id = PROP_USER_ID;
		}
		else if(!strcmp(str, "type"))
		{
			id = PROP_TYPE;
		}
		else
		{
			FATALF("parser", "Invalid property: %s", str);
		}
	}

	return id;
}

OperatorType
ast_str_to_operator(const char *str)
{
	OperatorType op = OP_UNDEFINED;

	if(str)
	{
		op = !strcmp(str, "and") ? OP_AND : OP_OR;
	}

	return op;
}

UnitType
ast_str_to_unit(const char *str)
{
	UnitType unit = UNIT_UNDEFINED;

	if(str)
	{
		switch(*str)
		{
			case 'b':
				unit = UNIT_BYTES;
				break;

			case 'k':
				unit = UNIT_KB;
				break;

			case 'M':
			case 'm':
				unit = UNIT_MB;
				break;

			case 'g':
			case 'G':
				unit = UNIT_G;
				break;

			default:
				FATALF("parser", "Invalid unit: %s", str);
		}
	}

	return unit;
}

FileType
ast_str_to_type(const char *str)
{
	FileType type = FILE_UNDEFINED;

	if(str)
	{
		switch(*str)
		{
			case 'f':
				type = FILE_REGULAR;
				break;

			case 'd':
				type = FILE_DIRECTORY;
				break;

			case 'b':
				type = FILE_BLOCK;
				break;

			case 'c':
				type = FILE_CHARACTER;
				break;

			case 'p':
				type = FILE_PIPE;
				break;

			case 's':
				type = FILE_SOCKET;
				break;

			case 'l':
				type = FILE_SYMLINK;
				break;

			default:
				FATALF("parser", "Invalid file type: %s", str);
		}
	}

	return type;
}

FileFlag
ast_str_to_flag(const char *str)
{
	FileFlag flag = FILE_FLAG_UNDEFINED;

	if(str)
	{
		if(!strcmp(str, "readable"))
		{
			flag = FILE_FLAG_READABLE;
		}
		else if(!strcmp(str, "writable"))
		{
			flag = FILE_FLAG_WRITABLE;
		}
		else if(!strcmp(str, "executable"))
		{
			flag = FILE_FLAG_EXECUTABLE;
		}
		else if(!strcmp(str, "empty"))
		{
			flag = FILE_FLAG_EMPTY;
		}
		else
		{
			FATALF("parser", "Invalid flag: %s", str);
		}
	}

	return flag;
}

static void *
_node_new(Allocator *alloc, const YYLTYPE *locp, size_t size, NodeType type)
{
	assert(alloc != NULL);
	assert(size > 0);
	assert(NODE_TYPE_IS_VALID(type));
	assert(locp != NULL);

	void *ptr = alloc->alloc(alloc);

	memset(ptr, 0, size);

	((Node *)ptr)->type = type;
	((Node *)ptr)->loc = *locp;

	 return ptr;
}

/*! @cond INTERNAL */
#define node_new(a, l, t, id) (t *)_node_new(a, l, sizeof(t), id)
/*! @endcond */

Node *
ast_true_node_new(Allocator *alloc, const YYLTYPE *locp)
{
	TrueNode *node = node_new(alloc, locp, TrueNode, NODE_TRUE);

	TRACEF("parser", "new TrueNode[%#x]", NODE_TRUE);

	return (Node *)node;
}

Node *
ast_value_node_new_str_nodup(Allocator *alloc, const YYLTYPE *locp, char *value)
{
	ValueNode *node = node_new(alloc, locp, ValueNode, NODE_VALUE);

	TRACEF("parser", "new ValueNode[%#x] (vtype=%#x, value=%s)", NODE_VALUE, VALUE_STRING, value ? value : "NULL");

	node->vtype = VALUE_STRING;
	node->value.svalue = value;

	return (Node *)node;
}

Node *
ast_value_node_new_int(Allocator *alloc, const YYLTYPE *locp, int value)
{
	ValueNode *node = node_new(alloc, locp, ValueNode, NODE_VALUE);

	TRACEF("parser", "new ValueNode[%#x] (vtype=%#x, value=%d)", NODE_VALUE, VALUE_NUMERIC, value);

	node->vtype = VALUE_NUMERIC;
	node->value.ivalue = value;

	return (Node *)node;
}

Node *
ast_value_node_new_type(Allocator *alloc, const YYLTYPE *locp, FileType type)
{
	ValueNode *node = node_new(alloc, locp, ValueNode, NODE_VALUE);

	TRACEF("parser", "new ValueNode[%#x] (vtype=%#x, type=%#x)", NODE_VALUE, VALUE_TYPE, type);

	node->vtype = VALUE_TYPE;
	node->value.ivalue = type;

	return (Node *)node;
}

Node *
ast_value_node_new_flag(Allocator *alloc, const YYLTYPE *locp, FileFlag flag)
{
	ValueNode *node = node_new(alloc, locp, ValueNode, NODE_VALUE);

	TRACEF("parser", "new ValueNode[%#x] (vtype=%#x, flag=%#x)", NODE_VALUE, VALUE_FLAG, flag);

	node->vtype = VALUE_FLAG;
	node->value.ivalue = flag;

	return (Node *)node;
}

Node *
ast_value_node_new_int_pair(Allocator *alloc, const YYLTYPE *locp, ValueType type, int a, int b)
{
	assert(type == VALUE_TIME || type == VALUE_SIZE);

	TRACEF("parser", "new ValueNode[%#x] (vtype=%#x, pair.a=%d, pair.b=%d)", NODE_VALUE, type, a, b);

	ValueNode *node = node_new(alloc, locp, ValueNode, NODE_VALUE);

	node->vtype = type;
	node->value.pair.a = a;
	node->value.pair.b = b;

	return (Node *)node;
}

Node *
ast_compare_node_new(Allocator *alloc, const YYLTYPE *locp, Node *first, CompareType cmp, Node *second)
{
	CompareNode *node = node_new(alloc, locp, CompareNode, NODE_COMPARE);

	assert(first != NULL);
	assert(second != NULL);
	assert(CMP_TYPE_IS_VALID(cmp));

	TRACEF("parser", "new CompareNode[%#x] (first=new Node(type=%#x), cmp=%#x, second=new Node(type=%#x))", NODE_COMPARE, first->type, cmp, second->type);

	node->first = first;
	node->cmp = cmp;
	node->second = second;

	return (Node *)node;
}

static void
_ast_cond_node_init(ConditionNode *node, PropertyId prop, CompareType cmp, ValueNode *value)
{
	assert(node != NULL);
	assert(PROPERTY_IS_VALID(prop));
	assert(CMP_TYPE_IS_VALID(cmp));
	assert((cmp == CMP_UNDEFINED && value == NULL) || (cmp != CMP_UNDEFINED && value != NULL));

	node->prop = prop;
	node->cmp = cmp;
	node->value = value;
}

Node *
ast_cond_node_new(Allocator *alloc, const YYLTYPE *locp, PropertyId prop, CompareType cmp, ValueNode *value)
{
	ConditionNode *node = node_new(alloc, locp, ConditionNode, NODE_CONDITION);

	TRACEF("parser", "new ConditionNode[%#x] (prop=%#x, cmp=%#x, new ValueNode(vtype=%#x))", NODE_CONDITION, prop, cmp, value->vtype);

	_ast_cond_node_init(node, prop, cmp, value);

	return (Node *)node;
}

static void
_ast_expr_node_init(ExpressionNode *node, Node *first, OperatorType op, Node *second)
{
	assert(node != NULL);
	assert(first != NULL);
	assert(OPERATOR_IS_VALID(op));
	assert((op == OP_UNDEFINED && second == NULL) || (op != OP_UNDEFINED && second != NULL));

	node->first = first;
	node->op = op;
	node->second = second;
}

Node *
ast_expr_node_new(Allocator *alloc, const YYLTYPE *locp, Node *first, OperatorType op, Node *second)
{
	ExpressionNode *node;

	assert(OPERATOR_IS_VALID(op));

	TRACEF("parser", "new ExpressionNode[%#x] (first=new Node(type=%#x), op=%#x, second=new Node(type=%#x))", NODE_EXPRESSION, first->type, op, second->type);

	node = node_new(alloc, locp, ExpressionNode, NODE_EXPRESSION);

	_ast_expr_node_init(node, first, op, second);

	return (Node *)node;
}

Node *
ast_func_node_new(Allocator *alloc, const YYLTYPE *locp, char *name, Node *args)
{
	FuncNode *node;

	assert(name != NULL);

	if(args)
	{
		TRACEF("parser", "new FuncNode[%#x] (name=%s, args=new Node(type=%#x))", NODE_FUNC, name, args->type);
	}
	else
	{
		TRACEF("parser", "new FuncNode[%#x] (name=%s, args=NULL)", NODE_FUNC, name); 
	}

	node = node_new(alloc, locp, FuncNode, NODE_FUNC);

	node->name = name;
	node->args = args;

	return (Node *)node;
}

RootNode *
ast_root_node_new(Allocator *alloc, const YYLTYPE *locp, Node *exprs, Node *post_exprs)
{
	RootNode *node;

	assert(exprs != NULL || post_exprs != NULL);

	if(exprs && post_exprs)
	{
		TRACEF("parser", "new RootNode[%#x] (exprs=new Node(type=%#x), post_exprs=new Node(type=%#x))", NODE_ROOT, exprs->type, post_exprs->type);
	}
	else if(!post_exprs)
	{
		TRACEF("parser", "new RootNode[%#x] (exprs=new Node(type=%#x), post_exprs=NULL)", NODE_ROOT, exprs->type);
	}
	else
	{
		TRACEF("parser", "new RootNode[%#x] (exprs=NULL, post_exprs=new Node(type=%#x))", NODE_ROOT, post_exprs->type);
	}

	node = node_new(alloc, locp, RootNode, NODE_ROOT);

	node->exprs = exprs;
	node->post_exprs = post_exprs;

	return node;
}

