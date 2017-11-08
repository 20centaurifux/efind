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
   @file format-fields.h
   @brief Substitute format characters for field names.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#ifndef FORMAT_FIELDS_H
#define FORMAT_FIELDS_H

enum
{
	/*! No flags. */
	FORMAT_FIELDS_FLAG_NONE     = 0,
	/*! Field names are enclosed in brackets. */
	FORMAT_FIELDS_FLAG_BRACKETS = 1
};

/**
   @param str format string
   @param flags substitution flags
   @return a newly-allocated string

   Substitutes format characters for field names.
 */
char *format_substitute(const char *str, int flags);

#endif

