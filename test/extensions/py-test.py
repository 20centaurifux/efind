EXTENSION_NAME="py-test"
EXTENSION_VERSION="0.1.0"
EXTENSION_DESCRIPTION="efind test extension."

def py_name_equals(filename, name):
    return filename == name

py_name_equals.__signature__=[str]

def py_add(filename, a, b):
    return a + b

py_add.__signature__=[int, int]

def py_sub(filename, a, b):
    return a - b

py_sub.__signature__=[int, int]

EXTENSION_EXPORT=[py_name_equals, py_add, py_sub]
