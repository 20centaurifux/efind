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
from testhelpers import *
import unittest, random, os, os.path, shutil, sys, stat

class InvalidArgs(unittest.TestCase):
    def test_invalid_args(self):
        for length in range(10, 10000, 1000):
            returncode, _ = run_executable("efind", ["--" + random_string(length)])

        assert(returncode == 1)

class PrintInfo(unittest.TestCase):
    def test_print_version(self):
        self.__run_long_and_short_option__("--version", "-v")
        self.__run_long_and_short_option__("--version", "-v", optional_args=self.__make_log_level_argv__())
        self.__run_long_and_short_option__("--version", "-v", optional_args=self.__make_log_level_argv_with_color__())

    def test_print_help(self):
        self.__run_long_and_short_option__("--help", "-h")
        self.__run_long_and_short_option__("--help", "-h", optional_args=self.__make_log_level_argv__())
        self.__run_long_and_short_option__("--help", "-h", optional_args=self.__make_log_level_argv_with_color__())

    def __run_long_and_short_option__(self, arg_long, arg_short, optional_args=[]):
        returncode, output_long = run_executable_and_split_output("efind", [arg_long] + optional_args)
        assert(returncode == 0)

        returncode, output_short = run_executable_and_split_output("efind", [arg_short] + optional_args)
        assert(returncode == 0)

        assert_sequence_equality(output_long, output_short)

    def __make_log_level_argv__(self):
        return ["--log-level=%d" % random.randint(0, 6)]

    def __make_log_level_argv_with_color__(self):
        return ["--enable-log-color"] + self.__make_log_level_argv__()

class AttributeTester():
    def __init__(self, search_dir):
        self.__search_dir = search_dir

    def test_string(self, attr, expected_code=1):
        returncode, _ = run_executable("efind", [self.__search_dir, '%s="%s"' % (attr, random_string())])

        assert(returncode == expected_code)

    def test_integer(self, attr, min=99999, max=9999999, expected_code=1):
        returncode, _ = run_executable("efind", [self.__search_dir, '%s=%d' % (attr, random.randint(min, max))])

        assert(returncode == expected_code)

    def test_time(self, attr, expected_code=1):
        units = ["minute", "minutes", "hour", "hours", "day", "days"]

        returncode, _ = run_executable("efind", [self.__search_dir, '%s=%d %s' % (attr, random.randint(1, 100), random.choice(units))])

        assert(returncode ==  expected_code)

    def test_size(self, attr, expected_code=1):
        units = ["byte", "bytes", "b", "kilobyte", "kilobytes", "kb", "k",
                 "megabyte", "megabytes", "mb", "M", "gigabyte", "gigabytes",
                 "G", "g"]

        returncode, _ = run_executable("efind", [self.__search_dir, '%s=%d %s' % (attr, random.randint(1, 100), random.choice(units))])

        assert(returncode ==  expected_code)

    def test_filetype(self, attr, expected_code=1):
        types = ["file", "directory", "block", "character", "pipe", "link", "socket"]

        returncode, _ = run_executable("efind", [self.__search_dir, '%s=%s' % (attr, random.choice(types))])

        assert(returncode ==  expected_code)

    def test_flag(self, attr, expected_code=1):
        flags = ["readable", "writable", "executable", "empty"]

        returncode, _ = run_executable("efind", [self.__search_dir, '%s=%s' % (attr, random.choice(flags))])
        assert(returncode ==  expected_code)

        returncode, _ = run_executable("efind", [self.__search_dir, '%s' % attr])
        assert(returncode ==  expected_code)

class AssertSearch():
    def __assert_search__(self, args, expected_files):
        returncode, output = run_executable_and_split_output("efind", args)

        assert(returncode == 0)
        assert_sequence_equality(output, expected_files)

