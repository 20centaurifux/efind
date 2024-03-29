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
   @file search.c
   @brief GNU find wrapper.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
/*! @cond INTERNAL */
#define _GNU_SOURCE
/*! @endcond */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <datatypes.h>

#include "search.h"
#include "log.h"
#include "parser.h"
#include "utils.h"
#include "eval.h"
#include "gettext.h"

/*! @cond INTERNAL */
typedef EvalResult (*Filter)(const char *filename, void *user_data);

typedef struct
{
	ParserResult *result;
	ExtensionManager *extensions;
} FilterArgs;

typedef struct
{
	pid_t child_pid;
	int outfd;
	int errfd;
	FilterArgs filter_args;
	Callback found_file;
	Callback err_message;
	void *user_data;
} ParentCtx;

typedef struct
{
	Buffer *buffer;
	char *line;
	int32_t count;
	size_t llen;
	bool filter;
	FilterArgs *filter_args;
	Callback cb;
	void *user_data;
} ReaderArgs;

const int PROCESS_STATUS_OK       = 0;
const int PROCESS_STATUS_ERROR    = 1;
const int PROCESS_STATUS_FINISHED = 2;
const int PROCESS_STATUS_STOP     = 3;
/*! @endcond */

void
search_options_free(SearchOptions *opts)
{
	assert(opts != NULL);

	if(opts->regex_type)
	{
		free(opts->regex_type);
	}
}

static void
_search_merge_options(size_t *argc, char ***argv, const char *path, const SearchOptions *opts)
{
	assert(argc != NULL);
	assert(argv != NULL);
	assert(path != NULL);
	assert(opts != NULL);

	char **nargv;
	size_t maxsize;
	size_t index = 0;

	/* initialize argument vector */
	maxsize = (*argc) + 8; /* "find" + path + *argv + options + NULL */

	nargv = utils_new(maxsize, char *);

	/* first argument (executable name) */
	nargv[index++] = utils_strdup("find");

	/* follow symbolic links? */
	if(opts && opts->follow)
	{
		nargv[index++] = utils_strdup("-L");
	}

	/* copy search path */
	nargv[index++] = utils_strdup(path);

	/* regex type */
	if(opts && opts->regex_type)
	{
		nargv[index++] = utils_strdup("-regextype");
		nargv[index++] = utils_strdup(opts->regex_type);
	}

	/* copy translated find arguments */
	for(size_t i = 0; i < *argc; i++)
	{
		nargv[index++] = (*argv)[i];
	}

	/* maximum search depth */
	if(opts && opts->max_depth >= 0)
	{
		char buffer[32];

		snprintf(buffer, sizeof(buffer), "%d", opts->max_depth);

		nargv[index++] = utils_strdup("-maxdepth");
		nargv[index++] = utils_strdup(buffer);
	}

	*argc = index;

	free(*argv);
	*argv = nargv;
}

static ParserResult *
_search_translate_expr(const char *path, const char *expr, TranslationFlags flags, const SearchOptions *opts, size_t *argc, char ***argv)
{
	assert(path != NULL);
	assert(expr != NULL);
	assert(opts != NULL);
	assert(argc != NULL);
	assert(argv != NULL);

	*argc = 0;
	*argv = NULL;

	ParserResult *result = parse_string(expr);

	if(result->success)
	{
		char *err = NULL;

		if(translate(result->root->exprs, flags, argc, argv, &err))
		{
			_search_merge_options(argc, argv, path, opts);
		}
		else
		{
			result->success = false;
			result->err = err;
		}
	}

	if(!result->success && *argv)
	{
		for(size_t i = 0; i < *argc; i++)
		{
			free((*argv)[i]);
		}

		*argc = 0;
		free(*argv);
		*argv = NULL;
	}

	return result;
}

static void
_search_child_process(char **argv)
{
	assert(argv != NULL);

	char *exe;

	if((exe = utils_whereis("find")))
	{
		if(execv(exe, argv) == -1)
		{
			perror("execl()");
			exit(EXIT_FAILURE);
		}
	}
	else
	{
		fprintf(stderr, _("Couldn't find `find' executable.\n"));
	}
}

static EvalResult
_search_filter(const char *filename, void *user_data)
{
	EvalResult result = EVAL_RESULT_TRUE;

	assert(filename != NULL);
	assert(user_data != NULL);

	FilterArgs *args = (FilterArgs *)user_data;

	if(args->result->root->filter_exprs)
	{
		TRACEF("search", "Filtering file: %s", filename);

		if(args->extensions)
		{
			result = evaluate(args->extensions, args->result->root->filter_exprs, filename);

			if(result == EVAL_RESULT_ABORTED)
			{
				fprintf(stderr, _("Evaluation aborted.\n"));
			}
		}
		else
		{
			fprintf(stderr, _("Couldn't evaluate expression, no extensions loaded.\n"));
			result = EVAL_RESULT_ABORTED;
		}
	}

	return result;
}

