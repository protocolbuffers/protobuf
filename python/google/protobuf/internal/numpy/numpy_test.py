# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Test use of numpy types with repeated and non-repeated scalar fields."""

from datetime import datetime
import unittest

from google.protobuf.internal import api_implementation
from google.protobuf.internal import testing_refleaks
import numpy as np

from absl.testing import parameterized
from google.protobuf import unittest_pb2
from google.protobuf import unittest_proto3_arena_pb2
from google.protobuf.util import json_format_pb2

message = unittest_pb2.TestAllTypes()
np_float_scalar = np.float64(0.0)
np_1_float_array = np.zeros(shape=(1,), dtype=np.float64)
np_2_float_array = np.zeros(shape=(2,), dtype=np.float64)
np_11_float_array = np.zeros(shape=(1, 1), dtype=np.float64)
np_22_float_array = np.zeros(shape=(2, 2), dtype=np.float64)

np_int_scalar = np.int64(0)
np_1_int_array = np.zeros(shape=(1,), dtype=np.int64)
np_2_int_array = np.zeros(shape=(2,), dtype=np.int64)
np_11_int_array = np.zeros(shape=(1, 1), dtype=np.int64)
np_22_int_array = np.zeros(shape=(2, 2), dtype=np.int64)

np_uint_scalar = np.uint64(0)
np_1_uint_array = np.zeros(shape=(1,), dtype=np.uint64)
np_2_uint_array = np.zeros(shape=(2,), dtype=np.uint64)
np_11_uint_array = np.zeros(shape=(1, 1), dtype=np.uint64)
np_22_uint_array = np.zeros(shape=(2, 2), dtype=np.uint64)

np_bool_scalar = np.bool_(False)
np_1_bool_array = np.zeros(shape=(1,), dtype=np.bool_)
np_2_bool_array = np.zeros(shape=(2,), dtype=np.bool_)
np_11_bool_array = np.zeros(shape=(1, 1), dtype=np.bool_)
np_22_bool_array = np.zeros(shape=(2, 2), dtype=np.bool_)


@testing_refleaks.TestCase
class NumpyIntProtoTest(unittest.TestCase):

  # Assigning dim 1 ndarray of ints to repeated field should pass
  def testNumpyDim1IntArrayToRepeated_IsValid(self):
    message.repeated_int64[:] = np_1_int_array
    message.repeated_int64[:] = np_2_int_array

    message.repeated_uint64[:] = np_1_uint_array
    message.repeated_uint64[:] = np_2_uint_array

  # Assigning dim 2 ndarray of ints to repeated field should fail
  def testNumpyDim2IntArrayToRepeated_RaisesTypeError(self):
    with self.assertRaises(TypeError):
      message.repeated_int64[:] = np_11_int_array
    with self.assertRaises(TypeError):
      message.repeated_int64[:] = np_22_int_array

    with self.assertRaises(TypeError):
      message.repeated_uint64[:] = np_11_uint_array
    with self.assertRaises(TypeError):
      message.repeated_uint64[:] = np_22_uint_array

  # Assigning any ndarray of floats to repeated int field should fail
  def testNumpyFloatArrayToRepeated_RaisesTypeError(self):
    with self.assertRaises(TypeError):
      message.repeated_int64[:] = np_1_float_array
    with self.assertRaises(TypeError):
      message.repeated_int64[:] = np_11_float_array
    with self.assertRaises(TypeError):
      message.repeated_int64[:] = np_22_float_array

  # Assigning any np int to scalar field should pass
  def testNumpyIntScalarToScalar_IsValid(self):
    message.optional_int64 = np_int_scalar
    message.optional_uint64 = np_uint_scalar

  # Assigning any ndarray of ints to scalar field should fail
  def testNumpyIntArrayToScalar_RaisesTypeError(self):
    with self.assertRaises(TypeError):
      message.optional_int64 = np_1_int_array
    with self.assertRaises(TypeError):
      message.optional_int64 = np_11_int_array
    with self.assertRaises(TypeError):
      message.optional_int64 = np_22_int_array

    with self.assertRaises(TypeError):
      message.optional_uint64 = np_1_uint_array
    with self.assertRaises(TypeError):
      message.optional_uint64 = np_11_uint_array
    with self.assertRaises(TypeError):
      message.optional_uint64 = np_22_uint_array

  # Assigning any ndarray of floats to scalar field should fail
  def testNumpyFloatArrayToScalar_RaisesTypeError(self):
    with self.assertRaises(TypeError):
      message.optional_int64 = np_1_float_array
    with self.assertRaises(TypeError):
      message.optional_int64 = np_11_float_array
    with self.assertRaises(TypeError):
      message.optional_int64 = np_22_float_array


