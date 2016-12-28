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
   @version 0.1.0
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
#include "parser.h"
#include "utils.h"
#include "eval.h"

/*! @cond INTERNAL */
typedef EvalResult (*PreCondition)(const char *filename, struct stat *stbuf, void *user_data);

typedef struct
{
	ParserResult *result;
	ExtensionDir *dir;
} PostExprsArgs;

#define ABORT_SEARCH -1
/*! @endcond */

static EvalResult
_search_post_exprs(const char *filename, struct stat *stbuf, void *user_data)
{
	PostExprsArgs *args = (PostExprsArgs *)user_data;
	EvalResult result = EVAL_RESULT_TRUE;

	assert(filename != NULL);
	assert(stbuf != NULL);
	assert(args != NULL);

	if(args->result->root->post_exprs)
	{
		result = evaluate(args->result->root->post_exprs, args->dir, filename, stbuf);
	}

	return result;
}

static int
_search_process_lines(Buffer *buf, char **line, size_t *llen, PreCondition pre, void *pre_data, Callback cb, void *user_data)
{
	int count = 0;

	assert(buf != NULL);
	assert(line != NULL);
	assert(llen != NULL);
	assert(cb != NULL);

	while(count != ABORT_SEARCH && buffer_read_line(buf, line, llen))
	{
		if(cb)
		{
			if(pre)
			{
				struct stat stbuf;

				if(!stat(*line, &stbuf))
				{
					EvalResult result = pre(*line, &stbuf, pre_data);

					if(result == EVAL_RESULT_TRUE)
					{
						cb(*line, user_data);
						++count;
					}
					else if(result == EVAL_RESULT_ABORTED)
					{
						count = ABORT_SEARCH;
					}
				}
				else
				{
					fprintf(stderr, "Couldn't stat \"%s\".", *line);
				}
			}
			else
			{
				cb(*line, user_data);
				++count;
			}
		}
	}

	return count;
}

static int
_search_flush_buffer(Buffer *buf, char **line, size_t *llen, PreCondition pre, void *pre_data, Callback cb, void *user_data)
{
	int count = 0;

	assert(buf != NULL);
	assert(line != NULL);
	assert(llen != NULL);
	assert(cb != NULL);

	if(cb && !buffer_is_empty(buf))
	{
		count = _search_process_lines(buf, line, llen, pre, pre_data, cb, user_data);

		if(count != ABORT_SEARCH)
		{
			if(buffer_flush(buf, line, llen))
			{
				cb(*line, user_data);
				++count;
			}
		}
	}

	return count;
}

static ExtensionDir *
_search_load_extension_dir(void)
{
	ExtensionDir *dir = NULL;
	const char *home = getenv("HOME");
	char path[PATH_MAX];

	if(home)
	{
		if(utils_path_join(home, ".efind/extensions", path, PATH_MAX))
		{
			char *err = NULL;

			dir = extension_dir_load(path, &err);

			if(err)
			{
				fprintf(stderr, "%s\n", err);
				free(err);
			}
		}
	}

	return dir;
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
			search_merge_options(argc, argv, path, opts);
		}
		else
		{
			result->success = false;
			result->err = err;
		}
	}
	else
	{
		/* reset parsed arguments if translation has failed */
		if(*argv)
		{
			for(size_t i = 0; i < *argc; ++i)
			{
				free((*argv)[i]);
			}

			*argc = 0;
			free(*argv);
			*argv = NULL;
		}
	}

	return result;
}

void
search_merge_options(size_t *argc, char ***argv, const char *path, const SearchOptions *opts)
{
	char **nargv;
	size_t maxsize;
	size_t index = 0;

	assert(argc != NULL);
	assert(argv != NULL);
	assert(path != NULL);
	assert(opts != NULL);

	/* initialize argument vector */
	maxsize = (*argc) + 6; /* "find" + path + *argv + options + NULL */

	nargv = (char **)utils_malloc(sizeof(char *) * maxsize);
	memset(nargv, 0, sizeof(char *) * maxsize);

	/* first argument (executable name) */
	nargv[index++] = strdup("find");

	/* follow symbolic links? */
	if(opts && opts->follow)
	{
		nargv[index++] = strdup("-L");
	}

	/* copy search path */
	nargv[index++] = strdup(path);

	/* copy translated find arguments */
	for(size_t i = 0; i < *argc; ++i)
	{
		nargv[index++] = (*argv)[i];
	}

	/* maximum search depth */
	if(opts && opts->max_depth >= 0)
	{
		char buffer[16];

		snprintf(buffer, 16, "%d", opts->max_depth);

		nargv[index++] = strdup("-maxdepth");
		nargv[index++] = strdup(buffer);
	}

	*argc = index;

	free(*argv);
	*argv = nargv;
}

