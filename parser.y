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

#define BUFFER_MAX 512

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
		fprintf(stderr, "parsing failed, ");
	}

	if(locp->first_line == locp->last_line)
	{
		fprintf(stderr, "line: %d, ", locp->first_line);
	}
	else
	{
		fprintf(stderr, "line: %d-%d, ", locp->first_line, locp->last_line);
	}

	if(locp->first_column == locp->last_column)
	{
		fprintf(stderr, "column: %d\n", locp->first_column);
	}
	else
	{
		fprintf(stderr, "column: %d-%d\n", locp->first_column, locp->last_column);
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

bool
parse_string(const char *str, TranslationFlags flags, size_t *argc, char ***argv, char **err)
{
	void* scanner;
	RootNode *root = NULL;
	ParserExtra extra;
	YY_BUFFER_STATE buf;
	bool success = false;
	size_t i;

	assert(argc != NULL);
	assert(argv != NULL);
	assert(err != NULL);

	*argc = 0;
	*argv = NULL;
	*err = NULL;

	/* test expression length */
	if(!str)
	{
		*err = strdup("Expression cannot be empty.");
		return false;
	}

	if(strlen(str) > PARSER_MAX_EXPRESSION_LENGTH)
	{
		*err = strdup("Expression length exceeds maximum.");
		return false;
	}

	/* initialize extra data */
	memset(&extra, 0, sizeof(ParserExtra));
	buffer_init(&extra.buffer, BUFFER_MAX);

	extra.alloc = (Allocator *)chunk_allocator_new(_parser_get_alloc_item_size(), 16);
	extra.column = 1;
	extra.lineno = 1;

	slist_init(&extra.strings, &direct_equal, &free, NULL);

	/* setup scanner */
	yylex_init(&scanner);
	yyset_extra(&extra, scanner);

	/* parse string*/
	buf = yy_scan_string(str, scanner);

	if(!yyparse(scanner, (void *)&root))
	{
		/* string parsed successfully => translate generated tree to argument list */
		success = translate(root->exprs, flags, argc, argv, err);

		if(root->post_exprs)
		{
			printf("%d\n", root->post_exprs->type);
		}

		/* reset parsed arguments if translation has failed */
		if(!success && *argv)
		{
			for(i = 0; i < *argc; ++i)
			{
				free((*argv)[i]);
			}

			*argc = 0;
			free(*argv);
			*argv = NULL;
		}
	}

	/* free buffer state & scanner */
	yy_delete_buffer(buf, scanner);
	yylex_destroy(scanner);

	/* cleanup */
	chunk_allocator_destroy((ChunkAllocator *)extra.alloc);
	slist_clear(&extra.strings);
	buffer_free(&extra.buffer);

	return success;
}

#define ALLOC(scanner) ((ParserExtra *)yyget_extra(scanner))->alloc

static char *
_parser_memorize_string(void *scanner, char *str)
{
	ParserExtra *extra = ((ParserExtra *)yyget_extra(scanner));

	slist_append(&extra->strings, str);

	return str;
}
%}

%token TOKEN_LPAREN
%token TOKEN_RPAREN
%token TOKEN_LBRACE
%token TOKEN_RBRACE
%token TOKEN_CMP
%token TOKEN_OPERATOR
%token TOKEN_PROPERTY
%token TOKEN_FLAG
%token TOKEN_COMMA
%token <ivalue>TOKEN_INTERVAL
%token <ivalue> TOKEN_NUMBER
%token <svalue> TOKEN_STRING
%token <svalue> TOKEN_FN_NAME
%token <ivalue> TOKEN_UNIT
%token <ivalue> TOKEN_TYPE

%type <root> query
%type <node> value cond term exprs flag post_exprs post_term fn fn_arg fn_args
%type <ivalue> property number interval unit compare operator

%%
query:
    exprs                                   { *root = ast_root_node_new(ALLOC(scanner), &@1, $1, NULL); }
    | exprs post_exprs                      { *root = ast_root_node_new(ALLOC(scanner), &@1, $1, $2); }
    ;

