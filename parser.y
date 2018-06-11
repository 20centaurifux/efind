%output  "parser.y.c"
%defines "parser.y.h"
%define api.pure full
%locations

%parse-param {void* scanner}
%parse-param {void **root}
%lex-param   {void* scanner}

%union
{
    int ivalue;
    char *svalue;
    void *node;
    void *root;
}

%{
#include <assert.h>

#include "ast.h"
#include "translate.h"
#include "lexer.l.h"
#include "parser.h"
#include "search.h"
#include "utils.h"
#include "gettext.h"

int
yyparse(void *scanner, void **root);

void
yyerror(YYLTYPE *locp, void *scanner, void **root, const char *str)
{
	assert(locp != NULL);

	if(str)
	{
		fprintf(stderr, "%s, ", str);
	}
	else
	{
		fprintf(stderr, _("parsing failed, "));
	}

	if(locp->first_line == locp->last_line)
	{
		fprintf(stderr, _("line: %d, "), locp->first_line);
	}
	else
	{
		fprintf(stderr, _("line: %d-%d, "), locp->first_line, locp->last_line);
	}

	if(locp->first_column == locp->last_column)
	{
		fprintf(stderr, _("column: %d\n"), locp->first_column);
	}
	else
	{
		fprintf(stderr, _("column: %d-%d\n"), locp->first_column, locp->last_column);
	}
}

static size_t
_parser_get_alloc_item_size(void)
{
	size_t size = sizeof(ExpressionNode);

	if(sizeof(RootNode) > size)
	{
		size = sizeof(RootNode);
	}

	if(sizeof(ConditionNode) > size)
	{
		size = sizeof(ConditionNode);
	}

	if(sizeof(ValueNode) > size)
	{
		size = sizeof(ValueNode);
	}

	if(sizeof(ExpressionNode) > size)
	{
		size = sizeof(ExpressionNode);
	}

	if(sizeof(FuncNode) > size)
	{
		size = sizeof(FuncNode);
	}

	if(sizeof(CompareNode) > size)
	{
		size = sizeof(CompareNode);
	}

	return size;
}

ParserResult *
parse_string(const char *str)
{
	void* scanner;
	ParserResult *result;
	YY_BUFFER_STATE buf;

	/* create and initialize parser result */
	result = (ParserResult *)utils_malloc(sizeof(ParserResult));
	memset(result, 0, sizeof(ParserResult));

	buffer_init(&result->data.buffer, PARSER_MAX_EXPRESSION_LENGTH);
	slist_init(&result->data.strings, &direct_compare, &free, NULL);

	result->data.pool = (Pool *)memory_pool_new(_parser_get_alloc_item_size(), 16);
	result->data.column = 1;
	result->data.lineno = 1;

	/* test expression length */
	if(!str)
	{
		result->err = strdup(_("Expression cannot be empty."));
		return result;
	}

	if(strlen(str) > PARSER_MAX_EXPRESSION_LENGTH)
	{
		result->err = strdup(_("Expression length exceeds maximum allowed length."));
		return result;
	}

	/* setup scanner */
	yylex_init(&scanner);
	yyset_extra(&result->data, scanner);

	/* parse string*/
	buf = yy_scan_string(str, scanner);

	if(!yyparse(scanner, (void *)&result->root))
	{
		result->success = true;
	}

	/* free buffer state & scanner */
	yy_delete_buffer(buf, scanner);
	yylex_destroy(scanner);

	return result;
}

void parser_result_free(ParserResult *result)
{
	if(result)
	{
		free(result->err);
		memory_pool_destroy((MemoryPool *)result->data.pool);
		slist_clear(&result->data.strings);
		buffer_free(&result->data.buffer);
		free(result);
	}
}

#define POOL(scanner) ((ParserExtra *)yyget_extra(scanner))->pool
%}

%token TOKEN_LPAREN
%token TOKEN_RPAREN
%token TOKEN_LBRACE
%token TOKEN_RBRACE
%token TOKEN_CMP
%token TOKEN_OPERATOR
%token TOKEN_NOT_OPERATOR
%token TOKEN_PROPERTY
%token TOKEN_FLAG
%token TOKEN_COMMA
%token <ivalue>TOKEN_INTERVAL
%token <ivalue> TOKEN_NUMBER
%token <svalue> TOKEN_STRING
%token <svalue> TOKEN_FN_NAME
%token <ivalue> TOKEN_UNIT
%token <ivalue> TOKEN_TYPE

%left TOKEN_OPERATOR

%type <root> query
%type <node> value cond term exprs flag post_exprs post_expr post_term fn_name fn_call fn_arg fn_args
%type <ivalue> property number interval unit compare operator