class FileNameAttributes(unittest.TestCase, AssertSearch):
    def test_name(self):
        self.__assert_search__(['./test-data', 'name="*720b.2*"'], ["./test-data/02/720b.2"])

    def test_iname(self):
        self.__assert_search__(['./test-data', 'iname="*2?m*"'], ["./test-data/00/20M.0", "./test-data/02/20M.2"])

    def test_invalid_values(self):
        t = AttributeTester("./test-data")

        for attr in ["name", "iname"]:
            t.test_integer(attr)
            t.test_size(attr)
            t.test_time(attr)
            t.test_filetype(attr)
            t.test_flag(attr)

class TimeAttributes(unittest.TestCase, AssertSearch):
    def setUp(self):
        os.mkdir("./test-time")

        character = 97
        interval = 60

        for offset in [-5, -3, -2]:
            self.__create_file("atime-%c" % chr(character), offset * interval, 0)
            interval = self.__next_interval__(interval)
            character += 1

        for offset in [-3, -2, -6]:
            self.__create_file("mtime-%c" % chr(character), 0, offset * interval)
            interval = self.__next_interval__(interval)
            character += 1

    def tearDown(self):
        shutil.rmtree("./test-time")

    def test_atime(self):
        self.__assert_search__(['./test-time', 'atime>=5 minutes and atime<1hour"'], ['./test-time/atime-a'])
        self.__assert_search__(['./test-time', 'atime>=3 hours and atime<1day"'], ['./test-time/atime-b'])
        self.__assert_search__(['./test-time', 'atime>=2 days'], ['./test-time/atime-c'])

    def test_mtime(self):
        self.__assert_search__(['./test-time', 'mtime<=4 minutes and mtime>1minute"'], ['./test-time/mtime-d'])
        self.__assert_search__(['./test-time', 'mtime<=3 hours and mtime>1hour"'], ['./test-time/mtime-e'])
        self.__assert_search__(['./test-time', 'mtime<=7 days and mtime>1day"'], ['./test-time/mtime-f'])

    def test_ctime(self):
        self.__assert_search__(['./test-time', 'type=file and ctime<=1minute and ctime<2hours and ctime<= 1day'],
                               ['./test-time/atime-a', './test-time/atime-b', './test-time/atime-c',
                                './test-time/mtime-d', './test-time/mtime-e', './test-time/mtime-f'])

        self.__assert_search__(['./test-time', 'type=file and ctime<=0'], [])

    def test_invalid_values(self):
        t = AttributeTester("./test-data")

        for attr in ["atime", "ctime", "mtime"]:
            t.test_string(attr)
            t.test_size(attr)
            t.test_filetype(attr)
            t.test_flag(attr)

    def __create_file(self, name, atime_offset, mtime_offset):
        path = os.path.join("./test-time", name)

        open(path, "w").close()

        st = os.stat(path)

        atime = st[stat.ST_ATIME] + atime_offset
        mtime = st[stat.ST_MTIME] + mtime_offset

        os.utime(path, (atime, mtime))

    def __next_interval__(self, interval):
        if interval == 60:
            interval *= 60
        elif interval == 60 * 60:
            interval *= 24
        else:
            interval = 60

        return interval

