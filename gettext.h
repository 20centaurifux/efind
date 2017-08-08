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
   @file gettext.h
   @brief GNU gettext functionality.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#ifndef GETTEXT_H
#define GETTEXT_H
#include <libintl.h>

/*! gettext macro. */
#define _(str) gettext(str)

/*! efind's gettext domain name. */
#define EFIND_DOMAIN_NAME "efind"

/*! Set base directory and domain. */
void gettext_init(void);

#endif

