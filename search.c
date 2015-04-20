#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "buffer.h"
#include "search.h"
#include "parser.h"
#include "utils.h"

static int
_search_read_lines(Buffer *buf, char **line, size_t *llen, Callback cb, void *user_data)
{
	int count = 0;

	while(buffer_read_line(buf, line, llen))
	{
		if(cb)
		{
			cb(*line, user_data);
		}
	}

	return count;
}

static int
_search_flush_buffer(Buffer *buf, char **line, size_t *llen, Callback cb, void *user_data)
{
	int ret = 0;

	if(cb && !buffer_is_empty(buf))
	{
		ret = _search_read_lines(buf, line, llen, cb, user_data);

		if(buffer_flush(buf, line, llen))
		{
			cb(*line, user_data);
		}
	}

	return ret;
}

static int
_search_close_fd(int *fd)
{
	int ret = 0;

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

static bool
_search_translate_expr(const char *path, const char *expr, TranslationFlags flags, size_t *argc, char ***argv)
{
	size_t fargc;
	char **fargv;
	char *err = NULL;
	bool success = false;

	if(parse_string(expr, flags, &fargc, &fargv, &err))
	{
		*argc = fargc + 3;
		*argv = (char **)utils_malloc(sizeof(char *) * (*argc));

		memset(*argv, 0, sizeof(char *) * (*argc));

		(*argv)[0] = strdup("find");
		(*argv)[1] = strdup(path);

		for(size_t i = 0; i < fargc; ++i)
		{
			(*argv)[i + 2] = fargv[i];
		}

		free(fargv);

		success = true;
	}

	if(err)
	{
		fprintf(stderr, "%s\n", err);
		free(err);
	}

	return success;
}

int
search_files_expr(const char *path, const char *expr, TranslationFlags flags, Callback found_file, Callback err_message, void *user_data)
{
	int i;
	int ret = -1;
	pid_t pid;
	int outfds[2]; // redirect stdout
	int errfds[2]; // redirect stderr

	memset(outfds, 0, sizeof(outfds));
	memset(errfds, 0, sizeof(errfds));

	/* create pipes */
	if(pipe2(outfds, 0) == -1 || pipe2(errfds, 0) == -1)
	{
		perror("pipe2()");
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
		char **argv;
		size_t argc;

		if(_search_translate_expr(path, expr, flags, &argc, &argv))
		{
			if(execv("/usr/bin/find", argv) == -1)
			{
				perror("execl()");
				goto out;
			}
		}
	}
	else
	{
		/* close descriptors */
		if(_search_close_fd(&outfds[1]) || _search_close_fd(&errfds[1]))
		{
			goto out;
		}

		/* read from pipes until child process terminates */
		int status;
		fd_set rfds;
		int maxfd = (errfds[0] > outfds[0] ? errfds[0] : outfds[0]) + 1;
		int rc;

		Buffer outbuf;
		Buffer errbuf;

		buffer_init(&outbuf);
		buffer_init(&errbuf);

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
							ret += _search_read_lines(&outbuf, &line, &llen, found_file, user_data);
							sum += bytes;
						}
					}

					if(FD_ISSET(errfds[0], &rfds))
					{
						while((bytes = buffer_fill_from_fd(&errbuf, errfds[0], 512)) > 0)
						{
							_search_read_lines(&errbuf, &line, &llen, err_message, user_data);
							sum += bytes;
						}
					}
				}
			} while(sum);

			/* test if child is still running */
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
			}
		}

		/* flush buffers */
		ret += _search_flush_buffer(&outbuf, &line, &llen, found_file, user_data);
		_search_flush_buffer(&errbuf, &line, &llen, err_message, user_data);

		/* clean up */
		buffer_free(&outbuf);
		buffer_free(&errbuf);
		free(line);
	}

	out:
		/* close descriptors */
		for(i = 0; i < 2; ++i)
		{
			_search_close_fd(&outfds[i]);
			_search_close_fd(&errfds[i]);
		}

		return ret;
}

