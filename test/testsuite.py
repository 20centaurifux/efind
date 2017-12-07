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
from getopt import getopt
import unittest, inspect, random, os, os.path, shutil, sys, stat

class InvalidArgs(unittest.TestCase):
    def test_invalid_args(self):
        for length in range(10, 10000, 1000):
            returncode, _ = run_executable("efind", ["--" + random_string(length)])

        assert(returncode == 1)

class PrintInfo(unittest.TestCase):
    def test_print_version(self):
        self.__run_long_and_short_option("--version", "-v")
        self.__run_long_and_short_option("--version", "-v", optional_args=self.__make_log_level_argv())
        self.__run_long_and_short_option("--version", "-v", optional_args=self.__make_log_level_argv_with_color())

    def test_print_help(self):
        self.__run_long_and_short_option("--help", "-h")
        self.__run_long_and_short_option("--help", "-h", optional_args=self.__make_log_level_argv())
        self.__run_long_and_short_option("--help", "-h", optional_args=self.__make_log_level_argv_with_color())

    def __run_long_and_short_option(self, arg_long, arg_short, optional_args=[]):
        returncode, output_long = run_executable_and_split_output("efind", [arg_long] + optional_args)

        assert(returncode == 0)

        returncode, output_short = run_executable_and_split_output("efind", [arg_short] + optional_args)

        assert(returncode == 0)
        assert_sequence_equality(output_long, output_short)

    def __make_log_level_argv(self):
        return ["--log-level=%d" % random.randint(0, 6)]

    def __make_log_level_argv_with_color(self):
        return ["--enable-log-color"] + self.__make_log_level_argv()

class AttributeTester:
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

class AssertSearch:
    def assert_search(self, args, expected_files):
        returncode, output = run_executable_and_split_output("efind", args)

        assert(returncode == 0)
        assert_sequence_equality(output, expected_files)

class FileNameAttributes(unittest.TestCase, AssertSearch):
    def test_name(self):
        self.assert_search(['./test-data', 'name="*720b.2*"'], ["./test-data/02/720b.2"])

    def test_iname(self):
        self.assert_search(['./test-data', 'iname="*2?m*"'], ["./test-data/00/20M.0", "./test-data/02/20M.2"])

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
            interval = self.__next_interval(interval)
            character += 1

        for offset in [-3, -2, -6]:
            self.__create_file("mtime-%c" % chr(character), 0, offset * interval)
            interval = self.__next_interval(interval)
            character += 1

    def tearDown(self):
        shutil.rmtree("./test-time")

    def test_atime(self):
        self.assert_search(['./test-time', 'atime>=5 minutes and atime<1hour"'], ['./test-time/atime-a'])
        self.assert_search(['./test-time', 'atime>=3 hours and atime<1day"'], ['./test-time/atime-b'])
        self.assert_search(['./test-time', 'atime>=2 days'], ['./test-time/atime-c'])

    def test_mtime(self):
        self.assert_search(['./test-time', 'mtime<=4 minutes and mtime>1minute"'], ['./test-time/mtime-d'])
        self.assert_search(['./test-time', 'mtime<=3 hours and mtime>1hour"'], ['./test-time/mtime-e'])
        self.assert_search(['./test-time', 'mtime<=7 days and mtime>1day"'], ['./test-time/mtime-f'])

    def test_ctime(self):
        self.assert_search(['./test-time', 'type=file and ctime<=1minute and ctime<2hours and ctime<= 1day'],
                           ['./test-time/atime-a', './test-time/atime-b', './test-time/atime-c',
                            './test-time/mtime-d', './test-time/mtime-e', './test-time/mtime-f'])

        self.assert_search(['./test-time', 'type=file and ctime<=0'], [])

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

    def __next_interval(self, interval):
        if interval == 60:
            interval *= 60
        elif interval == 60 * 60:
            interval *= 24
        else:
            interval = 60

        return interval

