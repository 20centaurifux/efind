#!/bin/bash
#
#  project............: efind
#  description........: efind test suite.
#  copyright..........: Sebastian Fedrau
#  email..............: sebastian.fedrau@gmail.com
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License v3 as published by
#  the Free Software Foundation.
#
#  This program is distributed in the hope that it will be useful, but
#  WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
#  General Public License v3 for more details.

set -e

PYTHON=python3

generate_test_files()
{
	echo "Generating test files."

	# generate regular files:
	if [ ! -d ./test-data ]; then
		mkdir -p test-data/00 test-data/01 test-data/02

		truncate ./test-data/00/100b.0 -s100
		truncate ./test-data/00/1kb.0 -s1K
		truncate ./test-data/00/5kb.0 -s5K
		truncate ./test-data/00/10kb.0 -s10K
		truncate ./test-data/00/1M.0 -s1M
		truncate ./test-data/00/20M.0 -s20M
		truncate ./test-data/00/1G.0 -s1G

		truncate ./test-data/01/5b.1 -s5
		truncate ./test-data/01/15kb.1 -s15K
		truncate ./test-data/01/10kb.1 -s10K
		truncate ./test-data/01/5M.1 -s5M
		truncate ./test-data/01/2G.1 -s2G

		truncate ./test-data/02/720b.2 -s720
		truncate ./test-data/02/5kb.2 -s5K
		truncate ./test-data/02/7kb.2 -s7K
		truncate ./test-data/02/5M.2 -s5M
		truncate ./test-data/02/20M.2 -s20M
		truncate ./test-data/02/1G.2 -s1G
	fi

	# create symlinks:
	mkdir ./test-links
	ln -s ../test-data/00 ./test-links/00
	ln -s ../test-data/02 ./test-links/02
}

build_extensions()
{
	make -C ./extensions
}

run_testsuite()
{
	echo "Running test suite."
	LC_ALL=C $PYTHON ./testsuite.py
}

run()
{
	generate_test_files && build_extensions && run_testsuite && cleanup
}

cleanup()
{
	echo "Deleting test files."
	rm -fr ./test-data ./test-links ./fake-dirs

	make -C ./extensions clean
}

case "$1" in
	""|tests)
		run
		;;
	clean)
		cleanup
		;;
	*)
		echo "Usage: ${0} {tests|clean}"
		exit 1
		;;

esac
