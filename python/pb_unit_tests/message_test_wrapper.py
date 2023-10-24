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

from google.protobuf.internal.message_test import *
import unittest

MessageTest.testExtendFloatWithNothing_proto2.__unittest_skip__ = True
MessageTest.testExtendFloatWithNothing_proto3.__unittest_skip__ = True
MessageTest.testExtendInt32WithNothing_proto2.__unittest_skip__ = True
MessageTest.testExtendInt32WithNothing_proto3.__unittest_skip__ = True
MessageTest.testExtendStringWithNothing_proto2.__unittest_skip__ = True
MessageTest.testExtendStringWithNothing_proto3.__unittest_skip__ = True

# Python/C++ customizes the C++ TextFormat to always print trailing ".0" for
# floats. upb doesn't do this, it matches C++ TextFormat.
MessageTest.testFloatPrinting_proto2.__unittest_expecting_failure__ = True
MessageTest.testFloatPrinting_proto3.__unittest_expecting_failure__ = True

# For these tests we are throwing the correct error, only the text of the error
# message is a mismatch.  For technical reasons around the limited API, matching
# the existing error message exactly is not feasible.
Proto3Test.testCopyFromBadType.__unittest_expecting_failure__ = True
Proto3Test.testMergeFromBadType.__unittest_expecting_failure__ = True

Proto2Test.test_documentation.__unittest_expecting_failure__ = True

if __name__ == '__main__':
  unittest.main(verbosity=2)
