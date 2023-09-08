# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Unittest for thread safe"""

import threading
import time
import unittest

from google.protobuf import unittest_pb2


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
    count = 5000
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


if __name__ == '__main__':
  unittest.main()
