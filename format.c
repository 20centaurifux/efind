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
#include <math.h>
#include <assert.h>

#include "format.h"
#include "utils.h"

static bool
_format_build_fmt_string(char *fmt, size_t len, ssize_t width, ssize_t precision, int flags, const char *conversion)
{
	char *offset = fmt;
	bool success = false;

	assert(fmt != NULL);
	assert(conversion != NULL);

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
			
			if(utils_int_add_checkoverflow(required, (int)floor(log10(width)), &required))
			{
				return false;
			}

			if(precision > 0)
			{
				if(utils_int_add_checkoverflow(required, (int)floor(log10(precision)), &required))
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
		fprintf(stderr, "%s: couldn't write string, built format string exceeds maximum buffer length.\n", __func__);
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
		fprintf(stderr, "%s: couldn't write integer, built format string exceeds maximum buffer length.\n", __func__);
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
		fprintf(stderr, "%s: couldn't write double, built format string exceeds maximum buffer length.\n", __func__);
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
				fprintf(stderr, "%s: date-time string exceeds maximum or is empty.\n", __func__);
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
		_format_write_string(buffer, width, precision, flags, out);
	}
}

bool
format_write(const FormatParserResult *result, FileInfo *info, const char *arg, const char *filename, FILE *out)
{
	FileInfo *pinfo;
	FileInfo _info;
	bool success = false;

	assert(result != NULL);
	assert(result->success == true);
	assert(arg != NULL);
	assert(filename != NULL);
	assert(out != NULL);

	if(info)
	{
		pinfo = info;
	}
	else
	{
		pinfo = &_info;
		file_info_init(pinfo);
	}

	if(pinfo && file_info_get(pinfo, arg, filename))
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
				FileAttr attr;

				if(file_info_get_attr(pinfo, &attr, ((FormatAttrNode*)node)->attr))
				{
					if(attr.flags & FILE_ATTR_FLAG_STRING)
					{
						_format_write_string(file_attr_get_string(&attr), node->width, node->precision, node->flags, out);
					}
					else if(attr.flags & FILE_ATTR_FLAG_INTEGER)
					{
						if(((FormatAttrNode *)node)->attr == 'm')
						{
							_format_write_number(file_attr_get_integer(&attr), node->width, node->precision, node->flags, "o", out);
						}
						else
						{
							_format_write_number(file_attr_get_integer(&attr), node->width, node->precision, node->flags, "d", out);
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
						fprintf(stderr, "%s: couldn't read attribute type: %#x\n", __func__, attr.flags);
						success = false;
					}

					file_attr_free(&attr);
				}
			}
			else
			{
				fprintf(stderr, "%s: invalid node type: %#x\n", __func__, node->type_id);
				success = false;
			}

			iter = slist_item_next(iter);
		}
	}

	if(!info && pinfo)
	{
		file_info_clear(pinfo);
	}

	return success;
}