%%
query:
    exprs                                       { *root = ast_root_node_new(POOL(scanner), &@1, $1, NULL); }
    | post_exprs                                { *root = ast_root_node_new(POOL(scanner), &@1, NULL, $1); }
    | exprs operator post_exprs                 { if($2 == OP_AND)
                                                  {
                                                    *root = ast_root_node_new(POOL(scanner), &@1, $1, $3);
                                                  }
                                                  else
                                                  {
                                                    yyerror(&@1, scanner, NULL, _("Please use custom functions after the \"and\" operator"));
                                                    YYABORT;
                                                  }
                                                }
    ;

exprs:
    exprs operator term                         { $$ = ast_expr_node_new(POOL(scanner), &@1, $1, $2, $3); }
    | term                                      { $$ = $1; }
    ;

term:
    TOKEN_LPAREN exprs TOKEN_RPAREN             { $$ = $2; }
    | cond                                      { $$ = $1; }
    | flag                                      { $$ = $1; }
    | TOKEN_NOT_OPERATOR term                   { $$ = ast_not_node_new(POOL(scanner), &@1, $2); }
    ;

cond:
    property compare value                      { $$ = ast_cond_node_new(POOL(scanner), &@1, $1, $2, $3); }
    ;

flag:
    TOKEN_FLAG                                  { $$ = ast_value_node_new_flag(POOL(scanner), &@1, yylval.ivalue); }
    ;

property:
    TOKEN_PROPERTY                              { $$ = yylval.ivalue; }
    ;

value:
    number                                      { $$ = ast_value_node_new_int(POOL(scanner), &@1, $1); }
    | number interval                           { $$ = ast_value_node_new_int_pair(POOL(scanner), &@1, VALUE_TIME, $1, $2); }
    | number unit                               { $$ = ast_value_node_new_int_pair(POOL(scanner), &@1, VALUE_SIZE, $1, $2); }
    | TOKEN_STRING                              { $$ = ast_value_node_new_str_nodup(POOL(scanner), &@1, yylval.svalue); }
    | TOKEN_TYPE                                { $$ = ast_value_node_new_type(POOL(scanner), &@1, yylval.ivalue); }
    ;

post_exprs:
    post_expr                                   { $$ = $1; }
    | post_expr operator post_exprs             { $$ = ast_expr_node_new(POOL(scanner), &@1, $1, $2, $3); }
    ;

post_expr:
    post_term                                   { $$ = ast_compare_node_new(POOL(scanner), &@1, $1, CMP_EQ, ast_true_node_new(POOL(scanner), &@1)); }
    | post_term compare post_term               { $$ = ast_compare_node_new(POOL(scanner), &@1, $1, $2, $3); }
    | TOKEN_LPAREN post_exprs TOKEN_RPAREN      { $$ = $2; }
    | TOKEN_NOT_OPERATOR post_expr              { $$ = ast_not_node_new(POOL(scanner), &@1, $2); }
    ;

post_term:
    fn_call                                     { $$ = $1; }
    | number                                    { $$ = ast_value_node_new_int(POOL(scanner), &@1, yylval.ivalue); }
    ;

fn_name:
    TOKEN_FN_NAME                               { $$ = yylval.svalue; }
    ;

fn_call:
    fn_name TOKEN_LPAREN TOKEN_RPAREN           { $$ = ast_func_node_new(POOL(scanner), &@1, $1, NULL); }
    | fn_name TOKEN_LPAREN fn_args TOKEN_RPAREN { $$ = ast_func_node_new(POOL(scanner), &@1, $1, $3); }


fn_args:
     fn_arg
     | fn_arg TOKEN_COMMA fn_args               { $$ = ast_expr_node_new(POOL(scanner), &@1, $1, OP_COMMA, $3); }
     ;

fn_arg:
    number                                      { $$ = ast_value_node_new_int(POOL(scanner), &@1, yylval.ivalue); }
    | TOKEN_STRING                              { $$ = ast_value_node_new_str_nodup(POOL(scanner), &@1, yylval.svalue); }
    | fn_call                                   { $$ = $1; }
    ;

number:
    TOKEN_NUMBER                                { $$ = yylval.ivalue; }
    ;

interval:
    TOKEN_INTERVAL                              { $$ = yylval.ivalue; }
    ;

unit:
    TOKEN_UNIT                                  { $$ = yylval.ivalue; }
    ;

compare:
    TOKEN_CMP                                   { $$ = yylval.ivalue; }
    ;

operator:
    TOKEN_OPERATOR                              { $$ = yylval.ivalue; }
    ;
