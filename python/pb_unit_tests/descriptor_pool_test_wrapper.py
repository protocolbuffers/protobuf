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
import copy

print(unittest)

descriptor_pool_test.AddDescriptorTest.testAddTypeError.__unittest_expecting_failure__ = True
descriptor_pool_test.AddDescriptorTest.testEnum.__unittest_expecting_failure__ = True
descriptor_pool_test.AddDescriptorTest.testFile.__unittest_expecting_failure__ = True
descriptor_pool_test.AddDescriptorTest.testMessage.__unittest_expecting_failure__ = True
descriptor_pool_test.AddDescriptorTest.testService.__unittest_expecting_failure__ = True
descriptor_pool_test.CreateDescriptorPoolTest.testFindFieldByName.__unittest_expecting_failure__ = True
descriptor_pool_test.CreateDescriptorPoolTest.testFindMessageTypeByName.__unittest_expecting_failure__ = True
descriptor_pool_test.CreateDescriptorPoolTest.testFindService.__unittest_expecting_failure__ = True
descriptor_pool_test.CreateDescriptorPoolTest.testFindTypeErrors.__unittest_expecting_failure__ = True
descriptor_pool_test.CreateDescriptorPoolTest.testUserDefinedDB.__unittest_expecting_failure__ = True
descriptor_pool_test.SecondaryDescriptorFromDescriptorDB.testErrorCollector.__unittest_expecting_failure__ = True

# Some tests are defined in a base class and inherited by multiple sub-classes.
# If only some of the subclasses fail, we need to duplicate the test method
# before marking it, otherwise the mark will affect all subclasses.
def wrap(cls, method):
  existing = getattr(cls, method)
  setattr(cls, method, lambda self: existing(self))
  getattr(cls, method).__unittest_expecting_failure__ = True

wrap(descriptor_pool_test.CreateDescriptorPoolTest, "testAddSerializedFile")
wrap(descriptor_pool_test.CreateDescriptorPoolTest, "testComplexNesting")
wrap(descriptor_pool_test.DefaultDescriptorPoolTest, "testAddSerializedFile")
wrap(descriptor_pool_test.DefaultDescriptorPoolTest, "testComplexNesting")
wrap(descriptor_pool_test.SecondaryDescriptorFromDescriptorDB, "testFindAllExtensions")
wrap(descriptor_pool_test.SecondaryDescriptorFromDescriptorDB, "testFindEnumTypeByName")
wrap(descriptor_pool_test.SecondaryDescriptorFromDescriptorDB, "testFindExtensionByName")
wrap(descriptor_pool_test.SecondaryDescriptorFromDescriptorDB, "testFindExtensionByNumber")
wrap(descriptor_pool_test.SecondaryDescriptorFromDescriptorDB, "testFindFileByName")
wrap(descriptor_pool_test.SecondaryDescriptorFromDescriptorDB, "testFindFileContainingSymbol")
wrap(descriptor_pool_test.SecondaryDescriptorFromDescriptorDB, "testFindOneofByName")

if __name__ == '__main__':
  unittest.main(module=descriptor_pool_test, verbosity=2)
