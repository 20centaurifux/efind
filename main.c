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
   @file main.c
   @brief efind application code.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
   @version 0.1.0
*/
/*! @cond INTERNAL */
#define _GNU_SOURCE
/*! @endcond */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <readline/readline.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

#include "search.h"
#include "parser.h"
#include "utils.h"
#include "extension.h"

/*! Major version. */
#define VERSION_MAJOR 0
/*! Minor version. */
#define VERSION_MINOR 1
/*! Patch level. */
#define VERSION_PATCH 0

/**
   @enum Action
   @brief Available applicatio actions.
 */
typedef enum
{
	/*! Abort program. */
	ACTION_ABORT,
	/*! Translate expression & execute find. */
	ACTION_EXEC,
	/*! Translate & print expression. */
	ACTION_PRINT,
	/*! Show help and quit. */
	ACTION_PRINT_HELP,
	/*! Show version and quit. */
	ACTION_PRINT_VERSION,
	/*! Show extensions and quit. */
	ACTION_LIST_EXTENSIONS
} Action;

/**
   @enum Flags
   @brief Application flags.
 */
typedef enum
{
	/*! No flags. */
	FLAG_NONE   = 0,
	/*! Read expression from stdin. */
	FLAG_STDIN  = 1,
	/*! Quote special terminal characters. */
	FLAG_QUOTE  = 2
} Flags;

/**
   @struct Options
   @brief Application options.
 */
typedef struct
{
	/*! Flags. */
	int32_t flags;
	/*! Expression to translate. */
	char *expr;
	/*! Directory to search. */
	char *dir;
	/*! Directory search level limitation. */
	int32_t max_depth;
	/*! Dereference symbolic links. */
	bool follow;
} Options;

static Action
_read_options(int argc, char *argv[], Options *opts)
{
	static struct option long_options[] =
	{
		{ "expr", required_argument, 0, 'e' },
		{ "quote", no_argument, 0, 'q' },
		{ "dir", required_argument, 0, 'd' },
		{ "print", no_argument, 0, 'p' },
		{ "follow", no_argument, 0, 'L' },
		{ "maxdepth", required_argument, 0, 0 },
		{ "version", no_argument, 0, 'v' },
		{ "help", no_argument, 0, 'h' },
		{ "list-extensions", no_argument, 0, 0 },
		{ 0, 0, 0, 0 }
	};

	int index = 0;
	int offset = 0;
	Action action = ACTION_EXEC;

	assert(argc >= 1);
	assert(argv != NULL);
	assert(opts != NULL);

	memset(opts, 0, sizeof(Options));

	opts->flags = FLAG_STDIN;
	opts->max_depth = -1;

	/* try to handle first two options as path & expression strings */
	if(argc >= 2 && argv[1][0] != '-')
	{
		if(argv[1][0] != '-')
		{
			opts->dir = strdup(argv[1]);
			++offset;
		}
	}

	if(argc >= 3 && argv[2][0] != '-')
	{
		opts->expr = strdup(argv[2]);
		opts->flags &= ~FLAG_STDIN; 
		++offset;
	}

	/* read options */
	while(action != ACTION_ABORT)
	{
		int opt = getopt_long(argc - offset, argv + offset, "e:d:qpvhL", long_options, &index);

		if(opt == -1)
		{
			break;
		}

		switch(opt)
		{
			case 'e':
				if(!opts->expr)
				{
					opts->expr = strdup(optarg);
					opts->flags &= ~FLAG_STDIN; 
				}
				break;

			case 'd':
				if(!opts->dir)
				{
					opts->dir = strdup(optarg);
				}
				break;

			case 'q':
				opts->flags |= FLAG_QUOTE;
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
				opts->follow = true;
				break;

			case 0:
				if(!strcmp(long_options[index].name, "maxdepth"))
				{
					opts->max_depth = atoi(optarg);
				}
				else if(!strcmp(long_options[index].name, "list-extensions"))
				{
					action = ACTION_LIST_EXTENSIONS;
				}

				break;

			default:
				action = ACTION_ABORT;
		}
	}

	return action;
}

static char *
_read_expr_from_stdin(void)
{
	char *expr = (char *)utils_malloc(PARSER_MAX_EXPRESSION_LENGTH);
	size_t bytes = PARSER_MAX_EXPRESSION_LENGTH;
	
	bytes = getline(&expr, &bytes, stdin);

	if(!bytes)
	{
		if(bytes == -1)
		{
			perror("getline()");
		}

		free(expr);
		expr = NULL;
	}
	else
	{
		/* delete newline character */
		expr[bytes - 1] = '\0';
	}

	return expr;
}

static char *
_autodetect_homedir(void)
{
	const char *envpath = getenv("HOME");

	if(envpath)
	{
		return strdup(envpath);
	}

	return NULL;
}

