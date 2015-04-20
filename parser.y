%output  "parser.y.c"
%defines "parser.y.h"
%define api.pure

%parse-param {void* scanner}
%parse-param {void **root}
%lex-param   {void* scanner}

%union
{
    int ivalue;
    char *svalue;
    void *node;
}

%{

#include "ast.h"
#include "translate.h"
#include "lexer.l.h"
#include "parser.h"

int
yyparse(void *scanner, void **root);

void
yyerror(void *scanner, void **root, const char *str)
{
	fprintf(stderr, "error: \"%s\"\n", str);
}

bool
parse_string(const char *str, TranslationFlags flags, size_t *argc, char ***argv, char **err)
{
	void* scanner;
	Node *root = NULL;
	ParserExtra extra;
	YY_BUFFER_STATE buf;
	bool success = false;
	size_t i;

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
	buffer_init(&extra.buffer);

	/* setup scanner */
	yylex_init(&scanner);
	yyset_extra(&extra, scanner);

	/* parse string*/
	buf = yy_scan_string(str, scanner);

	if(!yyparse(scanner, (void *)&root))
	{
		/* string parsed successfully => translate generated tree to argument list */
		success = translate(root, flags, argc, argv, err);

		/* reset parsed arguments if translation has failed */
		if(!success && *argv)
		{
			for(i = 0; i < *argc; ++i)
			{
				free(argv[i]);
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
	if(root)
	{
		ast_free(root);
	}

	buffer_free(&extra.buffer);

	return success;
}

bool
parse_string_and_print(FILE *out, FILE *err, const char *str, TranslationFlags flags)
{
	size_t argc;
	char **argv;
	char *errmsg = NULL;
	bool success = false;

	if(parse_string(str, flags, &argc, &argv, &errmsg))
	{
		for(size_t i = 0; i < argc; ++i)
		{
			fprintf(out, "%s ", argv[i]);
			free(argv[i]);
		}

		fprintf(out, "\n");
		free(argv);

		success = true;
	}

	if(errmsg)
	{
		fprintf(err, "%s\n", errmsg);
		free(errmsg);
	}

	return success;
}
%}

%token TOKEN_LPAREN
%token TOKEN_RPAREN
%token TOKEN_LBRACE
%token TOKEN_RBRACE
%token TOKEN_CMP
%token TOKEN_OPERATOR
%token TOKEN_PROPERTY
%token <ivalue>TOKEN_INTERVAL
%token TOKEN_OPTION
%token <ivalue> TOKEN_NUMBER
%token <svalue> TOKEN_STRING
%token <ivalue> TOKEN_UNIT
%token <ivalue> TOKEN_TYPE
%token TOKEN_UNDEFINED

%type <node> value cond term exprs query
%type <ivalue> property number interval unit compare operator

%%
query:
    exprs                           { scanner = NULL; *root = $$; }

exprs:
    term operator exprs             { $$ = ast_expr_node_new($1, $2, $3); }
    | term                          { $$ = ast_expr_node_new($1, OP_UNDEFINED, NULL); }
    ;

term:
    TOKEN_LPAREN exprs TOKEN_RPAREN { $$ = ast_expr_node_new($2, OP_UNDEFINED, NULL); }
    | cond                          { $$ = $1; }
    ;

cond:
    property compare value          { $$ = ast_cond_node_new($1, $2, $3); }
    ;

property:
    TOKEN_PROPERTY                  { $$ = yylval.ivalue; }

value:
    number                          { $$ = ast_value_node_new_int($1); }
    | number interval               { $$ = ast_value_node_new_int_pair(VALUE_TIME, $1, $2); }
    | number unit                   { $$ = ast_value_node_new_int_pair(VALUE_SIZE, $1, $2); }
    | TOKEN_STRING                  { $$ = ast_value_node_new_str(yylval.svalue); }
    | TOKEN_TYPE                    { $$ = ast_value_node_new_type(yylval.ivalue); }

number:
    TOKEN_NUMBER                    { $$ = yylval.ivalue; }
    ;

interval:
    TOKEN_INTERVAL                  { $$ = yylval.ivalue; }

unit:
    TOKEN_UNIT                      { $$ = yylval.ivalue; }

compare:
    TOKEN_CMP                       { $$ = yylval.ivalue; }

operator:
    TOKEN_OPERATOR                  { $$ = yylval.ivalue; }
