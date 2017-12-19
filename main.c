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
#include "format-fields.h"
#include "gettext.h"
#include "version.h"

/*! @cond INTERNAL */
typedef enum
{
	ACTION_ABORT,
	ACTION_EXEC,
	ACTION_PRINT,
	ACTION_PRINT_HELP,
	ACTION_PRINT_VERSION,
	ACTION_PRINT_EXTENSIONS,
	ACTION_PRINT_BLACKLIST
} Action;

typedef enum
{
	FLAG_NONE   = 0,
	FLAG_STDIN  = 1,
	FLAG_QUOTE  = 2
} Flags;

typedef struct
{
	LogLevel log_level;
	bool log_color;
	int32_t flags;
	char *expr;
	SList dirs;
	int32_t max_depth;
	bool follow;
	char *regex_type;
	char *printf;
	char *orderby;
} Options;

typedef struct
{
	const char *dir;
	FormatParserResult *fmt;
	FileList *files;
} FoundArg;
/*! @endcond */

static void
_print_blacklist(void)
{
	Blacklist *blacklist;

	blacklist = blacklist_new();
	blacklist_load_default(blacklist);

	ListItem *iter = blacklist_head(blacklist);

	while(iter)
	{
		printf("%s\n", (char *)list_item_get_data(iter));
		iter = list_item_next(iter);
	}

	blacklist_destroy(blacklist);
}

static void
_print_extensions(void)
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
_print_version(const char *name)
{
	assert(name != NULL);

	printf(_("%s %d.%d.%d (%s)\nWebsite: %s\n(C) %s %s <%s>\nThis program is released under the terms of the %s (%s)\n"),
	       name, EFIND_VERSION_MAJOR, EFIND_VERSION_MINOR, EFIND_VERSION_PATCH, EFIND_VERSION_CODE_NAME,
	       EFIND_WEBSITE,
	       EFIND_COPYRIGHT_DATE, EFIND_AUTHOR_NAME, EFIND_AUTHOR_EMAIL,
	       EFIND_LICENSE_NAME, EFIND_LICENSE_URL);
}