exprs:
    term operator exprs                     { $$ = ast_expr_node_new(ALLOC(scanner), &@1, $1, $2, $3); }
    | term                                  { $$ = $1; }
    ;

term:
    TOKEN_LPAREN exprs TOKEN_RPAREN         { $$ = $2; }
    | cond                                  { $$ = $1; }
    | flag                                  { $$ = $1; }
    ;

cond:
    property compare value                  { $$ = ast_cond_node_new(ALLOC(scanner), &@1, $1, $2, $3); }
    ;

flag:
    TOKEN_FLAG                              { $$ = ast_value_node_new_flag(ALLOC(scanner), &@1, yylval.ivalue); }
    ;

property:
    TOKEN_PROPERTY                          { $$ = yylval.ivalue; }
    ;

value:
    number                                  { $$ = ast_value_node_new_int(ALLOC(scanner), &@1, $1); }
    | number interval                       { $$ = ast_value_node_new_int_pair(ALLOC(scanner), &@1, VALUE_TIME, $1, $2); }
    | number unit                           { $$ = ast_value_node_new_int_pair(ALLOC(scanner), &@1, VALUE_SIZE, $1, $2); }
    | TOKEN_STRING                          { $$ = ast_value_node_new_str_nodup(ALLOC(scanner), &@1, _parser_memorize_string(scanner, yylval.svalue)); }
    | TOKEN_TYPE                            { $$ = ast_value_node_new_type(ALLOC(scanner), &@1, yylval.ivalue); }
    ;

post_exprs:
    post_term                               { $$ = $1; }
    | post_term operator post_exprs         { $$ = ast_expr_node_new(ALLOC(scanner), &@1, $1, $2, $3); }
    ;

post_term:
    fn TOKEN_LPAREN TOKEN_RPAREN            { $$ = ast_compare_node_new(
                                                     ALLOC(scanner),
                                                     &@1,
                                                     ast_func_node_new(ALLOC(scanner), &@1, _parser_memorize_string(scanner, (char *)$1), NULL),
                                                     CMP_EQ,
                                                     ast_true_node_new(ALLOC(scanner), &@1)); }
    | fn TOKEN_LPAREN fn_args TOKEN_RPAREN  { $$ = ast_compare_node_new(
                                                     ALLOC(scanner), &@1,
                                                     ast_func_node_new(ALLOC(scanner), &@1, _parser_memorize_string(scanner, (char *)$1), $3),
                                                     CMP_EQ,
                                                     ast_true_node_new(ALLOC(scanner), &@1)); }
    | TOKEN_LPAREN post_exprs TOKEN_RPAREN  { $$ = $2; }
    ;

fn:
    TOKEN_FN_NAME                           { $$ = yylval.svalue; }
    ;

fn_args:
     fn_arg
     | fn_arg TOKEN_COMMA fn_args           { $$ = ast_expr_node_new(ALLOC(scanner), &@1, $1, OP_COMMA, $3); }
     ;

fn_arg:
    number                                  { $$ = ast_value_node_new_int(ALLOC(scanner), &@1, yylval.ivalue); }
    | TOKEN_STRING                          { $$ = ast_value_node_new_str_nodup(ALLOC(scanner), &@1, _parser_memorize_string(scanner, yylval.svalue)); }
    | fn TOKEN_LPAREN TOKEN_RPAREN          { $$ = ast_func_node_new(ALLOC(scanner), &@1, _parser_memorize_string(scanner, (char *)$1), NULL); }
    | fn TOKEN_LPAREN fn_args TOKEN_RPAREN  { $$ = ast_func_node_new(ALLOC(scanner), &@1, _parser_memorize_string(scanner, (char *)$1), $3); }
    ;

number:
    TOKEN_NUMBER                            { $$ = yylval.ivalue; }
    ;

interval:
    TOKEN_INTERVAL                          { $$ = yylval.ivalue; }
    ;

unit:
    TOKEN_UNIT                              { $$ = yylval.ivalue; }
    ;

compare:
    TOKEN_CMP                               { $$ = yylval.ivalue; }
    ;

operator:
    TOKEN_OPERATOR                          { $$ = yylval.ivalue; }
    ;
