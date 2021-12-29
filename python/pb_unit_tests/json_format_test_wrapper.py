# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of Google LLC nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

from google.protobuf.internal import json_format_test
import unittest

json_format_test.JsonFormatTest.testAllFieldsToJson.__unittest_expecting_failure__ = True
json_format_test.JsonFormatTest.testAlwaysSeriliaze.__unittest_expecting_failure__ = True
json_format_test.JsonFormatTest.testAnyMessage.__unittest_expecting_failure__ = True
json_format_test.JsonFormatTest.testAnyMessageDescriptorPoolMissingType.__unittest_expecting_failure__ = True
json_format_test.JsonFormatTest.testDurationMessage.__unittest_expecting_failure__ = True
json_format_test.JsonFormatTest.testEmptyMessageToJson.__unittest_expecting_failure__ = True
json_format_test.JsonFormatTest.testExtensionSerializationDictMatchesProto3Spec.__unittest_expecting_failure__ = True
json_format_test.JsonFormatTest.testExtensionSerializationDictMatchesProto3SpecMore.__unittest_expecting_failure__ = True
json_format_test.JsonFormatTest.testExtensionSerializationJsonMatchesProto3Spec.__unittest_expecting_failure__ = True
json_format_test.JsonFormatTest.testExtensionToDictAndBack.__unittest_expecting_failure__ = True
json_format_test.JsonFormatTest.testExtensionToDictAndBackWithScalar.__unittest_expecting_failure__ = True
json_format_test.JsonFormatTest.testExtensionToJsonAndBack.__unittest_expecting_failure__ = True
json_format_test.JsonFormatTest.testFieldMaskMessage.__unittest_expecting_failure__ = True
json_format_test.JsonFormatTest.testFloatPrecision.__unittest_expecting_failure__ = True
json_format_test.JsonFormatTest.testIgnoreUnknownField.__unittest_expecting_failure__ = True
json_format_test.JsonFormatTest.testInvalidAny.__unittest_expecting_failure__ = True
json_format_test.JsonFormatTest.testInvalidMap.__unittest_expecting_failure__ = True
json_format_test.JsonFormatTest.testJsonEscapeString.__unittest_expecting_failure__ = True
json_format_test.JsonFormatTest.testJsonName.__unittest_expecting_failure__ = True
json_format_test.JsonFormatTest.testJsonParseDictToAnyDoesNotAlterInput.__unittest_expecting_failure__ = True
json_format_test.JsonFormatTest.testListValueMessage.__unittest_expecting_failure__ = True
json_format_test.JsonFormatTest.testMapFields.__unittest_expecting_failure__ = True
json_format_test.JsonFormatTest.testNullValue.__unittest_expecting_failure__ = True
json_format_test.JsonFormatTest.testOneofFields.__unittest_expecting_failure__ = True
json_format_test.JsonFormatTest.testParseDictAnyDescriptorPoolMissingType.__unittest_expecting_failure__ = True
json_format_test.JsonFormatTest.testParseNull.__unittest_expecting_failure__ = True
json_format_test.JsonFormatTest.testPartialMessageToJson.__unittest_expecting_failure__ = True
json_format_test.JsonFormatTest.testStructMessage.__unittest_expecting_failure__ = True
json_format_test.JsonFormatTest.testTimestampMessage.__unittest_expecting_failure__ = True
json_format_test.JsonFormatTest.testUnknownEnumToJsonAndBack.__unittest_expecting_failure__ = True
json_format_test.JsonFormatTest.testValueMessage.__unittest_expecting_failure__ = True
json_format_test.JsonFormatTest.testWellKnownInAnyMessage.__unittest_expecting_failure__ = True
json_format_test.JsonFormatTest.testWrapperMessage.__unittest_expecting_failure__ = True

if __name__ == '__main__':
  unittest.main(module=json_format_test, verbosity=2)
