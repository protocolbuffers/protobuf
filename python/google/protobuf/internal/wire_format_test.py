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

"""Test for google.protobuf.internal.wire_format."""

__author__ = 'robinson@google.com (Will Robinson)'

import pytest

from google.protobuf import message
from google.protobuf.internal import wire_format


class TestWireFormat:
    def test_pack_tag(self):
        field_number = 0xabc
        tag_type = 2
        assert ((field_number << 3) | tag_type ==
                        wire_format.PackTag(field_number, tag_type))
        PackTag = wire_format.PackTag
        # Number too high.
        with pytest.raises(message.EncodeError):
            PackTag(field_number, 6)
        # Number too low.
        with pytest.raises(message.EncodeError):
            PackTag(field_number, -1)

    def test_unpack_tag(self):
        # Test field numbers that will require various varint sizes.
        for expected_field_number in (1, 15, 16, 2047, 2048):
            # Highest-numbered wiretype is 5.
            for expected_wire_type in range(6):
                field_number, wire_type = wire_format.UnpackTag(
                    wire_format.PackTag(expected_field_number, expected_wire_type))
                assert expected_field_number == field_number
                assert expected_wire_type == wire_type

        with pytest.raises(TypeError):
            wire_format.UnpackTag(None)
        with pytest.raises(TypeError):
            wire_format.UnpackTag('abc')
        with pytest.raises(TypeError):
            wire_format.UnpackTag(0.0)
        with pytest.raises(TypeError):
            wire_format.UnpackTag(object())

    def test_zig_zag_encode(self):
        Z = wire_format.ZigZagEncode
        assert 0 == Z(0)
        assert 1 == Z(-1)
        assert 2 == Z(1)
        assert 3 == Z(-2)
        assert 4 == Z(2)
        assert 0xfffffffe == Z(0x7fffffff)
        assert 0xffffffff == Z(-0x80000000)
        assert 0xfffffffffffffffe == Z(0x7fffffffffffffff)
        assert 0xffffffffffffffff == Z(-0x8000000000000000)

        with pytest.raises(TypeError):
            Z(None)
        with pytest.raises(TypeError):
            Z('abcd')
        with pytest.raises(TypeError):
            Z(0.0)
        with pytest.raises(TypeError):
            Z(object())

    def test_zig_zag_decode(self):
        Z = wire_format.ZigZagDecode
        assert 0 == Z(0)
        assert -1 == Z(1)
        assert 1 == Z(2)
        assert -2 == Z(3)
        assert 2 == Z(4)
        assert 0x7fffffff == Z(0xfffffffe)
        assert -0x80000000 == Z(0xffffffff)
        assert 0x7fffffffffffffff == Z(0xfffffffffffffffe)
        assert -0x8000000000000000 == Z(0xffffffffffffffff)

        with pytest.raises(TypeError):
            Z(None)
        with pytest.raises(TypeError):
            Z('abcd')
        with pytest.raises(TypeError):
            Z(0.0)
        with pytest.raises(TypeError):
            Z(object())

    def NumericByteSizeTestHelper(self, byte_size_fn, value, expected_value_size):
        # Use field numbers that cause various byte sizes for the tag information.
        for field_number, tag_bytes in ((15, 1), (16, 2), (2047, 2), (2048, 3)):
            expected_size = expected_value_size + tag_bytes
            actual_size = byte_size_fn(field_number, value)
            assert expected_size == actual_size, (
                'byte_size_fn: %s, field_number: %d, value: %r\n'
                'Expected: %d, Actual: %d' % (
                byte_size_fn, field_number, value, expected_size, actual_size))

    def test_byte_size_functions(self):
        # Test all numeric *ByteSize() functions.
        NUMERIC_ARGS = [
            # Int32ByteSize().
            [wire_format.Int32ByteSize, 0, 1],
            [wire_format.Int32ByteSize, 127, 1],
            [wire_format.Int32ByteSize, 128, 2],
            [wire_format.Int32ByteSize, -1, 10],
            # Int64ByteSize().
            [wire_format.Int64ByteSize, 0, 1],
            [wire_format.Int64ByteSize, 127, 1],
            [wire_format.Int64ByteSize, 128, 2],
            [wire_format.Int64ByteSize, -1, 10],
            # UInt32ByteSize().
            [wire_format.UInt32ByteSize, 0, 1],
            [wire_format.UInt32ByteSize, 127, 1],
            [wire_format.UInt32ByteSize, 128, 2],
            [wire_format.UInt32ByteSize, wire_format.UINT32_MAX, 5],
            # UInt64ByteSize().
            [wire_format.UInt64ByteSize, 0, 1],
            [wire_format.UInt64ByteSize, 127, 1],
            [wire_format.UInt64ByteSize, 128, 2],
            [wire_format.UInt64ByteSize, wire_format.UINT64_MAX, 10],
            # SInt32ByteSize().
            [wire_format.SInt32ByteSize, 0, 1],
            [wire_format.SInt32ByteSize, -1, 1],
            [wire_format.SInt32ByteSize, 1, 1],
            [wire_format.SInt32ByteSize, -63, 1],
            [wire_format.SInt32ByteSize, 63, 1],
            [wire_format.SInt32ByteSize, -64, 1],
            [wire_format.SInt32ByteSize, 64, 2],
            # SInt64ByteSize().
            [wire_format.SInt64ByteSize, 0, 1],
            [wire_format.SInt64ByteSize, -1, 1],
            [wire_format.SInt64ByteSize, 1, 1],
            [wire_format.SInt64ByteSize, -63, 1],
            [wire_format.SInt64ByteSize, 63, 1],
            [wire_format.SInt64ByteSize, -64, 1],
            [wire_format.SInt64ByteSize, 64, 2],
            # Fixed32ByteSize().
            [wire_format.Fixed32ByteSize, 0, 4],
            [wire_format.Fixed32ByteSize, wire_format.UINT32_MAX, 4],
            # Fixed64ByteSize().
            [wire_format.Fixed64ByteSize, 0, 8],
            [wire_format.Fixed64ByteSize, wire_format.UINT64_MAX, 8],
            # SFixed32ByteSize().
            [wire_format.SFixed32ByteSize, 0, 4],
            [wire_format.SFixed32ByteSize, wire_format.INT32_MIN, 4],
            [wire_format.SFixed32ByteSize, wire_format.INT32_MAX, 4],
            # SFixed64ByteSize().
            [wire_format.SFixed64ByteSize, 0, 8],
            [wire_format.SFixed64ByteSize, wire_format.INT64_MIN, 8],
            [wire_format.SFixed64ByteSize, wire_format.INT64_MAX, 8],
            # FloatByteSize().
            [wire_format.FloatByteSize, 0.0, 4],
            [wire_format.FloatByteSize, 1000000000.0, 4],
            [wire_format.FloatByteSize, -1000000000.0, 4],
            # DoubleByteSize().
            [wire_format.DoubleByteSize, 0.0, 8],
            [wire_format.DoubleByteSize, 1000000000.0, 8],
            [wire_format.DoubleByteSize, -1000000000.0, 8],
            # BoolByteSize().
            [wire_format.BoolByteSize, False, 1],
            [wire_format.BoolByteSize, True, 1],
            # EnumByteSize().
            [wire_format.EnumByteSize, 0, 1],
            [wire_format.EnumByteSize, 127, 1],
            [wire_format.EnumByteSize, 128, 2],
            [wire_format.EnumByteSize, wire_format.UINT32_MAX, 5],
            ]
        for args in NUMERIC_ARGS:
            self.NumericByteSizeTestHelper(*args)

        # Test strings and bytes.
        for byte_size_fn in (wire_format.StringByteSize, wire_format.BytesByteSize):
            # 1 byte for tag, 1 byte for length, 3 bytes for contents.
            assert 5 == byte_size_fn(10, 'abc')
            # 2 bytes for tag, 1 byte for length, 3 bytes for contents.
            assert 6 == byte_size_fn(16, 'abc')
            # 2 bytes for tag, 2 bytes for length, 128 bytes for contents.
            assert 132 == byte_size_fn(16, 'a' * 128)

        # Test UTF-8 string byte size calculation.
        # 1 byte for tag, 1 byte for length, 8 bytes for content.
        assert (10 == wire_format.StringByteSize(
            5, b'\xd0\xa2\xd0\xb5\xd1\x81\xd1\x82'.decode('utf-8')))

        class MockMessage:
            def __init__(self, byte_size):
                self.byte_size = byte_size
            def ByteSize(self):
                return self.byte_size

        message_byte_size = 10
        mock_message = MockMessage(byte_size=message_byte_size)
        # Test groups.
        # (2 * 1) bytes for begin and end tags, plus message_byte_size.
        assert (2 + message_byte_size ==
                wire_format.GroupByteSize(1, mock_message))
        # (2 * 2) bytes for begin and end tags, plus message_byte_size.
        assert (4 + message_byte_size ==
                wire_format.GroupByteSize(16, mock_message))

        # Test messages.
        # 1 byte for tag, plus 1 byte for length, plus contents.
        assert (2 + mock_message.byte_size ==
                wire_format.MessageByteSize(1, mock_message))
        # 2 bytes for tag, plus 1 byte for length, plus contents.
        assert (3 + mock_message.byte_size ==
                wire_format.MessageByteSize(16, mock_message))
        # 2 bytes for tag, plus 2 bytes for length, plus contents.
        mock_message.byte_size = 128
        assert (4 + mock_message.byte_size ==
                wire_format.MessageByteSize(16, mock_message))


        # Test message set item byte size.
        # 4 bytes for tags, plus 1 byte for length, plus 1 byte for type_id,
        # plus contents.
        mock_message.byte_size = 10
        assert (mock_message.byte_size + 6 ==
                wire_format.MessageSetItemByteSize(1, mock_message))

        # 4 bytes for tags, plus 2 bytes for length, plus 1 byte for type_id,
        # plus contents.
        mock_message.byte_size = 128
        assert (mock_message.byte_size + 7 ==
                wire_format.MessageSetItemByteSize(1, mock_message))

        # 4 bytes for tags, plus 2 bytes for length, plus 2 byte for type_id,
        # plus contents.
        assert (mock_message.byte_size + 8 ==
                wire_format.MessageSetItemByteSize(128, mock_message))

        # Too-long varint.
        with pytest.raises(message.EncodeError):
            wire_format.UInt32ByteSize(1, 1 << 128)
