#!/usr/bin/env python3

import sys

if len(sys.argv) < 2:
    print("Must pass a filename argument")
    sys.exit(1)

in_file = sys.argv[1]
out_file = in_file.replace(".in.md", ".md")

if in_file == out_file:
    print("File must end in .in.md")
    sys.exit(1)
