# -*- coding: utf-8 -*-
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

"""Tests python protocol buffers against the golden message.

Note that the golden messages exercise every known field type, thus this
test ends up exercising and verifying nearly all of the parsing and
serialization code in the whole library.
"""

__author__ = 'gps@google.com (Gregory P. Smith)'

import collections
import copy
import math
import operator
import pickle
import pydoc
import sys
import unittest
import warnings

import pytest

cmp = lambda x, y: (x > y) - (x < y)

from google.protobuf.internal import api_implementation # pylint: disable=g-import-not-at-top
from google.protobuf.internal import encoder
from google.protobuf.internal import more_extensions_pb2
from google.protobuf.internal import more_messages_pb2
from google.protobuf.internal import packed_field_test_pb2
from google.protobuf.internal import test_proto3_optional_pb2
from google.protobuf.internal import test_util
from google.protobuf.internal import testing_refleaks
from google.protobuf import descriptor
from google.protobuf import message
from google.protobuf import map_proto2_unittest_pb2
from google.protobuf import map_unittest_pb2
from google.protobuf import unittest_pb2
from google.protobuf import unittest_proto3_arena_pb2

UCS2_MAXUNICODE = 65535

warnings.simplefilter('error', DeprecationWarning)


@pytest.mark.parametrize(
    'message_module',
    [unittest_pb2, unittest_proto3_arena_pb2],
    ids=['_proto2', '_proto3']
)
@testing_refleaks.TestCase
class TestMessage:
    def test_bad_utf8_string(self, message_module):
        if api_implementation.Type() != 'python':
            self.skipTest('Skipping testBadUtf8String, currently only the python '
                        'api implementation raises UnicodeDecodeError when a '
                        'string field contains bad utf-8.')
        bad_utf8_data = test_util.GoldenFileData('bad_utf8_string')
        with pytest.raises(UnicodeDecodeError) as context:
            message_module.TestAllTypes.FromString(bad_utf8_data)
        assert 'TestAllTypes.optional_string' in str(context.value)

    def test_golden_message(self, message_module):
        # Proto3 doesn't have the "default_foo" members or foreign enums,
        # and doesn't preserve unknown fields, so for proto3 we use a golden
        # message that doesn't have these fields set.
        if message_module is unittest_pb2:
            golden_data = test_util.GoldenFileData('golden_message_oneof_implemented')
        else:
            golden_data = test_util.GoldenFileData('golden_message_proto3')

        golden_message = message_module.TestAllTypes()
        golden_message.ParseFromString(golden_data)
        if message_module is unittest_pb2:
            test_util.ExpectAllFieldsSet(self, golden_message)
        assert golden_data == golden_message.SerializeToString()
        golden_copy = copy.deepcopy(golden_message)
        assert golden_data == golden_copy.SerializeToString()

    def test_golden_packed_message(self, message_module):
        golden_data = test_util.GoldenFileData('golden_packed_fields_message')
        golden_message = message_module.TestPackedTypes()
        parsed_bytes = golden_message.ParseFromString(golden_data)
        all_set = message_module.TestPackedTypes()
        test_util.SetAllPackedFields(all_set)
        assert parsed_bytes == len(golden_data)
        assert all_set == golden_message
        assert golden_data == all_set.SerializeToString()
        golden_copy = copy.deepcopy(golden_message)
        assert golden_data == golden_copy.SerializeToString()

    def test_parse_errors(self, message_module):
        msg = message_module.TestAllTypes()
        pytest.raises(TypeError, msg.FromString, 0)
        pytest.raises(Exception, msg.FromString, '0')
        # TODO(jieluo): Fix cpp extension to raise error instead of warning.
        # b/27494216
        end_tag = encoder.TagBytes(1, 4)
        if (api_implementation.Type() == 'python' or
                api_implementation.Type() == 'upb'):
            with pytest.raises(message.DecodeError) as context:
                msg.FromString(end_tag)
            if api_implementation.Type() == 'python':
                # Only pure-Python has an error message this specific.
                assert 'Unexpected end-group tag.' == str(context.value)

        # Field number 0 is illegal.
        pytest.raises(message.DecodeError, msg.FromString, b'\3\4')

    def test_determinism_parameters(self, message_module):
        # This message is always deterministically serialized, even if determinism
        # is disabled, so we can use it to verify that all the determinism
        # parameters work correctly.
        golden_data = (b'\xe2\x02\nOne string'
                       b'\xe2\x02\nTwo string'
                       b'\xe2\x02\nRed string'
                       b'\xe2\x02\x0bBlue string')
        golden_message = message_module.TestAllTypes()
        golden_message.repeated_string.extend([
            'One string',
            'Two string',
            'Red string',
            'Blue string',
        ])
        assert golden_data == golden_message.SerializeToString(deterministic=None)
        assert golden_data == golden_message.SerializeToString(deterministic=False)
        assert golden_data == golden_message.SerializeToString(deterministic=True)

        class BadArgError(Exception):
            pass

        class BadArg(object):
            def __nonzero__(self):
                raise BadArgError()

            def __bool__(self):
                raise BadArgError()

        with pytest.raises(BadArgError):
            golden_message.SerializeToString(deterministic=BadArg())

    def test_pickle_support(self, message_module):
        golden_data = test_util.GoldenFileData('golden_message')
        golden_message = message_module.TestAllTypes()
        golden_message.ParseFromString(golden_data)
        pickled_message = pickle.dumps(golden_message)

        unpickled_message = pickle.loads(pickled_message)
        assert unpickled_message == golden_message

    def test_pickle_nested_message(self, message_module):
        golden_message = message_module.TestPickleNestedMessage.NestedMessage(bb=1)
        pickled_message = pickle.dumps(golden_message)
        unpickled_message = pickle.loads(pickled_message)
        assert unpickled_message == golden_message

    def test_pickle_nested_nested_message(self, message_module):
        cls = message_module.TestPickleNestedMessage.NestedMessage
        golden_message = cls.NestedNestedMessage(cc=1)
        pickled_message = pickle.dumps(golden_message)
        unpickled_message = pickle.loads(pickled_message)
        assert unpickled_message == golden_message

    def test_positive_infinity(self, message_module):
        if message_module is unittest_pb2:
            golden_data = (b'\x5D\x00\x00\x80\x7F'
                          b'\x61\x00\x00\x00\x00\x00\x00\xF0\x7F'
                          b'\xCD\x02\x00\x00\x80\x7F'
                          b'\xD1\x02\x00\x00\x00\x00\x00\x00\xF0\x7F')
        else:
            golden_data = (b'\x5D\x00\x00\x80\x7F'
                          b'\x61\x00\x00\x00\x00\x00\x00\xF0\x7F'
                          b'\xCA\x02\x04\x00\x00\x80\x7F'
                          b'\xD2\x02\x08\x00\x00\x00\x00\x00\x00\xF0\x7F')

        golden_message = message_module.TestAllTypes()
        golden_message.ParseFromString(golden_data)
        assert golden_message.optional_float == math.inf
        assert golden_message.optional_double == math.inf
        assert golden_message.repeated_float[0] == math.inf
        assert golden_message.repeated_double[0] == math.inf
        assert golden_data == golden_message.SerializeToString()

    def test_negative_infinity(self, message_module):
        if message_module is unittest_pb2:
            golden_data = (b'\x5D\x00\x00\x80\xFF'
                           b'\x61\x00\x00\x00\x00\x00\x00\xF0\xFF'
                           b'\xCD\x02\x00\x00\x80\xFF'
                           b'\xD1\x02\x00\x00\x00\x00\x00\x00\xF0\xFF')
        else:
            golden_data = (b'\x5D\x00\x00\x80\xFF'
                           b'\x61\x00\x00\x00\x00\x00\x00\xF0\xFF'
                           b'\xCA\x02\x04\x00\x00\x80\xFF'
                           b'\xD2\x02\x08\x00\x00\x00\x00\x00\x00\xF0\xFF')

        golden_message = message_module.TestAllTypes()
        golden_message.ParseFromString(golden_data)
        assert golden_message.optional_float == -math.inf
        assert golden_message.optional_double == -math.inf
        assert golden_message.repeated_float[0] == -math.inf
        assert golden_message.repeated_double[0] == -math.inf
        assert golden_data == golden_message.SerializeToString()

    def test_not_a_number(self, message_module):
        golden_data = (b'\x5D\x00\x00\xC0\x7F'
                       b'\x61\x00\x00\x00\x00\x00\x00\xF8\x7F'
                       b'\xCD\x02\x00\x00\xC0\x7F'
                       b'\xD1\x02\x00\x00\x00\x00\x00\x00\xF8\x7F')
        golden_message = message_module.TestAllTypes()
        golden_message.ParseFromString(golden_data)
        assert math.isnan(golden_message.optional_float)
        assert math.isnan(golden_message.optional_double)
        assert math.isnan(golden_message.repeated_float[0])
        assert math.isnan(golden_message.repeated_double[0])

        # The protocol buffer may serialize to any one of multiple different
        # representations of a NaN.  Rather than verify a specific representation,
        # verify the serialized string can be converted into a correctly
        # behaving protocol buffer.
        serialized = golden_message.SerializeToString()
        message = message_module.TestAllTypes()
        message.ParseFromString(serialized)
        assert math.isnan(message.optional_float)
        assert math.isnan(message.optional_double)
        assert math.isnan(message.repeated_float[0])
        assert math.isnan(message.repeated_double[0])

    def test_positive_infinity_packed(self, message_module):
        golden_data = (b'\xA2\x06\x04\x00\x00\x80\x7F'
                       b'\xAA\x06\x08\x00\x00\x00\x00\x00\x00\xF0\x7F')
        golden_message = message_module.TestPackedTypes()
        golden_message.ParseFromString(golden_data)
        assert golden_message.packed_float[0] == math.inf
        assert golden_message.packed_double[0] == math.inf
        assert golden_data == golden_message.SerializeToString()

    def test_negative_infinity_packed(self, message_module):
        golden_data = (b'\xA2\x06\x04\x00\x00\x80\xFF'
                      b'\xAA\x06\x08\x00\x00\x00\x00\x00\x00\xF0\xFF')
        golden_message = message_module.TestPackedTypes()
        golden_message.ParseFromString(golden_data)
        assert golden_message.packed_float[0] == -math.inf
        assert golden_message.packed_double[0] == -math.inf
        assert golden_data == golden_message.SerializeToString()

    def test_not_a_number_packed(self, message_module):
        golden_data = (b'\xA2\x06\x04\x00\x00\xC0\x7F'
                       b'\xAA\x06\x08\x00\x00\x00\x00\x00\x00\xF8\x7F')
        golden_message = message_module.TestPackedTypes()
        golden_message.ParseFromString(golden_data)
        assert math.isnan(golden_message.packed_float[0])
        assert math.isnan(golden_message.packed_double[0])

        serialized = golden_message.SerializeToString()
        message = message_module.TestPackedTypes()
        message.ParseFromString(serialized)
        assert math.isnan(message.packed_float[0])
        assert math.isnan(message.packed_double[0])

    def test_extreme_float_values(self, message_module):
        message = message_module.TestAllTypes()

        # Most positive exponent, no significand bits set.
        kMostPosExponentNoSigBits = math.pow(2, 127)
        message.optional_float = kMostPosExponentNoSigBits
        message.ParseFromString(message.SerializeToString())
        assert message.optional_float == kMostPosExponentNoSigBits

        # Most positive exponent, one significand bit set.
        kMostPosExponentOneSigBit = 1.5 * math.pow(2, 127)
        message.optional_float = kMostPosExponentOneSigBit
        message.ParseFromString(message.SerializeToString())
        assert message.optional_float == kMostPosExponentOneSigBit

        # Repeat last two cases with values of same magnitude, but negative.
        message.optional_float = -kMostPosExponentNoSigBits
        message.ParseFromString(message.SerializeToString())
        assert message.optional_float == -kMostPosExponentNoSigBits

        message.optional_float = -kMostPosExponentOneSigBit
        message.ParseFromString(message.SerializeToString())
        assert message.optional_float == -kMostPosExponentOneSigBit

        # Most negative exponent, no significand bits set.
        kMostNegExponentNoSigBits = math.pow(2, -127)
        message.optional_float = kMostNegExponentNoSigBits
        message.ParseFromString(message.SerializeToString())
        assert message.optional_float == kMostNegExponentNoSigBits

        # Most negative exponent, one significand bit set.
        kMostNegExponentOneSigBit = 1.5 * math.pow(2, -127)
        message.optional_float = kMostNegExponentOneSigBit
        message.ParseFromString(message.SerializeToString())
        assert message.optional_float == kMostNegExponentOneSigBit

        # Repeat last two cases with values of the same magnitude, but negative.
        message.optional_float = -kMostNegExponentNoSigBits
        message.ParseFromString(message.SerializeToString())
        assert message.optional_float == -kMostNegExponentNoSigBits

        message.optional_float = -kMostNegExponentOneSigBit
        message.ParseFromString(message.SerializeToString())
        assert message.optional_float == -kMostNegExponentOneSigBit

        # Max 4 bytes float value
        max_float = float.fromhex('0x1.fffffep+127')
        message.optional_float = max_float
        assert message.optional_float == pytest.approx(max_float)
        serialized_data = message.SerializeToString()
        message.ParseFromString(serialized_data)
        assert message.optional_float == pytest.approx(max_float)

        # Test set double to float field.
        message.optional_float = 3.4028235e+39
        assert message.optional_float == float('inf')
        serialized_data = message.SerializeToString()
        message.ParseFromString(serialized_data)
        assert message.optional_float == float('inf')

        message.optional_float = -3.4028235e+39
        assert message.optional_float == float('-inf')

        message.optional_float = 1.4028235e-39
        assert message.optional_float == pytest.approx(1.4028235e-39)

    def test_extreme_double_values(self, message_module):
        message = message_module.TestAllTypes()

        # Most positive exponent, no significand bits set.
        kMostPosExponentNoSigBits = math.pow(2, 1023)
        message.optional_double = kMostPosExponentNoSigBits
        message.ParseFromString(message.SerializeToString())
        assert message.optional_double == kMostPosExponentNoSigBits

        # Most positive exponent, one significand bit set.
        kMostPosExponentOneSigBit = 1.5 * math.pow(2, 1023)
        message.optional_double = kMostPosExponentOneSigBit
        message.ParseFromString(message.SerializeToString())
        assert message.optional_double == kMostPosExponentOneSigBit

        # Repeat last two cases with values of same magnitude, but negative.
        message.optional_double = -kMostPosExponentNoSigBits
        message.ParseFromString(message.SerializeToString())
        assert message.optional_double == -kMostPosExponentNoSigBits

        message.optional_double = -kMostPosExponentOneSigBit
        message.ParseFromString(message.SerializeToString())
        assert message.optional_double == -kMostPosExponentOneSigBit

        # Most negative exponent, no significand bits set.
        kMostNegExponentNoSigBits = math.pow(2, -1023)
        message.optional_double = kMostNegExponentNoSigBits
        message.ParseFromString(message.SerializeToString())
        assert message.optional_double == kMostNegExponentNoSigBits

        # Most negative exponent, one significand bit set.
        kMostNegExponentOneSigBit = 1.5 * math.pow(2, -1023)
        message.optional_double = kMostNegExponentOneSigBit
        message.ParseFromString(message.SerializeToString())
        assert message.optional_double == kMostNegExponentOneSigBit

        # Repeat last two cases with values of the same magnitude, but negative.
        message.optional_double = -kMostNegExponentNoSigBits
        message.ParseFromString(message.SerializeToString())
        assert message.optional_double == -kMostNegExponentNoSigBits

        message.optional_double = -kMostNegExponentOneSigBit
        message.ParseFromString(message.SerializeToString())
        assert message.optional_double == -kMostNegExponentOneSigBit

    def test_float_printing(self, message_module):
        message = message_module.TestAllTypes()
        message.optional_float = 2.0
        assert str(message) == 'optional_float: 2.0\n'

    def test_high_precision_float_printing(self, message_module):
        msg = message_module.TestAllTypes()
        msg.optional_float = 0.12345678912345678
        old_float = msg.optional_float
        msg.ParseFromString(msg.SerializeToString())
        assert old_float == msg.optional_float

    def test_high_precision_double_printing(self, message_module):
        msg = message_module.TestAllTypes()
        msg.optional_double = 0.12345678912345678
        assert str(msg) == 'optional_double: 0.12345678912345678\n'

    def test_unknown_field_printing(self, message_module):
        populated = message_module.TestAllTypes()
        test_util.SetAllNonLazyFields(populated)
        empty = message_module.TestEmptyMessage()
        empty.ParseFromString(populated.SerializeToString())
        assert str(empty) == ''

    def test_append_repeated_composite_field(self, message_module):
        msg = message_module.TestAllTypes()
        msg.repeated_nested_message.append(
            message_module.TestAllTypes.NestedMessage(bb=1))
        nested = message_module.TestAllTypes.NestedMessage(bb=2)
        msg.repeated_nested_message.append(nested)
        try:
            msg.repeated_nested_message.append(1)
        except TypeError:
            pass
        assert 2 == len(msg.repeated_nested_message)
        assert [1, 2] == [m.bb for m in msg.repeated_nested_message]

    def test_insert_repeated_composite_field(self, message_module):
        msg = message_module.TestAllTypes()
        msg.repeated_nested_message.insert(
            -1, message_module.TestAllTypes.NestedMessage(bb=1))
        sub_msg = msg.repeated_nested_message[0]
        msg.repeated_nested_message.insert(
            0, message_module.TestAllTypes.NestedMessage(bb=2))
        msg.repeated_nested_message.insert(
            99, message_module.TestAllTypes.NestedMessage(bb=3))
        msg.repeated_nested_message.insert(
            -2, message_module.TestAllTypes.NestedMessage(bb=-1))
        msg.repeated_nested_message.insert(
            -1000, message_module.TestAllTypes.NestedMessage(bb=-1000))
        try:
            msg.repeated_nested_message.insert(1, 999)
        except TypeError:
            pass
        assert 5 == len(msg.repeated_nested_message)
        assert [-1000, 2, -1, 1, 3] == [m.bb for m in msg.repeated_nested_message]
        assert (
            str(msg) == 'repeated_nested_message {\n'
            '  bb: -1000\n'
            '}\n'
            'repeated_nested_message {\n'
            '  bb: 2\n'
            '}\n'
            'repeated_nested_message {\n'
            '  bb: -1\n'
            '}\n'
            'repeated_nested_message {\n'
            '  bb: 1\n'
            '}\n'
            'repeated_nested_message {\n'
            '  bb: 3\n'
            '}\n')
        assert sub_msg.bb == 1

    def test_assign_repeated_field(self, message_module):
        msg = message_module.NestedTestAllTypes()
        msg.payload.repeated_int32[:] = [1, 2, 3, 4]
        assert 4 == len(msg.payload.repeated_int32)
        assert [1, 2, 3, 4] == msg.payload.repeated_int32

    def test_merge_from_repeated_field(self, message_module):
        msg = message_module.TestAllTypes()
        msg.repeated_int32.append(1)
        msg.repeated_int32.append(3)
        msg.repeated_nested_message.add(bb=1)
        msg.repeated_nested_message.add(bb=2)
        other_msg = message_module.TestAllTypes()
        other_msg.repeated_nested_message.add(bb=3)
        other_msg.repeated_nested_message.add(bb=4)
        other_msg.repeated_int32.append(5)
        other_msg.repeated_int32.append(7)

        msg.repeated_int32.MergeFrom(other_msg.repeated_int32)
        assert 4 == len(msg.repeated_int32)

        msg.repeated_nested_message.MergeFrom(other_msg.repeated_nested_message)
        assert [1, 2, 3, 4] == [m.bb for m in msg.repeated_nested_message]

    def test_internal_merge_with_missing_required_field(self, message_module):
        req = more_messages_pb2.RequiredField()
        more_messages_pb2.RequiredWrapper(request=req)

    def test_add_wrong_repeated_nested_field(self, message_module):
        msg = message_module.TestAllTypes()
        try:
            msg.repeated_nested_message.add('wrong')
        except TypeError:
            pass
        try:
            msg.repeated_nested_message.add(value_field='wrong')
        except ValueError:
            pass
        assert len(msg.repeated_nested_message) == 0

    def test_repeated_contains(self, message_module):
        msg = message_module.TestAllTypes()
        msg.repeated_int32.extend([1, 2, 3])
        assert 2 in msg.repeated_int32
        assert 0 not in msg.repeated_int32

        msg.repeated_nested_message.add(bb=1)
        sub_msg1 = msg.repeated_nested_message[0]
        sub_msg2 = message_module.TestAllTypes.NestedMessage(bb=2)
        sub_msg3 = message_module.TestAllTypes.NestedMessage(bb=3)
        msg.repeated_nested_message.append(sub_msg2)
        msg.repeated_nested_message.insert(0, sub_msg3)
        assert sub_msg1 in msg.repeated_nested_message
        assert sub_msg2 in msg.repeated_nested_message
        assert sub_msg3 in msg.repeated_nested_message

    def test_repeated_scalar_iterable(self, message_module):
        msg = message_module.TestAllTypes()
        msg.repeated_int32.extend([1, 2, 3])
        add = 0
        for item in msg.repeated_int32:
          add += item
        assert add == 6

    def test_repeated_nested_field_iteration(self, message_module):
        msg = message_module.TestAllTypes()
        msg.repeated_nested_message.add(bb=1)
        msg.repeated_nested_message.add(bb=2)
        msg.repeated_nested_message.add(bb=3)
        msg.repeated_nested_message.add(bb=4)

        assert [1, 2, 3, 4] == [m.bb for m in msg.repeated_nested_message]
        assert [4, 3, 2, 1] == [m.bb for m in reversed(msg.repeated_nested_message)]
        assert [4, 3, 2, 1] == [m.bb for m in msg.repeated_nested_message[::-1]]

    def test_sorting_repeated_scalar_fields_default_comparator(self, message_module):
        """Check some different types with the default comparator."""
        message = message_module.TestAllTypes()

        # TODO(mattp): would testing more scalar types strengthen test?
        message.repeated_int32.append(1)
        message.repeated_int32.append(3)
        message.repeated_int32.append(2)
        message.repeated_int32.sort()
        assert message.repeated_int32[0] == 1
        assert message.repeated_int32[1] == 2
        assert message.repeated_int32[2] == 3
        assert str(message.repeated_int32), str([1, 2 == 3])

        message.repeated_float.append(1.1)
        message.repeated_float.append(1.3)
        message.repeated_float.append(1.2)
        message.repeated_float.sort()
        assert message.repeated_float[0] == pytest.approx(1.1)
        assert message.repeated_float[1] == pytest.approx(1.2)
        assert message.repeated_float[2] == pytest.approx(1.3)

        message.repeated_string.append('a')
        message.repeated_string.append('c')
        message.repeated_string.append('b')
        message.repeated_string.sort()
        assert message.repeated_string[0] == 'a'
        assert message.repeated_string[1] == 'b'
        assert message.repeated_string[2] == 'c'
        assert str(message.repeated_string), str([u'a', u'b' == u'c'])

        message.repeated_bytes.append(b'a')
        message.repeated_bytes.append(b'c')
        message.repeated_bytes.append(b'b')
        message.repeated_bytes.sort()
        assert message.repeated_bytes[0] == b'a'
        assert message.repeated_bytes[1] == b'b'
        assert message.repeated_bytes[2] == b'c'
        assert str(message.repeated_bytes), str([b'a', b'b' == b'c'])

    def test_sorting_repeated_scalar_fields_custom_comparator(self, message_module):
        """Check some different types with custom comparator."""
        message = message_module.TestAllTypes()

        message.repeated_int32.append(-3)
        message.repeated_int32.append(-2)
        message.repeated_int32.append(-1)
        message.repeated_int32.sort(key=abs)
        assert message.repeated_int32[0] == -1
        assert message.repeated_int32[1] == -2
        assert message.repeated_int32[2] == -3

        message.repeated_string.append('aaa')
        message.repeated_string.append('bb')
        message.repeated_string.append('c')
        message.repeated_string.sort(key=len)
        assert message.repeated_string[0] == 'c'
        assert message.repeated_string[1] == 'bb'
        assert message.repeated_string[2] == 'aaa'

    def test_sorting_repeated_composite_fields_custom_comparator(self, message_module):
        """Check passing a custom comparator to sort a repeated composite field."""
        message = message_module.TestAllTypes()

        message.repeated_nested_message.add().bb = 1
        message.repeated_nested_message.add().bb = 3
        message.repeated_nested_message.add().bb = 2
        message.repeated_nested_message.add().bb = 6
        message.repeated_nested_message.add().bb = 5
        message.repeated_nested_message.add().bb = 4
        message.repeated_nested_message.sort(key=operator.attrgetter('bb'))
        assert message.repeated_nested_message[0].bb == 1
        assert message.repeated_nested_message[1].bb == 2
        assert message.repeated_nested_message[2].bb == 3
        assert message.repeated_nested_message[3].bb == 4
        assert message.repeated_nested_message[4].bb == 5
        assert message.repeated_nested_message[5].bb == 6
        assert (
            str(message.repeated_nested_message)
            == '[bb: 1\n, bb: 2\n, bb: 3\n, bb: 4\n, bb: 5\n, bb: 6\n]')

    def test_sorting_repeated_composite_fields_stable(self, message_module):
        """Check passing a custom comparator to sort a repeated composite field."""
        message = message_module.TestAllTypes()

        message.repeated_nested_message.add().bb = 21
        message.repeated_nested_message.add().bb = 20
        message.repeated_nested_message.add().bb = 13
        message.repeated_nested_message.add().bb = 33
        message.repeated_nested_message.add().bb = 11
        message.repeated_nested_message.add().bb = 24
        message.repeated_nested_message.add().bb = 10
        message.repeated_nested_message.sort(key=lambda z: z.bb // 10)
        assert [13, 11, 10, 21, 20, 24, 33] == [n.bb for n in message.repeated_nested_message]

        # Make sure that for the C++ implementation, the underlying fields
        # are actually reordered.
        pb = message.SerializeToString()
        message.Clear()
        message.MergeFromString(pb)
        assert [13, 11, 10, 21, 20, 24, 33] == [n.bb for n in message.repeated_nested_message]

    def test_repeated_composite_field_sort_arguments(self, message_module):
        """Check sorting a repeated composite field using list.sort() arguments."""
        message = message_module.TestAllTypes()

        get_bb = operator.attrgetter('bb')
        message.repeated_nested_message.add().bb = 1
        message.repeated_nested_message.add().bb = 3
        message.repeated_nested_message.add().bb = 2
        message.repeated_nested_message.add().bb = 6
        message.repeated_nested_message.add().bb = 5
        message.repeated_nested_message.add().bb = 4
        message.repeated_nested_message.sort(key=get_bb)
        assert [k.bb for k in message.repeated_nested_message], [1, 2, 3, 4, 5 == 6]
        message.repeated_nested_message.sort(key=get_bb, reverse=True)
        assert [k.bb for k in message.repeated_nested_message], [6, 5, 4, 3, 2 == 1]

    def test_repeated_scalar_field_sort_arguments(self, message_module):
        """Check sorting a scalar field using list.sort() arguments."""
        message = message_module.TestAllTypes()

        message.repeated_int32.append(-3)
        message.repeated_int32.append(-2)
        message.repeated_int32.append(-1)
        message.repeated_int32.sort(key=abs)
        assert list(message.repeated_int32), [-1, -2 == -3]
        message.repeated_int32.sort(key=abs, reverse=True)
        assert list(message.repeated_int32), [-3, -2 == -1]

        message.repeated_string.append('aaa')
        message.repeated_string.append('bb')
        message.repeated_string.append('c')
        message.repeated_string.sort(key=len)
        assert list(message.repeated_string), ['c', 'bb' == 'aaa']
        message.repeated_string.sort(key=len, reverse=True)
        assert list(message.repeated_string), ['aaa', 'bb' == 'c']

    def test_repeated_fields_comparable(self, message_module):
        m1 = message_module.TestAllTypes()
        m2 = message_module.TestAllTypes()
        m1.repeated_int32.append(0)
        m1.repeated_int32.append(1)
        m1.repeated_int32.append(2)
        m2.repeated_int32.append(0)
        m2.repeated_int32.append(1)
        m2.repeated_int32.append(2)
        m1.repeated_nested_message.add().bb = 1
        m1.repeated_nested_message.add().bb = 2
        m1.repeated_nested_message.add().bb = 3
        m2.repeated_nested_message.add().bb = 1
        m2.repeated_nested_message.add().bb = 2
        m2.repeated_nested_message.add().bb = 3

    def test_repeated_fields_are_sequences(self, message_module):
        m = message_module.TestAllTypes()
        assert isinstance(m.repeated_int32, collections.abc.MutableSequence)
        assert isinstance(m.repeated_nested_message, collections.abc.MutableSequence)

    def test_repeated_fields_not_hashable(self, message_module):
        m = message_module.TestAllTypes()
        with pytest.raises(TypeError):
            hash(m.repeated_int32)
        with pytest.raises(TypeError):
            hash(m.repeated_nested_message)

    def test_repeated_field_inside_nested_message(self, message_module):
        m = message_module.NestedTestAllTypes()
        m.payload.repeated_int32.extend([])
        assert m.HasField('payload')

    def test_merge_from(self, message_module):
        m1 = message_module.TestAllTypes()
        m2 = message_module.TestAllTypes()
        # Cpp extension will lazily create a sub message which is immutable.
        nested = m1.optional_nested_message
        assert 0 == nested.bb
        m2.optional_nested_message.bb = 1
        # Make sure cmessage pointing to a mutable message after merge instead of
        # the lazily created message.
        m1.MergeFrom(m2)
        assert 1 == nested.bb

        # Test more nested sub message.
        msg1 = message_module.NestedTestAllTypes()
        msg2 = message_module.NestedTestAllTypes()
        nested = msg1.child.payload.optional_nested_message
        assert 0 == nested.bb
        msg2.child.payload.optional_nested_message.bb = 1
        msg1.MergeFrom(msg2)
        assert 1 == nested.bb

        # Test repeated field.
        assert msg1.payload.repeated_nested_message == msg1.payload.repeated_nested_message
        nested = msg2.payload.repeated_nested_message.add()
        nested.bb = 1
        msg1.MergeFrom(msg2)
        assert 1 == len(msg1.payload.repeated_nested_message)
        assert 1 == nested.bb

    def test_merge_from_string(self, message_module):
        m1 = message_module.TestAllTypes()
        m2 = message_module.TestAllTypes()
        # Cpp extension will lazily create a sub message which is immutable.
        assert 0 == m1.optional_nested_message.bb
        m2.optional_nested_message.bb = 1
        # Make sure cmessage pointing to a mutable message after merge instead of
        # the lazily created message.
        m1.MergeFromString(m2.SerializeToString())
        assert 1 == m1.optional_nested_message.bb

    def test_merge_from_string_using_memory_view(self, message_module):
        m2 = message_module.TestAllTypes()
        m2.optional_string = 'scalar string'
        m2.repeated_string.append('repeated string')
        m2.optional_bytes = b'scalar bytes'
        m2.repeated_bytes.append(b'repeated bytes')

        serialized = m2.SerializeToString()
        memview = memoryview(serialized)
        m1 = message_module.TestAllTypes.FromString(memview)

        assert m1.optional_bytes == b'scalar bytes'
        assert m1.repeated_bytes == [b'repeated bytes']
        assert m1.optional_string == 'scalar string'
        assert m1.repeated_string == ['repeated string']
        # Make sure that the memoryview was correctly converted to bytes, and
        # that a sub-sliced memoryview is not being used.
        assert isinstance(m1.optional_bytes, bytes)
        assert isinstance(m1.repeated_bytes[0], bytes)
        assert isinstance(m1.optional_string, str)
        assert isinstance(m1.repeated_string[0], str)

    def test_merge_from_empty(self, message_module):
        m1 = message_module.TestAllTypes()
        # Cpp extension will lazily create a sub message which is immutable.
        assert 0 == m1.optional_nested_message.bb
        assert not m1.HasField('optional_nested_message')
        # Make sure the sub message is still immutable after merge from empty.
        m1.MergeFromString(b'')  # field state should not change
        assert not m1.HasField('optional_nested_message')

    def ensure_nested_message_exists(self, msg, attribute):
        """Make sure that a nested message object exists.

        As soon as a nested message attribute is accessed, it will be present in the
        _fields dict, without being marked as actually being set.
        """
        getattr(msg, attribute)
        assert not msg.HasField(attribute)

    def test_oneof_get_case_nonexisting_field(self, message_module):
        m = message_module.TestAllTypes()
        pytest.raises(ValueError, m.WhichOneof, 'no_such_oneof_field')
        pytest.raises(Exception, m.WhichOneof, 0)

    def test_oneof_default_values(self, message_module):
        m = message_module.TestAllTypes()
        assert None is m.WhichOneof('oneof_field')
        assert not m.HasField('oneof_field')
        assert not m.HasField('oneof_uint32')

        # Oneof is set even when setting it to a default value.
        m.oneof_uint32 = 0
        assert 'oneof_uint32' == m.WhichOneof('oneof_field')
        assert m.HasField('oneof_field')
        assert m.HasField('oneof_uint32')
        assert not m.HasField('oneof_string')

        m.oneof_string = ''
        assert 'oneof_string' == m.WhichOneof('oneof_field')
        assert m.HasField('oneof_string')
        assert not m.HasField('oneof_uint32')

    def test_oneof_semantics(self, message_module):
        m = message_module.TestAllTypes()
        assert None is m.WhichOneof('oneof_field')

        m.oneof_uint32 = 11
        assert 'oneof_uint32' == m.WhichOneof('oneof_field')
        assert m.HasField('oneof_uint32')

        m.oneof_string = u'foo'
        assert 'oneof_string' == m.WhichOneof('oneof_field')
        assert not m.HasField('oneof_uint32')
        assert m.HasField('oneof_string')

        # Read nested message accessor without accessing submessage.
        m.oneof_nested_message
        assert 'oneof_string' == m.WhichOneof('oneof_field')
        assert m.HasField('oneof_string')
        assert not m.HasField('oneof_nested_message')

        # Read accessor of nested message without accessing submessage.
        m.oneof_nested_message.bb
        assert 'oneof_string' == m.WhichOneof('oneof_field')
        assert m.HasField('oneof_string')
        assert not m.HasField('oneof_nested_message')

        m.oneof_nested_message.bb = 11
        assert 'oneof_nested_message' == m.WhichOneof('oneof_field')
        assert not m.HasField('oneof_string')
        assert m.HasField('oneof_nested_message')

        m.oneof_bytes = b'bb'
        assert 'oneof_bytes' == m.WhichOneof('oneof_field')
        assert not m.HasField('oneof_nested_message')
        assert m.HasField('oneof_bytes')

    def test_oneof_composite_field_read_access(self, message_module):
        m = message_module.TestAllTypes()
        m.oneof_uint32 = 11

        self.ensure_nested_message_exists(m, 'oneof_nested_message')
        assert 'oneof_uint32' == m.WhichOneof('oneof_field')
        assert 11 == m.oneof_uint32

    def test_oneof_which_oneof(self, message_module):
        m = message_module.TestAllTypes()
        assert None is m.WhichOneof('oneof_field')
        if message_module is unittest_pb2:
            assert not m.HasField('oneof_field')

        m.oneof_uint32 = 11
        assert 'oneof_uint32' == m.WhichOneof('oneof_field')
        if message_module is unittest_pb2:
            assert m.HasField('oneof_field')

        m.oneof_bytes = b'bb'
        assert 'oneof_bytes' == m.WhichOneof('oneof_field')

        m.ClearField('oneof_bytes')
        assert None is m.WhichOneof('oneof_field')
        if message_module is unittest_pb2:
            assert not m.HasField('oneof_field')

    def test_oneof_clear_field(self, message_module):
        m = message_module.TestAllTypes()
        m.ClearField('oneof_field')
        m.oneof_uint32 = 11
        m.ClearField('oneof_field')
        if message_module is unittest_pb2:
            assert not m.HasField('oneof_field')
        assert not m.HasField('oneof_uint32')
        assert None is m.WhichOneof('oneof_field')

    def test_oneof_clear_set_field(self, message_module):
        m = message_module.TestAllTypes()
        m.oneof_uint32 = 11
        m.ClearField('oneof_uint32')
        if message_module is unittest_pb2:
          assert not m.HasField('oneof_field')
        assert not m.HasField('oneof_uint32')
        assert None is m.WhichOneof('oneof_field')

    def test_oneof_clear_unset_field(self, message_module):
        m = message_module.TestAllTypes()
        m.oneof_uint32 = 11
        self.ensure_nested_message_exists(m, 'oneof_nested_message')
        m.ClearField('oneof_nested_message')
        assert 11 == m.oneof_uint32
        if message_module is unittest_pb2:
            assert m.HasField('oneof_field')
        assert m.HasField('oneof_uint32')
        assert 'oneof_uint32' == m.WhichOneof('oneof_field')

    def test_oneof_deserialize(self, message_module):
        m = message_module.TestAllTypes()
        m.oneof_uint32 = 11
        m2 = message_module.TestAllTypes()
        m2.ParseFromString(m.SerializeToString())
        assert 'oneof_uint32' == m2.WhichOneof('oneof_field')

    def test_oneof_copy_from(self, message_module):
        m = message_module.TestAllTypes()
        m.oneof_uint32 = 11
        m2 = message_module.TestAllTypes()
        m2.CopyFrom(m)
        assert 'oneof_uint32' == m2.WhichOneof('oneof_field')

    def test_oneof_nested_merge_from(self, message_module):
        m = message_module.NestedTestAllTypes()
        m.payload.oneof_uint32 = 11
        m2 = message_module.NestedTestAllTypes()
        m2.payload.oneof_bytes = b'bb'
        m2.child.payload.oneof_bytes = b'bb'
        m2.MergeFrom(m)
        assert 'oneof_uint32' == m2.payload.WhichOneof('oneof_field')
        assert 'oneof_bytes' == m2.child.payload.WhichOneof('oneof_field')

    def test_oneof_message_merge_from(self, message_module):
        m = message_module.NestedTestAllTypes()
        m.payload.oneof_nested_message.bb = 11
        m.child.payload.oneof_nested_message.bb = 12
        m2 = message_module.NestedTestAllTypes()
        m2.payload.oneof_uint32 = 13
        m2.MergeFrom(m)
        assert 'oneof_nested_message' == m2.payload.WhichOneof('oneof_field')
        assert 'oneof_nested_message' == m2.child.payload.WhichOneof('oneof_field')

    def test_oneof_nested_message_init(self, message_module):
        m = message_module.TestAllTypes(
            oneof_nested_message=message_module.TestAllTypes.NestedMessage())
        assert 'oneof_nested_message' == m.WhichOneof('oneof_field')

    def test_oneof_clear(self, message_module):
        m = message_module.TestAllTypes()
        m.oneof_uint32 = 11
        m.Clear()
        assert m.WhichOneof('oneof_field') is None
        m.oneof_bytes = b'bb'
        assert 'oneof_bytes' == m.WhichOneof('oneof_field')

    def test_assign_byte_string_to_unicode_field(self, message_module):
        """Assigning a byte string to a string field should result

        in the value being converted to a Unicode string.
        """
        m = message_module.TestAllTypes()
        m.optional_string = str('')
        assert isinstance(m.optional_string, str)

    def test_long_valued_slice(self, message_module):
        """It should be possible to use int-valued indices in slices.

        This didn't used to work in the v2 C++ implementation.
        """
        m = message_module.TestAllTypes()

        # Repeated scalar
        m.repeated_int32.append(1)
        sl = m.repeated_int32[int(0):int(len(m.repeated_int32))]
        assert len(m.repeated_int32) == len(sl)

        # Repeated composite
        m.repeated_nested_message.add().bb = 3
        sl = m.repeated_nested_message[int(0):int(len(m.repeated_nested_message))]
        assert len(m.repeated_nested_message) == len(sl)

    def test_extend_should_not_swallow_exceptions(self, message_module):
        """This didn't use to work in the v2 C++ implementation."""
        m = message_module.TestAllTypes()
        with pytest.raises(NameError):
            m.repeated_int32.extend(a for i in range(10))  # pylint: disable=undefined-variable
        with pytest.raises(NameError):
            m.repeated_nested_enum.extend(a for i in range(10))  # pylint: disable=undefined-variable

    FALSY_VALUES = [None, False, 0, 0.0]
    EMPTY_VALUES = [b'', u'', bytearray(), [], {}, set()]

    def test_extend_int32_with_nothing(self, message_module):
        """Test no-ops extending repeated int32 fields."""
        m = message_module.TestAllTypes()
        assert [] == m.repeated_int32

        for empty_value in TestMessage.EMPTY_VALUES:
            m.repeated_int32.extend(empty_value)
            assert [] == m.repeated_int32

    def test_extend_float_with_nothing(self, message_module):
        """Test no-ops extending repeated float fields."""
        m = message_module.TestAllTypes()
        assert [] == m.repeated_float

        for empty_value in TestMessage.EMPTY_VALUES:
            m.repeated_float.extend(empty_value)
            assert [] == m.repeated_float

    def test_extend_string_with_nothing(self, message_module):
        """Test no-ops extending repeated string fields."""
        m = message_module.TestAllTypes()
        assert [] == m.repeated_string

        for empty_value in TestMessage.EMPTY_VALUES:
            m.repeated_string.extend(empty_value)
            assert [] == m.repeated_string

    def test_extend_int32_with_python_list(self, message_module):
        """Test extending repeated int32 fields with python lists."""
        m = message_module.TestAllTypes()
        assert [] == m.repeated_int32
        m.repeated_int32.extend([0])
        assert [0] == m.repeated_int32
        m.repeated_int32.extend([1, 2])
        assert [0, 1, 2] == m.repeated_int32
        m.repeated_int32.extend([3, 4])
        assert [0, 1, 2, 3, 4] == m.repeated_int32

    def test_extend_float_with_python_list(self, message_module):
        """Test extending repeated float fields with python lists."""
        m = message_module.TestAllTypes()
        assert [] == m.repeated_float
        m.repeated_float.extend([0.0])
        assert [0.0] == m.repeated_float
        m.repeated_float.extend([1.0, 2.0])
        assert [0.0, 1.0, 2.0] == m.repeated_float
        m.repeated_float.extend([3.0, 4.0])
        assert [0.0, 1.0, 2.0, 3.0, 4.0] == m.repeated_float

    def test_extend_string_with_python_list(self, message_module):
        """Test extending repeated string fields with python lists."""
        m = message_module.TestAllTypes()
        assert [] == m.repeated_string
        m.repeated_string.extend([''])
        assert [''] == m.repeated_string
        m.repeated_string.extend(['11', '22'])
        assert ['', '11', '22'] == m.repeated_string
        m.repeated_string.extend(['33', '44'])
        assert ['', '11', '22', '33', '44'] == m.repeated_string

    def test_extend_string_with_string(self, message_module):
        """Test extending repeated string fields with characters from a string."""
        m = message_module.TestAllTypes()
        assert [] == m.repeated_string
        m.repeated_string.extend('abc')
        assert ['a', 'b', 'c'] == m.repeated_string

    class TestIterable(object):
        """This iterable object mimics the behavior of numpy.array.

        __nonzero__ fails for length > 1, and returns bool(item[0]) for length == 1.

        """

        def __init__(self, values=None):
          self._list = values or []

        def __nonzero__(self):
          size = len(self._list)
          if size == 0:
            return False
          if size == 1:
            return bool(self._list[0])
          raise ValueError('Truth value is ambiguous.')

        def __len__(self):
          return len(self._list)

        def __iter__(self):
          return self._list.__iter__()

    def test_extend_int32_with_iterable(self, message_module):
        """Test extending repeated int32 fields with iterable."""
        m = message_module.TestAllTypes()
        assert [] == m.repeated_int32
        m.repeated_int32.extend(TestMessage.TestIterable([]))
        assert [] == m.repeated_int32
        m.repeated_int32.extend(TestMessage.TestIterable([0]))
        assert [0] == m.repeated_int32
        m.repeated_int32.extend(TestMessage.TestIterable([1, 2]))
        assert [0, 1, 2] == m.repeated_int32
        m.repeated_int32.extend(TestMessage.TestIterable([3, 4]))
        assert [0, 1, 2, 3, 4] == m.repeated_int32

    def test_extend_float_with_iterable(self, message_module):
        """Test extending repeated float fields with iterable."""
        m = message_module.TestAllTypes()
        assert [] == m.repeated_float
        m.repeated_float.extend(TestMessage.TestIterable([]))
        assert [] == m.repeated_float
        m.repeated_float.extend(TestMessage.TestIterable([0.0]))
        assert [0.0] == m.repeated_float
        m.repeated_float.extend(TestMessage.TestIterable([1.0, 2.0]))
        assert [0.0, 1.0, 2.0] == m.repeated_float
        m.repeated_float.extend(TestMessage.TestIterable([3.0, 4.0]))
        assert [0.0, 1.0, 2.0, 3.0, 4.0] == m.repeated_float

    def test_extend_string_with_iterable(self, message_module):
        """Test extending repeated string fields with iterable."""
        m = message_module.TestAllTypes()
        assert [] == m.repeated_string
        m.repeated_string.extend(TestMessage.TestIterable([]))
        assert [] == m.repeated_string
        m.repeated_string.extend(TestMessage.TestIterable(['']))
        assert [''] == m.repeated_string
        m.repeated_string.extend(TestMessage.TestIterable(['1', '2']))
        assert ['', '1', '2'] == m.repeated_string
        m.repeated_string.extend(TestMessage.TestIterable(['3', '4']))
        assert ['', '1', '2', '3', '4'] == m.repeated_string

    class TestIndex(object):
        """This index object mimics the behavior of numpy.int64 and other types."""

        def __init__(self, value=None):
            self.value = value

        def __index__(self):
            return self.value

    def test_repeated_indexing_with_int_index(self, message_module):
        msg = message_module.TestAllTypes()
        msg.repeated_int32.extend([1, 2, 3])
        assert 1 == msg.repeated_int32[TestMessage.TestIndex(0)]

    def test_repeated_indexing_with_negative1_int_index(self, message_module):
        msg = message_module.TestAllTypes()
        msg.repeated_int32.extend([1, 2, 3])
        assert 3 == msg.repeated_int32[TestMessage.TestIndex(-1)]

    def test_repeated_indexing_with_negative1_int(self, message_module):
        msg = message_module.TestAllTypes()
        msg.repeated_int32.extend([1, 2, 3])
        assert 3 == msg.repeated_int32[-1]

    def test_pickle_repeated_scalar_container(self, message_module):
        # Pickle repeated scalar container is not supported.
        m = message_module.TestAllTypes()
        with pytest.raises(pickle.PickleError):
            pickle.dumps(m.repeated_int32, pickle.HIGHEST_PROTOCOL)

    def test_sort_empty_repeated_composite_container(self, message_module):
        """Exercise a scenario that has led to segfaults in the past."""
        m = message_module.TestAllTypes()
        m.repeated_nested_message.sort()

    def test_has_field_on_repeated_field(self, message_module):
        """Using HasField on a repeated field should raise an exception."""
        m = message_module.TestAllTypes()
        with pytest.raises(ValueError):
            m.HasField('repeated_int32')

    def test_repeated_scalar_field_pop(self, message_module):
        m = message_module.TestAllTypes()
        with pytest.raises(IndexError):
           m.repeated_int32.pop()
        m.repeated_int32.extend(range(5))
        assert 4 == m.repeated_int32.pop()
        assert 0 == m.repeated_int32.pop(0)
        assert 2 == m.repeated_int32.pop(1)
        assert [1, 3] == m.repeated_int32

    def test_repeated_composite_field_pop(self, message_module):
        m = message_module.TestAllTypes()
        with pytest.raises(IndexError):
            m.repeated_nested_message.pop()
        with pytest.raises(TypeError):
            m.repeated_nested_message.pop('0')
        for i in range(5):
            n = m.repeated_nested_message.add()
            n.bb = i
        assert 4 == m.repeated_nested_message.pop().bb
        assert 0 == m.repeated_nested_message.pop(0).bb
        assert 2 == m.repeated_nested_message.pop(1).bb
        assert [1, 3] == [n.bb for n in m.repeated_nested_message]

    def test_repeated_compare_with_self(self, message_module):
        m = message_module.TestAllTypes()
        for i in range(5):
            m.repeated_int32.insert(i, i)
            n = m.repeated_nested_message.add()
            n.bb = i
        assert m.repeated_int32 == m.repeated_int32
        assert m.repeated_nested_message == m.repeated_nested_message

    def test_released_nested_messages(self, message_module):
        """A case that lead to a segfault when a message detached from its parent

        container has itself a child container.
        """
        m = message_module.NestedTestAllTypes()
        m = m.repeated_child.add()
        m = m.child
        m = m.repeated_child.add()
        assert m.payload.optional_int32 == 0

    def test_set_repeated_composite(self, message_module):
        m = message_module.TestAllTypes()
        with pytest.raises(AttributeError):
            m.repeated_int32 = []
        m.repeated_int32.append(1)
        with pytest.raises(AttributeError):
           m.repeated_int32 = []

    def test_returning_type(self, message_module):
        m = message_module.TestAllTypes()
        assert float == type(m.optional_float)
        assert float == type(m.optional_double)
        assert bool == type(m.optional_bool)
        m.optional_float = 1
        m.optional_double = 1
        m.optional_bool = 1
        m.repeated_float.append(1)
        m.repeated_double.append(1)
        m.repeated_bool.append(1)
        m.ParseFromString(m.SerializeToString())
        assert float == type(m.optional_float)
        assert float == type(m.optional_double)
        assert '1.0' == str(m.optional_double)
        assert bool == type(m.optional_bool)
        assert float == type(m.repeated_float[0])
        assert float == type(m.repeated_double[0])
        assert bool == type(m.repeated_bool[0])
        assert True == m.repeated_bool[0]


# Class to test proto2-only features (required, extensions, etc.)
@testing_refleaks.TestCase
class TestProto2:
    def test_field_presence(self):
        message = unittest_pb2.TestAllTypes()

        assert not message.HasField('optional_int32')
        assert not message.HasField('optional_bool')
        assert not message.HasField('optional_nested_message')

        with pytest.raises(ValueError):
            message.HasField('field_doesnt_exist')

        with pytest.raises(ValueError):
            message.HasField('repeated_int32')
        with pytest.raises(ValueError):
            message.HasField('repeated_nested_message')

        assert 0 == message.optional_int32
        assert False == message.optional_bool
        assert 0 == message.optional_nested_message.bb

        # Fields are set even when setting the values to default values.
        message.optional_int32 = 0
        message.optional_bool = False
        message.optional_nested_message.bb = 0
        assert message.HasField('optional_int32')
        assert message.HasField('optional_bool')
        assert message.HasField('optional_nested_message')

        # Set the fields to non-default values.
        message.optional_int32 = 5
        message.optional_bool = True
        message.optional_nested_message.bb = 15

        assert message.HasField(u'optional_int32')
        assert message.HasField('optional_bool')
        assert message.HasField('optional_nested_message')

        # Clearing the fields unsets them and resets their value to default.
        message.ClearField('optional_int32')
        message.ClearField(u'optional_bool')
        message.ClearField('optional_nested_message')

        assert not message.HasField('optional_int32')
        assert not message.HasField('optional_bool')
        assert not message.HasField('optional_nested_message')
        assert 0 == message.optional_int32
        assert message.optional_bool is False
        assert 0 == message.optional_nested_message.bb

    def test_assign_invalid_enum(self):
        """Assigning an invalid enum number is not allowed in proto2."""
        m = unittest_pb2.TestAllTypes()

        # Proto2 can not assign unknown enum.
        with pytest.raises(ValueError):
            m.optional_nested_enum = 1234567
        pytest.raises(ValueError, m.repeated_nested_enum.append, 1234567)
        # Assignment is a different code path than append for the C++ impl.
        m.repeated_nested_enum.append(2)
        m.repeated_nested_enum[0] = 2
        with pytest.raises(ValueError):
          m.repeated_nested_enum[0] = 123456

        # Unknown enum value can be parsed but is ignored.
        m2 = unittest_proto3_arena_pb2.TestAllTypes()
        m2.optional_nested_enum = 1234567
        m2.repeated_nested_enum.append(7654321)
        serialized = m2.SerializeToString()

        m3 = unittest_pb2.TestAllTypes()
        m3.ParseFromString(serialized)
        assert not m3.HasField('optional_nested_enum')
        # 1 is the default value for optional_nested_enum.
        assert 1 == m3.optional_nested_enum
        assert 0 == len(m3.repeated_nested_enum)
        m2.Clear()
        m2.ParseFromString(m3.SerializeToString())
        assert 1234567 == m2.optional_nested_enum
        assert 7654321 == m2.repeated_nested_enum[0]

    def test_unknown_enum_map(self):
        m = map_proto2_unittest_pb2.TestEnumMap()
        m.known_map_field[123] = 0
        with pytest.raises(ValueError):
            m.unknown_map_field[1] = 123

    def test_extensions_errors(self):
        msg = unittest_pb2.TestAllTypes()
        pytest.raises(AttributeError, getattr, msg, 'Extensions')

    def test_merge_from_extensions(self):
        msg1 = more_extensions_pb2.TopLevelMessage()
        msg2 = more_extensions_pb2.TopLevelMessage()
        # Cpp extension will lazily create a sub message which is immutable.
        assert (
            0 ==
            msg1.submessage.Extensions[more_extensions_pb2.optional_int_extension])
        assert not msg1.HasField('submessage')
        msg2.submessage.Extensions[more_extensions_pb2.optional_int_extension] = 123
        # Make sure cmessage and extensions pointing to a mutable message
        # after merge instead of the lazily created message.
        msg1.MergeFrom(msg2)
        assert (
            123 ==
            msg1.submessage.Extensions[more_extensions_pb2.optional_int_extension])

    def test_copy_from_all(self):
        message = unittest_pb2.TestAllTypes()
        test_util.SetAllFields(message)
        copy = unittest_pb2.TestAllTypes()
        copy.CopyFrom(message)
        assert message == copy
        message.repeated_nested_message.add().bb = 123
        assert message != copy

    def test_copy_from_all_extensions(self):
        all_set = unittest_pb2.TestAllExtensions()
        test_util.SetAllExtensions(all_set)
        copy =  unittest_pb2.TestAllExtensions()
        copy.CopyFrom(all_set)
        assert all_set == copy
        all_set.Extensions[unittest_pb2.repeatedgroup_extension].add().a = 321
        assert all_set != copy

    def test_copy_from_all_packed_extensions(self):
        all_set = unittest_pb2.TestPackedExtensions()
        test_util.SetAllPackedExtensions(all_set)
        copy =  unittest_pb2.TestPackedExtensions()
        copy.CopyFrom(all_set)
        assert all_set == copy
        all_set.Extensions[unittest_pb2.packed_float_extension].extend([61.0, 71.0])
        assert all_set != copy

    def test_golden_extensions(self):
        golden_data = test_util.GoldenFileData('golden_message')
        golden_message = unittest_pb2.TestAllExtensions()
        golden_message.ParseFromString(golden_data)
        all_set = unittest_pb2.TestAllExtensions()
        test_util.SetAllExtensions(all_set)
        assert all_set == golden_message
        assert golden_data == golden_message.SerializeToString()
        golden_copy = copy.deepcopy(golden_message)
        assert golden_message == golden_copy
        # Depend on a specific serialization order for extensions is not
        # reasonable to guarantee.
        if api_implementation.Type() != 'upb':
            assert golden_data == golden_copy.SerializeToString()

    def test_golden_packed_extensions(self):
        golden_data = test_util.GoldenFileData('golden_packed_fields_message')
        golden_message = unittest_pb2.TestPackedExtensions()
        golden_message.ParseFromString(golden_data)
        all_set = unittest_pb2.TestPackedExtensions()
        test_util.SetAllPackedExtensions(all_set)
        assert all_set == golden_message
        assert golden_data == all_set.SerializeToString()
        golden_copy = copy.deepcopy(golden_message)
        assert golden_message == golden_copy
        # Depend on a specific serialization order for extensions is not
        # reasonable to guarantee.
        if api_implementation.Type() != 'upb':
            assert golden_data == golden_copy.SerializeToString()

    def test_pickle_incomplete_proto(self):
        golden_message = unittest_pb2.TestRequired(a=1)
        pickled_message = pickle.dumps(golden_message)

        unpickled_message = pickle.loads(pickled_message)
        assert unpickled_message == golden_message
        assert unpickled_message.a == 1
        # This is still an incomplete proto - so serializing should fail
        pytest.raises(message.EncodeError, unpickled_message.SerializeToString)

    # TODO(haberman): this isn't really a proto2-specific test except that this
    # message has a required field in it.  Should probably be factored out so
    # that we can test the other parts with proto3.
    def test_parsing_merge(self):
        """Check the merge behavior when a required or optional field appears

        multiple times in the input.
        """
        messages = [
            unittest_pb2.TestAllTypes(),
            unittest_pb2.TestAllTypes(),
            unittest_pb2.TestAllTypes()
        ]
        messages[0].optional_int32 = 1
        messages[1].optional_int64 = 2
        messages[2].optional_int32 = 3
        messages[2].optional_string = 'hello'

        merged_message = unittest_pb2.TestAllTypes()
        merged_message.optional_int32 = 3
        merged_message.optional_int64 = 2
        merged_message.optional_string = 'hello'

        generator = unittest_pb2.TestParsingMerge.RepeatedFieldsGenerator()
        generator.field1.extend(messages)
        generator.field2.extend(messages)
        generator.field3.extend(messages)
        generator.ext1.extend(messages)
        generator.ext2.extend(messages)
        generator.group1.add().field1.MergeFrom(messages[0])
        generator.group1.add().field1.MergeFrom(messages[1])
        generator.group1.add().field1.MergeFrom(messages[2])
        generator.group2.add().field1.MergeFrom(messages[0])
        generator.group2.add().field1.MergeFrom(messages[1])
        generator.group2.add().field1.MergeFrom(messages[2])

        data = generator.SerializeToString()
        parsing_merge = unittest_pb2.TestParsingMerge()
        parsing_merge.ParseFromString(data)

        # Required and optional fields should be merged.
        assert parsing_merge.required_all_types == merged_message
        assert parsing_merge.optional_all_types == merged_message
        assert (parsing_merge.optionalgroup.optional_group_all_types
                == merged_message)
        assert (
            parsing_merge.Extensions[unittest_pb2.TestParsingMerge.optional_ext] == merged_message)

        # Repeated fields should not be merged.
        assert len(parsing_merge.repeated_all_types) == 3
        assert len(parsing_merge.repeatedgroup) == 3
        assert (
            len(parsing_merge.Extensions[unittest_pb2.TestParsingMerge.repeated_ext]) == 3)

    def test_pythonic_init(self):
        message = unittest_pb2.TestAllTypes(
            optional_int32=100,
            optional_fixed32=200,
            optional_float=300.5,
            optional_bytes=b'x',
            optionalgroup={'a': 400},
            optional_nested_message={'bb': 500},
            optional_foreign_message={},
            optional_nested_enum='BAZ',
            repeatedgroup=[{
                'a': 600
            }, {
                'a': 700
            }],
            repeated_nested_enum=['FOO', unittest_pb2.TestAllTypes.BAR],
            default_int32=800,
            oneof_string='y')
        assert isinstance(message, unittest_pb2.TestAllTypes)
        assert 100 == message.optional_int32
        assert 200 == message.optional_fixed32
        assert 300.5 == message.optional_float
        assert b'x' == message.optional_bytes
        assert 400 == message.optionalgroup.a
        assert isinstance(message.optional_nested_message,
                          unittest_pb2.TestAllTypes.NestedMessage)
        assert 500 == message.optional_nested_message.bb
        assert message.HasField('optional_foreign_message')
        assert message.optional_foreign_message == unittest_pb2.ForeignMessage()
        assert unittest_pb2.TestAllTypes.BAZ == message.optional_nested_enum
        assert 2 == len(message.repeatedgroup)
        assert 600 == message.repeatedgroup[0].a
        assert 700 == message.repeatedgroup[1].a
        assert 2 == len(message.repeated_nested_enum)
        assert unittest_pb2.TestAllTypes.FOO == message.repeated_nested_enum[0]
        assert unittest_pb2.TestAllTypes.BAR == message.repeated_nested_enum[1]
        assert 800 == message.default_int32
        assert 'y' == message.oneof_string
        assert not message.HasField('optional_int64')
        assert 0 == len(message.repeated_float)
        assert 42 == message.default_int64

        message = unittest_pb2.TestAllTypes(optional_nested_enum=u'BAZ')
        assert unittest_pb2.TestAllTypes.BAZ == message.optional_nested_enum

        with pytest.raises(ValueError):
          unittest_pb2.TestAllTypes(
              optional_nested_message={'INVALID_NESTED_FIELD': 17})

        with pytest.raises(TypeError):
          unittest_pb2.TestAllTypes(
              optional_nested_message={'bb': 'INVALID_VALUE_TYPE'})

        with pytest.raises(ValueError):
          unittest_pb2.TestAllTypes(optional_nested_enum='INVALID_LABEL')

        with pytest.raises(ValueError):
          unittest_pb2.TestAllTypes(repeated_nested_enum='FOO')

    def test_pythonic_init_with_dict(self):
        # Both string/unicode field name keys should work.
        kwargs = {
            'optional_int32': 100,
            u'optional_fixed32': 200,
        }
        msg = unittest_pb2.TestAllTypes(**kwargs)
        assert 100 == msg.optional_int32
        assert 200 == msg.optional_fixed32

    def test_documentation(self):
        # Also used by the interactive help() function.
        doc = pydoc.html.document(unittest_pb2.TestAllTypes, 'message')
        assert 'class TestAllTypes' in doc
        assert 'SerializePartialToString' in doc
        assert 'repeated_float' in doc
        base = unittest_pb2.TestAllTypes.__bases__[0]
        pytest.raises(AttributeError, getattr, base, '_extensions_by_name')


# Class to test proto3-only features/behavior (updated field presence & enums)
@testing_refleaks.TestCase
class TestProto3:
    # Utility method for comparing equality with a map.
    def assert_map_iter_equals(self, map_iter, dict_value):
        # Avoid mutating caller's copy.
        dict_value = dict(dict_value)

        for k, v in map_iter:
          assert v == dict_value[k]
          del dict_value[k]

        assert {} == dict_value

    def test_field_presence(self):
        message = unittest_proto3_arena_pb2.TestAllTypes()

        # We can't test presence of non-repeated, non-submessage fields.
        with pytest.raises(ValueError):
            message.HasField('optional_int32')
        with pytest.raises(ValueError):
            message.HasField('optional_float')
        with pytest.raises(ValueError):
            message.HasField('optional_string')
        with pytest.raises(ValueError):
            message.HasField('optional_bool')

        # But we can still test presence of submessage fields.
        assert not message.HasField('optional_nested_message')

        # As with proto2, we can't test presence of fields that don't exist, or
        # repeated fields.
        with pytest.raises(ValueError):
            message.HasField('field_doesnt_exist')

        with pytest.raises(ValueError):
            message.HasField('repeated_int32')
        with pytest.raises(ValueError):
            message.HasField('repeated_nested_message')

        # Fields should default to their type-specific default.
        assert 0 == message.optional_int32
        assert 0 == message.optional_float
        assert '' == message.optional_string
        assert False == message.optional_bool
        assert 0 == message.optional_nested_message.bb

        # Setting a submessage should still return proper presence information.
        message.optional_nested_message.bb = 0
        assert message.HasField('optional_nested_message')

        # Set the fields to non-default values.
        message.optional_int32 = 5
        message.optional_float = 1.1
        message.optional_string = 'abc'
        message.optional_bool = True
        message.optional_nested_message.bb = 15

        # Clearing the fields unsets them and resets their value to default.
        message.ClearField('optional_int32')
        message.ClearField('optional_float')
        message.ClearField('optional_string')
        message.ClearField('optional_bool')
        message.ClearField('optional_nested_message')

        assert 0 == message.optional_int32
        assert 0 == message.optional_float
        assert '' == message.optional_string
        assert False == message.optional_bool
        assert 0 == message.optional_nested_message.bb

    def test_proto3_parser_drop_default_scalar(self):
        message_proto2 = unittest_pb2.TestAllTypes()
        message_proto2.optional_int32 = 0
        message_proto2.optional_string = ''
        message_proto2.optional_bytes = b''
        assert len(message_proto2.ListFields()) == 3

        message_proto3 = unittest_proto3_arena_pb2.TestAllTypes()
        message_proto3.ParseFromString(message_proto2.SerializeToString())
        assert len(message_proto3.ListFields()) == 0

    def test_proto3_optional(self):
        msg = test_proto3_optional_pb2.TestProto3Optional()
        assert not msg.HasField('optional_int32')
        assert not msg.HasField('optional_float')
        assert not msg.HasField('optional_string')
        assert not msg.HasField('optional_nested_message')
        assert not msg.optional_nested_message.HasField('bb')

        # Set fields.
        msg.optional_int32 = 1
        msg.optional_float = 1.0
        msg.optional_string = '123'
        msg.optional_nested_message.bb = 1
        assert msg.HasField('optional_int32')
        assert msg.HasField('optional_float')
        assert msg.HasField('optional_string')
        assert msg.HasField('optional_nested_message')
        assert msg.optional_nested_message.HasField('bb')
        # Set to default value does not clear the fields
        msg.optional_int32 = 0
        msg.optional_float = 0.0
        msg.optional_string = ''
        msg.optional_nested_message.bb = 0
        assert msg.HasField('optional_int32')
        assert msg.HasField('optional_float')
        assert msg.HasField('optional_string')
        assert msg.HasField('optional_nested_message')
        assert msg.optional_nested_message.HasField('bb')

        # Test serialize
        msg2 = test_proto3_optional_pb2.TestProto3Optional()
        msg2.ParseFromString(msg.SerializeToString())
        assert msg2.HasField('optional_int32')
        assert msg2.HasField('optional_float')
        assert msg2.HasField('optional_string')
        assert msg2.HasField('optional_nested_message')
        assert msg2.optional_nested_message.HasField('bb')

        assert msg.WhichOneof('_optional_int32') == 'optional_int32'

        # Clear these fields.
        msg.ClearField('optional_int32')
        msg.ClearField('optional_float')
        msg.ClearField('optional_string')
        msg.ClearField('optional_nested_message')
        assert not msg.HasField('optional_int32')
        assert not msg.HasField('optional_float')
        assert not msg.HasField('optional_string')
        assert not msg.HasField('optional_nested_message')
        assert not msg.optional_nested_message.HasField('bb')

        assert msg.WhichOneof('_optional_int32') == None

        # Test has presence:
        for field in test_proto3_optional_pb2.TestProto3Optional.DESCRIPTOR.fields:
            assert field.has_presence
        for field in unittest_pb2.TestAllTypes.DESCRIPTOR.fields:
            if field.label == descriptor.FieldDescriptor.LABEL_REPEATED:
                assert not field.has_presence
            else:
                assert field.has_presence
        proto3_descriptor = unittest_proto3_arena_pb2.TestAllTypes.DESCRIPTOR
        repeated_field = proto3_descriptor.fields_by_name['repeated_int32']
        assert not repeated_field.has_presence
        singular_field = proto3_descriptor.fields_by_name['optional_int32']
        assert not singular_field.has_presence
        optional_field = proto3_descriptor.fields_by_name['proto3_optional_int32']
        assert optional_field.has_presence
        message_field = proto3_descriptor.fields_by_name['optional_nested_message']
        assert message_field.has_presence
        oneof_field = proto3_descriptor.fields_by_name['oneof_uint32']
        assert oneof_field.has_presence

    def test_assign_unknown_enum(self):
        """Assigning an unknown enum value is allowed and preserves the value."""
        m = unittest_proto3_arena_pb2.TestAllTypes()

        # Proto3 can assign unknown enums.
        m.optional_nested_enum = 1234567
        assert 1234567 == m.optional_nested_enum
        m.repeated_nested_enum.append(22334455)
        assert 22334455 == m.repeated_nested_enum[0]
        # Assignment is a different code path than append for the C++ impl.
        m.repeated_nested_enum[0] = 7654321
        assert 7654321 == m.repeated_nested_enum[0]
        serialized = m.SerializeToString()

        m2 = unittest_proto3_arena_pb2.TestAllTypes()
        m2.ParseFromString(serialized)
        assert 1234567 == m2.optional_nested_enum
        assert 7654321 == m2.repeated_nested_enum[0]

    # Map isn't really a proto3-only feature. But there is no proto2 equivalent
    # of google/protobuf/map_unittest.proto right now, so it's not easy to
    # test both with the same test like we do for the other proto2/proto3 tests.
    # (google/protobuf/map_proto2_unittest.proto is very different in the set
    # of messages and fields it contains).
    def test_scalar_map_defaults(self):
        msg = map_unittest_pb2.TestMap()

        # Scalars start out unset.
        assert not -123 in msg.map_int32_int32
        assert not -2**33 in msg.map_int64_int64
        assert not 123 in msg.map_uint32_uint32
        assert not 2**33 in msg.map_uint64_uint64
        assert not 123 in msg.map_int32_double
        assert not False in msg.map_bool_bool
        assert not 'abc' in msg.map_string_string
        assert not 111 in msg.map_int32_bytes
        assert not 888 in msg.map_int32_enum

        # Accessing an unset key returns the default.
        assert 0 == msg.map_int32_int32[-123]
        assert 0 == msg.map_int64_int64[-2**33]
        assert 0 == msg.map_uint32_uint32[123]
        assert 0 == msg.map_uint64_uint64[2**33]
        assert 0.0 == msg.map_int32_double[123]
        assert isinstance(msg.map_int32_double[123], float)
        assert False == msg.map_bool_bool[False]
        assert isinstance(msg.map_bool_bool[False], bool)
        assert '' == msg.map_string_string['abc']
        assert b'' == msg.map_int32_bytes[111]
        assert 0 == msg.map_int32_enum[888]

        # It also sets the value in the map
        assert -123 in msg.map_int32_int32
        assert -2**33 in msg.map_int64_int64
        assert 123 in msg.map_uint32_uint32
        assert 2**33 in msg.map_uint64_uint64
        assert 123 in msg.map_int32_double
        assert False in msg.map_bool_bool
        assert 'abc' in msg.map_string_string
        assert 111 in msg.map_int32_bytes
        assert 888 in msg.map_int32_enum

        assert isinstance(msg.map_string_string['abc'], str)

        # Accessing an unset key still throws TypeError if the type of the key
        # is incorrect.
        with pytest.raises(TypeError):
            msg.map_string_string[123]

        with pytest.raises(TypeError):
            123 in msg.map_string_string

    def test_scalar_map_comparison(self):
        msg1 = map_unittest_pb2.TestMap()
        msg2 = map_unittest_pb2.TestMap()

        assert msg1.map_int32_int32 == msg2.map_int32_int32

    def test_message_map_comparison(self):
        msg1 = map_unittest_pb2.TestMap()
        msg2 = map_unittest_pb2.TestMap()

        assert msg1.map_int32_foreign_message == msg2.map_int32_foreign_message

    def test_map_get(self):
        # Need to test that get() properly returns the default, even though the dict
        # has defaultdict-like semantics.
        msg = map_unittest_pb2.TestMap()

        assert msg.map_int32_int32.get(5) is None
        assert 10 == msg.map_int32_int32.get(5, 10)
        assert 10 == msg.map_int32_int32.get(key=5, default=10)
        assert msg.map_int32_int32.get(5) is None

        msg.map_int32_int32[5] = 15
        assert 15 == msg.map_int32_int32.get(5)
        assert 15 == msg.map_int32_int32.get(5)
        with pytest.raises(TypeError):
            msg.map_int32_int32.get('')

        assert msg.map_int32_foreign_message.get(5) is None
        assert 10 == msg.map_int32_foreign_message.get(5, 10)
        assert 10 == msg.map_int32_foreign_message.get(key=5, default=10)

        submsg = msg.map_int32_foreign_message[5]
        assert submsg is msg.map_int32_foreign_message.get(5)
        with pytest.raises(TypeError):
            msg.map_int32_foreign_message.get('')

    def test_scalar_map(self):
        msg = map_unittest_pb2.TestMap()

        assert 0 == len(msg.map_int32_int32)
        assert not 5 in msg.map_int32_int32

        msg.map_int32_int32[-123] = -456
        msg.map_int64_int64[-2**33] = -2**34
        msg.map_uint32_uint32[123] = 456
        msg.map_uint64_uint64[2**33] = 2**34
        msg.map_int32_float[2] = 1.2
        msg.map_int32_double[1] = 3.3
        msg.map_string_string['abc'] = '123'
        msg.map_bool_bool[True] = True
        msg.map_int32_enum[888] = 2
        # Unknown numeric enum is supported in proto3.
        msg.map_int32_enum[123] = 456

        assert [] == msg.FindInitializationErrors()

        assert 1 == len(msg.map_string_string)

        # Bad key.
        with pytest.raises(TypeError):
            msg.map_string_string[123] = '123'

        # Verify that trying to assign a bad key doesn't actually add a member to
        # the map.
        assert 1 == len(msg.map_string_string)

        # Bad value.
        with pytest.raises(TypeError):
            msg.map_string_string['123'] = 123

        serialized = msg.SerializeToString()
        msg2 = map_unittest_pb2.TestMap()
        msg2.ParseFromString(serialized)

        # Bad key.
        with pytest.raises(TypeError):
            msg2.map_string_string[123] = '123'

        # Bad value.
        with pytest.raises(TypeError):
            msg2.map_string_string['123'] = 123

        assert -456 == msg2.map_int32_int32[-123]
        assert -2**34 == msg2.map_int64_int64[-2**33]
        assert 456 == msg2.map_uint32_uint32[123]
        assert 2**34 == msg2.map_uint64_uint64[2**33]
        assert 1.2 == pytest.approx(msg.map_int32_float[2])
        assert 3.3 == msg.map_int32_double[1]
        assert '123' == msg2.map_string_string['abc']
        assert True == msg2.map_bool_bool[True]
        assert 2 == msg2.map_int32_enum[888]
        assert 456 == msg2.map_int32_enum[123]
        assert '{-123: -456}' == str(msg2.map_int32_int32)

    def test_map_entry_always_serialized(self):
        msg = map_unittest_pb2.TestMap()
        msg.map_int32_int32[0] = 0
        msg.map_string_string[''] = ''
        assert msg.ByteSize() == 12
        assert (b'\n\x04\x08\x00\x10\x00r\x04\n\x00\x12\x00'
                == msg.SerializeToString())

    def test_string_unicode_conversion_in_map(self):
        msg = map_unittest_pb2.TestMap()

        unicode_obj = u'\u1234'
        bytes_obj = unicode_obj.encode('utf8')

        msg.map_string_string[bytes_obj] = bytes_obj

        (key, value) = list(msg.map_string_string.items())[0]

        assert key == unicode_obj
        assert value == unicode_obj

        assert isinstance(key, str)
        assert isinstance(value, str)

    def test_message_map(self):
        msg = map_unittest_pb2.TestMap()

        assert 0 == len(msg.map_int32_foreign_message)
        assert not 5 in msg.map_int32_foreign_message

        msg.map_int32_foreign_message[123]
        # get_or_create() is an alias for getitem.
        msg.map_int32_foreign_message.get_or_create(-456)

        assert 2 == len(msg.map_int32_foreign_message)
        assert 123 in msg.map_int32_foreign_message
        assert -456 in msg.map_int32_foreign_message
        assert 2 == len(msg.map_int32_foreign_message)

        # Bad key.
        with pytest.raises(TypeError):
          msg.map_int32_foreign_message['123']

        # Can't assign directly to submessage.
        with pytest.raises(ValueError):
          msg.map_int32_foreign_message[999] = msg.map_int32_foreign_message[123]

        # Verify that trying to assign a bad key doesn't actually add a member to
        # the map.
        assert 2 == len(msg.map_int32_foreign_message)

        serialized = msg.SerializeToString()
        msg2 = map_unittest_pb2.TestMap()
        msg2.ParseFromString(serialized)

        assert 2 == len(msg2.map_int32_foreign_message)
        assert 123 in msg2.map_int32_foreign_message
        assert -456 in msg2.map_int32_foreign_message
        assert 2 == len(msg2.map_int32_foreign_message)
        msg2.map_int32_foreign_message[123].c = 1
        # TODO(jieluo): Fix text format for message map.
        assert (
            str(msg2.map_int32_foreign_message)
            in ('{-456: , 123: c: 1\n}', '{123: c: 1\n, -456: }'))

    def test_nested_message_map_item_delete(self):
        msg = map_unittest_pb2.TestMap()
        msg.map_int32_all_types[1].optional_nested_message.bb = 1
        del msg.map_int32_all_types[1]
        msg.map_int32_all_types[2].optional_nested_message.bb = 2
        assert 1 == len(msg.map_int32_all_types)
        msg.map_int32_all_types[1].optional_nested_message.bb = 1
        assert 2 == len(msg.map_int32_all_types)

        serialized = msg.SerializeToString()
        msg2 = map_unittest_pb2.TestMap()
        msg2.ParseFromString(serialized)
        keys = [1, 2]
        # The loop triggers PyErr_Occurred() in c extension.
        for key in keys:
            del msg2.map_int32_all_types[key]

    def test_map_byte_size(self):
        msg = map_unittest_pb2.TestMap()
        msg.map_int32_int32[1] = 1
        size = msg.ByteSize()
        msg.map_int32_int32[1] = 128
        assert msg.ByteSize() == size + 1

        msg.map_int32_foreign_message[19].c = 1
        size = msg.ByteSize()
        msg.map_int32_foreign_message[19].c = 128
        assert msg.ByteSize() == size + 1

    def test_merge_from(self):
        msg = map_unittest_pb2.TestMap()
        msg.map_int32_int32[12] = 34
        msg.map_int32_int32[56] = 78
        msg.map_int64_int64[22] = 33
        msg.map_int32_foreign_message[111].c = 5
        msg.map_int32_foreign_message[222].c = 10

        msg2 = map_unittest_pb2.TestMap()
        msg2.map_int32_int32[12] = 55
        msg2.map_int64_int64[88] = 99
        msg2.map_int32_foreign_message[222].c = 15
        msg2.map_int32_foreign_message[222].d = 20
        old_map_value = msg2.map_int32_foreign_message[222]

        msg2.MergeFrom(msg)
        # Compare with expected message instead of call
        # msg2.map_int32_foreign_message[222] to make sure MergeFrom does not
        # sync with repeated field and there is no duplicated keys.
        expected_msg = map_unittest_pb2.TestMap()
        expected_msg.CopyFrom(msg)
        expected_msg.map_int64_int64[88] = 99
        assert msg2 == expected_msg

        assert 34 == msg2.map_int32_int32[12]
        assert 78 == msg2.map_int32_int32[56]
        assert 33 == msg2.map_int64_int64[22]
        assert 99 == msg2.map_int64_int64[88]
        assert 5 == msg2.map_int32_foreign_message[111].c
        assert 10 == msg2.map_int32_foreign_message[222].c
        assert not msg2.map_int32_foreign_message[222].HasField('d')
        if api_implementation.Type() != 'cpp':
            # During the call to MergeFrom(), the C++ implementation will have
            # deallocated the underlying message, but this is very difficult to detect
            # properly. The line below is likely to cause a segmentation fault.
            # With the Python implementation, old_map_value is just 'detached' from
            # the main message. Using it will not crash of course, but since it still
            # have a reference to the parent message I'm sure we can find interesting
            # ways to cause inconsistencies.
            assert 15 == old_map_value.c

        # Verify that there is only one entry per key, even though the MergeFrom
        # may have internally created multiple entries for a single key in the
        # list representation.
        as_dict = {}
        for key in msg2.map_int32_foreign_message:
            assert not key in as_dict
            as_dict[key] = msg2.map_int32_foreign_message[key].c

        assert {111: 5, 222: 10} == as_dict

        # Special case: test that delete of item really removes the item, even if
        # there might have physically been duplicate keys due to the previous merge.
        # This is only a special case for the C++ implementation which stores the
        # map as an array.
        del msg2.map_int32_int32[12]
        assert not 12 in msg2.map_int32_int32

        del msg2.map_int32_foreign_message[222]
        assert not 222 in msg2.map_int32_foreign_message
        with pytest.raises(TypeError):
            del msg2.map_int32_foreign_message['']

    def test_map_merge_from(self):
        msg = map_unittest_pb2.TestMap()
        msg.map_int32_int32[12] = 34
        msg.map_int32_int32[56] = 78
        msg.map_int64_int64[22] = 33
        msg.map_int32_foreign_message[111].c = 5
        msg.map_int32_foreign_message[222].c = 10

        msg2 = map_unittest_pb2.TestMap()
        msg2.map_int32_int32[12] = 55
        msg2.map_int64_int64[88] = 99
        msg2.map_int32_foreign_message[222].c = 15
        msg2.map_int32_foreign_message[222].d = 20

        msg2.map_int32_int32.MergeFrom(msg.map_int32_int32)
        assert 34 == msg2.map_int32_int32[12]
        assert 78 == msg2.map_int32_int32[56]

        msg2.map_int64_int64.MergeFrom(msg.map_int64_int64)
        assert 33 == msg2.map_int64_int64[22]
        assert 99 == msg2.map_int64_int64[88]

        msg2.map_int32_foreign_message.MergeFrom(msg.map_int32_foreign_message)
        # Compare with expected message instead of call
        # msg.map_int32_foreign_message[222] to make sure MergeFrom does not
        # sync with repeated field and no duplicated keys.
        expected_msg = map_unittest_pb2.TestMap()
        expected_msg.CopyFrom(msg)
        expected_msg.map_int64_int64[88] = 99
        assert msg2 == expected_msg

        # Test when cpp extension cache a map.
        m1 = map_unittest_pb2.TestMap()
        m2 = map_unittest_pb2.TestMap()
        assert m1.map_int32_foreign_message == m1.map_int32_foreign_message
        m2.map_int32_foreign_message[123].c = 10
        m1.MergeFrom(m2)
        assert 10 == m2.map_int32_foreign_message[123].c

        # Test merge maps within different message types.
        m1 = map_unittest_pb2.TestMap()
        m2 = map_unittest_pb2.TestMessageMap()
        m2.map_int32_message[123].optional_int32 = 10
        m1.map_int32_all_types.MergeFrom(m2.map_int32_message)
        assert 10 == m1.map_int32_all_types[123].optional_int32

        # Test overwrite message value map
        msg = map_unittest_pb2.TestMap()
        msg.map_int32_foreign_message[222].c = 123
        msg2 = map_unittest_pb2.TestMap()
        msg2.map_int32_foreign_message[222].d = 20
        msg.MergeFromString(msg2.SerializeToString())
        assert msg.map_int32_foreign_message[222].d == 20
        assert msg.map_int32_foreign_message[222].c != 123

        # Merge a dict to map field is not accepted
        with pytest.raises(AttributeError):
            m1.map_int32_all_types.MergeFrom(
                {1: unittest_proto3_arena_pb2.TestAllTypes()})

    def test_merge_from_bad_type(self):
        msg = map_unittest_pb2.TestMap()
        with pytest.raises(
            TypeError,
            match=r'Parameter to MergeFrom\(\) must be instance of same class: expected '
            r'.+TestMap got int\.'):
          msg.MergeFrom(1)

    def test_copy_from_bad_type(self):
        msg = map_unittest_pb2.TestMap()
        with pytest.raises(
              TypeError,
              match='Parameter to [A-Za-z]*From\(\) must be instance of same class: '
              r'expected .+TestMap got int\.'):
          msg.CopyFrom(1)

    def test_integer_map_with_longs(self):
        msg = map_unittest_pb2.TestMap()
        msg.map_int32_int32[int(-123)] = int(-456)
        msg.map_int64_int64[int(-2**33)] = int(-2**34)
        msg.map_uint32_uint32[int(123)] = int(456)
        msg.map_uint64_uint64[int(2**33)] = int(2**34)

        serialized = msg.SerializeToString()
        msg2 = map_unittest_pb2.TestMap()
        msg2.ParseFromString(serialized)

        assert -456 == msg2.map_int32_int32[-123]
        assert -2**34 == msg2.map_int64_int64[-2**33]
        assert 456 == msg2.map_uint32_uint32[123]
        assert 2**34 == msg2.map_uint64_uint64[2**33]

    def test_map_assignment_causes_presence(self):
        msg = map_unittest_pb2.TestMapSubmessage()
        msg.test_map.map_int32_int32[123] = 456

        serialized = msg.SerializeToString()
        msg2 = map_unittest_pb2.TestMapSubmessage()
        msg2.ParseFromString(serialized)

        assert msg == msg2

        # Now test that various mutations of the map properly invalidate the
        # cached size of the submessage.
        msg.test_map.map_int32_int32[888] = 999
        serialized = msg.SerializeToString()
        msg2.ParseFromString(serialized)
        assert msg == msg2

        msg.test_map.map_int32_int32.clear()
        serialized = msg.SerializeToString()
        msg2.ParseFromString(serialized)
        assert msg == msg2

    def test_map_assignment_causes_presence_for_submessages(self):
        msg = map_unittest_pb2.TestMapSubmessage()
        msg.test_map.map_int32_foreign_message[123].c = 5

        serialized = msg.SerializeToString()
        msg2 = map_unittest_pb2.TestMapSubmessage()
        msg2.ParseFromString(serialized)

        assert msg == msg2

        # Now test that various mutations of the map properly invalidate the
        # cached size of the submessage.
        msg.test_map.map_int32_foreign_message[888].c = 7
        serialized = msg.SerializeToString()
        msg2.ParseFromString(serialized)
        assert msg == msg2

        msg.test_map.map_int32_foreign_message[888].MergeFrom(
            msg.test_map.map_int32_foreign_message[123])
        serialized = msg.SerializeToString()
        msg2.ParseFromString(serialized)
        assert msg == msg2

        msg.test_map.map_int32_foreign_message.clear()
        serialized = msg.SerializeToString()
        msg2.ParseFromString(serialized)
        assert msg == msg2

    def test_modify_map_while_iterating(self):
        msg = map_unittest_pb2.TestMap()

        string_string_iter = iter(msg.map_string_string)
        int32_foreign_iter = iter(msg.map_int32_foreign_message)

        msg.map_string_string['abc'] = '123'
        msg.map_int32_foreign_message[5].c = 5

        with pytest.raises(RuntimeError):
            for key in string_string_iter:
                pass

        with pytest.raises(RuntimeError):
            for key in int32_foreign_iter:
                pass

    def test_modify_map_entry_while_iterating(self):
        msg = map_unittest_pb2.TestMap()

        msg.map_string_string['abc'] = '123'
        msg.map_string_string['def'] = '456'
        msg.map_string_string['ghi'] = '789'

        msg.map_int32_foreign_message[5].c = 5
        msg.map_int32_foreign_message[6].c = 6
        msg.map_int32_foreign_message[7].c = 7

        string_string_keys = list(msg.map_string_string.keys())
        int32_foreign_keys = list(msg.map_int32_foreign_message.keys())

        keys = []
        for key in msg.map_string_string:
            keys.append(key)
            msg.map_string_string[key] = '000'
        assert keys == string_string_keys
        assert keys == list(msg.map_string_string.keys())

        keys = []
        for key in msg.map_int32_foreign_message:
            keys.append(key)
            msg.map_int32_foreign_message[key].c = 0
        assert keys == int32_foreign_keys
        assert keys == list(msg.map_int32_foreign_message.keys())

    def test_submessage_map(self):
        msg = map_unittest_pb2.TestMap()

        submsg = msg.map_int32_foreign_message[111]
        assert submsg is msg.map_int32_foreign_message[111]
        assert isinstance(submsg, unittest_pb2.ForeignMessage)

        submsg.c = 5

        serialized = msg.SerializeToString()
        msg2 = map_unittest_pb2.TestMap()
        msg2.ParseFromString(serialized)

        assert 5 == msg2.map_int32_foreign_message[111].c

        # Doesn't allow direct submessage assignment.
        with pytest.raises(ValueError):
            msg.map_int32_foreign_message[88] = unittest_pb2.ForeignMessage()

    def test_map_iteration(self):
        msg = map_unittest_pb2.TestMap()

        for k, v in msg.map_int32_int32.items():
            # Should not be reached.
            assert False

        msg.map_int32_int32[2] = 4
        msg.map_int32_int32[3] = 6
        msg.map_int32_int32[4] = 8
        assert 3 == len(msg.map_int32_int32)

        matching_dict = {2: 4, 3: 6, 4: 8}
        self.assert_map_iter_equals(msg.map_int32_int32.items(), matching_dict)

    def test_map_items(self):
        # Map items used to have strange behaviors when use c extension. Because
        # [] may reorder the map and invalidate any existing iterators.
        # TODO(jieluo): Check if [] reordering the map is a bug or intended
        # behavior.
        msg = map_unittest_pb2.TestMap()
        msg.map_string_string['local_init_op'] = ''
        msg.map_string_string['trainable_variables'] = ''
        msg.map_string_string['variables'] = ''
        msg.map_string_string['init_op'] = ''
        msg.map_string_string['summaries'] = ''
        items1 = msg.map_string_string.items()
        items2 = msg.map_string_string.items()
        assert items1 == items2

    def test_map_deterministic_serialization(self):
        golden_data = (b'r\x0c\n\x07init_op\x12\x01d'
                      b'r\n\n\x05item1\x12\x01e'
                      b'r\n\n\x05item2\x12\x01f'
                      b'r\n\n\x05item3\x12\x01g'
                      b'r\x0b\n\x05item4\x12\x02QQ'
                      b'r\x12\n\rlocal_init_op\x12\x01a'
                      b'r\x0e\n\tsummaries\x12\x01e'
                      b'r\x18\n\x13trainable_variables\x12\x01b'
                      b'r\x0e\n\tvariables\x12\x01c')
        msg = map_unittest_pb2.TestMap()
        msg.map_string_string['local_init_op'] = 'a'
        msg.map_string_string['trainable_variables'] = 'b'
        msg.map_string_string['variables'] = 'c'
        msg.map_string_string['init_op'] = 'd'
        msg.map_string_string['summaries'] = 'e'
        msg.map_string_string['item1'] = 'e'
        msg.map_string_string['item2'] = 'f'
        msg.map_string_string['item3'] = 'g'
        msg.map_string_string['item4'] = 'QQ'

        # If deterministic serialization is not working correctly, this will be
        # "flaky" depending on the exact python dict hash seed.
        #
        # Fortunately, there are enough items in this map that it is extremely
        # unlikely to ever hit the "right" in-order combination, so the test
        # itself should fail reliably.
        assert golden_data == msg.SerializeToString(deterministic=True)

    def test_map_iteration_clear_message(self):
        # Iterator needs to work even if message and map are deleted.
        msg = map_unittest_pb2.TestMap()

        msg.map_int32_int32[2] = 4
        msg.map_int32_int32[3] = 6
        msg.map_int32_int32[4] = 8

        it = msg.map_int32_int32.items()
        del msg

        matching_dict = {2: 4, 3: 6, 4: 8}
        self.assert_map_iter_equals(it, matching_dict)

    def test_map_construction(self):
        msg = map_unittest_pb2.TestMap(map_int32_int32={1: 2, 3: 4})
        assert 2 == msg.map_int32_int32[1]
        assert 4 == msg.map_int32_int32[3]

        msg = map_unittest_pb2.TestMap(
            map_int32_foreign_message={3: unittest_pb2.ForeignMessage(c=5)})
        assert 5 == msg.map_int32_foreign_message[3].c

    def test_map_scalar_field_construction(self):
        msg1 = map_unittest_pb2.TestMap()
        msg1.map_int32_int32[1] = 42
        msg2 = map_unittest_pb2.TestMap(map_int32_int32=msg1.map_int32_int32)
        assert 42 == msg2.map_int32_int32[1]

    def test_map_message_field_construction(self):
        msg1 = map_unittest_pb2.TestMap()
        msg1.map_string_foreign_message['test'].c = 42
        msg2 = map_unittest_pb2.TestMap(
            map_string_foreign_message=msg1.map_string_foreign_message)
        assert 42 == msg2.map_string_foreign_message['test'].c

    def test_map_field_raises_correct_error(self):
        # Should raise a TypeError when given a non-iterable.
        with pytest.raises(TypeError):
            map_unittest_pb2.TestMap(map_string_foreign_message=1)

    def test_map_valid_after_field_cleared(self):
        # Map needs to work even if field is cleared.
        # For the C++ implementation this tests the correctness of
        # MapContainer::Release()
        msg = map_unittest_pb2.TestMap()
        int32_map = msg.map_int32_int32

        int32_map[2] = 4
        int32_map[3] = 6
        int32_map[4] = 8

        msg.ClearField('map_int32_int32')
        assert b'' == msg.SerializeToString()
        matching_dict = {2: 4, 3: 6, 4: 8}
        self.assert_map_iter_equals(int32_map.items(), matching_dict)

    def test_message_map_valid_after_field_cleared(self):
        # Map needs to work even if field is cleared.
        # For the C++ implementation this tests the correctness of
        # MapContainer::Release()
        msg = map_unittest_pb2.TestMap()
        int32_foreign_message = msg.map_int32_foreign_message

        int32_foreign_message[2].c = 5

        msg.ClearField('map_int32_foreign_message')
        assert b'' == msg.SerializeToString()
        assert 2 in int32_foreign_message.keys()

    def test_message_map_item_valid_after_top_message_cleared(self):
        # Message map item needs to work even if it is cleared.
        # For the C++ implementation this tests the correctness of
        # MapContainer::Release()
        msg = map_unittest_pb2.TestMap()
        msg.map_int32_all_types[2].optional_string = 'bar'

        if api_implementation.Type() == 'cpp':
            # Need to keep the map reference because of b/27942626.
            # TODO(jieluo): Remove it.
            unused_map = msg.map_int32_all_types  # pylint: disable=unused-variable
        msg_value = msg.map_int32_all_types[2]
        msg.Clear()

        # Reset to trigger sync between repeated field and map in c++.
        msg.map_int32_all_types[3].optional_string = 'foo'
        assert msg_value.optional_string == 'bar'

    def test_map_iter_invalidated_by_clear_field(self):
        # Map iterator is invalidated when field is cleared.
        # But this case does need to not crash the interpreter.
        # For the C++ implementation this tests the correctness of
        # ScalarMapContainer::Release()
        msg = map_unittest_pb2.TestMap()

        it = iter(msg.map_int32_int32)

        msg.ClearField('map_int32_int32')
        with pytest.raises(RuntimeError):
            for _ in it:
                pass

        it = iter(msg.map_int32_foreign_message)
        msg.ClearField('map_int32_foreign_message')
        with pytest.raises(RuntimeError):
            for _ in it:
                pass

    def test_map_delete(self):
        msg = map_unittest_pb2.TestMap()

        assert 0 == len(msg.map_int32_int32)

        msg.map_int32_int32[4] = 6
        assert 1 == len(msg.map_int32_int32)

        with pytest.raises(KeyError):
            del msg.map_int32_int32[88]

        del msg.map_int32_int32[4]
        assert 0 == len(msg.map_int32_int32)

        with pytest.raises(KeyError):
            del msg.map_int32_all_types[32]

    def test_maps_are_mapping(self):
        msg = map_unittest_pb2.TestMap()
        assert isinstance(msg.map_int32_int32, collections.abc.Mapping)
        assert isinstance(msg.map_int32_int32, collections.abc.MutableMapping)
        assert isinstance(msg.map_int32_foreign_message, collections.abc.Mapping)
        assert isinstance(msg.map_int32_foreign_message, collections.abc.MutableMapping)

    def test_maps_compare(self):
        msg = map_unittest_pb2.TestMap()
        msg.map_int32_int32[-123] = -456
        assert msg.map_int32_int32 == msg.map_int32_int32
        assert msg.map_int32_foreign_message == msg.map_int32_foreign_message
        assert msg.map_int32_int32 != 0

    def test_map_find_initialization_errors_smoke_test(self):
        msg = map_unittest_pb2.TestMap()
        msg.map_string_string['abc'] = '123'
        msg.map_int32_int32[35] = 64
        msg.map_string_foreign_message['foo'].c = 5
        assert 0 == len(msg.FindInitializationErrors())

    @pytest.mark.skipif(sys.maxunicode == UCS2_MAXUNICODE, reason='Skip for ucs2')
    def test_strict_utf8_check(self):
        # Test u'\ud801' is rejected at parser in both python2 and python3.
        serialized = (b'r\x03\xed\xa0\x81')
        msg = unittest_proto3_arena_pb2.TestAllTypes()
        with pytest.raises(Exception) as context:
            msg.MergeFromString(serialized)
        if api_implementation.Type() == 'python':
            assert 'optional_string' in str(context.value)
        else:
            assert 'Error parsing message' in str(context.value)

        # Test optional_string='😍' is accepted.
        serialized = unittest_proto3_arena_pb2.TestAllTypes(
            optional_string='😍').SerializeToString()
        msg2 = unittest_proto3_arena_pb2.TestAllTypes()
        msg2.MergeFromString(serialized)
        assert msg2.optional_string == '😍'

        msg = unittest_proto3_arena_pb2.TestAllTypes(optional_string=u'\ud001')
        assert msg.optional_string == '\ud001'

    def test_surrogates_in_python3(self):
        # Surrogates are rejected at setters in Python3.
        with pytest.raises(ValueError):
            unittest_proto3_arena_pb2.TestAllTypes(optional_string='\ud801\udc01')
        with pytest.raises(ValueError):
            unittest_proto3_arena_pb2.TestAllTypes(optional_string=b'\xed\xa0\x81')
        with pytest.raises(ValueError):
            unittest_proto3_arena_pb2.TestAllTypes(optional_string='\ud801')
        with pytest.raises(ValueError):
            unittest_proto3_arena_pb2.TestAllTypes(optional_string='\ud801\ud801')

    def test_crash_null_aa(self):
        assert (
            unittest_proto3_arena_pb2.TestAllTypes.NestedMessage()
            == unittest_proto3_arena_pb2.TestAllTypes.NestedMessage())

    def test_crash_null_ab(self):
        assert (
            unittest_proto3_arena_pb2.TestAllTypes.NestedMessage()
            == unittest_proto3_arena_pb2.TestAllTypes().optional_nested_message)

    def test_crash_null_ba(self):
        assert (
            unittest_proto3_arena_pb2.TestAllTypes().optional_nested_message
            == unittest_proto3_arena_pb2.TestAllTypes.NestedMessage())

    def test_crash_null_bb(self):
        assert (
            unittest_proto3_arena_pb2.TestAllTypes().optional_nested_message
            == unittest_proto3_arena_pb2.TestAllTypes().optional_nested_message)


@testing_refleaks.TestCase
class ValidTypeNamesTest(unittest.TestCase):

  def assertImportFromName(self, msg, base_name):
    # Parse <type 'module.class_name'> to extra 'some.name' as a string.
    tp_name = str(type(msg)).split("'")[1]
    valid_names = ('Repeated%sContainer' % base_name,
                   'Repeated%sFieldContainer' % base_name)
    assert any(tp_name.endswith(v) for v in valid_names), (
        '%r does end with any of %r' % (tp_name, valid_names))

    parts = tp_name.split('.')
    class_name = parts[-1]
    module_name = '.'.join(parts[:-1])
    __import__(module_name, fromlist=[class_name])

  def testTypeNamesCanBeImported(self):
    # If import doesn't work, pickling won't work either.
    pb = unittest_pb2.TestAllTypes()
    self.assertImportFromName(pb.repeated_int32, 'Scalar')
    self.assertImportFromName(pb.repeated_nested_message, 'Composite')


@testing_refleaks.TestCase
class PackedFieldTest(unittest.TestCase):

  def setMessage(self, message):
    message.repeated_int32.append(1)
    message.repeated_int64.append(1)
    message.repeated_uint32.append(1)
    message.repeated_uint64.append(1)
    message.repeated_sint32.append(1)
    message.repeated_sint64.append(1)
    message.repeated_fixed32.append(1)
    message.repeated_fixed64.append(1)
    message.repeated_sfixed32.append(1)
    message.repeated_sfixed64.append(1)
    message.repeated_float.append(1.0)
    message.repeated_double.append(1.0)
    message.repeated_bool.append(True)
    message.repeated_nested_enum.append(1)

  def testPackedFields(self):
    message = packed_field_test_pb2.TestPackedTypes()
    self.setMessage(message)
    golden_data = (b'\x0A\x01\x01'
                   b'\x12\x01\x01'
                   b'\x1A\x01\x01'
                   b'\x22\x01\x01'
                   b'\x2A\x01\x02'
                   b'\x32\x01\x02'
                   b'\x3A\x04\x01\x00\x00\x00'
                   b'\x42\x08\x01\x00\x00\x00\x00\x00\x00\x00'
                   b'\x4A\x04\x01\x00\x00\x00'
                   b'\x52\x08\x01\x00\x00\x00\x00\x00\x00\x00'
                   b'\x5A\x04\x00\x00\x80\x3f'
                   b'\x62\x08\x00\x00\x00\x00\x00\x00\xf0\x3f'
                   b'\x6A\x01\x01'
                   b'\x72\x01\x01')
    assert golden_data == message.SerializeToString()

  def testUnpackedFields(self):
    message = packed_field_test_pb2.TestUnpackedTypes()
    self.setMessage(message)
    golden_data = (b'\x08\x01'
                   b'\x10\x01'
                   b'\x18\x01'
                   b'\x20\x01'
                   b'\x28\x02'
                   b'\x30\x02'
                   b'\x3D\x01\x00\x00\x00'
                   b'\x41\x01\x00\x00\x00\x00\x00\x00\x00'
                   b'\x4D\x01\x00\x00\x00'
                   b'\x51\x01\x00\x00\x00\x00\x00\x00\x00'
                   b'\x5D\x00\x00\x80\x3f'
                   b'\x61\x00\x00\x00\x00\x00\x00\xf0\x3f'
                   b'\x68\x01'
                   b'\x70\x01')
    assert golden_data == message.SerializeToString()


@unittest.skipIf(api_implementation.Type() == 'python',
                 'explicit tests of the C++ implementation')
@testing_refleaks.TestCase
class OversizeProtosTest(unittest.TestCase):

  def GenerateNestedProto(self, n):
    msg = unittest_pb2.TestRecursiveMessage()
    sub = msg
    for _ in range(n):
      sub = sub.a
    sub.i = 0
    return msg.SerializeToString()

  def testSucceedOkSizedProto(self):
    msg = unittest_pb2.TestRecursiveMessage()
    msg.ParseFromString(self.GenerateNestedProto(100))

  def testAssertOversizeProto(self):
    api_implementation._c_module.SetAllowOversizeProtos(False)
    msg = unittest_pb2.TestRecursiveMessage()
    with pytest.raises(message.DecodeError) as context:
      msg.ParseFromString(self.GenerateNestedProto(101))
    assert 'Error parsing message' in str(context.value)

  def testSucceedOversizeProto(self):
    api_implementation._c_module.SetAllowOversizeProtos(True)
    msg = unittest_pb2.TestRecursiveMessage()
    msg.ParseFromString(self.GenerateNestedProto(101))
