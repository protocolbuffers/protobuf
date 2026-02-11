# -*- coding: utf-8 -*-
# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Unittest for nested public imports."""

import unittest

from google.protobuf import unknown_fields
from google.protobuf.internal.import_test_package import outer_pb2

from google.protobuf import unittest_custom_options_pb2
from google.protobuf import unittest_import_option_pb2


class ImportTest(unittest.TestCase):

  def testPackageInitializationImport(self):
    """Test that we can import nested import public messages."""

    msg = outer_pb2.Outer()
    self.assertEqual(58, msg.import_public_nested.value)

  def testImportOptionKnown(self):
    file_descriptor = unittest_import_option_pb2.DESCRIPTOR
    message_descriptor = unittest_import_option_pb2.TestMessage.DESCRIPTOR
    field_descriptor = message_descriptor.fields_by_name['field1']

    self.assertEqual(
        file_descriptor.GetOptions().Extensions[
            unittest_custom_options_pb2.file_opt1
        ],
        1,
    )
    self.assertEqual(
        message_descriptor.GetOptions().Extensions[
            unittest_custom_options_pb2.message_opt1
        ],
        2,
    )
    self.assertEqual(
        field_descriptor.GetOptions().Extensions[
            unittest_custom_options_pb2.field_opt1
        ],
        3,
    )

  def testImportOptionUnknown(self):
    file_descriptor = unittest_import_option_pb2.DESCRIPTOR
    message_descriptor = unittest_import_option_pb2.TestMessage.DESCRIPTOR
    field_descriptor = message_descriptor.fields_by_name['field1']

    # Options from import option that are not linked in should be in unknown
    # fields.
    unknown_fields_file = unknown_fields.UnknownFieldSet(
        file_descriptor.GetOptions()
    )
    unknown_fields_message = unknown_fields.UnknownFieldSet(
        message_descriptor.GetOptions()
    )
    unknown_fields_field = unknown_fields.UnknownFieldSet(
        field_descriptor.GetOptions()
    )

    self.assertEqual(len(unknown_fields_file), 1)
    self.assertEqual(unknown_fields_file[0].field_number, 7736975)
    self.assertEqual(unknown_fields_file[0].data, 1)

    self.assertEqual(len(unknown_fields_message), 1)
    self.assertEqual(unknown_fields_message[0].field_number, 7739037)
    self.assertEqual(unknown_fields_message[0].data, 2)

    self.assertEqual(len(unknown_fields_field), 1)
    self.assertEqual(unknown_fields_field[0].field_number, 7740937)
    self.assertEqual(unknown_fields_field[0].data, 3)


if __name__ == '__main__':
  unittest.main()
