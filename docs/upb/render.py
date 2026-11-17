#!/usr/bin/env python3

# Protocol Buffers - Google's data interchange format
# Copyright 2023 Google LLC.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

import subprocess
import sys
import shutil
import os

if len(sys.argv) < 2:
    print("Must pass a filename argument")
    sys.exit(1)

in_filename = sys.argv[1]
out_filename = in_filename.replace(".in.md", ".md")
out_dir = in_filename.replace(".in.md", "")

if in_filename == out_filename:
    print("File must end in .in.md")
    sys.exit(1)

if os.path.isdir(out_dir):
    shutil.rmtree(out_dir)

os.mkdir(out_dir)
file_num = 1

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
            svg_filename = out_dir + "/" + str(file_num) + ".svg"
            svg = subprocess.check_output(['dot', '-Tsvg', '-o', svg_filename], input=dot_input)
            out_file.write(b"<div align=center>\n")
            out_file.write(b"<img src='%s'/>\n" % (svg_filename.encode('utf-8')))
            out_file.write(b"</div>\n")
            file_num += 1
        else:
            out_file.write(line)