class RegexAttributes(unittest.TestCase, AssertSearch):
    def test_regex(self):
        self.assert_search(['./test-data', 'regex="\./test.*M\.[1-2]"'],
                           ["./test-data/01/5M.1", "./test-data/02/5M.2", "./test-data/02/20M.2"])

        self.assert_search(['./test-data', 'regex="\./test.*M\.[1-2]"', '--regex-type', "emacs"],
                           ["./test-data/01/5M.1", "./test-data/02/5M.2", "./test-data/02/20M.2"])

    def test_iregex(self):
        self.assert_search(['./test-data', 'iregex="\./TEST.*m\.[1-2]"'],
                           ["./test-data/01/5M.1", "./test-data/02/5M.2", "./test-data/02/20M.2"])

        self.assert_search(['./test-data', 'iregex="\./TEST.*m\.[1-2]"', '--regex-type', "emacs"],
                           ["./test-data/01/5M.1", "./test-data/02/5M.2", "./test-data/02/20M.2"])

    def test_regex_types(self):
        for regex_type in ["awk", "emacs", "gnu-awk", "egrep", "posix-awk", "posix-egrep", "posix-extended"]:
            self.assert_search(['./test-data', 'regex="^\./test\-dat[a|b|c].*2+G\.[0-2]$"', '--regex-type', regex_type],
                               ["./test-data/01/2G.1"])

        for regex_type in ["ed", "grep", "sed", "posix-minimal-basic"]:
            self.assert_search(['./test-data', 'regex="^\./test\-dat[a|b|c].*2+G\.[0-2]$"', '--regex-type', regex_type], [])

        for regex_type in ["awk", "ed", "egrep", "emacs", "grep", "gnu-awk", "posix-awk", "posix-egrep", "posix-extended", "posix-minimal-basic", "sed"]:
            self.assert_search(['./test-data', 'regex=".*\.1"', '--regex-type', regex_type],
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
        self.assert_search(['./test-data', 'type=file and size<1'], [])

        self.assert_search(['./test-data', 'type=file and size>5 and size<1000'],
                           ["./test-data/00/100b.0", "./test-data/02/720b.2"])

        self.assert_search(['./test-data', 'type=file and size=5'],
                           ["./test-data/01/5b.1"])

    def test_byte(self):
        self.assert_search(['./test-data', 'type=file and size=720b'],
                           ["./test-data/02/720b.2"])

        self.assert_search(['./test-data', 'type=file and size>=720byte and size<=1024 bytes'],
                           ["./test-data/00/1kb.0", "./test-data/02/720b.2"])

    def test_kilobyte(self):
        self.assert_search(['./test-data', 'type=file and size=1kb'],
                           ["./test-data/00/1kb.0"])

        self.assert_search(['./test-data', 'type=file and size>=1k and size <= 5 kilobytes'],
                           ["./test-data/00/5kb.0", "./test-data/00/1kb.0", "./test-data/02/5kb.2"])

        self.assert_search(['./test-data', 'type=file and size =1kilobyte'],
                           ["./test-data/00/1kb.0"])

    def test_megabyte(self):
        self.assert_search(['./test-data', 'type=file and size=1M'],
                           ["./test-data/00/1M.0"])

        self.assert_search(['./test-data',  'type=file and size>5mb and size <= 20megabytes'],
                           ["./test-data/00/20M.0", "./test-data/02/20M.2"])

        self.assert_search(['./test-data',  'type=file and size>1 megabyte and size <= 5M'],
                           ["./test-data/01/5M.1", "./test-data/02/5M.2"])

    def test_gigabyte(self):
        self.assert_search(['./test-data', 'type=file and size=1G'],
                           ["./test-data/00/1G.0", "./test-data/02/1G.2"])

        self.assert_search(['./test-data', 'type=file and size>1gigabyte and size <= 5 gb'],
                           ["./test-data/01/2G.1"])

        self.assert_search(['./test-data',  'type=file and size = 2 gigabytes'],
                           ["./test-data/01/2G.1"])

    def test_invalid_values(self):
        t = AttributeTester("./test-data")

        t.test_string("size")
        t.test_time("size")
        t.test_filetype("size")
        t.test_flag("size")

class FileGroupAttributes(unittest.TestCase, AssertSearch):
    def test_group(self):
        self.assert_search(['test-data', 'group="%s" and name="*5M*.2"' % run_id("-gn")],
                           ["test-data/02/5M.2"])

        returncode, _ = run_executable("efind", ['test-data', 'group="%s"' % random_string()])

        assert(returncode == 1)

    def test_gid(self):
        self.assert_search(['test-data', 'gid=%s and name="*20M*.2"' % run_id("-g")],
                           ["test-data/02/20M.2"])

        self.assert_search(['test-data', 'gid=%s and name="*20M*.2"' % random.randint(999999, 9999999)], [])

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
        self.assert_search(['test-data', 'size=1G and user="%s"' % run_id("-un")],
                           ["test-data/00/1G.0", "test-data/02/1G.2"])

        returncode, _ = run_executable("efind", ['test-data', 'user="%s"' % random_string()])

        assert(returncode == 1)

    def test_uid(self):
        self.assert_search(['test-data', 'size=1G and uid=%s' % run_id("-u")],
                           ["test-data/00/1G.0", "test-data/02/1G.2"])

        self.assert_search(['test-data', 'uid=%d' % random.randint(99999, 9999999)], [])

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

        self.__make_empty_file("empty")

        with open(self.__build_filename("not-empty"), "w") as f:
            f.write(random_string())

        self.__make_empty_file("readonly", 0400)
        self.__make_empty_file("writeonly", 0200)
        self.__make_empty_file("executable", 0100)

    def tearDown(self):
        shutil.rmtree("./test-flags")

    def test_readable(self):
        self.assert_search(['./test-flags', 'type=file and readable'],
                           ["./test-flags/readonly", "./test-flags/not-empty", "./test-flags/empty"])

    def test_writable(self):
        self.assert_search(['./test-flags', 'type=file and writable'],
                           ["./test-flags/writeonly", "./test-flags/not-empty", "./test-flags/empty"])

    def test_executable(self):
        self.assert_search(['./test-flags', 'type=file and executable'],
                           ["./test-flags/executable"])

    def test_empty(self):
        self.assert_search(['./test-flags', 'type=file and empty'],
                           ["./test-flags/writeonly", "./test-flags/readonly", "./test-flags/executable", "./test-flags/empty"])

        self.assert_search(['./test-flags', 'type=file and not empty'],
                           ["./test-flags/not-empty"])

    def __build_filename(self, name):
        return os.path.join("./test-flags", name)

    def __make_empty_file(self, name, permissions=None):
        path = self.__build_filename(name)

        open(path, "w").close()

        if not permissions is None:
            os.chmod(path, permissions)

class FileTypeAttribute(unittest.TestCase, AssertSearch):
    def test_regular_file(self):
        self.assert_search(["./test-data", "type=file and size=2G"],
                           ["./test-data/01/2G.1"])

    def test_directory(self):
        self.assert_search(["./test-data", "type=directory"],
                           ["./test-data", "./test-data/00", "./test-data/01", "./test-data/02"])

    def test_links(self):
        self.assert_search(["./test-links", "type=link"],
                           ["./test-links/00", "./test-links/02"])

    def test_block(self):
        self.assert_search(["./test-data", "type=block"], [])

    def test_character(self):
        self.assert_search(["./test-data", "type=character"], [])

    def test_pipe(self):
        self.assert_search(["./test-data", "type=pipe"], [])

    def test_socket(self):
        self.assert_search(["./test-data", "type=socket"], [])

    def test_invalid_values(self):
        t = AttributeTester("./test-data")

        t.test_integer("type")
        t.test_string("type")
        t.test_time("type")
        t.test_size("type")
        t.test_flag("type")

class FileSystemAttribute(unittest.TestCase, AssertSearch):
    def test_filesystem(self):
        self.assert_search(['./test-data', 'filesystem="%s"' % random_string()], [])

    def test_invalid_values(self):
        t = AttributeTester("./test-data")

        t.test_integer("filesystem")
        t.test_filetype("filesystem")
        t.test_time("filesystem")
        t.test_size("filesystem")
        t.test_flag("filesystem")

class SearchSingleDirectory(unittest.TestCase, AssertSearch):
    def test_search(self):
        self.assert_search(['--expr', 'size=720', '--dir', 'test-data'],
                           ["test-data/02/720b.2"])

        self.assert_search(['-e', 'size>=720 and size<=5k and type=file', '-d', './test-data'],
                           ["./test-data/00/1kb.0", "./test-data/00/5kb.0", "./test-data/02/5kb.2", "./test-data/02/720b.2"])

        self.assert_search(['test-data', 'type=file and size<=100b and group="%s"' % (run_id('-gn'))],
                           ["test-data/00/100b.0", "test-data/01/5b.1"])

        self.assert_search(['./test-data', '-e','type=file and size=5M or (size>=1G and name="*.1")'],
                           ["./test-data/01/2G.1", "./test-data/01/5M.1", "./test-data/02/5M.2"])

    def test_max_depth(self):
        self.assert_search(['./test-data', '-e','size=5M or (size>=1G and name="*.1")', '--max-depth', '3'],
                           ["./test-data/01/2G.1", "./test-data/01/5M.1", "./test-data/02/5M.2"])

        self.assert_search(['./test-data', '-e','size=5M or (size>=1G and name="*.1")', '--max-depth=1'], [])

    def test_follow_links(self):
        self.assert_search(['./test-links', '-e','size=5M or (size>=1G and name="*.1")', '-L'],
                           ["./test-links/02/5M.2"])

        self.assert_search(['./test-links/', 'size=1G', '--follow'],
                           ["./test-links/00/1G.0", "./test-links/02/1G.2"])

        self.assert_search(['./test-links/', 'size=1G'], [])

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
        args = ["./test-data/00", "./test-data/02", "type=file"]
        returncode, output = run_executable_and_split_output("efind", args)

        assert(returncode == 0)

        output = sorted(map(lambda p: p[:14], output))

        assert(set(output[:7]) == set(["./test-data/00"]))
        assert(set(output[7:]) == set(["./test-data/02"]))

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

class FakeDirTest(unittest.TestCase):
    def __init__(self, name, **kwargs):
        unittest.TestCase.__init__(self, name, **kwargs)

        self.global_path = "./fake-dirs/libdir/efind/extensions"
        self.local_path = "./fake-dirs/home/.efind/extensions"

    def setUp(self):
        self.__home = os.environ["HOME"]

        os.mkdir("./fake-dirs")
        os.makedirs("./fake-dirs/libdir/efind/extensions")
        os.makedirs("./fake-dirs/home/.efind/extensions")

        os.environ["EFIND_LIBDIR"] = "./fake-dirs/libdir"
        os.environ["HOME"] = "./fake-dirs/home"

    def tearDown(self):
        os.environ["HOME"] = self.__home
        shutil.rmtree("./fake-dirs")
        del os.environ["EFIND_LIBDIR"]

    def copy_extension(self, filename, destination):
        shutil.copy(os.path.join("./extensions", filename), destination)

class ListExtensions(FakeDirTest):
    def __init__(self, name, **kwargs):
        FakeDirTest.__init__(self, name, **kwargs)

    def test_list_no_extensions(self):
        returncode, output = run_executable_and_split_output("efind", ["--list-extensions"])

        assert(returncode == 0)
        assert(len(output) == 0)

    def test_list_local_installed_py_extension(self):
        self.__install_and_list_extension("py-test.py", self.local_path, self.__build_py_extension_description)

    def test_list_global_installed_py_extension(self):
        self.__install_and_list_extension("py-test.py", self.global_path, self.__build_py_extension_description)

    def test_list_local_installed_so_extension(self):
        self.__install_and_list_extension("c-test.so", self.local_path, self.__build_so_extension_description)

    def test_list_global_installed_so_extension(self):
        self.__install_and_list_extension("c-test.so", self.global_path, self.__build_so_extension_description)

    def test_list_extensions(self):
        self.copy_extension("c-test.so", self.global_path)
        self.copy_extension("py-test.py", self.local_path)

        returncode, output = run_executable_and_split_output("efind", ["--list-extensions"])

        assert(returncode == 0)
        assert_sequence_equality(output, self.__build_so_extension_description(self.global_path) +
                                         self.__build_py_extension_description(self.local_path))

    def __install_and_list_extension(self, filename, destination, builder):
        self.copy_extension(filename, destination)

        returncode, output = run_executable_and_split_output("efind", ["--list-extensions"])

        assert(returncode == 0)
        assert_sequence_equality(output, builder(destination))

    def __build_py_extension_description(self, directory):
        return ['%s/py-test.py' % directory,
                '\tpy-test, version 0.1.0',
                '\tefind test extension.',
                '\tpy_add(integer, integer)',
                '\tpy_name_equals(string)',
                '\tpy_sub(integer, integer)']

    def __build_so_extension_description(self, directory):
        return ['%s/c-test.so' % directory,
                '\tc-test, version 0.1.0',
                '\tefind test extension.',
                '\tc_add(integer, integer)',
                '\tc_name_equals(string)',
                '\tc_sub(integer, integer)']

class Blacklist(FakeDirTest):
    def __init__(self, name, **kwargs):
        FakeDirTest.__init__(self, name, **kwargs)

        self.__so_file = os.path.join(self.global_path, "c-test.so")
        self.__py_file = os.path.join(self.local_path, "py-test.py")

    def setUp(self):
        FakeDirTest.setUp(self)

        self.copy_extension("c-test.so", self.global_path)
        self.copy_extension("py-test.py", self.local_path)

    def test_empty_blacklist(self):
        self.__test_blacklist([self.__so_file, self.__py_file])

    def test_no_libdir(self):
        shutil.copy("./blacklist-nolibdir", "./fake-dirs/home/.efind/blacklist")
        self.__test_blacklist([self.__py_file], [self.__so_file])

    def test_no_local(self):
        shutil.copy("./blacklist-nolocal", "./fake-dirs/home/.efind/blacklist")
        self.__test_blacklist([self.__so_file], [self.__py_file])

    def __test_blacklist(self, installed, blacklisted=[]):
        returncode, output = run_executable_and_split_output("efind", ["--list-extensions"])
        output = self.__filter_output(output)

        assert(returncode == 0)
        assert_sequence_equality(output, installed)

        returncode, output = run_executable_and_split_output("efind", ["--show-blacklist"])
 
        assert(returncode == 0)
        assert_sequence_equality(output, blacklisted)

    def __filter_output(self, output):
        return filter(lambda l: l[0] != '\t', output)

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

class PythonExtensions(unittest.TestCase, AssertSearch):
    def setUp(self):
        os.environ["EFIND_EXTENSION_PATH"] = "./extensions"

    def tearDown(self):
        del os.environ["EFIND_EXTENSION_PATH"]

    def test_extension(self):
        folder = self.__list_folder("./test-data/00")
        self.assert_search(['./test-data/00', 'py_add(19, 4)=23'], folder)

        folder = self.__list_folder("./test-data/01")
        self.assert_search(['./test-data/01', 'py_add(45, 2) >= 47 and py_sub(-5, 3) > -9'], folder)

        folder = self.__list_folder("./test-data/02")
        self.assert_search(['./test-data/02', 'py_add(100, 200) = py_sub(600, 300)'], folder)

        self.assert_search(['./test-data', 'py_name_equals("./test-data/02/1G.2")'], ["./test-data/02/1G.2"])

        expr = 'name="5kb.2" and (py_add(1, 1)=3 or py_sub ( 2 , 1) <0 ) or py_add(py_sub(100, 99), py_add (0, 1)) = 2'
        self.assert_search(['./test-data', expr], ["./test-data/02/5kb.2"])

    def test_invalid_args(self):
        exprs = ['py_add(5, 1',
                 'py_add(-1, 1)>0 or py_sub("%s")' % random_string(),
                 'type=file or py_name_equals("%s")' % random_string(),
                 'py_add(1, 2, 3) > py_sub(4, 3)']

        for expr in exprs:
            returncode, _ = run_executable('efind', ['./test-data', expr])
            assert(returncode == 1)

    def __list_folder(self, folder):
        return map(lambda d: os.path.join(folder, d), os.listdir(folder)) + [folder]

class CExtensions(unittest.TestCase, AssertSearch):
    def setUp(self):
        os.environ["EFIND_EXTENSION_PATH"] = "./extensions"

    def tearDown(self):
        del os.environ["EFIND_EXTENSION_PATH"]

    def test_extension(self):
        folder = self.__list_folder("./test-data/00")
        self.assert_search(['./test-data/00', 'c_add(-523, 58212)=57689'], folder)

        folder = self.__list_folder("./test-data/01")
        self.assert_search(['./test-data/01', 'c_add(913, 37) >= 950 and c_sub(-17, 987) > -1005'], folder)

        folder = self.__list_folder("./test-data/02")
        self.assert_search(['./test-data/02', 'c_add(23750235, 523597) = c_sub(24517089, 243257)'], folder)

        self.assert_search(['./test-data', 'c_name_equals("./test-data/02/20M.2")'], ["./test-data/02/20M.2"])

        expr = 'name="7kb.2" and (c_add(3523, 6436)=3 or c_sub ( 643 , -1241) <0 ) or c_add(c_sub(100, 99), c_add (0, 1)) = 2'
        self.assert_search(['./test-data', expr], ["./test-data/02/7kb.2"])

    def test_invalid_args(self):
        exprs = ['c_add(5, 1',
                 'c_add(-1, 1)>0 or c_sub("%s")' % random_string(),
                 'type=file or c_name_equals("%s")' % random_string(),
                 'c_add(1, 2, 3) > c_sub(4, 3)']

        for expr in exprs:
            returncode, _ = run_executable('efind', ['./test-data', expr])
            assert(returncode == 1)

    def __list_folder(self, folder):
        return map(lambda d: os.path.join(folder, d), os.listdir(folder)) + [folder]

def get_test_cases():
    mod = sys.modules[__name__]

    attrs = map(lambda k: getattr(mod, k), dir(mod))

    return filter(lambda attr: inspect.isclass(attr) and issubclass(attr, unittest.TestCase), attrs)

if __name__ == "__main__":
    cases = get_test_cases()

    loader = unittest.TestLoader()
    suite = unittest.TestSuite()

    for case in cases:
        suite.addTest(loader.loadTestsFromTestCase(case))

    unittest.TextTestRunner().run(suite)
