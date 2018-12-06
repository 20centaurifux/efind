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
   @file options_getopt.c
   @brief Read command line options.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "efind.h"
#include "exec-args.h"
#include "utils.h"
#include "gettext.h"

static void
_get_opt_append_single_search_dir(char *argv[], Options *opts)
{
	assert(argv != NULL);
	assert(opts != NULL);

	if(!slist_contains(&opts->dirs, argv[1]))
	{
		slist_append(&opts->dirs, utils_strdup(argv[1]));
	}
}

static void
_get_opt_append_multiple_search_dirs(char *argv[], int offset, Options *opts)
{
	int limit = offset;

	assert(argv != NULL);
	assert(opts != NULL);

	/* handle all arguments as directories if expression is already set */
	if(opts->expr)
	{
		++limit;
	}
	else
	{
		opts->expr = utils_strdup(argv[offset]);
		opts->flags &= ~FLAG_STDIN; 
	}

	for(int i = 1; i < limit; ++i)
	{
		if(!slist_contains(&opts->dirs, argv[i]))
		{
			slist_append(&opts->dirs, utils_strdup(argv[i]));
		}
	}
}

static void
_get_opt_append_search_dirs_and_expr_from_argv(char *argv[], int offset, Options *opts)
{
	assert(argv != NULL);
	assert(opts != NULL);

	if(offset)
	{
		if(offset == 1)
		{
			_get_opt_append_single_search_dir(argv, opts);
		}
		else
		{
			_get_opt_append_multiple_search_dirs(argv, offset, opts);
		}
	}
}

static bool
_get_opt_parse_flag(const char *value, Options *opts, const char *name, int flag)
{
	bool set;

	assert(value != NULL);
	assert(opts != NULL);

	bool success = utils_parse_bool(optarg, &set);

	if(success)
	{
		if(set)
		{
			opts->flags |= flag;
		}
		else
		{
			opts->flags &= ~flag;
		}
	}
	else
	{
		fprintf(stderr, _("Argument of option `%s' is malformed.\n"), name);
	}

	return success;
}

static char **
_get_opt_copy_argv(int argc, char *argv[], int offset)
{
	char **argv_c = NULL;

	if(offset && offset < argc)
	{
		argv_c = utils_malloc(sizeof(char *) * (argc - offset));
		*argv_c = utils_strdup(*argv);

		for(int i = 1; i < argc - offset; ++i)
		{
			argv_c[i] = utils_strdup(argv[i + offset]);
		}
	}

	return argv_c;
}

static Action
_get_opt(int argc, char *argv[], int offset, Options *opts)
{
	static struct option long_options[] =
	{
		{ "expr", required_argument, 0, 'e' },
		{ "quote", optional_argument, 0, 'q' },
		{ "dir", required_argument, 0, 'd' },
		{ "print", no_argument, 0, 'p' },
		{ "follow", optional_argument, 0, 'L' },
		{ "max-depth", required_argument, 0, 0 },
		{ "skip", required_argument, 0, 0 },
		{ "limit", required_argument, 0, 0 },
		{ "regex-type", required_argument, 0, 0 },
		{ "printf", required_argument, 0, 0 },
		{ "order-by", required_argument, 0, 0 },
		{ "print-extensions", no_argument, 0, 0 },
		{ "print-blacklist", no_argument, 0, 0 },
		{ "log-level", required_argument, 0, 0 },
		{ "log-color", required_argument, 0, 0 },
		{ "version", no_argument, 0, 'v' },
		{ "help", no_argument, 0, 'h' },
		{ 0, 0, 0, 0 }
	};

	Action action = ACTION_EXEC;
	int index = 0;

	assert(argc >= 1);
	assert(argv != NULL);
	assert(offset < argc);
	assert(opts != NULL);

	char **argv_ptr = argv;
	char **argv_heap = _get_opt_copy_argv(argc, argv, offset);

	if(argv_heap)
	{
		argv_ptr = argv_heap;
	}

	while(action != ACTION_ABORT)
	{
		int opt = getopt_long(argc - offset, argv_ptr, "e:d:q;pvhL;", long_options, &index);

		if(opt == -1)
		{
			break;
		}

		switch(opt)
		{
			case 'e':
				utils_copy_string(optarg, &opts->expr);
				opts->flags &= ~FLAG_STDIN; 
				break;

			case 'd':
				if(!slist_contains(&opts->dirs, optarg))
				{
					slist_append(&opts->dirs, utils_strdup(optarg));
				}
				break;

			case 'q':
				if(optarg == NULL)
				{
					opts->flags |= FLAG_QUOTE;
				}
				else if(!_get_opt_parse_flag(optarg, opts, "quote", FLAG_QUOTE))
				{
					action = ACTION_ABORT;
				}
				break;

			case 'p':
				action = ACTION_PRINT;
				break;

			case 'v':
				action = ACTION_PRINT_VERSION;
				break;

			case 'h':
				action = ACTION_PRINT_HELP;
				break;

			case 'L':
				if(optarg == NULL)
				{
					opts->follow = true;
				}
				else if(!utils_parse_bool(optarg, &opts->follow))
				{
					fprintf(stderr, _("Argument of option `%s' is malformed.\n"), "follow");
					action = ACTION_ABORT;
				}
				break;

			case 0:
				if(!strcmp(long_options[index].name, "log-level") && optarg)
				{
					opts->log_level = atoi(optarg);
				}
				else if(!strcmp(long_options[index].name, "log-color"))
				{
					if(!utils_parse_bool(optarg, &opts->log_color))
					{
						fprintf(stderr, _("Argument of option `%s' is malformed.\n"), "log-color");
					}
				}
				else if(!strcmp(long_options[index].name, "max-depth") && optarg)
				{
					opts->max_depth = atoi(optarg);
				}
				else if(!strcmp(long_options[index].name, "print-extensions"))
				{
					action = ACTION_PRINT_EXTENSIONS;
				}
				else if(!strcmp(long_options[index].name, "print-blacklist"))
				{
					action = ACTION_PRINT_BLACKLIST;
				}
				else if(!strcmp(long_options[index].name, "regex-type"))
				{
					utils_copy_string(optarg, &opts->regex_type);
				}
				else if(!strcmp(long_options[index].name, "printf"))
				{
					utils_copy_string(optarg, &opts->printf);
				}
				else if(!strcmp(long_options[index].name, "order-by"))
				{
					utils_copy_string(optarg, &opts->orderby);
				}
				else if(!strcmp(long_options[index].name, "skip") && optarg)
				{
					opts->skip = atoi(optarg);
				}
				else if(!strcmp(long_options[index].name, "limit") && optarg)
				{
					opts->limit = atoi(optarg);
				}

				break;

			default:
				action = ACTION_ABORT;
		}
	}

	if(argv_heap)
	{
		for(int i = 0; i < argc - offset; ++i)
		{
			free(argv_heap[i]);
		}

		free(argv_heap);
	}

	return action;
}

