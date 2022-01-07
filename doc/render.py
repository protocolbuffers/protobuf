#!/usr/bin/env python3

import subprocess
import sys

if len(sys.argv) < 2:
    print("Must pass a filename argument")
    sys.exit(1)

in_filename = sys.argv[1]
out_filename = in_filename.replace(".in.md", ".md")

if in_filename == out_filename:
    print("File must end in .in.md")
    sys.exit(1)

with open(out_filename, "wb") as out_file, open(in_filename, "rb") as in_file:
    for line in in_file:
        if line.startswith(b"```dot"):
            dot_lines = []
            while True:
                dot_line = next(in_file)
                if dot_line == b"```\n":
                    break
                dot_lines.append(dot_line)
            dot_input = b"".join(dot_lines)
            svg = subprocess.check_output(['dot', '-Tsvg'], input=dot_input)
            out_file.write(b"<div align=center>")
            out_file.write(svg)
            out_file.write(b"</div>")
        else:
            out_file.write(line)
