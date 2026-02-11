# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Tests for google.protobuf.runtime_version."""

__author__ = 'shaod@google.com (Dennis Shao)'

import unittest
from google.protobuf import runtime_version


class RuntimeVersionTest(unittest.TestCase):

  def test_invalid_version(self):
    with self.assertRaisesRegex(
        runtime_version.VersionError, 'Invalid gencode version: -1.-2.-3'
    ):
      runtime_version.ValidateProtobufRuntimeVersion(
          runtime_version.DOMAIN, -1, -2, -3, '', 'dummy.proto'
      )

  def test_cross_domain_disallowed(self):
    gencode_domain = runtime_version.Domain.GOOGLE_INTERNAL
    runtime_domain = runtime_version.Domain.PUBLIC
    with self.assertRaisesRegex(
        runtime_version.VersionError,
        'Detected mismatched Protobuf Gencode/Runtime domains when loading'
        f' foo.proto: gencode {gencode_domain.name} runtime'
        f' {runtime_domain.name}',
    ):
      runtime_version.ValidateProtobufRuntimeVersion(
          gencode_domain, 1, 2, 3, '', 'foo.proto'
      )

  def test_same_version_allowed(self):
    runtime_version.ValidateProtobufRuntimeVersion(
        runtime_version.DOMAIN,
        runtime_version.MAJOR,
        runtime_version.MINOR,
        runtime_version.PATCH,
        runtime_version.SUFFIX,
        'dummy.proto',
    )

  def test_newer_runtime_version_allowed(self):
    runtime_version.ValidateProtobufRuntimeVersion(
        runtime_version.DOMAIN,
        runtime_version.MAJOR,
        runtime_version.MINOR - 1,
        runtime_version.PATCH,
        runtime_version.SUFFIX,
        'dummy.proto',
    )

  def test_older_runtime_version_disallowed(self):
    with self.assertRaisesRegex(
        runtime_version.VersionError,
        'Detected incompatible Protobuf Gencode/Runtime versions when'
        ' loading foo.proto',
    ):
      runtime_version.ValidateProtobufRuntimeVersion(
          runtime_version.DOMAIN,
          runtime_version.MAJOR,
          runtime_version.MINOR + 1,
          runtime_version.PATCH,
          runtime_version.SUFFIX,
          'foo.proto',
      )

    with self.assertRaisesRegex(
        runtime_version.VersionError,
        'Detected incompatible Protobuf Gencode/Runtime versions when'
        ' loading foo.proto',
    ):
      runtime_version.ValidateProtobufRuntimeVersion(
          runtime_version.DOMAIN,
          runtime_version.MAJOR,
          runtime_version.MINOR,
          runtime_version.PATCH + 1,
          runtime_version.SUFFIX,
          'foo.proto',
      )


if __name__ == '__main__':
  unittest.main()
