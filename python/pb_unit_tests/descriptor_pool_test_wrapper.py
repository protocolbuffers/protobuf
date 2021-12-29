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

from google.protobuf.internal import descriptor_pool_test
import unittest

print(unittest)

descriptor_pool_test.AddDescriptorTest.testAddTypeError.__unittest_expecting_failure__ = True
descriptor_pool_test.AddDescriptorTest.testEmptyDescriptorPool.__unittest_expecting_failure__ = True
descriptor_pool_test.AddDescriptorTest.testEnum.__unittest_expecting_failure__ = True
descriptor_pool_test.AddDescriptorTest.testFile.__unittest_expecting_failure__ = True
descriptor_pool_test.AddDescriptorTest.testFileDescriptorOptionsWithCustomDescriptorPool.__unittest_expecting_failure__ = True
descriptor_pool_test.AddDescriptorTest.testMessage.__unittest_expecting_failure__ = True
descriptor_pool_test.AddDescriptorTest.testService.__unittest_expecting_failure__ = True

# We must skip these tests entirely (rather than running them with
# __unittest_expecting_failure__) because they error out in setUp():
#
#  AttributeError: 'google.protobuf.pyext._message.FileDescriptor' object has no attribute 'serialized_pb'
#
# TODO: change to __unittest_expecting_failure__ when serialized_pb is available.
descriptor_pool_test.CreateDescriptorPoolTest.testAddSerializedFile.__unittest_skip__ = True
descriptor_pool_test.CreateDescriptorPoolTest.testComplexNesting.__unittest_skip__ = True
descriptor_pool_test.CreateDescriptorPoolTest.testConflictRegister.__unittest_skip__ = True
descriptor_pool_test.CreateDescriptorPoolTest.testDefaultValueForCustomMessages.__unittest_skip__ = True
descriptor_pool_test.CreateDescriptorPoolTest.testEnumDefaultValue.__unittest_skip__ = True
descriptor_pool_test.CreateDescriptorPoolTest.testExtensionsAreNotFields.__unittest_skip__ = True
descriptor_pool_test.CreateDescriptorPoolTest.testFindAllExtensions.__unittest_skip__ = True
descriptor_pool_test.CreateDescriptorPoolTest.testFindEnumTypeByName.__unittest_skip__ = True
descriptor_pool_test.CreateDescriptorPoolTest.testFindEnumTypeByNameFailure.__unittest_skip__ = True
descriptor_pool_test.CreateDescriptorPoolTest.testFindExtensionByName.__unittest_skip__ = True
descriptor_pool_test.CreateDescriptorPoolTest.testFindExtensionByNumber.__unittest_skip__ = True
descriptor_pool_test.CreateDescriptorPoolTest.testFindFieldByName.__unittest_skip__ = True
descriptor_pool_test.CreateDescriptorPoolTest.testFindFileByName.__unittest_skip__ = True
descriptor_pool_test.CreateDescriptorPoolTest.testFindFileByNameFailure.__unittest_skip__ = True
descriptor_pool_test.CreateDescriptorPoolTest.testFindFileContainingSymbol.__unittest_skip__ = True
descriptor_pool_test.CreateDescriptorPoolTest.testFindFileContainingSymbolFailure.__unittest_skip__ = True
descriptor_pool_test.CreateDescriptorPoolTest.testFindMessageTypeByName.__unittest_skip__ = True
descriptor_pool_test.CreateDescriptorPoolTest.testFindMessageTypeByNameFailure.__unittest_skip__ = True
descriptor_pool_test.DefaultDescriptorPoolTest.testFindMethods.__unittest_skip__ = True
descriptor_pool_test.CreateDescriptorPoolTest.testFindOneofByName.__unittest_skip__ = True
descriptor_pool_test.CreateDescriptorPoolTest.testFindService.__unittest_skip__ = True
descriptor_pool_test.CreateDescriptorPoolTest.testFindTypeErrors.__unittest_skip__ = True
descriptor_pool_test.CreateDescriptorPoolTest.testUserDefinedDB.__unittest_skip__ = True
descriptor_pool_test.CreateDescriptorPoolTest.testAddFileDescriptor.__unittest_skip__ = True
descriptor_pool_test.SecondaryDescriptorFromDescriptorDB.testErrorCollector.__unittest_skip__ = True

if __name__ == '__main__':
  unittest.main(module=descriptor_pool_test, verbosity=2)
