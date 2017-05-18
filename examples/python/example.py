import os

EXTENSION_NAME="example extension"
EXTENSION_VERSION="0.1.0"
EXTENSION_DESCRIPTION="An example extension written in Python."

def py_check_extension(filename, extension, icase):
    _, ext = os.path.splitext(filename)

    if icase == 0:
        return ext == extension
    else:
        return ext.lower() == extension.lower()

py_check_extension.__signature__=[str, int]

EXTENSION_EXPORT=[py_check_extension]
