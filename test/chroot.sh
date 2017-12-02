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

set -e

BINARY=$(which efind)

function install_binary()
{
	local path=$1

	echo "Installing efind in chroot."

	cp "$BINARY" "$path/usr/bin"
}

function install_etc()
{
	local path=$1

	echo "Installing /etc/efind in chroot."

	mkdir -p "$path/etc/efind/extensions"
}

function uninstall()
{
	local path=$1

	echo "Removing efind from chroot."

	rm -f "$path/usr/bin/efind"
	rm -fr "$path/etc/efind"
	rm -fr "$path/root/.efind"
}

function install_extension()
{
	local path=$1
	local filename=$2
	local location=$3
	local dst="/etc/efind/extensions"

	if [ "$location" == "--local" ]; then
		dst="/root/.efind/extensions"
	fi

	if [ ! -d "$path$dst" ]; then
		mkdir -p "$path$dst"
	fi

	cp "./extensions/$filename" "$path$dst/"

	echo "Installed extension '$filename' in directory '$dst'."
}

function install_blacklist()
{
	local path=$1
	local filename=$2

	if [ ! -d "$path/root/.efind" ]; then
		mkdir -p "$path/root/.efind"
	fi

	cp "./$filename" "$path/root/.efind/blacklist"

	echo "Installed new blacklist."
}

function test_settings()
{
	if [[ -z "$BINARY" ]]; then
		echo "'efind' binary not found."
		exit 1
	fi

	if [[ -z "$1" ]]; then
		echo "Path to chroot directory can't be empty.'"
		exit 1
	fi

	if [[ ! -d "$1" ]]; then
		echo "Path to chroot directory is invalid."
		exit 1
	fi
}

test_settings $1

case "$2" in
	install-binary)
		install_binary $1
		;;
	install-etc)
		install_etc $1
		;;
	install-blacklist)
		install_blacklist $1 $3
		;;
	uninstall)
		uninstall $1
		;;
	install-extension)
		install_extension $1 $3 $4
		;;
	*)
		echo "No valid action specified. Supported actions are: install-binary|install-etc|install-extension|install-blacklist|uninstall"
		exit 1
		;;
esac
