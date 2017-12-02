# efind test suite - README

## Overview

This suite tests **efind** by running multiple Python unit tests.
It tests the first executable named "efind" found in one of the
directories specified in your $PATH environment variable.

## Running the default test suite

Start the "run.sh" script to generate required test files and run
the unit tests:

	$ ./run.sh

This will not affect any files outside of the "test" directory.

If the script should abort without removing the generated test
files you can delete them with the "cleanup" option:

	$ ./run.sh cleanup

## Running the full test suite

Some tests need write access to your "/etc" directory. They can
be executed in a special root directory using the chroot utility.
Unfortunately this requires root privileges.

To run all available tests setup a chroot environment. Then execute
the test script the following way:

	$ CHROOT_PATH=/path/to/root/directory ./run.sh
