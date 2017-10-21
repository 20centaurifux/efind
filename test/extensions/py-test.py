EXTENSION_NAME="py-test"
EXTENSION_VERSION="0.1.0"
EXTENSION_DESCRIPTION="efind test extension."

def name_equals(filename, name):
    return filename == name

name_equals.__signature__=[str]

def add(filename, a, b):
    return a + b

add.__signature__=[int, int]

def sub(filename, a, b):
    return a - b

sub.__signature__=[int, int]

EXTENSION_EXPORT=[name_equals, add, sub]
