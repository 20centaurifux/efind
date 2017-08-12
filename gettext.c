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
   @file gettext.c
   @brief GNU gettext functionality.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#include <stdio.h>

#include "gettext.h"

void
gettext_init(void)
{
	if(!bindtextdomain(EFIND_DOMAIN_NAME, LOCALEDIR))
	{
		textdomain(EFIND_DOMAIN_NAME);
		fprintf(stderr, "Couldn't set text domain.\n");
	}

	textdomain(EFIND_DOMAIN_NAME);
}

