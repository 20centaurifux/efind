#ifndef SEARCH_H
#define SEARCH_H

#include <stdbool.h>
#include <stdint.h>

#include "translate.h"

typedef void (*Callback)(const char *str, void *user_data);

int search_files_expr(const char *path, const char *expr, TranslationFlags flags, Callback found_file, Callback err_message, void *user_data);

#endif