static int
_search_process_line(ReaderArgs *args)
{
	int status = PROCESS_STATUS_OK;

	assert(args != NULL);
	assert(args->line != NULL);

	if(args->filter)
	{
		EvalResult result = _search_filter(args->line, args->filter_args);

		if(result == EVAL_RESULT_TRUE && args->cb)
		{
			if(args->cb(args->line, args->user_data))
			{
				status = PROCESS_STATUS_STOP;
			}
		}
		else if(result == EVAL_RESULT_ABORTED)
		{
			status = PROCESS_STATUS_ERROR;
		}
	}
	else if(args->cb)
	{
		if(args->cb(args->line, args->user_data))
		{
			status = PROCESS_STATUS_STOP;
		}
	}

	return status;
}

static int
_search_process_lines_from_buffer(ReaderArgs *args)
{
	int status = PROCESS_STATUS_OK;

	assert(args != NULL);
	assert(args->buffer != NULL);

	args->count = 0;

	while(status == PROCESS_STATUS_OK && buffer_read_line(args->buffer, &args->line, &args->llen))
	{
		status = _search_process_line(args);

		if(status == PROCESS_STATUS_OK)
		{
			if(args->count < INT32_MAX)
			{
				++args->count;
			}
		}
	}

	return status;
}

static int
_search_flush_and_process_buffer(ReaderArgs *args)
{
	int status = PROCESS_STATUS_OK;

	assert(args != NULL);
	assert(args->buffer != NULL);

	args->count = 0;

	if(args->cb && !buffer_is_empty(args->buffer))
	{
		status = _search_process_lines_from_buffer(args);

		if(status == PROCESS_STATUS_OK && buffer_flush(args->buffer, &args->line, &args->llen))
		{
			if(_search_process_line(args) == PROCESS_STATUS_OK)
			{
				if(args->count < INT32_MAX)
				{
					++args->count;
				}
			}
		}
	}

	return status;
}

static int
_search_wait_for_child(ParentCtx *ctx, int status)
{
	assert(ctx != NULL);
	assert(ctx->child_pid > 0);

	DEBUGF("search", "Waiting for child process with pid %ld.", ctx->child_pid);

	int child_status;
	int rc;

	if((rc = waitpid(ctx->child_pid, &child_status, 0)) == ctx->child_pid)
	{
		if(WIFEXITED(child_status) && status == PROCESS_STATUS_OK)
		{
			if(child_status)
			{
				status = PROCESS_STATUS_ERROR;
			}
			else
			{
				status = PROCESS_STATUS_FINISHED;
			}
		}
	}
	else if(rc == -1)
	{
		ERRORF("search", "`waitpid' failed, rc=%d.", rc);
		status = PROCESS_STATUS_ERROR;
	}

	return status;
}

static void
_search_kill_child(pid_t pid)
{
	DEBUGF("search", "Killing child process with pid %ld.", pid);

	int result = kill(pid, SIGTERM);

	if(result == -1)
	{
		WARNINGF("search", "Kill failed with status %d.", errno);

		result = kill(pid, SIGKILL);

		if(result == -1)
		{
			ERRORF("search", "Kill failed with status %d.", errno);
		}
	}
}

static void
_search_reader_args_init(ReaderArgs *args, FilterArgs *filter_args, void *user_data)
{
	assert(args != NULL);

	memset(args, 0, sizeof(ReaderArgs));
	args->filter_args = filter_args;
	args->user_data = user_data;
}

static void
_search_reader_args_free(ReaderArgs *args)
{
	assert(args != NULL);

	if(args && args->line)
	{
		free(args->line);
	}
}

