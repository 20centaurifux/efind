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
#include <stdbool.h>
#include <assert.h>
#include <linux/limits.h>
#include <regex.h>
#include <taglib/tag_c.h>

#include "extension-interface.h"

#define NAME        "taglib"
#define VERSION     "0.1.0"
#define DESCRIPTION "Read tags and properties from audio files."

#define MAX_TAG 512

typedef struct
{
	char filename[PATH_MAX];
	char artist[MAX_TAG];
	char album[MAX_TAG];
	char title[MAX_TAG];
	char genre[MAX_TAG];
	int length;
	int bitrate;
	int samplerate;
	int channels;
} AudioProperties;

void
registration(RegistrationCtx *ctx, RegisterExtension fn)
{
	fn(ctx, NAME, VERSION, DESCRIPTION);
}

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

static AudioProperties *
_read_properties(const char *filename)
{
	static AudioProperties cache = { .filename = "" };
	AudioProperties *properties = NULL;

	assert(filename != NULL);

	if(strcmp(filename, cache.filename))
	{
		TagLib_File *file;
	
		memset(&cache, 0, sizeof(AudioProperties));

		if((file = taglib_file_new(filename)))
		{
			TagLib_Tag *tag;
			bool success = false;

			if((tag = taglib_file_tag(file)))
			{
				char *ptr = taglib_tag_artist(tag);

				if(ptr)
				{
					strncpy(cache.artist, ptr, MAX_TAG);
				}

				if((ptr = taglib_tag_album(tag)))
				{
					strncpy(cache.album, ptr, MAX_TAG);
				}

				if((ptr = taglib_tag_title(tag)))
				{
					strncpy(cache.title, ptr, MAX_TAG);
				}

				if((ptr = taglib_tag_genre(tag)))
				{
					strncpy(cache.genre, ptr, MAX_TAG);
				}

				taglib_tag_free_strings();

				const TagLib_AudioProperties *audio_properties;

				if((audio_properties = taglib_file_audioproperties(file)))
				{
					cache.length = taglib_audioproperties_length(audio_properties);
					cache.bitrate = taglib_audioproperties_bitrate(audio_properties);
					cache.samplerate = taglib_audioproperties_samplerate(audio_properties);
					cache.channels = taglib_audioproperties_channels(audio_properties);

					success = true;
				}
			}

			if(success)
			{
				strncpy(cache.filename, filename, PATH_MAX);
				properties = &cache;
			}

			taglib_file_free(file);
		}
	}
	else
	{
		properties = &cache;
	}

	return properties;
}

static char *
_regex_escape(const char *str)
{
	char *dst = NULL;
	char *ptr;
	size_t len;

	if(str)
	{
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
	}
	else
	{
		dst = strdup("");
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
_compare_tags(const char *tag, const char *query)
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
_match_tags(const char *tag, const char *query)
{
	char *escaped;
	int ret = 0;

	if((escaped = _regex_escape(query)))
	{
		ret = _regexec(tag, escaped);

		free(escaped);
	}

	return ret;
}

#define AUDIO_TAG_EQUALS(field) \
int \
field##_equals(const char *filename, int argc, void *argv[]) \
{ \
	AudioProperties *props; \
\
	if((props = _read_properties(filename))) \
	{ \
		return _compare_tags(props->field, (char *)*argv); \
	} \
\
	return 0; \
}

AUDIO_TAG_EQUALS(artist)
AUDIO_TAG_EQUALS(album)
AUDIO_TAG_EQUALS(title)
AUDIO_TAG_EQUALS(genre)

#define AUDIO_TAG_MATCHES(field) \
int \
field##_matches(const char *filename, int argc, void *argv[]) \
{ \
	AudioProperties *props; \
\
	if((props = _read_properties(filename))) \
	{ \
		return _match_tags(props->field, (char *)*argv); \
	} \
\
	return 0; \
}

AUDIO_TAG_MATCHES(artist)
AUDIO_TAG_MATCHES(album)
AUDIO_TAG_MATCHES(title)
AUDIO_TAG_MATCHES(genre)

#define GET_AUDIO_PROPERTY(field) \
int \
audio_##field(const char *filename, int argc, void *argv[]) \
{ \
	AudioProperties *props; \
\
	if((props = _read_properties(filename))) \
	{ \
		return props->field; \
	} \
\
	return 0; \
}

GET_AUDIO_PROPERTY(length)
GET_AUDIO_PROPERTY(bitrate)
GET_AUDIO_PROPERTY(samplerate)
GET_AUDIO_PROPERTY(channels)

