EXTENSION_NAME="py-test"
EXTENSION_VERSION="0.1.0"
EXTENSION_DESCRIPTION="efind test extension."

def py_name_equals(filename: str, name: str):
    return filename == name

def py_add(filename: str, a: int, b: int):
    return a + b

def py_sub(filename: str, a: int, b: int):
    return a - b

EXTENSION_EXPORT=[py_name_equals, py_add, py_sub]