@testing_refleaks.TestCase
class NumpyFloatProtoTest(unittest.TestCase):

  # Assigning dim 1 ndarray of floats to repeated field should pass
  def testNumpyDim1FloatArrayToRepeated_IsValid(self):
    message.repeated_float[:] = np_1_float_array
    message.repeated_float[:] = np_2_float_array

  # Assigning dim 2 ndarray of floats to repeated field should fail
  def testNumpyDim2FloatArrayToRepeated_RaisesTypeError(self):
    with self.assertRaises(TypeError):
      message.repeated_float[:] = np_11_float_array
    with self.assertRaises(TypeError):
      message.repeated_float[:] = np_22_float_array

  # Assigning any np float to scalar field should pass
  def testNumpyFloatScalarToScalar_IsValid(self):
    message.optional_float = np_float_scalar

  # Assigning any ndarray of float to scalar field should fail
  def testNumpyFloatArrayToScalar_RaisesTypeError(self):
    with self.assertRaises(TypeError):
      message.optional_float = np_1_float_array
    with self.assertRaises(TypeError):
      message.optional_float = np_11_float_array
    with self.assertRaises(TypeError):
      message.optional_float = np_22_float_array


@testing_refleaks.TestCase
class NumpyBoolProtoTest(unittest.TestCase):

  # Assigning dim 1 ndarray of bool to repeated field should pass
  def testNumpyDim1BoolArrayToRepeated_IsValid(self):
    message.repeated_bool[:] = np_1_bool_array
    message.repeated_bool[:] = np_2_bool_array

  # Assigning dim 2 ndarray of bool to repeated field should fail
  def testNumpyDim2BoolArrayToRepeated_RaisesTypeError(self):
    with self.assertRaises(TypeError):
      message.repeated_bool[:] = np_11_bool_array
    with self.assertRaises(TypeError):
      message.repeated_bool[:] = np_22_bool_array

  # Assigning any np bool to scalar field should pass
  def testNumpyBoolScalarToScalar_IsValid(self):
    message.optional_bool = np_bool_scalar

  # Assigning any ndarray of bool to scalar field should fail
  def testNumpyBoolArrayToScalar_RaisesTypeError(self):
    with self.assertRaises(TypeError):
      message.optional_bool = np_1_bool_array
    with self.assertRaises(TypeError):
      message.optional_bool = np_11_bool_array
    with self.assertRaises(TypeError):
      message.optional_bool = np_22_bool_array


@testing_refleaks.TestCase
class NumpyProtoIndexingTest(unittest.TestCase):

  def testNumpyIntScalarIndexing_Passes(self):
    data = unittest_pb2.TestAllTypes(repeated_int64=[0, 1, 2])
    self.assertEqual(0, data.repeated_int64[np.int64(0)])

  def testNumpyNegative1IntScalarIndexing_Passes(self):
    data = unittest_pb2.TestAllTypes(repeated_int64=[0, 1, 2])
    self.assertEqual(2, data.repeated_int64[np.int64(-1)])

  def testNumpyFloatScalarIndexing_Fails(self):
    data = unittest_pb2.TestAllTypes(repeated_int64=[0, 1, 2])
    with self.assertRaises(TypeError):
      _ = data.repeated_int64[np.float64(0.0)]

  def testNumpyIntArrayIndexing_Fails(self):
    data = unittest_pb2.TestAllTypes(repeated_int64=[0, 1, 2])
    with self.assertRaises(TypeError):
      _ = data.repeated_int64[np.array([0])]
    with self.assertRaises(TypeError):
      _ = data.repeated_int64[np.ndarray((1,), buffer=np.array([0]), dtype=int)]
    with self.assertRaises(TypeError):
      _ = data.repeated_int64[np.ndarray((1, 1),
                                         buffer=np.array([0]),
                                         dtype=int)]


