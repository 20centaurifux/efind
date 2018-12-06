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
   @file exec-args.c
   @brief List of --exec arguments.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */

#include <assert.h>
#include <stdint.h>

#include "exec-args.h"
#include "utils.h"
#include "log.h"

ExecArgs *
exec_args_new(void)
{
	ExecArgs *args = utils_new(1, ExecArgs);

	args->size = 8;
	args->argv = utils_new(args->size, char *);

	return args;
}

void
exec_args_destroy(ExecArgs *args)
{
	assert(args != NULL);

	if(args)
	{
		if(args->path)
		{
			free(args->path);
		}

		if(args->argc)
		{
			for(size_t i = 0; i < args->argc; ++i)
			{
				free(args->argv[i]);
			}
		}

		free(args->argv);
		free(args);
	}
}

static bool
_exec_args_resize_if_necessary(ExecArgs *args)
{
	bool success = true;

	assert(args != NULL);

	if(args->argc == args->size)
	{
		if(args->size > SIZE_MAX / 2)
		{
			ERROR("misc", "Integer overflow.");
			success = false;
		}
		else
		{
			args->size *= 2;
			args->argv = (char **)realloc(args->argv, sizeof(char **) * args->size);
		}
	}

	return success;
}

static void
_exec_args_append_arg(ExecArgs *args, const char *arg)
{
	assert(args != NULL);
	assert(arg != NULL);

	if(_exec_args_resize_if_necessary(args))
	{
		args->argv[args->argc] = utils_strdup(arg);
		++args->argc;
	}
}

void
exec_args_append(ExecArgs *args, const char *arg)
{
	assert(args != NULL);
	assert(arg != NULL);
	
	if(args->path)
	{
		_exec_args_append_arg(args, arg);
	}
	else
	{
		args->path = utils_strdup(arg);
	}
}

