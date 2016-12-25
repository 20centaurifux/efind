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
%}

NUMBER			[0-9]+
LPAREN			"("
RPAREN			")"
OPERATOR		or|and
PROPERTY		name|iname|atime|ctime|mtime|size|group|gid|user|uid|type
FLAG			readable|writable|executable
INTERVAL		hours|hour|h|minutes|minute|m|days|day|d
UNIT			b|bytes|byte|k|kb|kilobytes|kilobyte|M|mb|megabytes|megabyte|G|gb|gigabytes|gigabyte
TYPE			file|dir|directory|block|character|pipe|link|socket
FN_NAME			[a-zA-Z_]++[a-zA-Z0-9_]+
COMMA			","

%x STRING

%%
\n			yyextra->column = 1; yyextra->lineno++;
[ \r\t]*		/* ignore whitespace */
{NUMBER}		{ yylval->ivalue = atoi(yytext); return TOKEN_NUMBER; }
{LPAREN}		{ return TOKEN_LPAREN; }
{RPAREN}		{ return TOKEN_RPAREN; }
\<=			{ yylval->ivalue = CMP_LT_EQ; return TOKEN_CMP; }
\>=			{ yylval->ivalue = CMP_GT_EQ; return TOKEN_CMP; }
=			{ yylval->ivalue = CMP_EQ; return TOKEN_CMP; }
\>			{ yylval->ivalue = CMP_GT; return TOKEN_CMP; }
\<			{ yylval->ivalue = CMP_LT; return TOKEN_CMP; }
{OPERATOR}		{ yylval->ivalue = ast_str_to_operator(yytext); return TOKEN_OPERATOR; }
{PROPERTY}		{ yylval->ivalue = ast_str_to_property_id(yytext); return TOKEN_PROPERTY; }
{FLAG}			{ yylval->ivalue = ast_str_to_flag(yytext); return TOKEN_FLAG; }
{INTERVAL}		{ yylval->ivalue = ast_str_to_interval(yytext); return TOKEN_INTERVAL; }
{UNIT}			{ yylval->ivalue = ast_str_to_unit(yytext); return TOKEN_UNIT; }
{TYPE}			{ yylval->ivalue = ast_str_to_type(yytext); return TOKEN_TYPE; }
{FN_NAME}		{ yylval->svalue = strdup(yytext); return TOKEN_FN_NAME; }
{COMMA}			{ return TOKEN_COMMA; }
\"                      { BEGIN STRING; buffer_clear(&(yyextra->buffer)); }
.			{ return 1; }

<STRING>\" {
	if(buffer_is_valid(&(yyextra->buffer)))
	{
		yylval->svalue = buffer_to_string(&(yyextra->buffer)); BEGIN INITIAL; return TOKEN_STRING;
	}
	else
	{
		yyerror(yylloc, yyscanner, NULL, "Couldn't parse string.");

		return 0;
	}
}
<STRING>\\.		{ if(!_write_to_buffer(yylloc, yyscanner, &(yyextra->buffer), yytext)) return 1; }
<STRING>.		{ if(!_write_to_buffer(yylloc, yyscanner, &(yyextra->buffer), yytext)) return 1; }
%%
