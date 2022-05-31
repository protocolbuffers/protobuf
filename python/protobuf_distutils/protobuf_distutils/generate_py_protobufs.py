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

import os
import shutil
import subprocess
from pathlib import Path, PurePosixPath
from setuptools import Command, DistutilsOptionError


class generate_py_protobufs(Command):
    """Generates Python sources for .proto files."""

    description = 'Generate Python sources for .proto files'
    user_options = [
        ('extra-proto-paths=', None,
         'Additional paths to resolve imports in .proto files.'),

        ('protoc=', None,
         'Path to a specific `protoc` command to use.'),
    ]
    boolean_options = ['recurse']

    def initialize_options(self):
        """Sets the defaults for the command options."""
        self.source_dir = None
        self.proto_root_path = None
        self.extra_proto_paths = []
        self.output_dir = '.'
        self.proto_files = None
        self.recurse = True
        self.protoc = None

    @staticmethod
    def _test_path(s, force=False):
        path = Path(s).resolve()
        if not path.is_dir() and not (force and path.parent.is_dir()):
            raise DistutilsOptionError(s + " does not exist or is not a directory")
        if force and path.parent.is_dir():
            path.mkdir()
        return path

    def _test_paths(self):
        self.source_path = self._test_path(self.source_dir)
        if self.output_dir is None:
            self.output_dir = '.'
        self.output_path = self._test_path(self.output_dir, True)

    def finalize_options(self):
        """Sets the final values for the command options.

        Defaults were set in `initialize_options`, but could have been changed
        by command-line options or by other commands.
        """
        self._test_paths()
        self.ensure_string_list('extra_proto_paths')

        # SUBTLE: if 'source_dir' is a subdirectory of any entry in
        # 'extra_proto_paths', then in general, the shortest --proto_path prefix
        # (and the longest relative .proto filenames) must be used for
        # correctness. For example, consider:
        #
        #     source_dir = 'a/b/c'
        #     extra_proto_paths = ['a/b', 'x/y']
        #
        # In this case, we must ensure that a/b/c/d/foo.proto resolves
        # canonically as c/d/foo.proto, not just d/foo.proto. Otherwise, this
        # import:
        #
        #     import "c/d/foo.proto";
        #
        # would result in different FileDescriptor.name keys from "d/foo.proto".
        # That will cause all the definitions in the file to be flagged as
        # duplicates, with an error similar to:
        #
        #     c/d/foo.proto: "packagename.MessageName" is already defined in file "d/foo.proto"
        #
        # For paths in self.proto_files, we transform them to be relative to
        # self.proto_root_path, which may be different from self.source_dir.
        #
        # Although the order of --proto_paths is significant, shadowed filenames
        # are errors: if 'a/b/c.proto' resolves to different files under two
        # different --proto_path arguments, then the path is rejected as an
        # error. (Implementation note: this is enforced in protoc's
        # DiskSourceTree class.)

        if self.proto_root_path is None:
            self.proto_root_path = self.source_path
            for root_candidate in self.extra_proto_paths:
                root_candidate_path = self._test_path(root_candidate)
                if root_candidate_path == self.proto_root_path or root_candidate_path in self.proto_root_path.parents:
                    self.proto_root_path = root_candidate_path
            if self.proto_root_path != self.source_dir:
                self.announce(f'using computed proto_root_path: '
                              f'{self.proto_root_path}', level=2)
        else:
            self.proto_root_path = Path(self.proto_root_path).resolve()

        if self.proto_root_path != self.source_path and self.proto_root_path not in self.source_path.parents:
            raise DistutilsOptionError(f'source_dir {self.source_path} '
                                       f'is not under proto_root_path {self.proto_root_path}')

        if self.proto_files is None:
            files = []
            if self.recurse:
                files.extend(self.source_path.rglob('*.proto'))
            else:
                files.extend(self.source_path.glob('*.proto'))
            if not files:
                raise DistutilsOptionError(f'no .proto files were found under {self.source_path}')
            # Forced to cast to PurePosixPath since protoc requires POSIX-compliant relative paths
            self.proto_files = [f'{PurePosixPath(f.relative_to(self.proto_root_path))}' for f in files]

        self.ensure_string_list('proto_files')

        if self.protoc is None:
            self.protoc = os.getenv('PROTOC')
        if self.protoc is None:
            self.protoc = shutil.which('protoc')

    def run(self):
        # All proto file paths were adjusted in finalize_options to be relative
        # to self.proto_root_path.
        proto_paths = [f'--proto_path={self.proto_root_path}']
        proto_paths.extend([f'--proto_path={x}' for x in self.extra_proto_paths])

        # Run protoc. It was already resolved, so don't try to resolve
        # through PATH.
        command = [self.protoc,
                   f'--python_out={self.output_path}',
                   ] + proto_paths + self.proto_files
        self.announce(' '.join(command), level=2)
        subprocess.run(command)
