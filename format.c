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
#include <math.h>
#include <assert.h>

#include "format.h"
#include "fileinfo.h"
#include "log.h"
#include "utils.h"

static bool
_format_build_fmt_string(char *fmt, size_t len, ssize_t width, ssize_t precision, int flags, const char *conversion)
{
	bool success = false;

	assert(fmt != NULL);
	assert(conversion != NULL);

	char *offset = fmt;

	if(len > 1)
	{
		memset(fmt, 0, len);

		*offset++ = '%'; 

		/*! @cond INTERNAL */
		#define TEST_FMT_BUFFER(LEN) if(len - (offset - fmt) <= (size_t)LEN) { return false; }
		/*! @endcond */

		if(flags & FORMAT_PRINT_FLAG_MINUS)
		{
			TEST_FMT_BUFFER(1);
			*offset++ = '-';
		}

		if(flags & FORMAT_PRINT_FLAG_ZERO)
		{
			TEST_FMT_BUFFER(1);
			*offset++ = '0';
		}

		if(flags & FORMAT_PRINT_FLAG_SPACE)
		{
			TEST_FMT_BUFFER(1);
			*offset++ = ' ';
		}

		if(flags & FORMAT_PRINT_FLAG_PLUS)
		{
			TEST_FMT_BUFFER(1);
			*offset++ = '+';
		}

		if(flags & FORMAT_PRINT_FLAG_HASH)
		{
			TEST_FMT_BUFFER(1);
			*offset++ = '#';
		}

		if(width > 0)
		{
			int required = 1;
			
			if(utils_int_add_checkoverflow(required, (int)floor(log10(width)) + 1, &required))
			{
				return false;
			}

			if(precision > 0)
			{
				if(utils_int_add_checkoverflow(required, (int)floor(log10(precision)) + 1, &required))
				{
					return false;
				}

				if(utils_int_add_checkoverflow(required, strlen(conversion), &required))
				{
					return false;
				}

				if(utils_int_add_checkoverflow(required, 2, &required))
				{
					return false;
				}
			}

			TEST_FMT_BUFFER(required);

			if(precision > 0)
			{
				sprintf(offset, "%ld.%ld%s", (long)width, (long)precision, conversion);
			}
			else
			{
				sprintf(offset, "%ld%s", (long)width, conversion);
			}
		}
		else
		{
			TEST_FMT_BUFFER(strlen(conversion));
			strcpy(offset, conversion);
		}

		success = true;
	}

	return success;
}

static void
_format_write_string(const char *text, ssize_t width, ssize_t precision, int flags, FILE *out)
{
	char fmt[128];

	if(_format_build_fmt_string(fmt, 128, width, precision, flags, "s"))
	{
		fprintf(out, fmt, text);
	}
	else
	{
		ERROR("format", "Couldn't write string, built format string exceeds maximum buffer length.");
	}
}

static void
_format_write_number(long long n, ssize_t width, ssize_t precision, int flags, const char *conversion, FILE *out)
{
	char fmt[128];

	if(_format_build_fmt_string(fmt, 128, width, precision, flags, conversion))
	{
		fprintf(out, fmt, n);
	}
	else
	{
		ERROR("format", "Couldn't write integer, built format string exceeds maximum buffer length.");
	}
}

static void
_format_write_double(double d, ssize_t width, ssize_t precision, int flags, FILE *out)
{
	char fmt[128];

	if(_format_build_fmt_string(fmt, 128, width, precision, flags, "f"))
	{
		fprintf(out, fmt, d);
	}
	else
	{
		ERROR("format", "Couldn't write double, built format string exceeds maximum buffer length.");
	}
}

static void
_format_write_date(time_t time, ssize_t width, ssize_t precision, int flags, const char *format, FILE *out)
{
	char buffer[4096];

	if(format && *format)
	{
		char time_format[256];
		size_t len = strlen(format);

		if(len < SIZE_MAX && len + 1 < sizeof(time_format) / 2)
		{
			struct tm *tm;

			tm = localtime(&time);

			/* build format string */
			memset(time_format, 0, sizeof(time_format));

			for(size_t i = 0; i < len; ++i)
			{
				time_format[i * 2] = '%';
				time_format[i * 2 + 1] = format[i];
			}

			/* write date string */
			if(strftime(buffer, sizeof(buffer), time_format, tm))
			{
				_format_write_string(buffer, width, precision, flags, out);
			}
			else
			{
				ERROR("format", "Date-time string is empty or exceeds allowed maximum buffer length.");
			}
		}
		else
		{
			ERROR("format", "Format string exceeds allowed maximum length.");
		}
	}
	else
	{
		strcpy(buffer, ctime(&time));
		buffer[strlen(buffer) - 1] = '\0';
		_format_write_string(buffer, width, precision, flags, out);
	}
}

static bool
_format_write_attr_node(const FormatParserResult *result, const FormatNodeBase *node, FileInfo *info, FILE *out)
{
	bool success = true;
	FileAttr attr;

	if(file_info_get_attr(info, &attr, ((FormatAttrNode*)node)->attr))
	{
		if(attr.flags & FILE_ATTR_FLAG_STRING)
		{
			_format_write_string(file_attr_get_string(&attr), node->width, node->precision, node->flags, out);
		}
		else if(attr.flags & FILE_ATTR_FLAG_INTEGER)
		{
			if(((FormatAttrNode *)node)->attr == 'm')
			{
				_format_write_number(file_attr_get_integer(&attr), node->width, node->precision, node->flags, "llo", out);
			}
			else
			{
				_format_write_number(file_attr_get_integer(&attr), node->width, node->precision, node->flags, "lld", out);
			}
		}
		else if(attr.flags & FILE_ATTR_FLAG_LLONG)
		{
			_format_write_number(file_attr_get_llong(&attr), node->width, node->precision, node->flags, "lld", out);
		}
		else if(attr.flags & FILE_ATTR_FLAG_TIME)
		{
			_format_write_date(file_attr_get_time(&attr), node->width, node->precision, node->flags, ((FormatAttrNode *)node)->format, out);
		}
		else if(attr.flags & FILE_ATTR_FLAG_DOUBLE)
		{
			_format_write_double(file_attr_get_double(&attr), node->width, node->precision, node->flags, out);
		}
		else
		{
			FATALF("format", "Couldn't read attribute type from flags: %#x", attr.flags);
			success = false;
		}

		file_attr_free(&attr);
	}

	return success;
}

bool
format_write(const FormatParserResult *result, const char *arg, const char *filename, FILE *out)
{
	FileInfo info;
	bool success = false;

	assert(result != NULL);
	assert(result->success == true);
	assert(arg != NULL);
	assert(filename != NULL);
	assert(out != NULL);

	file_info_init(&info);

	if(file_info_get(&info, arg, false, filename))
	{
		SListItem *iter = slist_head(result->nodes);
		success = true;

		while(success && iter)
		{
			FormatNodeBase *node = (FormatNodeBase *)slist_item_get_data(iter);

			if(node->type_id == FORMAT_NODE_TEXT)
			{
				_format_write_string(((FormatTextNode *)node)->text, node->width, node->precision, node->flags, out);
			}
			else if(node->type_id == FORMAT_NODE_ATTR)
			{
				success = _format_write_attr_node(result, node, &info, out);
			}
			else
			{
				FATALF("format", "Invalid node type: %#x", node->type_id);
				success = false;
			}

			iter = slist_item_next(iter);
		}

		file_info_clear(&info);
	}

	return success;
}

