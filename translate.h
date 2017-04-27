/***************************************************************************
    begin........: April 2015
    copyright....: Sebastian Fedrau
    email........: sebastian.fedrau@gmail.com
 ***************************************************************************/

/***************************************************************************
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License v3 as published by
    the Free Software Foundation.

    This program is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License v3 for more details.
 ***************************************************************************/
/**
 * @file translate.h
 * @brief Translate abstract syntax tree to find arguments.
 * @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#ifndef TRANSLATE_H
#define TRANSLATE_H

#include <stdbool.h>
#include <stdlib.h>

#include "ast.h"

/**
   @enum TranslationFlags
   @brief Translation flags.
 */
typedef enum
{
	/*! No flags. */ 
	TRANSLATION_FLAG_NONE  = 0,
	/*! Quote special shell characters. */
	TRANSLATION_FLAG_QUOTE = 1
} TranslationFlags;

/**
   @param root root node
   @param flags translation flags
   @param argc number of translated find arguments
   @param argv vector to store translated find arguments
   @param err location to store error messages
   @return true on success

   Translates an abstract syntax tree to GNU find arguments.
 */
bool translate(Node *root, TranslationFlags flags, size_t *argc, char ***argv, char **err);
#endif

