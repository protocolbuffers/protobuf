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

"""Tests for google.protobuf.python."""
from __future__ import print_function
#from google3.testing.pybase import unittest
import google3
from google.protobuf.internal import descriptor_database_test
from google.protobuf.internal import descriptor_pool_test
from google.protobuf.internal import descriptor_test
from google.protobuf.internal import generator_test
from google.protobuf.internal import message_factory_test
from google.protobuf.internal import message_test
from google.protobuf.internal import proto_builder_test
from google.protobuf.internal import reflection_test
from google.protobuf.internal import service_reflection_test
from google.protobuf.internal import symbol_database_test
from google.protobuf.internal import text_encoding_test
from google.protobuf.internal import text_format_test
from google.protobuf.internal import unknown_fields_test
from google.protobuf.internal import wire_format_test
from google.protobuf.internal import well_known_types_test
import sys
import unittest

class ProtobufTests(unittest.TestCase):

  def testAll(self):
    testModules = [
      descriptor_database_test,
      descriptor_pool_test,
      descriptor_test,
      generator_test,
      message_factory_test,
      message_test,
      proto_builder_test,
      reflection_test,
      service_reflection_test,
      symbol_database_test,
      text_encoding_test,
      text_format_test,
      unknown_fields_test,
      wire_format_test,
      well_known_types_test,
    ]
    testSuite = unittest.TestSuite()
    for testModule in testModules:
      testSuite.addTests(
        unittest.defaultTestLoader.loadTestsFromModule(
          testModule))
    testResult = unittest.TestResult()
    testSuite.run(testResult)
    print(testResult.errors, file=sys.stderr)
    print(testResult.failures)
    self.assertTrue(testResult.wasSuccessful())

if __name__ == '__main__':
  unittest.main()