static int
_get_opt_index_of_first_option(int argc, char *argv[])
{
	int index = 0;

	assert(argc >= 1);
	assert(argv != NULL);

	for(int i = 1; i < argc; ++i)
	{
		if(*argv[i] != '-')
		{
			++index;
		}
		else
		{
			break;
		}
	}

	return index;
}

static void
_get_opt_free_argv(int argc, char ***argv)
{
	assert(argv != NULL);

	if(*argv)
	{
		for(int i = 0; i < argc; ++i)
		{
			free((*argv)[i]);
		}

		free(*argv);
	}
}

static bool
_get_opt_steal_exec_args(int argc, char *argv[], int *new_argc, char ***new_argv, Options *opts)
{
	bool success = true;

	assert(argc > 0);
	assert(argv != NULL);
	assert(new_argc != NULL);
	assert(new_argv != NULL);
	assert(opts != NULL);

	*new_argv = (char **)utils_malloc(sizeof(char *) * argc);
	*new_argc = 0;

	bool open = false;
	bool malformed = false;
	int start = 0;

	for(int i = 0; i < argc && !malformed; ++i)
	{
		if(open)
		{
			if(!strcmp(argv[i], ";"))
			{
				open = false;

				if(i - start)
				{
					ExecArgs *args = exec_args_new();

					for(int offset = start; offset < i; ++offset)
					{
						exec_args_append(args, argv[offset]);
					}

					slist_append(&opts->exec, args);
				}
				else
				{
					malformed = true;
				}
			}
		}
		else if(!strcmp(argv[i], "--exec"))
		{
			open = true;
			start = i + 1;
		}
		else
		{
			(*new_argv)[*new_argc] = utils_strdup(argv[i]);
			++(*new_argc);
		}
	}

	success = !open && !malformed;

	if(open)
	{
		fprintf(stderr, _("Invalid --exec option, `;' argument is missing.\n"));
	}
	else if(malformed)
	{
		fprintf(stderr, _("Invalid --exec option, argument list is empty.\n"));
	}

	if(!success)
	{
		_get_opt_free_argv(*new_argc, new_argv);
	}

	return success;
}

Action
options_getopt(Options *opts, int argc, char *argv[])
{
	Action action = ACTION_ABORT;

	assert(opts != NULL);
	assert(argc > 0);
	assert(argv != NULL);

	int no_exec_argc = 0;
	char **no_exec_argv = NULL;

	if(_get_opt_steal_exec_args(argc, argv, &no_exec_argc, &no_exec_argv, opts))
	{
		int offset = _get_opt_index_of_first_option(no_exec_argc, no_exec_argv);

		action = _get_opt(no_exec_argc, no_exec_argv, offset, opts);

		if(action != ACTION_ABORT)
		{
			_get_opt_append_search_dirs_and_expr_from_argv(no_exec_argv, offset, opts);
		}

		_get_opt_free_argv(no_exec_argc, &no_exec_argv);
	}

	return action;
}

