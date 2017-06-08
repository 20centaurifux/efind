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
   @file format.h
   @brief Format and print file attributes.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#ifndef FORMAT_H
#define FORMAT_H

#include "format-parser.h"

/**
   @param result a FormatParserResult instance
   @param arg command line arguments under which the file was found
   @param filename found file
   @param out stream to write output to

   Print file attributes according to the specified format.
 */
void format_write(const FormatParserResult *result, const char *arg, const char *filename, FILE *out);

#endif

