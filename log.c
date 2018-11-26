/***************************************************************************
    prefix........: April 2015
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
   @file log.c
   @brief Logging functions.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>

#include "log.h"

/*! @cond INTERNAL */
static struct
{
	LogLevel verbosity;
	bool enable_color;
} _log_settings = { .verbosity = LOG_LEVEL_NONE, .enable_color = true };

static const char *_LOG_LEVEL_NAMES[6] =
{
	"TRACE", "DEBUG", "INFO", "WARNING", "ERROR", "FATAL"
};

static const char *_LOG_COLORS[6] =
{
	"\x1B[36m", /* cyan*/
	"\x1B[32m", /* green*/
	"\x1B[37m", /* white */
	"\x1B[33m", /* yellow */
	"\x1B[31m", /* magenta */
	"\x1B[35m"  /* red */
};
/*! @endcond */

static void
_log_prefix(FILE *out, LogLevel level, const char *filename, const char *fn, int line, const char *domain)
{
	char timestamp[64];
	time_t now;
	struct tm *timeinfo;

	assert(level >= LOG_LEVEL_TRACE && level <= LOG_LEVEL_FATAL);

	time(&now);
	timeinfo = localtime(&now);
	strftime(timestamp, 64, "%c", timeinfo);

	if(!domain || !*domain)
	{
		domain = "n/a";
	}

	if(_log_settings.enable_color)
	{
		fprintf(out, "\033[0m%s", _LOG_COLORS[level - 1]);
	}

	fprintf(out, "%s %s <%s> %s, %s(), line %d => ", _LOG_LEVEL_NAMES[level - 1], timestamp, domain, filename, fn, line);
}

static void
_log_suffix(FILE *fp)
{
	if(_log_settings.enable_color)
	{
		fputs("\033[0m\n", fp);
	}
	else
	{
		fputc('\n', fp);
	}
}

void
log_set_verbosity(LogLevel level)
{
	if(level < LOG_LEVEL_NONE)
	{
		_log_settings.verbosity = LOG_LEVEL_NONE;
	}
	else if(level > LOG_LEVEL_FATAL)
	{
		_log_settings.verbosity = LOG_LEVEL_FATAL;
	}
	else
	{
		_log_settings.verbosity = level;
	}
}

void
log_enable_color(bool enable)
{
	_log_settings.enable_color = enable;
}

static bool
_log_check_level(LogLevel level)
{
	if(_log_settings.verbosity > LOG_LEVEL_NONE)
	{
		if(level >= LOG_LEVEL_TRACE && level <= LOG_LEVEL_FATAL)
		{
			return level > LOG_LEVEL_FATAL - _log_settings.verbosity;
		}

	}

	return false;
}

static FILE *
_log_get_file(LogLevel level)
{
	return level < LOG_LEVEL_WARNING ? stdout : stderr;
}

void
log_print(LogLevel level, const char *filename, const char *fn, int line, const char *domain, const char *msg)
{
	if(_log_check_level(level))
	{
		FILE *fp = _log_get_file(level);

		_log_prefix(fp, level, filename, fn, line, domain);
		fputs(msg, fp);
		_log_suffix(fp);
	}
}

void
log_printf(LogLevel level, const char *filename, const char *fn, int line, const char *domain, const char *format, ...)
{
	if(_log_check_level(level))
	{
		FILE *fp = _log_get_file(level);
		va_list ap;

		_log_prefix(fp, level, filename, fn, line, domain);

		va_start(ap, format);

		vfprintf(fp, format, ap);
		_log_suffix(fp);

		va_end(ap);
	}
}

