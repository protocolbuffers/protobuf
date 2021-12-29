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


"""A bare-bones unit test that doesn't load any generated code."""


import unittest
from google.protobuf.pyext import _message
from google.protobuf.internal import api_implementation
from google.protobuf import unittest_pb2

class TestMessageExtension(unittest.TestCase):

    def test_descriptor_pool(self):
        serialized_desc = b'\n\ntest.proto\"\x0e\n\x02M1*\x08\x08\x01\x10\x80\x80\x80\x80\x02:\x15\n\x08test_ext\x12\x03.M1\x18\x01 \x01(\x05'
        pool = _message.DescriptorPool()
        file_desc = pool.AddSerializedFile(serialized_desc)
        self.assertEqual("test.proto", file_desc.name)
        ext_desc = pool.FindExtensionByName("test_ext")
        self.assertEqual(1, ext_desc.number)

        # Test object cache: repeatedly retrieving the same descriptor
        # should result in the same object
        self.assertIs(ext_desc, pool.FindExtensionByName("test_ext"))


    def test_lib_is_upb(self):
        # Ensure we are not pulling in a different protobuf library on the
        # system.
        print(_message._IS_UPB)
        self.assertTrue(_message._IS_UPB)
        self.assertEqual(api_implementation.Type(), "cpp")

    def test_repeated_field_slice_delete(self):
        def test_slice(start, end, step):
            vals = list(range(20))
            message = unittest_pb2.TestAllTypes(repeated_int32=vals)
            del vals[start:end:step]
            del message.repeated_int32[start:end:step]
            self.assertEqual(vals, list(message.repeated_int32))
        test_slice(3, 11, 1)
        test_slice(3, 11, 2)
        test_slice(3, 11, 3)
        test_slice(11, 3, -1)
        test_slice(11, 3, -2)
        test_slice(11, 3, -3)
        test_slice(10, 25, 4)

#TestMessageExtension.test_descriptor_pool.__unittest_expecting_failure__ = True


if __name__ == '__main__':
    unittest.main(verbosity=2)
