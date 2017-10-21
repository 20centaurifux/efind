#!/bin/bash
#
#  efind test suite.
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License v3 as published by
#  the Free Software Foundation.
#
#  This program is distributed in the hope that it will be useful, but
#  WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
#  General Public License v3 for more details.

PYTHON=/usr/bin/python2

generate_test_files()
{
	echo "Generating test files."

	# does test data directory already exist?
	if [ ! -d ./test-data ]; then
		# directory not found => create test files:
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

	# set timestamps:
	find ./test-data -exec touch {} \;
	touch -d "now - 5 days" -m ./test-data/00/1M.0
	touch -d "now - 3 days" -m ./test-data/01/15kb.1
	touch -d "now - 6 days" -m ./test-data/02/20M.2
	touch -d "now - 2 days" -a ./test-data/01/2G.1
	touch -d "now - 5 days" -a ./test-data/02/5kb.2
}

run_test()
{
	# run Python script:
	echo "Running test suite."
        $PYTHON ./test.py
}

cleanup()
{
	# delete generated test files:
	echo "Deleting test files."
	rm -fr ./test-data
}

generate_test_files && EFIND_EXTENSION_PATH=./extensions run_test && cleanup
