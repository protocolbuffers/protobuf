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

from google.protobuf.internal import message_test
import unittest

message_test.MessageTest.testAppendRepeatedCompositeField_proto2.__unittest_expecting_failure__ = True
message_test.MessageTest.testAppendRepeatedCompositeField_proto3.__unittest_expecting_failure__ = True
message_test.MessageTest.testBadUtf8String_proto2.__unittest_expecting_failure__ = True
message_test.MessageTest.testBadUtf8String_proto3.__unittest_expecting_failure__ = True
message_test.MessageTest.testExtendFloatWithNothing_proto2.__unittest_expecting_failure__ = True
message_test.MessageTest.testExtendFloatWithNothing_proto3.__unittest_expecting_failure__ = True
message_test.MessageTest.testExtendInt32WithNothing_proto2.__unittest_expecting_failure__ = True
message_test.MessageTest.testExtendInt32WithNothing_proto3.__unittest_expecting_failure__ = True
message_test.MessageTest.testExtendStringWithNothing_proto2.__unittest_expecting_failure__ = True
message_test.MessageTest.testExtendStringWithNothing_proto3.__unittest_expecting_failure__ = True
message_test.MessageTest.testFloatPrinting_proto2.__unittest_expecting_failure__ = True
message_test.MessageTest.testFloatPrinting_proto3.__unittest_expecting_failure__ = True
message_test.MessageTest.testGoldenMessage_proto2.__unittest_expecting_failure__ = True
message_test.MessageTest.testGoldenMessage_proto3.__unittest_expecting_failure__ = True
message_test.MessageTest.testGoldenPackedMessage_proto2.__unittest_expecting_failure__ = True
message_test.MessageTest.testGoldenPackedMessage_proto3.__unittest_expecting_failure__ = True
message_test.MessageTest.testHighPrecisionDoublePrinting_proto2.__unittest_expecting_failure__ = True
message_test.MessageTest.testHighPrecisionDoublePrinting_proto3.__unittest_expecting_failure__ = True
message_test.MessageTest.testInsertRepeatedCompositeField_proto2.__unittest_expecting_failure__ = True
message_test.MessageTest.testInsertRepeatedCompositeField_proto3.__unittest_expecting_failure__ = True
message_test.MessageTest.testLongValuedSlice_proto2.__unittest_expecting_failure__ = True
message_test.MessageTest.testLongValuedSlice_proto3.__unittest_expecting_failure__ = True
message_test.MessageTest.testMergeFromRepeatedField_proto2.__unittest_expecting_failure__ = True
message_test.MessageTest.testMergeFromRepeatedField_proto3.__unittest_expecting_failure__ = True
message_test.MessageTest.testMergeFromString_proto2.__unittest_expecting_failure__ = True
message_test.MessageTest.testMergeFromString_proto3.__unittest_expecting_failure__ = True
message_test.MessageTest.testMergeFrom_proto2.__unittest_expecting_failure__ = True
message_test.MessageTest.testMergeFrom_proto3.__unittest_expecting_failure__ = True
message_test.MessageTest.testOneofMessageMergeFrom_proto2.__unittest_expecting_failure__ = True
message_test.MessageTest.testOneofMessageMergeFrom_proto3.__unittest_expecting_failure__ = True
message_test.MessageTest.testOneofNestedMergeFrom_proto2.__unittest_expecting_failure__ = True
message_test.MessageTest.testOneofNestedMergeFrom_proto3.__unittest_expecting_failure__ = True
message_test.MessageTest.testPickleNestedMessage_proto2.__unittest_expecting_failure__ = True
message_test.MessageTest.testPickleNestedMessage_proto3.__unittest_expecting_failure__ = True
message_test.MessageTest.testPickleNestedNestedMessage_proto2.__unittest_expecting_failure__ = True
message_test.MessageTest.testPickleNestedNestedMessage_proto3.__unittest_expecting_failure__ = True
message_test.MessageTest.testPickleSupport_proto2.__unittest_expecting_failure__ = True
message_test.MessageTest.testPickleSupport_proto3.__unittest_expecting_failure__ = True
message_test.MessageTest.testRepeatedCompositeFieldPop_proto2.__unittest_expecting_failure__ = True
message_test.MessageTest.testRepeatedCompositeFieldPop_proto3.__unittest_expecting_failure__ = True
message_test.MessageTest.testRepeatedCompositeFieldSortArguments_proto2.__unittest_expecting_failure__ = True
message_test.MessageTest.testRepeatedCompositeFieldSortArguments_proto3.__unittest_expecting_failure__ = True
message_test.MessageTest.testRepeatedContains_proto2.__unittest_expecting_failure__ = True
message_test.MessageTest.testRepeatedContains_proto3.__unittest_expecting_failure__ = True
message_test.MessageTest.testRepeatedNestedFieldIteration_proto2.__unittest_expecting_failure__ = True
message_test.MessageTest.testRepeatedNestedFieldIteration_proto3.__unittest_expecting_failure__ = True
message_test.MessageTest.testSortingRepeatedCompositeFieldsCustomComparator_proto2.__unittest_expecting_failure__ = True
message_test.MessageTest.testSortingRepeatedCompositeFieldsCustomComparator_proto3.__unittest_expecting_failure__ = True
message_test.MessageTest.testSortingRepeatedCompositeFieldsStable_proto2.__unittest_expecting_failure__ = True
message_test.MessageTest.testSortingRepeatedCompositeFieldsStable_proto3.__unittest_expecting_failure__ = True
message_test.Proto2Test.testExtensionsErrors.__unittest_expecting_failure__ = True
message_test.Proto2Test.testFieldPresence.__unittest_expecting_failure__ = True
message_test.Proto2Test.testGoldenExtensions.__unittest_expecting_failure__ = True
message_test.Proto2Test.testGoldenPackedExtensions.__unittest_expecting_failure__ = True
message_test.Proto2Test.testMergeFromExtensions.__unittest_expecting_failure__ = True
message_test.Proto2Test.testParsingMerge.__unittest_expecting_failure__ = True
message_test.Proto2Test.testPickleIncompleteProto.__unittest_expecting_failure__ = True
message_test.Proto2Test.testPythonicInit.__unittest_expecting_failure__ = True
message_test.Proto2Test.testUnknownEnumMap.__unittest_expecting_failure__ = True
message_test.Proto2Test.test_documentation.__unittest_expecting_failure__ = True
message_test.Proto3Test.testCopyFromBadType.__unittest_expecting_failure__ = True
message_test.Proto3Test.testFieldPresence.__unittest_expecting_failure__ = True
message_test.Proto3Test.testIntegerMapWithLongs.__unittest_expecting_failure__ = True
message_test.Proto3Test.testMapAssignmentCausesPresence.__unittest_expecting_failure__ = True
message_test.Proto3Test.testMapAssignmentCausesPresenceForSubmessages.__unittest_expecting_failure__ = True
message_test.Proto3Test.testMapByteSize.__unittest_expecting_failure__ = True
message_test.Proto3Test.testMapConstruction.__unittest_expecting_failure__ = True
message_test.Proto3Test.testMapDelete.__unittest_expecting_failure__ = True
message_test.Proto3Test.testMapDeterministicSerialization.__unittest_expecting_failure__ = True
message_test.Proto3Test.testMapEntryAlwaysSerialized.__unittest_expecting_failure__ = True
message_test.Proto3Test.testMapFieldRaisesCorrectError.__unittest_expecting_failure__ = True
message_test.Proto3Test.testMapFindInitializationErrorsSmokeTest.__unittest_expecting_failure__ = True
message_test.Proto3Test.testMapGet.__unittest_expecting_failure__ = True
message_test.Proto3Test.testMapItems.__unittest_expecting_failure__ = True
message_test.Proto3Test.testMapIterInvalidatedByClearField.__unittest_expecting_failure__ = True
message_test.Proto3Test.testMapIteration.__unittest_expecting_failure__ = True
message_test.Proto3Test.testMapIterationClearMessage.__unittest_expecting_failure__ = True
message_test.Proto3Test.testMapMergeFrom.__unittest_expecting_failure__ = True
message_test.Proto3Test.testMapMessageFieldConstruction.__unittest_expecting_failure__ = True
message_test.Proto3Test.testMapScalarFieldConstruction.__unittest_expecting_failure__ = True
message_test.Proto3Test.testMapValidAfterFieldCleared.__unittest_expecting_failure__ = True
message_test.Proto3Test.testMapsAreMapping.__unittest_expecting_failure__ = True
message_test.Proto3Test.testMapsCompare.__unittest_expecting_failure__ = True
message_test.Proto3Test.testMergeFrom.__unittest_expecting_failure__ = True
message_test.Proto3Test.testMergeFromBadType.__unittest_expecting_failure__ = True
message_test.Proto3Test.testMessageMap.__unittest_expecting_failure__ = True
message_test.Proto3Test.testMessageMapItemValidAfterTopMessageCleared.__unittest_expecting_failure__ = True
message_test.Proto3Test.testMessageMapValidAfterFieldCleared.__unittest_expecting_failure__ = True
message_test.Proto3Test.testModifyMapWhileIterating.__unittest_expecting_failure__ = True
message_test.Proto3Test.testNestedMessageMapItemDelete.__unittest_expecting_failure__ = True
message_test.Proto3Test.testProto3Optional.__unittest_expecting_failure__ = True
message_test.Proto3Test.testProto3ParserDropDefaultScalar.__unittest_expecting_failure__ = True
message_test.Proto3Test.testScalarMap.__unittest_expecting_failure__ = True
message_test.Proto3Test.testScalarMapDefaults.__unittest_expecting_failure__ = True
message_test.Proto3Test.testStringUnicodeConversionInMap.__unittest_expecting_failure__ = True
message_test.Proto3Test.testSubmessageMap.__unittest_expecting_failure__ = True

# We must skip these tests entirely (rather than running them with
# __unittest_expecting_failure__) because they error out in setUpClass():
#
#  AttributeError: 'google.protobuf.pyext._message.DescriptorPool' object has no attribute 'Add'
#
# TODO: change to __unittest_expecting_failure__ when DescriptorPoo.Add is implemented
message_test.OversizeProtosTest.__unittest_skip__ = True

if __name__ == '__main__':
  unittest.main(module=message_test, verbosity=2)
