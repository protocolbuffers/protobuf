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

from google.protobuf.internal import reflection_test
import unittest

reflection_test.ByteSizeTest.testRepeatedCompositesDelete.__unittest_expecting_failure__ = True
reflection_test.ByteSizeTest.testRepeatedScalarsRemove.__unittest_expecting_failure__ = True
reflection_test.ClassAPITest.testMakeClassWithNestedDescriptor.__unittest_expecting_failure__ = True
reflection_test.Proto2ReflectionTest.testExtensionContainsError.__unittest_expecting_failure__ = True
reflection_test.Proto2ReflectionTest.testExtensionDelete.__unittest_expecting_failure__ = True
reflection_test.Proto2ReflectionTest.testExtensionFailureModes.__unittest_expecting_failure__ = True
reflection_test.Proto2ReflectionTest.testExtensionIter.__unittest_expecting_failure__ = True
reflection_test.Proto2ReflectionTest.testIsInitialized.__unittest_expecting_failure__ = True
reflection_test.Proto2ReflectionTest.testListFieldsAndExtensions.__unittest_expecting_failure__ = True
reflection_test.Proto2ReflectionTest.testNestedExtensions.__unittest_expecting_failure__ = True
reflection_test.Proto2ReflectionTest.testRepeatedCompositeRemove.__unittest_expecting_failure__ = True
reflection_test.Proto2ReflectionTest.testRepeatedCompositeReverse_Empty.__unittest_expecting_failure__ = True
reflection_test.Proto2ReflectionTest.testRepeatedCompositeReverse_NonEmpty.__unittest_expecting_failure__ = True
reflection_test.Proto2ReflectionTest.testRepeatedListExtensions.__unittest_expecting_failure__ = True
reflection_test.Proto2ReflectionTest.testRepeatedScalars.__unittest_expecting_failure__ = True
reflection_test.Proto2ReflectionTest.testRepeatedScalarsRemove.__unittest_expecting_failure__ = True
reflection_test.Proto2ReflectionTest.testRepeatedScalarsReverse_Empty.__unittest_expecting_failure__ = True
reflection_test.Proto2ReflectionTest.testRepeatedScalarsReverse_NonEmpty.__unittest_expecting_failure__ = True
reflection_test.Proto2ReflectionTest.testSingularListExtensions.__unittest_expecting_failure__ = True
reflection_test.Proto2ReflectionTest.testStringUTF8Serialization.__unittest_expecting_failure__ = True
reflection_test.Proto2ReflectionTest.testTopLevelExtensionsForOptionalMessage.__unittest_expecting_failure__ = True
reflection_test.Proto2ReflectionTest.testTopLevelExtensionsForOptionalScalar.__unittest_expecting_failure__ = True
reflection_test.Proto2ReflectionTest.testTopLevelExtensionsForRepeatedMessage.__unittest_expecting_failure__ = True
reflection_test.Proto2ReflectionTest.testTopLevelExtensionsForRepeatedScalar.__unittest_expecting_failure__ = True
reflection_test.ReflectionTest.testClearFieldWithUnknownFieldName_proto2.__unittest_expecting_failure__ = True
reflection_test.ReflectionTest.testClearFieldWithUnknownFieldName_proto3.__unittest_expecting_failure__ = True
reflection_test.ReflectionTest.testConstructorTypeError_proto2.__unittest_expecting_failure__ = True
reflection_test.ReflectionTest.testConstructorTypeError_proto3.__unittest_expecting_failure__ = True
reflection_test.ReflectionTest.testDeepCopy_proto2.__unittest_expecting_failure__ = True
reflection_test.ReflectionTest.testDeepCopy_proto3.__unittest_expecting_failure__ = True
reflection_test.ReflectionTest.testDisconnectingBeforeClear_proto2.__unittest_expecting_failure__ = True
reflection_test.ReflectionTest.testDisconnectingBeforeClear_proto3.__unittest_expecting_failure__ = True
reflection_test.ReflectionTest.testEnum_KeysAndValues_proto2.__unittest_expecting_failure__ = True
reflection_test.ReflectionTest.testEnum_KeysAndValues_proto3.__unittest_expecting_failure__ = True
reflection_test.ReflectionTest.testEnum_Name_proto2.__unittest_expecting_failure__ = True
reflection_test.ReflectionTest.testEnum_Name_proto3.__unittest_expecting_failure__ = True
reflection_test.ReflectionTest.testEnum_Value_proto2.__unittest_expecting_failure__ = True
reflection_test.ReflectionTest.testEnum_Value_proto3.__unittest_expecting_failure__ = True
reflection_test.ReflectionTest.testIllegalValuesForIntegers_proto2.__unittest_expecting_failure__ = True
reflection_test.ReflectionTest.testIllegalValuesForIntegers_proto3.__unittest_expecting_failure__ = True
reflection_test.ReflectionTest.testRepeatedScalarTypeSafety_proto2.__unittest_expecting_failure__ = True
reflection_test.ReflectionTest.testRepeatedScalarTypeSafety_proto3.__unittest_expecting_failure__ = True
reflection_test.ReflectionTest.testSingleScalarTypeSafety_proto2.__unittest_expecting_failure__ = True
reflection_test.ReflectionTest.testSingleScalarTypeSafety_proto3.__unittest_expecting_failure__ = True
reflection_test.SerializationTest.testCanonicalSerializationOrder.__unittest_expecting_failure__ = True
reflection_test.SerializationTest.testCanonicalSerializationOrderSameAsCpp.__unittest_expecting_failure__ = True
reflection_test.SerializationTest.testExtensionFieldNumbers.__unittest_expecting_failure__ = True
reflection_test.SerializationTest.testFieldDataDescriptor.__unittest_expecting_failure__ = True
reflection_test.SerializationTest.testFieldNumbers.__unittest_expecting_failure__ = True
reflection_test.SerializationTest.testFieldProperties.__unittest_expecting_failure__ = True
reflection_test.SerializationTest.testInitArgsUnknownFieldName.__unittest_expecting_failure__ = True
reflection_test.SerializationTest.testInitKwargs.__unittest_expecting_failure__ = True
reflection_test.SerializationTest.testInitRepeatedKwargs.__unittest_expecting_failure__ = True
reflection_test.SerializationTest.testInitRequiredForeignKwargs.__unittest_expecting_failure__ = True
reflection_test.SerializationTest.testInitRequiredKwargs.__unittest_expecting_failure__ = True
reflection_test.SerializationTest.testSerializeUninitialized.__unittest_expecting_failure__ = True
reflection_test.SerializationTest.testSerializeUninitializedSubMessage.__unittest_expecting_failure__ = True

if __name__ == '__main__':
  unittest.main(module=reflection_test, verbosity=2)
