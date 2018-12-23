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
   @file print.h
   @brief Print found files.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#ifndef PRINT_H
#define PRINT_H

#include <stdlib.h>

#include "processor.h"

/**
   @return a new Processor

   Prints a found file to stdout.
 */
Processor *print_processor_new(void);

/**
   @param format a format string
   @return a new Processor

   Prints a found file using a format string.
 */
Processor *print_format_processor_new(const char *format);

#endif