class RegexAttributes(unittest.TestCase, AssertSearch):
    def test_regex(self):
        self.__assert_search__(['./test-data', 'regex="\./test.*M\.[1-2]"'],
                               ["./test-data/01/5M.1", "./test-data/02/5M.2", "./test-data/02/20M.2"])

        self.__assert_search__(['./test-data', 'regex="\./test.*M\.[1-2]"', '--regex-type', "emacs"],
                               ["./test-data/01/5M.1", "./test-data/02/5M.2", "./test-data/02/20M.2"])
    def test_iregex(self):
        self.__assert_search__(['./test-data', 'iregex="\./TEST.*m\.[1-2]"'],
                               ["./test-data/01/5M.1", "./test-data/02/5M.2", "./test-data/02/20M.2"])

        self.__assert_search__(['./test-data', 'iregex="\./TEST.*m\.[1-2]"', '--regex-type', "emacs"],
                               ["./test-data/01/5M.1", "./test-data/02/5M.2", "./test-data/02/20M.2"])

    def test_regex_types(self):
        for regex_type in ["awk", "emacs", "gnu-awk", "egrep", "posix-awk", "posix-egrep", "posix-extended"]:
            self.__assert_search__(['./test-data', 'regex="^\./test\-dat[a|b|c].*2+G\.[0-2]$"', '--regex-type', regex_type],
                                   ["./test-data/01/2G.1"])

        for regex_type in ["ed", "grep", "sed", "posix-minimal-basic"]:
            self.__assert_search__(['./test-data', 'regex="^\./test\-dat[a|b|c].*2+G\.[0-2]$"', '--regex-type', regex_type],
                                   [])

        for regex_type in ["awk", "ed", "egrep", "emacs", "grep", "gnu-awk", "posix-awk", "posix-egrep", "posix-extended", "posix-minimal-basic", "sed"]:
            self.__assert_search__(['./test-data', 'regex=".*\.1"', '--regex-type', regex_type],
                                   ["./test-data/01/2G.1", "./test-data/01/5M.1", "./test-data/01/15kb.1", "./test-data/01/5b.1", "./test-data/01/10kb.1"])

    def test_unsupported_regex_type(self):
        returncode, _ = run_executable_and_split_output("efind", ['./test-data', 'regex=".*\.1"', '--regex-type', random_string()])
        assert(returncode == 1)

    def test_invalid_values(self):
        t = AttributeTester("./test-data")

        for attr in ["regex", "iregex"]:
            t.test_integer(attr)
            t.test_size(attr)
            t.test_time(attr)
            t.test_filetype(attr)
            t.test_flag(attr)

class FileSizeAttribute(unittest.TestCase, AssertSearch):
    def test_nounit(self):
        self.__assert_search__(['./test-data', 'type=file and size<1'], [])

        self.__assert_search__(['./test-data', 'type=file and size>5 and size<1000'],
                               ["./test-data/00/100b.0", "./test-data/02/720b.2"])

        self.__assert_search__(['./test-data', 'type=file and size=5'],
                               ["./test-data/01/5b.1"])

    def test_byte(self):
        self.__assert_search__(['./test-data', 'type=file and size=720b'],
                               ["./test-data/02/720b.2"])

        self.__assert_search__(['./test-data', 'type=file and size>=720byte and size<=1024 bytes'],
                               ["./test-data/00/1kb.0", "./test-data/02/720b.2"])

    def test_kilobyte(self):
        self.__assert_search__(['./test-data', 'type=file and size=1kb'],
                               ["./test-data/00/1kb.0"])

        self.__assert_search__(['./test-data', 'type=file and size>=1k and size <= 5 kilobytes'],
                               ["./test-data/00/5kb.0", "./test-data/00/1kb.0", "./test-data/02/5kb.2"])

        self.__assert_search__(['./test-data', 'type=file and size =1kilobyte'],
                               ["./test-data/00/1kb.0"])

    def test_megabyte(self):
        self.__assert_search__(['./test-data', 'type=file and size=1M'],
                               ["./test-data/00/1M.0"])

        self.__assert_search__(['./test-data',  'type=file and size>5mb and size <= 20megabytes'],
                               ["./test-data/00/20M.0", "./test-data/02/20M.2"])

        self.__assert_search__(['./test-data',  'type=file and size>1 megabyte and size <= 5M'],
                               ["./test-data/01/5M.1", "./test-data/02/5M.2"])

    def test_gigabyte(self):
        self.__assert_search__(['./test-data', 'type=file and size=1G'],
                               ["./test-data/00/1G.0", "./test-data/02/1G.2"])

        self.__assert_search__(['./test-data', 'type=file and size>1gigabyte and size <= 5 gb'],
                               ["./test-data/01/2G.1"])

        self.__assert_search__(['./test-data',  'type=file and size = 2 gigabytes'],
                               ["./test-data/01/2G.1"])

    def test_invalid_values(self):
        t = AttributeTester("./test-data")

        t.test_string("size")
        t.test_time("size")
        t.test_filetype("size")
        t.test_flag("size")

