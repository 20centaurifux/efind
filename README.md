# efind

## Introduction

**efind** (extendable find) searches for files in a directory hierarchy.

Basically it's a wrapper for [GNU find](https://www.gnu.org/software/findutils/)
providing an easier and more intuitive expression syntax. Additionally
you can filter search results with custom functions.


## Quick overview

Let's assume you want to find all writable source and header files of a C project
that were modified in a time frame less than two days. That's no problem with
**efind's** self-explanatory expression syntax:

```
$ efind . '(name="*.h" or name="*.c") and type=file and writable and mtime < 2 days'
```

A similar GNU find expression looks more complicated:

```
$ find . \( -name "*.h" -o -name "*.c" \) -a -type f -a -writable -a -mtime -2
```

Besides the more user-friendly syntax, **efind** always tries to act in the way
an average user would expect. A good counter-example is the way GNU find rounds
up file sizes.

The following expression finds *all* documents in the current folder with a file size less
or equal to 1G because every file with *at least one byte* is rounded up:

```
$ find . -size 1G
```

**efind** converts file sizes to byte to avoid this confusing behaviour:

```
$ efind . "size=1G" --print
$ find . -size 1073741824c
```

I created a [ticket](https://savannah.gnu.org/bugs/?46815) regarding this "feature" of
GNU find.

As mentioned, **efind** can be extended with custom functions. You want to filter
the search result by audio tags and properties? No problem :)

```
$ efind ~/amazon_music 'iname="*.mp3" and artist_matches("the cure") and title_matches("street") and audio_length()>200'
$ /home/sf/amazon_music/the cure/Bestival Live 2011/03 - Fascination Street (Bestival Live 2011).mp3
```

You want to filter images by height and width? Just type in

```
$ efind ~/tmp 'iname="*.JPG" and image_width()>=3840 and image_height()>=2400'
$ /home/sf/tmp/xmas.jpg
```


## Building efind

