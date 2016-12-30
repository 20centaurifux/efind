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
   @file utils.h
   @brief Utility functions.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
   @version 0.1.0
*/
#include <stdlib.h>

#ifndef UTILS_H
#define UTILS_H

#include "ast.h"

/**
   @param size number of bytes to allocate
   @return pointer to the allocated memory block

   Allocates memory and abort on failure.
 */
void *utils_malloc(size_t size);

/**
   @param ptr pointer to memory block to resize
   @param size number of bytes to allocate
   @return pointer to the new memory block

   Resizes a memory block.
 */
void *utils_realloc(void *ptr, size_t size);

/**
   @param n a number
   @return a power of two

   Finds the smallest power of two that's greater or equal to n.
 */
size_t utils_next_pow2(size_t n);

/**
   @param name executable to find
   @return a new allocated string or NULL

   Searches for an executable testing all directories found in PATH
   environment variable.
 */
char * utils_whereis(const char *name);

/**
   @param node a node providing location details
   @param buf buffer to write message to
   @param size size of buffer
   @param format a format string
   @param ... optional arguments
   @return length of the message written to buffer

   Writes a message to a string and prepends location details.
 */
size_t utils_printf_loc(const Node *node, char *buf, size_t size, const char *format, ...);

/**
   @param dir directory name
   @param filename a filename
   @param dst location to write combined path to
   @param max_len maximum buffer size
   @return TRUE on success

   Joins a path with a filename.
 */
bool utils_path_join(const char *dir, const char *filename, char *dst, size_t max_len);

/**
   @param dst return location for the newly-allocated string
   @param fmt format string

   Similar to the standard sprintf() function but writes the string to new allocated memory.
   The function fails if the string length exceeds the internal maximum buffer size (4096).
 */
void utils_strdup_printf(char **dst, const char *fmt, ...);
#endif

