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
   @file exec-args.h
   @brief List of --exec arguments.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */

#ifndef EXEC_ARGS_H
#define EXEC_ARGS_H

#include <stddef.h>

/**
   @struct ExecArgs
   @brief List of --exec arguments.
 */
typedef struct
{
	/*! File to execute. */
	char *path;
	/*! Argument list. */
	char **argv;
	/*! Number of arguments. */
	size_t argc;
	/*! Size of the argv array. */
	size_t size;
} ExecArgs;

/**
   @return an initialized ExecArgs instance

   Creates a new ExecArgs instance.
 */
ExecArgs *exec_args_new(void);

/**
   @param args ExecArgs instance to destroy

   Frees an ExecArgs instance.
 */
void exec_args_destroy(ExecArgs *args);

/**
   @param args ExecArgs an ExecArgs instance
   @param arg argument to append to the list

   Adds an argument to the ExecArgs list. The first element is the path.
 */
void exec_args_append(ExecArgs *args, const char *arg);

#endif

