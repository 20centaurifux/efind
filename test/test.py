"""
	project............: efind
	description........: efind test suite.
	copyright..........: Sebastian Fedrau

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License v3 as published by
	the Free Software Foundation.

	This program is distributed in the hope that it will be useful, but
	WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
	General Public License v3 for more details.
"""
import subprocess, os, random, string

def test_search(argv, expected, success=True):
	cmd = ["efind", "test-data"] + argv

	print("Running efind, argv=[%s]" % ", ".join(cmd[1:]))

	proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	result = filter(lambda l: l != "", str(proc.stdout.read()).split("\n"))
	proc.wait()

	if not expected is None:
		assert(set(result) == set(expected))

	assert((success and proc.returncode == 0) or (not success and proc.returncode != 0))

def test_printf(path, printf):
    # run find:
    cmd = ["find", "./test-data", "-size", "+0", "-printf", printf]

    print("Running find, argv=[%s]" % ", ".join(cmd[1:]))

    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    find_result = filter(lambda l: l != "", str(proc.stdout.read()).split("\n"))
    proc.wait()

    # run efind:
    cmd = ["efind", "./test-data", "size>=0", "--printf", printf]

    print("Running efind, argv=[%s]" % ", ".join(cmd[1:]))

    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    efind_result = filter(lambda l: l != "", str(proc.stdout.read()).split("\n"))
    proc.wait()

    assert(set(find_result) == set(efind_result))

def id(arg):
	proc = subprocess.Popen(["id", arg], stdout=subprocess.PIPE)

	return proc.stdout.read().strip()

def random_string(length=32):
	return "".join(random.choice(string.ascii_uppercase + string.digits) for _ in range(length))

SEARCH_ARGS = [[['--version'], None],
               [['--help'], None],
               [['--list-extensions'], None],
               [['--list-extensions', '--log-level=6'], None],
               [['--list-extensions', '--log-level=3', '--enable-log-color'], None],
               [['--version', '--log-level=abc', '--enable-log-color'], None],
               [['--show-blacklist', '--log-level=5', '--enable-log-color'], None],
               [['size>0', '--print', '--quote'], None],
               [['size=720', '-L'], ["test-data/02/720b.2"]],
               [['size>=720 and size<=5k and type=file'], ["test-data/00/1kb.0", "test-data/00/5kb.0", "test-data/02/5kb.2", "test-data/02/720b.2"]],
               [['size<100b and type=file', '--order-by', 'P-fGu'], ["test-data/01/5b.1"]],
               [['type=file and size<=100b and group="%s"' % (id('-gn'))], ["test-data/00/100b.0", "test-data/01/5b.1"]],
               [['type=file and size=5M or (size>=1G and name="*.1")', '--order-by', '-psC-AF'], ["test-data/01/2G.1", "test-data/01/5M.1", "test-data/02/5M.2"]],
               [['size=1G and (readable or writable)'], ["test-data/00/1G.0", "test-data/02/1G.2"]],
               [['type=directory and iname="00" and gid=%s and readable' % (id('-g'))], ["test-data/00"]],
               [['--expr', 'type=directory and uid=%s' % (id('-u')), '--max-depth=0'], ["test-data"]],
               [['mtime<=5days and mtime>=1day and type=file'], ["test-data/00/1M.0", "test-data/01/15kb.1"]],
               [['mtime=6days'], ["test-data/02/20M.2"]],
               [['mtime=4days and name=""'], []],
               [['mtime>96hours'], ["test-data/00/1M.0", "test-data/02/20M.2"]],
               [['atime>2days and user="%s"' % (id('-un'))], ["test-data/02/5kb.2"]],
               [['atime>=24hours and readable and name="*2*"'], ["test-data/01/2G.1", "test-data/02/5kb.2"]],
               [['name_equals("test-data/02/5M.2") and (sub(17, 3) and add(192, 19))'], ["test-data/02/5M.2"]],
               [['not size >= 1kb and not (sub(1, 1) or not add(100, 75))'], ["test-data/00/100b.0", "test-data/01/5b.1", "test-data/02/720b.2"]]]

INVALID_SEARCH_ARGS = [['ctime>5bytes and writable'],
                       ['size=1G', '--order-by', random_string(8192)],
                       ['name="foobar" and uid=8days'],
                       ['type=socket or ('],
                       ['name or readable'],
                       [random_string()],
                       ['gid=%s' % (random_string())],
                       ['iname="%s' % (random_string(64))],
                       ['type=socket and iname="%s"' % (random_string(4096))],
                       ['size>=%s' % (random_string(2048))]]

PRINTF_ARGS = ["%b %20p%-#0P<%5s> USER: %u \x43\x052 USER ID: %U\n",
               "%p => %Ca%CA%Cb%CB%Cd|%TD%Th%Tj%Tm\052\a\0532%TU%Tw\v%TW[%Ty]|%TY %h '%023H' |%l| %m %#m %M\n",
               "FI{{%p}}LE %b %% %20p%-#0P<%5s> USER: %0500u\tUSER ID: %U\n",
               "",
               random_string(64),
               random_string(2048) + "\n"]

if __name__ == "__main__":
	for argv, expected in SEARCH_ARGS:
		test_search(argv, expected)

	for argv in INVALID_SEARCH_ARGS:
		test_search(argv, None, False)

        for arg in PRINTF_ARGS:
            for dir in ["./test-data", "./test-data/", ".", "./"]:
                test_printf(dir, arg)
