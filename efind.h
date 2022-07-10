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
   @file efind.h
   @brief Version information & option parsing.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#ifndef EFIND_H
#define EFIND_H

#include <datatypes.h>

#include "log.h"

/*! Major version. */
#define EFIND_VERSION_MAJOR     0
/*! Minor version. */
#define EFIND_VERSION_MINOR     5
/*! Patch level. */
#define EFIND_VERSION_PATCH     9
/*! Code name. */
#define EFIND_VERSION_CODE_NAME "James Cook"
/*! Project website. */
#define EFIND_WEBSITE           "http://efind.dixieflatline.de"
/*! Copyright date. */
#define EFIND_COPYRIGHT_DATE    "2017-2021"
/*! License name. */
#define EFIND_LICENSE_NAME      "GPLv3"
/*! URL to license text. */
#define EFIND_LICENSE_URL       "https://www.gnu.org/licenses/gpl-3.0.txt"
/*! Author's name. */
#define EFIND_AUTHOR_NAME       "Sebastian Fedrau"
/*! Author's mail address. */
#define EFIND_AUTHOR_EMAIL      "sebastian.fedrau@gmail.com"

/**
   @enum Action
   @brief Available actions.
 */
typedef enum
{
	/*! Abort application. */
	ACTION_ABORT,
	/*! Search files. */
	ACTION_EXEC,
	/*! Translate and print search expressions. */
	ACTION_PRINT,
	/*! Print help and exit. */
	ACTION_PRINT_HELP,
	/*! Print version and exit. */
	ACTION_PRINT_VERSION,
	/*! Print extensions and exit. */
	ACTION_PRINT_EXTENSIONS,
	/*! Print ignore-list and exit. */
	ACTION_PRINT_IGNORELIST
} Action;

/**
   @enum Flags
   @brief Optional runtime flags.
 */
typedef enum
{
	/*! No flags set. */
	FLAG_NONE   = 0,
	/*! Read expression from stdin. */
	FLAG_STDIN  = 1,
	/*! Quote printed `find' arguments. */
	FLAG_QUOTE  = 2
} Flags;

/**
   @struct Options
   @brief Runtime options.
 */
typedef struct
{
	/*! Verbosity. */
	LogLevel log_level;
	/*! Enable log colors. */
	bool log_color;
	/*! Runtime flags. */
	int32_t flags;
	/*! Search expression. */
	char *expr;
	/*! Directories to search. */
	SList dirs;
	/*! Maximum search depth. */
	int32_t max_depth;
	/*! Follow symlinks. */
	bool follow;
	/*! Understood regular expression syntax. */
	char *regex_type;
	/*! Format string. */
	char *printf;
	/*! List of --exec argument lists. */
	SList exec;
	/*! Don't stop if command exits with non-zero result. */
	bool exec_ignore_errors;
	/*! Sort string. */
	char *orderby;
	/*! Number of files to skip before printing to stdout. */
	int32_t skip;
	/*! Maximum number of files to print to stdout. */
	int32_t limit;
} Options;

/**
   @param opts Options to set

   Reads and sets options from INI files.
 */
void options_load_ini(Options *opts);

/**
   @param opts Options to set
   @param argc number of command-line arguments
   @param argv command-line arguments
   @return Action to run.

   Reads and sets options from command-line arguments.
 */
Action options_getopt(Options *opts, int argc, char *argv[]);

#endif

