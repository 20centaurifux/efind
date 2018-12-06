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
   @file exec.h
   @brief Executes shell commands.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */

#ifndef EXEC_H
#define EXEC_H

#include <stdlib.h>

#include "processor.h"
#include "exec-args.h"

/**
   @param args command & arguments to execute
   @return a new Processor

   Excecutes shell commands.
 */
Processor *exec_processor_new(const ExecArgs *args);

#endif

