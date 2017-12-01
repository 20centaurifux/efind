#!/bin/sh

CHROOT=./chroot
BINARY=../efind

function install_binary()
{
	echo "Installing efind in chroot."

	cp "$BINARY" "$CHROOT/usr/bin"
}

function install_etc()
{
	echo "Installing /etc/efind in chroot."

	mkdir -p "$CHROOT/etc/efind/extensions"
}

function uninstall()
{
	echo "Removing efind from chroot."

	rm -f "$CHROOT/usr/bin/efind"
	rm -fr "$CHROOT/etc/efind"
	rm -fr "$CHROOT/root/.efind"
}

function install_extension()
{
	local filename=$1
	local location=$2
	local dst="/etc/efind/extensions"

	if [ "$location" == "--local" ]; then
		dst="/root/.efind/extensions"
	fi

	if [ ! -d "$CHROOT$dst" ]; then
		mkdir -p "$CHROOT$dst"
	fi

	cp "./extensions/$filename" "$CHROOT$dst/"

	echo "Installed extension '$filename' in directory '$dst'."
}

function install_blacklist()
{
	local filename=$1

	if [ ! -d "$CHROOT/root/.efind" ]; then
		mkdir -p "$CHROOT/root/.efind"
	fi

	cp "./$filename" "$CHROOT/root/.efind/blacklist"

	echo "Installed new blacklist."
}

function append_to_blacklist()
{
	if [ ! -d "$CHROOT/root/.efind" ]; then
		mkdir "$CHROOT/root/.efind"
	fi

	echo $1 >> "$CHROOT/root/.efind/blacklist"
}

case "$1" in
	install-binary)
		install_binary
		;;
	install-etc)
		install_etc
		;;
	install-blacklist)
		install_blacklist $2
		;;
	uninstall)
		uninstall
		;;
	install-extension)
		install_extension $2 $3
		;;
	*)
		echo $"Usage: $0 {install-binary|install-etc|install_extension|install-blacklist|uninstall}"
		exit 1
		;;
esac