class FileGroupAttributes(unittest.TestCase, AssertSearch):
    def test_group(self):
        self.__assert_search__(['test-data', 'group="%s" and name="*5M*.2"' % run_id("-gn")],
                               ["test-data/02/5M.2"])

        returncode, _ = run_executable("efind", ['test-data', 'group="%s"' % random_string()])
        assert(returncode == 1)

    def test_gid(self):
        self.__assert_search__(['test-data', 'gid=%s and name="*20M*.2"' % run_id("-g")],
                               ["test-data/02/20M.2"])

        self.__assert_search__(['test-data', 'gid=%s and name="*20M*.2"' % random.randint(999999, 9999999)],
                               [])

    def test_invalid_values(self):
        t = AttributeTester("./test-data")

        for attr in ["group", "gid"]:
            t.test_size(attr)
            t.test_time(attr)
            t.test_filetype(attr)
            t.test_flag(attr)

        t.test_string("gid")
        t.test_integer("group")

class FileUserAttributes(unittest.TestCase, AssertSearch):
    def test_user(self):
        self.__assert_search__(['test-data', 'size=1G and user="%s"' % run_id("-un")],
                               ["test-data/00/1G.0", "test-data/02/1G.2"])

        returncode, _ = run_executable("efind", ['test-data', 'user="%s"' % random_string()])
        assert(returncode == 1)

    def test_uid(self):
        self.__assert_search__(['test-data', 'size=1G and uid=%s' % run_id("-u")],
                               ["test-data/00/1G.0", "test-data/02/1G.2"])

        self.__assert_search__(['test-data', 'uid=%d' % random.randint(99999, 9999999)], [])

    def test_invalid_values(self):
        t = AttributeTester("./test-data")

        for attr in ["user", "uid"]:
            t.test_size(attr)
            t.test_time(attr)
            t.test_filetype(attr)
            t.test_flag(attr)

        t.test_string("uid")
        t.test_integer("user")

class FileFlags(unittest.TestCase, AssertSearch):
    def setUp(self):
        os.mkdir("./test-flags")

        self.__make_empty_file__("empty")

        with open(self.__build_filename__("not-empty"), "w") as f:
            f.write(random_string())

        self.__make_empty_file__("readonly", 0400)
        self.__make_empty_file__("writeonly", 0200)
        self.__make_empty_file__("executable", 0100)

    def tearDown(self):
        shutil.rmtree("./test-flags")

    def test_readable(self):
        self.__assert_search__(['./test-flags', 'type=file and readable'],
                               ["./test-flags/readonly", "./test-flags/not-empty", "./test-flags/empty"])

    def test_writable(self):
        self.__assert_search__(['./test-flags', 'type=file and writable'],
                               ["./test-flags/writeonly", "./test-flags/not-empty", "./test-flags/empty"])

    def test_executable(self):
        self.__assert_search__(['./test-flags', 'type=file and executable'],
                               ["./test-flags/executable"])

    def test_empty(self):
        self.__assert_search__(['./test-flags', 'type=file and empty'],
                               ["./test-flags/writeonly", "./test-flags/readonly", "./test-flags/executable", "./test-flags/empty"])

        self.__assert_search__(['./test-flags', 'type=file and not empty'],
                               ["./test-flags/not-empty"])

    def __build_filename__(self, name):
        return os.path.join("./test-flags", name)

    def __make_empty_file__(self, name, permissions=None):
        path = self.__build_filename__(name)

        open(path, "w").close()

        if not permissions is None:
            os.chmod(path, permissions)

