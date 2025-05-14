# -*- coding: utf-8 -*-
# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Tests proto Duration APIs."""

import datetime
import unittest

from google.protobuf import duration
from google.protobuf.internal import testing_refleaks
from google.protobuf.internal import well_known_types_test_pb2

from google.protobuf import duration_pb2


@testing_refleaks.TestCase
class DurationTest(unittest.TestCase):

  def test_duration_integer_conversion(self):
    self.assertEqual(1, duration.to_nanoseconds(duration.from_nanoseconds(1)))
    self.assertEqual(-1, duration.to_seconds(duration.from_seconds(-1)))
    self.assertEqual(
        123, duration.to_milliseconds(duration.from_milliseconds(123))
    )
    self.assertEqual(
        321, duration.to_microseconds(duration.from_microseconds(321))
    )

  def test_duration_json(self):

    def check_duration(message, text):
      self.assertEqual(text, duration.to_json_string(message))
      parsed_duration = duration.from_json_string(text)
      self.assertEqual(message, parsed_duration)

    message = duration_pb2.Duration()
    message.seconds = 0
    message.nanos = 0
    check_duration(message, '0s')
    message.nanos = 10000000
    check_duration(message, '0.010s')
    message.nanos = 10000
    check_duration(message, '0.000010s')
    message.nanos = 10
    check_duration(message, '0.000000010s')

  def test_duration_timedelta(self):
    message = duration.from_nanoseconds(1999999999)
    td = duration.to_timedelta(message)
    self.assertEqual(1, td.seconds)
    self.assertEqual(999999, td.microseconds)

    message = duration.from_microseconds(-1)
    td = duration.to_timedelta(message)
    converted_message = duration.from_timedelta(td)
    self.assertEqual(message, converted_message)

  def test_duration_construction(self):
    expected_td = datetime.timedelta(microseconds=123)
    message = well_known_types_test_pb2.WKTMessage(
        optional_duration=expected_td
    )
    self.assertEqual(expected_td, message.optional_duration.ToTimedelta())

  def test_duration_sub_annotation(self):
    dt = datetime.datetime.now()
    dr = duration_pb2.Duration()
    td = datetime.timedelta(microseconds=123)
    # datetime - Duration
    self.assertEqual(dt - dr, dt - duration.to_timedelta(dr))
    # timedelta - Duration and Duration - Duration
    self.assertEqual(td - dr, duration.from_timedelta(td) - dr)
    # Duration - timedelta
    self.assertEqual(dr - td, dr - duration.from_timedelta(td))

  def test_duration_add_annotation(self):
    dt = datetime.datetime.now()
    dr = duration_pb2.Duration()
    dr2 = duration_pb2.Duration(seconds=100)
    # datetime + Duration and Duration + datetime
    self.assertEqual(dt + dr, dr + dt)
    message = well_known_types_test_pb2.WKTMessage(optional_timestamp=dt)
    # Duration + Timestamp
    self.assertEqual(dr + message.optional_timestamp, dr + dt)
    td = datetime.timedelta(microseconds=123)
    # Duration + timedelta and timedelta + Duration
    self.assertEqual(dr + td, td + dr)
    # Duration + Duration
    self.assertEqual(dr + dr2, dr2 + dr)


if __name__ == '__main__':
  unittest.main()
