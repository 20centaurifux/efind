# efind - README

## Overview

**efind** (extendable find) searches for files in a directory hierarchy.

Basically it's a wrapper for [GNU find](https://www.gnu.org/software/findutils/)
providing an easier and more intuitive expression syntax. It can be extended
by custom functions to filter search results.

## A quick example

Let's assume you want to find all MP3 (\*.mp3) and Ogg Vorbis (\*.ogg) files
that were modified less than two days ago in your music folder. That's no
problem with **efind's** self-explanatory expression syntax:

	$ efind ~/music '(name="*.mp3" or name="*.ogg") and mtime<2 days'

Additionally you can filter the search result by audio tags and properties with
the [taglib](https://github.com/20centaurifux/efind-taglib) extension:

	$ efind ~/music '(name="*.mp3" or name="*.ogg") and mtime<2 days \
	  and artist_matches("Welle: Erdball") and audio_length()>200'

## Usage

Running **efind** without any argument the search expression is read from
*standard input (stdin)* and files are searched in the user's home directory.
A different directory and expression can be specified with the *--dir*
and *--expr* options:

	$ efind --dir=/tmp --expr="size>1M and type=file"

**efind** tries to handle the first two arguments as path and expression. It's
valid to run **efind** the following way:

	$ efind ~/git 'type=file and name="CHANGELOG"'

If you want to show the translated arguments without running GNU find use the
*--print* option. To quote special shell characters append *--quote*:

	$ efind . 'iregex=".*\.txt" and writable' --print --quote

**efind** is shipped with a manpage, of course.

	$ man efind

## Expression Syntax

A search expression consists of at least one comparison or file flag to test. Multiple
expressions can be evaluated by using conditional operators:

| Operator | Description                                                                                                                                                   |
| :------- | :------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| and      | If an expression returns logical false it returns that value and doesn't evaluate the next expression. Otherwise it returns the value of the last expression. |
| or       | If an expression returns logical true it returns that value and doesn't evaluate the next expression. Otherwise it returns the value of the last expression.  |

Expressions are evaluated from left to right. Use parentheses to force precedence.

The following operators can be used to compare a file attribute to a value:

| Operator | Description      |
| :------- | :--------------- |
| =        | equals to        |
| >        | greater than     |
| >=       | greater or equal |
| <        | less than        |
| <=       | less or equal    |

A value must be of one of the data types listed below:

| Type          | Description                                                                                                            |
| :------------ | :--------------------------------------------------------------------------------------------------------------------- |
| string        | Quoted sequence of characters.                                                                                         |
| number        | Whole number.                                                                                                          |
| time interval | Time interval (number) with suffix. Supported suffixes are "minute(s)", "hour(s)" and "day(s)".                        |
| file size     | Units of space (number) with suffix. Supported suffixes are "byte(s)", "kilobyte(s)", "megabyte(s)" and "gigabyte(s)". |
| file type     | "file", "directory", "block", "character", "pipe", "link" or "socket".                                                 |

The following file attributes are searchable:

| Attribute  | Description                           | Type            | Example     |
| :--------- | :------------------------------------ | :-------------- | :---------- |
| name       | case sensitive filename pattern       | string          | "*.txt"     |
| iname      | case insensitive filename pattern     | string          | "Foo.bar"   |
| regex      | case sensitive regular expression     | string          | ".*\\.html" |
| iregex     | case insensitive regular expression   | string          | ".*\\.TxT"  |
| atime      | last access time                      | time interval   | 1 minute    |
| ctime      | last file status change               | time interval   | 15 hours    |
| mtime      | last modification time                | time interval   | 30 days     |
| size       | file size                             | size            | 10 megabyte |
| group      | name of the group owning the file     | string          | "users"     |
| gid        | id of the group owning the file       | number          | 1000        |
| user       | name of the user owning the file      | string          | "john"      |
| uid        | id of the user owning the file        | number          | 1000        |
| type       | file type                             | file type       | pipe        |
| filesystem | name of the filesystem the file is on | string          | "ext4"      |

Additionally you can test these flags:

| Flag       | Description                                                   |
| :--------- | :------------------------------------------------------------ |
| readable   | the file can be read by the user                              |
| writable   | the user can write to the file                                |
| executable | the user is allowed to execute the file                       |
| empty      | the file is empty and is either a regular file or a directory |

## Differences to GNU find

Sometimes GNU find doesn't behave in a way an average user would expect. The following
expression finds all documents in the current folder with a file size less or equal than
1G because every file with at least *one byte* is rounded up:

	$ find . -size 1G

**efind** converts file sizes to byte to avoid this confusing behaviour:

	$ efind . "size=1G" --print
	$ find . -size 1073741824c

**efind's** *--printf* option is not fully compatible with GNU find:

* In contrast to GNU find numeric values like file size or group id are *not* converted
  to string. This means that all number related flags work with **efind**.
* Width and precision are interpreted *exactly the same way* as the printf C function does.
* The fields %a, %c and %t are not available. To print a date string in the
  format returned by the "ctime" C function use %A, %C or %T without a format string.
* Date format strings are not limited to a single field. The string "%AHMS" prints hour,
  minute and second of the last file access, for example.
* When printing an undefined escape sequence (e.g. "\P") only the character following the
  backslash is printed.

## Planned features

* not operator
