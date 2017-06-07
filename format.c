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
   @file format.c
   @brief Format and print file attributes.
   @author Sebastian Fedrau <sebastian.fedrau@gmail.com>
 */
#include <alloca.h>
#include <assert.h>

#include "format.h"
#include "fileinfo.h"

static void
_format_write_string(const char *text, ssize_t width, int flags, FILE *out)
{
	size_t len;
	char *ptr = NULL;

	len = strlen(text);

	if(width > 0 && (size_t)width > len)
	{
		ptr = alloca(width + 1);

		if(ptr)
		{
			memset(ptr, ' ', width + 1);
			ptr[width] = '\0';

			if(flags & FORMAT_PRINT_FLAG_MINUS)
			{
				memcpy(ptr, text, len);
			}
			else
			{
				memcpy(ptr + (width - len), text, len);
			}
		}
	}
	else
	{
		ptr = (char *)text;
	}

	if(ptr)
	{
		fputs(ptr, out);
	}
}

static void
_format_write_integer(int n, ssize_t width, int flags, const char conversion, FILE *out)
{
	char fmt[64];
	char *offset = fmt;

	/* build format string */
	memset(fmt, 0, 64);

	*offset++ = '%'; 

	if(flags & FORMAT_PRINT_FLAG_MINUS)
	{
		*offset++ = '-';
	}

	if(flags & FORMAT_PRINT_FLAG_ZERO)
	{
		*offset++ = '0';
	}

	if(flags & FORMAT_PRINT_FLAG_SPACE)
	{
		*offset++ = ' ';
	}

	if(flags & FORMAT_PRINT_FLAG_PLUS)
	{
		*offset++ = '+';
	}

	if(flags & FORMAT_PRINT_FLAG_HASH)
	{
		*offset++ = '#';
	}

	if(width > 0)
	{
		sprintf(offset, "%ldd", width);
	}
	else
	{
		*offset = conversion;
	}

	/* print string */
	fprintf(out, fmt, n);
}

static void
_format_write_date(time_t time, ssize_t width, int flags, const char *format, FILE *out)
{
	char buffer[765];

	if(format && *format)
	{
		char time_format[64];
		size_t len = strlen(format);

		if(len < sizeof(time_format) / 2)
		{
			struct tm *tm;

			tm = localtime(&time);

			/* build format string */
			memset(time_format, 0, len * 2 + 1);

			for(size_t i = 0; i < len; ++i)
			{
				time_format[i * 2] = '%';
				time_format[i * 2 + 1] = format[i];
			}

			/* build date string */
			if(strftime(buffer, sizeof(buffer), time_format, tm))
			{
				fputs(buffer, out);
			}
			else
			{
				fprintf(stderr, "%s: date-time string exceeds maximum.\n", __func__);
			}
		}
		else
		{
			fprintf(stderr, "%s: format string exceeds maximum.\n", __func__);
		}
	}
	else
	{
		strcpy(buffer, ctime(&time));
		buffer[strlen(buffer) - 1] = '\0';
		fputs(buffer, out);
	}
}

void
format_write(const FormatParserResult *result, const char *arg, const char *filename, FILE *out)
{
	FileInfo info;

	assert(result != NULL);
	assert(result->success == true);
	assert(arg != NULL);
	assert(filename != NULL);
	assert(out != NULL);

	file_info_init(&info);

	if(file_info_get(&info, arg, filename))
	{
		SListItem *iter = slist_head(result->nodes);

		while(iter)
		{
			FormatNodeBase *node = (FormatNodeBase *)slist_item_get_data(iter);

			if(node->type_id == FORMAT_NODE_TEXT)
			{
				_format_write_string(((FormatTextNode *)node)->text, node->width, node->flags, out);
			}
			else if(node->type_id == FORMAT_NODE_ATTR)
			{
				FileAttr attr;

				if(file_info_get_attr(&info, &attr, ((FormatAttrNode*)node)->attr))
				{
					if(attr.flags & FILE_ATTR_FLAG_STRING)
					{
						_format_write_string(file_attr_get_string(&attr), node->width, node->flags, out);
					}
					else if(attr.flags & FILE_ATTR_FLAG_INTEGER)
					{
						if(((FormatAttrNode *)node)->attr == 'm')
						{
							_format_write_integer(file_attr_get_integer(&attr), node->width, node->flags, 'o', out);
						}
						else
						{
							_format_write_integer(file_attr_get_integer(&attr), node->width, node->flags, 'd', out);
						}
					}
					else if(attr.flags & FILE_ATTR_FLAG_TIME)
					{
						_format_write_date(file_attr_get_time(&attr), node->width, node->flags, ((FormatAttrNode *)node)->format, out);
					}
					else
					{
						/* TODO */
						abort();
					}

					file_attr_free(&attr);
				}
			}
			else
			{
				/* TODO */
				abort();
			}

			iter = slist_item_next(iter);
		}

		file_info_clear(&info);
	}
}

