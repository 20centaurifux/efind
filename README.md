# efind

## Introduction

**efind** (expression find) searches for files in a directory hierarchy.
Basically, it's a wrapper for GNU find providing an easier and more
intuitive expression syntax.

## Building efind

**efind** uses GNU Make as build system. To compile and install the program run

```
$ make
$ sudo make install
```

Installation options can be customized in the Makefile.

Please ensure that GNU Bison and GNU Flex is installed on your system before
you build **efind**. You will also need [libdatatypes](https://github.com/20centaurifux/datatypes)
and [libbsd](https://libbsd.freedesktop.org/wiki/).

If you want to generate the source code documentation type in

```
$ make doc
```

This step requires Doxygen.

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

## Expression example

To find all writable source and header files of a C project that are greater than 100 bytes you could
use the following expression:

```
(name="*.h" or name="*.c") and type=file and size>100 bytes and writable
```

## Running efind

Running **efind** without any argument the search expression is read from *stdin*
and files are searched in the user's home directory. A different directory and
expression can be specified with the *--dir* and *--expr* options:

```
$ efind --dir=/tmp --expr="size>1M and type=file"
```

If you want to show the translated arguments without running GNU find use the
*--print* option. To quote special shell characters append *--quote*:

```
$ echo 'name="*.py" and mtime<30 days' | efind --print --quote
```

The available options can be displayed with

```
$ efind --help
```
