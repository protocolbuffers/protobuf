#! /usr/bin/python
#
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

"""Test that the api_implementation defaults are what we expect."""

import os
import sys
# Clear environment implementation settings before the google3 imports.
os.environ.pop('PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION', None)
os.environ.pop('PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION_VERSION', None)

# pylint: disable=g-import-not-at-top
from google.apputils import basetest
from google.protobuf.internal import api_implementation


class ApiImplementationDefaultTest(basetest.TestCase):

  if sys.version_info.major <= 2:

    def testThatPythonIsTheDefault(self):
      """If -DPYTHON_PROTO_*IMPL* was given at build time, this may fail."""
      self.assertEqual('python', api_implementation.Type())

  else:

    def testThatCppApiV2IsTheDefault(self):
      """If -DPYTHON_PROTO_*IMPL* was given at build time, this may fail."""
      self.assertEqual('cpp', api_implementation.Type())
      self.assertEqual(2, api_implementation.Version())


if __name__ == '__main__':
  basetest.main()