**efind** uses [GNU Make](https://www.gnu.org/software/make/) as build system.
Installation options can be customized in the Makefile.

Please ensure that [GNU Bison](https://www.gnu.org/software/bison/) and
[GNU Flex](https://www.gnu.org/software/flex/) is installed on your system before
you build **efind**. You will also need [libdatatypes](https://github.com/20centaurifux/datatypes)
and [libreadline](https://cnswww.cns.cwru.edu/php/chet/readline/rltop.html).

If you want to build **efind** on a Debian based distribution follow the steps found in the
[README](https://github.com/20centaurifux/datatypes/blob/master/README.md) of the datatypes
library. Then type in these additional commands to prepare your system and checkout the code:

```
$ apt-get install libbsd-dev bison flex libreadline-dev
$ git clone https://github.com/20centaurifux/efind.git
```

Now you can build and install the application:

```
$ cd efind
$ make
$ sudo make install
```

If you want to generate the source code documentation type in

```
$ make doc
```

This step requires [Doxygen](http://www.stack.nl/~dimitri/doxygen/).


## Expression syntax

A search expression consists of at least one comparison or file flag to test. Multiple
expressions can be evaluated by using conditional operators:

| Operator | Description                                                                                                                                                   |
| :------- | :------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| and      | If an expression returns logical false it returns that value and doesn't evaluate the next expression. Otherwise it returns the value of the last expression. |
| or       | If an expression returns logical true it returns that value and doesn't evaluate the next expression. Otherwise it returns the value of the last expression.  |

Expressions are evaluated from left to right. You can use parentheses to force precedence.

The following operators can be used to compare a file attribute to a value:

| Operator | Description      |
| :------- | :--------------- |
| =        | equals to        |
| >        | greater than     |
| >=       | greater or equal |
| <        | less than        |
| <=       | less or equal    |

A value must be of one of the data types described below:

| Type          | Description                                                                                                            |
| :------------ | :--------------------------------------------------------------------------------------------------------------------- |
| string        | Quoted sequence of characters.                                                                                         |
| number        | Natural number.                                                                                                        |
| time interval | Time interval (number) with suffix. Supported suffixes are "minute(s)", "hour(s)" and "day(s)".                        |
| file size     | Units of space (number) with suffix. Supported suffixes are "byte(s)", "kilobyte(s)", "megabyte(s)" and "gigabyte(s)". |
| file type     | "file", "directory", "block", "character", "pipe", "link" or "socket".                                                 |

You can search the following file attributes:

| Attribute | Description                       | Type            | Example     |
| :-------- | :-------------------------------- | :-------------- | :---------- |
| name      | case sensitive filename pattern   | string          | "*.txt"     |
| iname     | case insensitive filename pattern | string          | "Foo.bar"   |
| atime     | last access time                  | time interval   | 1 minute    |
| ctime     | last file status change           | time interval   | 15 hours    |
| mtime     | last modification time            | time interval   | 30 days     |
| size      | file size                         | size            | 10 megabyte |
| group     | name of the group owning the file | string          | "users"     |
| gid       | id of the group owning the file   | number          | 1000        |
| user      | name of the user owning the file  | string          | "john"      |
| uid       | id of the user owning the file    | number          | 1000        |
| type      | file type                         | file type       | pipe        |

Additionally you can test these flags:

| Flag       | Description                             |
| :--------- | :-------------------------------------- |
| readable   | the file can be read by the user        |
| writable   | the user can write to the file          |
| executable | the user is allowed to execute the file |


## Running efind

Running **efind** without any argument the search expression is read from *stdin*
and files are searched in the user's home directory. A different directory and
expression can be specified with the *--dir* and *--expr* options:

```
$ efind --dir=/tmp --expr="size>1M and type=file"
```

**efind** tries to handle the first two arguments as path and expression. It's
valid to run **efind** the following way:

```
$ efind ~ "type=dir" --follow
```

If you want to show the translated arguments without running GNU find use the
*--print* option. To quote special shell characters append *--quote*:

```
$ efind ~/tmp/foo 'iname="*.py" and (mtime<30 days or size>=1M)' --print --quote
$ find /home/sf/tmp/foo -iname "*.py" -a \( -mtime -30 -o \( -size 1M -o -size +1M \) \)
```

All available options can be displayed with

```
$ efind --help
```


## Extensions

Extensions are custom functions that can be used to filter GNU find's result. A function can
have optional arguments and returns always an integer. Non-zero values evaluate to true.

Extensions can only be used *after* the find expression. 

At the current stage **efind** can only be extendend with functions loaded from shared libraries.
It's planned to support scripting languages like [Python](https://www.python.org/)
or [GNU Guile](https://www.gnu.org/software/guile/) in the future.

You find two example extensions in the "examples" folder. Install [taglib](http://taglib.org/)
and [GdkPixbuf](https://wiki.gnome.org/Projects/GdkPixbuf) to build both extensions.

On a Debian based distribution the following step is required:

```
$ apt-get install libtagc0-dev libgdk-pixbuf2.0-dev
```

Now you can build the extension modules:

```
$ cd examples
$ make
```

Copy the generated *.so files to ~/.efind/extensions:

```
$ mkdir -p ~/.efind/extensions
$ cp *.so ~/.efind/extensions
```

You can print a list with all available functions found in the installed extensions:

```
$ efind --list-extensions
$ /home/sf/.efind/extensions/gdk-pixbuf.so
$ 	GDK-PixBuf, version 0.1.0
$ 
$ 	Read image data with GDK-PixBuf.
$ 
$ 	image_bits_per_sample()
$ 	image_channels()
$ 	image_has_alpha()
$ 	image_height()
$ 	image_width()
$ 
$ /home/sf/.efind/extensions/taglib.so
$ 	taglib, version 0.1.0
$ 
$ 	Read tags and properties from audio files.
$ 
$ 	album_equals(string)
$ 	album_matches(string)
$ 	artist_equals(string)
$ 	artist_matches(string)
$ 	audio_bitrate()
$ 	audio_channels()
$ 	audio_length()
$ 	audio_samplerate()
$ 	genre_equals(string)
$ 	genre_matches(string)
$ 	title_equals(string)
$ 	title_matches(string)
```

To make extensions available for all users copy them to /etc/efind/extensions.


## Writing own extensions

To write your own extension create a C source file and include the "extension-interface.h" header.

Your library has to provide two functions: registration() and discover().

The registration() function will register name, version and description of your extension:

```
void
registration(RegistrationCtx *ctx, RegisterExtension fn)
{
	fn(ctx, "my extension", "0.1.0", "my first extension");
}
```

The discover() function is used to register exported functions and their signatures:

```
void
discover(RegistrationCtx *ctx, RegisterCallback fn)
{
	fn(ctx, "add_numbers", 2, CALLBACK_ARG_TYPE_INTEGER, CALLBACK_ARG_TYPE_INTEGER);
}

int
add_numbers(const char *filename, int argc, void *argv[])
{
	int *a = (int *)argv[0];
	int *b = (int *)argv[1];

	return *a + *b;
}
```