class FileTypeAttribute(unittest.TestCase, AssertSearch):
    def test_regular_file(self):
        self.__assert_search__(["./test-data", "type=file and size=2G"],
                               ["./test-data/01/2G.1"])

    def test_directory(self):
        self.__assert_search__(["./test-data", "type=directory"],
                               ["./test-data", "./test-data/00", "./test-data/01", "./test-data/02"])

    def test_links(self):
        self.__assert_search__(["./test-links", "type=link"],
                               ["./test-links/00", "./test-links/02"])

    def test_block(self):
        self.__assert_search__(["./test-data", "type=block"], [])

    def test_character(self):
        self.__assert_search__(["./test-data", "type=character"], [])

    def test_pipe(self):
        self.__assert_search__(["./test-data", "type=pipe"], [])

    def test_socket(self):
        self.__assert_search__(["./test-data", "type=socket"], [])

    def test_invalid_values(self):
        t = AttributeTester("./test-data")

        t.test_integer("type")
        t.test_string("type")
        t.test_time("type")
        t.test_size("type")
        t.test_flag("type")

class FileSystemAttribute(unittest.TestCase, AssertSearch):
    def test_filesystem(self):
        self.__assert_search__(['./test-data', 'filesystem="%s"' % random_string()], [])

    def test_invalid_values(self):
        t = AttributeTester("./test-data")

        t.test_integer("filesystem")
        t.test_filetype("filesystem")
        t.test_time("filesystem")
        t.test_size("filesystem")
        t.test_flag("filesystem")

class SearchSingleDirectory(unittest.TestCase, AssertSearch):
    def test_search(self):
        self.__assert_search__(['--expr', 'size=720', '--dir', 'test-data'],
                               ["test-data/02/720b.2"])

        self.__assert_search__(['-e', 'size>=720 and size<=5k and type=file', '-d', './test-data'],
                               ["./test-data/00/1kb.0", "./test-data/00/5kb.0", "./test-data/02/5kb.2", "./test-data/02/720b.2"])

        self.__assert_search__(['test-data', 'type=file and size<=100b and group="%s"' % (run_id('-gn'))],
                               ["test-data/00/100b.0", "test-data/01/5b.1"])

        self.__assert_search__(['./test-data', '-e','type=file and size=5M or (size>=1G and name="*.1")'],
                               ["./test-data/01/2G.1", "./test-data/01/5M.1", "./test-data/02/5M.2"])

    def test_max_depth(self):
        self.__assert_search__(['./test-data', '-e','size=5M or (size>=1G and name="*.1")', '--max-depth', '3'],
                               ["./test-data/01/2G.1", "./test-data/01/5M.1", "./test-data/02/5M.2"])

        self.__assert_search__(['./test-data', '-e','size=5M or (size>=1G and name="*.1")', '--max-depth=1'], [])

    def test_follow_links(self):
        self.__assert_search__(['./test-links', '-e','size=5M or (size>=1G and name="*.1")', '-L'],
                               ["./test-links/02/5M.2"])

        self.__assert_search__(['./test-links/', 'size=1G', '--follow'],
                               ["./test-links/00/1G.0", "./test-links/02/1G.2"])

        self.__assert_search__(['./test-links/', 'size=1G'], [])

    def test_invalid_dir(self):
        length = 64

        returncode, _ = run_executable("efind", ['dir', random_string(64), '-e', 'type=file'])
        assert(returncode == 1)

        returncode, _ = run_executable("efind", ['-d', random_string(length * 16), '-e', 'type=file'])
        assert(returncode == 1)

        returncode, _ = run_executable("efind", [random_string(length * 32), 'type=file'])
        assert(returncode == 1)

        returncode, _ = run_executable("efind", ['./test-data/01/2G.1', 'type=file'])
        assert(returncode == 1)

