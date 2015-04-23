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
   @version 0.1.0
*/
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "ast.h"
#include "utils.h"

TimeInterval
ast_str_to_interval(const char *str)
{
	TimeInterval interval = TIME_UNDEFINED;

	if(str)
	{
		if(str[0] == 'm')
		{
			interval = TIME_MINUTES;
		}
		else if(str[0] == 'h')
		{
			interval = TIME_HOURS;
		}
		else if(str[0] == 'd')
		{
			interval = TIME_DAYS;
		}
	}

	return interval;
}

PropertyId
ast_str_to_property_id(const char *str)
{
	PropertyId id = PROP_UNDEFINED;

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
		fprintf(stderr, "Invalid str name: \"%s\"", str);
	}

	return id;
}

OperatorType
ast_str_to_operator(const char *str)
{
	return !strcmp(str, "and") ? OP_AND : OP_OR;
}

UnitType
ast_str_to_unit(const char *str)
{
	UnitType unit = UNIT_UNDEFINED;

	if(str)
	{
		if(str[0] == 'b')
		{
			unit = UNIT_BYTES;
		}
		else if(str[0] == 'k')
		{
			unit = UNIT_KB;
		}
		else if(str[0] == 'M' || str[0] == 'm')
		{
			unit = UNIT_MB;
		}
		else if(str[0] == 'g' || str[0] == 'G')
		{
			unit = UNIT_G;
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
		if(str[0] == 'f')
		{
			type = FILE_REGULAR;
		}
		else if(str[0] == 'd')
		{
			type = FILE_DIRECTORY;
		}
		else if(str[0] == 'b')
		{
			type = FILE_BLOCK;
		}
		else if(str[0] == 'c')
		{
			type = FILE_CHARACTER;
		}
		else if(str[0] == 'p')
		{
			type = FILE_PIPE;
		}
		else if(str[0] == 's')
		{
			type = FILE_SOCKET;
		}
		else if(str[0] == 'l')
		{
			type = FILE_SYMLINK;
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
		switch(str[0])
		{
			case 'r':
				flag = FILE_FLAG_READABLE;
				break;

			case 'w':
				flag = FILE_FLAG_WRITABLE;
				break;

			case 'e':
				flag = FILE_FLAG_EXECUTABLE;
				break;

			default:
				fprintf(stderr, "Invalid str name: \"%s\"", str);
		}
	}

	return flag;
}

static void *
_node_new(size_t size, NodeType type)
{
	void *ptr = utils_malloc(size);

	memset(ptr, 0, size);

	((Node *)ptr)->type = type;

	return ptr;
}

/*! @cond INTERNAL */
#define node_new(t, id) (t *)_node_new(sizeof(t), id)
/*! @endcond */

Node *
ast_value_node_new_str_nodup(char *value)
{
	ValueNode *node = node_new(ValueNode, NODE_VALUE);

	node->vtype = VALUE_STRING;
	node->value.svalue = value;

	return (Node *)node;
}

Node *
ast_value_node_new_int(int value)
{
	ValueNode *node = node_new(ValueNode, NODE_VALUE);

	node->vtype = VALUE_NUMERIC;
	node->value.ivalue = value;

	return (Node *)node;
}

Node *
ast_value_node_new_type(FileType type)
{
	ValueNode *node = node_new(ValueNode, NODE_VALUE);

	node->vtype = VALUE_TYPE;
	node->value.ivalue = type;

	return (Node *)node;
}

Node *
ast_value_node_new_flag(FileFlag flag)
{
	ValueNode *node = node_new(ValueNode, NODE_VALUE);

	node->vtype = VALUE_FLAG;
	node->value.ivalue = flag;

	return (Node *)node;
}

Node *
ast_value_node_new_int_pair(ValueType type, int a, int b)
{
	ValueNode *node = node_new(ValueNode, NODE_VALUE);

	node->vtype = type;
	node->value.pair.a = a;
	node->value.pair.b = b;

	return (Node *)node;
}

Node *
ast_cond_node_new(PropertyId prop, CompareType cmp, ValueNode *value)
{
	ConditionNode *node = node_new(ConditionNode, NODE_CONDITION);

	node->prop = prop;
	node->cmp = cmp;
	node->value = value;

	return (Node *)node;
}

Node *
ast_expr_node_new(Node *first, OperatorType op, Node *second)
{
	ExpressionNode *node = node_new(ExpressionNode, NODE_EXPRESSION);

	node->first = first;
	node->op = op;
	node->second = second;

	return (Node *)node;
}

void
ast_free(Node *node)
{
	if(node)
	{
		if(node->type == NODE_EXPRESSION)
		{
			ExpressionNode *exnode = (ExpressionNode *)node;

			/* free child nodes */
			ast_free(exnode->first);
			ast_free(exnode->second);
		}
		else if(node->type == NODE_CONDITION)
		{
			ConditionNode *cnode = (ConditionNode *)node;

			/* free value node assigned to condition */
			ast_free((Node *)cnode->value);
		}
		else if(node->type == NODE_VALUE)
		{
			ValueNode *vnode = (ValueNode *)node;

			/* free string value */
			if(vnode->vtype == VALUE_STRING)
			{
				free(vnode->value.svalue);
			}
		}
		else
		{
			fprintf(stderr, "%s:: invalid node type: %d\n", __func__, node->type);
		}

		free(node);
	}
}

