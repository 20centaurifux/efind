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

#include "efind.h"
#include "log.h"
#include "search.h"
#include "parser.h"
#include "utils.h"
#include "extension.h"
#include "blacklist.h"
#include "gettext.h"
#include "processor.h"
#include "range.h"
#include "print.h"
#include "sort.h"

/*! @cond INTERNAL */
typedef struct
{
	const char *dir;
	ProcessorChain *chain;
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
		FATAL("action", "Creation of ExtensionManager instance failed.");
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
	printf(_("  -e, --expr expression  expression to evaluate when finding files\n"));
	printf(_("  -q, --quote <yes|no>   quote special characters in translated expression\n"));
	printf(_("  -d, --dir path         directory to search (multiple directories are possible)\n"));
	printf(_("  -L, --follow <yes|no>  follow symbolic links\n"));
	printf(_("  --regex-type type      set regular expression type; see manpage\n"));
	printf(_("  --printf format        print format on standard output; see manpage\n"));
	printf(_("  --order-by fields      fields to order search result by; see manpage\n"));
	printf(_("  --max-depth levels     maximum search depth\n"));
	printf(_("  --skip number          number of files to skip before printing\n"));
	printf(_("  --limit number         maximum number of files to print\n"));
	printf(_("  -p, --print            don't search files but print expression to stdout\n"));
	printf(_("  --print-extensions     print a list of installed extensions\n"));
	printf(_("  --print-blacklist      print blacklisted extensions\n"));
	printf(_("  --log-level level      set the log level (0-6)\n"));
	printf(_("  --log-color <yes|no>   enable/disable colored log messages\n"));
	printf(_("  -v, --version          print version and exit\n"));
	printf(_("  -h, --help             display this help and exit\n"));
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

static bool
_file_cb(const char *path, void *user_data)
{
	FoundArg *arg = (FoundArg *)user_data;

	assert(path != NULL);
	assert(arg != NULL);
	assert(arg->chain != NULL);
	assert(arg->dir != NULL);

	return processor_chain_write(arg->chain, arg->dir, path);
}

static bool
_error_cb(const char *msg, void *user_data)
{
	if(msg)
	{
		fputs(msg, stderr);
		fputc('\n', stderr);
	}

	return false;
}

static bool
_search_dirs(const Options *opts, SearchOptions *sopts, Callback cb, ProcessorChain *chain)
{
	SListItem *item;
	FoundArg arg;
	bool success = true;

	assert(opts != NULL);
	assert(sopts != NULL);
	assert(cb != NULL);
	assert(chain != NULL);

	arg.chain = chain;
	arg.dir = NULL;

	item = slist_head(&opts->dirs);

	while(item && success)
	{
		const char *path = (const char *)slist_item_get_data(item);

		assert(path != NULL);

		TRACEF("action", "Searching directory: \"%s\"", path);

		arg.dir = path;

		if(path)
		{
			success = search_files(path, opts->expr, _get_translation_flags(opts), sopts, cb, _error_cb, &arg) >= 0;
		}
		else
		{
			success = false;
		}

		item = slist_item_next(item);
	}

	if(success && arg.dir)
	{
		processor_chain_complete(chain, arg.dir);
	}

	return success;
}

static void
_prepend_sort_processor(ProcessorChainBuilder *builder)
{
	assert(builder != NULL);
	assert(builder->user_data != NULL);

	const Options *opts = (Options *)builder->user_data;

	if(opts->orderby)
	{
		TRACE("action", "Prepending sort processor.");

		Processor *sort = sort_processor_new(opts->orderby);

		if(!processor_chain_builder_try_prepend(builder, sort))
		{
			fprintf(stderr, _("Couldn't parse sort string.\n"));
		}
	}
}

static void
_prepend_skip_processor(ProcessorChainBuilder *builder)
{
	assert(builder != NULL);
	assert(builder->user_data != NULL);

	const Options *opts = (Options *)builder->user_data;

	if(opts->skip > 0)
	{
		TRACE("action", "Prepending skip processor.");

		Processor *skip = skip_processor_new(opts->skip);

		processor_chain_builder_try_prepend(builder, skip);
	}
}

static void
_prepend_limit_processor(ProcessorChainBuilder *builder)
{
	assert(builder != NULL);
	assert(builder->user_data != NULL);

	const Options *opts = (Options *)builder->user_data;

	if(opts->limit >= 0)
	{
		TRACE("action", "Prepending limit processor.");

		Processor *limit = limit_processor_new(opts->limit);

		processor_chain_builder_try_prepend(builder, limit);
	}
}

static void
_prepend_print_processor(ProcessorChainBuilder *builder)
{
	assert(builder != NULL);
	assert(builder->user_data != NULL);

	const Options *opts = (Options *)builder->user_data;

	if(opts->printf)
	{
		TRACE("action", "Prepending print-format processor.");

		Processor *format = print_format_processor_new(opts->printf);

		if(!processor_chain_builder_try_prepend(builder, format))
		{
			fprintf(stderr, _("Couldn't parse format string: %s\n"), opts->printf);
		}
	}
	else
	{
		TRACE("action", "Prepending print processor.");

		Processor *print = print_processor_new();

		processor_chain_builder_try_prepend(builder, print);
	}
}

static ProcessorChain *
_build_processor_chain(const Options *opts)
{
	assert(opts != NULL);

	ProcessorChainBuilder builder;

	processor_chain_builder_init(&builder, opts);

	processor_chain_builder_do(&builder,
	                           _prepend_print_processor,
	                           _prepend_limit_processor,
	                           _prepend_skip_processor,
	                           _prepend_sort_processor,
	                           PROCESSOR_CHAIN_BUILDER_NULL_FN);

	return processor_chain_builder_get_chain(&builder);
}

static bool
_exec_find(const Options *opts)
{
	SearchOptions sopts;
	bool success = false;

	assert(opts != NULL);
	assert(opts->expr != NULL);

	TRACE("action", "Preparing file search.");

	ProcessorChain *chain = _build_processor_chain(opts);

	if(chain)
	{
		_build_search_options(opts, &sopts);

		success = _search_dirs(opts, &sopts, _file_cb, chain);

		TRACE("action", "Cleaning up file search.");

		search_options_free(&sopts);
		processor_chain_destroy(chain);
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

static Action
_read_options(int argc, char *argv[], Options *opts)
{
	assert(argc >= 1);
	assert(argv != NULL);
	assert(opts != NULL);

	Action action = options_load_ini(opts);

	if(action != ACTION_ABORT)
	{
		action = options_getopt(opts, argc, argv);
	}

	return action;
}

static void
_options_init(Options *opts)
{
	assert(opts != NULL);

	memset(opts, 0, sizeof(Options));
	slist_init(&opts->dirs, str_compare, &free, NULL);

	opts->flags = FLAG_STDIN;
	opts->max_depth = -1;
	opts->log_color = true;
	opts->skip = -1;
	opts->limit = -1;
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

