# -*- coding: utf-8 -*-
# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Tests proto Timestamp APIs."""

import datetime
import unittest

from google.protobuf import timestamp
from google.protobuf.internal import well_known_types_test_pb2

from google.protobuf import timestamp_pb2


class TimestampTest(unittest.TestCase):

  def test_timestamp_integer_conversion(self):
    self.assertEqual(1, timestamp.to_nanoseconds(timestamp.from_nanoseconds(1)))
    self.assertEqual(-1, timestamp.to_seconds(timestamp.from_seconds(-1)))
    self.assertEqual(
        123, timestamp.to_milliseconds(timestamp.from_milliseconds(123))
    )
    self.assertEqual(
        321, timestamp.to_microseconds(timestamp.from_microseconds(321))
    )

  def test_timestamp_current(self):
    # It is not easy to check with current time. For test coverage only.
    self.assertNotEqual(8 * 3600, timestamp.from_current_time().seconds)

  def test_timestamp_json(self):

    def check_timestamp(ts, text):
      self.assertEqual(text, timestamp.to_json_string(ts))
      parsed_ts = timestamp.from_json_string(text)
      self.assertEqual(ts, parsed_ts)

    message = timestamp_pb2.Timestamp()
    message.seconds = 0
    message.nanos = 0
    check_timestamp(message, '1970-01-01T00:00:00Z')
    message.nanos = 10000000
    check_timestamp(message, '1970-01-01T00:00:00.010Z')
    message.nanos = 10000
    check_timestamp(message, '1970-01-01T00:00:00.000010Z')

  def test_timestamp_datetime(self):
    naive_utc_epoch = datetime.datetime(1970, 1, 1)
    message = well_known_types_test_pb2.WKTMessage()
    message.optional_timestamp = naive_utc_epoch
    self.assertEqual(0, message.optional_timestamp.seconds)  # pytype: disable=attribute-error
    self.assertEqual(0, message.optional_timestamp.nanos)  # pytype: disable=attribute-error
    self.assertEqual(
        naive_utc_epoch, timestamp.to_datetime(message.optional_timestamp)  # pytype: disable=wrong-arg-types
    )


if __name__ == '__main__':
  unittest.main()
