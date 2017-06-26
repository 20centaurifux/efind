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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <locale.h>
#include <assert.h>

#include "search.h"
#include "parser.h"
#include "utils.h"
#include "format.h"
#include "extension.h"
#include "version.h"

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
	/*! Regular expression type. */
	char *regex_type;
	/*! Print format on stdout. */
	char *printf;
} Options;

/**
   @struct PrintArg
   @brief Additional print arguments.
 */
typedef struct
{
	/*! Starting point of the search. */
	const char *dir;
	/*! Optional parsed format string. */
	FormatParserResult *fmt;
	/*! FileInfo instance used to receive file attributes when printing. */
	FileInfo info;
} PrintArg;

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
		{ "regex-type", required_argument, 0, 0 },
		{ "printf", required_argument, 0, 0 },
		{ "list-extensions", no_argument, 0, 0 },
		{ "version", no_argument, 0, 'v' },
		{ "help", no_argument, 0, 'h' },
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
			opts->dir = utils_strdup(argv[1]);
			++offset;
		}
	}

	if(argc >= 3 && argv[2][0] != '-')
	{
		opts->expr = utils_strdup(argv[2]);
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
					opts->expr = utils_strdup(optarg);
					opts->flags &= ~FLAG_STDIN; 
				}
				break;

			case 'd':
				if(!opts->dir)
				{
					opts->dir = utils_strdup(optarg);
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
				else if(!strcmp(long_options[index].name, "regex-type"))
				{
					opts->regex_type = utils_strdup(optarg);
				}
				else if(!strcmp(long_options[index].name, "printf"))
				{
					opts->printf = utils_strdup(optarg);
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
	size_t read = PARSER_MAX_EXPRESSION_LENGTH;
	ssize_t bytes;
	
	bytes = getline(&expr, &read, stdin);

	if(bytes <= 0)
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
		return utils_strdup(envpath);
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

	memset(sopts, 0, sizeof(SearchOptions));

	sopts->max_depth = opts->max_depth;
	sopts->follow = opts->follow;

	if(opts->regex_type)
	{
		sopts->regex_type = utils_strdup(opts->regex_type);
	}
}

static void
_file_cb(const char *path, void *user_data)
{
	PrintArg *arg = (PrintArg *)user_data;

	if(path)
	{
		if(arg->fmt)
		{
			assert(arg->dir != NULL);
			assert(arg->fmt->success == true);

			format_write(arg->fmt, &arg->info, arg->dir, path, stdout);
		}
		else
		{
			printf("%s\n", path);
		}
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
	FormatParserResult *fmt = NULL;
	SearchOptions sopts;
	int result = -1;

	assert(opts != NULL);

	if(opts->printf)
	{
		fmt = format_parse(opts->printf);
	}

	if(!fmt || fmt->success)
	{
		PrintArg arg;

		arg.dir = opts->dir;
		arg.fmt = fmt;

		if(arg.fmt)
		{
			file_info_init(&arg.info);
		}

		_build_search_options(opts, &sopts);

		result = search_files_expr(opts->dir, opts->expr, _get_translation_flags(opts), &sopts, _file_cb, _error_cb, &arg) >= 0;

		search_options_free(&sopts);

		if(arg.fmt)
		{
			file_info_clear(&arg.info);
		}
	}
	else
	{
		fprintf(stderr, "Couldn't parse format string: \"%s\"\n", opts->printf);
	}

	if(fmt)
	{
		format_parser_result_free(fmt);
	}

	return result;
}

static bool
_print_expr(const Options *opts)
{
	SearchOptions sopts;
	int result;

	assert(opts != NULL);

	_build_search_options(opts, &sopts);

	result = search_debug(stdout, stderr, opts->dir, opts->expr, _get_translation_flags(opts), &sopts);
	search_options_free(&sopts);

	return result;
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
	printf("  --regex-type type   set regular expression type; see manpage\n");
	printf("  --printf format     print format on standard output; see manpage\n");
	printf("  --maxdepth levels   maximum search depth\n");
	printf("  -p, --print         don't search files but print expression to stdout\n");
	printf("  --list-extensions   show a list with installed extensions\n");
	printf("  -v, --version       show version and exit\n");
	printf("  -h, --help          display this help and exit\n");
}

static void
_print_version(const char *name)
{
	printf("%s %d.%d.%d (%s)\nWebsite: %s\n(C) %s %s <%s>\nThis program is released under the terms of the %s (%s)\n",
	       name, EFIND_VERSION_MAJOR, EFIND_VERSION_MINOR, EFIND_VERSION_PATCH, EFIND_VERSION_CODE_NAME,
	       EFIND_WEBSITE,
	       EFIND_COPYRIGHT_DATE, EFIND_AUTHOR_NAME, EFIND_AUTHOR_EMAIL,
	       EFIND_LICENSE_NAME, EFIND_LICENSE_URL);
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

	/* set locale */
	setlocale(LC_ALL, "");

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

		if(opts.regex_type)
		{
			free(opts.regex_type);
		}

		if(opts.printf)
		{
			free(opts.printf);
		}

	return result;
}

