"""Exercises proto_api.h. Most of the code is in proto_api_test.cc."""

import os
import subprocess
import sys

from google.protobuf.internal import proto_api_test_ext
from google3.testing.pybase import unittest
from google.protobuf import unittest_pb2

# Check if we should trigger the death test before running tests.
if os.environ.get('DEATH_TEST_TRIGGER_CRASH') == '1':
  proto_api_test_ext.trigger_descriptor_pool_destruction_crash()
  sys.exit(1)


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

  def test_dynamic_message_shared_pool(self):
    result = proto_api_test_ext.repr_dynamic_message_shared_pool(789)
    self.assertEqual(result, 'optional_int32: 789\n')

  def test_dynamic_message_shared_pool_and_db(self):
    result = proto_api_test_ext.repr_dynamic_message_shared_pool_and_db(789)
    self.assertEqual(result, 'val: 789\n')

  def test_dynamic_pool_message(self):
    result = proto_api_test_ext.create_dynamic_pool_message()
    self.assertEqual(result.DESCRIPTOR.full_name, 'test_package.MyMessage')
    self.assertEqual(result.my_field, 42)

  def test_descriptor_pool_destruction_crash(self):

    # Re-run death test with DEATH_TEST_TRIGGER_CRASH=1 in env.
    env = dict(os.environ)
    env['DEATH_TEST_TRIGGER_CRASH'] = '1'

    cmd = [sys.argv[0]]
    process = subprocess.Popen(
        cmd, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE
    )
    _, stderr = process.communicate()

    self.assertNotEqual(process.returncode, 0)
    self.assertIn(
        b'DescriptorPool destroyed while Python still holds a reference',
        stderr,
    )


if __name__ == '__main__':
  unittest.main()
