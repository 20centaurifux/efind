#ifndef TRANSLATE_H
#define TRANSLATE_H

#include <stdbool.h>
#include <stdlib.h>

#include "ast.h"

typedef enum
{
	TRANSLATION_FLAG_NONE  = 0,
	TRANSLATION_FLAG_QUOTE = 1
} TranslationFlags;

bool translate(Node *root, TranslationFlags flags, size_t *argc, char ***argv, char **err);
#endif

