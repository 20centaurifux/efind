# efind - README

## Overview

**efind** (extendable find) searches for files in a directory hierarchy.

Basically it's a wrapper for [GNU find](https://www.gnu.org/software/findutils/) providing an easier and more intuitive
expression syntax. It can be extended by custom functions to filter search
results.

## A quick example

Let's assume you want to find all MP3 (\*.mp3) and Ogg Vorbis (\*.ogg) files
that were modified less than two days ago in your music folder. That's no
problem with **efind's** self-explanatory expression syntax:

	$ efind ~/music '(name="*.mp3" or name="*.ogg") and mtime<2 days'

Additionally you can filter the search result by audio tags and properties with
the [taglib](https://github.com/20centaurifux/efind-taglib) extension:

	$ efind ~/music '(name="*.mp3" or name="*.ogg") and mtime<2 days \
	  and artist_matches("Welle: Erdball") and audio_length()>200'

## General Usage

Running **efind** without any argument the search expression is read from
standard input and files are searched in the user's home directory.
A different directory and expression can be specified with the --dir
and --expr options:

	$ efind --dir=/tmp --expr="size>1M and type=file"

**efind** tries to handle the first two arguments as path and expression. It's
valid to run **efind** the following way:

	$ efind ~/git 'type=file and name="CHANGELOG"'

If you want to show the translated arguments without running GNU find use the --print
option. To quote special shell characters append --quote:

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
| <=       | less or equal    |

Use the not operator to test if an expression evaluates to logical false.

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

## Advanced Features

### Output Format

You can change the output format of **efind** with the --printf option.

The following escape sequences and directives are available:

#### Escape Sequences

| Sequence        | Description                                    |
| :-------------- | :----------------------------------------------|
| \\a             | Alarm bell.                                    |
| \\b             | Backspace.                                     |
| \\f             | Form feed.                                     |
| \\n             | Newline.                                       |
| \\r             | Carriage return.                               |
| \\t             | Horizontal tab.                                |
| \\v             | Vertical tab.                                  |
| \\0             | ASCII NUL.                                     |
| \\\\            | A literal blackslash.                          |
| \\NNN           | The character whose ASCII code is NNN (octal). |
| \\XNN           | The character whose ASCII code is NN (hex).    |

Any other character following a \\ is printed.

#### Directives

| Directive       | Description                                                                                             |
| :-------------- | :-------------------------------------------------------------------------------------------------------|
| %%              | A literal percent sign.                                                                                 |
| %Ak             | File's last access time in the format specified by k or in the format returned by the C ctime function. |
| %b              | The amount of disk space used for this file in 512-byte blocks.                                         |
| %Ck             | File's last status change time in the format specified by k, which is the same as for %A.               |
| %D              | The device number on which the file exists (the st\_dev field of struct stat) in decimal.               |
| %f              | File's name with any leading directories removed (only the last element).                               |
| %F              | Type of the filesystem the file is on.                                                                  |
| %g              | File's group name, or numeric group ID if the group has no name.                                        |
| %G              | File's numeric group ID.                                                                                |
| %h              | Leading  directories of file's name (all but the last element).                                         |
| %H              | Starting-point under which file was found.                                                              |
| %i              | File's inode number (in decimal).                                                                       |
| %k              | The amount of disk space used for this file in 1K blocks.                                               |
| %l              | Object of symbolic link (empty string if file is not a symbolic link).                                  |
| %m              | File's permission bits (in octal).                                                                      |
| %M              | File's permissions (in symbolic form, as for ls).                                                       |
| %n              | Number of hard links to file.                                                                           |
| %p              | File's name.                                                                                            |
| %P              | File's name with the name of the starting-point under which it was found removed.                       |
| %s              | File's size in bytes.                                                                                   |
| %S              | File's sparseness. If the file size is zero, the value printed is undefined.                            |
| %Tk             | File's last modification time in the format specified by k, which is the same as for %A.                |
| %u              | File's user name, or numeric user ID if the user has no name.                                           |
| %U              | File's numeric user ID.                                                                                 |

#### Time/Date fields

Available time and date fields are

| Field       | Description                                                       |
| :---------- | :-----------------------------------------------------------------|
| H           | hour (00..23)                                                     |
| I           | hour (01..12)                                                     |
| k           | hour (0..23)                                                      |
| l           | hour (1..12)                                                      |
| M           | minute (00..59)                                                   |
| p           | locale's AM or PM                                                 |
| r           | time, 12-hour (hh:mm:ss AM\|PM)                                   |
| S           | second (00..60)                                                   |
| T           | time, 24-hour (hh:mm:ss)                                          |
| X           | locale's time representation (H:M:S)                              |
| Z           | time zone (e.g. EDT), or nothing if no time zone is determinable  |
| a           | locale's abbreviated weekday name (Sun..Sat)                      |
| A           | locale's full weekday name, variable length (Sunday..Saturday)    |
| b           | locale's abbreviated month name (Jan..Dec)                        |
| B           | locale's full month name, variable length (January..December)     |
| c           | locale's date and time (Sat Nov 04 12:02:33 EST 1989)             |
| d           | day of month (01..31)                                             |
| D           | date (mm/dd/yy)                                                   |
| h           | same as b                                                         |
| j           | day of year (001..366)                                            |
| m           | month (01..12)                                                    |
| U           | week number of year with Sunday as first day of week (00..53)     |
| w           | day of week (0..6)                                                |
| W           | week number of year with Monday as first day of week (00..53)     |
| x           | locale's date representation (mm/dd/yy)                           |
| y           | last two digits of year (00..99)                                  |
| Y           | year (1970...)                                                    |

### Sorting

You can use the same directives to sort the search result as with the --printf option.
Prepend a minus sign to change direction:

	$ efind . "type=file" --order-by "-sp"

## Differences to GNU find

Sometimes GNU find doesn't behave in a way an average user would expect. The following
expression finds all documents in the current folder with a file size less or equal than
1G because every file with at least *one byte* is rounded up:

	$ find . -size 1G

**efind** converts file sizes to byte to avoid this confusing behaviour:

	$ efind . "size=1G" --print
	$ find . -size 1073741824c

**efind's** --printf option is not fully compatible with GNU find:

* In contrast to GNU find numeric values like file size or group id are *not* converted
  to string. This means that all number related flags work with **efind**.
* Width and precision are interpreted *exactly the same way* as the printf C function does.
* The fields %a, %c and %t print the timestamp in seconds.
* Date format strings are not limited to a single field. The string "%AHMS" prints hour,
  minute and second of the last file access, for example.
* **efind's** printf format supports user-friendly field names like "{path}" or "{group}".
* When printing an undefined escape sequence (e.g. "\P") only the character following the
  backslash is printed.
