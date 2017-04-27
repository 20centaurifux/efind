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
 * @file ast.h
 * @brief Abstract syntax tree construction.
 * @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
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
	OP_OR,
	/*! Comma operator. */
	OP_COMMA
} OperatorType;

/**
   @enum UnitType
   @brief File size units.
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
   @brief Node types of the abstract syntax tree.
 */
typedef enum
{
	/*! Undefined. */
	NODE_UNDEFINED,
	/*! Node is of type RootNode. */
	NODE_ROOT,
	/*! Node is of type TrueNode. */
	NODE_TRUE,
	/*! Node is of type ExpressionNode. */
	NODE_EXPRESSION,
	/*! Node is of type ConditionNode. */
	NODE_CONDITION,
	/*! Node is of type ValueNode. */
	NODE_VALUE,
	/*! Node is of type FuncNode. */
	NODE_FUNC,
	/*! Node is of type CompareNode. */
	NODE_COMPARE
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
   @struct RootNode
   @brief Root node of the tree.
 */
typedef struct
{
	/*! Base type. */
	Node padding;
	/*! Root node of the expression tree. */
	Node *exprs;
	/*! Root node of the post-processing expression tree. */
	Node *post_exprs;
} RootNode;

/**
   @struct TrueNode
   @brief A node representing true.
 */
typedef struct
{
	/*! Base type. */
	Node padding;
} TrueNode;

/**
   @enum ValueType
   @brief Type of data stored in a ValueNode.
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
   @struct CompareNode
   @brief Compare nodes compare a node to another.
 */
typedef struct
{
	/*! Base type. */
	Node padding;
	/*! A node. */
	Node *first;
	/*! A compare operator. */
	CompareType cmp;
	/*! A second node. */
	Node *second;
} CompareNode;

/**
   @struct ExpressionNode
   @brief Expression nodes combine two nodes with an operator. */
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
   @struct FuncNode
   @brief FuncNodes hold a function name and arguments.
 */
typedef struct
{
	/*! Base type. */
	Node padding;
	/*! Function name. */
	char *name;
	/*! First function argument. */
	Node *args;
} FuncNode;

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

   Converts a string to an OperatorType.
 */
OperatorType ast_str_to_operator(const char *str);

/**
   @param str string to convert
   @return An UnitType.

   Converts a string to an UnitType.
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
   @param alloc an Allocator
   @param locp location information
   @return a new Node

   Creates a new TrueNode from an Allocator.
 */
Node *ast_true_node_new(Allocator *alloc, const YYLTYPE *locp);

/**
   @param alloc an Allocator
   @param locp location information
   @param value string to assign
   @return a new Node

   Creates a new ValueNode with an assigned string from an Allocator. Only the pointer,
   not the full string, is copied to the node.
 */
Node *ast_value_node_new_str_nodup(Allocator *alloc, const YYLTYPE *locp, char *value);

/**
   @param alloc an Allocator
   @param locp location information
   @param value a number to assign
   @return a new Node

   Creates a new ValueNode with an assigned integer from an Allocator.
 */
Node *ast_value_node_new_int(Allocator *alloc, const YYLTYPE *locp, int value);

/**
   @param alloc an Allocator
   @param locp location information
   @param type file type
   @return a new Node

   Creates a new ValueNode with an assigned file type from an Allocator.
 */
Node *ast_value_node_new_type(Allocator *alloc, const YYLTYPE *locp, FileType type);

/**
   @param alloc an Allocator
   @param locp location information
   @param flag file flag
   @return a new Node

   Creates a new ValueNode with an assigned file flag from an Allocator.
 */
Node *ast_value_node_new_flag(Allocator *alloc, const YYLTYPE *locp, FileFlag flag);

/**
   @param alloc an Allocator
   @param locp location information
   @param type type of the value to store
   @param a first integer to store
   @param b second integer to store
   @return a new Node

   Creates a new ValueNode with an assigned integer pair from an Allocator.
 */
Node *ast_value_node_new_int_pair(Allocator *alloc, const YYLTYPE *locp, ValueType type, int a, int b);

/**
   @param alloc an Allocator
   @param locp location information
   @param first a node
   @param cmp a compare operator
   @param second another node
   @return a new Node

   Creates a new CompareNode from an Allocator.
 */
Node *ast_compare_node_new(Allocator *alloc, const YYLTYPE *locp, Node *first, CompareType cmp, Node *second);

/**
   @param alloc an Allocator
   @param locp location information
   @param prop a PropertyId
   @param cmp a compare operator
   @param value a ValueNode
   @return a new Node

   Creates a new ConditionNode from an Allocator.
 */
Node *ast_cond_node_new(Allocator *alloc, const YYLTYPE *locp, PropertyId prop, CompareType cmp, ValueNode *value);

/**
   @param alloc an Allocator
   @param locp location information
   @param first a Node
   @param op an operator
   @param second a second node
   @return a new Node

   Creates a new ExpressionNode from an Allocator.
 */
Node *ast_expr_node_new(Allocator *alloc, const YYLTYPE *locp, Node *first, OperatorType op, Node *second);

/**
   @param alloc an Allocator
   @param locp location information
   @param name functon name
   @param args first argument of the function
   @return a new Node

   Creates a new FuncNode from an Allocator.
 */
Node *ast_func_node_new(Allocator *alloc, const YYLTYPE *locp, char *name, Node *args);

/**
   @param alloc an Allocator
   @param locp location information
   @param exprs expressions root node
   @param post_exprs post expressions root node
   @return a new RootNode

   Creates a RootNode from an Allocator.
 */
RootNode *ast_root_node_new(Allocator *alloc, const YYLTYPE *locp, Node *exprs, Node *post_exprs);
#endif

