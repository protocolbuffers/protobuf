# -*- coding: utf-8 -*-
# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Unittest for reflection.py, which also indirectly tests the output of the
pure-Python protocol compiler.
"""

import copy
import gc
import operator
import struct
import sys
import warnings
import unittest

from google.protobuf import descriptor_pb2
from google.protobuf import descriptor
from google.protobuf import message
from google.protobuf import reflection
from google.protobuf import text_format
from google.protobuf.internal import api_implementation
from google.protobuf.internal import more_extensions_pb2
from google.protobuf.internal import more_messages_pb2
from google.protobuf.internal import message_set_extensions_pb2
from google.protobuf.internal import wire_format
from google.protobuf.internal import test_util
from google.protobuf.internal import testing_refleaks
from google.protobuf.internal import decoder
from google.protobuf.internal import _parameterized
from google.protobuf import unittest_import_pb2
from google.protobuf import unittest_mset_pb2
from google.protobuf import unittest_pb2
from google.protobuf import unittest_proto3_arena_pb2


warnings.simplefilter('error', DeprecationWarning)


class _MiniDecoder(object):
  """Decodes a stream of values from a string.

  Once upon a time we actually had a class called decoder.Decoder.  Then we
  got rid of it during a redesign that made decoding much, much faster overall.
  But a couple tests in this file used it to check that the serialized form of
  a message was correct.  So, this class implements just the methods that were
  used by said tests, so that we don't have to rewrite the tests.
  """

  def __init__(self, bytes):
    self._bytes = bytes
    self._pos = 0

  def ReadVarint(self):
    result, self._pos = decoder._DecodeVarint(self._bytes, self._pos)
    return result

  ReadInt32 = ReadVarint
  ReadInt64 = ReadVarint
  ReadUInt32 = ReadVarint
  ReadUInt64 = ReadVarint

  def ReadSInt64(self):
    return wire_format.ZigZagDecode(self.ReadVarint())

  ReadSInt32 = ReadSInt64

  def ReadFieldNumberAndWireType(self):
    return wire_format.UnpackTag(self.ReadVarint())

  def ReadFloat(self):
    result = struct.unpack('<f', self._bytes[self._pos:self._pos+4])[0]
    self._pos += 4
    return result

  def ReadDouble(self):
    result = struct.unpack('<d', self._bytes[self._pos:self._pos+8])[0]
    self._pos += 8
    return result

  def EndOfStream(self):
    return self._pos == len(self._bytes)


@_parameterized.named_parameters(
    ('_proto2', unittest_pb2),
    ('_proto3', unittest_proto3_arena_pb2))
@testing_refleaks.TestCase
class ReflectionTest(unittest.TestCase):

  def assertListsEqual(self, values, others):
    self.assertEqual(len(values), len(others))
    for i in range(len(values)):
      self.assertEqual(values[i], others[i])

  def testScalarConstructor(self, message_module):
    # Constructor with only scalar types should succeed.
    proto = message_module.TestAllTypes(
        optional_int32=24,
        optional_double=54.321,
        optional_string='optional_string',
        optional_float=None)

    self.assertEqual(24, proto.optional_int32)
    self.assertEqual(54.321, proto.optional_double)
    self.assertEqual('optional_string', proto.optional_string)
    if message_module is unittest_pb2:
      self.assertFalse(proto.HasField("optional_float"))

  def testRepeatedScalarConstructor(self, message_module):
    # Constructor with only repeated scalar types should succeed.
    proto = message_module.TestAllTypes(
        repeated_int32=[1, 2, 3, 4],
        repeated_double=[1.23, 54.321],
        repeated_bool=[True, False, False],
        repeated_string=["optional_string"],
        repeated_float=None)

    self.assertEqual([1, 2, 3, 4], list(proto.repeated_int32))
    self.assertEqual([1.23, 54.321], list(proto.repeated_double))
    self.assertEqual([True, False, False], list(proto.repeated_bool))
    self.assertEqual(["optional_string"], list(proto.repeated_string))
    self.assertEqual([], list(proto.repeated_float))

  def testMixedConstructor(self, message_module):
    # Constructor with only mixed types should succeed.
    proto = message_module.TestAllTypes(
        optional_int32=24,
        optional_string='optional_string',
        repeated_double=[1.23, 54.321],
        repeated_bool=[True, False, False],
        repeated_nested_message=[
            message_module.TestAllTypes.NestedMessage(
                bb=message_module.TestAllTypes.FOO),
            message_module.TestAllTypes.NestedMessage(
                bb=message_module.TestAllTypes.BAR)],
        repeated_foreign_message=[
            message_module.ForeignMessage(c=-43),
            message_module.ForeignMessage(c=45324),
            message_module.ForeignMessage(c=12)],
        optional_nested_message=None)

    self.assertEqual(24, proto.optional_int32)
    self.assertEqual('optional_string', proto.optional_string)
    self.assertEqual([1.23, 54.321], list(proto.repeated_double))
    self.assertEqual([True, False, False], list(proto.repeated_bool))
    self.assertEqual(
        [message_module.TestAllTypes.NestedMessage(
            bb=message_module.TestAllTypes.FOO),
         message_module.TestAllTypes.NestedMessage(
             bb=message_module.TestAllTypes.BAR)],
        list(proto.repeated_nested_message))
    self.assertEqual(
        [message_module.ForeignMessage(c=-43),
         message_module.ForeignMessage(c=45324),
         message_module.ForeignMessage(c=12)],
        list(proto.repeated_foreign_message))
    self.assertFalse(proto.HasField("optional_nested_message"))

  def testConstructorTypeError(self, message_module):
    self.assertRaises(
        TypeError, message_module.TestAllTypes, optional_int32='foo')
    self.assertRaises(
        TypeError, message_module.TestAllTypes, optional_string=1234)
    self.assertRaises(
        TypeError, message_module.TestAllTypes, optional_nested_message=1234)
    self.assertRaises(
        TypeError, message_module.TestAllTypes, repeated_int32=1234)
    self.assertRaises(
        TypeError, message_module.TestAllTypes, repeated_int32=['foo'])
    self.assertRaises(
        TypeError, message_module.TestAllTypes, repeated_string=1234)
    self.assertRaises(
        TypeError, message_module.TestAllTypes, repeated_string=[1234])
    self.assertRaises(
        TypeError, message_module.TestAllTypes, repeated_nested_message=1234)
    self.assertRaises(
        TypeError, message_module.TestAllTypes, repeated_nested_message=[1234])

  def testConstructorInvalidatesCachedByteSize(self, message_module):
    message = message_module.TestAllTypes(optional_int32=12)
    self.assertEqual(2, message.ByteSize())

    message = message_module.TestAllTypes(
        optional_nested_message=message_module.TestAllTypes.NestedMessage())
    self.assertEqual(3, message.ByteSize())

    message = message_module.TestAllTypes(repeated_int32=[12])
    # TODO: Add this test back for proto3
    if message_module is unittest_pb2:
      self.assertEqual(3, message.ByteSize())

    message = message_module.TestAllTypes(
        repeated_nested_message=[message_module.TestAllTypes.NestedMessage()])
    self.assertEqual(3, message.ByteSize())

  def testReferencesToNestedMessage(self, message_module):
    proto = message_module.TestAllTypes()
    nested = proto.optional_nested_message
    del proto
    # A previous version had a bug where this would raise an exception when
    # hitting a now-dead weak reference.
    nested.bb = 23

  def testOneOf(self, message_module):
    proto = message_module.TestAllTypes()
    proto.oneof_uint32 = 10
    proto.oneof_nested_message.bb = 11
    self.assertEqual(11, proto.oneof_nested_message.bb)
    self.assertFalse(proto.HasField('oneof_uint32'))
    nested = proto.oneof_nested_message
    proto.oneof_string = 'abc'
    self.assertEqual('abc', proto.oneof_string)
    self.assertEqual(11, nested.bb)
    self.assertFalse(proto.HasField('oneof_nested_message'))

  def testGetDefaultMessageAfterDisconnectingDefaultMessage(
      self, message_module):
    proto = message_module.TestAllTypes()
    nested = proto.optional_nested_message
    proto.ClearField('optional_nested_message')
    del proto
    del nested
    # Force a garbage collect so that the underlying CMessages are freed along
    # with the Messages they point to. This is to make sure we're not deleting
    # default message instances.
    gc.collect()
    proto = message_module.TestAllTypes()
    nested = proto.optional_nested_message

  def testDisconnectingNestedMessageAfterSettingField(self, message_module):
    proto = message_module.TestAllTypes()
    nested = proto.optional_nested_message
    nested.bb = 5
    self.assertTrue(proto.HasField('optional_nested_message'))
    proto.ClearField('optional_nested_message')  # Should disconnect from parent
    self.assertEqual(5, nested.bb)
    self.assertEqual(0, proto.optional_nested_message.bb)
    self.assertIsNot(nested, proto.optional_nested_message)
    nested.bb = 23
    self.assertFalse(proto.HasField('optional_nested_message'))
    self.assertEqual(0, proto.optional_nested_message.bb)

  def testDisconnectingNestedMessageBeforeGettingField(self, message_module):
    proto = message_module.TestAllTypes()
    self.assertFalse(proto.HasField('optional_nested_message'))
    proto.ClearField('optional_nested_message')
    self.assertFalse(proto.HasField('optional_nested_message'))

  def testDisconnectingNestedMessageAfterMerge(self, message_module):
    # This test exercises the code path that does not use ReleaseMessage().
    # The underlying fear is that if we use ReleaseMessage() incorrectly,
    # we will have memory leaks.  It's hard to check that that doesn't happen,
    # but at least we can exercise that code path to make sure it works.
    proto1 = message_module.TestAllTypes()
    proto2 = message_module.TestAllTypes()
    proto2.optional_nested_message.bb = 5
    proto1.MergeFrom(proto2)
    self.assertTrue(proto1.HasField('optional_nested_message'))
    proto1.ClearField('optional_nested_message')
    self.assertFalse(proto1.HasField('optional_nested_message'))

  def testDisconnectingLazyNestedMessage(self, message_module):
    # This test exercises releasing a nested message that is lazy. This test
    # only exercises real code in the C++ implementation as Python does not
    # support lazy parsing, but the current C++ implementation results in
    # memory corruption and a crash.
    if api_implementation.Type() != 'python':
      return
    proto = message_module.TestAllTypes()
    proto.optional_lazy_message.bb = 5
    proto.ClearField('optional_lazy_message')
    del proto
    gc.collect()

  def testSingularListFields(self, message_module):
    proto = message_module.TestAllTypes()
    proto.optional_fixed32 = 1
    proto.optional_int32 = 5
    proto.optional_string = 'foo'
    # Access sub-message but don't set it yet.
    nested_message = proto.optional_nested_message
    self.assertEqual(
      [ (proto.DESCRIPTOR.fields_by_name['optional_int32'  ], 5),
        (proto.DESCRIPTOR.fields_by_name['optional_fixed32'], 1),
        (proto.DESCRIPTOR.fields_by_name['optional_string' ], 'foo') ],
      proto.ListFields())

    proto.optional_nested_message.bb = 123
    self.assertEqual(
      [ (proto.DESCRIPTOR.fields_by_name['optional_int32'  ], 5),
        (proto.DESCRIPTOR.fields_by_name['optional_fixed32'], 1),
        (proto.DESCRIPTOR.fields_by_name['optional_string' ], 'foo'),
        (proto.DESCRIPTOR.fields_by_name['optional_nested_message' ],
             nested_message) ],
      proto.ListFields())

  def testRepeatedListFields(self, message_module):
    proto = message_module.TestAllTypes()
    proto.repeated_fixed32.append(1)
    proto.repeated_int32.append(5)
    proto.repeated_int32.append(11)
    proto.repeated_string.extend(['foo', 'bar'])
    proto.repeated_string.extend([])
    proto.repeated_string.append('baz')
    proto.repeated_string.extend(str(x) for x in range(2))
    proto.optional_int32 = 21
    proto.repeated_bool  # Access but don't set anything; should not be listed.
    self.assertEqual(
      [ (proto.DESCRIPTOR.fields_by_name['optional_int32'  ], 21),
        (proto.DESCRIPTOR.fields_by_name['repeated_int32'  ], [5, 11]),
        (proto.DESCRIPTOR.fields_by_name['repeated_fixed32'], [1]),
        (proto.DESCRIPTOR.fields_by_name['repeated_string' ],
          ['foo', 'bar', 'baz', '0', '1']) ],
      proto.ListFields())

  def testClearFieldWithUnknownFieldName(self, message_module):
    proto = message_module.TestAllTypes()
    self.assertRaises(ValueError, proto.ClearField, 'nonexistent_field')
    self.assertRaises(ValueError, proto.ClearField, b'nonexistent_field')

  def testDisallowedAssignments(self, message_module):
    # It's illegal to assign values directly to repeated fields
    # or to nonrepeated composite fields.  Ensure that this fails.
    proto = message_module.TestAllTypes()
    # Repeated fields.
    self.assertRaises(AttributeError, setattr, proto, 'repeated_int32', 10)
    # Lists shouldn't work, either.
    self.assertRaises(AttributeError, setattr, proto, 'repeated_int32', [10])
    # Composite fields.
    self.assertRaises(AttributeError, setattr, proto,
                      'optional_nested_message', 23)
    # Assignment to a repeated nested message field without specifying
    # the index in the array of nested messages.
    self.assertRaises(AttributeError, setattr, proto.repeated_nested_message,
                      'bb', 34)
    # Assignment to an attribute of a repeated field.
    self.assertRaises(AttributeError, setattr, proto.repeated_float,
                      'some_attribute', 34)
    # proto.nonexistent_field = 23 should fail as well.
    self.assertRaises(AttributeError, setattr, proto, 'nonexistent_field', 23)

  def testSingleScalarTypeSafety(self, message_module):
    proto = message_module.TestAllTypes()
    self.assertRaises(TypeError, setattr, proto, 'optional_int32', 1.1)
    self.assertRaises(TypeError, setattr, proto, 'optional_int32', 'foo')
    self.assertRaises(TypeError, setattr, proto, 'optional_string', 10)
    self.assertRaises(TypeError, setattr, proto, 'optional_bytes', 10)
    self.assertRaises(TypeError, setattr, proto, 'optional_bool', 'foo')
    self.assertRaises(TypeError, setattr, proto, 'optional_float', 'foo')
    self.assertRaises(TypeError, setattr, proto, 'optional_double', 'foo')
    # TODO: Fix type checking difference for python and c extension
    if (api_implementation.Type() == 'python' or
        (sys.version_info.major, sys.version_info.minor) >= (3, 10)):
      self.assertRaises(TypeError, setattr, proto, 'optional_bool', 1.1)
    else:
      proto.optional_bool = 1.1

  def assertIntegerTypes(self, integer_fn, message_module):
    """Verifies setting of scalar integers.

    Args:
      integer_fn: A function to wrap the integers that will be assigned.
      message_module: unittest_pb2 or unittest_proto3_arena_pb2
    """
    def TestGetAndDeserialize(field_name, value, expected_type):
      proto = message_module.TestAllTypes()
      value = integer_fn(value)
      setattr(proto, field_name, value)
      self.assertIsInstance(getattr(proto, field_name), expected_type)
      proto2 = message_module.TestAllTypes()
      proto2.ParseFromString(proto.SerializeToString())
      self.assertIsInstance(getattr(proto2, field_name), expected_type)

    TestGetAndDeserialize('optional_int32', 1, int)
    TestGetAndDeserialize('optional_int32', 1 << 30, int)
    TestGetAndDeserialize('optional_uint32', 1 << 30, int)
    integer_64 = int
    if struct.calcsize('L') == 4:
      # Python only has signed ints, so 32-bit python can't fit an uint32
      # in an int.
      TestGetAndDeserialize('optional_uint32', 1 << 31, integer_64)
    else:
      # 64-bit python can fit uint32 inside an int
      TestGetAndDeserialize('optional_uint32', 1 << 31, int)
    TestGetAndDeserialize('optional_int64', 1 << 30, integer_64)
    TestGetAndDeserialize('optional_int64', 1 << 60, integer_64)
    TestGetAndDeserialize('optional_uint64', 1 << 30, integer_64)
    TestGetAndDeserialize('optional_uint64', 1 << 60, integer_64)

  def testIntegerTypes(self, message_module):
    self.assertIntegerTypes(lambda x: x, message_module)

  def testNonStandardIntegerTypes(self, message_module):
    self.assertIntegerTypes(test_util.NonStandardInteger, message_module)

  def testIllegalValuesForIntegers(self, message_module):
    pb = message_module.TestAllTypes()

    # Strings are illegal, even when the represent an integer.
    with self.assertRaises(TypeError):
      pb.optional_uint64 = '2'

    # The exact error should propagate with a poorly written custom integer.
    with self.assertRaisesRegex(RuntimeError, 'my_error'):
      pb.optional_uint64 = test_util.NonStandardInteger(5, 'my_error')

  def assetIntegerBoundsChecking(self, integer_fn, message_module):
    """Verifies bounds checking for scalar integer fields.

    Args:
      integer_fn: A function to wrap the integers that will be assigned.
      message_module: unittest_pb2 or unittest_proto3_arena_pb2
    """
    def TestMinAndMaxIntegers(field_name, expected_min, expected_max):
      pb = message_module.TestAllTypes()
      expected_min = integer_fn(expected_min)
      expected_max = integer_fn(expected_max)
      setattr(pb, field_name, expected_min)
      self.assertEqual(expected_min, getattr(pb, field_name))
      setattr(pb, field_name, expected_max)
      self.assertEqual(expected_max, getattr(pb, field_name))
      self.assertRaises((ValueError, TypeError), setattr, pb, field_name,
                        expected_min - 1)
      self.assertRaises((ValueError, TypeError), setattr, pb, field_name,
                        expected_max + 1)

    TestMinAndMaxIntegers('optional_int32', -(1 << 31), (1 << 31) - 1)
    TestMinAndMaxIntegers('optional_uint32', 0, 0xffffffff)
    TestMinAndMaxIntegers('optional_int64', -(1 << 63), (1 << 63) - 1)
    TestMinAndMaxIntegers('optional_uint64', 0, 0xffffffffffffffff)
    # A bit of white-box testing since -1 is an int and not a long in C++ and
    # so goes down a different path.
    pb = message_module.TestAllTypes()
    with self.assertRaises((ValueError, TypeError)):
      pb.optional_uint64 = integer_fn(-(1 << 63))

    pb = message_module.TestAllTypes()
    pb.optional_nested_enum = integer_fn(1)
    self.assertEqual(1, pb.optional_nested_enum)

  def testSingleScalarBoundsChecking(self, message_module):
    self.assetIntegerBoundsChecking(lambda x: x, message_module)

  def testNonStandardSingleScalarBoundsChecking(self, message_module):
    self.assetIntegerBoundsChecking(
        test_util.NonStandardInteger, message_module)

  def testRepeatedScalarTypeSafety(self, message_module):
    proto = message_module.TestAllTypes()
    self.assertRaises(TypeError, proto.repeated_int32.append, 1.1)
    self.assertRaises(TypeError, proto.repeated_int32.append, 'foo')
    self.assertRaises(TypeError, proto.repeated_string, 10)
    self.assertRaises(TypeError, proto.repeated_bytes, 10)

    proto.repeated_int32.append(10)
    proto.repeated_int32[0] = 23
    self.assertRaises(IndexError, proto.repeated_int32.__setitem__, 500, 23)
    self.assertRaises(TypeError, proto.repeated_int32.__setitem__, 0, 'abc')
    self.assertRaises(TypeError, proto.repeated_int32.__setitem__, 0, [])
    self.assertRaises(TypeError, proto.repeated_int32.__setitem__,
                      'index', 23)

    proto.repeated_string.append('2')
    self.assertRaises(TypeError, proto.repeated_string.__setitem__, 0, 10)

    # Repeated enums tests.
    # proto.repeated_nested_enum.append(0)

  def testSingleScalarGettersAndSetters(self, message_module):
    proto = message_module.TestAllTypes()
    self.assertEqual(0, proto.optional_int32)
    proto.optional_int32 = 1
    self.assertEqual(1, proto.optional_int32)

    proto.optional_uint64 = 0xffffffffffff
    self.assertEqual(0xffffffffffff, proto.optional_uint64)
    proto.optional_uint64 = 0xffffffffffffffff
    self.assertEqual(0xffffffffffffffff, proto.optional_uint64)
    # TODO: Test all other scalar field types.

  def testEnums(self, message_module):
    proto = message_module.TestAllTypes()
    self.assertEqual(1, proto.FOO)
    self.assertEqual(1, message_module.TestAllTypes.FOO)
    self.assertEqual(2, proto.BAR)
    self.assertEqual(2, message_module.TestAllTypes.BAR)
    self.assertEqual(3, proto.BAZ)
    self.assertEqual(3, message_module.TestAllTypes.BAZ)

  def testEnum_Name(self, message_module):
    self.assertEqual(
        'FOREIGN_FOO',
        message_module.ForeignEnum.Name(message_module.FOREIGN_FOO))
    self.assertEqual(
        'FOREIGN_BAR',
        message_module.ForeignEnum.Name(message_module.FOREIGN_BAR))
    self.assertEqual(
        'FOREIGN_BAZ',
        message_module.ForeignEnum.Name(message_module.FOREIGN_BAZ))
    self.assertRaises(ValueError,
                      message_module.ForeignEnum.Name, 11312)

    proto = message_module.TestAllTypes()
    self.assertEqual('FOO',
                     proto.NestedEnum.Name(proto.FOO))
    self.assertEqual('FOO',
                     message_module.TestAllTypes.NestedEnum.Name(proto.FOO))
    self.assertEqual('BAR',
                     proto.NestedEnum.Name(proto.BAR))
    self.assertEqual('BAR',
                     message_module.TestAllTypes.NestedEnum.Name(proto.BAR))
    self.assertEqual('BAZ',
                     proto.NestedEnum.Name(proto.BAZ))
    self.assertEqual('BAZ',
                     message_module.TestAllTypes.NestedEnum.Name(proto.BAZ))
    self.assertRaises(ValueError,
                      proto.NestedEnum.Name, 11312)
    self.assertRaises(ValueError,
                      message_module.TestAllTypes.NestedEnum.Name, 11312)

    # Check some coercion cases.
    self.assertRaises(TypeError, message_module.TestAllTypes.NestedEnum.Name,
                      11312.0)
    self.assertRaises(TypeError, message_module.TestAllTypes.NestedEnum.Name,
                      None)
    self.assertEqual('FOO', message_module.TestAllTypes.NestedEnum.Name(True))

  def testEnum_Value(self, message_module):
    self.assertEqual(message_module.FOREIGN_FOO,
                     message_module.ForeignEnum.Value('FOREIGN_FOO'))
    self.assertEqual(message_module.FOREIGN_FOO,
                     message_module.ForeignEnum.FOREIGN_FOO)

    self.assertEqual(message_module.FOREIGN_BAR,
                     message_module.ForeignEnum.Value('FOREIGN_BAR'))
    self.assertEqual(message_module.FOREIGN_BAR,
                     message_module.ForeignEnum.FOREIGN_BAR)

    self.assertEqual(message_module.FOREIGN_BAZ,
                     message_module.ForeignEnum.Value('FOREIGN_BAZ'))
    self.assertEqual(message_module.FOREIGN_BAZ,
                     message_module.ForeignEnum.FOREIGN_BAZ)

    self.assertRaises(ValueError,
                      message_module.ForeignEnum.Value, 'FO')
    with self.assertRaises(AttributeError):
      message_module.ForeignEnum.FO

    proto = message_module.TestAllTypes()
    self.assertEqual(proto.FOO,
                     proto.NestedEnum.Value('FOO'))
    self.assertEqual(proto.FOO,
                     proto.NestedEnum.FOO)

    self.assertEqual(proto.FOO,
                     message_module.TestAllTypes.NestedEnum.Value('FOO'))
    self.assertEqual(proto.FOO,
                     message_module.TestAllTypes.NestedEnum.FOO)

    self.assertEqual(proto.BAR,
                     proto.NestedEnum.Value('BAR'))
    self.assertEqual(proto.BAR,
                     proto.NestedEnum.BAR)

    self.assertEqual(proto.BAR,
                     message_module.TestAllTypes.NestedEnum.Value('BAR'))
    self.assertEqual(proto.BAR,
                     message_module.TestAllTypes.NestedEnum.BAR)

    self.assertEqual(proto.BAZ,
                     proto.NestedEnum.Value('BAZ'))
    self.assertEqual(proto.BAZ,
                     proto.NestedEnum.BAZ)

    self.assertEqual(proto.BAZ,
                     message_module.TestAllTypes.NestedEnum.Value('BAZ'))
    self.assertEqual(proto.BAZ,
                     message_module.TestAllTypes.NestedEnum.BAZ)

    self.assertRaises(ValueError,
                      proto.NestedEnum.Value, 'Foo')
    with self.assertRaises(AttributeError):
      proto.NestedEnum.Value.Foo

    self.assertRaises(ValueError,
                      message_module.TestAllTypes.NestedEnum.Value, 'Foo')
    with self.assertRaises(AttributeError):
      message_module.TestAllTypes.NestedEnum.Value.Foo

  def testEnum_KeysAndValues(self, message_module):
    if message_module == unittest_pb2:
      keys = ['FOREIGN_FOO', 'FOREIGN_BAR', 'FOREIGN_BAZ', 'FOREIGN_BAX']
      values = [4, 5, 6, 32]
      items = [
          ('FOREIGN_FOO', 4),
          ('FOREIGN_BAR', 5),
          ('FOREIGN_BAZ', 6),
          ('FOREIGN_BAX', 32),
      ]
    else:
      keys = ['FOREIGN_ZERO', 'FOREIGN_FOO', 'FOREIGN_BAR', 'FOREIGN_BAZ']
      values = [0, 4, 5, 6]
      items = [('FOREIGN_ZERO', 0), ('FOREIGN_FOO', 4),
               ('FOREIGN_BAR', 5), ('FOREIGN_BAZ', 6)]
    self.assertEqual(keys,
                     list(message_module.ForeignEnum.keys()))
    self.assertEqual(values,
                     list(message_module.ForeignEnum.values()))
    self.assertEqual(items,
                     list(message_module.ForeignEnum.items()))

    proto = message_module.TestAllTypes()
    if message_module == unittest_pb2:
      keys = ['FOO', 'BAR', 'BAZ', 'NEG']
      values = [1, 2, 3, -1]
      items = [('FOO', 1), ('BAR', 2), ('BAZ', 3), ('NEG', -1)]
    else:
      keys = ['ZERO', 'FOO', 'BAR', 'BAZ', 'NEG']
      values = [0, 1, 2, 3, -1]
      items = [('ZERO', 0), ('FOO', 1), ('BAR', 2), ('BAZ', 3), ('NEG', -1)]
    self.assertEqual(keys, list(proto.NestedEnum.keys()))
    self.assertEqual(values, list(proto.NestedEnum.values()))
    self.assertEqual(items,
                     list(proto.NestedEnum.items()))

  def testStaticParseFrom(self, message_module):
    proto1 = message_module.TestAllTypes()
    test_util.SetAllFields(proto1)

    string1 = proto1.SerializeToString()
    proto2 = message_module.TestAllTypes.FromString(string1)

    # Messages should be equal.
    self.assertEqual(proto2, proto1)

  def testMergeFromSingularField(self, message_module):
    # Test merge with just a singular field.
    proto1 = message_module.TestAllTypes()
    proto1.optional_int32 = 1

    proto2 = message_module.TestAllTypes()
    # This shouldn't get overwritten.
    proto2.optional_string = 'value'

    proto2.MergeFrom(proto1)
    self.assertEqual(1, proto2.optional_int32)
    self.assertEqual('value', proto2.optional_string)

  def testMergeFromRepeatedField(self, message_module):
    # Test merge with just a repeated field.
    proto1 = message_module.TestAllTypes()
    proto1.repeated_int32.append(1)
    proto1.repeated_int32.append(2)

    proto2 = message_module.TestAllTypes()
    proto2.repeated_int32.append(0)
    proto2.MergeFrom(proto1)

    self.assertEqual(0, proto2.repeated_int32[0])
    self.assertEqual(1, proto2.repeated_int32[1])
    self.assertEqual(2, proto2.repeated_int32[2])

  def testMergeFromRepeatedNestedMessage(self, message_module):
    # Test merge with a repeated nested message.
    proto1 = message_module.TestAllTypes()
    m = proto1.repeated_nested_message.add()
    m.bb = 123
    m = proto1.repeated_nested_message.add()
    m.bb = 321

    proto2 = message_module.TestAllTypes()
    m = proto2.repeated_nested_message.add()
    m.bb = 999
    proto2.MergeFrom(proto1)
    self.assertEqual(999, proto2.repeated_nested_message[0].bb)
    self.assertEqual(123, proto2.repeated_nested_message[1].bb)
    self.assertEqual(321, proto2.repeated_nested_message[2].bb)

    proto3 = message_module.TestAllTypes()
    proto3.repeated_nested_message.MergeFrom(proto2.repeated_nested_message)
    self.assertEqual(999, proto3.repeated_nested_message[0].bb)
    self.assertEqual(123, proto3.repeated_nested_message[1].bb)
    self.assertEqual(321, proto3.repeated_nested_message[2].bb)

  def testMergeFromAllFields(self, message_module):
    # With all fields set.
    proto1 = message_module.TestAllTypes()
    test_util.SetAllFields(proto1)
    proto2 = message_module.TestAllTypes()
    proto2.MergeFrom(proto1)

    # Messages should be equal.
    self.assertEqual(proto2, proto1)

    # Serialized string should be equal too.
    string1 = proto1.SerializeToString()
    string2 = proto2.SerializeToString()
    self.assertEqual(string1, string2)

  def testMergeFromBug(self, message_module):
    message1 = message_module.TestAllTypes()
    message2 = message_module.TestAllTypes()

    # Cause optional_nested_message to be instantiated within message1, even
    # though it is not considered to be "present".
    message1.optional_nested_message
    self.assertFalse(message1.HasField('optional_nested_message'))

    # Merge into message2.  This should not instantiate the field is message2.
    message2.MergeFrom(message1)
    self.assertFalse(message2.HasField('optional_nested_message'))

  def testCopyFromSingularField(self, message_module):
    # Test copy with just a singular field.
    proto1 = message_module.TestAllTypes()
    proto1.optional_int32 = 1
    proto1.optional_string = 'important-text'

    proto2 = message_module.TestAllTypes()
    proto2.optional_string = 'value'

    proto2.CopyFrom(proto1)
    self.assertEqual(1, proto2.optional_int32)
    self.assertEqual('important-text', proto2.optional_string)

  def testCopyFromRepeatedField(self, message_module):
    # Test copy with a repeated field.
    proto1 = message_module.TestAllTypes()
    proto1.repeated_int32.append(1)
    proto1.repeated_int32.append(2)

    proto2 = message_module.TestAllTypes()
    proto2.repeated_int32.append(0)
    proto2.CopyFrom(proto1)

    self.assertEqual(1, proto2.repeated_int32[0])
    self.assertEqual(2, proto2.repeated_int32[1])

  def testCopyFromAllFields(self, message_module):
    # With all fields set.
    proto1 = message_module.TestAllTypes()
    test_util.SetAllFields(proto1)
    proto2 = message_module.TestAllTypes()
    proto2.CopyFrom(proto1)

    # Messages should be equal.
    self.assertEqual(proto2, proto1)

    # Serialized string should be equal too.
    string1 = proto1.SerializeToString()
    string2 = proto2.SerializeToString()
    self.assertEqual(string1, string2)

  def testCopyFromSelf(self, message_module):
    proto1 = message_module.TestAllTypes()
    proto1.repeated_int32.append(1)
    proto1.optional_int32 = 2
    proto1.optional_string = 'important-text'

    proto1.CopyFrom(proto1)
    self.assertEqual(1, proto1.repeated_int32[0])
    self.assertEqual(2, proto1.optional_int32)
    self.assertEqual('important-text', proto1.optional_string)

  def testDeepCopy(self, message_module):
    proto1 = message_module.TestAllTypes()
    proto1.optional_int32 = 1
    proto2 = copy.deepcopy(proto1)
    self.assertEqual(1, proto2.optional_int32)

    proto1.repeated_int32.append(2)
    proto1.repeated_int32.append(3)
    container = copy.deepcopy(proto1.repeated_int32)
    self.assertEqual([2, 3], container)
    container.remove(container[0])
    self.assertEqual([3], container)

    message1 = proto1.repeated_nested_message.add()
    message1.bb = 1
    messages = copy.deepcopy(proto1.repeated_nested_message)
    self.assertEqual(proto1.repeated_nested_message, messages)
    message1.bb = 2
    self.assertNotEqual(proto1.repeated_nested_message, messages)
    messages.remove(messages[0])
    self.assertEqual(len(messages), 0)

  def testEmptyDeepCopy(self, message_module):
    proto1 = message_module.TestAllTypes()
    nested2 = copy.deepcopy(proto1.optional_nested_message)
    self.assertEqual(0, nested2.bb)

    # TODO: Implement deepcopy for extension dict

  def testDisconnectingBeforeClear(self, message_module):
    proto = message_module.TestAllTypes()
    nested = proto.optional_nested_message
    proto.Clear()
    self.assertIsNot(nested, proto.optional_nested_message)
    nested.bb = 23
    self.assertFalse(proto.HasField('optional_nested_message'))
    self.assertEqual(0, proto.optional_nested_message.bb)

    proto = message_module.TestAllTypes()
    nested = proto.optional_nested_message
    nested.bb = 5
    foreign = proto.optional_foreign_message
    foreign.c = 6
    proto.Clear()
    self.assertIsNot(nested, proto.optional_nested_message)
    self.assertIsNot(foreign, proto.optional_foreign_message)
    self.assertEqual(5, nested.bb)
    self.assertEqual(6, foreign.c)
    nested.bb = 15
    foreign.c = 16
    self.assertFalse(proto.HasField('optional_nested_message'))
    self.assertEqual(0, proto.optional_nested_message.bb)
    self.assertFalse(proto.HasField('optional_foreign_message'))
    self.assertEqual(0, proto.optional_foreign_message.c)

  def testStringUTF8Encoding(self, message_module):
    proto = message_module.TestAllTypes()

    # Assignment of a unicode object to a field of type 'bytes' is not allowed.
    self.assertRaises(TypeError,
                      setattr, proto, 'optional_bytes', u'unicode object')

    # Check that the default value is of python's 'unicode' type.
    self.assertEqual(type(proto.optional_string), str)

    proto.optional_string = str('Testing')
    self.assertEqual(proto.optional_string, str('Testing'))

    # Assign a value of type 'str' which can be encoded in UTF-8.
    proto.optional_string = str('Testing')
    self.assertEqual(proto.optional_string, str('Testing'))

    # Try to assign a 'bytes' object which contains non-UTF-8.
    self.assertRaises(ValueError,
                      setattr, proto, 'optional_string', b'a\x80a')
    # No exception: Assign already encoded UTF-8 bytes to a string field.
    utf8_bytes = u'Тест'.encode('utf-8')
    proto.optional_string = utf8_bytes
    # No exception: Assign the a non-ascii unicode object.
    proto.optional_string = u'Тест'
    # No exception thrown (normal str assignment containing ASCII).
    proto.optional_string = 'abc'

  def testBytesInTextFormat(self, message_module):
    proto = message_module.TestAllTypes(optional_bytes=b'\x00\x7f\x80\xff')
    self.assertEqual(u'optional_bytes: "\\000\\177\\200\\377"\n', str(proto))

  def testEmptyNestedMessage(self, message_module):
    proto = message_module.TestAllTypes()
    proto.optional_nested_message.MergeFrom(
        message_module.TestAllTypes.NestedMessage())
    self.assertTrue(proto.HasField('optional_nested_message'))

    proto = message_module.TestAllTypes()
    proto.optional_nested_message.CopyFrom(
        message_module.TestAllTypes.NestedMessage())
    self.assertTrue(proto.HasField('optional_nested_message'))

    proto = message_module.TestAllTypes()
    bytes_read = proto.optional_nested_message.MergeFromString(b'')
    self.assertEqual(0, bytes_read)
    self.assertTrue(proto.HasField('optional_nested_message'))

    proto = message_module.TestAllTypes()
    proto.optional_nested_message.ParseFromString(b'')
    self.assertTrue(proto.HasField('optional_nested_message'))

    serialized = proto.SerializeToString()
    proto2 = message_module.TestAllTypes()
    self.assertEqual(
        len(serialized),
        proto2.MergeFromString(serialized))
    self.assertTrue(proto2.HasField('optional_nested_message'))


# Class to test proto2-only features (required, extensions, etc.)
@testing_refleaks.TestCase
class Proto2ReflectionTest(unittest.TestCase):

  def testRepeatedCompositeConstructor(self):
    # Constructor with only repeated composite types should succeed.
    proto = unittest_pb2.TestAllTypes(
        repeated_nested_message=[
            unittest_pb2.TestAllTypes.NestedMessage(
                bb=unittest_pb2.TestAllTypes.FOO),
            unittest_pb2.TestAllTypes.NestedMessage(
                bb=unittest_pb2.TestAllTypes.BAR)],
        repeated_foreign_message=[
            unittest_pb2.ForeignMessage(c=-43),
            unittest_pb2.ForeignMessage(c=45324),
            unittest_pb2.ForeignMessage(c=12)],
        repeatedgroup=[
            unittest_pb2.TestAllTypes.RepeatedGroup(),
            unittest_pb2.TestAllTypes.RepeatedGroup(a=1),
            unittest_pb2.TestAllTypes.RepeatedGroup(a=2)])

    self.assertEqual(
        [unittest_pb2.TestAllTypes.NestedMessage(
            bb=unittest_pb2.TestAllTypes.FOO),
         unittest_pb2.TestAllTypes.NestedMessage(
             bb=unittest_pb2.TestAllTypes.BAR)],
        list(proto.repeated_nested_message))
    self.assertEqual(
        [unittest_pb2.ForeignMessage(c=-43),
         unittest_pb2.ForeignMessage(c=45324),
         unittest_pb2.ForeignMessage(c=12)],
        list(proto.repeated_foreign_message))
    self.assertEqual(
        [unittest_pb2.TestAllTypes.RepeatedGroup(),
         unittest_pb2.TestAllTypes.RepeatedGroup(a=1),
         unittest_pb2.TestAllTypes.RepeatedGroup(a=2)],
        list(proto.repeatedgroup))

  def assertListsEqual(self, values, others):
    self.assertEqual(len(values), len(others))
    for i in range(len(values)):
      self.assertEqual(values[i], others[i])

  def testSimpleHasBits(self):
    # Test a scalar.
    proto = unittest_pb2.TestAllTypes()
    self.assertFalse(proto.HasField('optional_int32'))
    self.assertEqual(0, proto.optional_int32)
    # HasField() shouldn't be true if all we've done is
    # read the default value.
    self.assertFalse(proto.HasField('optional_int32'))
    proto.optional_int32 = 1
    # Setting a value however *should* set the "has" bit.
    self.assertTrue(proto.HasField('optional_int32'))
    proto.ClearField('optional_int32')
    # And clearing that value should unset the "has" bit.
    self.assertFalse(proto.HasField('optional_int32'))

  def testHasBitsWithSinglyNestedScalar(self):
    # Helper used to test foreign messages and groups.
    #
    # composite_field_name should be the name of a non-repeated
    # composite (i.e., foreign or group) field in TestAllTypes,
    # and scalar_field_name should be the name of an integer-valued
    # scalar field within that composite.
    #
    # I never thought I'd miss C++ macros and templates so much. :(
    # This helper is semantically just:
    #
    #   assert proto.composite_field.scalar_field == 0
    #   assert not proto.composite_field.HasField('scalar_field')
    #   assert not proto.HasField('composite_field')
    #
    #   proto.composite_field.scalar_field = 10
    #   old_composite_field = proto.composite_field
    #
    #   assert proto.composite_field.scalar_field == 10
    #   assert proto.composite_field.HasField('scalar_field')
    #   assert proto.HasField('composite_field')
    #
    #   proto.ClearField('composite_field')
    #
    #   assert not proto.composite_field.HasField('scalar_field')
    #   assert not proto.HasField('composite_field')
    #   assert proto.composite_field.scalar_field == 0
    #
    #   # Now ensure that ClearField('composite_field') disconnected
    #   # the old field object from the object tree...
    #   assert old_composite_field is not proto.composite_field
    #   old_composite_field.scalar_field = 20
    #   assert not proto.composite_field.HasField('scalar_field')
    #   assert not proto.HasField('composite_field')
    def TestCompositeHasBits(composite_field_name, scalar_field_name):
      proto = unittest_pb2.TestAllTypes()
      # First, check that we can get the scalar value, and see that it's the
      # default (0), but that proto.HasField('omposite') and
      # proto.composite.HasField('scalar') will still return False.
      composite_field = getattr(proto, composite_field_name)
      original_scalar_value = getattr(composite_field, scalar_field_name)
      self.assertEqual(0, original_scalar_value)
      # Assert that the composite object does not "have" the scalar.
      self.assertFalse(composite_field.HasField(scalar_field_name))
      # Assert that proto does not "have" the composite field.
      self.assertFalse(proto.HasField(composite_field_name))

      # Now set the scalar within the composite field.  Ensure that the setting
      # is reflected, and that proto.HasField('composite') and
      # proto.composite.HasField('scalar') now both return True.
      new_val = 20
      setattr(composite_field, scalar_field_name, new_val)
      self.assertEqual(new_val, getattr(composite_field, scalar_field_name))
      # Hold on to a reference to the current composite_field object.
      old_composite_field = composite_field
      # Assert that the has methods now return true.
      self.assertTrue(composite_field.HasField(scalar_field_name))
      self.assertTrue(proto.HasField(composite_field_name))

      # Now call the clear method...
      proto.ClearField(composite_field_name)

      # ...and ensure that the "has" bits are all back to False...
      composite_field = getattr(proto, composite_field_name)
      self.assertFalse(composite_field.HasField(scalar_field_name))
      self.assertFalse(proto.HasField(composite_field_name))
      # ...and ensure that the scalar field has returned to its default.
      self.assertEqual(0, getattr(composite_field, scalar_field_name))

      self.assertIsNot(old_composite_field, composite_field)
      setattr(old_composite_field, scalar_field_name, new_val)
      self.assertFalse(composite_field.HasField(scalar_field_name))
      self.assertFalse(proto.HasField(composite_field_name))
      self.assertEqual(0, getattr(composite_field, scalar_field_name))

    # Test simple, single-level nesting when we set a scalar.
    TestCompositeHasBits('optionalgroup', 'a')
    TestCompositeHasBits('optional_nested_message', 'bb')
    TestCompositeHasBits('optional_foreign_message', 'c')
    TestCompositeHasBits('optional_import_message', 'd')

  def testHasBitsWhenModifyingRepeatedFields(self):
    # Test nesting when we add an element to a repeated field in a submessage.
    proto = unittest_pb2.TestNestedMessageHasBits()
    proto.optional_nested_message.nestedmessage_repeated_int32.append(5)
    self.assertEqual(
        [5], proto.optional_nested_message.nestedmessage_repeated_int32)
    self.assertTrue(proto.HasField('optional_nested_message'))

    # Do the same test, but with a repeated composite field within the
    # submessage.
    proto.ClearField('optional_nested_message')
    self.assertFalse(proto.HasField('optional_nested_message'))
    proto.optional_nested_message.nestedmessage_repeated_foreignmessage.add()
    self.assertTrue(proto.HasField('optional_nested_message'))

  def testHasBitsForManyLevelsOfNesting(self):
    # Test nesting many levels deep.
    recursive_proto = unittest_pb2.TestMutualRecursionA()
    self.assertFalse(recursive_proto.HasField('bb'))
    self.assertEqual(0, recursive_proto.bb.a.bb.a.bb.optional_int32)
    self.assertFalse(recursive_proto.HasField('bb'))
    recursive_proto.bb.a.bb.a.bb.optional_int32 = 5
    self.assertEqual(5, recursive_proto.bb.a.bb.a.bb.optional_int32)
    self.assertTrue(recursive_proto.HasField('bb'))
    self.assertTrue(recursive_proto.bb.HasField('a'))
    self.assertTrue(recursive_proto.bb.a.HasField('bb'))
    self.assertTrue(recursive_proto.bb.a.bb.HasField('a'))
    self.assertTrue(recursive_proto.bb.a.bb.a.HasField('bb'))
    self.assertFalse(recursive_proto.bb.a.bb.a.bb.HasField('a'))
    self.assertTrue(recursive_proto.bb.a.bb.a.bb.HasField('optional_int32'))

  def testSingularListExtensions(self):
    proto = unittest_pb2.TestAllExtensions()
    proto.Extensions[unittest_pb2.optional_fixed32_extension] = 1
    proto.Extensions[unittest_pb2.optional_int32_extension  ] = 5
    proto.Extensions[unittest_pb2.optional_string_extension ] = 'foo'
    self.assertEqual(
      [ (unittest_pb2.optional_int32_extension  , 5),
        (unittest_pb2.optional_fixed32_extension, 1),
        (unittest_pb2.optional_string_extension , 'foo') ],
      proto.ListFields())
    del proto.Extensions[unittest_pb2.optional_fixed32_extension]
    self.assertEqual(
        [(unittest_pb2.optional_int32_extension, 5),
         (unittest_pb2.optional_string_extension, 'foo')],
        proto.ListFields())

  def testRepeatedListExtensions(self):
    proto = unittest_pb2.TestAllExtensions()
    proto.Extensions[unittest_pb2.repeated_fixed32_extension].append(1)
    proto.Extensions[unittest_pb2.repeated_int32_extension  ].append(5)
    proto.Extensions[unittest_pb2.repeated_int32_extension  ].append(11)
    proto.Extensions[unittest_pb2.repeated_string_extension ].append('foo')
    proto.Extensions[unittest_pb2.repeated_string_extension ].append('bar')
    proto.Extensions[unittest_pb2.repeated_string_extension ].append('baz')
    proto.Extensions[unittest_pb2.optional_int32_extension  ] = 21
    self.assertEqual(
      [ (unittest_pb2.optional_int32_extension  , 21),
        (unittest_pb2.repeated_int32_extension  , [5, 11]),
        (unittest_pb2.repeated_fixed32_extension, [1]),
        (unittest_pb2.repeated_string_extension , ['foo', 'bar', 'baz']) ],
      proto.ListFields())
    del proto.Extensions[unittest_pb2.repeated_int32_extension]
    del proto.Extensions[unittest_pb2.repeated_string_extension]
    self.assertEqual(
        [(unittest_pb2.optional_int32_extension, 21),
         (unittest_pb2.repeated_fixed32_extension, [1])],
        proto.ListFields())

  def testListFieldsAndExtensions(self):
    proto = unittest_pb2.TestFieldOrderings()
    test_util.SetAllFieldsAndExtensions(proto)
    unittest_pb2.my_extension_int
    self.assertEqual(
      [ (proto.DESCRIPTOR.fields_by_name['my_int'   ], 1),
        (unittest_pb2.my_extension_int               , 23),
        (proto.DESCRIPTOR.fields_by_name['my_string'], 'foo'),
        (unittest_pb2.my_extension_string            , 'bar'),
        (proto.DESCRIPTOR.fields_by_name['my_float' ], 1.0) ],
      proto.ListFields())

  def testDefaultValues(self):
    proto = unittest_pb2.TestAllTypes()
    self.assertEqual(0, proto.optional_int32)
    self.assertEqual(0, proto.optional_int64)
    self.assertEqual(0, proto.optional_uint32)
    self.assertEqual(0, proto.optional_uint64)
    self.assertEqual(0, proto.optional_sint32)
    self.assertEqual(0, proto.optional_sint64)
    self.assertEqual(0, proto.optional_fixed32)
    self.assertEqual(0, proto.optional_fixed64)
    self.assertEqual(0, proto.optional_sfixed32)
    self.assertEqual(0, proto.optional_sfixed64)
    self.assertEqual(0.0, proto.optional_float)
    self.assertEqual(0.0, proto.optional_double)
    self.assertEqual(False, proto.optional_bool)
    self.assertEqual('', proto.optional_string)
    self.assertEqual(b'', proto.optional_bytes)

    self.assertEqual(41, proto.default_int32)
    self.assertEqual(42, proto.default_int64)
    self.assertEqual(43, proto.default_uint32)
    self.assertEqual(44, proto.default_uint64)
    self.assertEqual(-45, proto.default_sint32)
    self.assertEqual(46, proto.default_sint64)
    self.assertEqual(47, proto.default_fixed32)
    self.assertEqual(48, proto.default_fixed64)
    self.assertEqual(49, proto.default_sfixed32)
    self.assertEqual(-50, proto.default_sfixed64)
    self.assertEqual(51.5, proto.default_float)
    self.assertEqual(52e3, proto.default_double)
    self.assertEqual(True, proto.default_bool)
    self.assertEqual('hello', proto.default_string)
    self.assertEqual(b'world', proto.default_bytes)
    self.assertEqual(unittest_pb2.TestAllTypes.BAR, proto.default_nested_enum)
    self.assertEqual(unittest_pb2.FOREIGN_BAR, proto.default_foreign_enum)
    self.assertEqual(unittest_import_pb2.IMPORT_BAR,
                     proto.default_import_enum)

    proto = unittest_pb2.TestExtremeDefaultValues()
    self.assertEqual(u'\u1234', proto.utf8_string)

  def testHasFieldWithUnknownFieldName(self):
    proto = unittest_pb2.TestAllTypes()
    self.assertRaises(ValueError, proto.HasField, 'nonexistent_field')

  def testClearRemovesChildren(self):
    # Make sure there aren't any implementation bugs that are only partially
    # clearing the message (which can happen in the more complex C++
    # implementation which has parallel message lists).
    proto = unittest_pb2.TestRequiredForeign()
    for i in range(10):
      proto.repeated_message.add()
    proto2 = unittest_pb2.TestRequiredForeign()
    proto.CopyFrom(proto2)
    self.assertRaises(IndexError, lambda: proto.repeated_message[5])

  def testSingleScalarClearField(self):
    proto = unittest_pb2.TestAllTypes()
    # Should be allowed to clear something that's not there (a no-op).
    proto.ClearField('optional_int32')
    proto.optional_int32 = 1
    self.assertTrue(proto.HasField('optional_int32'))
    proto.ClearField('optional_int32')
    self.assertEqual(0, proto.optional_int32)
    self.assertFalse(proto.HasField('optional_int32'))
    # TODO: Test all other scalar field types.

  def testRepeatedScalars(self):
    proto = unittest_pb2.TestAllTypes()

    self.assertFalse(proto.repeated_int32)
    self.assertEqual(0, len(proto.repeated_int32))
    proto.repeated_int32.append(5)
    proto.repeated_int32.append(10)
    proto.repeated_int32.append(15)
    self.assertTrue(proto.repeated_int32)
    self.assertEqual(3, len(proto.repeated_int32))

    self.assertEqual([5, 10, 15], proto.repeated_int32)

    # Test single retrieval.
    self.assertEqual(5, proto.repeated_int32[0])
    self.assertEqual(15, proto.repeated_int32[-1])
    # Test out-of-bounds indices.
    self.assertRaises(IndexError, proto.repeated_int32.__getitem__, 1234)
    self.assertRaises(IndexError, proto.repeated_int32.__getitem__, -1234)
    # Test incorrect types passed to __getitem__.
    self.assertRaises(TypeError, proto.repeated_int32.__getitem__, 'foo')
    self.assertRaises(TypeError, proto.repeated_int32.__getitem__, None)

    # Test single assignment.
    proto.repeated_int32[1] = 20
    self.assertEqual([5, 20, 15], proto.repeated_int32)

    # Test insertion.
    proto.repeated_int32.insert(1, 25)
    self.assertEqual([5, 25, 20, 15], proto.repeated_int32)

    # Test slice retrieval.
    proto.repeated_int32.append(30)
    self.assertEqual([25, 20, 15], proto.repeated_int32[1:4])
    self.assertEqual([5, 25, 20, 15, 30], proto.repeated_int32[:])

    # Test slice assignment with an iterator
    proto.repeated_int32[1:4] = (i for i in range(3))
    self.assertEqual([5, 0, 1, 2, 30], proto.repeated_int32)

    # Test slice assignment.
    proto.repeated_int32[1:4] = [35, 40, 45]
    self.assertEqual([5, 35, 40, 45, 30], proto.repeated_int32)

    # Test that we can use the field as an iterator.
    result = []
    for i in proto.repeated_int32:
      result.append(i)
    self.assertEqual([5, 35, 40, 45, 30], result)

    # Test single deletion.
    del proto.repeated_int32[2]
    self.assertEqual([5, 35, 45, 30], proto.repeated_int32)

    # Test slice deletion.
    del proto.repeated_int32[2:]
    self.assertEqual([5, 35], proto.repeated_int32)

    # Test extending.
    proto.repeated_int32.extend([3, 13])
    self.assertEqual([5, 35, 3, 13], proto.repeated_int32)

    # Test clearing.
    proto.ClearField('repeated_int32')
    self.assertFalse(proto.repeated_int32)
    self.assertEqual(0, len(proto.repeated_int32))

    proto.repeated_int32.append(1)
    self.assertEqual(1, proto.repeated_int32[-1])
    # Test assignment to a negative index.
    proto.repeated_int32[-1] = 2
    self.assertEqual(2, proto.repeated_int32[-1])

    # Test deletion at negative indices.
    proto.repeated_int32[:] = [0, 1, 2, 3]
    del proto.repeated_int32[-1]
    self.assertEqual([0, 1, 2], proto.repeated_int32)

    del proto.repeated_int32[-2]
    self.assertEqual([0, 2], proto.repeated_int32)

    self.assertRaises(IndexError, proto.repeated_int32.__delitem__, -3)
    self.assertRaises(IndexError, proto.repeated_int32.__delitem__, 300)

    del proto.repeated_int32[-2:-1]
    self.assertEqual([2], proto.repeated_int32)

    del proto.repeated_int32[100:10000]
    self.assertEqual([2], proto.repeated_int32)

  def testRepeatedScalarsRemove(self):
    proto = unittest_pb2.TestAllTypes()

    self.assertFalse(proto.repeated_int32)
    self.assertEqual(0, len(proto.repeated_int32))
    proto.repeated_int32.append(5)
    proto.repeated_int32.append(10)
    proto.repeated_int32.append(5)
    proto.repeated_int32.append(5)

    self.assertEqual(4, len(proto.repeated_int32))
    proto.repeated_int32.remove(5)
    self.assertEqual(3, len(proto.repeated_int32))
    self.assertEqual(10, proto.repeated_int32[0])
    self.assertEqual(5, proto.repeated_int32[1])
    self.assertEqual(5, proto.repeated_int32[2])

    proto.repeated_int32.remove(5)
    self.assertEqual(2, len(proto.repeated_int32))
    self.assertEqual(10, proto.repeated_int32[0])
    self.assertEqual(5, proto.repeated_int32[1])

    proto.repeated_int32.remove(10)
    self.assertEqual(1, len(proto.repeated_int32))
    self.assertEqual(5, proto.repeated_int32[0])

    # Remove a non-existent element.
    self.assertRaises(ValueError, proto.repeated_int32.remove, 123)

  def testRepeatedScalarsReverse_Empty(self):
    proto = unittest_pb2.TestAllTypes()

    self.assertFalse(proto.repeated_int32)
    self.assertEqual(0, len(proto.repeated_int32))

    self.assertIsNone(proto.repeated_int32.reverse())

    self.assertFalse(proto.repeated_int32)
    self.assertEqual(0, len(proto.repeated_int32))

  def testRepeatedScalarsReverse_NonEmpty(self):
    proto = unittest_pb2.TestAllTypes()

    self.assertFalse(proto.repeated_int32)
    self.assertEqual(0, len(proto.repeated_int32))

    proto.repeated_int32.append(1)
    proto.repeated_int32.append(2)
    proto.repeated_int32.append(3)
    proto.repeated_int32.append(4)

    self.assertEqual(4, len(proto.repeated_int32))

    self.assertIsNone(proto.repeated_int32.reverse())

    self.assertEqual(4, len(proto.repeated_int32))
    self.assertEqual(4, proto.repeated_int32[0])
    self.assertEqual(3, proto.repeated_int32[1])
    self.assertEqual(2, proto.repeated_int32[2])
    self.assertEqual(1, proto.repeated_int32[3])

  def testRepeatedComposites(self):
    proto = unittest_pb2.TestAllTypes()
    self.assertFalse(proto.repeated_nested_message)
    self.assertEqual(0, len(proto.repeated_nested_message))
    m0 = proto.repeated_nested_message.add()
    m1 = proto.repeated_nested_message.add()
    self.assertTrue(proto.repeated_nested_message)
    self.assertEqual(2, len(proto.repeated_nested_message))
    self.assertListsEqual([m0, m1], proto.repeated_nested_message)
    self.assertIsInstance(m0, unittest_pb2.TestAllTypes.NestedMessage)

    # Test out-of-bounds indices.
    self.assertRaises(IndexError, proto.repeated_nested_message.__getitem__,
                      1234)
    self.assertRaises(IndexError, proto.repeated_nested_message.__getitem__,
                      -1234)

    # Test incorrect types passed to __getitem__.
    self.assertRaises(TypeError, proto.repeated_nested_message.__getitem__,
                      'foo')
    self.assertRaises(TypeError, proto.repeated_nested_message.__getitem__,
                      None)

    # Test slice retrieval.
    m2 = proto.repeated_nested_message.add()
    m3 = proto.repeated_nested_message.add()
    m4 = proto.repeated_nested_message.add()
    self.assertListsEqual(
        [m1, m2, m3], proto.repeated_nested_message[1:4])
    self.assertListsEqual(
        [m0, m1, m2, m3, m4], proto.repeated_nested_message[:])
    self.assertListsEqual(
        [m0, m1], proto.repeated_nested_message[:2])
    self.assertListsEqual(
        [m2, m3, m4], proto.repeated_nested_message[2:])
    self.assertEqual(
        m0, proto.repeated_nested_message[0])
    self.assertListsEqual(
        [m0], proto.repeated_nested_message[:1])

    # Test that we can use the field as an iterator.
    result = []
    for i in proto.repeated_nested_message:
      result.append(i)
    self.assertListsEqual([m0, m1, m2, m3, m4], result)

    # Test single deletion.
    del proto.repeated_nested_message[2]
    self.assertListsEqual([m0, m1, m3, m4], proto.repeated_nested_message)

    # Test slice deletion.
    del proto.repeated_nested_message[2:]
    self.assertListsEqual([m0, m1], proto.repeated_nested_message)

    # Test extending.
    n1 = unittest_pb2.TestAllTypes.NestedMessage(bb=1)
    n2 = unittest_pb2.TestAllTypes.NestedMessage(bb=2)
    proto.repeated_nested_message.extend([n1,n2])
    self.assertEqual(4, len(proto.repeated_nested_message))
    self.assertEqual(n1, proto.repeated_nested_message[2])
    self.assertEqual(n2, proto.repeated_nested_message[3])
    self.assertRaises(TypeError,
                      proto.repeated_nested_message.extend, n1)
    self.assertRaises(TypeError,
                      proto.repeated_nested_message.extend, [0])
    wrong_message_type = unittest_pb2.TestAllTypes()
    self.assertRaises(TypeError,
                      proto.repeated_nested_message.extend,
                      [wrong_message_type])

    # Test clearing.
    proto.ClearField('repeated_nested_message')
    self.assertFalse(proto.repeated_nested_message)
    self.assertEqual(0, len(proto.repeated_nested_message))

    # Test constructing an element while adding it.
    proto.repeated_nested_message.add(bb=23)
    self.assertEqual(1, len(proto.repeated_nested_message))
    self.assertEqual(23, proto.repeated_nested_message[0].bb)
    self.assertRaises(TypeError, proto.repeated_nested_message.add, 23)
    with self.assertRaises(Exception):
      proto.repeated_nested_message[0] = 23

  def testRepeatedCompositeRemove(self):
    proto = unittest_pb2.TestAllTypes()

    self.assertEqual(0, len(proto.repeated_nested_message))
    m0 = proto.repeated_nested_message.add()
    # Need to set some differentiating variable so m0 != m1 != m2:
    m0.bb = len(proto.repeated_nested_message)
    m1 = proto.repeated_nested_message.add()
    m1.bb = len(proto.repeated_nested_message)
    self.assertTrue(m0 != m1)
    m2 = proto.repeated_nested_message.add()
    m2.bb = len(proto.repeated_nested_message)
    self.assertListsEqual([m0, m1, m2], proto.repeated_nested_message)

    self.assertEqual(3, len(proto.repeated_nested_message))
    proto.repeated_nested_message.remove(m0)
    self.assertEqual(2, len(proto.repeated_nested_message))
    self.assertEqual(m1, proto.repeated_nested_message[0])
    self.assertEqual(m2, proto.repeated_nested_message[1])

    # Removing m0 again or removing None should raise error
    self.assertRaises(ValueError, proto.repeated_nested_message.remove, m0)
    self.assertRaises(ValueError, proto.repeated_nested_message.remove, None)
    self.assertEqual(2, len(proto.repeated_nested_message))

    proto.repeated_nested_message.remove(m2)
    self.assertEqual(1, len(proto.repeated_nested_message))
    self.assertEqual(m1, proto.repeated_nested_message[0])

  def testRepeatedCompositeReverse_Empty(self):
    proto = unittest_pb2.TestAllTypes()

    self.assertFalse(proto.repeated_nested_message)
    self.assertEqual(0, len(proto.repeated_nested_message))

    self.assertIsNone(proto.repeated_nested_message.reverse())

    self.assertFalse(proto.repeated_nested_message)
    self.assertEqual(0, len(proto.repeated_nested_message))

  def testRepeatedCompositeReverse_NonEmpty(self):
    proto = unittest_pb2.TestAllTypes()

    self.assertFalse(proto.repeated_nested_message)
    self.assertEqual(0, len(proto.repeated_nested_message))

    m0 = proto.repeated_nested_message.add()
    m0.bb = len(proto.repeated_nested_message)
    m1 = proto.repeated_nested_message.add()
    m1.bb = len(proto.repeated_nested_message)
    m2 = proto.repeated_nested_message.add()
    m2.bb = len(proto.repeated_nested_message)
    self.assertListsEqual([m0, m1, m2], proto.repeated_nested_message)

    self.assertIsNone(proto.repeated_nested_message.reverse())

    self.assertListsEqual([m2, m1, m0], proto.repeated_nested_message)

  def testHandWrittenReflection(self):
    # Hand written extensions are only supported by the pure-Python
    # implementation of the API.
    if api_implementation.Type() != 'python':
      return

    FieldDescriptor = descriptor.FieldDescriptor
    foo_field_descriptor = FieldDescriptor(
        name='foo_field', full_name='MyProto.foo_field',
        index=0, number=1, type=FieldDescriptor.TYPE_INT64,
        cpp_type=FieldDescriptor.CPPTYPE_INT64,
        label=FieldDescriptor.LABEL_OPTIONAL, default_value=0,
        containing_type=None, message_type=None, enum_type=None,
        is_extension=False, extension_scope=None,
        options=descriptor_pb2.FieldOptions(),
        # pylint: disable=protected-access
        create_key=descriptor._internal_create_key)
    mydescriptor = descriptor.Descriptor(
        name='MyProto', full_name='MyProto', filename='ignored',
        containing_type=None, nested_types=[], enum_types=[],
        fields=[foo_field_descriptor], extensions=[],
        options=descriptor_pb2.MessageOptions(),
        # pylint: disable=protected-access
        create_key=descriptor._internal_create_key)

    class MyProtoClass(
        message.Message, metaclass=reflection.GeneratedProtocolMessageType):
      DESCRIPTOR = mydescriptor
    myproto_instance = MyProtoClass()
    self.assertEqual(0, myproto_instance.foo_field)
    self.assertFalse(myproto_instance.HasField('foo_field'))
    myproto_instance.foo_field = 23
    self.assertEqual(23, myproto_instance.foo_field)
    self.assertTrue(myproto_instance.HasField('foo_field'))

  @testing_refleaks.SkipReferenceLeakChecker('MakeDescriptor is not repeatable')
  def testDescriptorProtoSupport(self):
    # Hand written descriptors/reflection are only supported by the pure-Python
    # implementation of the API.
    if api_implementation.Type() != 'python':
      return

    def AddDescriptorField(proto, field_name, field_type):
      AddDescriptorField.field_index += 1
      new_field = proto.field.add()
      new_field.name = field_name
      new_field.type = field_type
      new_field.number = AddDescriptorField.field_index
      new_field.label = descriptor_pb2.FieldDescriptorProto.LABEL_OPTIONAL

    AddDescriptorField.field_index = 0

    desc_proto = descriptor_pb2.DescriptorProto()
    desc_proto.name = 'Car'
    fdp = descriptor_pb2.FieldDescriptorProto
    AddDescriptorField(desc_proto, 'name', fdp.TYPE_STRING)
    AddDescriptorField(desc_proto, 'year', fdp.TYPE_INT64)
    AddDescriptorField(desc_proto, 'automatic', fdp.TYPE_BOOL)
    AddDescriptorField(desc_proto, 'price', fdp.TYPE_DOUBLE)
    # Add a repeated field
    AddDescriptorField.field_index += 1
    new_field = desc_proto.field.add()
    new_field.name = 'owners'
    new_field.type = fdp.TYPE_STRING
    new_field.number = AddDescriptorField.field_index
    new_field.label = descriptor_pb2.FieldDescriptorProto.LABEL_REPEATED

    desc = descriptor.MakeDescriptor(desc_proto)
    self.assertTrue('name' in desc.fields_by_name)
    self.assertTrue('year' in desc.fields_by_name)
    self.assertTrue('automatic' in desc.fields_by_name)
    self.assertTrue('price' in desc.fields_by_name)
    self.assertTrue('owners' in desc.fields_by_name)

    class CarMessage(
        message.Message, metaclass=reflection.GeneratedProtocolMessageType):
      DESCRIPTOR = desc

    prius = CarMessage()
    prius.name = 'prius'
    prius.year = 2010
    prius.automatic = True
    prius.price = 25134.75
    prius.owners.extend(['bob', 'susan'])

    serialized_prius = prius.SerializeToString()
    new_prius = reflection.ParseMessage(desc, serialized_prius)
    self.assertIsNot(new_prius, prius)
    self.assertEqual(prius, new_prius)

    # these are unnecessary assuming message equality works as advertised but
    # explicitly check to be safe since we're mucking about in metaclass foo
    self.assertEqual(prius.name, new_prius.name)
    self.assertEqual(prius.year, new_prius.year)
    self.assertEqual(prius.automatic, new_prius.automatic)
    self.assertEqual(prius.price, new_prius.price)
    self.assertEqual(prius.owners, new_prius.owners)

  def testExtensionDelete(self):
    extendee_proto = more_extensions_pb2.ExtendedMessage()

    extension_int32 = more_extensions_pb2.optional_int_extension
    extendee_proto.Extensions[extension_int32] = 23

    extension_repeated = more_extensions_pb2.repeated_int_extension
    extendee_proto.Extensions[extension_repeated].append(11)

    extension_msg = more_extensions_pb2.optional_message_extension
    extendee_proto.Extensions[extension_msg].foreign_message_int = 56

    self.assertEqual(len(extendee_proto.Extensions), 3)
    del extendee_proto.Extensions[extension_msg]
    self.assertEqual(len(extendee_proto.Extensions), 2)
    del extendee_proto.Extensions[extension_repeated]
    self.assertEqual(len(extendee_proto.Extensions), 1)
    # Delete a none exist extension. It is OK to "del m.Extensions[ext]"
    # even if the extension is not present in the message; we don't
    # raise KeyError. This is consistent with "m.Extensions[ext]"
    # returning a default value even if we did not set anything.
    del extendee_proto.Extensions[extension_repeated]
    self.assertEqual(len(extendee_proto.Extensions), 1)
    del extendee_proto.Extensions[extension_int32]
    self.assertEqual(len(extendee_proto.Extensions), 0)

  def testExtensionIter(self):
    extendee_proto = more_extensions_pb2.ExtendedMessage()

    extension_int32 = more_extensions_pb2.optional_int_extension
    extendee_proto.Extensions[extension_int32] = 23

    extension_repeated = more_extensions_pb2.repeated_int_extension
    extendee_proto.Extensions[extension_repeated].append(11)

    extension_msg = more_extensions_pb2.optional_message_extension
    extendee_proto.Extensions[extension_msg].foreign_message_int = 56

    # Set some normal fields.
    extendee_proto.optional_int32 = 1
    extendee_proto.repeated_string.append('hi')

    expected = (extension_int32, extension_msg, extension_repeated)
    count = 0
    for item in extendee_proto.Extensions:
      self.assertEqual(item.name, expected[count].name)
      self.assertIn(item, extendee_proto.Extensions)
      count += 1
    self.assertEqual(count, 3)

  def testExtensionContainsError(self):
    extendee_proto = more_extensions_pb2.ExtendedMessage()
    self.assertRaises(KeyError, extendee_proto.Extensions.__contains__, 0)

    field = more_extensions_pb2.ExtendedMessage.DESCRIPTOR.fields_by_name[
        'optional_int32']
    self.assertRaises(KeyError, extendee_proto.Extensions.__contains__, field)

  def testTopLevelExtensionsForOptionalScalar(self):
    extendee_proto = unittest_pb2.TestAllExtensions()
    extension = unittest_pb2.optional_int32_extension
    self.assertFalse(extendee_proto.HasExtension(extension))
    self.assertNotIn(extension, extendee_proto.Extensions)
    self.assertEqual(0, extendee_proto.Extensions[extension])
    # As with normal scalar fields, just doing a read doesn't actually set the
    # "has" bit.
    self.assertFalse(extendee_proto.HasExtension(extension))
    self.assertNotIn(extension, extendee_proto.Extensions)
    # Actually set the thing.
    extendee_proto.Extensions[extension] = 23
    self.assertEqual(23, extendee_proto.Extensions[extension])
    self.assertTrue(extendee_proto.HasExtension(extension))
    self.assertIn(extension, extendee_proto.Extensions)
    # Ensure that clearing works as well.
    extendee_proto.ClearExtension(extension)
    self.assertEqual(0, extendee_proto.Extensions[extension])
    self.assertFalse(extendee_proto.HasExtension(extension))
    self.assertNotIn(extension, extendee_proto.Extensions)

  def testTopLevelExtensionsForRepeatedScalar(self):
    extendee_proto = unittest_pb2.TestAllExtensions()
    extension = unittest_pb2.repeated_string_extension
    self.assertEqual(0, len(extendee_proto.Extensions[extension]))
    self.assertNotIn(extension, extendee_proto.Extensions)
    extendee_proto.Extensions[extension].append('foo')
    self.assertEqual(['foo'], extendee_proto.Extensions[extension])
    self.assertIn(extension, extendee_proto.Extensions)
    string_list = extendee_proto.Extensions[extension]
    extendee_proto.ClearExtension(extension)
    self.assertEqual(0, len(extendee_proto.Extensions[extension]))
    self.assertNotIn(extension, extendee_proto.Extensions)
    self.assertIsNot(string_list, extendee_proto.Extensions[extension])
    # Shouldn't be allowed to do Extensions[extension] = 'a'
    self.assertRaises(TypeError, operator.setitem, extendee_proto.Extensions,
                      extension, 'a')

  def testTopLevelExtensionsForOptionalMessage(self):
    extendee_proto = unittest_pb2.TestAllExtensions()
    extension = unittest_pb2.optional_foreign_message_extension
    self.assertFalse(extendee_proto.HasExtension(extension))
    self.assertNotIn(extension, extendee_proto.Extensions)
    self.assertEqual(0, extendee_proto.Extensions[extension].c)
    # As with normal (non-extension) fields, merely reading from the
    # thing shouldn't set the "has" bit.
    self.assertFalse(extendee_proto.HasExtension(extension))
    self.assertNotIn(extension, extendee_proto.Extensions)
    extendee_proto.Extensions[extension].c = 23
    self.assertEqual(23, extendee_proto.Extensions[extension].c)
    self.assertTrue(extendee_proto.HasExtension(extension))
    self.assertIn(extension, extendee_proto.Extensions)
    # Save a reference here.
    foreign_message = extendee_proto.Extensions[extension]
    extendee_proto.ClearExtension(extension)
    self.assertIsNot(foreign_message, extendee_proto.Extensions[extension])
    # Setting a field on foreign_message now shouldn't set
    # any "has" bits on extendee_proto.
    foreign_message.c = 42
    self.assertEqual(42, foreign_message.c)
    self.assertTrue(foreign_message.HasField('c'))
    self.assertFalse(extendee_proto.HasExtension(extension))
    self.assertNotIn(extension, extendee_proto.Extensions)
    # Shouldn't be allowed to do Extensions[extension] = 'a'
    self.assertRaises(TypeError, operator.setitem, extendee_proto.Extensions,
                      extension, 'a')

  def testTopLevelExtensionsForRepeatedMessage(self):
    extendee_proto = unittest_pb2.TestAllExtensions()
    extension = unittest_pb2.repeatedgroup_extension
    self.assertEqual(0, len(extendee_proto.Extensions[extension]))
    group = extendee_proto.Extensions[extension].add()
    group.a = 23
    self.assertEqual(23, extendee_proto.Extensions[extension][0].a)
    group.a = 42
    self.assertEqual(42, extendee_proto.Extensions[extension][0].a)
    group_list = extendee_proto.Extensions[extension]
    extendee_proto.ClearExtension(extension)
    self.assertEqual(0, len(extendee_proto.Extensions[extension]))
    self.assertIsNot(group_list, extendee_proto.Extensions[extension])
    # Shouldn't be allowed to do Extensions[extension] = 'a'
    self.assertRaises(TypeError, operator.setitem, extendee_proto.Extensions,
                      extension, 'a')

  def testNestedExtensions(self):
    extendee_proto = unittest_pb2.TestAllExtensions()
    extension = unittest_pb2.TestRequired.single

    # We just test the non-repeated case.
    self.assertFalse(extendee_proto.HasExtension(extension))
    self.assertNotIn(extension, extendee_proto.Extensions)
    required = extendee_proto.Extensions[extension]
    self.assertEqual(0, required.a)
    self.assertFalse(extendee_proto.HasExtension(extension))
    self.assertNotIn(extension, extendee_proto.Extensions)
    required.a = 23
    self.assertEqual(23, extendee_proto.Extensions[extension].a)
    self.assertTrue(extendee_proto.HasExtension(extension))
    self.assertIn(extension, extendee_proto.Extensions)
    extendee_proto.ClearExtension(extension)
    self.assertIsNot(required, extendee_proto.Extensions[extension])
    self.assertFalse(extendee_proto.HasExtension(extension))
    self.assertNotIn(extension, extendee_proto.Extensions)

  def testRegisteredExtensions(self):
    pool = unittest_pb2.DESCRIPTOR.pool
    self.assertTrue(
        pool.FindExtensionByNumber(
            unittest_pb2.TestAllExtensions.DESCRIPTOR, 1))
    self.assertIs(
        pool.FindExtensionByName(
            'protobuf_unittest.optional_int32_extension').containing_type,
        unittest_pb2.TestAllExtensions.DESCRIPTOR)
    # Make sure extensions haven't been registered into types that shouldn't
    # have any.
    self.assertEqual(0, len(
        pool.FindAllExtensions(unittest_pb2.TestAllTypes.DESCRIPTOR)))

  # If message A directly contains message B, and
  # a.HasField('b') is currently False, then mutating any
  # extension in B should change a.HasField('b') to True
  # (and so on up the object tree).
  def testHasBitsForAncestorsOfExtendedMessage(self):
    # Optional scalar extension.
    toplevel = more_extensions_pb2.TopLevelMessage()
    self.assertFalse(toplevel.HasField('submessage'))
    self.assertEqual(0, toplevel.submessage.Extensions[
        more_extensions_pb2.optional_int_extension])
    self.assertFalse(toplevel.HasField('submessage'))
    toplevel.submessage.Extensions[
        more_extensions_pb2.optional_int_extension] = 23
    self.assertEqual(23, toplevel.submessage.Extensions[
        more_extensions_pb2.optional_int_extension])
    self.assertTrue(toplevel.HasField('submessage'))

    # Repeated scalar extension.
    toplevel = more_extensions_pb2.TopLevelMessage()
    self.assertFalse(toplevel.HasField('submessage'))
    self.assertEqual([], toplevel.submessage.Extensions[
        more_extensions_pb2.repeated_int_extension])
    self.assertFalse(toplevel.HasField('submessage'))
    toplevel.submessage.Extensions[
        more_extensions_pb2.repeated_int_extension].append(23)
    self.assertEqual([23], toplevel.submessage.Extensions[
        more_extensions_pb2.repeated_int_extension])
    self.assertTrue(toplevel.HasField('submessage'))

    # Optional message extension.
    toplevel = more_extensions_pb2.TopLevelMessage()
    self.assertFalse(toplevel.HasField('submessage'))
    self.assertEqual(0, toplevel.submessage.Extensions[
        more_extensions_pb2.optional_message_extension].foreign_message_int)
    self.assertFalse(toplevel.HasField('submessage'))
    toplevel.submessage.Extensions[
        more_extensions_pb2.optional_message_extension].foreign_message_int = 23
    self.assertEqual(23, toplevel.submessage.Extensions[
        more_extensions_pb2.optional_message_extension].foreign_message_int)
    self.assertTrue(toplevel.HasField('submessage'))

    # Repeated message extension.
    toplevel = more_extensions_pb2.TopLevelMessage()
    self.assertFalse(toplevel.HasField('submessage'))
    self.assertEqual(0, len(toplevel.submessage.Extensions[
        more_extensions_pb2.repeated_message_extension]))
    self.assertFalse(toplevel.HasField('submessage'))
    foreign = toplevel.submessage.Extensions[
        more_extensions_pb2.repeated_message_extension].add()
    self.assertEqual(foreign, toplevel.submessage.Extensions[
        more_extensions_pb2.repeated_message_extension][0])
    self.assertTrue(toplevel.HasField('submessage'))

  def testDisconnectionAfterClearingEmptyMessage(self):
    toplevel = more_extensions_pb2.TopLevelMessage()
    extendee_proto = toplevel.submessage
    extension = more_extensions_pb2.optional_message_extension
    extension_proto = extendee_proto.Extensions[extension]
    extendee_proto.ClearExtension(extension)
    extension_proto.foreign_message_int = 23

    self.assertIsNot(extension_proto, extendee_proto.Extensions[extension])

  def testExtensionFailureModes(self):
    extendee_proto = unittest_pb2.TestAllExtensions()

    # Try non-extension-handle arguments to HasExtension,
    # ClearExtension(), and Extensions[]...
    self.assertRaises(KeyError, extendee_proto.HasExtension, 1234)
    self.assertRaises(KeyError, extendee_proto.ClearExtension, 1234)
    self.assertRaises(KeyError, extendee_proto.Extensions.__getitem__, 1234)
    self.assertRaises(KeyError, extendee_proto.Extensions.__setitem__, 1234, 5)

    # Try something that *is* an extension handle, just not for
    # this message...
    for unknown_handle in (more_extensions_pb2.optional_int_extension,
                           more_extensions_pb2.optional_message_extension,
                           more_extensions_pb2.repeated_int_extension,
                           more_extensions_pb2.repeated_message_extension):
      self.assertRaises(KeyError, extendee_proto.HasExtension,
                        unknown_handle)
      self.assertRaises(KeyError, extendee_proto.ClearExtension,
                        unknown_handle)
      self.assertRaises(KeyError, extendee_proto.Extensions.__getitem__,
                        unknown_handle)
      self.assertRaises(KeyError, extendee_proto.Extensions.__setitem__,
                        unknown_handle, 5)

    # Try call HasExtension() with a valid handle, but for a
    # *repeated* field.  (Just as with non-extension repeated
    # fields, Has*() isn't supported for extension repeated fields).
    self.assertRaises(KeyError, extendee_proto.HasExtension,
                      unittest_pb2.repeated_string_extension)

  def testMergeFromOptionalGroup(self):
    # Test merge with an optional group.
    proto1 = unittest_pb2.TestAllTypes()
    proto1.optionalgroup.a = 12
    proto2 = unittest_pb2.TestAllTypes()
    proto2.MergeFrom(proto1)
    self.assertEqual(12, proto2.optionalgroup.a)

  def testMergeFromExtensionsSingular(self):
    proto1 = unittest_pb2.TestAllExtensions()
    proto1.Extensions[unittest_pb2.optional_int32_extension] = 1

    proto2 = unittest_pb2.TestAllExtensions()
    proto2.MergeFrom(proto1)
    self.assertEqual(
        1, proto2.Extensions[unittest_pb2.optional_int32_extension])

  def testMergeFromExtensionsRepeated(self):
    proto1 = unittest_pb2.TestAllExtensions()
    proto1.Extensions[unittest_pb2.repeated_int32_extension].append(1)
    proto1.Extensions[unittest_pb2.repeated_int32_extension].append(2)

    proto2 = unittest_pb2.TestAllExtensions()
    proto2.Extensions[unittest_pb2.repeated_int32_extension].append(0)
    proto2.MergeFrom(proto1)
    self.assertEqual(
        3, len(proto2.Extensions[unittest_pb2.repeated_int32_extension]))
    self.assertEqual(
        0, proto2.Extensions[unittest_pb2.repeated_int32_extension][0])
    self.assertEqual(
        1, proto2.Extensions[unittest_pb2.repeated_int32_extension][1])
    self.assertEqual(
        2, proto2.Extensions[unittest_pb2.repeated_int32_extension][2])

  def testMergeFromExtensionsNestedMessage(self):
    proto1 = unittest_pb2.TestAllExtensions()
    ext1 = proto1.Extensions[
        unittest_pb2.repeated_nested_message_extension]
    m = ext1.add()
    m.bb = 222
    m = ext1.add()
    m.bb = 333

    proto2 = unittest_pb2.TestAllExtensions()
    ext2 = proto2.Extensions[
        unittest_pb2.repeated_nested_message_extension]
    m = ext2.add()
    m.bb = 111

    proto2.MergeFrom(proto1)
    ext2 = proto2.Extensions[
        unittest_pb2.repeated_nested_message_extension]
    self.assertEqual(3, len(ext2))
    self.assertEqual(111, ext2[0].bb)
    self.assertEqual(222, ext2[1].bb)
    self.assertEqual(333, ext2[2].bb)

  def testCopyFromBadType(self):
    # The python implementation doesn't raise an exception in this
    # case. In theory it should.
    if api_implementation.Type() == 'python':
      return
    proto1 = unittest_pb2.TestAllTypes()
    proto2 = unittest_pb2.TestAllExtensions()
    self.assertRaises(TypeError, proto1.CopyFrom, proto2)

  def testClear(self):
    proto = unittest_pb2.TestAllTypes()
    # C++ implementation does not support lazy fields right now so leave it
    # out for now.
    if api_implementation.Type() == 'python':
      test_util.SetAllFields(proto)
    else:
      test_util.SetAllNonLazyFields(proto)
    # Clear the message.
    proto.Clear()
    self.assertEqual(proto.ByteSize(), 0)
    empty_proto = unittest_pb2.TestAllTypes()
    self.assertEqual(proto, empty_proto)

    # Test if extensions which were set are cleared.
    proto = unittest_pb2.TestAllExtensions()
    test_util.SetAllExtensions(proto)
    # Clear the message.
    proto.Clear()
    self.assertEqual(proto.ByteSize(), 0)
    empty_proto = unittest_pb2.TestAllExtensions()
    self.assertEqual(proto, empty_proto)

  def testDisconnectingInOneof(self):
    m = unittest_pb2.TestOneof2()  # This message has two messages in a oneof.
    m.foo_message.moo_int = 5
    sub_message = m.foo_message
    # Accessing another message's field does not clear the first one
    self.assertEqual(m.foo_lazy_message.moo_int, 0)
    self.assertEqual(m.foo_message.moo_int, 5)
    # But mutating another message in the oneof detaches the first one.
    m.foo_lazy_message.moo_int = 6
    self.assertEqual(m.foo_message.moo_int, 0)
    # The reference we got above was detached and is still valid.
    self.assertEqual(sub_message.moo_int, 5)
    sub_message.moo_int = 7

  def assertInitialized(self, proto):
    self.assertTrue(proto.IsInitialized())
    # Neither method should raise an exception.
    proto.SerializeToString()
    proto.SerializePartialToString()

  def assertNotInitialized(self, proto, error_size=None):
    errors = []
    self.assertFalse(proto.IsInitialized())
    self.assertFalse(proto.IsInitialized(errors))
    self.assertEqual(error_size, len(errors))
    self.assertRaises(message.EncodeError, proto.SerializeToString)
    # "Partial" serialization doesn't care if message is uninitialized.
    proto.SerializePartialToString()

  def testIsInitialized(self):
    # Trivial cases - all optional fields and extensions.
    proto = unittest_pb2.TestAllTypes()
    self.assertInitialized(proto)
    proto = unittest_pb2.TestAllExtensions()
    self.assertInitialized(proto)

    # The case of uninitialized required fields.
    proto = unittest_pb2.TestRequired()
    self.assertNotInitialized(proto, 3)
    proto.a = proto.b = proto.c = 2
    self.assertInitialized(proto)

    # The case of uninitialized submessage.
    proto = unittest_pb2.TestRequiredForeign()
    self.assertInitialized(proto)
    proto.optional_message.a = 1
    self.assertNotInitialized(proto, 2)
    proto.optional_message.b = 0
    proto.optional_message.c = 0
    self.assertInitialized(proto)

    # Uninitialized repeated submessage.
    message1 = proto.repeated_message.add()
    self.assertNotInitialized(proto, 3)
    message1.a = message1.b = message1.c = 0
    self.assertInitialized(proto)

    # Uninitialized repeated group in an extension.
    proto = unittest_pb2.TestAllExtensions()
    extension = unittest_pb2.TestRequired.multi
    message1 = proto.Extensions[extension].add()
    message2 = proto.Extensions[extension].add()
    self.assertNotInitialized(proto, 6)
    message1.a = 1
    message1.b = 1
    message1.c = 1
    self.assertNotInitialized(proto, 3)
    message2.a = 2
    message2.b = 2
    message2.c = 2
    self.assertInitialized(proto)

    # Uninitialized nonrepeated message in an extension.
    proto = unittest_pb2.TestAllExtensions()
    extension = unittest_pb2.TestRequired.single
    proto.Extensions[extension].a = 1
    self.assertNotInitialized(proto, 2)
    proto.Extensions[extension].b = 2
    proto.Extensions[extension].c = 3
    self.assertInitialized(proto)

    # Try passing an errors list.
    errors = []
    proto = unittest_pb2.TestRequired()
    self.assertFalse(proto.IsInitialized(errors))
    self.assertEqual(errors, ['a', 'b', 'c'])
    self.assertRaises(TypeError, proto.IsInitialized, 1, 2, 3)

  @unittest.skipIf(
      api_implementation.Type() == 'python',
      'Errors are only available from the most recent C++ implementation.')
  def testFileDescriptorErrors(self):
    file_name = 'test_file_descriptor_errors.proto'
    package_name = 'test_file_descriptor_errors.proto'
    file_descriptor_proto = descriptor_pb2.FileDescriptorProto()
    file_descriptor_proto.name = file_name
    file_descriptor_proto.package = package_name
    m1 = file_descriptor_proto.message_type.add()
    m1.name = 'msg1'
    # Compiles the proto into the C++ descriptor pool
    descriptor.FileDescriptor(
        file_name,
        package_name,
        serialized_pb=file_descriptor_proto.SerializeToString())
    # Add a FileDescriptorProto that has duplicate symbols
    another_file_name = 'another_test_file_descriptor_errors.proto'
    file_descriptor_proto.name = another_file_name
    m2 = file_descriptor_proto.message_type.add()
    m2.name = 'msg2'
    with self.assertRaises(TypeError) as cm:
      descriptor.FileDescriptor(
          another_file_name,
          package_name,
          serialized_pb=file_descriptor_proto.SerializeToString())
      self.assertTrue(hasattr(cm, 'exception'), '%s not raised' %
                      getattr(cm.expected, '__name__', cm.expected))
      self.assertIn('test_file_descriptor_errors.proto', str(cm.exception))
      # Error message will say something about this definition being a
      # duplicate, though we don't check the message exactly to avoid a
      # dependency on the C++ logging code.
      self.assertIn('test_file_descriptor_errors.msg1', str(cm.exception))

  def testDescriptorProtoHasFileOptions(self):
    self.assertTrue(descriptor_pb2.DESCRIPTOR.has_options)
    self.assertEqual(
        descriptor_pb2.DESCRIPTOR.GetOptions().java_package,
        'com.google.protobuf',
    )

  def testDescriptorProtoHasFieldOptions(self):
    self.assertTrue(descriptor_pb2.DESCRIPTOR.has_options)
    self.assertEqual(
        descriptor_pb2.DESCRIPTOR.GetOptions().java_package,
        'com.google.protobuf',
    )
    packed_desc = (
        descriptor_pb2.SourceCodeInfo.DESCRIPTOR.nested_types_by_name.get(
            'Location'
        ).fields_by_name.get('path')
    )
    self.assertTrue(packed_desc.has_options)
    self.assertTrue(packed_desc.GetOptions().packed)

  def testDescriptorProtoHasFeatureOptions(self):
    self.assertTrue(descriptor_pb2.DESCRIPTOR.has_options)
    self.assertEqual(
        descriptor_pb2.DESCRIPTOR.GetOptions().java_package,
        'com.google.protobuf',
    )
    presence_desc = descriptor_pb2.FeatureSet.DESCRIPTOR.fields_by_name.get(
        'field_presence'
    )
    self.assertTrue(presence_desc.has_options)
    self.assertEqual(
        presence_desc.GetOptions().retention,
        descriptor_pb2.FieldOptions.OptionRetention.RETENTION_RUNTIME,
    )
    self.assertListsEqual(
        presence_desc.GetOptions().targets,
        [
            descriptor_pb2.FieldOptions.OptionTargetType.TARGET_TYPE_FIELD,
            descriptor_pb2.FieldOptions.OptionTargetType.TARGET_TYPE_FILE,
        ],
    )

  def testStringUTF8Serialization(self):
    proto = message_set_extensions_pb2.TestMessageSet()
    extension_message = message_set_extensions_pb2.TestMessageSetExtension2
    extension = extension_message.message_set_extension

    test_utf8 = u'Тест'
    test_utf8_bytes = test_utf8.encode('utf-8')

    # 'Test' in another language, using UTF-8 charset.
    proto.Extensions[extension].str = test_utf8

    # Serialize using the MessageSet wire format (this is specified in the
    # .proto file).
    serialized = proto.SerializeToString()

    # Check byte size.
    self.assertEqual(proto.ByteSize(), len(serialized))

    raw = unittest_mset_pb2.RawMessageSet()
    bytes_read = raw.MergeFromString(serialized)
    self.assertEqual(len(serialized), bytes_read)

    message2 = message_set_extensions_pb2.TestMessageSetExtension2()

    self.assertEqual(1, len(raw.item))
    # Check that the type_id is the same as the tag ID in the .proto file.
    self.assertEqual(raw.item[0].type_id, 98418634)

    # Check the actual bytes on the wire.
    self.assertTrue(raw.item[0].message.endswith(test_utf8_bytes))
    bytes_read = message2.MergeFromString(raw.item[0].message)
    self.assertEqual(len(raw.item[0].message), bytes_read)

    self.assertEqual(type(message2.str), str)
    self.assertEqual(message2.str, test_utf8)

    # The pure Python API throws an exception on MergeFromString(),
    # if any of the string fields of the message can't be UTF-8 decoded.
    # The C++ implementation of the API has no way to check that on
    # MergeFromString and thus has no way to throw the exception.
    #
    # The pure Python API always returns objects of type 'unicode' (UTF-8
    # encoded), or 'bytes' (in 7 bit ASCII).
    badbytes = raw.item[0].message.replace(
        test_utf8_bytes, len(test_utf8_bytes) * b'\xff')

    unicode_decode_failed = False
    try:
      message2.MergeFromString(badbytes)
    except UnicodeDecodeError:
      unicode_decode_failed = True
    string_field = message2.str
    self.assertTrue(unicode_decode_failed or type(string_field) is bytes)

  def testSetInParent(self):
    proto = unittest_pb2.TestAllTypes()
    self.assertFalse(proto.HasField('optionalgroup'))
    proto.optionalgroup.SetInParent()
    self.assertTrue(proto.HasField('optionalgroup'))

  def testPackageInitializationImport(self):
    """Test that we can import nested messages from their __init__.py.

    Such setup is not trivial since at the time of processing of __init__.py one
    can't refer to its submodules by name in code, so expressions like
    google.protobuf.internal.import_test_package.inner_pb2
    don't work. They do work in imports, so we have assign an alias at import
    and then use that alias in generated code.
    """
    # We import here since it's the import that used to fail, and we want
    # the failure to have the right context.
    # pylint: disable=g-import-not-at-top
    from google.protobuf.internal import import_test_package
    # pylint: enable=g-import-not-at-top
    msg = import_test_package.myproto.Outer()
    # Just check the default value.
    self.assertEqual(57, msg.inner.value)

#  Since we had so many tests for protocol buffer equality, we broke these out
#  into separate TestCase classes.


@testing_refleaks.TestCase
class TestAllTypesEqualityTest(unittest.TestCase):

  def setUp(self):
    self.first_proto = unittest_pb2.TestAllTypes()
    self.second_proto = unittest_pb2.TestAllTypes()

  def testNotHashable(self):
    self.assertRaises(TypeError, hash, self.first_proto)

  def testSelfEquality(self):
    self.assertEqual(self.first_proto, self.first_proto)

  def testEmptyProtosEqual(self):
    self.assertEqual(self.first_proto, self.second_proto)


@testing_refleaks.TestCase
class FullProtosEqualityTest(unittest.TestCase):

  """Equality tests using completely-full protos as a starting point."""

  def setUp(self):
    self.first_proto = unittest_pb2.TestAllTypes()
    self.second_proto = unittest_pb2.TestAllTypes()
    test_util.SetAllFields(self.first_proto)
    test_util.SetAllFields(self.second_proto)

  def testNotHashable(self):
    self.assertRaises(TypeError, hash, self.first_proto)

  def testNoneNotEqual(self):
    self.assertNotEqual(self.first_proto, None)
    self.assertNotEqual(None, self.second_proto)

  def testNotEqualToOtherMessage(self):
    third_proto = unittest_pb2.TestRequired()
    self.assertNotEqual(self.first_proto, third_proto)
    self.assertNotEqual(third_proto, self.second_proto)

  def testAllFieldsFilledEquality(self):
    self.assertEqual(self.first_proto, self.second_proto)

  def testNonRepeatedScalar(self):
    # Nonrepeated scalar field change should cause inequality.
    self.first_proto.optional_int32 += 1
    self.assertNotEqual(self.first_proto, self.second_proto)
    # ...as should clearing a field.
    self.first_proto.ClearField('optional_int32')
    self.assertNotEqual(self.first_proto, self.second_proto)

  def testNonRepeatedComposite(self):
    # Change a nonrepeated composite field.
    self.first_proto.optional_nested_message.bb += 1
    self.assertNotEqual(self.first_proto, self.second_proto)
    self.first_proto.optional_nested_message.bb -= 1
    self.assertEqual(self.first_proto, self.second_proto)
    # Clear a field in the nested message.
    self.first_proto.optional_nested_message.ClearField('bb')
    self.assertNotEqual(self.first_proto, self.second_proto)
    self.first_proto.optional_nested_message.bb = (
        self.second_proto.optional_nested_message.bb)
    self.assertEqual(self.first_proto, self.second_proto)
    # Remove the nested message entirely.
    self.first_proto.ClearField('optional_nested_message')
    self.assertNotEqual(self.first_proto, self.second_proto)

  def testRepeatedScalar(self):
    # Change a repeated scalar field.
    self.first_proto.repeated_int32.append(5)
    self.assertNotEqual(self.first_proto, self.second_proto)
    self.first_proto.ClearField('repeated_int32')
    self.assertNotEqual(self.first_proto, self.second_proto)

  def testRepeatedComposite(self):
    # Change value within a repeated composite field.
    self.first_proto.repeated_nested_message[0].bb += 1
    self.assertNotEqual(self.first_proto, self.second_proto)
    self.first_proto.repeated_nested_message[0].bb -= 1
    self.assertEqual(self.first_proto, self.second_proto)
    # Add a value to a repeated composite field.
    self.first_proto.repeated_nested_message.add()
    self.assertNotEqual(self.first_proto, self.second_proto)
    self.second_proto.repeated_nested_message.add()
    self.assertEqual(self.first_proto, self.second_proto)

  def testNonRepeatedScalarHasBits(self):
    # Ensure that we test "has" bits as well as value for
    # nonrepeated scalar field.
    self.first_proto.ClearField('optional_int32')
    self.second_proto.optional_int32 = 0
    self.assertNotEqual(self.first_proto, self.second_proto)

  def testNonRepeatedCompositeHasBits(self):
    # Ensure that we test "has" bits as well as value for
    # nonrepeated composite field.
    self.first_proto.ClearField('optional_nested_message')
    self.second_proto.optional_nested_message.ClearField('bb')
    self.assertNotEqual(self.first_proto, self.second_proto)
    self.first_proto.optional_nested_message.bb = 0
    self.first_proto.optional_nested_message.ClearField('bb')
    self.assertEqual(self.first_proto, self.second_proto)


@testing_refleaks.TestCase
class ExtensionEqualityTest(unittest.TestCase):

  def testExtensionEquality(self):
    first_proto = unittest_pb2.TestAllExtensions()
    second_proto = unittest_pb2.TestAllExtensions()
    self.assertEqual(first_proto, second_proto)
    test_util.SetAllExtensions(first_proto)
    self.assertNotEqual(first_proto, second_proto)
    test_util.SetAllExtensions(second_proto)
    self.assertEqual(first_proto, second_proto)

    # Ensure that we check value equality.
    first_proto.Extensions[unittest_pb2.optional_int32_extension] += 1
    self.assertNotEqual(first_proto, second_proto)
    first_proto.Extensions[unittest_pb2.optional_int32_extension] -= 1
    self.assertEqual(first_proto, second_proto)

    # Ensure that we also look at "has" bits.
    first_proto.ClearExtension(unittest_pb2.optional_int32_extension)
    second_proto.Extensions[unittest_pb2.optional_int32_extension] = 0
    self.assertNotEqual(first_proto, second_proto)
    first_proto.Extensions[unittest_pb2.optional_int32_extension] = 0
    self.assertEqual(first_proto, second_proto)

    # Ensure that differences in cached values
    # don't matter if "has" bits are both false.
    first_proto = unittest_pb2.TestAllExtensions()
    second_proto = unittest_pb2.TestAllExtensions()
    self.assertEqual(
        0, first_proto.Extensions[unittest_pb2.optional_int32_extension])
    self.assertEqual(first_proto, second_proto)


@testing_refleaks.TestCase
class MutualRecursionEqualityTest(unittest.TestCase):

  def testEqualityWithMutualRecursion(self):
    first_proto = unittest_pb2.TestMutualRecursionA()
    second_proto = unittest_pb2.TestMutualRecursionA()
    self.assertEqual(first_proto, second_proto)
    first_proto.bb.a.bb.optional_int32 = 23
    self.assertNotEqual(first_proto, second_proto)
    second_proto.bb.a.bb.optional_int32 = 23
    self.assertEqual(first_proto, second_proto)


@testing_refleaks.TestCase
class ByteSizeTest(unittest.TestCase):

  def setUp(self):
    self.proto = unittest_pb2.TestAllTypes()
    self.extended_proto = more_extensions_pb2.ExtendedMessage()
    self.packed_proto = unittest_pb2.TestPackedTypes()
    self.packed_extended_proto = unittest_pb2.TestPackedExtensions()

  def Size(self):
    return self.proto.ByteSize()

  def testEmptyMessage(self):
    self.assertEqual(0, self.proto.ByteSize())

  def testSizedOnKwargs(self):
    # Use a separate message to ensure testing right after creation.
    proto = unittest_pb2.TestAllTypes()
    self.assertEqual(0, proto.ByteSize())
    proto_kwargs = unittest_pb2.TestAllTypes(optional_int64 = 1)
    # One byte for the tag, one to encode varint 1.
    self.assertEqual(2, proto_kwargs.ByteSize())

  def testVarints(self):
    def Test(i, expected_varint_size):
      self.proto.Clear()
      self.proto.optional_int64 = i
      # Add one to the varint size for the tag info
      # for tag 1.
      self.assertEqual(expected_varint_size + 1, self.Size())
    Test(0, 1)
    Test(1, 1)
    for i, num_bytes in zip(range(7, 63, 7), range(1, 10000)):
      Test((1 << i) - 1, num_bytes)
    Test(-1, 10)
    Test(-2, 10)
    Test(-(1 << 63), 10)

  def testStrings(self):
    self.proto.optional_string = ''
    # Need one byte for tag info (tag #14), and one byte for length.
    self.assertEqual(2, self.Size())

    self.proto.optional_string = 'abc'
    # Need one byte for tag info (tag #14), and one byte for length.
    self.assertEqual(2 + len(self.proto.optional_string), self.Size())

    self.proto.optional_string = 'x' * 128
    # Need one byte for tag info (tag #14), and TWO bytes for length.
    self.assertEqual(3 + len(self.proto.optional_string), self.Size())

  def testOtherNumerics(self):
    self.proto.optional_fixed32 = 1234
    # One byte for tag and 4 bytes for fixed32.
    self.assertEqual(5, self.Size())
    self.proto = unittest_pb2.TestAllTypes()

    self.proto.optional_fixed64 = 1234
    # One byte for tag and 8 bytes for fixed64.
    self.assertEqual(9, self.Size())
    self.proto = unittest_pb2.TestAllTypes()

    self.proto.optional_float = 1.234
    # One byte for tag and 4 bytes for float.
    self.assertEqual(5, self.Size())
    self.proto = unittest_pb2.TestAllTypes()

    self.proto.optional_double = 1.234
    # One byte for tag and 8 bytes for float.
    self.assertEqual(9, self.Size())
    self.proto = unittest_pb2.TestAllTypes()

    self.proto.optional_sint32 = 64
    # One byte for tag and 2 bytes for zig-zag-encoded 64.
    self.assertEqual(3, self.Size())
    self.proto = unittest_pb2.TestAllTypes()

  def testComposites(self):
    # 3 bytes.
    self.proto.optional_nested_message.bb = (1 << 14)
    # Plus one byte for bb tag.
    # Plus 1 byte for optional_nested_message serialized size.
    # Plus two bytes for optional_nested_message tag.
    self.assertEqual(3 + 1 + 1 + 2, self.Size())

  def testGroups(self):
    # 4 bytes.
    self.proto.optionalgroup.a = (1 << 21)
    # Plus two bytes for |a| tag.
    # Plus 2 * two bytes for START_GROUP and END_GROUP tags.
    self.assertEqual(4 + 2 + 2*2, self.Size())

  def testRepeatedScalars(self):
    self.proto.repeated_int32.append(10)  # 1 byte.
    self.proto.repeated_int32.append(128)  # 2 bytes.
    # Also need 2 bytes for each entry for tag.
    self.assertEqual(1 + 2 + 2*2, self.Size())

  def testRepeatedScalarsExtend(self):
    self.proto.repeated_int32.extend([10, 128])  # 3 bytes.
    # Also need 2 bytes for each entry for tag.
    self.assertEqual(1 + 2 + 2*2, self.Size())

  def testRepeatedScalarsRemove(self):
    self.proto.repeated_int32.append(10)  # 1 byte.
    self.proto.repeated_int32.append(128)  # 2 bytes.
    # Also need 2 bytes for each entry for tag.
    self.assertEqual(1 + 2 + 2*2, self.Size())
    self.proto.repeated_int32.remove(128)
    self.assertEqual(1 + 2, self.Size())

  def testRepeatedComposites(self):
    # Empty message.  2 bytes tag plus 1 byte length.
    foreign_message_0 = self.proto.repeated_nested_message.add()
    # 2 bytes tag plus 1 byte length plus 1 byte bb tag 1 byte int.
    foreign_message_1 = self.proto.repeated_nested_message.add()
    foreign_message_1.bb = 7
    self.assertEqual(2 + 1 + 2 + 1 + 1 + 1, self.Size())

  def testRepeatedCompositesDelete(self):
    # Empty message.  2 bytes tag plus 1 byte length.
    foreign_message_0 = self.proto.repeated_nested_message.add()
    # 2 bytes tag plus 1 byte length plus 1 byte bb tag 1 byte int.
    foreign_message_1 = self.proto.repeated_nested_message.add()
    foreign_message_1.bb = 9
    self.assertEqual(2 + 1 + 2 + 1 + 1 + 1, self.Size())
    repeated_nested_message = copy.deepcopy(
        self.proto.repeated_nested_message)

    # 2 bytes tag plus 1 byte length plus 1 byte bb tag 1 byte int.
    del self.proto.repeated_nested_message[0]
    self.assertEqual(2 + 1 + 1 + 1, self.Size())

    # Now add a new message.
    foreign_message_2 = self.proto.repeated_nested_message.add()
    foreign_message_2.bb = 12

    # 2 bytes tag plus 1 byte length plus 1 byte bb tag 1 byte int.
    # 2 bytes tag plus 1 byte length plus 1 byte bb tag 1 byte int.
    self.assertEqual(2 + 1 + 1 + 1 + 2 + 1 + 1 + 1, self.Size())

    # 2 bytes tag plus 1 byte length plus 1 byte bb tag 1 byte int.
    del self.proto.repeated_nested_message[1]
    self.assertEqual(2 + 1 + 1 + 1, self.Size())

    del self.proto.repeated_nested_message[0]
    self.assertEqual(0, self.Size())

    self.assertEqual(2, len(repeated_nested_message))
    del repeated_nested_message[0:1]
    # TODO: Fix cpp extension bug when delete repeated message.
    if api_implementation.Type() == 'python':
      self.assertEqual(1, len(repeated_nested_message))
    del repeated_nested_message[-1]
    # TODO: Fix cpp extension bug when delete repeated message.
    if api_implementation.Type() == 'python':
      self.assertEqual(0, len(repeated_nested_message))

  def testRepeatedGroups(self):
    # 2-byte START_GROUP plus 2-byte END_GROUP.
    group_0 = self.proto.repeatedgroup.add()
    # 2-byte START_GROUP plus 2-byte |a| tag + 1-byte |a|
    # plus 2-byte END_GROUP.
    group_1 = self.proto.repeatedgroup.add()
    group_1.a =  7
    self.assertEqual(2 + 2 + 2 + 2 + 1 + 2, self.Size())

  def testExtensions(self):
    proto = unittest_pb2.TestAllExtensions()
    self.assertEqual(0, proto.ByteSize())
    extension = unittest_pb2.optional_int32_extension  # Field #1, 1 byte.
    proto.Extensions[extension] = 23
    # 1 byte for tag, 1 byte for value.
    self.assertEqual(2, proto.ByteSize())
    field = unittest_pb2.TestAllTypes.DESCRIPTOR.fields_by_name[
        'optional_int32']
    with self.assertRaises(KeyError):
      proto.Extensions[field] = 23

  def testCacheInvalidationForNonrepeatedScalar(self):
    # Test non-extension.
    self.proto.optional_int32 = 1
    self.assertEqual(2, self.proto.ByteSize())
    self.proto.optional_int32 = 128
    self.assertEqual(3, self.proto.ByteSize())
    self.proto.ClearField('optional_int32')
    self.assertEqual(0, self.proto.ByteSize())

    # Test within extension.
    extension = more_extensions_pb2.optional_int_extension
    self.extended_proto.Extensions[extension] = 1
    self.assertEqual(2, self.extended_proto.ByteSize())
    self.extended_proto.Extensions[extension] = 128
    self.assertEqual(3, self.extended_proto.ByteSize())
    self.extended_proto.ClearExtension(extension)
    self.assertEqual(0, self.extended_proto.ByteSize())

  def testCacheInvalidationForRepeatedScalar(self):
    # Test non-extension.
    self.proto.repeated_int32.append(1)
    self.assertEqual(3, self.proto.ByteSize())
    self.proto.repeated_int32.append(1)
    self.assertEqual(6, self.proto.ByteSize())
    self.proto.repeated_int32[1] = 128
    self.assertEqual(7, self.proto.ByteSize())
    self.proto.ClearField('repeated_int32')
    self.assertEqual(0, self.proto.ByteSize())

    # Test within extension.
    extension = more_extensions_pb2.repeated_int_extension
    repeated = self.extended_proto.Extensions[extension]
    repeated.append(1)
    self.assertEqual(2, self.extended_proto.ByteSize())
    repeated.append(1)
    self.assertEqual(4, self.extended_proto.ByteSize())
    repeated[1] = 128
    self.assertEqual(5, self.extended_proto.ByteSize())
    self.extended_proto.ClearExtension(extension)
    self.assertEqual(0, self.extended_proto.ByteSize())

  def testCacheInvalidationForNonrepeatedMessage(self):
    # Test non-extension.
    self.proto.optional_foreign_message.c = 1
    self.assertEqual(5, self.proto.ByteSize())
    self.proto.optional_foreign_message.c = 128
    self.assertEqual(6, self.proto.ByteSize())
    self.proto.optional_foreign_message.ClearField('c')
    self.assertEqual(3, self.proto.ByteSize())
    self.proto.ClearField('optional_foreign_message')
    self.assertEqual(0, self.proto.ByteSize())

    if api_implementation.Type() == 'python':
      # This is only possible in pure-Python implementation of the API.
      child = self.proto.optional_foreign_message
      self.proto.ClearField('optional_foreign_message')
      child.c = 128
      self.assertEqual(0, self.proto.ByteSize())

    # Test within extension.
    extension = more_extensions_pb2.optional_message_extension
    child = self.extended_proto.Extensions[extension]
    self.assertEqual(0, self.extended_proto.ByteSize())
    child.foreign_message_int = 1
    self.assertEqual(4, self.extended_proto.ByteSize())
    child.foreign_message_int = 128
    self.assertEqual(5, self.extended_proto.ByteSize())
    self.extended_proto.ClearExtension(extension)
    self.assertEqual(0, self.extended_proto.ByteSize())

  def testCacheInvalidationForRepeatedMessage(self):
    # Test non-extension.
    child0 = self.proto.repeated_foreign_message.add()
    self.assertEqual(3, self.proto.ByteSize())
    self.proto.repeated_foreign_message.add()
    self.assertEqual(6, self.proto.ByteSize())
    child0.c = 1
    self.assertEqual(8, self.proto.ByteSize())
    self.proto.ClearField('repeated_foreign_message')
    self.assertEqual(0, self.proto.ByteSize())

    # Test within extension.
    extension = more_extensions_pb2.repeated_message_extension
    child_list = self.extended_proto.Extensions[extension]
    child0 = child_list.add()
    self.assertEqual(2, self.extended_proto.ByteSize())
    child_list.add()
    self.assertEqual(4, self.extended_proto.ByteSize())
    child0.foreign_message_int = 1
    self.assertEqual(6, self.extended_proto.ByteSize())
    child0.ClearField('foreign_message_int')
    self.assertEqual(4, self.extended_proto.ByteSize())
    self.extended_proto.ClearExtension(extension)
    self.assertEqual(0, self.extended_proto.ByteSize())

  def testPackedRepeatedScalars(self):
    self.assertEqual(0, self.packed_proto.ByteSize())

    self.packed_proto.packed_int32.append(10)   # 1 byte.
    self.packed_proto.packed_int32.append(128)  # 2 bytes.
    # The tag is 2 bytes (the field number is 90), and the varint
    # storing the length is 1 byte.
    int_size = 1 + 2 + 3
    self.assertEqual(int_size, self.packed_proto.ByteSize())

    self.packed_proto.packed_double.append(4.2)   # 8 bytes
    self.packed_proto.packed_double.append(3.25)  # 8 bytes
    # 2 more tag bytes, 1 more length byte.
    double_size = 8 + 8 + 3
    self.assertEqual(int_size+double_size, self.packed_proto.ByteSize())

    self.packed_proto.ClearField('packed_int32')
    self.assertEqual(double_size, self.packed_proto.ByteSize())

  def testPackedExtensions(self):
    self.assertEqual(0, self.packed_extended_proto.ByteSize())
    extension = self.packed_extended_proto.Extensions[
        unittest_pb2.packed_fixed32_extension]
    extension.extend([1, 2, 3, 4])   # 16 bytes
    # Tag is 3 bytes.
    self.assertEqual(19, self.packed_extended_proto.ByteSize())


# Issues to be sure to cover include:
#   * Handling of unrecognized tags ("uninterpreted_bytes").
#   * Handling of MessageSets.
#   * Consistent ordering of tags in the wire format,
#     including ordering between extensions and non-extension
#     fields.
#   * Consistent serialization of negative numbers, especially
#     negative int32s.
#   * Handling of empty submessages (with and without "has"
#     bits set).

@testing_refleaks.TestCase
class SerializationTest(unittest.TestCase):

  def testSerializeEmtpyMessage(self):
    first_proto = unittest_pb2.TestAllTypes()
    second_proto = unittest_pb2.TestAllTypes()
    serialized = first_proto.SerializeToString()
    self.assertEqual(first_proto.ByteSize(), len(serialized))
    self.assertEqual(
        len(serialized),
        second_proto.MergeFromString(serialized))
    self.assertEqual(first_proto, second_proto)

  def testSerializeAllFields(self):
    first_proto = unittest_pb2.TestAllTypes()
    second_proto = unittest_pb2.TestAllTypes()
    test_util.SetAllFields(first_proto)
    serialized = first_proto.SerializeToString()
    self.assertEqual(first_proto.ByteSize(), len(serialized))
    self.assertEqual(
        len(serialized),
        second_proto.MergeFromString(serialized))
    self.assertEqual(first_proto, second_proto)

  def testSerializeAllExtensions(self):
    first_proto = unittest_pb2.TestAllExtensions()
    second_proto = unittest_pb2.TestAllExtensions()
    test_util.SetAllExtensions(first_proto)
    serialized = first_proto.SerializeToString()
    self.assertEqual(
        len(serialized),
        second_proto.MergeFromString(serialized))
    self.assertEqual(first_proto, second_proto)

  def testSerializeWithOptionalGroup(self):
    first_proto = unittest_pb2.TestAllTypes()
    second_proto = unittest_pb2.TestAllTypes()
    first_proto.optionalgroup.a = 242
    serialized = first_proto.SerializeToString()
    self.assertEqual(
        len(serialized),
        second_proto.MergeFromString(serialized))
    self.assertEqual(first_proto, second_proto)

  def testSerializeNegativeValues(self):
    first_proto = unittest_pb2.TestAllTypes()

    first_proto.optional_int32 = -1
    first_proto.optional_int64 = -(2 << 40)
    first_proto.optional_sint32 = -3
    first_proto.optional_sint64 = -(4 << 40)
    first_proto.optional_sfixed32 = -5
    first_proto.optional_sfixed64 = -(6 << 40)

    second_proto = unittest_pb2.TestAllTypes.FromString(
        first_proto.SerializeToString())

    self.assertEqual(first_proto, second_proto)

  def testParseTruncated(self):
    # This test is only applicable for the Python implementation of the API.
    if api_implementation.Type() != 'python':
      return

    first_proto = unittest_pb2.TestAllTypes()
    test_util.SetAllFields(first_proto)
    serialized = memoryview(first_proto.SerializeToString())

    for truncation_point in range(len(serialized) + 1):
      try:
        second_proto = unittest_pb2.TestAllTypes()
        unknown_fields = unittest_pb2.TestEmptyMessage()
        pos = second_proto._InternalParse(serialized, 0, truncation_point)
        # If we didn't raise an error then we read exactly the amount expected.
        self.assertEqual(truncation_point, pos)

        # Parsing to unknown fields should not throw if parsing to known fields
        # did not.
        try:
          pos2 = unknown_fields._InternalParse(serialized, 0, truncation_point)
          self.assertEqual(truncation_point, pos2)
        except message.DecodeError:
          self.fail('Parsing unknown fields failed when parsing known fields '
                    'did not.')
      except message.DecodeError:
        # Parsing unknown fields should also fail.
        self.assertRaises(message.DecodeError, unknown_fields._InternalParse,
                          serialized, 0, truncation_point)

  def testCanonicalSerializationOrder(self):
    proto = more_messages_pb2.OutOfOrderFields()
    # These are also their tag numbers.  Even though we're setting these in
    # reverse-tag order AND they're listed in reverse tag-order in the .proto
    # file, they should nonetheless be serialized in tag order.
    proto.optional_sint32 = 5
    proto.Extensions[more_messages_pb2.optional_uint64] = 4
    proto.optional_uint32 = 3
    proto.Extensions[more_messages_pb2.optional_int64] = 2
    proto.optional_int32 = 1
    serialized = proto.SerializeToString()
    self.assertEqual(proto.ByteSize(), len(serialized))
    d = _MiniDecoder(serialized)
    ReadTag = d.ReadFieldNumberAndWireType
    self.assertEqual((1, wire_format.WIRETYPE_VARINT), ReadTag())
    self.assertEqual(1, d.ReadInt32())
    self.assertEqual((2, wire_format.WIRETYPE_VARINT), ReadTag())
    self.assertEqual(2, d.ReadInt64())
    self.assertEqual((3, wire_format.WIRETYPE_VARINT), ReadTag())
    self.assertEqual(3, d.ReadUInt32())
    self.assertEqual((4, wire_format.WIRETYPE_VARINT), ReadTag())
    self.assertEqual(4, d.ReadUInt64())
    self.assertEqual((5, wire_format.WIRETYPE_VARINT), ReadTag())
    self.assertEqual(5, d.ReadSInt32())

  def testCanonicalSerializationOrderSameAsCpp(self):
    # Copy of the same test we use for C++.
    proto = unittest_pb2.TestFieldOrderings()
    test_util.SetAllFieldsAndExtensions(proto)
    serialized = proto.SerializeToString()
    test_util.ExpectAllFieldsAndExtensionsInOrder(serialized)

  def testMergeFromStringWhenFieldsAlreadySet(self):
    first_proto = unittest_pb2.TestAllTypes()
    first_proto.repeated_string.append('foobar')
    first_proto.optional_int32 = 23
    first_proto.optional_nested_message.bb = 42
    serialized = first_proto.SerializeToString()

    second_proto = unittest_pb2.TestAllTypes()
    second_proto.repeated_string.append('baz')
    second_proto.optional_int32 = 100
    second_proto.optional_nested_message.bb = 999

    bytes_parsed = second_proto.MergeFromString(serialized)
    self.assertEqual(len(serialized), bytes_parsed)

    # Ensure that we append to repeated fields.
    self.assertEqual(['baz', 'foobar'], list(second_proto.repeated_string))
    # Ensure that we overwrite nonrepeatd scalars.
    self.assertEqual(23, second_proto.optional_int32)
    # Ensure that we recursively call MergeFromString() on
    # submessages.
    self.assertEqual(42, second_proto.optional_nested_message.bb)

  def testMessageSetWireFormat(self):
    proto = message_set_extensions_pb2.TestMessageSet()
    extension_message1 = message_set_extensions_pb2.TestMessageSetExtension1
    extension_message2 = message_set_extensions_pb2.TestMessageSetExtension2
    extension1 = extension_message1.message_set_extension
    extension2 = extension_message2.message_set_extension
    extension3 = message_set_extensions_pb2.message_set_extension3
    proto.Extensions[extension1].i = 123
    proto.Extensions[extension2].str = 'foo'
    proto.Extensions[extension3].text = 'bar'

    # Serialize using the MessageSet wire format (this is specified in the
    # .proto file).
    serialized = proto.SerializeToString()

    raw = unittest_mset_pb2.RawMessageSet()
    self.assertEqual(False,
                     raw.DESCRIPTOR.GetOptions().message_set_wire_format)
    self.assertEqual(
        len(serialized),
        raw.MergeFromString(serialized))
    self.assertEqual(3, len(raw.item))

    message1 = message_set_extensions_pb2.TestMessageSetExtension1()
    self.assertEqual(
        len(raw.item[0].message),
        message1.MergeFromString(raw.item[0].message))
    self.assertEqual(123, message1.i)

    message2 = message_set_extensions_pb2.TestMessageSetExtension2()
    self.assertEqual(
        len(raw.item[1].message),
        message2.MergeFromString(raw.item[1].message))
    self.assertEqual('foo', message2.str)

    message3 = message_set_extensions_pb2.TestMessageSetExtension3()
    self.assertEqual(
        len(raw.item[2].message),
        message3.MergeFromString(raw.item[2].message))
    self.assertEqual('bar', message3.text)

    # Deserialize using the MessageSet wire format.
    proto2 = message_set_extensions_pb2.TestMessageSet()
    self.assertEqual(
        len(serialized),
        proto2.MergeFromString(serialized))
    self.assertEqual(123, proto2.Extensions[extension1].i)
    self.assertEqual('foo', proto2.Extensions[extension2].str)
    self.assertEqual('bar', proto2.Extensions[extension3].text)

    # Check byte size.
    self.assertEqual(proto2.ByteSize(), len(serialized))
    self.assertEqual(proto.ByteSize(), len(serialized))

  def testMessageSetWireFormatUnknownExtension(self):
    # Create a message using the message set wire format with an unknown
    # message.
    raw = unittest_mset_pb2.RawMessageSet()

    # Add an item.
    item = raw.item.add()
    item.type_id = 98418603
    extension_message1 = message_set_extensions_pb2.TestMessageSetExtension1
    message1 = message_set_extensions_pb2.TestMessageSetExtension1()
    message1.i = 12345
    item.message = message1.SerializeToString()

    # Add a second, unknown extension.
    item = raw.item.add()
    item.type_id = 98418604
    extension_message1 = message_set_extensions_pb2.TestMessageSetExtension1
    message1 = message_set_extensions_pb2.TestMessageSetExtension1()
    message1.i = 12346
    item.message = message1.SerializeToString()

    # Add another unknown extension.
    item = raw.item.add()
    item.type_id = 98418605
    message1 = message_set_extensions_pb2.TestMessageSetExtension2()
    message1.str = 'foo'
    item.message = message1.SerializeToString()

    serialized = raw.SerializeToString()

    # Parse message using the message set wire format.
    proto = message_set_extensions_pb2.TestMessageSet()
    self.assertEqual(
        len(serialized),
        proto.MergeFromString(serialized))

    # Check that the message parsed well.
    extension_message1 = message_set_extensions_pb2.TestMessageSetExtension1
    extension1 = extension_message1.message_set_extension
    self.assertEqual(12345, proto.Extensions[extension1].i)

  def testUnknownFields(self):
    proto = unittest_pb2.TestAllTypes()
    test_util.SetAllFields(proto)

    serialized = proto.SerializeToString()

    # The empty message should be parsable with all of the fields
    # unknown.
    proto2 = unittest_pb2.TestEmptyMessage()

    # Parsing this message should succeed.
    self.assertEqual(
        len(serialized),
        proto2.MergeFromString(serialized))

    # Now test with a int64 field set.
    proto = unittest_pb2.TestAllTypes()
    proto.optional_int64 = 0x0fffffffffffffff
    serialized = proto.SerializeToString()
    # The empty message should be parsable with all of the fields
    # unknown.
    proto2 = unittest_pb2.TestEmptyMessage()
    # Parsing this message should succeed.
    self.assertEqual(
        len(serialized),
        proto2.MergeFromString(serialized))

  def _CheckRaises(self, exc_class, callable_obj, exception):
    """This method checks if the exception type and message are as expected."""
    try:
      callable_obj()
    except exc_class as ex:
      # Check if the exception message is the right one.
      self.assertEqual(exception, str(ex))
      return
    else:
      raise self.failureException('%s not raised' % str(exc_class))

  def testSerializeUninitialized(self):
    proto = unittest_pb2.TestRequired()
    self._CheckRaises(
        message.EncodeError,
        proto.SerializeToString,
        'Message protobuf_unittest.TestRequired is missing required fields: '
        'a,b,c')
    # Shouldn't raise exceptions.
    partial = proto.SerializePartialToString()

    proto2 = unittest_pb2.TestRequired()
    self.assertFalse(proto2.HasField('a'))
    # proto2 ParseFromString does not check that required fields are set.
    proto2.ParseFromString(partial)
    self.assertFalse(proto2.HasField('a'))

    proto.a = 1
    self._CheckRaises(
        message.EncodeError,
        proto.SerializeToString,
        'Message protobuf_unittest.TestRequired is missing required fields: b,c')
    # Shouldn't raise exceptions.
    partial = proto.SerializePartialToString()

    proto.b = 2
    self._CheckRaises(
        message.EncodeError,
        proto.SerializeToString,
        'Message protobuf_unittest.TestRequired is missing required fields: c')
    # Shouldn't raise exceptions.
    partial = proto.SerializePartialToString()

    proto.c = 3
    serialized = proto.SerializeToString()
    # Shouldn't raise exceptions.
    partial = proto.SerializePartialToString()

    proto2 = unittest_pb2.TestRequired()
    self.assertEqual(
        len(serialized),
        proto2.MergeFromString(serialized))
    self.assertEqual(1, proto2.a)
    self.assertEqual(2, proto2.b)
    self.assertEqual(3, proto2.c)
    self.assertEqual(
        len(partial),
        proto2.MergeFromString(partial))
    self.assertEqual(1, proto2.a)
    self.assertEqual(2, proto2.b)
    self.assertEqual(3, proto2.c)

  def testSerializeUninitializedSubMessage(self):
    proto = unittest_pb2.TestRequiredForeign()

    # Sub-message doesn't exist yet, so this succeeds.
    proto.SerializeToString()

    proto.optional_message.a = 1
    self._CheckRaises(
        message.EncodeError,
        proto.SerializeToString,
        'Message protobuf_unittest.TestRequiredForeign '
        'is missing required fields: '
        'optional_message.b,optional_message.c')

    proto.optional_message.b = 2
    proto.optional_message.c = 3
    proto.SerializeToString()

    proto.repeated_message.add().a = 1
    proto.repeated_message.add().b = 2
    self._CheckRaises(
        message.EncodeError,
        proto.SerializeToString,
        'Message protobuf_unittest.TestRequiredForeign is missing required fields: '
        'repeated_message[0].b,repeated_message[0].c,'
        'repeated_message[1].a,repeated_message[1].c')

    proto.repeated_message[0].b = 2
    proto.repeated_message[0].c = 3
    proto.repeated_message[1].a = 1
    proto.repeated_message[1].c = 3
    proto.SerializeToString()

  def testSerializeAllPackedFields(self):
    first_proto = unittest_pb2.TestPackedTypes()
    second_proto = unittest_pb2.TestPackedTypes()
    test_util.SetAllPackedFields(first_proto)
    serialized = first_proto.SerializeToString()
    self.assertEqual(first_proto.ByteSize(), len(serialized))
    bytes_read = second_proto.MergeFromString(serialized)
    self.assertEqual(second_proto.ByteSize(), bytes_read)
    self.assertEqual(first_proto, second_proto)

  def testSerializeAllPackedExtensions(self):
    first_proto = unittest_pb2.TestPackedExtensions()
    second_proto = unittest_pb2.TestPackedExtensions()
    test_util.SetAllPackedExtensions(first_proto)
    serialized = first_proto.SerializeToString()
    bytes_read = second_proto.MergeFromString(serialized)
    self.assertEqual(second_proto.ByteSize(), bytes_read)
    self.assertEqual(first_proto, second_proto)

  def testMergePackedFromStringWhenSomeFieldsAlreadySet(self):
    first_proto = unittest_pb2.TestPackedTypes()
    first_proto.packed_int32.extend([1, 2])
    first_proto.packed_double.append(3.0)
    serialized = first_proto.SerializeToString()

    second_proto = unittest_pb2.TestPackedTypes()
    second_proto.packed_int32.append(3)
    second_proto.packed_double.extend([1.0, 2.0])
    second_proto.packed_sint32.append(4)

    self.assertEqual(
        len(serialized),
        second_proto.MergeFromString(serialized))
    self.assertEqual([3, 1, 2], second_proto.packed_int32)
    self.assertEqual([1.0, 2.0, 3.0], second_proto.packed_double)
    self.assertEqual([4], second_proto.packed_sint32)

  def testPackedFieldsWireFormat(self):
    proto = unittest_pb2.TestPackedTypes()
    proto.packed_int32.extend([1, 2, 150, 3])  # 1 + 1 + 2 + 1 bytes
    proto.packed_double.extend([1.0, 1000.0])  # 8 + 8 bytes
    proto.packed_float.append(2.0)             # 4 bytes, will be before double
    serialized = proto.SerializeToString()
    self.assertEqual(proto.ByteSize(), len(serialized))
    d = _MiniDecoder(serialized)
    ReadTag = d.ReadFieldNumberAndWireType
    self.assertEqual((90, wire_format.WIRETYPE_LENGTH_DELIMITED), ReadTag())
    self.assertEqual(1+1+1+2, d.ReadInt32())
    self.assertEqual(1, d.ReadInt32())
    self.assertEqual(2, d.ReadInt32())
    self.assertEqual(150, d.ReadInt32())
    self.assertEqual(3, d.ReadInt32())
    self.assertEqual((100, wire_format.WIRETYPE_LENGTH_DELIMITED), ReadTag())
    self.assertEqual(4, d.ReadInt32())
    self.assertEqual(2.0, d.ReadFloat())
    self.assertEqual((101, wire_format.WIRETYPE_LENGTH_DELIMITED), ReadTag())
    self.assertEqual(8+8, d.ReadInt32())
    self.assertEqual(1.0, d.ReadDouble())
    self.assertEqual(1000.0, d.ReadDouble())
    self.assertTrue(d.EndOfStream())

  def testParsePackedFromUnpacked(self):
    unpacked = unittest_pb2.TestUnpackedTypes()
    test_util.SetAllUnpackedFields(unpacked)
    packed = unittest_pb2.TestPackedTypes()
    serialized = unpacked.SerializeToString()
    self.assertEqual(
        len(serialized),
        packed.MergeFromString(serialized))
    expected = unittest_pb2.TestPackedTypes()
    test_util.SetAllPackedFields(expected)
    self.assertEqual(expected, packed)

  def testParseUnpackedFromPacked(self):
    packed = unittest_pb2.TestPackedTypes()
    test_util.SetAllPackedFields(packed)
    unpacked = unittest_pb2.TestUnpackedTypes()
    serialized = packed.SerializeToString()
    self.assertEqual(
        len(serialized),
        unpacked.MergeFromString(serialized))
    expected = unittest_pb2.TestUnpackedTypes()
    test_util.SetAllUnpackedFields(expected)
    self.assertEqual(expected, unpacked)

  def testFieldNumbers(self):
    proto = unittest_pb2.TestAllTypes()
    self.assertEqual(unittest_pb2.TestAllTypes.NestedMessage.BB_FIELD_NUMBER, 1)
    self.assertEqual(unittest_pb2.TestAllTypes.OPTIONAL_INT32_FIELD_NUMBER, 1)
    self.assertEqual(unittest_pb2.TestAllTypes.OPTIONALGROUP_FIELD_NUMBER, 16)
    self.assertEqual(
      unittest_pb2.TestAllTypes.OPTIONAL_NESTED_MESSAGE_FIELD_NUMBER, 18)
    self.assertEqual(
      unittest_pb2.TestAllTypes.OPTIONAL_NESTED_ENUM_FIELD_NUMBER, 21)
    self.assertEqual(unittest_pb2.TestAllTypes.REPEATED_INT32_FIELD_NUMBER, 31)
    self.assertEqual(unittest_pb2.TestAllTypes.REPEATEDGROUP_FIELD_NUMBER, 46)
    self.assertEqual(
      unittest_pb2.TestAllTypes.REPEATED_NESTED_MESSAGE_FIELD_NUMBER, 48)
    self.assertEqual(
      unittest_pb2.TestAllTypes.REPEATED_NESTED_ENUM_FIELD_NUMBER, 51)

  def testExtensionFieldNumbers(self):
    self.assertEqual(unittest_pb2.TestRequired.single.number, 1000)
    self.assertEqual(unittest_pb2.TestRequired.SINGLE_FIELD_NUMBER, 1000)
    self.assertEqual(unittest_pb2.TestRequired.multi.number, 1001)
    self.assertEqual(unittest_pb2.TestRequired.MULTI_FIELD_NUMBER, 1001)
    self.assertEqual(unittest_pb2.optional_int32_extension.number, 1)
    self.assertEqual(unittest_pb2.OPTIONAL_INT32_EXTENSION_FIELD_NUMBER, 1)
    self.assertEqual(unittest_pb2.optionalgroup_extension.number, 16)
    self.assertEqual(unittest_pb2.OPTIONALGROUP_EXTENSION_FIELD_NUMBER, 16)
    self.assertEqual(unittest_pb2.optional_nested_message_extension.number, 18)
    self.assertEqual(
      unittest_pb2.OPTIONAL_NESTED_MESSAGE_EXTENSION_FIELD_NUMBER, 18)
    self.assertEqual(unittest_pb2.optional_nested_enum_extension.number, 21)
    self.assertEqual(unittest_pb2.OPTIONAL_NESTED_ENUM_EXTENSION_FIELD_NUMBER,
      21)
    self.assertEqual(unittest_pb2.repeated_int32_extension.number, 31)
    self.assertEqual(unittest_pb2.REPEATED_INT32_EXTENSION_FIELD_NUMBER, 31)
    self.assertEqual(unittest_pb2.repeatedgroup_extension.number, 46)
    self.assertEqual(unittest_pb2.REPEATEDGROUP_EXTENSION_FIELD_NUMBER, 46)
    self.assertEqual(unittest_pb2.repeated_nested_message_extension.number, 48)
    self.assertEqual(
      unittest_pb2.REPEATED_NESTED_MESSAGE_EXTENSION_FIELD_NUMBER, 48)
    self.assertEqual(unittest_pb2.repeated_nested_enum_extension.number, 51)
    self.assertEqual(unittest_pb2.REPEATED_NESTED_ENUM_EXTENSION_FIELD_NUMBER,
      51)

  def testFieldProperties(self):
    cls = unittest_pb2.TestAllTypes
    self.assertIs(cls.optional_int32.DESCRIPTOR,
                  cls.DESCRIPTOR.fields_by_name['optional_int32'])
    self.assertEqual(cls.OPTIONAL_INT32_FIELD_NUMBER,
                     cls.optional_int32.DESCRIPTOR.number)
    self.assertIs(cls.optional_nested_message.DESCRIPTOR,
                  cls.DESCRIPTOR.fields_by_name['optional_nested_message'])
    self.assertEqual(cls.OPTIONAL_NESTED_MESSAGE_FIELD_NUMBER,
                     cls.optional_nested_message.DESCRIPTOR.number)
    self.assertIs(cls.repeated_int32.DESCRIPTOR,
                  cls.DESCRIPTOR.fields_by_name['repeated_int32'])
    self.assertEqual(cls.REPEATED_INT32_FIELD_NUMBER,
                     cls.repeated_int32.DESCRIPTOR.number)

  def testFieldDataDescriptor(self):
    msg = unittest_pb2.TestAllTypes()
    msg.optional_int32 = 42
    self.assertEqual(unittest_pb2.TestAllTypes.optional_int32.__get__(msg), 42)
    unittest_pb2.TestAllTypes.optional_int32.__set__(msg, 25)
    self.assertEqual(msg.optional_int32, 25)
    with self.assertRaises(AttributeError):
      del msg.optional_int32
    try:
      unittest_pb2.ForeignMessage.c.__get__(msg)
    except TypeError:
      pass  # The cpp implementation cannot mix fields from other messages.
      # This test exercises a specific check that avoids a crash.
    else:
      pass  # The python implementation allows fields from other messages.
      # This is useless, but works.

  def testInitKwargs(self):
    proto = unittest_pb2.TestAllTypes(
        optional_int32=1,
        optional_string='foo',
        optional_bool=True,
        optional_bytes=b'bar',
        optional_nested_message=unittest_pb2.TestAllTypes.NestedMessage(bb=1),
        optional_foreign_message=unittest_pb2.ForeignMessage(c=1),
        optional_nested_enum=unittest_pb2.TestAllTypes.FOO,
        optional_foreign_enum=unittest_pb2.FOREIGN_FOO,
        repeated_int32=[1, 2, 3])
    self.assertTrue(proto.IsInitialized())
    self.assertTrue(proto.HasField('optional_int32'))
    self.assertTrue(proto.HasField('optional_string'))
    self.assertTrue(proto.HasField('optional_bool'))
    self.assertTrue(proto.HasField('optional_bytes'))
    self.assertTrue(proto.HasField('optional_nested_message'))
    self.assertTrue(proto.HasField('optional_foreign_message'))
    self.assertTrue(proto.HasField('optional_nested_enum'))
    self.assertTrue(proto.HasField('optional_foreign_enum'))
    self.assertEqual(1, proto.optional_int32)
    self.assertEqual('foo', proto.optional_string)
    self.assertEqual(True, proto.optional_bool)
    self.assertEqual(b'bar', proto.optional_bytes)
    self.assertEqual(1, proto.optional_nested_message.bb)
    self.assertEqual(1, proto.optional_foreign_message.c)
    self.assertEqual(unittest_pb2.TestAllTypes.FOO,
                     proto.optional_nested_enum)
    self.assertEqual(unittest_pb2.FOREIGN_FOO, proto.optional_foreign_enum)
    self.assertEqual([1, 2, 3], proto.repeated_int32)

  def testInitArgsUnknownFieldName(self):
    def InitalizeEmptyMessageWithExtraKeywordArg():
      unused_proto = unittest_pb2.TestEmptyMessage(unknown='unknown')
    self._CheckRaises(
        ValueError,
        InitalizeEmptyMessageWithExtraKeywordArg,
        'Protocol message TestEmptyMessage has no "unknown" field.')

  def testInitRequiredKwargs(self):
    proto = unittest_pb2.TestRequired(a=1, b=1, c=1)
    self.assertTrue(proto.IsInitialized())
    self.assertTrue(proto.HasField('a'))
    self.assertTrue(proto.HasField('b'))
    self.assertTrue(proto.HasField('c'))
    self.assertFalse(proto.HasField('dummy2'))
    self.assertEqual(1, proto.a)
    self.assertEqual(1, proto.b)
    self.assertEqual(1, proto.c)

  def testInitRequiredForeignKwargs(self):
    proto = unittest_pb2.TestRequiredForeign(
        optional_message=unittest_pb2.TestRequired(a=1, b=1, c=1))
    self.assertTrue(proto.IsInitialized())
    self.assertTrue(proto.HasField('optional_message'))
    self.assertTrue(proto.optional_message.IsInitialized())
    self.assertTrue(proto.optional_message.HasField('a'))
    self.assertTrue(proto.optional_message.HasField('b'))
    self.assertTrue(proto.optional_message.HasField('c'))
    self.assertFalse(proto.optional_message.HasField('dummy2'))
    self.assertEqual(unittest_pb2.TestRequired(a=1, b=1, c=1),
                     proto.optional_message)
    self.assertEqual(1, proto.optional_message.a)
    self.assertEqual(1, proto.optional_message.b)
    self.assertEqual(1, proto.optional_message.c)

  def testInitRepeatedKwargs(self):
    proto = unittest_pb2.TestAllTypes(repeated_int32=[1, 2, 3])
    self.assertTrue(proto.IsInitialized())
    self.assertEqual(1, proto.repeated_int32[0])
    self.assertEqual(2, proto.repeated_int32[1])
    self.assertEqual(3, proto.repeated_int32[2])


@testing_refleaks.TestCase
class OptionsTest(unittest.TestCase):

  def testMessageOptions(self):
    proto = message_set_extensions_pb2.TestMessageSet()
    self.assertEqual(True,
                     proto.DESCRIPTOR.GetOptions().message_set_wire_format)
    proto = unittest_pb2.TestAllTypes()
    self.assertEqual(False,
                     proto.DESCRIPTOR.GetOptions().message_set_wire_format)

  def testPackedOptions(self):
    proto = unittest_pb2.TestAllTypes()
    proto.optional_int32 = 1
    proto.optional_double = 3.0
    for field_descriptor, _ in proto.ListFields():
      self.assertEqual(False, field_descriptor.GetOptions().packed)

    proto = unittest_pb2.TestPackedTypes()
    proto.packed_int32.append(1)
    proto.packed_double.append(3.0)
    for field_descriptor, _ in proto.ListFields():
      self.assertEqual(True, field_descriptor.GetOptions().packed)
      self.assertEqual(descriptor.FieldDescriptor.LABEL_REPEATED,
                       field_descriptor.label)


@testing_refleaks.TestCase
class ClassAPITest(unittest.TestCase):

  @unittest.skipIf(
      api_implementation.Type() != 'python',
      'C++ implementation requires a call to MakeDescriptor()')
  @testing_refleaks.SkipReferenceLeakChecker('MakeClass is not repeatable')
  def testMakeClassWithNestedDescriptor(self):
    leaf_desc = descriptor.Descriptor(
        'leaf', 'package.parent.child.leaf', '',
        containing_type=None, fields=[],
        nested_types=[], enum_types=[],
        extensions=[],
        # pylint: disable=protected-access
        create_key=descriptor._internal_create_key)
    child_desc = descriptor.Descriptor(
        'child', 'package.parent.child', '',
        containing_type=None, fields=[],
        nested_types=[leaf_desc], enum_types=[],
        extensions=[],
        # pylint: disable=protected-access
        create_key=descriptor._internal_create_key)
    sibling_desc = descriptor.Descriptor(
        'sibling', 'package.parent.sibling',
        '', containing_type=None, fields=[],
        nested_types=[], enum_types=[],
        extensions=[],
        # pylint: disable=protected-access
        create_key=descriptor._internal_create_key)
    parent_desc = descriptor.Descriptor(
        'parent', 'package.parent', '',
        containing_type=None, fields=[],
        nested_types=[child_desc, sibling_desc],
        enum_types=[], extensions=[],
        # pylint: disable=protected-access
        create_key=descriptor._internal_create_key)
    reflection.MakeClass(parent_desc)

  def _GetSerializedFileDescriptor(self, name):
    """Get a serialized representation of a test FileDescriptorProto.

    Args:
      name: All calls to this must use a unique message name, to avoid
          collisions in the cpp descriptor pool.
    Returns:
      A string containing the serialized form of a test FileDescriptorProto.
    """
    file_descriptor_str = (
        'message_type {'
        '  name: "' + name + '"'
        '  field {'
        '    name: "flat"'
        '    number: 1'
        '    label: LABEL_REPEATED'
        '    type: TYPE_UINT32'
        '  }'
        '  field {'
        '    name: "bar"'
        '    number: 2'
        '    label: LABEL_OPTIONAL'
        '    type: TYPE_MESSAGE'
        '    type_name: "Bar"'
        '  }'
        '  nested_type {'
        '    name: "Bar"'
        '    field {'
        '      name: "baz"'
        '      number: 3'
        '      label: LABEL_OPTIONAL'
        '      type: TYPE_MESSAGE'
        '      type_name: "Baz"'
        '    }'
        '    nested_type {'
        '      name: "Baz"'
        '      enum_type {'
        '        name: "deep_enum"'
        '        value {'
        '          name: "VALUE_A"'
        '          number: 0'
        '        }'
        '      }'
        '      field {'
        '        name: "deep"'
        '        number: 4'
        '        label: LABEL_OPTIONAL'
        '        type: TYPE_UINT32'
        '      }'
        '    }'
        '  }'
        '}')
    file_descriptor = descriptor_pb2.FileDescriptorProto()
    text_format.Merge(file_descriptor_str, file_descriptor)
    return file_descriptor.SerializeToString()

  @testing_refleaks.SkipReferenceLeakChecker('MakeDescriptor is not repeatable')
  # This test can only run once; the second time, it raises errors about
  # conflicting message descriptors.
  def testParsingFlatClassWithExplicitClassDeclaration(self):
    """Test that the generated class can parse a flat message."""
    # TODO: This test fails with cpp implementation in the call
    # of six.with_metaclass(). The other two callsites of with_metaclass
    # in this file are both excluded from cpp test, so it might be expected
    # to fail. Need someone more familiar with the python code to take a
    # look at this.
    if api_implementation.Type() != 'python':
      return
    file_descriptor = descriptor_pb2.FileDescriptorProto()
    file_descriptor.ParseFromString(self._GetSerializedFileDescriptor('A'))
    msg_descriptor = descriptor.MakeDescriptor(
        file_descriptor.message_type[0])

    class MessageClass(
        message.Message, metaclass=reflection.GeneratedProtocolMessageType):
      DESCRIPTOR = msg_descriptor
    msg = MessageClass()
    msg_str = (
        'flat: 0 '
        'flat: 1 '
        'flat: 2 ')
    text_format.Merge(msg_str, msg)
    self.assertEqual(msg.flat, [0, 1, 2])

  @testing_refleaks.SkipReferenceLeakChecker('MakeDescriptor is not repeatable')
  def testParsingFlatClass(self):
    """Test that the generated class can parse a flat message."""
    file_descriptor = descriptor_pb2.FileDescriptorProto()
    file_descriptor.ParseFromString(self._GetSerializedFileDescriptor('B'))
    msg_descriptor = descriptor.MakeDescriptor(
        file_descriptor.message_type[0])
    msg_class = reflection.MakeClass(msg_descriptor)
    msg = msg_class()
    msg_str = (
        'flat: 0 '
        'flat: 1 '
        'flat: 2 ')
    text_format.Merge(msg_str, msg)
    self.assertEqual(msg.flat, [0, 1, 2])

  @testing_refleaks.SkipReferenceLeakChecker('MakeDescriptor is not repeatable')
  def testParsingNestedClass(self):
    """Test that the generated class can parse a nested message."""
    file_descriptor = descriptor_pb2.FileDescriptorProto()
    file_descriptor.ParseFromString(self._GetSerializedFileDescriptor('C'))
    msg_descriptor = descriptor.MakeDescriptor(
        file_descriptor.message_type[0])
    msg_class = reflection.MakeClass(msg_descriptor)
    msg = msg_class()
    msg_str = (
        'bar {'
        '  baz {'
        '    deep: 4'
        '  }'
        '}')
    text_format.Merge(msg_str, msg)
    self.assertEqual(msg.bar.baz.deep, 4)

if __name__ == '__main__':
  unittest.main()
