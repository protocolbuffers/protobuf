"""Exercises proto_api.h. Most of the code is in proto_api_test.cc."""

from google.protobuf.internal import proto_api_test_ext
from google3.testing.pybase import unittest
from google.protobuf import unittest_pb2


class ProtoApiTest(unittest.TestCase):

  def test_get_const_message(self):
    msg = unittest_pb2.TestAllTypes(optional_int32=123, optional_string='hello')
    result = proto_api_test_ext.get_const_message(msg)
    self.assertEqual(result, (123, 'hello'))

  def test_set_message_field_with_mutator(self):
    msg = unittest_pb2.TestAllTypes(optional_int32=123, optional_string='hello')
    proto_api_test_ext.set_message_field_with_mutator(msg, 456)
    # GetClearedMessageMutator clears message first, then update it.
    # On destruction, it copies content back to python message.
    self.assertEqual(456, msg.optional_int32)
    self.assertFalse(msg.HasField('optional_string'))

  def test_dynamic_message(self):
    result = proto_api_test_ext.repr_dynamic_message(789)
    self.assertEqual(result, 'optional_int32: 789\n')

  def test_dynamic_pool_message(self):
    result = proto_api_test_ext.create_dynamic_pool_message()
    self.assertEqual(result.DESCRIPTOR.full_name, 'test_package.MyMessage')
    self.assertEqual(result.my_field, 42)


if __name__ == '__main__':
  unittest.main()
