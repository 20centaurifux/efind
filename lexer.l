%option outfile="lexer.l.c" header-file="lexer.l.h"
%option noyywrap nounput noinput
%option reentrant bison-bridge bison-locations

%{
#include <stdio.h>

#include "ast.h"
#include "parser.h"
#include "parser.y.h"

/*! @cond INTERNAL */
#define YY_USER_ACTION yylloc->first_line = yylloc->last_line; \
                       yylloc->last_line = yyextra->lineno; \
                       yylloc->first_column = yyextra->column; \
                       yylloc->last_column = yyextra->column + yyleng - 1; \
                       yyextra->column += yyleng;
/*! @endcond */

void yyerror(YYLTYPE *locp, void *scanner, void **root, const char *msg);

#define YY_EXTRA_TYPE ParserExtra *

static bool
_write_to_buffer(YYLTYPE *locp, void *scanner, Buffer *buf, const char *str)
{
	bool success = true;

	if(!buffer_fill(buf, str, strlen(str)))
	{
		yyerror(locp, scanner, NULL, "Couldn't parse string.");
		success = false;
	}

	return success;
}

static char *
_memorize_string(SList *pool, char *str)
{
	slist_append(pool, str);

	return str;
}
%}

NUMBER       [-]?[0-9]+
LPAREN       "("
RPAREN       ")"
OPERATOR     or|and
NOT_OPERATOR not
EQ           =|equals|equal
LT           \<|"less than"|less
GT           \>|"greater than"|greater
AT_LEAST     \>=|"at least"
AT_MOST      \<=|"at most"
PROPERTY     name|iname|atime|ctime|mtime|size|group|gid|user|uid|type|regex|iregex|filesystem|fs
FLAG         readable|writable|executable|empty
INTERVAL     hours|hour|h|minutes|minute|m|days|day|d
UNIT         b|bytes|byte|k|kb|kilobytes|kilobyte|M|mb|megabytes|megabyte|G|gb|gigabytes|gigabyte
TYPE         file|dir|directory|block|character|pipe|link|socket
FN_NAME      [a-zA-Z_]++[a-zA-Z0-9_]+
COMMA        ","

%x STRING

%%
\n             yyextra->column = 1; yyextra->lineno++;
[ \r\t]*       /* ignore whitespace */
{NUMBER}       { yylval->ivalue = atoi(yytext); return TOKEN_NUMBER; }
{LPAREN}       { return TOKEN_LPAREN; }
{RPAREN}       { return TOKEN_RPAREN; }
{AT_MOST}      { yylval->ivalue = CMP_LT_EQ; return TOKEN_CMP; }
{AT_LEAST}     { yylval->ivalue = CMP_GT_EQ; return TOKEN_CMP; }
{EQ}           { yylval->ivalue = CMP_EQ; return TOKEN_CMP; }
{GT}           { yylval->ivalue = CMP_GT; return TOKEN_CMP; }
{LT}           { yylval->ivalue = CMP_LT; return TOKEN_CMP; }
{OPERATOR}     { yylval->ivalue = ast_str_to_operator(yytext); return TOKEN_OPERATOR; }
{NOT_OPERATOR} { return TOKEN_NOT_OPERATOR; }
{PROPERTY}     { yylval->ivalue = ast_str_to_property_id(yytext); return TOKEN_PROPERTY; }
{FLAG}         { yylval->ivalue = ast_str_to_flag(yytext); return TOKEN_FLAG; }
{INTERVAL}     { yylval->ivalue = ast_str_to_interval(yytext); return TOKEN_INTERVAL; }
{UNIT}         { yylval->ivalue = ast_str_to_unit(yytext); return TOKEN_UNIT; }
{TYPE}         { yylval->ivalue = ast_str_to_type(yytext); return TOKEN_TYPE; }
{FN_NAME}      { yylval->svalue = _memorize_string(&yyextra->strings, strdup(yytext)); return TOKEN_FN_NAME; }
{COMMA}        { return TOKEN_COMMA; }
\"             { BEGIN STRING; yyextra->quote = '\"'; buffer_clear(&(yyextra->buffer)); }
'              { BEGIN STRING; yyextra->quote = '\''; buffer_clear(&(yyextra->buffer)); }
.              { return 1; }

<STRING>\"|' {
	if(*yytext == yyextra->quote)
	{
		yylval->svalue = _memorize_string(&yyextra->strings, buffer_to_string(&(yyextra->buffer)));

		BEGIN INITIAL;

		return TOKEN_STRING;
	}
	else if(!_write_to_buffer(yylloc, yyscanner, &(yyextra->buffer), yytext))
	{
		return 1;
	}
}
<STRING>\\. { if(!_write_to_buffer(yylloc, yyscanner, &(yyextra->buffer), yytext)) return 1; }
<STRING>.   { if(!_write_to_buffer(yylloc, yyscanner, &(yyextra->buffer), yytext)) return 1; }
%%