static int
_search_parent_process(ParentCtx *ctx)
{
	assert(ctx != NULL);
	assert(ctx->child_pid > 0);
	assert(ctx->outfd > 0);
	assert(ctx->errfd > 0);

	fd_set rfds;
	Buffer outbuf;
	Buffer errbuf;
	ReaderArgs reader_args;
	int lc = 0;
	int status = PROCESS_STATUS_OK;

	DEBUG("search", "Initializing parent process.");

	buffer_init(&outbuf, 4096);
	buffer_init(&errbuf, 4096);

	_search_reader_args_init(&reader_args, &ctx->filter_args, ctx->user_data);

	DEBUGF("search", "Reading data from child process (pid=%ld).", ctx->child_pid);

	int maxfd = (ctx->errfd > ctx->outfd ? ctx->errfd : ctx->outfd) + 1;

	while(status == PROCESS_STATUS_OK)
	{
		FD_ZERO(&rfds);
		FD_SET(ctx->outfd, &rfds);
		FD_SET(ctx->errfd, &rfds);

		ssize_t bytes;
		ssize_t sum = 0;

		do
		{
			sum = 0;

			if(select(maxfd, &rfds, NULL, NULL, NULL) > 0)
			{
				if(FD_ISSET(ctx->outfd, &rfds))
				{
					reader_args.buffer = &outbuf;
					reader_args.cb = ctx->found_file;
					reader_args.filter = true;

					while(status == PROCESS_STATUS_OK && (bytes = buffer_fill_from_fd(&outbuf, ctx->outfd, 512)) > 0)
					{
						status = _search_process_lines_from_buffer(&reader_args);

						TRACEF("search", "Received %ld byte(s) from stdout, read %d line(s) from stdout buffer.", bytes, reader_args.count);

						if(status == PROCESS_STATUS_OK)
						{
							if(INT32_MAX - reader_args.count >= lc)
							{
								lc += reader_args.count;
							}
							else
							{
								ERROR("search", "Integer overflow.");
							}
						}

						if(SSIZE_MAX - bytes >= sum)
						{
							sum += bytes;
						}
					}
				}

				if(FD_ISSET(ctx->errfd, &rfds))
				{
					reader_args.buffer = &errbuf;
					reader_args.cb = ctx->err_message;
					reader_args.filter = false;

					while(status == PROCESS_STATUS_OK && (bytes = buffer_fill_from_fd(&errbuf, ctx->errfd, 512)) > 0)
					{
						TRACEF("search", "Read %ld byte(s) from stderr.", bytes);

						_search_process_lines_from_buffer(&reader_args);

						if(SSIZE_MAX - sum >= bytes)
						{
							sum += bytes;
						}
					}
				}
			}
		} while(status == PROCESS_STATUS_OK && sum);

		if(status == PROCESS_STATUS_STOP || status == PROCESS_STATUS_ERROR)
		{
			_search_kill_child(ctx->child_pid);
		}

		status = _search_wait_for_child(ctx, status);
	}

	TRACEF("search", "Child process exited: lc=%d, status=%#x.", lc, status);

	if(status == PROCESS_STATUS_OK)
	{
		TRACE("search", "Flushing all buffers.");

		reader_args.buffer = &outbuf;
		reader_args.cb = ctx->found_file;
		reader_args.filter = true;

		if(_search_flush_and_process_buffer(&reader_args) == PROCESS_STATUS_OK)
		{
			if(INT32_MAX - reader_args.count >= lc)
			{
				lc += reader_args.count;
			}
			else
			{
				ERROR("search", "Integer overflow.");
			}

			TRACEF("search", "Flushed stdout, lc=%d.", lc);
		}

		reader_args.buffer = &errbuf;
		reader_args.cb = ctx->err_message;
		reader_args.filter = false;

		_search_flush_and_process_buffer(&reader_args);
	}
	else if(status == PROCESS_STATUS_ERROR)
	{
		lc = -1;
	}

	TRACE("search", "Cleaning up parent process.");

	_search_reader_args_free(&reader_args);

	buffer_free(&outbuf);
	buffer_free(&errbuf);

	return lc;
}

static bool
_search_close_fd(int *fd)
{
	bool success = true;

	assert(fd != NULL);

	if(*fd >= 0)
	{
		if(close(*fd))
		{
			perror("close()");
			success = false;
		}
		else
		{
			*fd = -1;
		}
	}

	return success;
}

static bool
_search_close_parent_fds(int outfds[2], int errfds[2])
{
	return _search_close_fd(&outfds[1]) | _search_close_fd(&errfds[1]);
}

static bool
_search_close_and_dup_child_fds(int outfds[2], int errfds[2])
{
	bool success = false;

	if(close(0))
	{
		perror("close()");
		goto out;
	}

	if(dup2(outfds[1], 1) == -1)
	{
		perror("dup2()");
		goto out;
	}

	if(dup2(errfds[1], 2) == -1)
	{
		perror("dup2()");
		goto out;
	}

	success = true;

	for(int i = 0; i < 2; i++)
	{
		if((!_search_close_fd(&outfds[i])) | (!_search_close_fd(&errfds[i])))
		{
			success = false;
		}
	}

out:
	return success;
}

static void
_search_close_all_fds(int outfds[2], int errfds[2])
{
	for(int i = 0; i < 2; i++)
	{
		_search_close_fd(&outfds[i]);
		_search_close_fd(&errfds[i]);
	}
}

static void
_search_filter_args_init(FilterArgs *args, ParserResult *parser_result)
{
	assert(args != NULL);
	assert(parser_result != NULL);

	args->result = parser_result;
	args->extensions = extension_manager_new();

	extension_manager_load_default(args->extensions);
}

