#ifndef PARSER_EXTRA_H
#define PARSER_EXTRA_H

#include <stdio.h>
#include <stdint.h>

#include "buffer.h"
#include "translate.h"

#define PARSER_MAX_EXPRESSION_LENGTH 512

typedef struct
{
	Buffer buffer;
} ParserExtra;

bool parse_string(const char *str, TranslationFlags flags, size_t *argc, char ***argv, char **err);
bool parse_string_and_print(FILE *out, FILE *err, const char *str, TranslationFlags flags);
#endif


