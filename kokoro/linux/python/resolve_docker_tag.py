#!/usr/bin/python

"""Retrieve an image's information."""

import re
import sys

def _parse_image(name):
    m = re.match(r'^(?:(?P<host>[\w.-^_]+(?:[:][\d]+))/)?(?P<name>[\w.-]+/[\w.-]+)(?:[:](?P<tag>[\w.-]+))?$',
                 name)
    if m:
        return m.groupdict()

if __name__ == '__main__':
    for arg in sys.argv:
        print(arg, _parse_image(arg))
