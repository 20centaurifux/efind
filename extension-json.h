/***************************************************************************
    begin........: May 2012
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
/*!
 * \file extension-json.h
 * \brief Load modules from JSON files.
 * \author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 * \version 0.1.0
 * \date 24. December 2016
 */

#ifndef __EXTENSION_JSON_H__
#define __EXTENSION_JSON_H__

#include "extension.h"

/*!
 *\param dir ExtensionDir instance
 *\param filename name of the extension description file
 *\param err destination to store error message
 *\return true on success
 *
 * Loads modules from a JSON file.
 */
bool extension_json_load_file(ExtensionDir *dir, const char *filename, char **err);

#endif