static void
_print_help(const char *name)
{
	assert(name != NULL);

	printf(_("Usage: %s [options]\n"), name);
	printf(_("       %s [starting-points] [options]\n"), name);
	printf(_("       %s [starting-points] [expression] [options]\n\n"), name);
	printf(_("  -e, --expr          expression to evaluate when finding files\n"));
	printf(_("  -q, --quote         quote special characters in translated expression\n"));
	printf(_("  -d, --dir           directory to search (multiple directories are possible)\n"));
	printf(_("  -L, --follow        follow symbolic links\n"));
	printf(_("  --regex-type type   set regular expression type; see manpage\n"));
	printf(_("  --printf format     print format on standard output; see manpage\n"));
	printf(_("  --order-by fields   fields to order search result by; see manpage\n"));
	printf(_("  --max-depth levels  maximum search depth\n"));
	printf(_("  -p, --print         don't search files but print expression to stdout\n"));
	printf(_("  --print-extensions  print a list of installed extensions\n"));
	printf(_("  --print-blacklist   print blacklisted extensions\n"));
	printf(_("  --log-level level   set the log level (0-6)\n"));
	printf(_("  --disable-log-color disable colored log messages\n"));
	printf(_("  -v, --version       print version and exit\n"));
	printf(_("  -h, --help          display this help and exit\n"));
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

static int32_t
_get_translation_flags(const Options *opts)
{
	assert(opts != NULL);

	return (opts->flags & FLAG_QUOTE) ? TRANSLATION_FLAG_QUOTE : TRANSLATION_FLAG_NONE;
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
_file_cb(const char *path, void *user_data)
{
	FoundArg *arg = (FoundArg *)user_data;

	assert(path != NULL);
	assert(arg != NULL);

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

	assert(path != NULL);
	assert(arg != NULL);
	assert(arg->files != NULL);
	assert(arg->dir != NULL);

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

static void
_sort_and_print_files(FoundArg *arg)
{
	assert(arg != NULL);
	assert(arg->files != NULL);

	file_list_sort(arg->files);

	for(size_t i = 0; i < file_list_count(arg->files); i++)
	{
		FileListEntry *entry = file_list_at(arg->files, i);

		arg->dir = entry->info->cli;
		_file_cb(entry->info->path, arg);
	}
}

static bool
_search_dirs(const Options *opts, SearchOptions *sopts, Callback cb, FoundArg *arg)
{
	SListItem *item;
	bool success = true;

	item = slist_head(&opts->dirs);

	while(item && success)
	{
		const char *path = (const char *)slist_item_get_data(item);

		assert(path != NULL);

		TRACEF("startup", "Searching directory: \"%s\"", path);

		arg->dir = path;

		if(path)
		{
			success = search_files(path, opts->expr, _get_translation_flags(opts), sopts, cb, _error_cb, arg) >= 0;
		}
		else
		{
			success = false;
		}

		item = slist_item_next(item);
	}

	return success;
}

static bool
_parse_printf_arg(const Options *opts, FoundArg *arg)
{
	bool success = true;

	if(opts->printf)
	{
		DEBUGF("action", "Parsing format string: %s", opts->printf);

		arg->fmt = format_parse(opts->printf);

		if(!arg->fmt->success)
		{
			DEBUG("action", "Parsing of format string failed.");

			fprintf(stderr, _("Couldn't parse format string: %s\n"), opts->printf);
			success = false;
		}
	}

	return success;
}

static bool
_parse_orderby_arg(const Options *opts, FoundArg *arg)
{
	bool success = true;

	if(opts->orderby)
	{
		DEBUGF("action", "Preparing sort string: %s", opts->orderby);

		char *orderby = format_substitute(opts->orderby);

		DEBUGF("action", "Testing sort string: %s", orderby);

		if(sort_string_test(orderby) == -1)
		{
			DEBUG("action", "Parsing of sort string failed.");

			fprintf(stderr, _("Couldn't parse sort string.\n"));
			success = false;
		}
		else
		{
			arg->files = utils_new(1, FileList);
			file_list_init(arg->files, orderby);
		}

		free(orderby);
	}

	return success;
}

/*! @cond INTERNAL */
#define COLLECT_AND_SORT_FILES(arg) (arg.files != NULL)
/*! @endcond */

static bool
_exec_find(const Options *opts)
{
	SearchOptions sopts;
	FoundArg arg;
	Callback cb = _file_cb;
	bool success = false;

	assert(opts != NULL);
	assert(opts->expr != NULL);

	TRACE("action", "Preparing file search.");

	memset(&arg, 0, sizeof(FoundArg));

	if(_parse_printf_arg(opts, &arg) && _parse_orderby_arg(opts, &arg))
	{
		TRACE("startup", "Starting file search.");

		if(COLLECT_AND_SORT_FILES(arg))
		{
			cb = _collect_cb;
		}

		_build_search_options(opts, &sopts);

		success = _search_dirs(opts, &sopts, cb, &arg);

		if(success && COLLECT_AND_SORT_FILES(arg))
		{
			_sort_and_print_files(&arg);
		}

		TRACE("action", "Cleaning up file search.");

		search_options_free(&sopts);
	}

	TRACE("action", "Cleaning up format parser.");

	if(arg.fmt)
	{
		format_parser_result_free(arg.fmt);
	}

	TRACE("action", "Cleaning up file list & sort options.");

	if(arg.files)
	{
		file_list_free(arg.files);
		free(arg.files);
	}

	DEBUGF("action", "Action %#x finished with result=%d.", ACTION_EXEC, success);

	return success;
}

static int
_run_action(Action action, char argc, char **argv, const Options *opts)
{
	int result = EXIT_FAILURE;

	DEBUGF("startup", "Running action: %#x", action);

	switch(action)
	{
		case ACTION_EXEC:
			if(_exec_find(opts))
			{
				result = EXIT_SUCCESS;
			}
			break;

		case ACTION_PRINT:
			if(_print_expr(opts))
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

		case ACTION_PRINT_EXTENSIONS:
			_print_extensions();
			result = EXIT_SUCCESS;
			break;

		case ACTION_PRINT_BLACKLIST:
			_print_blacklist();
			result = EXIT_SUCCESS;
			break;

		default:
			result = EXIT_FAILURE;
	}

	return result;
}

static bool
_test_expr_is_not_empty(Options *opts)
{
	bool success = true;

	if(!opts->expr || !opts->expr[0])
	{
		fprintf(stderr, _("Expression cannot be empty.\n"));
		success = false;
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

	TRACE("startup", "Validating starting-points.");

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

static bool
_search_dirs_are_empty(const Options *opts)
{
	assert(opts != NULL);

	return slist_count(&opts->dirs) == 0;
}

static bool
_test_search_dirs_are_valid(Options *opts)
{
	bool success = true;

	assert(opts != NULL);

	const char *path = _find_invalid_search_dir(opts);

	if(path)
	{
		fprintf(stderr, _("The specified directory is invalid: %s\n"), path);
		success = false;
	}

	return success;
}

static bool
_validate_options(Options *opts)
{
	assert(opts != NULL);

	TRACE("startup", "Testing required options.");

	return !_search_dirs_are_empty(opts) && _test_search_dirs_are_valid(opts) && _test_expr_is_not_empty(opts);
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
_append_missing_homedir(Options *opts)
{
	bool success = true;

	assert(opts != NULL);

	if(_search_dirs_are_empty(opts))
	{
		DEBUG("startup", "No directory specified, running auto-detection.");

		if(!_append_homedir(opts))
		{
			fprintf(stderr, _("Couldn't detect home directory.\n"));
			success = false;
		}
	}

	return success;
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

static void
_read_missing_expr_from_stdin(Options *opts)
{
	assert(opts != NULL);

	if(opts->flags & FLAG_STDIN)
	{
		DEBUG("startup", "No expression specified, reading from standard input.");

		opts->expr = _read_expr_from_stdin();
	}
}

static bool
_prepare_processing(Options *opts)
{
	bool success = false;

	_read_missing_expr_from_stdin(opts);

	if(_append_missing_homedir(opts))
	{
		success = _validate_options(opts);
	}

	return success;
}

static void
_append_single_search_dir(char *argv[], Options *opts)
{
	assert(argv != NULL);
	assert(opts != NULL);

	if(!slist_contains(&opts->dirs, argv[1]))
	{
		slist_append(&opts->dirs, utils_strdup(argv[1]));
	}
}

static void
_append_multiple_search_dirs(char *argv[], int offset, Options *opts)
{
	int limit = offset;

	assert(argv != NULL);
	assert(opts != NULL);

	/* handle all arguments as directories if expression is already set */
	if(opts->expr)
	{
		limit++;
	}
	else
	{
		opts->expr = utils_strdup(argv[offset]);
		opts->flags &= ~FLAG_STDIN; 
	}

	for(int i = 1; i < limit; i++)
	{
		if(!slist_contains(&opts->dirs, argv[i]))
		{
			slist_append(&opts->dirs, utils_strdup(argv[i]));
		}
	}
}

static void
_append_search_dirs_and_expr_from_argv(char *argv[], int offset, Options *opts)
{
	assert(argv != NULL);
	assert(opts != NULL);

	if(offset)
	{
		if(offset == 1)
		{
			_append_single_search_dir(argv, opts);
		}
		else
		{
			_append_multiple_search_dirs(argv, offset, opts);
		}
	}
}

static Action
_get_opt(int argc, char *argv[], int offset, Options *opts)
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
		{ "print-extensions", no_argument, 0, 0 },
		{ "print-blacklist", no_argument, 0, 0 },
		{ "log-level", required_argument, 0, 0 },
		{ "disable-log-color", no_argument, 0, 0 },
		{ "version", no_argument, 0, 'v' },
		{ "help", no_argument, 0, 'h' },
		{ 0, 0, 0, 0 }
	};

	Action action = ACTION_EXEC;
	int index = 0;

	assert(argc >= 1);
	assert(argv != NULL);
	assert(opts != NULL);

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
				opts->expr = utils_strdup(optarg);
				opts->flags &= ~FLAG_STDIN; 
				break;

			case 'd':
				if(!slist_contains(&opts->dirs, argv[1]))
				{
					slist_append(&opts->dirs, utils_strdup(optarg));
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
				if(!strcmp(long_options[index].name, "log-level"))
				{
					opts->log_level = atoi(optarg);
				}
				else if(!strcmp(long_options[index].name, "disable-log-color"))
				{
					opts->log_color = false;
				}
				else if(!strcmp(long_options[index].name, "max-depth"))
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

static int
_index_of_first_option(int argc, char *argv[])
{
	int index = 0;

	assert(argc >= 1);
	assert(argv != NULL);

	for(int i = 1; i < argc; i++)
	{
		if(*argv[i] != '-')
		{
			index++;
		}
		else
		{
			break;
		}
	}

	return index;
}

static Action
_read_options(int argc, char *argv[], Options *opts)
{
	assert(argc >= 1);
	assert(argv != NULL);
	assert(opts != NULL);

	opts->flags = FLAG_STDIN;
	opts->max_depth = -1;
	opts->log_color = true;

	int offset = _index_of_first_option(argc, argv);
	Action action = _get_opt(argc, argv, offset, opts);

	if(action != ACTION_ABORT)
	{
		_append_search_dirs_and_expr_from_argv(argv, offset, opts);
	}

	return action;
}

static void
_options_init(Options *opts)
{
	assert(opts != NULL);

	memset(opts, 0, sizeof(Options));
	slist_init(&opts->dirs, str_compare, &free, NULL);
}

static void
_options_free(Options *opts)
{
	assert(opts != NULL);

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

static void
_set_log_options(const Options *opts)
{
	assert(opts != NULL);

	if(opts->log_level)
	{
		log_set_verbosity(opts->log_level);
	}

	log_enable_color(opts->log_color);
}

static void
_set_locale(void)
{
	setlocale(LC_ALL, "");
	gettext_init();
}

/*! @cond INTERNAL */
#define ACTION_IS_PROCESSING_EXPRESSION(action) action == ACTION_EXEC || action == ACTION_PRINT
/*! @endcond */

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

	_set_locale();
	_options_init(&opts);

	action = _read_options(argc, argv, &opts);

	if(action != ACTION_ABORT)
	{
		_set_log_options(&opts);

		INFOF("startup", "%s started successfully.", *argv);

		bool run = true;

		if(ACTION_IS_PROCESSING_EXPRESSION(action))
		{
			run = _prepare_processing(&opts);
		}

		if(run)
		{
			result = _run_action(action, argc, argv, &opts);
		}
	}
	else
	{ 
		printf(_("Try '%s --help' for more information.\n"), *argv);
	}

	INFO("shutdown", "Cleaning up.");

	_options_free(&opts);

	return result;
}

