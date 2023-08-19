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

"""Test for google3.net.proto2.python.internal.well_known_types."""

import pytest

from google.protobuf import field_mask_pb2
from google.protobuf.internal import field_mask
from google.protobuf.internal import test_util
from google.protobuf import descriptor
from google.protobuf import map_unittest_pb2
from google.protobuf import unittest_pb2


class FieldMaskTest:
    def test_string_format(self):
        mask = field_mask_pb2.FieldMask()
        assert '' == mask.ToJsonString()
        mask.paths.append('foo')
        assert 'foo' == mask.ToJsonString()
        mask.paths.append('bar')
        assert 'foo,bar' == mask.ToJsonString()

        mask.FromJsonString('')
        assert '' == mask.ToJsonString()
        mask.FromJsonString('foo')
        assert ['foo'] == mask.paths
        mask.FromJsonString('foo,bar')
        assert ['foo', 'bar'] == mask.paths

        # Test camel case
        mask.Clear()
        mask.paths.append('foo_bar')
        assert 'fooBar' == mask.ToJsonString()
        mask.paths.append('bar_quz')
        assert 'fooBar,barQuz' == mask.ToJsonString()

        mask.FromJsonString('')
        assert '' == mask.ToJsonString()
        assert [] == mask.paths
        mask.FromJsonString('fooBar')
        assert ['foo_bar'] == mask.paths
        mask.FromJsonString('fooBar,barQuz')
        assert ['foo_bar', 'bar_quz'] == mask.paths

    def test_descriptor_to_field_mask(self):
        mask = field_mask_pb2.FieldMask()
        msg_descriptor = unittest_pb2.TestAllTypes.DESCRIPTOR
        mask.AllFieldsFromDescriptor(msg_descriptor)
        assert 76 == len(mask.paths)
        assert mask.IsValidForDescriptor(msg_descriptor)
        for field in msg_descriptor.fields:
            assert field.name in mask.paths

    def test_is_valid_for_descriptor(self):
        msg_descriptor = unittest_pb2.TestAllTypes.DESCRIPTOR
        # Empty mask
        mask = field_mask_pb2.FieldMask()
        assert mask.IsValidForDescriptor(msg_descriptor)
        # All fields from descriptor
        mask.AllFieldsFromDescriptor(msg_descriptor)
        assert mask.IsValidForDescriptor(msg_descriptor)
        # Child under optional message
        mask.paths.append('optional_nested_message.bb')
        assert mask.IsValidForDescriptor(msg_descriptor)
        # Repeated field is only allowed in the last position of path
        mask.paths.append('repeated_nested_message.bb')
        assert not mask.IsValidForDescriptor(msg_descriptor)
        # Invalid top level field
        mask = field_mask_pb2.FieldMask()
        mask.paths.append('xxx')
        assert not mask.IsValidForDescriptor(msg_descriptor)
        # Invalid field in root
        mask = field_mask_pb2.FieldMask()
        mask.paths.append('xxx.zzz')
        assert not mask.IsValidForDescriptor(msg_descriptor)
        # Invalid field in internal node
        mask = field_mask_pb2.FieldMask()
        mask.paths.append('optional_nested_message.xxx.zzz')
        assert not mask.IsValidForDescriptor(msg_descriptor)
        # Invalid field in leaf
        mask = field_mask_pb2.FieldMask()
        mask.paths.append('optional_nested_message.xxx')
        assert not mask.IsValidForDescriptor(msg_descriptor)

    def test_canonical_from(self):
        mask = field_mask_pb2.FieldMask()
        out_mask = field_mask_pb2.FieldMask()
        # Paths will be sorted.
        mask.FromJsonString('baz.quz,bar,foo')
        out_mask.CanonicalFormFromMask(mask)
        assert 'bar,baz.quz,foo' == out_mask.ToJsonString()
        # Duplicated paths will be removed.
        mask.FromJsonString('foo,bar,foo')
        out_mask.CanonicalFormFromMask(mask)
        assert 'bar,foo' == out_mask.ToJsonString()
        # Sub-paths of other paths will be removed.
        mask.FromJsonString('foo.b1,bar.b1,foo.b2,bar')
        out_mask.CanonicalFormFromMask(mask)
        assert 'bar,foo.b1,foo.b2' == out_mask.ToJsonString()

        # Test more deeply nested cases.
        mask.FromJsonString(
            'foo.bar.baz1,foo.bar.baz2.quz,foo.bar.baz2')
        out_mask.CanonicalFormFromMask(mask)
        assert 'foo.bar.baz1,foo.bar.baz2' == out_mask.ToJsonString()
        mask.FromJsonString(
            'foo.bar.baz1,foo.bar.baz2,foo.bar.baz2.quz')
        out_mask.CanonicalFormFromMask(mask)
        assert 'foo.bar.baz1,foo.bar.baz2' == out_mask.ToJsonString()
        mask.FromJsonString(
            'foo.bar.baz1,foo.bar.baz2,foo.bar.baz2.quz,foo.bar')
        out_mask.CanonicalFormFromMask(mask)
        assert 'foo.bar' == out_mask.ToJsonString()
        mask.FromJsonString(
            'foo.bar.baz1,foo.bar.baz2,foo.bar.baz2.quz,foo')
        out_mask.CanonicalFormFromMask(mask)
        assert 'foo' == out_mask.ToJsonString()

    def test_union(self):
        mask1 = field_mask_pb2.FieldMask()
        mask2 = field_mask_pb2.FieldMask()
        out_mask = field_mask_pb2.FieldMask()
        mask1.FromJsonString('foo,baz')
        mask2.FromJsonString('bar,quz')
        out_mask.Union(mask1, mask2)
        assert 'bar,baz,foo,quz' == out_mask.ToJsonString()
        # Overlap with duplicated paths.
        mask1.FromJsonString('foo,baz.bb')
        mask2.FromJsonString('baz.bb,quz')
        out_mask.Union(mask1, mask2)
        assert 'baz.bb,foo,quz' == out_mask.ToJsonString()
        # Overlap with paths covering some other paths.
        mask1.FromJsonString('foo.bar.baz,quz')
        mask2.FromJsonString('foo.bar,bar')
        out_mask.Union(mask1, mask2)
        assert 'bar,foo.bar,quz' == out_mask.ToJsonString()
        src = unittest_pb2.TestAllTypes()
        with pytest.raises(ValueError):
            out_mask.Union(src, mask2)

    def test_intersect(self):
        mask1 = field_mask_pb2.FieldMask()
        mask2 = field_mask_pb2.FieldMask()
        out_mask = field_mask_pb2.FieldMask()
        # Test cases without overlapping.
        mask1.FromJsonString('foo,baz')
        mask2.FromJsonString('bar,quz')
        out_mask.Intersect(mask1, mask2)
        assert '' == out_mask.ToJsonString()
        assert len(out_mask.paths) == 0
        assert out_mask.paths == []
        # Overlap with duplicated paths.
        mask1.FromJsonString('foo,baz.bb')
        mask2.FromJsonString('baz.bb,quz')
        out_mask.Intersect(mask1, mask2)
        assert 'baz.bb' == out_mask.ToJsonString()
        # Overlap with paths covering some other paths.
        mask1.FromJsonString('foo.bar.baz,quz')
        mask2.FromJsonString('foo.bar,bar')
        out_mask.Intersect(mask1, mask2)
        assert 'foo.bar.baz' == out_mask.ToJsonString()
        mask1.FromJsonString('foo.bar,bar')
        mask2.FromJsonString('foo.bar.baz,quz')
        out_mask.Intersect(mask1, mask2)
        assert 'foo.bar.baz' == out_mask.ToJsonString()
        # Intersect '' with ''
        mask1.Clear()
        mask2.Clear()
        mask1.paths.append('')
        mask2.paths.append('')
        assert mask1.paths == ['']
        assert '' == mask1.ToJsonString()
        out_mask.Intersect(mask1, mask2)
        assert out_mask.paths == []

    def test_merge_message_without_map_fields(self):
        # Test merge one field.
        src = unittest_pb2.TestAllTypes()
        test_util.SetAllFields(src)
        for field in src.DESCRIPTOR.fields:
            if field.containing_oneof:
                continue
            field_name = field.name
            dst = unittest_pb2.TestAllTypes()
            # Only set one path to mask.
            mask = field_mask_pb2.FieldMask()
            mask.paths.append(field_name)
            mask.MergeMessage(src, dst)
            # The expected result message.
            msg = unittest_pb2.TestAllTypes()
            if field.label == descriptor.FieldDescriptor.LABEL_REPEATED:
                repeated_src = getattr(src, field_name)
                repeated_msg = getattr(msg, field_name)
                if field.cpp_type == descriptor.FieldDescriptor.CPPTYPE_MESSAGE:
                    for item in repeated_src:
                        repeated_msg.add().CopyFrom(item)
                else:
                    repeated_msg.extend(repeated_src)
            elif field.cpp_type == descriptor.FieldDescriptor.CPPTYPE_MESSAGE:
                getattr(msg, field_name).CopyFrom(getattr(src, field_name))
            else:
                setattr(msg, field_name, getattr(src, field_name))
            # Only field specified in mask is merged.
            assert msg == dst

        # Test merge nested fields.
        nested_src = unittest_pb2.NestedTestAllTypes()
        nested_dst = unittest_pb2.NestedTestAllTypes()
        nested_src.child.payload.optional_int32 = 1234
        nested_src.child.child.payload.optional_int32 = 5678
        mask = field_mask_pb2.FieldMask()
        mask.FromJsonString('child.payload')
        mask.MergeMessage(nested_src, nested_dst)
        assert 1234 == nested_dst.child.payload.optional_int32
        assert 0 == nested_dst.child.child.payload.optional_int32

        mask.FromJsonString('child.child.payload')
        mask.MergeMessage(nested_src, nested_dst)
        assert 1234 == nested_dst.child.payload.optional_int32
        assert 5678 == nested_dst.child.child.payload.optional_int32

        nested_dst.Clear()
        mask.FromJsonString('child.child.payload')
        mask.MergeMessage(nested_src, nested_dst)
        assert 0 == nested_dst.child.payload.optional_int32
        assert 5678 == nested_dst.child.child.payload.optional_int32

        nested_dst.Clear()
        mask.FromJsonString('child')
        mask.MergeMessage(nested_src, nested_dst)
        assert 1234 == nested_dst.child.payload.optional_int32
        assert 5678 == nested_dst.child.child.payload.optional_int32

        # Test MergeOptions.
        nested_dst.Clear()
        nested_dst.child.payload.optional_int64 = 4321
        # Message fields will be merged by default.
        mask.FromJsonString('child.payload')
        mask.MergeMessage(nested_src, nested_dst)
        assert 1234 == nested_dst.child.payload.optional_int32
        assert 4321 == nested_dst.child.payload.optional_int64
        # Change the behavior to replace message fields.
        mask.FromJsonString('child.payload')
        mask.MergeMessage(nested_src, nested_dst, True, False)
        assert 1234 == nested_dst.child.payload.optional_int32
        assert 0 == nested_dst.child.payload.optional_int64

        # By default, fields missing in source are not cleared in destination.
        nested_dst.payload.optional_int32 = 1234
        assert nested_dst.HasField('payload')
        mask.FromJsonString('payload')
        mask.MergeMessage(nested_src, nested_dst)
        assert nested_dst.HasField('payload')
        # But they are cleared when replacing message fields.
        nested_dst.Clear()
        nested_dst.payload.optional_int32 = 1234
        mask.FromJsonString('payload')
        mask.MergeMessage(nested_src, nested_dst, True, False)
        assert not nested_dst.HasField('payload')

        nested_src.payload.repeated_int32.append(1234)
        nested_dst.payload.repeated_int32.append(5678)
        # Repeated fields will be appended by default.
        mask.FromJsonString('payload.repeatedInt32')
        mask.MergeMessage(nested_src, nested_dst)
        assert 2 == len(nested_dst.payload.repeated_int32)
        assert 5678 == nested_dst.payload.repeated_int32[0]
        assert 1234 == nested_dst.payload.repeated_int32[1]
        # Change the behavior to replace repeated fields.
        mask.FromJsonString('payload.repeatedInt32')
        mask.MergeMessage(nested_src, nested_dst, False, True)
        assert 1 == len(nested_dst.payload.repeated_int32)
        assert 1234 == nested_dst.payload.repeated_int32[0]

        # Test Merge oneof field.
        new_msg = unittest_pb2.TestOneof2()
        dst = unittest_pb2.TestOneof2()
        dst.foo_message.moo_int = 1
        mask = field_mask_pb2.FieldMask()
        mask.FromJsonString('fooMessage,fooLazyMessage.mooInt')
        mask.MergeMessage(new_msg, dst)
        assert dst.HasField('foo_message')
        assert not dst.HasField('foo_lazy_message')

    def test_merge_message_with_map_field(self):
        empty_map = map_unittest_pb2.TestRecursiveMapMessage()
        src_level_2 = map_unittest_pb2.TestRecursiveMapMessage()
        src_level_2.a['src level 2'].CopyFrom(empty_map)
        src = map_unittest_pb2.TestRecursiveMapMessage()
        src.a['common key'].CopyFrom(src_level_2)
        src.a['src level 1'].CopyFrom(src_level_2)

        dst_level_2 = map_unittest_pb2.TestRecursiveMapMessage()
        dst_level_2.a['dst level 2'].CopyFrom(empty_map)
        dst = map_unittest_pb2.TestRecursiveMapMessage()
        dst.a['common key'].CopyFrom(dst_level_2)
        dst.a['dst level 1'].CopyFrom(empty_map)

        mask = field_mask_pb2.FieldMask()
        mask.FromJsonString('a')
        mask.MergeMessage(src, dst)

        # map from dst is replaced with map from src.
        assert dst.a['common key'] == src_level_2
        assert dst.a['src level 1'] == src_level_2
        assert dst.a['dst level 1'] == empty_map

    def test_merge_errors(self):
        src = unittest_pb2.TestAllTypes()
        dst = unittest_pb2.TestAllTypes()
        mask = field_mask_pb2.FieldMask()
        test_util.SetAllFields(src)
        mask.FromJsonString('optionalInt32.field')
        with pytest.raises(ValueError) as e:
            mask.MergeMessage(src, dst)
        assert ('Error: Field optional_int32 in message '
                'protobuf_unittest.TestAllTypes is not a singular '
                'message field and cannot have sub-fields.'
                == str(e.exception))

    def test_snake_case_to_camel_case(self):
        assert 'fooBar' == field_mask._SnakeCaseToCamelCase('foo_bar')
        assert 'FooBar' == field_mask._SnakeCaseToCamelCase('_foo_bar')
        assert 'foo3Bar' == field_mask._SnakeCaseToCamelCase('foo3_bar')

        # No uppercase letter is allowed.
        with pytest.raises(ValueError) as e:
            field_mask._SnakeCaseToCamelCase('Foo')
        assert (
            'Fail to print FieldMask to Json string: Path name Foo must '
            'not contain uppercase letters.'
            == e)
            
        # Any character after a "_" must be a lowercase letter.
        #   1. "_" cannot be followed by another "_".
        #   2. "_" cannot be followed by a digit.
        #   3. "_" cannot appear as the last character.
        with pytest.raises(ValueError) as e:
            field_mask._SnakeCaseToCamelCase('foo__bar')
        assert (
            'Fail to print FieldMask to Json string: The character after a '
            '"_" must be a lowercase letter in path name foo__bar.'
            == e)
        with pytest.raises(ValueError) as e:
            field_mask._SnakeCaseToCamelCase('foo_3bar')
        assert (
            'Fail to print FieldMask to Json string: The character after a '
            '"_" must be a lowercase letter in path name foo_3bar.'
            == e)
        with pytest.raises(ValueError) as e:
            field_mask._SnakeCaseToCamelCase('foo_bar_')
        assert (
            'Fail to print FieldMask to Json string: Trailing "_" in path '
            'name foo_bar_.'
            == e)

    def test_camel_case_to_snake_case(self):
        assert 'foo_bar' == field_mask._CamelCaseToSnakeCase('fooBar')
        assert '_foo_bar' == field_mask._CamelCaseToSnakeCase('FooBar')
        assert 'foo3_bar' == field_mask._CamelCaseToSnakeCase('foo3Bar')
        with pytest.raises(ValueError) as e:
            field_mask._CamelCaseToSnakeCase('foo_bar')
        assert (
            'Fail to parse FieldMask: Path name foo_bar must not contain "_"s.'
            == e)
