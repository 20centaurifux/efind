# efind - README

## Introduction

**efind** (expression find) searches for files in a directory hierarchy.
Basically it's a wrapper for *GNU find* providing an easier expression syntax.

## Building efind

**efind** is shipped with a small Makefile. To compile and install the program
type in *make* and *make install*. The source code documentation can be build
with *make doc*. Kindly note that this step requires *doxygen*. Installation
options can be customized in the Makefile.

Before building **efind** ensure *GNU Bison* and *GNU Flex* is installed on
your system. You also need *libdatatypes-0.1*
(https://github.com/20centaurifux/datatypes).

## Expression syntax

A search expression consists of at least one comparison or flag to test.
Multiple comparisons and flags can be combined with the *and* and *or*
operators. Use parentheses to force precedence.

The following operators are supported by **efind** when comparing a file
attribute to a value:

* = equals
* > greater than
* >= greater or equal
* < less than
* <= less or equal

Values can be a strings, numbers, time intervals, sizes and file types.

Time intervals consist of a number and an unit. Supported units are
*minute(s)*, *hour(s)* and *day(s)*.

A file size consist of a number and an unit too. *byte(s)*, *kilobyte(s)*,
*megabyte(s)* and *gigabyte(s)* can be used to specify the unit.

Allowed file types are *file*, *directory*, *block*, *character*, *pipe*,
*link* and *socket*.

At the current stage the following file attributes can be searched:

* (i)name: (case insensitive) filename pattern *(string)*
* atime: last access time *(time inverval, e.g. '1 minute')*
* ctime: last file status change *(time interval, e.g. '15 hours')* 
* mtime: last modification time *(time interval, e.g. '30 days')*
* size: file size *(size, e.g. '10 megabyte')*
* group, gid: group name/id owning the file *(string, number)*
* user, uid: user name/id owning the file *(string, number)*
* type: file type *(type, e.g. 'socket')*

**efind** supports the following flags:

* readable: the file can be read by the user
* writable: the user can write to the file
* executable: the user is allowed to execute the file

## Expression example

To find all source and header files of a project (written in C) greater than
100 bytes one could use the expression below:

	(name="*.h" or name="*.c") and type=file and size>=100 bytes

## Running efind

Running **efind** without any arguments the search expression is read from
*stdin* and files are searched in the user's home directory. A different
directory and expression can be specified with the *--dir* and *--expr*
options:

	efind --dir=/tmp --expr="size>1M and type=file"

If you want to show the translated arguments without running *GNU find* please
use the *--print* option. To quote special shell characters append *--quote*:

	echo 'name="*.py" and mtime<30 days' | efind --quote --print

To show additional options type in

	efind --help

## Notes

The development of **efind** is at a very early stage. It has been tested only
on *x86_64* with *GNU find 4.4.2*, *GNU Bison 2.7* *GNU Flex 2.5.3* and *gcc
4.8.4*.
