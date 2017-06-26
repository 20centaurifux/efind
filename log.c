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

static LogLevel _verbosity = LOG_LEVEL_WARNING;

static const char *_LOG_LEVEL_NAMES[7] =
{
	"TRACE", "DEBUG", "INFO", "WARNING", "ERROR", "FATAL"
};

static void
_log_prefix(FILE *out, LogLevel level, const char *filename, const char *fn, int line, const char *domain)
{
	char timestamp[64];
	time_t now;
	struct tm *timeinfo;

	assert(level >= LOG_LEVEL_TRACE && level <= LOG_LEVEL_FATAL);

	time(&now);
	timeinfo = localtime(&now);
	strcpy(timestamp, asctime(timeinfo));
	timestamp[strlen(timestamp) - 1] = '\0';

	if(!domain || !*domain)
	{
		domain = "n/a";
	}

	fprintf(out, "%s %s <%s> %s, %s(), line %d => ", _LOG_LEVEL_NAMES[level - 1], timestamp, domain, filename, fn, line);
}

void
log_set_verbosity(LogLevel level)
{
	_verbosity = level;
}

static bool
_log_check_level(LogLevel level)
{
	if(level >= LOG_LEVEL_TRACE && level <= LOG_LEVEL_FATAL)
	{
		return level >= _verbosity;
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
		fputc('\n', fp);
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
		fputc('\n', fp);

		va_end(ap);
	}
}