class SearchMultipleDirectories(unittest.TestCase):
    def test_search(self):
        args = ["./test-data/00", "./test-data/02", "type=file", "--order-by", "-h", "--printf", "%h\n"]
        returncode, output = run_executable_and_split_output("efind", args)

        assert(returncode == 0)
        assert(set(output[:6]) == set(["./test-data/02"]))
        assert(set(output[6:]) == set(["./test-data/00"]))

    def test_args_order(self):
        args = ["./test-data/00", "./test-data/01", "./test-data/02", "type=file"]
        returncode, output = run_executable_and_split_output("efind", args)

        assert(returncode == 0)
        assert(len(output) == 18)

        args = ["./test-data/02", "type=file", "-d", "./test-data/01", "--dir", "./test-data/00", "type=file"]
        returncode, output2 = run_executable_and_split_output("efind", args)

        assert(returncode == 0)
        assert_sequence_equality(output, output2)

        args = ["-e", "type=file", "-d", "./test-data/01", "--dir", "./test-data/00", "--dir", "./test-data/02", "type=file"]
        returncode, output2 = run_executable_and_split_output("efind", args)

        assert(returncode == 0)
        assert_sequence_equality(output, output2)

        args = ["./test-data/00", "./test-data/01", "./test-data/02", "./test-data/02", "type=file", "--dir", "./test-data/00"]
        returncode, output2 = run_executable_and_split_output("efind", args)

        assert(returncode == 0)
        assert_sequence_equality(output, output2)

    def test_max_depth(self):
        args = ["test-data/00", "test-data/02", "size=1G", "--max-depth", "1"]
        returncode, output = run_executable_and_split_output("efind", args)

        assert(returncode == 0)
        assert_sequence_equality(output, ["test-data/00/1G.0", "test-data/02/1G.2"])

        args = ["test-data/00", "test-data/02", "size=1G", "--max-depth=0"]
        returncode, output = run_executable_and_split_output("efind", args)

        assert(returncode == 0)
        assert(len(output) == 0)

    def test_follow_links(self):
        args = ["test-data/00", "test-links/02", "size=1G", "-L"]
        returncode, output = run_executable_and_split_output("efind", args)
 
        assert(returncode == 0)
        assert_sequence_equality(output, ["test-data/00/1G.0", "test-links/02/1G.2"])

        args = ["test-data/00", "test-links/02", "size=1G"]
        returncode, output = run_executable_and_split_output("efind", args)

        assert(returncode == 0)
        assert_sequence_equality(output, ["test-data/00/1G.0"])

    def test_invalid_dir(self):
        args = ["./test-data/00", "./test-data/01", random_string(128), "type=file"]
        returncode, _ = run_executable_and_split_output("efind", args)

        assert(returncode == 1)

class ChangeRoot:
    def __run_chroot_script__(self, args):
        returncode, _ = run_executable("sudo", ["./chroot.sh"] + args)
        assert(returncode == 0)

    def __run_in_chroot__(self, cmd, args=[]):
        return run_executable_and_split_output("sudo", ["chroot", "./chroot"] + [cmd] + args)

