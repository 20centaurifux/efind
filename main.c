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
#include <datatypes.h>

#include "log.h"
#include "search.h"
#include "parser.h"
#include "utils.h"
#include "format.h"
#include "extension.h"
#include "blacklist.h"
#include "filelist.h"
#include "gettext.h"
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
	ACTION_LIST_EXTENSIONS,
	/*! Show blacklisted files and quit. */
	ACTION_SHOW_BLACKLIST
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
	/*! Log level. */
	LogLevel log_level;
	/*! Enable colored log messages. */
	bool log_color;
	/*! Flags. */
	int32_t flags;
	/*! Expression to translate. */
	char *expr;
	/*! Directories to search. */
	SList dirs;
	/*! Directory search level limitation. */
	int32_t max_depth;
	/*! Dereference symbolic links. */
	bool follow;
	/*! Regular expression type. */
	char *regex_type;
	/*! Print format on stdout. */
	char *printf;
	/*! Sort string. */
	char *orderby;
} Options;

/**
   @struct FoundArg
   @brief Additional callback arguments.
 */
typedef struct
{
	/*! Starting point of the search. */
	const char *dir;
	/*! Optional parsed format string. */
	FormatParserResult *fmt;
	/*! Store and sort found files. */
	FileList *files;
} FoundArg;

static void
_options_init(Options *opts)
{
	memset(opts, 0, sizeof(Options));
	slist_init(&opts->dirs, str_compare, &free, NULL);
}

