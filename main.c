#define _GNU_SOURCE
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

#include "search.h"
#include "parser.h"
#include "utils.h"

#define VERSION_MAJOR 0
#define VERSION_MINOR 1
#define VERSION_PATCH 0

typedef enum
{
	ACTION_ABORT,
	ACTION_EXEC,
	ACTION_PRINT,
	ACTION_PRINT_HELP,
	ACTION_PRINT_VERSION
} Action;

typedef enum
{
	FLAG_NONE   = 0,
	FLAG_STDIN  = 1,
	FLAG_QUOTE  = 2
} Flags;

typedef struct
{
	int32_t flags;
	char *expr;
	char *dir;
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
		{ "version", no_argument, 0, 'v' },
		{ "help", no_argument, 0, 'h' },
		{ 0, 0, 0, 0 }
	};

	int index = 0;
	Action action = ACTION_EXEC;

	memset(opts, 0, sizeof(Options));

	opts->flags = FLAG_STDIN;
	opts->expr = NULL;

	while(action != ACTION_ABORT)
	{
		int opt = getopt_long(argc, argv, "e:d:qpvh", long_options, &index);

		if(opt == -1)
		{
			break;
		}

		switch(opt)
		{
			case 'e':
				opts->expr = strdup(optarg);
				opts->flags &= ~FLAG_STDIN; 
				break;

			case 'd':
				opts->dir = strdup(optarg);
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

			default:
				action = ACTION_ABORT;
		}
	}

	return action;
}

static char *
_read_expr_from_stdin(void)
{
	char *expr = (char *)utils_malloc(sizeof(char *) * PARSER_MAX_EXPRESSION_LENGTH);
	size_t bytes = PARSER_MAX_EXPRESSION_LENGTH;
	
	bytes = getline(&expr, &bytes, stdin);

	if(bytes == -1)
	{
		fprintf(stderr, "getline() failed\n");
		free(expr);
		expr = NULL;
	}
	else
	{
		expr[strlen(expr) - 1] = '\0';
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
	return (opts->flags & FLAG_QUOTE) ? TRANSLATION_FLAG_QUOTE : TRANSLATION_FLAG_NONE;
}

static void
_file_cb(const char *path, void *user_data)
{
	fprintf(stdout, "%s\n", path);
}

static void
_error_cb(const char *msg, void *user_data)
{
	fprintf(stderr, "WARNING: %s\n", msg);
}

static bool
_exec_find(const Options *opts)
{
	return search_files_expr(opts->dir, opts->expr, _get_translation_flags(opts), _file_cb, _error_cb, NULL);
}

static bool
_print_expr(const Options *opts)
{
	return parse_string_and_print(stdout, stderr, opts->expr, _get_translation_flags(opts));
}

static void
_print_help(const char *name)
{
	printf("Usage: %s [OPTIONS]\n\n", name);
	printf("  -e, --expr       expression to evaluate when finding files\n");
	printf("  -q, --quote      quote special characters in translated expression\n");
	printf("  -dir, --dir      root directory\n");
	printf("  -p, --print      don't search files but print expression to stdout\n");
	printf("  -v, --version    show version and exit\n");
	printf("  -h, --help       display this help and exit\n");
}

static void
_print_version(const char *name)
{
	printf("%s, version %d.%d.%d\n", name, VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
}

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