class ListExtensions(unittest.TestCase, ChangeRoot):
    def setUp(self):
        self.__run_chroot_script__(["install-binary"])

    def tearDown(self):
        self.__run_chroot_script__(["uninstall"])

    def test_list_extensions_no_installed_no_etc(self):
        returncode, output = self.__run_in_chroot__("efind", ["--list-extensions"])

        assert(returncode == 0)
        assert(len(output) == 1)

    def test_list_extensions_no_installed_with_etc(self):
        self.__run_chroot_script__(["install-etc"])

        returncode, output = self.__run_in_chroot__("efind", ["--list-extensions"])

        assert(returncode == 0)
        assert(len(output) == 0)

    def test_list_locally_installed_py_extension(self):
        self.__install_and_list_extension("py-test.py", "--local", self.__build_py_extension_description__)

    def test_list_globally_installed_py_extension(self):
        self.__install_and_list_extension("py-test.py", "--global", self.__build_py_extension_description__)

    def test_list_locally_installed_c_extension(self):
        self.__install_and_list_extension("c-test.so", "--local", self.__build_c_extension_description__)

    def test_list_globally_installed_c_extension(self):
        self.__install_and_list_extension("c-test.so", "--global", self.__build_c_extension_description__)

    def __install_and_list_extension(self, filename, location, builder):
        self.__run_chroot_script__(["install-extension", filename, location])

        returncode, output = self.__run_in_chroot__("efind", ["--list-extensions"])

        assert(returncode == 0)

        if location == "--global":
            directory = "/etc/efind/extensions"
        else:
            directory = "/root/.efind/extensions"

        assert_sequence_equality(output, builder(directory))

    def __build_py_extension_description__(self, directory):
        return ['%s/py-test.py' % directory,
                '\tpy-test, version 0.1.0',
                '\tefind test extension.',
                '\tpy_add(integer, integer)',
                '\tpy_name_equals(string)',
                '\tpy_sub(integer, integer)']

    def __build_c_extension_description__(self, directory):
        return ['%s/c-test.so' % directory,
                '\tc-test, version 0.1.0',
                '\tefind test extension.',
                '\tc_add(integer, integer)',
                '\tc_name_equals(string)',
                '\tc_sub(integer, integer)']

class EnvExtensionPath(unittest.TestCase):
    def setUp(self):
        os.environ["EFIND_EXTENSION_PATH"] = "./extensions"

    def tearDown(self):
        del os.environ["EFIND_EXTENSION_PATH"]

    def test_list_extensions(self):
        returncode, output = run_executable_and_split_output("efind", ["--list-extensions"])

        assert(returncode == 0)
        assert("./extensions/py-test.py" in output)
        assert("./extensions/c-test.so" in output)

class Blacklist(unittest.TestCase, ChangeRoot):
    def setUp(self):
        self.__run_chroot_script__(["install-binary"])
        self.__run_chroot_script__(["install-extension", "c-test.so", "--global"])
        self.__run_chroot_script__(["install-extension", "py-test.py", "--local"])

    def tearDown(self):
        self.__run_chroot_script__(["uninstall"])

    def test_empty_blacklist(self):
        self.__test_blacklist__(["/etc/efind/extensions/c-test.so", "/root/.efind/extensions/py-test.py"])

    def test_global_blacklist(self):
        self.__run_chroot_script__(["install-blacklist", "blacklist-noetc"])
        self.__test_blacklist__(["/root/.efind/extensions/py-test.py"], ["/etc/efind/extensions/c-test.so"])

    def test_local_blacklist(self):
        self.__run_chroot_script__(["install-blacklist", "blacklist-nolocal"])
        self.__test_blacklist__(["/etc/efind/extensions/c-test.so"], ["/root/.efind/extensions/py-test.py"])

    def __test_blacklist__(self, installed, blacklisted=[]):
        returncode, output = self.__run_in_chroot__("efind", ["--list-extensions"])
        output = self.__filter_output__(output)

        assert(returncode == 0)
        assert_sequence_equality(output, installed)

        returncode, output = self.__run_in_chroot__("efind", ["--show-blacklist"])
 
        assert(returncode == 0)
        assert_sequence_equality(output, blacklisted)

    def __filter_output__(self, output):
        return filter(lambda l: l[0] != '\t', output)

if __name__ == "__main__":
    cases = [InvalidArgs, PrintInfo, TimeAttributes, RegexAttributes, FileNameAttributes,
             FileSizeAttribute, FileFlags, FileTypeAttribute, FileSystemAttribute,
             SearchSingleDirectory, SearchMultipleDirectories, ListExtensions, EnvExtensionPath,
             Blacklist]
 
    if "--without-chroot" in sys.argv:
        cases = filter(lambda c: not issubclass(c, ChangeRoot), cases)

    for case in cases:
        suite = unittest.TestLoader().loadTestsFromTestCase(case)
        unittest.TextTestRunner().run(suite)