static void
_search_filter_args_free(FilterArgs *args)
{
	assert(args != NULL);

	if(args->extensions)
	{
		extension_manager_destroy(args->extensions);
	}
}

int
search_files(const char *path, const char *expr, TranslationFlags flags, const SearchOptions *opts, Callback found_file, Callback err_message, void *user_data)
{
	int ret = -1;

	assert(path != NULL);
	assert(expr != NULL);
	assert(opts != NULL);

	int outfds[2];
	int errfds[2];

	memset(outfds, 0, sizeof(outfds));
	memset(errfds, 0, sizeof(errfds));

	TRACE("search", "Creating pipes.");

	if(pipe2(outfds, 0) >= 0)
	{
		if(pipe2(errfds, 0) >= 0)
		{
			char **argv = NULL;
			size_t argc = 0;
			ParserResult *result;

			TRACE("search", "Pipes created successfully, translating expression.");

			result = _search_translate_expr(path, expr, flags, opts, &argc, &argv);

			assert(result != NULL);

			if(result->success)
			{
				DEBUG("search", "Expression parsed successfully, forking and running `find'.");

				pid_t pid = fork();

				if(pid == -1)
				{
					FATALF("search", "`fork' failed with result %ld.", pid);
					perror("fork()");
				}
				else if(pid == 0)
				{
					if(_search_close_and_dup_child_fds(outfds, errfds))
					{
						_search_child_process(argv);
					}
					else
					{
						ERROR("search", "Couldn't initialize child's file descriptors.");
					}
				}
				else
				{
					if(_search_close_parent_fds(outfds, errfds))
					{
						ParentCtx ctx;

						memset(&ctx, 0, sizeof(ParentCtx));

						ctx.child_pid = pid;
						ctx.outfd = outfds[0];
						ctx.errfd = errfds[0];
						ctx.found_file = found_file;
						ctx.err_message = err_message;
						ctx.user_data = user_data;

						_search_filter_args_init(&ctx.filter_args, result);

						ret = _search_parent_process(&ctx);

						_search_filter_args_free(&ctx.filter_args);
					}
					else
					{
						WARNING("search", "Couldn't close parent's file descriptors.");
					}
				}
			}
			else if(result->err)
			{
				TRACEF("search", "Couldn't parse expression: %s", result->err);
				fprintf(stderr, "%s\n", result->err);
			}
			else
			{
				TRACE("search", "Couldn't parse expression, no error message set.");
			}

			DEBUGF("search", "Search finished with result %d.", ret);

			if(argv)
			{
				for(size_t i = 0; i < argc; i++)
				{
					free(argv[i]);
				}

				free(argv);
			}

			parser_result_free(result);
			_search_close_all_fds(outfds, errfds);
		}
		else
		{
			perror("pipe2()");

			if(close(outfds[0]))
			{
				perror("close()");
			}

			if(close(outfds[1]))
			{
				perror("close()");
			}
		}
	}
	else
	{
		perror("pipe2()");
	}

	return ret;
}

bool
search_debug(FILE *out, FILE *err, const char *path, const char *expr, TranslationFlags flags, const SearchOptions *opts)
{
	bool success = false;

	assert(out != NULL);
	assert(err != NULL);
	assert(path != NULL);
	assert(expr != NULL);
	assert(opts != NULL);

	TRACEF("search", "Translating expression: %s, flags=%#x", expr, flags);

	size_t argc = 0;
	char **argv = NULL;

	ParserResult *result = _search_translate_expr(path, expr, flags, opts, &argc, &argv);

	assert(result != NULL);

	if(result->success)
	{
		TRACE("search", "Expression translated successfully, printing `find' arguments.");

		if(*argv)
		{
			bool quote = false;

			fputs(*argv, out);
			free(*argv);

			for(size_t i = 1; i < argc; i++)
			{
				fputc(' ', out);

				if(quote)
				{
					fprintf(out, "\"%s\"", argv[i]);
				}
				else
				{
					fprintf(out, "%s", argv[i]);
				}

				quote = false;

				if(flags & TRANSLATION_FLAG_QUOTE)
				{
					if(!strcmp(argv[i], "-regextype"))
					{
						quote = true;
					}
				}

				free(argv[i]);
			}
		}

		fprintf(out, "\n");
		free(argv);

		success = true;
	}
	else if(result->err)
	{
		TRACEF("search", "Couldn't parse expression: %s", result->err);
		fprintf(err, "%s\n", result->err);
	}
	else
	{
		TRACE("search", "Couldn't parse expression, no error message set.");
	}

	parser_result_free(result);

	TRACEF("search", "Printing translated `find' arguments finished with result %d.", success);

	return success;
}

