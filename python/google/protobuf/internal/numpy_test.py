# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
# https://developers.google.com/protocol-buffers/
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Test use of numpy types with repeated and non-repeated scalar fields."""

import unittest

import numpy as np

from google.protobuf.internal import testing_refleaks
from google.protobuf import unittest_pb2

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

if __name__ == '__main__':
  unittest.main()
