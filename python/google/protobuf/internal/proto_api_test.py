"""Exercises proto_api.h. Most of the code is in proto_api_test.cc."""

from google.protobuf import descriptor_pb2
from google.protobuf import descriptor_pool
from google.protobuf import message_factory
from google.protobuf.internal import api_implementation
from google.protobuf.internal import proto_api_test_ext
from google.protobuf.internal import testing_refleaks
from google3.testing.pybase import unittest
from google.protobuf import unittest_pb2


@testing_refleaks.TestCase
class ProtoApiTest(unittest.TestCase):

  def _verify_pool_type(self, custom_pool):
    backend = api_implementation.Type()

    if backend == 'cpp':
      self.assertEqual(type(custom_pool).__name__, 'DescriptorPool')
      # In google3 it might be google3.net.proto2.python.internal.cpp._message
      # In opensource it is google.protobuf.pyext._message
      self.assertIn(
          type(custom_pool).__module__,
          [
              'google.protobuf.pyext._message',
              'google3.net.proto2.python.internal.cpp._message',
          ],
      )
    elif backend == 'upb':
      self.assertEqual(type(custom_pool).__name__, 'DescriptorPool')
      self.assertEqual(
          type(custom_pool).__module__,
          'google._upb._message',
      )
    elif backend == 'python':
      self.assertEqual(type(custom_pool).__name__, 'DescriptorPool')
      self.assertEqual(
          type(custom_pool).__module__, 'google.protobuf.descriptor_pool'
      )

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

  def test_get_pool_from_message(self):
    custom_pool = descriptor_pool.DescriptorPool()

    file_proto = descriptor_pb2.FileDescriptorProto()
    file_proto.name = 'custom_file.proto'
    file_proto.package = 'custom_package'
    msg_proto = file_proto.message_type.add()
    msg_proto.name = 'CustomMessage'
    custom_pool.Add(file_proto)

    # 2. Create message by GetMessageClass
    msg_class = message_factory.GetMessageClass(
        custom_pool.FindMessageTypeByName('custom_package.CustomMessage')
    )
    msg1 = msg_class()

    # 3. Pass in msg1 to extension
    returned_pool = proto_api_test_ext.get_pool_from_message(msg1)

    self.assertIs(returned_pool, custom_pool)

  def test_get_pool_from_generated_message(self):
    msg = unittest_pb2.TestAllTypes()
    returned_pool = proto_api_test_ext.get_pool_from_message(msg)
    self.assertIs(returned_pool, descriptor_pool.Default())

  def test_get_custom_pool_with_db(self):
    custom_pool = proto_api_test_ext.get_custom_pool_with_db()

    # Verify descriptor in pool can be found
    msg_desc = custom_pool.FindMessageTypeByName('custom_package.CustomMessage')
    self.assertIsNotNone(msg_desc)
    self.assertEqual(msg_desc.name, 'CustomMessage')

    # Verify descriptor only in database can be found
    db_msg_desc = custom_pool.FindMessageTypeByName(
        'custom_package.DatabaseMessage'
    )
    self.assertIsNotNone(db_msg_desc)
    self.assertEqual(db_msg_desc.name, 'DatabaseMessage')

    self._verify_pool_type(custom_pool)

  def test_get_custom_pool(self):
    custom_pool = proto_api_test_ext.get_custom_pool()

    # Verify descriptor in pool can be found
    msg_desc = custom_pool.FindMessageTypeByName('custom_package.CustomMessage')
    self.assertIsNotNone(msg_desc)
    self.assertEqual(msg_desc.name, 'CustomMessage')

    self._verify_pool_type(custom_pool)


if __name__ == '__main__':
  unittest.main()
