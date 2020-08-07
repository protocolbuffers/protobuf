# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
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
#     * Neither the name of Google Inc. nor the names of its
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

"""Implements the generate_py_protobufs command."""

__author__ = 'dlj@google.com (David L. Jones)'

import glob
import sys
import os
import distutils.spawn as spawn
from distutils.cmd import Command
from distutils.errors import DistutilsOptionError, DistutilsExecError

class generate_py_protobufs(Command):
    """Generates Python sources for .proto files."""

    description = 'Generate Python sources for .proto files'
    user_options = [
        ('extra-proto-paths=', None,
         'Additional paths to resolve imports in .proto files.'),

        ('protoc', None,
         'Path to a specific `protoc` command to use.'),
    ]
    boolean_options = ['recurse']

    def initialize_options(self):
        """Sets the defaults for the command options."""
        self.source_dir = None
        self.extra_proto_paths = []
        self.output_dir = '.'
        self.proto_files = None
        self.recurse = True
        self.protoc = None

    def finalize_options(self):
        """Sets the final values for the command options.

        Defaults were set in `initialize_options`, but could have been changed
        by command-line options or by other commands.
        """
        self.ensure_dirname('source_dir')
        self.ensure_string_list('extra_proto_paths')

        if self.output_dir is None:
            self.output_dir = '.'
        self.ensure_dirname('output_dir')

        if self.proto_files is None:
            files = glob.glob(os.path.join(self.source_dir, '*.proto'))
            if self.recurse:
                files.extend(glob.glob(os.path.join(self.source_dir, '**', '*.proto')))
            self.proto_files = [f.partition(self.source_dir + os.path.sep)[-1] for f in files]
            if not self.proto_files:
                raise DistutilsOptionError('no .proto files were found under ' + self.source_dir)
                
        self.ensure_string_list('proto_files')

        if self.protoc is None:
            self.protoc = spawn.find_executable('protoc')

    def run(self):
        # Run protoc. It was already resolved, so don't try to resolve
        # through PATH.
        proto_paths = ['--proto_path=' + self.source_dir]
        for extra in self.extra_proto_paths:
            proto_paths.append('--proto_path=' + extra)
        spawn.spawn(
            [self.protoc,
             '--python_out=' + self.output_dir,
            ] + proto_paths + self.proto_files,
            search_path=0)
