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
   @file log.h
   @brief Logging functions.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#ifndef LOG_H
#define LOG_H

#include <stdbool.h>

/**
   @enum LogLevel
   @brief Available log level.
 */
typedef enum
{
	/*! No logging. */
	LOG_LEVEL_NONE,
	/*! Trace message. */
	LOG_LEVEL_TRACE,
	/*! Debug message. */
	LOG_LEVEL_DEBUG,
	/*! General information. */
	LOG_LEVEL_INFO,
	/*! Warning. */
	LOG_LEVEL_WARNING,
	/*! Error message. */
	LOG_LEVEL_ERROR,
	/*! Fatal error. */
	LOG_LEVEL_FATAL
} LogLevel;

/**
   @param level set verbosity

   Sets the verbosity.
 */
void log_set_verbosity(LogLevel level);

/**
   @param enable enable colored log messages

   Enables/disables colored log messages.
 */
void log_enable_color(bool enable);

/**
   @param level log level
   @param filename name of the source file
   @param fn name of the calling function
   @param line line in the source file
   @param domain log domain
   @param msg message to log

   Logs a text message.
 */
void log_print(LogLevel level, const char *filename, const char *fn, int line, const char *domain, const char *msg);

/**
   @param level log level
   @param domain log domain
   @param msg message to log

   Logs a text message.
 */
#define LOG_PRINT(level, domain, msg)              log_print(level, __FILE__, __func__, __LINE__, domain, msg)

/**
   @param domain log domain
   @param msg message to log

   Logs a trace message.
 */
#define TRACE(domain, msg)                         log_print(LOG_LEVEL_TRACE, __FILE__, __func__, __LINE__, domain, msg)

/**
   @param domain log domain
   @param msg message to log

   Logs a debug message.
 */
#define DEBUG(domain, msg)                         log_print(LOG_LEVEL_DEBUG, __FILE__, __func__, __LINE__, domain, msg)

/**
   @param domain log domain
   @param msg message to log

   Logs a general message.
 */
#define INFO(domain, msg)                          log_print(LOG_LEVEL_INFO, __FILE__, __func__, __LINE__, domain, msg)

/**
   @param domain log domain
   @param msg message to log

   Logs a warning message.
 */
#define WARNING(domain, msg)                       log_print(LOG_LEVEL_WARNING, __FILE__, __func__, __LINE__, domain, msg)

/**
   @param domain log domain
   @param msg message to log

   Logs an error message.
 */
#define ERROR(domain, msg)                         log_print(LOG_LEVEL_ERROR, __FILE__, __func__, __LINE__, domain, msg)

/**
   @param domain log domain
   @param msg message to log

   Logs a fatal error message.
 */
#define FATAL(domain, msg)                         log_print(LOG_LEVEL_FATAL, __FILE__, __func__, __LINE__, domain, msg)

/**
   @param level log level
   @param filename name of the source file
   @param fn name of the calling function
   @param line line in the source file
   @param domain log domain
   @param format format string
   @param ... variable number of arguments

   Logs a formatted message.
 */
void log_printf(LogLevel level, const char *filename, const char *fn, int line, const char *domain, const char *format, ...);

/**
   @param level log level
   @param domain log domain
   @param format format string
   @param args variable number of arguments

   Prints a formatted log message.
 */
#define LOG_PRINTF(level, domain, format, args...) log_printf(level, __FILE__, __func__, __LINE__, domain, format, args)

/**
   @param domain log domain
   @param format format string
   @param args variable number of arguments

   Logs a formatted trace message.
 */
#define TRACEF(domain, format, args...)            log_printf(LOG_LEVEL_TRACE, __FILE__, __func__, __LINE__, domain, format, args)

/**
   @param domain log domain
   @param format format string
   @param args variable number of arguments

   Logs a formatted debug message.
 */
#define DEBUGF(domain, format, args...)            log_printf(LOG_LEVEL_DEBUG, __FILE__, __func__, __LINE__, domain, format, args)

/**
   @param domain log domain
   @param format format string
   @param args variable number of arguments

   Logs a formatted general message.
 */
#define INFOF(domain, format, args...)             log_printf(LOG_LEVEL_INFO, __FILE__, __func__, __LINE__, domain, format, args)

/**
   @param domain log domain
   @param format format string
   @param args variable number of arguments

   Logs a formatted warning message.
 */
#define WARNINGF(domain, format, args...)          log_printf(LOG_LEVEL_WARNING, __FILE__, __func__, __LINE__, domain, format, args)
/**
   @param domain log domain
   @param format format string
   @param args variable number of arguments

   Logs a formatted error message.
 */
#define ERRORF(domain, format, args...)            log_printf(LOG_LEVEL_ERROR, __FILE__, __func__, __LINE__, domain, format, args)
/**
   @param domain log domain
   @param format format string
   @param args variable number of arguments

   Logs a formatted fatal error.
 */
#define FATALF(domain, format, args...)             log_printf(LOG_LEVEL_FATAL, __FILE__, __func__, __LINE__, domain, format, args)

#endif

