#!/usr/bin/env python3

# Protocol Buffers - Google's data interchange format
# Copyright 2023 Google LLC.  All rights reserved.
# https://developers.google.com/protocol-buffers/
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google LLC nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
