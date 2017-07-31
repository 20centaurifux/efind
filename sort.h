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
   @file sort.h
   @brief Sort found files.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#ifndef SORT_H
#define SORT_H

#include <stdbool.h>

/**
   @param str string to validate
   @return true on success

   Tests if a sort string is valid.
  */
bool sort_string_test(const char *str);

/**
   @param str sort string
   @param field location to store found field
   @param asc location to store sort direction
   @return pointer to next field or NULL
  */
const char *sort_string_pop(const char *str, char *field, bool *asc);

#endif

