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
   @file exec.h
   @brief Executes shell commands.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */

#ifndef EXEC_H
#define EXEC_H

#include <stdlib.h>
#include <stdint.h>

#include "processor.h"
#include "exec-args.h"

/**
   @enum TranslationFlags
   @brief Translation flags.
 */
typedef enum
{
	/*! No flags. */
	EXEC_FLAG_NONE  = 0,
	/*! Don't stop if command exits with non-zero value. */
	EXEC_FLAG_IGNORE_ERROR = 1
} ExecFlags;

/**
   @param args command & arguments to execute
   @param flags execution flags
   @return a new Processor

   Excecutes shell commands.
 */
Processor *exec_processor_new(const ExecArgs *args, int32_t flags);

#endif

