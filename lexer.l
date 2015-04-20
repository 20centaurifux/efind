%option outfile="lexer.l.c" header-file="lexer.l.h"
%option noyywrap nounput noinput
%option reentrant bison-bridge

%{
#include <stdio.h>

#include "ast.h"
#include "buffer.h"
#include "parser.h"
#include "parser.y.h"

#define YY_EXTRA_TYPE ParserExtra *
%}

NUMBER			[0-9]+
LPAREN			"("
RPAREN			")"
OPERATOR		or|and
PROPERTY		name|iname|atime|ctime|mtime|size|group|gid|user|uid|type
OPTION			follow|readable|writable
INTERVAL		hours|hour|h|minutes|minute|m|days|day|d
UNIT			b|byte|bytes|k|kb|kilobyte|M|mb|megabyte|megabytes|G|gb|gigabyte
TYPE			file|dir|directory|block|character|pipe|link|socket

%x STRING

%%
[ \r\t\n]*		/* ignore whitespace */
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
{INTERVAL}		{ yylval->ivalue = ast_str_to_interval(yytext); return TOKEN_INTERVAL; }
{UNIT}			{ yylval->ivalue = ast_str_to_unit(yytext); return TOKEN_UNIT; }
{TYPE}			{ yylval->ivalue = ast_str_to_type(yytext); return TOKEN_TYPE; }
{OPTION}		{ return TOKEN_OPTION; }
\"                      { BEGIN STRING; buffer_clear(&(yyextra->buffer)); }
.                       { return TOKEN_UNDEFINED; }

<STRING>\"		{ yylval->svalue = buffer_to_string(&(yyextra->buffer)); BEGIN INITIAL; return TOKEN_STRING; }
<STRING>\\.		{ buffer_fill(&(yyextra->buffer), yytext, strlen(yytext)); }
<STRING>.		{ buffer_fill(&(yyextra->buffer), yytext, strlen(yytext)); }
%%