int
search_files_expr(const char *path, const char *expr, TranslationFlags flags, const SearchOptions *opts, Callback found_file, Callback err_message, void *user_data)
{
	int i;
	int ret = -1;
	pid_t pid;
	ParserResult *result = NULL;
	int outfds[2]; // redirect stdout
	int errfds[2]; // redirect stderr
	char **argv = NULL;
	size_t argc = 0;
	
	assert(path != NULL);
	assert(expr != NULL);
	assert(opts != NULL);
	assert(found_file != NULL);
	assert(err_message != NULL);

	memset(outfds, 0, sizeof(outfds));
	memset(errfds, 0, sizeof(errfds));

	/* create pipes */
	if(pipe2(outfds, 0) == -1 || pipe2(errfds, 0) == -1)
	{
		perror("pipe2()");
		goto out;
	}

	/* parse expression */
	result = _search_translate_expr(path, expr, flags, opts, &argc, &argv);

	if(!result->success)
	{
		goto out;
	}

	/* create child process */
	pid = fork();

	if(pid == -1)
	{
		perror("fork()");
		goto out;
	}

	if(pid == 0)
	{
		/* close/dup descriptors */
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

		for(i = 0; i < 2; ++i)
		{
			if(_search_close_fd(&outfds[i]) || _search_close_fd(&errfds[i]))
			{
				goto out;
			}
		}

		/* run find */
		char *exe;

		if((exe = utils_whereis("find")))
		{
			if(result->success)
			{
				if(execv(exe, argv) == -1)
				{
					perror("execl()");
					goto out;
				}
			}

			free(exe);
		}
		else
		{
			fprintf(stderr, "Couldn't find 'find' executable.\n");
		}
	}
	else
	{
		/* close descriptors */
		if(_search_close_fd(&outfds[1]) || _search_close_fd(&errfds[1]))
		{
			goto out;
		}

		/* initialize post processing arguments */
		PostExprsArgs post_args;
		memset(&post_args, 0, sizeof(PostExprsArgs));

		post_args.result = result;
		post_args.dir = _search_load_extension_dir();

		/* read from pipes until child process terminates */
		int status;
		fd_set rfds;
		int maxfd = (errfds[0] > outfds[0] ? errfds[0] : outfds[0]) + 1;

		Buffer outbuf;
		Buffer errbuf;

		buffer_init(&outbuf, 4096);
		buffer_init(&errbuf, 4096);

		char *line = NULL;
		size_t llen = 0;

		bool finished = false;

		ret = 0;

		while(!finished)
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
					if(FD_ISSET(outfds[0], &rfds))
					{
						while((bytes = buffer_fill_from_fd(&outbuf, outfds[0], 512)) > 0)
						{
							int count = _search_process_lines(&outbuf, &line, &llen, _search_post_exprs, &post_args, found_file, user_data);

							if(count == ABORT_SEARCH)
							{
								break;
							}

							ret += count;
							sum += bytes;
						}
					}

					if(FD_ISSET(errfds[0], &rfds))
					{
						while((bytes = buffer_fill_from_fd(&errbuf, errfds[0], 512)) > 0)
						{
							_search_process_lines(&errbuf, &line, &llen, NULL, NULL, err_message, user_data);
							sum += bytes;
						}
					}
				}
			} while(sum);

			/* test if child is still running */
			int rc;

			if((rc = waitpid(pid, &status, WNOHANG)) == pid)
			{
				if(WIFEXITED(status))
				{
					finished = true;
				}
			}
			else if(rc == -1)
			{
				perror("waitpid()");
				finished = true;
			}
		}

		/* flush buffers */
		ret += _search_flush_buffer(&outbuf, &line, &llen, _search_post_exprs, &post_args, found_file, user_data);
		_search_flush_buffer(&errbuf, &line, &llen, NULL, NULL, err_message, user_data);

		/* clean up */
		buffer_free(&outbuf);
		buffer_free(&errbuf);
		free(line);
		
		if(post_args.dir)
		{
			extension_dir_destroy(post_args.dir);
		}
	}


	out:
		/* cleanup */
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
		for(i = 0; i < 2; ++i)
		{
			_search_close_fd(&outfds[i]);
			_search_close_fd(&errfds[i]);
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

	result = _search_translate_expr(path, expr, flags, opts, &argc, &argv);

	if(result->success)
	{
		for(size_t i = 0; i < argc; ++i)
		{
			fprintf(out, "%s ", argv[i]);
			free(argv[i]);
		}

		fprintf(out, "\n");
		free(argv);

		success = true;
	}

	if(result->err)
	{
		fprintf(err, "%s\n", result->err);
	}

	parser_result_free(result);

	return success;
}

