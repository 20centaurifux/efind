/***************************************************************************
    begin........: December 2016
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

/***************************************************************************
   This extension for efind reads tags and properties from audio files.
 ***************************************************************************/

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <taglib/tag_c.h>
#include <regex.h>
#include "extension-interface.h"

void
discover(RegistrationCtx *ctx, RegisterCallback fn)
{
	fn(ctx, "artist_equals", 1, CALLBACK_ARG_TYPE_STRING);
	fn(ctx, "album_equals", 1, CALLBACK_ARG_TYPE_STRING);
	fn(ctx, "title_equals", 1, CALLBACK_ARG_TYPE_STRING);
	fn(ctx, "genre_equals", 1, CALLBACK_ARG_TYPE_STRING);
	fn(ctx, "artist_matches", 1, CALLBACK_ARG_TYPE_STRING);
	fn(ctx, "album_matches", 1, CALLBACK_ARG_TYPE_STRING);
	fn(ctx, "title_matches", 1, CALLBACK_ARG_TYPE_STRING);
	fn(ctx, "genre_matches", 1, CALLBACK_ARG_TYPE_STRING);
	fn(ctx, "audio_length", 0);
	fn(ctx, "audio_bitrate", 0);
	fn(ctx, "audio_samplerate", 0);
	fn(ctx, "audio_channels", 0);
}

static char *
_read_tag(const char *filename, char *(*cb)(const TagLib_Tag *tag))
{
	TagLib_File *file;
	char *result = NULL;

	if((file = taglib_file_new(filename)))
	{
		TagLib_Tag *tag;

		if((tag = taglib_file_tag(file)))
		{
			char *ptr = cb(tag);

			if(ptr)
			{
				result = strdup(ptr);
			}

			taglib_tag_free_strings();
		}

		taglib_file_free(file);
	}

	return result;
}

static int
_read_audio_property(const char *filename, int (*cb)(const TagLib_AudioProperties *prop))
{
	TagLib_File *file;
	int result = 0;

	if((file = taglib_file_new(filename)))
	{
		const TagLib_AudioProperties *props;

		if((props = taglib_file_audioproperties(file)))
		{
			result = cb(props);
		}

		taglib_file_free(file);
	}

	return result;
}

static char *
_regex_escape(const char *str)
{
	char *dst = NULL;
	char *ptr;
	size_t len;

	len = strlen(str);
	dst = (char *)malloc(sizeof(char *) * len * 2 + 1);

	if(dst)
	{
		ptr = dst;

		for(size_t i = 0; i < len; ++i)
		{
			if(strchr(".^$*+?()[{\\|", str[i]))
			{
				*ptr++ = '\\';
			}

			*ptr++ = str[i];
		}

		*ptr = '\0';
	}

	return dst;
}

static int
_regexec(const char *tag, const char *pattern)
{
	int ret = 0;

	regex_t re;

	if(!regcomp(&re, pattern, REG_EXTENDED|REG_NOSUB|REG_ICASE))
	{
		ret = regexec(&re, tag, (size_t) 0, NULL, 0) ? 0 : 1;
		regfree(&re);
	}

	return ret;
}

static int
_equals(const char *tag, const char *query)
{
	char *escaped;
	int ret = 0;

	escaped = _regex_escape(query);

	if(escaped)
	{
		char *pattern = (char *)malloc(sizeof(char *) * strlen(escaped) + 3);

		if(pattern)
		{
			sprintf(pattern, "^%s$", escaped);

			ret = _regexec(tag, pattern);
			free(pattern);
		}

		free(escaped);
	}

	return ret;
}

static int
_matches(const char *tag, const char *query)
{
	char *escaped;
	int ret = 0;

	escaped = _regex_escape(query);
	ret = _regexec(tag, escaped);
	free(escaped);

	return ret;
}

static int
_read_and_compare_tag(const char *filename, const char *query, char *(cb)(const TagLib_Tag *tag))
{
	char *str;
	int ret = 0;

	str = _read_tag(filename, cb);

	if(str)
	{
		ret = _equals(str, query);
		free(str);
	}

	return ret;
}

static int
_read_and_match_tag(const char *filename, const char *query, char *(cb)(const TagLib_Tag *tag))
{
	char *str;
	int ret = 0;

	str = _read_tag(filename, cb);

	if(str)
	{
		ret = _matches(str, query);
		free(str);
	}

	return ret;
}

int
artist_equals(const char *filename, int argc, void *argv[])
{
	return _read_and_compare_tag(filename, (char *)argv[0], taglib_tag_artist);
}

int
album_equals(const char *filename, int argc, void *argv[])
{
	return _read_and_compare_tag(filename, (char *)argv[0], taglib_tag_album);
}

int
title_equals(const char *filename, int argc, void *argv[])
{
	return _read_and_compare_tag(filename, (char *)argv[0], taglib_tag_title);
}

int
genre_equals(const char *filename, int argc, void *argv[])
{
	return _read_and_compare_tag(filename, (char *)argv[0], taglib_tag_genre);
}

int
artist_matches(const char *filename, int argc, void *argv[])
{
	return _read_and_match_tag(filename, (char *)argv[0], taglib_tag_artist);
}

int
album_matches(const char *filename, int argc, void *argv[])
{
	return _read_and_match_tag(filename, (char *)argv[0], taglib_tag_album);
}

int
title_matches(const char *filename, int argc, void *argv[])
{
	return _read_and_match_tag(filename, (char *)argv[0], taglib_tag_title);
}

int
genre_matches(const char *filename, int argc, void *argv[])
{
	return _read_and_match_tag(filename, (char *)argv[0], taglib_tag_genre);
}

int
audio_length(const char *filename, int argc, void *argv[])
{
	return _read_audio_property(filename, taglib_audioproperties_length);
}

int
audio_bitrate(const char *filename, int argc, void *argv[])
{
	return _read_audio_property(filename, taglib_audioproperties_bitrate);
}

int
audio_samplerate(const char *filename, int argc, void *argv[])
{
	return _read_audio_property(filename, taglib_audioproperties_samplerate);
}

int
audio_channels(const char *filename, int argc, void *argv[])
{
	return _read_audio_property(filename, taglib_audioproperties_channels);
}

