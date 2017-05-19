import os

EXTENSION_NAME="example extension"
EXTENSION_VERSION="0.1.0"
EXTENSION_DESCRIPTION="An example extension written in Python."

def py_check_extension(filename, extension, icase):
    result = 0

    _, ext = os.path.splitext(filename)

    if len(ext) > 0:
        if icase == 0:
            result = ext == extension
        else:
            result = ext.lower() == extension.lower()

    return result

py_check_extension.__signature__=[str, int]

EXTENSION_EXPORT=[py_check_extension]
