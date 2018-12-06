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
   @file exec.c
   @brief Executes shell commands.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>

#include "exec.h"
#include "log.h"
#include "utils.h"
#include "format.h"

/*! @cond INTERNAL */
typedef struct
{
	Processor padding;
	const char *path;
	const ExecArgs *args;
	char **argv;
} ExecProcessor;
/*! @endcond */

static const char *
_exec_processor_read(Processor *processor)
{
	assert(processor != NULL);

	processor->flags &= ~PROCESSOR_FLAGS_READABLE;

	return ((ExecProcessor *)processor)->path;
}

static bool
_exec_build_argv(ExecProcessor *processor)
{
	assert(processor != NULL);
	assert(processor->args != NULL);
	assert(processor->args->path != NULL);
	assert(processor->argv != NULL);

	TRACEF("exec", "Building argument list for command `%s'.", processor->args->path);

	utils_copy_string(processor->args->path, &processor->argv[0]);

	for(size_t i = 0; i < processor->args->argc; ++i)
	{
		TRACEF("exec", "Appending argument: `%s'", processor->args->argv[i]);

		utils_copy_string(processor->args->argv[i], &processor->argv[i + 1]);
	}

	return true;
}

static int
_exec_fork(const char *wd, ExecProcessor *processor)
{
	int exit_code = EXIT_FAILURE;

	assert(processor != NULL);
	assert(wd != NULL);
	assert(processor->args != NULL);

	if(_exec_build_argv(processor))
	{
		DEBUGF("exec", "Argument list built successfully, forking and running `%s'.", processor->args->path);

		pid_t pid = fork();

		if(pid == -1)
		{
			FATALF("exec", "`fork' failed with result %ld.", pid);
			perror("fork()");
		}
		else if(pid == 0)
		{
			TRACEF("exec", "Changing working directory: `%s'", wd);

			if(chdir(wd))
			{
				WARNINGF("exec", "Couldn't change to working directory `%s'.", wd);
			}

			if(execvp(processor->args->path, processor->argv) == -1)
			{
				perror("execl()");
			}
		}
		else
		{
			int rc;
			int status;

			DEBUGF("exec", "Waiting for child process with pid %ld.", pid);

			if((rc = waitpid(pid, &status, 0)) == pid)
			{
				if(WIFEXITED(status))
				{
					exit_code = WEXITSTATUS(status);

					DEBUGF("exec", "Child %ld finished with exit code %d.", pid, exit_code);
				}
			}
			else if(rc == -1)
			{
				ERRORF("exec", "`waitpid' failed, rc=%d.", rc);
				perror("waitpid()");
			}
		}
	}

	return exit_code;
}

static void
_exec_processor_write(Processor *processor, const char *dir, const char *path)
{
	assert(processor != NULL);
	assert(dir != NULL);
	assert(path != NULL);

	ExecProcessor *exec = (ExecProcessor *)processor;

	processor->flags |= PROCESSOR_FLAGS_READABLE;
	exec->path = path;

	_exec_fork(dir, exec);
}

static void
_exec_processor_close(Processor *processor)
{
	assert(processor != NULL);

	processor->flags |= PROCESSOR_FLAGS_CLOSED;
}

static void
_exec_processor_free(Processor *processor)
{
	assert(processor != NULL);

	ExecProcessor *exec = (ExecProcessor *)processor;

	if(exec->argv)
	{
		for(size_t i = 0; i <= exec->args->argc; ++i)
		{
			free(exec->argv[i]);
		}

		free(exec->argv);
	}
}

Processor *
exec_processor_new(const ExecArgs *args)
{
	Processor *processor = NULL;

	assert(args != NULL);

	if(args->argc < SIZE_MAX - 2)
	{
		processor = (Processor *)utils_malloc(sizeof(ExecProcessor));

		memset(processor, 0, sizeof(ExecProcessor));

		processor->read = _exec_processor_read;
		processor->write = _exec_processor_write;
		processor->close = _exec_processor_close;
		processor->free = _exec_processor_free;

		ExecProcessor *exec = (ExecProcessor *)processor;

		exec->args = args;
		exec->argv = utils_new(args->argc + 2, char *);
	}
	else
	{
		WARNING("exec", "Integer overflow.");
	}

	return processor;
}