static void
_options_free(Options *opts)
{
	slist_free(&opts->dirs);

	if(opts->expr)
	{
		free(opts->expr);
	}

	if(opts->regex_type)
	{
		free(opts->regex_type);
	}

	if(opts->printf)
	{
		free(opts->printf);
	}

	if(opts->orderby)
	{
		free(opts->orderby);
	}
}

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
		{ "max-depth", required_argument, 0, 0 },
		{ "regex-type", required_argument, 0, 0 },
		{ "printf", required_argument, 0, 0 },
		{ "order-by", required_argument, 0, 0 },
		{ "list-extensions", no_argument, 0, 0 },
		{ "show-blacklist", no_argument, 0, 0 },
		{ "log-level", required_argument, 0, 0 },
		{ "enable-log-color", no_argument, 0, 0 },
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

	opts->flags = FLAG_STDIN;
	opts->max_depth = -1;

	/* try to handle first two options as path & expression strings */
	/*
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
	*/

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
				slist_append(&opts->dirs, utils_strdup(optarg));
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
				if(!strcmp(long_options[index].name, "log-level"))
				{
					opts->log_level = atoi(optarg);
				}
				else if(!strcmp(long_options[index].name, "enable-log-color"))
				{
					opts->log_color = true;
				}
				else if(!strcmp(long_options[index].name, "max-depth"))
				{
					opts->max_depth = atoi(optarg);
				}
				else if(!strcmp(long_options[index].name, "list-extensions"))
				{
					action = ACTION_LIST_EXTENSIONS;
				}
				else if(!strcmp(long_options[index].name, "show-blacklist"))
				{
					action = ACTION_SHOW_BLACKLIST;
				}
				else if(!strcmp(long_options[index].name, "regex-type"))
				{
					opts->regex_type = utils_strdup(optarg);
				}
				else if(!strcmp(long_options[index].name, "printf"))
				{
					opts->printf = utils_strdup(optarg);
				}
				else if(!strcmp(long_options[index].name, "order-by"))
				{
					opts->orderby = utils_strdup(optarg);
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
	ssize_t bytes;
	size_t read = PARSER_MAX_EXPRESSION_LENGTH;
	
	bytes = getline(&expr, &read, stdin);

	TRACEF("startup", "getline() called: result=%ld, buffer size=%ld", bytes, read);

	if(bytes <= 0)
	{
		FATALF("startup", "getline() failed with result %ld.", bytes);

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

static bool
_append_homedir(Options *opts)
{
	const char *envpath;
	bool success = false;

	assert(opts != NULL);

	TRACE("startup", "Auto-detecting home directory.");

	envpath = getenv("HOME");

	if(envpath)
	{
		TRACEF("startup", "Found directory: %s", envpath);
		slist_append(&opts->dirs, utils_strdup(envpath));
		success = true;
	}
	else
	{
		FATAL("startup", "HOME environment variable not set.");
	}

	return success;
}

static bool
_dir_is_valid(const char *path)
{
	struct stat sb;

	assert(path != NULL);

	TRACEF("startup", "Testing directory: %s", path);

	if(stat(path, &sb))
	{
		return false;
	}

	TRACEF("startup", "sb.st_mode: %#x", sb.st_mode);

	return (sb.st_mode & S_IFMT) == S_IFDIR;
}

static const char *
_find_invalid_search_dir(Options *opts)
{
	SListItem *item;
	const char *dir = NULL;

	assert(opts != NULL);

	TRACE("startup", "Validating search directories.");

	item = slist_head(&opts->dirs);

	while(item && !dir)
	{
		char *path = (char *)slist_item_get_data(item);

		if(!_dir_is_valid(path))
		{
			dir = path;
		}

		item = slist_item_next(item);
	}

	return dir;
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
	FoundArg *arg = (FoundArg *)user_data;

	if(path)
	{
		if(arg->fmt)
		{
			assert(arg->dir != NULL);
			assert(arg->fmt->success == true);

			format_write(arg->fmt, arg->dir, path, stdout);
		}
		else
		{
			printf("%s\n", path);
		}
	}
}

static void
_collect_cb(const char *path, void *user_data)
{
	FoundArg *arg = (FoundArg *)user_data;

	file_list_append(arg->files, arg->dir, path);
}

static void
_error_cb(const char *msg, void *user_data)
{
	if(msg)
	{
		fputs(msg, stderr);
		fputc('\n', stderr);
	}
}

static bool
_exec_find(const Options *opts)
{
	SearchOptions sopts;
	FoundArg arg;
	Callback cb = _file_cb;
	SListItem *item;
	bool success = false;

	assert(opts != NULL);
	assert(opts->expr != NULL);

	TRACE("action", "Preparing file search.");

	memset(&arg, 0, sizeof(FoundArg));

	/* parse printf format string */
	if(opts->printf)
	{
		DEBUGF("action", "Parsing format string: %s", opts->printf);

		arg.fmt = format_parse(opts->printf);

		if(!arg.fmt->success)
		{
			DEBUG("action", "Parsing of format string failed.");
			fprintf(stderr, _("Couldn't parse format string: %s\n"), opts->printf);
			goto out;
		}
	}

	/* test sort string */
	if(opts->orderby)
	{
		DEBUGF("action", "Testing sort string: %s", opts->orderby);

		if(sort_string_test(opts->orderby) == -1)
		{
			DEBUG("action", "Parsing of sort string failed.");
			fprintf(stderr, _("Couldn't parse sort string.\n"));
			goto out;
		}
		else
		{
			arg.files = utils_new(1, FileList);

			file_list_init(arg.files, opts->orderby);

			cb = _collect_cb;
		}
	}

	/* search files */
	TRACE("startup", "Starting file search.");

	_build_search_options(opts, &sopts);

	item = slist_head(&opts->dirs);
	success = true;

	while(item && success)
	{
		const char *path = (const char *)slist_item_get_data(item);

		assert(path != NULL);

		TRACEF("startup", "Searching directory: \"%s\"", path);

		arg.dir = path;

		if(path)
		{
			success = search_files_expr(path, opts->expr, _get_translation_flags(opts), &sopts, cb, _error_cb, &arg) >= 0;
		}
		else
		{
			success = false;
		}

		item = slist_item_next(item);
	}

	/* sort files before printing search result */
	if(success && arg.files)
	{
		file_list_sort(arg.files);

		for(size_t i = 0; i < file_list_count(arg.files); ++i)
		{
			FileListEntry *entry = file_list_at(arg.files, i);

			arg.dir = entry->info->cli;
			_file_cb(entry->info->path, &arg);
		}
	}

	TRACE("action", "Cleaning up file search.");

	/* cleanup search options */
	search_options_free(&sopts);

out:
	/* cleanup */
	TRACE("action", "Cleaning up format parser.");

	if(arg.fmt)
	{
		format_parser_result_free(arg.fmt);
	}

	TRACE("action", "Cleaning up file list.");

	if(arg.files)
	{
		file_list_free(arg.files);
		free(arg.files);
	}

	DEBUGF("action", "Action %#x finished with result=%d.", ACTION_EXEC, success);

	return success;
}

static bool
_print_expr(const Options *opts)
{
	SearchOptions sopts;
	SListItem *item;
	bool success = true;

	assert(opts != NULL);
	assert(opts->expr != NULL);

	_build_search_options(opts, &sopts);

	item = slist_head(&opts->dirs);

	while(item && success)
	{
		const char *path = (const char *)slist_item_get_data(item);

		assert(path != NULL);
	
		if(path)
		{
			success = search_debug(stdout, stderr, path, opts->expr, _get_translation_flags(opts), &sopts);
		}
		else
		{
			success = false;
		}

		item = slist_item_next(item);
	}

	search_options_free(&sopts);

	DEBUGF("action", "Action %#x finished with result %d.", ACTION_PRINT, success);

	return success;
}

static void
_print_help(const char *name)
{
	assert(name != NULL);

	printf(_("Usage: %s [options]\n"), name);
	printf(_("       %s [path] [options]\n"), name);
	printf(_("       %s [path] [expression] [options]\n\n"), name);
	printf(_("  -e, --expr          expression to evaluate when finding files\n"));
	printf(_("  -q, --quote         quote special characters in translated expression\n"));
	printf(_("  -d, --dir           root directory\n"));
	printf(_("  -L, --follow        follow symbolic links\n"));
	printf(_("  --regex-type type   set regular expression type; see manpage\n"));
	printf(_("  --printf format     print format on standard output; see manpage\n"));
	printf(_("  --order-by fields   fields to order search result by; see manpage\n"));
	printf(_("  --max-depth levels  maximum search depth\n"));
	printf(_("  -p, --print         don't search files but print expression to stdout\n"));
	printf(_("  --list-extensions   show a list of installed extensions\n"));
	printf(_("  --show-blacklist    show blacklisted extensions\n"));
	printf(_("  --log-level level   set the log level (0-6)\n"));
	printf(_("  --enable-log-color  enable colored log messages\n"));
	printf(_("  -v, --version       show version and exit\n"));
	printf(_("  -h, --help          display this help and exit\n"));
}

static void
_print_version(const char *name)
{
	printf(_("%s %d.%d.%d (%s)\nWebsite: %s\n(C) %s %s <%s>\nThis program is released under the terms of the %s (%s)\n"),
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
			printf(_("No extensions loaded.\n"));
		}

		extension_manager_destroy(manager);
	}
	else
	{
		FATAL("action", "Creation of ExpressionManager instance failed.");
		fprintf(stderr, _("Couldn't load extensions.\n"));
	}
}

static void
_show_blacklist(void)
{
	Blacklist *blacklist;

	blacklist = blacklist_new();
	blacklist_load_default(blacklist);

	ListItem *iter = blacklist_head(blacklist);

	while(iter)
	{
		fprintf(stdout, "%s\n", (char *)list_item_get_data(iter));
		iter = list_item_next(iter);
	}

	blacklist_destroy(blacklist);
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

	/* initialize gettext */
	gettext_init();

	/* read command line options */
	_options_init(&opts);
	action = _read_options(argc, argv, &opts);

	if(action == ACTION_ABORT)
	{
		printf(_("Try '%s --help' for more information.\n"), argv[0]);
		goto out;
	}

	/* set verbosity */
	if(opts.log_level)
	{
		log_set_verbosity(opts.log_level);
	}

	log_enable_color(opts.log_color);

	INFOF("startup", "%s started successfully.", *argv);

	/* validate options */
	if(action == ACTION_EXEC || action == ACTION_PRINT)
	{
		TRACE("startup", "Testing required options.");

		/* autodetect home directory if no path has been specified */
		if(!slist_count(&opts.dirs))
		{
			DEBUG("startup", "No directory specified, running auto-detection.");

			if(!_append_homedir(&opts))
			{
				fprintf(stderr, _("Couldn't detect home directory.\n"));
				goto out;
			}
		}

		/* test if search paths are valid */
		const char *path = _find_invalid_search_dir(&opts);

		if(path)
		{
			fprintf(stderr, _("The specified directory is invalid: \"%s\"\n"), path);
			goto out;
		}

		/* read expression from stdin */
		if(opts.flags & FLAG_STDIN)
		{
			DEBUG("startup", "No expression specified, reading from standard input.");
			opts.expr = _read_expr_from_stdin();
		}

		/* test if expression is empty */
		if(!opts.expr || !opts.expr[0])
		{
			fprintf(stderr, _("Expression cannot be empty.\n"));
			goto out;
		}
	}

	/* start desired action */
	DEBUGF("startup", "Running action: %#x", action);

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
			_print_help(*argv);
			result = EXIT_SUCCESS;
			break;

		case ACTION_PRINT_VERSION:
			_print_version(*argv);
			result = EXIT_SUCCESS;
			break;

		case ACTION_LIST_EXTENSIONS:
			_list_extensions();
			result = EXIT_SUCCESS;
			break;

		case ACTION_SHOW_BLACKLIST:
			_show_blacklist();
			result = EXIT_SUCCESS;
			break;

		default:
			result = EXIT_FAILURE;
	}

	out:
		/* cleanup */
		INFO("shutdown", "Cleaning up.");
		_options_free(&opts);

	return result;
}

