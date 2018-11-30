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

#include <stdlib.h>

#include "processor.h"

/**
   @param orderby sort string
   @return a new Processor

   Sorts found files.
 */
Processor *sort_processor_new(const char *orderby);

#endif

