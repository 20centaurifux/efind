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
   @brief Execute shell commands.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>

#include "exec.h"
#include "log.h"
#include "utils.h"
#include "format-parser.h"
#include "format.h"
#include "gettext.h"

/*! @cond INTERNAL */
typedef struct
{
	Processor padding;
	int32_t flags;
	const char *dir;
	const char *path;
	const ExecArgs *args;
	FormatParserResult **formats;
	char **argv;
	FILE *fp;
	char *buffer;
	size_t buffer_len;
} ExecProcessor;
/*! @endcond */

static const char *
_exec_processor_read(Processor *processor)
{
	assert(processor != NULL);

	processor->flags &= ~PROCESSOR_FLAG_READABLE;

	return ((ExecProcessor *)processor)->path;
}

static bool
_exec_build_argv(ExecProcessor *processor)
{
	bool success = true;

	assert(processor != NULL);
	assert(processor->args != NULL);
	assert(processor->args->path != NULL);
	assert(processor->argv != NULL);

	TRACEF("exec", "Building argument list for command `%s'.", processor->args->path);

	utils_copy_string(processor->args->path, &processor->argv[0]);

	for(size_t i = 0; i < processor->args->argc && success; ++i)
	{
		TRACEF("exec", "Appending argument: `%s'", processor->args->argv[i]);

		if(fseek(processor->fp, 0, SEEK_SET))
		{
			success = false;

			ERROR("exec", "fseek() failed.");
			perror("fflush()");
		}
		else
		{
			success = format_write(processor->formats[i], processor->dir, processor->path, processor->fp);
		}

		if(success)
		{
			success = !fflush(processor->fp);

			if(success)
			{
				long int pos = ftell(processor->fp);

				if(pos != -1)
				{
					processor->buffer[pos] = '\0';
					utils_copy_string(processor->buffer, &processor->argv[i + 1]);
				}
				else
				{
					ERROR("exec", "ftell() failed.");
					perror("ftell()");
				}
			}
			else
			{
				ERROR("exec", "fflush() failed.");
				perror("fflush()");
			}
		}
		else
		{
			fprintf(stderr, _("Couldn't write format string at position %ld.\n"), i + 2);
		}
	}

	return success;
}

static int
_exec_fork(ExecProcessor *processor)
{
	int exit_code = EXIT_FAILURE;

	assert(processor != NULL);
	assert(processor->args != NULL);

	if(_exec_build_argv(processor))
	{
		DEBUGF("exec", "Argument list built successfully, forking and running `%s'.", processor->args->path);

		pid_t pid = fork();

		if(pid == -1)
		{
			FATALF("exec", "`fork' failed with result %ld.", pid);
			perror(processor->args->path);
		}
		else if(pid == 0)
		{
			if(execvp(processor->args->path, processor->argv) == -1)
			{
				perror(processor->args->path);
			}

			exit(EXIT_FAILURE);
		}
		else
		{
			DEBUGF("exec", "Waiting for child process with pid %ld.", pid);

			int rc;
			int status;

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

	processor->flags |= PROCESSOR_FLAG_READABLE;
	exec->dir = dir;
	exec->path = path;

	if(_exec_fork(exec) && !(exec->flags & EXEC_FLAG_IGNORE_ERROR))
	{
		processor->flags &= ~PROCESSOR_FLAG_READABLE;
		processor->flags |= PROCESSOR_FLAG_CLOSED | PROCESSOR_FLAG_ERROR;
	}
}

static void
_exec_free_argv(Processor *processor)
{
	assert(processor != NULL);

	ExecProcessor *exec = (ExecProcessor *)processor;

	if(exec->argv)
	{
		bool finished = false;

		for(size_t i = 0; i <= exec->args->argc && !finished; ++i)
		{
			if(exec->argv[i])
			{
				free(exec->argv[i]);
			}
			else
			{
				finished = true;
			}
		}

		free(exec->argv);
	}
}

static void
_exec_free_formats(Processor *processor)
{
	assert(processor != NULL);

	ExecProcessor *exec = (ExecProcessor *)processor;

	if(exec->formats)
	{
		bool finished = false;

		for(size_t i = 0; i < exec->args->argc && !finished; ++i)
		{
			if(exec->formats[i])
			{
				format_parser_result_free(exec->formats[i]);
			}
			else
			{
				finished = true;
			}
		}

		free(exec->formats);;
	}
}

static void
_exec_processor_free(Processor *processor)
{
	assert(processor != NULL);

	_exec_free_argv(processor);
	_exec_free_formats(processor);

	ExecProcessor *exec = (ExecProcessor *)processor;

	if(exec->fp)
	{
		if(fclose(exec->fp))
		{
			WARNING("exec", "fclose() failed.");
		}

		if(exec->buffer)
		{
			free(exec->buffer);
		}
	}
}

static bool
_exec_processor_parse_args(const ExecArgs *args, FormatParserResult ***formats)
{
	bool success = true;

	assert(args != NULL);
	assert(formats != NULL);

	*formats = NULL;

	if(args->argc)
	{
		size_t tail = 0;

		*formats = utils_new(args->argc, FormatParserResult *);

		for(size_t i = 0; i < args->argc && success; ++i)
		{
			(*formats)[i] = format_parse(args->argv[i]);
			success = (*formats)[i]->success;
			++tail;
		}

		if(!success)
		{
			fprintf(stderr, _("Couldn't parse --exec argument at position %ld.\n"), tail + 1);

			for(size_t i = 0; i < tail; ++i)
			{
				format_parser_result_free((*formats)[i]);
			}

			free(*formats);
			*formats = NULL;
		}
	}

	return success;
}

Processor *
exec_processor_new(const ExecArgs *args, int32_t flags)
{
	Processor *processor = NULL;

	assert(args != NULL);

	if(args->argc < SIZE_MAX - 2)
	{
		FormatParserResult **formats = NULL;

		if(_exec_processor_parse_args(args, &formats))
		{
			processor = (Processor *)utils_malloc(sizeof(ExecProcessor));

			memset(processor, 0, sizeof(ExecProcessor));

			processor->read = _exec_processor_read;
			processor->write = _exec_processor_write;
			processor->free = _exec_processor_free;

			ExecProcessor *exec = (ExecProcessor *)processor;

			exec->flags = flags;
			exec->args = args;
			exec->argv = utils_new(args->argc + 2, char *); // path + arguments + NULL
			exec->formats = formats;

			exec->fp = open_memstream(&exec->buffer, &exec->buffer_len);

			if(!exec->fp)
			{
				ERROR("exec", "open_memstream() failed.");
				perror("open_memstream()");

				_exec_processor_free(processor);
				free(processor);

				processor = NULL;
			}
		}
	}
	else
	{
		WARNING("exec", "Integer overflow.");
		fprintf(stderr, _("Couldn't allocate memory.\n"));
	}

	return processor;
}

