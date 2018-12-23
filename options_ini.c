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
   @file options_ini.c
   @brief Read configuration files.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#include <ini.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <assert.h>

#include "efind.h"
#include "pathbuilder.h"
#include "utils.h"
#include "log.h"
#include "gettext.h"

static void
_ini_handle_logging_section(Options *opts, const char *name, const char *value)
{
	assert(opts != NULL);
	assert(name != NULL);
	assert(value != NULL);

	if(!strcmp(name, "verbosity"))
	{
		long int level = 0;

		if(utils_parse_integer(value, LOG_LEVEL_NONE, LOG_LEVEL_FATAL, &level))
		{
			opts->log_level = (LogLevel)level;
		}
	}
	else if(!strcmp(name, "color"))
	{
		utils_parse_bool(value, &opts->log_color);
	}
}

static void
_ini_handle_general_section(Options *opts, const char *name, const char *value)
{
	assert(opts != NULL);
	assert(name != NULL);
	assert(value != NULL);

	if(!strcmp(name, "quote"))
	{
		bool set;

		if(utils_parse_bool(value, &set))
		{
			if(set)
			{
				opts->flags |= FLAG_QUOTE;
			}
			else
			{
				opts->flags &= ~FLAG_QUOTE;
			}
		}
	}
	else if(!strcmp(name, "follow-links"))
	{
		utils_parse_bool(value, &opts->follow);
	}
	else if(!strcmp(name, "max-depth"))
	{
		long int depth;

		if(utils_parse_integer(value, 0, INT32_MAX, &depth))
		{
			opts->max_depth = (int32_t)depth;
		}
	}
	else if(!strcmp(name, "regex-type"))
	{
		utils_copy_string(value, &opts->regex_type);
	}
	else if(!strcmp(name, "order-by"))
	{
		utils_copy_string(value, &opts->orderby);
	}
	else if(!strcmp(name, "printf"))
	{
		utils_copy_string(value, &opts->printf);
	}
	else if(!strcmp(name, "exec-ignore-errors"))
	{
		utils_parse_bool(value, &opts->exec_ignore_errors);
	}
}

static int
_ini_handler(void *user_data, const char *section, const char *name, const char *value)
{
	assert(user_data != NULL);
	assert(section != NULL);
	assert(name != NULL);
	assert(value != NULL);

	Options *opts = user_data;

	if(!strcmp(section, "general"))
	{
		_ini_handle_general_section(opts, name, value);
	}
	else if(!strcmp(section, "logging"))
	{
		_ini_handle_logging_section(opts, name, value);
	}

	return 1;
}

void
options_load_ini(Options *opts)
{
	char path[PATH_MAX];

	assert(opts != NULL);

	if(path_builder_global_ini(path, PATH_MAX))
	{
		int line = ini_parse(path, _ini_handler, opts);

		if(line > 0)
		{
			fprintf(stderr, _("Error in line %d of global configuration file: \"%s\"\n"), line, path);
		}
	}

	if(path_builder_local_ini(path, PATH_MAX))
	{
		int line = ini_parse(path, _ini_handler, opts);

		if(line > 0)
		{
			fprintf(stderr, _("Error in line %d of local configuration file: \"%s\"\n"), line, path);
		}
	}
}

