#!/usr/bin/python
#
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

"""Shared code for validating staleness_test() rules.

This code is used by test scripts generated from staleness_test() rules.
"""

from __future__ import absolute_import
from __future__ import print_function

import difflib
import sys
import os
from shutil import copyfile


class _FilePair(object):
  """Represents a single (target, generated) file pair."""

  def __init__(self, target, generated):
    self.target = target
    self.generated = generated


class Config(object):
  """Represents the configuration for a single staleness test target."""

  def __init__(self, file_list):
    # Duplicate to avoid modifying our arguments.
    file_list = list(file_list)

    # The file list contains a few other bits of information at the end.
    # This is packed by the code in build_defs.bzl.
    self.target_name = file_list.pop()
    self.package_name = file_list.pop()
    self.pattern = file_list.pop()

    self.file_list = file_list


def _GetFilePairs(config):
  """Generates the list of file pairs.

  Args:
    config: a Config object representing this target's config.

  Returns:
    A list of _FilePair objects.
  """

  ret = []

  has_bazel_genfiles = os.path.exists("bazel-bin")

  for filename in config.file_list:
    target = os.path.join(config.package_name, filename)
    generated = os.path.join(config.package_name, config.pattern % filename)
    if has_bazel_genfiles:
      generated = os.path.join("bazel-bin", generated)

    # Generated files should always exist.  Blaze should guarantee this before
    # we are run.
    if not os.path.isfile(generated):
      print("Generated file '%s' does not exist." % generated)
      print("Please run this command to generate it:")
      print("  bazel build %s:%s" % (config.package_name, config.target_name))
      sys.exit(1)
    ret.append(_FilePair(target, generated))

  return ret


def _GetMissingAndStaleFiles(file_pairs):
  """Generates lists of missing and stale files.

  Args:
    file_pairs: a list of _FilePair objects.

  Returns:
    missing_files: a list of _FilePair objects representing missing files.
      These target files do not exist at all.
    stale_files: a list of _FilePair objects representing stale files.
      These target files exist but have stale contents.
  """

  missing_files = []
  stale_files = []

  for pair in file_pairs:
    if not os.path.isfile(pair.target):
      missing_files.append(pair)
      continue

    with open(pair.generated) as g, open(pair.target) as t:
      if g.read() != t.read():
        stale_files.append(pair)

  return missing_files, stale_files


def _CopyFiles(file_pairs):
  """Copies all generated files to the corresponding target file.

  The target files must be writable already.

  Args:
    file_pairs: a list of _FilePair objects that we want to copy.
  """

  for pair in file_pairs:
    target_dir = os.path.dirname(pair.target)
    if not os.path.isdir(target_dir):
      os.makedirs(target_dir)
    copyfile(pair.generated, pair.target)


def FixFiles(config):
  """Implements the --fix option: overwrites missing or out-of-date files.

  Args:
    config: the Config object for this test.
  """

  file_pairs = _GetFilePairs(config)
  missing_files, stale_files = _GetMissingAndStaleFiles(file_pairs)

  _CopyFiles(stale_files + missing_files)


def CheckFilesMatch(config):
  """Checks whether each target file matches the corresponding generated file.

  Args:
    config: the Config object for this test.

  Returns:
    None if everything matches, otherwise a string error message.
  """

  diff_errors = []

  file_pairs = _GetFilePairs(config)
  missing_files, stale_files = _GetMissingAndStaleFiles(file_pairs)

  for pair in missing_files:
    diff_errors.append("File %s does not exist" % pair.target)
    continue

  for pair in stale_files:
    with open(pair.generated) as g, open(pair.target) as t:
        diff = ''.join(difflib.unified_diff(g.read().splitlines(keepends=True),
                                            t.read().splitlines(keepends=True)))
        diff_errors.append("File %s is out of date:\n%s" % (pair.target, diff))

  if diff_errors:
    error_msg = "Files out of date!\n\n"
    error_msg += "To fix run THIS command:\n"
    error_msg += "  bazel-bin/%s/%s --fix\n\n" % (config.package_name,
                                                  config.target_name)
    error_msg += "Errors:\n"
    error_msg += "  " + "\n  ".join(diff_errors)
    return error_msg
  else:
    return None