@testing_refleaks.TestCase
class NumpyBindingTest(parameterized.TestCase):

  @parameterized.product(
      message_module=[unittest_pb2, unittest_proto3_arena_pb2],
      field_name=[
          'repeated_int32',
          'repeated_int64',
          'repeated_uint32',
          'repeated_uint64',
          'repeated_sint32',
          'repeated_sint64',
          'repeated_fixed32',
          'repeated_fixed64',
          'repeated_sfixed32',
          'repeated_sfixed64',
          'repeated_float',
          'repeated_double',
      ],
  )
  def test_simple_np_array_from_repeated(self, message_module, field_name):
    m = message_module.TestAllTypes()
    field = getattr(m, field_name)
    field.append(42)
    field.append(127)
    arr = np.asarray(field)
    self.assertIsInstance(arr, np.ndarray)
    np.testing.assert_equal(arr, np.array([42, 127]))

  @parameterized.named_parameters(
      ('_proto2', unittest_pb2), ('_proto3', unittest_proto3_arena_pb2)
  )
  def test_simple_np_array_from_repeated_continue(self, message_module):
    m = message_module.TestAllTypes()
    m.repeated_nested_enum.extend([1, 2, 3])
    arr = np.asarray(m.repeated_nested_enum)
    self.assertIsInstance(arr, np.ndarray)
    np.testing.assert_equal(arr, np.array([1, 2, 3]))

    m.repeated_bool.append(False)
    m.repeated_bool.append(True)
    arr = np.asarray(m.repeated_bool)
    self.assertIsInstance(arr, np.ndarray)
    np.testing.assert_equal(arr, np.array([False, True]))

    m.repeated_string.extend([
        'One',
        'Two',
        'Three',
    ])
    arr = np.asarray(m.repeated_string)
    self.assertIsInstance(arr, np.ndarray)
    np.testing.assert_equal(arr, np.array(['One', 'Two', 'Three']))

    m.repeated_bytes.append(b'1')
    m.repeated_bytes.append(b'2')
    m.repeated_bytes.append(b'3')
    arr = np.asarray(m.repeated_bytes)
    self.assertIsInstance(arr, np.ndarray)
    np.testing.assert_equal(arr, np.array([b'1', b'2', b'3']))

  @parameterized.named_parameters(
      ('_proto2', unittest_pb2), ('_proto3', unittest_proto3_arena_pb2)
  )
  def test_simple_n_array_from_repeated_message(self, message_module):
    m = message_module.TestAllTypes(
        repeated_nested_message=(
            message_module.TestAllTypes.NestedMessage(bb=9),
            message_module.TestAllTypes.NestedMessage(bb=8),
        )
    )
    arr = np.array(m.repeated_nested_message)
    self.assertIsInstance(arr, np.ndarray)
    self.assertEqual(arr[0].bb, 9)
    self.assertEqual(arr[1].bb, 8)

  @parameterized.product(
      message_module=[unittest_pb2, unittest_proto3_arena_pb2],
      field_name=[
          'repeated_int32',
          'repeated_int64',
          'repeated_sint32',
          'repeated_sint64',
          'repeated_sfixed32',
          'repeated_sfixed64',
          'repeated_float',
          'repeated_double',
      ],
  )
  def test_numpy_signed_arrays_from_repeated(self, message_module, field_name):
    m = message_module.TestAllTypes()
    field = getattr(m, field_name)
    field.append(-42)
    field.append(0)
    field.append(127)
    arr = np.asarray(field)
    self.assertIsInstance(arr, np.ndarray)
    np.testing.assert_equal(arr, np.array([-42, 0, 127]))

  @parameterized.product(
      message_module=[unittest_pb2, unittest_proto3_arena_pb2],
      field_name=[
          'repeated_int32',
          'repeated_int64',
          'repeated_uint32',
          'repeated_uint64',
          'repeated_sint32',
          'repeated_sint64',
          'repeated_fixed32',
          'repeated_fixed64',
          'repeated_sfixed32',
          'repeated_sfixed64',
          'repeated_float',
          'repeated_double',
          'repeated_nested_enum',
      ],
  )
  def test_numpy_empty_repeated(self, message_module, field_name):
    m = message_module.TestAllTypes()
    field = getattr(m, field_name)
    arr = np.array(field)
    arr2 = np.array(field, dtype=np.int8)
    self.assertIsInstance(arr, np.ndarray)
    self.assertIsInstance(arr2, np.ndarray)
    np.testing.assert_equal(arr, np.array([]))
    np.testing.assert_equal(arr2, np.array([], dtype=np.int8))

  @parameterized.product(
      message_module=[unittest_pb2, unittest_proto3_arena_pb2],
      field_name=['packed_sint32', 'packed_sint64'],
  )
  def test_numpy_signed_packed_arrays_from_repeated(
      self, message_module, field_name
  ):
    m = message_module.TestPackedTypes()
    field = getattr(m, field_name)
    field.append(-42)
    field.append(0)
    field.append(127)
    arr = np.asarray(field)
    self.assertIsInstance(arr, np.ndarray)
    np.testing.assert_equal(arr, np.array([-42, 0, 127]))

  @parameterized.product(
      message_module=[unittest_pb2, unittest_proto3_arena_pb2],
      dtype=[
          'int8',
          'int16',
          'int32',
          'int64',
          'float16',
          'float32',
          'float64',
          'str',
          'bool',
          'object',
      ],
  )
  def test_repeated_bytes_to_all_types(self, message_module, dtype):
    m = message_module.TestAllTypes()
    m.repeated_bytes.extend([b'11', b'12'])
    arr = np.asarray(m.repeated_bytes, dtype=dtype)
    self.assertIsInstance(arr, np.ndarray)
    self.assertTrue(arr.flags.contiguous)

  @parameterized.named_parameters(
      ('_proto2', unittest_pb2), ('_proto3', unittest_proto3_arena_pb2)
  )
  def test_repeated_string_tobytes(self, message_module):
    m = message_module.TestAllTypes(repeated_string=['12'])
    arr = np.array(m.repeated_string)
    self.assertEqual(arr.tobytes(), b'1\x00\x00\x002\x00\x00\x00')

  @parameterized.named_parameters(
      ('_proto2', unittest_pb2), ('_proto3', unittest_proto3_arena_pb2)
  )
  def test_repeated_bytes_tobytes(self, message_module):
    m = message_module.TestAllTypes(repeated_bytes=[b'11', b'12', b'13'])
    arr = np.array(m.repeated_bytes)
    np.testing.assert_array_equal(
        arr, np.asarray([b'11', b'12', b'13'], dtype=bytes)
    )
    self.assertEqual(arr.tobytes(), b'111213')

  @parameterized.named_parameters(
      ('_proto2', unittest_pb2), ('_proto3', unittest_proto3_arena_pb2)
  )
  def test_repeated_string_none_dtype(self, message_module):
    m = message_module.TestAllTypes()
    m.repeated_string.extend(['12', '2321'])
    arr = np.asarray(m.repeated_string, dtype=None)
    self.assertIsInstance(arr, np.ndarray)
    np.testing.assert_array_equal(arr, np.asarray(['12', '2321'], dtype=str))

  @parameterized.named_parameters(
      ('_proto2', unittest_pb2), ('_proto3', unittest_proto3_arena_pb2)
  )
  def test_repeated_string_int8_dtype(self, message_module):
    m = message_module.TestAllTypes()
    m.repeated_string.extend(['123', '-15'])
    arr = np.asarray(m.repeated_string, dtype=np.int8)
    self.assertIsInstance(arr, np.ndarray)
    self.assertEqual(arr.dtype, np.int8)
    np.testing.assert_array_equal(arr, np.asarray([123, -15], dtype=np.int8))

  @parameterized.named_parameters(
      ('_proto2', unittest_pb2), ('_proto3', unittest_proto3_arena_pb2)
  )
  def test_repeated_bytes_none_dtype(self, message_module):
    m = message_module.TestAllTypes()
    m.repeated_bytes.append(bytes([122, 124]))
    m.repeated_bytes.append(bytes([13]))
    arr = np.asarray(m.repeated_bytes, dtype=None)
    self.assertIsInstance(arr, np.ndarray)
    expected = np.asarray([b'\x7A\x7C', b'\x0d'])
    np.testing.assert_array_equal(arr, expected)

  @parameterized.named_parameters(
      ('_proto2', unittest_pb2), ('_proto3', unittest_proto3_arena_pb2)
  )
  def test_repeated_bytes_object_dtype(self, message_module):
    m = message_module.TestAllTypes()
    t = np.array([b'932', b'124\x00'], dtype=object)
    m.repeated_bytes.extend(t)
    ss = m.SerializeToString()
    m2 = message_module.TestAllTypes.FromString(ss)
    arr = np.asarray(m2.repeated_bytes, dtype=object)
    self.assertIsInstance(arr, np.ndarray)
    np.testing.assert_array_equal(arr, t)

  @parameterized.named_parameters(
      ('_proto2', unittest_pb2), ('_proto3', unittest_proto3_arena_pb2)
  )
  def test_empty_list_object_dtype(self, message_module):
    m = message_module.TestAllTypes()
    t = np.array([], dtype=object)
    m.repeated_bytes.extend(t)
    ss = m.SerializeToString()
    m2 = message_module.TestAllTypes.FromString(ss)
    arr = np.asarray(m2.repeated_bytes, dtype=object)
    self.assertIsInstance(arr, np.ndarray)
    np.testing.assert_array_equal(arr, t)

  @parameterized.named_parameters(
      ('_proto2', unittest_pb2), ('_proto3', unittest_proto3_arena_pb2)
  )
  def test_default_dtype(self, message_module):
    m = message_module.TestAllTypes(
        repeated_int32=[1, 2],
        repeated_uint32=[2],
        repeated_int64=[1],
        repeated_uint64=[1],
        repeated_float=[1],
        repeated_double=[0.1],
        repeated_string=['1'],
        repeated_bytes=[b'123'],
        repeated_bool=[True],
    )
    self.assertEqual(np.array(m.repeated_int32).dtype, np.int32)
    self.assertEqual(np.array(m.repeated_uint32).dtype, np.uint32)
    self.assertEqual(np.array(m.repeated_int64).dtype, np.int64)
    self.assertEqual(np.array(m.repeated_uint64).dtype, np.uint64)
    self.assertEqual(np.array(m.repeated_float).dtype, np.float32)
    self.assertEqual(np.array(m.repeated_double).dtype, np.float64)
    self.assertEqual(np.array(m.repeated_string).dtype, np.dtype('<U1'))
    self.assertEqual(np.array(m.repeated_bytes).dtype, np.dtype('S3'))
    self.assertEqual(np.array(m.repeated_bool).dtype, np.dtype('bool'))
    message = json_format_pb2.TestRepeatedEnum(
        repeated_enum=[json_format_pb2.BUFFER]
    )
    self.assertEqual(np.array(message.repeated_enum).dtype, np.int32)

  @parameterized.named_parameters(
      ('_proto2', unittest_pb2), ('_proto3', unittest_proto3_arena_pb2)
  )
  def test_set_dtype(self, message_module):
    m = message_module.TestAllTypes(
        repeated_int32=[1, 2],
        repeated_uint32=[2],
        repeated_int64=[1],
        repeated_uint64=[1],
        repeated_float=[1.2, 1],
        repeated_double=[0.1],
        repeated_string=['1'],
        repeated_bytes=[b'123'],
        repeated_bool=[True],
    )
    arr = np.array(m.repeated_float)
    self.assertEqual(np.array(m.repeated_int32, dtype=np.int32).dtype, np.int32)
    self.assertEqual(
        np.array(m.repeated_uint32, dtype=np.int32).dtype, np.int32
    )
    self.assertEqual(
        np.array(m.repeated_int64, dtype=np.uint32).dtype, np.uint32
    )
    self.assertEqual(
        np.array(m.repeated_uint64, dtype=np.uint32).dtype, np.uint32
    )
    self.assertEqual(
        np.array(m.repeated_float, dtype=np.float32).dtype, np.float32
    )
    self.assertEqual(
        np.array(m.repeated_double, dtype=np.float32).dtype, np.float32
    )
    self.assertEqual(
        np.array(m.repeated_string, dtype=object).dtype, np.dtype('O')
    )
    self.assertEqual(
        np.array(m.repeated_bytes, dtype=object).dtype, np.dtype('O')
    )
    self.assertEqual(np.array(m.repeated_bool, dtype=np.int32).dtype, np.int32)

  @parameterized.named_parameters(
      ('_proto2', unittest_pb2), ('_proto3', unittest_proto3_arena_pb2)
  )
  def test_empty_repeated_default_dtype(self, message_module):
    m = message_module.TestAllTypes()
    self.assertEqual(np.array(m.repeated_int32).dtype, np.int32)
    self.assertEqual(np.array(m.repeated_uint32).dtype, np.uint32)
    self.assertEqual(np.array(m.repeated_int64).dtype, np.int64)
    self.assertEqual(np.array(m.repeated_uint64).dtype, np.uint64)
    self.assertEqual(np.array(m.repeated_float).dtype, np.float32)
    self.assertEqual(np.array(m.repeated_double).dtype, np.float64)
    self.assertEqual(np.array(m.repeated_string).dtype, np.dtype('<U1'))
    self.assertEqual(np.array(m.repeated_bytes).dtype, np.dtype('S1'))

  @parameterized.product(
      message_module=[unittest_pb2, unittest_proto3_arena_pb2],
      field_name=[
          'repeated_int32',
          'repeated_int64',
          'repeated_uint32',
          'repeated_uint64',
          'repeated_sint32',
          'repeated_sint64',
          'repeated_fixed32',
          'repeated_fixed64',
          'repeated_sfixed32',
          'repeated_sfixed64',
          'repeated_float',
          'repeated_double',
          'repeated_nested_enum',
      ],
  )
  def test_empty_repeated_set_dtype(self, message_module, field_name):
    m = message_module.TestAllTypes()
    field = getattr(m, field_name)
    self.assertEqual(np.array(field, dtype=np.int32).dtype, np.int32)

  @parameterized.named_parameters(
      ('_proto2', unittest_pb2), ('_proto3', unittest_proto3_arena_pb2)
  )
  def test_nested_message(self, message_module):
    m = message_module.NestedTestAllTypes()
    arr = np.array(m.child.payload.repeated_float)
    self.assertEqual(arr.dtype, np.float32)

  @parameterized.named_parameters(
      ('_proto2', unittest_pb2), ('_proto3', unittest_proto3_arena_pb2)
  )
  def test_float_compare(self, message_module):
    m = message_module.TestAllTypes()
    expected = [87.5011, 1.1]
    m.repeated_float.extend(expected)
    np.testing.assert_equal(
        np.array(m.repeated_float), np.array(expected, np.float32)
    )
    m.repeated_double.extend(expected)
    np.testing.assert_equal(np.array(m.repeated_double), np.array(expected))

  @parameterized.named_parameters(
      ('_proto2', unittest_pb2), ('_proto3', unittest_proto3_arena_pb2)
  )
  def test_nparray_modify(self, message_module):
    m = message_module.TestAllTypes()
    size = 10
    expected = np.full(size, 123, dtype=np.int32)
    m.repeated_int32.extend(expected)
    arr = np.array(m.repeated_int32, np.int32)
    arr[2] = 111
    self.assertEqual(arr[2], 111)
    self.assertEqual(arr[3], 123)
    self.assertEqual(m.repeated_int32[2], 123)

  @parameterized.named_parameters(
      ('_proto2', unittest_pb2), ('_proto3', unittest_proto3_arena_pb2)
  )
  def test_nparray_order(self, message_module):
    m = message_module.TestAllTypes(repeated_int32=[1, 2, 3])
    arr = np.array(m.repeated_int32, order='F')
    np.testing.assert_equal(arr, np.array([1, 2, 3]))


if __name__ == '__main__':
  unittest.main()
