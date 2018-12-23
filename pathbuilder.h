/***************************************************************************
    begin........: April 2015
    copyright....: Sebastian Fedrau
    email........: sebastian.fedrau@gmail.com
 ***************************************************************************/

/***************************************************************************
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License v3 as published by
    the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License v3 for more details.
 ***************************************************************************/
/**
   @file pathbuilder.h
   @brief Utility functions for building file paths.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#ifndef PATHBUILDER_H
#define PATHBUILDER_H

#include <stdbool.h>

/**
   @param path location to write built path to
   @param path_len maximum buffer size
   @return true on success

   Builds the path to the global configuration file.
 */
bool path_builder_global_ini(char *path, size_t path_len);

/**
   @param path location to write built path to
   @param path_len maximum buffer size
   @return true on success

   Builds the path to the local configuration file.
 */
bool path_builder_local_ini(char *path, size_t path_len);

/**
   @param path location to write built path to
   @param path_len maximum buffer size
   @return true on success

   Builds the path to globally installed extensions.
 */
bool path_builder_global_extensions(char *path, size_t path_len);

/**
   @param path location to write built path to
   @param path_len maximum buffer size
   @return true on success

   Builds the path to locally installed extensions.
 */
bool path_builder_local_extensions(char *path, size_t path_len);

/**
   @param path location to write built path to
   @param path_len maximum buffer size
   @return true on success

   Builds the path to the user's blacklist.
 */
bool path_builder_blacklist(char *path, size_t path_len);

#endif