static bool
_dir_is_valid(const char *path)
{
	assert(path != NULL);

	struct stat sb;

	if(stat(path, &sb))
	{
		return false;
	}

	return (sb.st_mode & S_IFMT) == S_IFDIR;
}

static int32_t
_get_translation_flags(const Options *opts)
{
	assert(opts != NULL);

	return (opts->flags & FLAG_QUOTE) ? TRANSLATION_FLAG_QUOTE : TRANSLATION_FLAG_NONE;
}

static void
_build_search_options(const Options *opts, SearchOptions *sopts)
{
	assert(opts != NULL);
	assert(sopts != NULL);

	sopts->max_depth = opts->max_depth;
	sopts->follow = opts->follow;
}

static void
_file_cb(const char *path, void *user_data)
{
	if(path)
	{
		printf("%s\n", path);
	}
}

static void
_error_cb(const char *msg, void *user_data)
{
	if(msg)
	{
		fprintf(stderr, "WARNING: %s\n", msg);
	}
}

static bool
_exec_find(const Options *opts)
{
	SearchOptions sopts;

	assert(opts != NULL);

	_build_search_options(opts, &sopts);

	return search_files_expr(opts->dir, opts->expr, _get_translation_flags(opts), &sopts, _file_cb, _error_cb, NULL) >= 0;
}

static bool
_print_expr(const Options *opts)
{
	SearchOptions sopts;

	assert(opts != NULL);

	_build_search_options(opts, &sopts);

	return search_debug(stdout, stderr, opts->dir, opts->expr, _get_translation_flags(opts), &sopts);
}

static void
_print_help(const char *name)
{
	assert(name != NULL);

	printf("Usage: %s [options]\n", name);
	printf("       %s [path] [options]\n", name);
	printf("       %s [path] [expression] [options]\n\n", name);
	printf("  -e, --expr          expression to evaluate when finding files\n");
	printf("  -q, --quote         quote special characters in translated expression\n");
	printf("  -d, --dir           root directory\n");
	printf("  -L, --follow        follow symbolic links\n");
	printf("  --mapdepth levels   maximum search depth\n");
	printf("  -p, --print         don't search files but print expression to stdout\n");
	printf("  --list-extensions   show a list with installed extensions\n");
	printf("  -v, --version       show version and exit\n");
	printf("  -h, --help          display this help and exit\n");
}

static void
_print_version(const char *name)
{
	printf("%s, version %d.%d.%d\n", name, VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
}

static void
_list_extensions(void)
{
	ExtensionManager *manager;

	manager = extension_manager_new();

	if(manager)
	{
		int count = extension_manager_load_default(manager);

		if(count)
		{
			extension_manager_export(manager, stdout);
		}
		else
		{
			fprintf(stdout, "No extensions loaded.\n");
		}

		extension_manager_destroy(manager);
	}
	else
	{
		fprintf(stderr, "Couldn't load extensions.\n");
	}
}

/**
   @param argc number of arguments
   @param argv argument vector
   @return EXIT_SUCCESS on success

   efind's entry point.
 */
int
main(int argc, char *argv[])
{
	Action action;
	Options opts;
	int result = EXIT_FAILURE;

	/* read command line options */
	action = _read_options(argc, argv, &opts);

	if(action == ACTION_ABORT)
	{
		printf("Try '%s --help' for more information.\n", argv[0]);
		goto out;
	}
		
	if(action == ACTION_EXEC || action == ACTION_PRINT)
	{
		/* autodetect home directory if path isn't specified */
		if(!opts.dir)
		{
			opts.dir = _autodetect_homedir();
		}

		/* test if directory is valid */
		if(!_dir_is_valid(opts.dir))
		{
			fprintf(stderr, "The specified directory is invalid.\n");
			goto out;
		}

		/* read expression from stdin */
		if(opts.flags & FLAG_STDIN)
		{
			opts.expr = _read_expr_from_stdin();
		}

		/* test if expression is empty */
		if(!opts.expr || !opts.expr[0])
		{
			fprintf(stderr, "Expression cannot be empty.\n");
			goto out;
		}
	}

	/* start desired action */
	switch(action)
	{
		case ACTION_EXEC:
			if(_exec_find(&opts))
			{
				result = EXIT_SUCCESS;
			}
			break;

		case ACTION_PRINT:
			if(_print_expr(&opts))
			{
				result = EXIT_SUCCESS;
			}
			break;

		case ACTION_PRINT_HELP:
			_print_help(argv[0]);
			result = EXIT_SUCCESS;
			break;

		case ACTION_PRINT_VERSION:
			_print_version(argv[0]);
			result = EXIT_SUCCESS;
			break;

		case ACTION_LIST_EXTENSIONS:
			_list_extensions();
			result = EXIT_SUCCESS;
			break;

		default:
			result = EXIT_FAILURE;
	}

	out:
		/* cleanup */
		if(opts.expr)
		{
			free(opts.expr);
		}

		if(opts.dir)
		{
			free(opts.dir);
		}

	return result;
}

