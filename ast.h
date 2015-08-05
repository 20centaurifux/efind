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
   @file ast.h
   @brief Abstract syntax tree construction.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
   @version 0.1.0
*/
#ifndef AST_H
#define AST_H

#include <datatypes.h>
#include "parser.y.h"

/**
   @enum PropertyId
   @brief Searchable file attributes.
 */
typedef enum
{
	/*! Undefined. */
	PROP_UNDEFINED,
	/*! Filename. */
	PROP_NAME,
	/*! Filename (case insensitive). */
	PROP_INAME,
	/*! Last access time. */
	PROP_ATIME,
	/*! Creation time. */
	PROP_CTIME,
	/*! Last modification time. */
	PROP_MTIME,
	/*! File size. */
	PROP_SIZE,
	/*! Name of the owning group. */
	PROP_GROUP,
	/*! ID of the owning group. */
	PROP_GROUP_ID,
	/*! Name of the owner. */
	PROP_USER,
	/*! ID of the owner. */
	PROP_USER_ID,
	/*! File type. */
	PROP_TYPE
} PropertyId;

/**
   @enum CompareType
   @brief Compare operators.
 */
typedef enum
{
	/*! Undefined. */
	CMP_UNDEFINED,
	/*! Equals. */
	CMP_EQ,
	/*! Less or equal. */
	CMP_LT_EQ,
	/*! Less than. */
	CMP_LT,
	/*! Greater or equal. */
	CMP_GT_EQ,
	/*! Greater than. */
	CMP_GT
} CompareType;

/**
   @enum OperatorType
   @brief Operators.
 */
typedef enum
{
	/*! Undefined. */
	OP_UNDEFINED,
	/*! And operator. */
	OP_AND,
	/*! Or operator. */
	OP_OR
} OperatorType;

/**
   @enum UnitType
   @brief File type units.
 */
typedef enum
{
	/*! Undefined. */
	UNIT_UNDEFINED,
	/*! Bytes. */
	UNIT_BYTES,
	/*! Kilobytes. */
	UNIT_KB,
	/*! Megabytes. */
	UNIT_MB,
	/*! Gigabytes. */
	UNIT_G
} UnitType;

/**
   @enum FileType
   @brief File types.
  */
typedef enum
{
	/*! Undefined. */
	FILE_UNDEFINED,
	/*! Regular file. */
	FILE_REGULAR,
	/*! Directory. */
	FILE_DIRECTORY,
	/*! Pipe. */
	FILE_PIPE,
	/*! Named pipe (FIFO). */
	FILE_SOCKET,
	/*! Block device. */
	FILE_BLOCK,
	/*! Character device. */
	FILE_CHARACTER,
	/*! Symbolic link. */
	FILE_SYMLINK
} FileType;

/**
   @enum FileFlag
   @brief File flags.
  */
typedef enum
{
	/*! Undefined. */
	FILE_FLAG_UNDEFINED,
	/*! File is readable. */
	FILE_FLAG_READABLE,
	/*! File is writable. */
	FILE_FLAG_WRITABLE,
	/*! File is executable. */
	FILE_FLAG_EXECUTABLE
} FileFlag;

/**
   @enum NodeType
   @brief Tree node types.
 */
typedef enum
{
	/*! Undefined. */
	NODE_UNDEFINED,
	/*! Node is of type ExpressionNode. */
	NODE_EXPRESSION,
	/*! Node is of type ConditionNode. */
	NODE_CONDITION,
	/*! Node is of type ValueNode. */
	NODE_VALUE
} NodeType;

/**
   @struct Node
   @brief Node base type.
 */
typedef struct
{
	/*! Type of the node. */
	NodeType type;
	/*! Location information. */
	YYLTYPE loc;
} Node;

/**
   @enum ValueType
   @brief Type of data stored in a ValueType.
  */
typedef enum
{
	/*! Undefined. */
	VALUE_UNDEFINED,
	/*! Number (int). */
	VALUE_NUMERIC,
	/*! String. */
	VALUE_STRING,
	/*! Time interval and value. */
	VALUE_TIME,
	/*! File size unit and value. */
	VALUE_SIZE,
	/*! File type. */
	VALUE_TYPE,
	/*! File flag. */
	VALUE_FLAG
} ValueType;

/**
   @enum TimeInterval
   @brief Time intervals.
 */
typedef enum
{
	/*! Undefined. */
	TIME_UNDEFINED,
	/*! Minutes.. */
	TIME_MINUTES,
	/*! Hours. */
	TIME_HOURS,
	/*! Days. */
	TIME_DAYS
} TimeInterval;

/**
   @struct ValueNode
   @brief Value nodes can hold a string, number or number-pair.
 */
typedef struct
{
	/*! Base type. */
	Node padding;
	/*! Type of the stored value. */
	ValueType vtype;
	/*! Stored value. */
	union
	{
		/*! A string. */
		char *svalue;
		/*! An integer. */
		int ivalue;
		/*! An integer-pair (e.g. file size and unit). */
		struct
		{
			/*! First element of the pair. */
			int a;
			/*! Last element of the pair*/
			int b;
		} pair;
	} value;
} ValueNode;

/**
   @struct ConditionNode
   @brief Condition nodes hold a property id, a compare operator and a value node.
 */
