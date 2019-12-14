#! /usr/bin/env python
#
# Protocol Buffers - Google's data interchange format
# Copyright 2019 Google Inc.  All rights reserved.
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

# TODO: Test the high-level API, not the low-level one.
"""Unittest for google.protobuf.protos."""

__author__ = 'rbellevi@google.com (Richard Belleville)'


import sys

try:
  import unittest2 as unittest  #PY26
except ImportError:
  import unittest

from google import protobuf

def _supported():
  return sys.version_info[0] > 2

module = sys.__class__


# TODO: Probably want to run in subprocesses.

class ProtocTest(unittest.TestCase):

  @unittest.skipIf(_supported(), reason="Unsupported")
  def testGracefulFailure(self):
    with self.assertRaises(NotImplementedError):
      protobuf.protos("foo.bar")

  @unittest.skipIf(not _supported(), reason="Unsupported")
  def testFunction(self):
    protos = protobuf.protos("google/protobuf/timestamp.proto",
                             include_paths=["src"])
    self.assertIsInstance(protos, module)
    timestamp = protos.Timestamp()
    self.assertIsNotNone(timestamp.seconds)
    self.assertIsNotNone(timestamp.nanos)


if __name__ == '__main__':
  unittest.main()
