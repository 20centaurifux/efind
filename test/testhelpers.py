"""
	project............: efind
	description........: efind test suite.
	copyright..........: Sebastian Fedrau
	email..............: sebastian.fedrau@gmail.com

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License v3 as published by
	the Free Software Foundation.

	This program is distributed in the hope that it will be useful, but
	WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
	General Public License v3 for more details.
"""
import subprocess, os, random, string

def join_args(args):
    return ", ".join(map(repr, args))

def run_executable(executable, args):
    cmd = [executable] + args
    
    print("Running %s, argv=[%s]" % (executable, join_args(cmd[1:])))

    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    output = proc.stdout.read().decode("utf-8")
    proc.wait()

    return proc.returncode, output

def run_executable_and_split_output(executable, args):
    returncode, output = run_executable(executable, args)

    return returncode, list(filter(lambda l: l != "", output.split("\n")))

def assert_sequence_equality(a, b):
    assert(set(a) == set(b))

def run_id(arg):
    returncode, output = run_executable("id", [arg])

    assert(returncode == 0)

    return output.strip()

def random_string(length=32):
    return "".join(random.choice(string.ascii_uppercase + string.digits) for _ in range(length))