typedef struct
{
	/*! Base type. */
	Node padding;
	/*! A property id. */
	PropertyId prop;
	/*! A compare operator. */
	CompareType cmp;
	/*! A value node. */
	ValueNode *value;
} ConditionNode;

/**
   @struct ExpressionNode
   @brief Expression nodes hold at least a single node. A second node and a related operator
          can also be assigned.
 */
typedef struct
{
	/*! Base type. */
	Node padding;
	/*! A node. */
	Node *first;
	/*! An operator. */
	OperatorType op;
	/*! A second node. */
	Node *second;
} ExpressionNode;

/**
   @param str string to convert
   @return a TimeInterval

   Converts a string to a TimeInterval.
 */
TimeInterval ast_str_to_interval(const char *str);

/**
   @param str string to convert
   @return a PropertyId

   Converts a string to a PropertyId.
 */
PropertyId ast_str_to_property_id(const char *str);

/**
   @param str string to convert
   @return an OperatorType

   Converts a string to a OperatorType.
 */
OperatorType ast_str_to_operator(const char *str);

/**
   @param str string to convert
   @return An UnitType.

   Converts a string to a UnitType.
 */
UnitType ast_str_to_unit(const char *str);

/**
   @param str string to convert
   @return a FileType

   Converts a string to a FileType.
 */
FileType ast_str_to_type(const char *str);

/**
   @param str string to convert
   @return a FileFlag

   Converts a string to a FileFlag.
 */
FileFlag ast_str_to_flag(const char *str);

/**
   @param value string to assign
   @return a new Node

   Creates a new ValueNode with an assigned string.
 */
Node *ast_value_node_new_str_nodup(char *value);

/**
   @param alloc an Allocator
   @param locp location information
   @param value string to assign
   @return a new Node

   Creates a new ValueNode with an assigned string getting memory from an Allocator.
 */
Node *ast_value_node_new_str_nodup_alloc(Allocator *alloc, const YYLTYPE *locp, char *value);

/**
   @param value a number to assign
   @return a new Node

   Creates a new ValueNode with an assigned integer.
 */
Node *ast_value_node_new_int(int value);

/**
   @param alloc an Allocator
   @param locp location information
   @param value a number to assign
   @return a new Node

   Creates a new ValueNode with an assigned integer getting memory from an Allocator.
 */
Node *ast_value_node_new_int_alloc(Allocator *alloc, const YYLTYPE *locp, int value);

/**
   @param type file type
   @return a new Node

   Creates a new ValueType with an assigned file type.
 */
Node *ast_value_node_new_type(FileType type);

/**
   @param alloc an Allocator
   @param locp location information
   @param type file type
   @return a new Node

   Creates a new ValueNode with an assigned file type getting memory from an Allocator.
 */
Node *ast_value_node_new_type_alloc(Allocator *alloc, const YYLTYPE *locp, FileType type);

/**
   @param flag file flag
   @return a new Node

   Creates a new ValueNode with an assigned file flag.
 */
Node *ast_value_node_new_flag(FileFlag flag);

/**
   @param alloc an Allocator
   @param locp location information
   @param flag file flag
   @return a new Node

   Creates a new ValueNode with an assigned file flag getting memory from an Allocator.
 */
Node *ast_value_node_new_flag_alloc(Allocator *alloc, const YYLTYPE *locp, FileFlag flag);

/**
   @param type type of the value to store
   @param a first integer to store
   @param b second integer to store
   @return a new Node

   Creates a new ValueNode with an assigned integer pair.
 */
Node *ast_value_node_new_int_pair(ValueType type, int a, int b);

/**
   @param alloc an Allocator
   @param locp location information
   @param type type of the value to store
   @param a first integer to store
   @param b second integer to store
   @return a new Node

   Creates a new ValueNode with an assigned integer pair getting memory from an Allocator.
 */
Node *ast_value_node_new_int_pair_alloc(Allocator *alloc, const YYLTYPE *locp, ValueType type, int a, int b);

/**
   @param prop a PropertyId
   @param cmp a compare operator
   @param value a ValueNode
   @return a new Node

   Creates a new ConditionNode.
 */
Node *ast_cond_node_new(PropertyId prop, CompareType cmp, ValueNode *value);

/**
   @param alloc an Allocator
   @param locp location information
   @param prop a PropertyId
   @param cmp a compare operator
   @param value a ValueNode
   @return a new Node

   Creates a new ConditionNode getting memory from an Allocator.
 */
Node *ast_cond_node_new_alloc(Allocator *alloc, const YYLTYPE *locp, PropertyId prop, CompareType cmp, ValueNode *value);

/**
   @param first a Node
   @param op an operator
   @param second a second node
   @return a new Node

   Creates a new ExpressionNode.
 */
Node *ast_expr_node_new(Node *first, OperatorType op, Node *second);

/**
   @param alloc an Allocator
   @param first a Node
   @param op an operator
   @param second a second node
   @return a new Node

   Creates a new ExpressionNode getting memory from an Allocator.
 */
Node *ast_expr_node_new_alloc(Allocator *alloc, const YYLTYPE *locp, Node *first, OperatorType op, Node *second);

/**
   @param node node to free

   Frees a node.
 */
void ast_free(Node *node);

#endif

