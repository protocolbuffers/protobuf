"""A bare-bones unit test, to be removed once upb can pass existing unit tests."""


import unittest
from google.protobuf.pyext import _message

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
        self.assertTrue(_message._IS_UPB)


if __name__ == '__main__':
    unittest.main()
