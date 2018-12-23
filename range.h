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
   @file range.h
   @brief Limit output.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#ifndef RANGE_H
#define RANGE_H

#include <stdlib.h>

#include "processor.h"

/**
   @param skip items to skip
   @return a new Processor

   Skips the first \p skip items.
 */
Processor *skip_processor_new(size_t skip);

/**
   @param limit maximum number of items to process
   @return a new Processor

   Closes the processor after \p limit items.
 */
Processor *limit_processor_new(size_t limit);

#endif

