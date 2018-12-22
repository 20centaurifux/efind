/***************************************************************************
    begin........: May 2015
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
   @file dl-ext-backend.h
   @brief libdl filter function backend.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#ifndef DL_EXT_BACKEND_H
#define DL_EXT_BACKEND_H

#include "extension.h"

/**
   @param cls extension class
 
   Extension backend functions for libdl.
 */
void dl_extension_backend_get_class(ExtensionBackendClass *cls);
#endif

