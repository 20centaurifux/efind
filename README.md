# efind - README

## Introduction

**efind** (expression find) searches for files in a directory hierarchy.
Basically it's a wrapper for the *find* utility providing an easier
expression syntax.

## Building efind

**efind** is shipped with a small Makefile. To compile the program type
in 'make' and to cleanup the working directory 'make clean'. The source code
documentation can be build with 'make doc'. Kindly note that this step
requires *doxygen*.

Before building **efind** ensure *GNU bison* and *GNU flex* is installed on
your system.

## Expression syntax

At the current stage the following file attributes can be searched:

* name, iname: (case insensitive) filename pattern *(string*)
* atime: last access time *(time inverval, e.g. '10 minutes')*
* ctime: last file status change *(time interval, e.g. '15 hours')* 
* mtime: last modification time *(time interval, e.g. '30 days')*
* size: file size *(size, e.g. '10 megabyte')*
* group, gid: group name/id owning the file *(string, number)*
* user, uid: user name/id owning the file *(string, number)*
* type: file type *(type, e.g. 'socket')*

The following intervals/units are available:

* time intervals: hours, minutes, days (alternatively h, m and d)
* file size: byte, kilobyte, megabyte, gigabyte (alternatively b, k M, G)

These are the supported file types:

* file, directory, block, character, pipe, link, socket

An expression compares at least one attribute to a value. Multiple comparisons
can be combined with the *and* and *or* operators. Use parentheses to force
precedence.

These operators can be used when comparing attributes and values, for instance:

* = equals
* > greater than
* >= greater or equal
* < less than
* <= less or equal

To find all source and header files greater than 100 bytes you can use the
following expression:

	(name="*.h" or name="*.c") and type=file and size>=100 bytes

## Running efind

Running **efind** without any arguments the search expression is read from stdin
and files are searched in the user's home directory. A different directory
and an expression can be specified with the *--dir* and *--expr* options:

# efind --dir=/tmp --expr="size>1M and type=file"

If you only want to show the translated arguments without running find please
add the *--print* option. To quote special shell characters use *--quote*.

	echo '(name="*.h" or name="*.c") and mtime<30 days' | efind --quote --print
