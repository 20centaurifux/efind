#ifndef AST_H
#define AST_H

typedef enum
{
	PROP_UNDEFINED,
	PROP_NAME,
	PROP_INAME,
	PROP_ATIME,
	PROP_CTIME,
	PROP_MTIME,
	PROP_SIZE,
	PROP_GROUP,
	PROP_GROUP_ID,
	PROP_USER,
	PROP_USER_ID,
	PROP_TYPE
} PropertyId;

typedef enum
{
	CMP_UNDEFINED,
	CMP_EQ,
	CMP_LT_EQ,
	CMP_LT,
	CMP_GT_EQ,
	CMP_GT
} CompareType;

typedef enum
{
	OP_UNDEFINED,
	OP_AND,
	OP_OR
} OperatorType;

typedef enum
{
	UNIT_UNDEFINED,
	UNIT_BYTES,
	UNIT_KB,
	UNIT_MB,
	UNIT_G
} UnitType;

typedef enum
{
	FILE_UNDEFINED,
	FILE_REGULAR,
	FILE_DIRECTORY,
	FILE_PIPE,
	FILE_SOCKET,
	FILE_BLOCK,
	FILE_CHARACTER,
	FILE_SYMLINK
} FileType;

typedef enum
{
	NODE_UNDEFINED,
	NODE_EXPRESSION,
	NODE_CONDITION,
	NODE_VALUE
} NodeType;

typedef struct
{
	NodeType type;
} Node;

typedef enum
{
	VALUE_UNDEFINED,
	VALUE_NUMERIC,
	VALUE_STRING,
	VALUE_TIME,
	VALUE_SIZE,
	VALUE_TYPE
} ValueType;

typedef enum
{
	TIME_UNDEFINED,
	TIME_MINUTES,
	TIME_HOURS,
	TIME_DAYS
} TimeInterval;

typedef struct
{
	Node padding;
	ValueType vtype;
	union
	{
		char *svalue;
		int ivalue;
		struct
		{
			int a;
			int b;
		} pair;
	} value;
} ValueNode;

typedef struct
{
	Node padding;
	PropertyId prop;
	CompareType cmp;
	ValueNode *value;
} ConditionNode;

typedef struct
{
	Node padding;
	Node *first;
	OperatorType op;
	Node *second;
} ExpressionNode;

TimeInterval ast_str_to_interval(const char *id);

PropertyId ast_str_to_property_id(const char *str);

OperatorType ast_str_to_operator(const char *str);

UnitType ast_str_to_unit(const char *str);

FileType ast_str_to_type(const char *str);

Node *ast_value_node_new_str(const char *value);

Node *ast_value_node_new_int(int value);

Node *ast_value_node_new_type(FileType type);

Node *ast_value_node_new_int_pair(ValueType type, int a, int b);

Node *ast_cond_node_new(PropertyId prop, CompareType cmp, ValueNode *value);

Node *ast_expr_node_new(Node *first, OperatorType op, Node *second);

void ast_free(Node *node);

#endif

