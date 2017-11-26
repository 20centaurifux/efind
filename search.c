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
   @brief A find-wrapper.
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
typedef EvalResult (*PreCondition)(const char *filename, void *user_data);

typedef struct
{
	ParserResult *result;
	ExtensionManager *extensions;
} PreArgs;

#define ABORT_SEARCH -1
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

static EvalResult
_search_evaluate_post_exprs(const char *filename, void *user_data)
{
	PreArgs *args = (PreArgs *)user_data;
	EvalResult result = EVAL_RESULT_TRUE;

	assert(filename != NULL);
	assert(args != NULL);

	if(args->result->root->post_exprs)
	{
		TRACEF("search", "Filtering file: %s", filename);

		if(args->extensions)
		{
			result = evaluate(args->extensions, args->result->root->post_exprs, filename);

			if(result == EVAL_RESULT_ABORTED)
			{
				TRACE("search", "Evaluation aborted.");
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

static bool
_search_process_line(const char *line, PreCondition pre, void *pre_data, Callback cb, void *user_data)
{
	bool success = true;

	assert(line != NULL);
	assert(cb != NULL);

	if(pre)
	{
		EvalResult result = pre(line, pre_data);

		if(result == EVAL_RESULT_TRUE)
		{
			cb(line, user_data);
		}
		else if(result == EVAL_RESULT_ABORTED)
		{
			success = false;
		}
	}
	else
	{
		cb(line, user_data);
	}

	return success;
}

static int
_search_process_lines_from_buffer(Buffer *buf, char **line, size_t *llen, PreCondition pre, void *pre_data, Callback cb, void *user_data)
{
	int count = 0;

	assert(buf != NULL);
	assert(line != NULL);
	assert(llen != NULL);
	assert(cb != NULL);

	while(count != ABORT_SEARCH && buffer_read_line(buf, line, llen))
	{
		if(_search_process_line(*line, pre, pre_data, cb, user_data))
		{
			if(count < INT32_MAX)
			{
				++count;
			}
		}
		else
		{
			count = ABORT_SEARCH;
		}
	}

	return count;
}

static int
_search_flush_and_process_buffer(Buffer *buf, char **line, size_t *llen, PreCondition pre, void *pre_data, Callback cb, void *user_data)
{
	int count = 0;

	assert(buf != NULL);
	assert(line != NULL);
	assert(llen != NULL);
	assert(cb != NULL);

	if(cb && !buffer_is_empty(buf))
	{
		count = _search_process_lines_from_buffer(buf, line, llen, pre, pre_data, cb, user_data);

		if(count != ABORT_SEARCH && buffer_flush(buf, line, llen))
		{
			if(_search_process_line(*line, pre, pre_data, cb, user_data))
			{
				if(count < INT32_MAX)
				{
					++count;
				}
			}
		}
	}

	return count;
}

static int
_search_close_fd(int *fd)
{
	int ret = 0;

	assert(fd != NULL);

	if(*fd >= 0)
	{
		if((ret = close(*fd)))
		{
			perror("close()");
		}
		else
		{
			*fd = -1;
		}
	}

	return ret;
}

static void
_search_merge_options(size_t *argc, char ***argv, const char *path, const SearchOptions *opts)
{
	char **nargv;
	size_t maxsize;
	size_t index = 0;

	assert(argc != NULL);
	assert(argv != NULL);
	assert(path != NULL);
	assert(opts != NULL);

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
	for(size_t i = 0; i < *argc; ++i)
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
	ParserResult *result;
	char *err = NULL;

	assert(path != NULL);
	assert(expr != NULL);
	assert(opts != NULL);
	assert(argc != NULL);
	assert(argv != NULL);

	*argc = 0;
	*argv = NULL;

	/* translate string to find arguments */
	result = parse_string(expr);

	if(result->success)
	{
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

	/* cleanup on failure */
	if(!result->success && *argv)
	{
		for(size_t i = 0; i < *argc; ++i)
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
_search_child_process(int outfds[2], int errfds[2], char **argv)
{
	char *exe;

	/* close/dup descriptors */
	if(close(0))
	{
		perror("close()");
		return;
	}

	if(dup2(outfds[1], 1) == -1)
	{
		perror("dup2()");
		return;
	}

	if(dup2(errfds[1], 2) == -1)
	{
		perror("dup2()");
		return;
	}

	for(int i = 0; i < 2; ++i)
	{
		if(_search_close_fd(&outfds[i]) || _search_close_fd(&errfds[i]))
		{
			return;
		}
	}

	/* run find */
	if((exe = utils_whereis("find")))
	{
		if(execv(exe, argv) == -1)
		{
			perror("execl()");
		}

		free(exe);
	}
	else
	{
		fprintf(stderr, _("Couldn't find `find' executable.\n"));
	}
}

static int
_search_parent_process(pid_t pid, int outfds[2], int errfds[2], ParserResult *result, Callback found_file, Callback err_message, void *user_data)
{
	fd_set rfds;
	Buffer outbuf;
	Buffer errbuf;
	char *line = NULL;
	size_t llen = 0;
	int lc = 0;
	PreArgs pre_args;

	DEBUG("search", "Initializing parent process.");

	const int PROCESS_STATUS_OK       = 0;
	const int PROCESS_STATUS_ERROR    = 1;
	const int PROCESS_STATUS_FINISHED = 2;

	int status = PROCESS_STATUS_OK;

	/* close descriptors */
	if(_search_close_fd(&outfds[1]) || _search_close_fd(&errfds[1]))
	{
		return -1;
	}

	/* initialize buffers */
	buffer_init(&outbuf, 4096);
	buffer_init(&errbuf, 4096);

	/* initialize pre condition argument */
	pre_args.result = result;
	pre_args.extensions = extension_manager_new();

	extension_manager_load_default(pre_args.extensions);

	/* read from pipes until child process terminates or an error occurs */
	int maxfd = (errfds[0] > outfds[0] ? errfds[0] : outfds[0]) + 1;

	DEBUGF("search", "Reading data from child process (pid=%ld).", pid);

	while(status == PROCESS_STATUS_OK)
	{
		/* read data from pipes */
		FD_ZERO(&rfds);
		FD_SET(outfds[0], &rfds);
		FD_SET(errfds[0], &rfds);

		ssize_t bytes;
		ssize_t sum = 0;

		do
		{
			sum = 0;

			if(select(maxfd, &rfds, NULL, NULL, NULL) > 0)
			{
				/* read from stdout & write bytes to related buffer */
				if(FD_ISSET(outfds[0], &rfds))
				{
					while(status == PROCESS_STATUS_OK && (bytes = buffer_fill_from_fd(&outbuf, outfds[0], 512)) > 0)
					{
						/* process received lines */
						int count = _search_process_lines_from_buffer(&outbuf, &line, &llen, _search_evaluate_post_exprs, &pre_args, found_file, user_data);

						TRACEF("search", "Received %ld byte(s) from stdout, read %d line(s) from stdout buffer.", bytes, count);

						if(count == ABORT_SEARCH)
						{
							status = PROCESS_STATUS_ERROR;
						}
						else
						{
							if(INT32_MAX - count >= lc)
							{
								lc += count;
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

				/* read from stderr & write bytes to related buffer */
				if(FD_ISSET(errfds[0], &rfds))
				{
					while(status == PROCESS_STATUS_OK && (bytes = buffer_fill_from_fd(&errbuf, errfds[0], 512)) > 0)
					{
						TRACEF("search", "Read %ld byte(s) from stderr.", bytes);

						/* process received lines */
						_search_process_lines_from_buffer(&errbuf, &line, &llen, NULL, NULL, err_message, user_data);

						if(SSIZE_MAX - sum >= bytes)
						{
							sum += bytes;
						}
					}
				}
			}
		} while(status == PROCESS_STATUS_OK && sum);

		/* check if child process is still running */
		int child_status;
		int rc;

		DEBUGF("search", "Waiting for child process with pid %ld.", pid);

		if((rc = waitpid(pid, &child_status, WNOHANG)) == pid)
		{
			if(WIFEXITED(child_status) && status == PROCESS_STATUS_OK)
			{
				status = PROCESS_STATUS_FINISHED;
			}
		}
		else if(rc == -1)
		{
			ERRORF("search", "`waitpid' failed, rc=%d.", rc);
			perror("waitpid()");
			status = PROCESS_STATUS_ERROR;
		}
	}

	/* flush buffers */
	TRACEF("search", "Child process exited: lc=%d, status=%#x.", lc, status);

	if(status == PROCESS_STATUS_OK)
	{
		TRACE("search", "Flushing all buffers.");

		int count = _search_flush_and_process_buffer(&outbuf, &line, &llen, _search_evaluate_post_exprs, &pre_args, found_file, user_data);

		if(count != ABORT_SEARCH)
		{
			if(INT32_MAX - count >= lc)
			{
				lc += count;
			}
			else
			{
				ERROR("search", "Integer overflow.");
			}

			TRACEF("search", "Flushed stdout, lc=%d.", lc);
		}

		_search_flush_and_process_buffer(&errbuf, &line, &llen, NULL, NULL, err_message, user_data);
	}
	else if(status == PROCESS_STATUS_ERROR)
	{
		lc = -1;
	}

	/* cleanup */
	TRACE("search", "Cleaning up parent process.");

	buffer_free(&outbuf);
	buffer_free(&errbuf);
	free(line);
	
	if(pre_args.extensions)
	{
		extension_manager_destroy(pre_args.extensions);
	}

	return lc;
}

int
search_files_expr(const char *path, const char *expr, TranslationFlags flags, const SearchOptions *opts, Callback found_file, Callback err_message, void *user_data)
{
	int outfds[2]; // redirect stdout
	int errfds[2]; // redirect stderr
	int ret = -1;

	assert(path != NULL);
	assert(expr != NULL);
	assert(opts != NULL);
	assert(found_file != NULL);
	assert(err_message != NULL);

	memset(outfds, 0, sizeof(outfds));
	memset(errfds, 0, sizeof(errfds));

	/* create pipes */
	TRACE("search", "Creating pipes.");

	if(pipe2(outfds, 0) >= 0 && pipe2(errfds, 0) >= 0)
	{
		char **argv = NULL;
		size_t argc = 0;
		ParserResult *result;

		TRACE("search", "Pipes created successfully, translating expression.");

		/* parse expression */
		result = _search_translate_expr(path, expr, flags, opts, &argc, &argv);

		assert(result != NULL);

		if(result->success)
		{
			/* execute find */
			DEBUG("search", "Expression parsed successfully, forking and running `find'.");

			pid_t pid = fork();

			if(pid == -1)
			{
				FATALF("search", "`fork' failed with result %ld.", pid);
				perror("fork()");
			}
			else if(pid == 0)
			{
				_search_child_process(outfds, errfds, argv);
			}
			else
			{
				ret = _search_parent_process(pid, outfds, errfds, result, found_file, err_message, user_data);
			}
		}
		else if(result->err)
		{
			fprintf(stderr, "%s\n", result->err);
		}

		DEBUGF("search", "Search finished with result %d.", ret);

		/* cleanup */
		TRACE("search", "Cleaning up.");

		if(argv)
		{
			for(size_t i = 0; i < argc; ++i)
			{
				free(argv[i]);
			}

			free(argv);
		}

		parser_result_free(result);

		/* close descriptors */
		for(int i = 0; i < 2; ++i)
		{
			_search_close_fd(&outfds[i]);
			_search_close_fd(&errfds[i]);
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
	size_t argc;
	char **argv;
	ParserResult *result;
	bool success = false;

	assert(out != NULL);
	assert(err != NULL);
	assert(path != NULL);
	assert(expr != NULL);
	assert(opts != NULL);

	TRACEF("search", "Translating expression: %s, flags=%#x", expr, flags);

	result = _search_translate_expr(path, expr, flags, opts, &argc, &argv);

	assert(result != NULL);

	if(result->success)
	{
		TRACE("search", "Expression translated successfully, printing `find' arguments.");

		if(*argv)
		{
			bool quote = false;

			fputs(*argv, out);
			free(*argv);

			for(size_t i = 1; i < argc; ++i)
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

