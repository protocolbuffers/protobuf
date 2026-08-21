# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Implements the generate_py_protobufs command."""

__author__ = 'dlj@google.com (David L. Jones)'

import glob
import os
import shutil
import subprocess
import sys
from setuptools import Command
from setuptools.errors import OptionError

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
      self.proto_root_path = os.path.normpath(self.source_dir)
      for root_candidate in self.extra_proto_paths:
        root_candidate = os.path.normpath(root_candidate)
        if self.proto_root_path.startswith(root_candidate):
          self.proto_root_path = root_candidate
      if self.proto_root_path != self.source_dir:
        self.announce('using computed proto_root_path: ' + self.proto_root_path, level=2)

    if not self.source_dir.startswith(self.proto_root_path):
      raise OptionError(
          'source_dir '
          + self.source_dir
          + ' is not under proto_root_path '
          + self.proto_root_path
      )

    if self.proto_files is None:
      files = glob.glob(os.path.join(self.source_dir, '*.proto'))
      if self.recurse:
        files.extend(
            glob.glob(
                os.path.join(self.source_dir, '**', '*.proto'), recursive=True
            )
        )
      self.proto_files = [
          f.partition(self.proto_root_path + os.path.sep)[-1] for f in files
      ]
      if not self.proto_files:
        raise OptionError('no .proto files were found under ' + self.source_dir)

    self.ensure_string_list('proto_files')

    if self.protoc is None:
      self.protoc = os.getenv('PROTOC')
    if self.protoc is None:
      self.protoc = shutil.which('protoc')

  def run(self):
    # All proto file paths were adjusted in finalize_options to be relative
    # to self.proto_root_path.
    proto_paths = ['--proto_path=' + self.proto_root_path]
    proto_paths.extend(['--proto_path=' + x for x in self.extra_proto_paths])

    # Run protoc.
    subprocess.run(
        [
            self.protoc,
            '--python_out=' + self.output_dir,
        ]
        + proto_paths
        + self.proto_files
    )
