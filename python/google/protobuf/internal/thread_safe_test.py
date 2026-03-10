# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Unittest for thread safe"""

import sys
import threading
import time
import unittest

from google.protobuf import descriptor_pb2
from google.protobuf import descriptor_pool
from google.protobuf import message_factory
from google.protobuf.internal import api_implementation

from google.protobuf import unittest_pb2
from google.protobuf import unittest_proto3_pb2


class ThreadSafeTest(unittest.TestCase):

  def setUp(self):
    self.success = 0

  def testFieldDecodersDataRace(self):
    msg = unittest_pb2.TestAllTypes(optional_int32=1)
    serialized_data = msg.SerializeToString()
    lock = threading.Lock()

    def ParseMessage():
      parsed_msg = unittest_pb2.TestAllTypes()
      time.sleep(0.005)
      parsed_msg.ParseFromString(serialized_data)
      with lock:
        if msg == parsed_msg:
          self.success += 1

    field_des = unittest_pb2.TestAllTypes.DESCRIPTOR.fields_by_name[
        'optional_int32'
    ]
    count = 1000
    for x in range(0, count):
      # delete the _decoders because only the first time parse the field
      # may cause data race.
      if hasattr(field_des, '_decoders'):
        delattr(field_des, '_decoders')
      thread1 = threading.Thread(target=ParseMessage)
      thread2 = threading.Thread(target=ParseMessage)
      thread1.start()
      thread2.start()
      thread1.join()
      thread2.join()

    self.assertEqual(count * 2, self.success)

  # This caused a Dealloc()/Dealloc() race.
  @unittest.skipIf(
      api_implementation.Type() == 'upb',
      'Upb has not been fixed to handle this case.',
  )
  def testGetType(self):

    def GetType():
      msg = unittest_proto3_pb2.TestAllTypes(
          optional_nested_message=unittest_proto3_pb2.TestAllTypes.NestedMessage(
              bb=1000
          ),
          optional_nested_enum=unittest_proto3_pb2.TestAllTypes.NestedEnum.ZERO,
      )
      msges = [msg] * 100
      for m in msges:
        # Fails in this line:
        unittest_proto3_pb2.TestAllTypes.NestedEnum.Name(m.optional_nested_enum)

    threads = []
    for i in range(100):
      thread = threading.Thread(target=GetType)
      threads.append(thread)
      thread.start()

    for thread in threads:
      thread.join()

  # This caused a race between constructing and using the type.
  @unittest.skipIf(
      api_implementation.Type() == 'upb',
      'Upb has not been fixed to handle this case.',
  )
  def testInitType(self):

    def InitType():
      array = []
      for i in range(100):
        array.append(
            unittest_proto3_pb2.TestAllTypes(
                optional_nested_message=unittest_proto3_pb2.TestAllTypes.NestedMessage(
                    bb=1000
                ),
                optional_nested_enum=unittest_proto3_pb2.TestAllTypes.NestedEnum.FOO,
            )
        )

    threads = []
    for i in range(100):
      thread = threading.Thread(target=InitType)
      threads.append(thread)
      thread.start()

    for thread in threads:
      thread.join()


class FreeThreadingTest(unittest.TestCase):

  def RunThreads(self, thread_size, func):
    threads = []
    for i in range(0, thread_size):
      threads.append(threading.Thread(target=func))
    for thread in threads:
      thread.start()
    for thread in threads:
      thread.join()

  def testDoNothing(self):
    thread_size = 10

    def DoNothing():
      return

    self.RunThreads(thread_size, DoNothing)

  def testDescriptorPoolMap(self):
    thread_size = 20
    self.success_count = 0
    lock = threading.Lock()

    def CreatePool():
      def DoCreate():
        pool = descriptor_pool.DescriptorPool()
        file_proto = descriptor_pb2.FileDescriptorProto(name='foo')
        message_proto = file_proto.message_type.add(name='SomeMessage')
        message_proto.field.add(
            name='int_field',
            number=1,
            type=descriptor_pb2.FieldDescriptorProto.TYPE_INT32,
            label=descriptor_pb2.FieldDescriptorProto.LABEL_OPTIONAL,
        )
        pool.Add(file_proto)
        desc = pool.FindMessageTypeByName('SomeMessage')
        msg = message_factory.GetMessageClass(desc)()
        msg.int_field = 1

      DoCreate()
      with lock:
        self.success_count += 1

    self.RunThreads(thread_size, CreatePool)
    self.assertEqual(thread_size, self.success_count)


if __name__ == '__main__':
  unittest.main()
