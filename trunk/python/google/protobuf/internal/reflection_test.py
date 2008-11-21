#! /usr/bin/python
# -*- coding: utf-8 -*-
#
# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
# http://code.google.com/p/protobuf/
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

"""Unittest for reflection.py, which also indirectly tests the output of the
pure-Python protocol compiler.
"""

__author__ = 'robinson@google.com (Will Robinson)'

import operator

import unittest
# TODO(robinson): When we split this test in two, only some of these imports
# will be necessary in each test.
from google.protobuf import unittest_import_pb2
from google.protobuf import unittest_mset_pb2
from google.protobuf import unittest_pb2
from google.protobuf import descriptor_pb2
from google.protobuf import descriptor
from google.protobuf import message
from google.protobuf import reflection
from google.protobuf.internal import more_extensions_pb2
from google.protobuf.internal import more_messages_pb2
from google.protobuf.internal import wire_format
from google.protobuf.internal import test_util
from google.protobuf.internal import decoder


class RefectionTest(unittest.TestCase):

  def testSimpleHasBits(self):
    # Test a scalar.
    proto = unittest_pb2.TestAllTypes()
    self.assertTrue(not proto.HasField('optional_int32'))
    self.assertEqual(0, proto.optional_int32)
    # HasField() shouldn't be true if all we've done is
    # read the default value.
    self.assertTrue(not proto.HasField('optional_int32'))
    proto.optional_int32 = 1
    # Setting a value however *should* set the "has" bit.
    self.assertTrue(proto.HasField('optional_int32'))
    proto.ClearField('optional_int32')
    # And clearing that value should unset the "has" bit.
    self.assertTrue(not proto.HasField('optional_int32'))

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
      self.assertTrue(not composite_field.HasField(scalar_field_name))
      # Assert that proto does not "have" the composite field.
      self.assertTrue(not proto.HasField(composite_field_name))

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
      self.assertTrue(not composite_field.HasField(scalar_field_name))
      self.assertTrue(not proto.HasField(composite_field_name))
      # ...and ensure that the scalar field has returned to its default.
      self.assertEqual(0, getattr(composite_field, scalar_field_name))

      # Finally, ensure that modifications to the old composite field object
      # don't have any effect on the parent.
      #
      # (NOTE that when we clear the composite field in the parent, we actually
      # don't recursively clear down the tree.  Instead, we just disconnect the
      # cleared composite from the tree.)
      self.assertTrue(old_composite_field is not composite_field)
      setattr(old_composite_field, scalar_field_name, new_val)
      self.assertTrue(not composite_field.HasField(scalar_field_name))
      self.assertTrue(not proto.HasField(composite_field_name))
      self.assertEqual(0, getattr(composite_field, scalar_field_name))

    # Test simple, single-level nesting when we set a scalar.
    TestCompositeHasBits('optionalgroup', 'a')
    TestCompositeHasBits('optional_nested_message', 'bb')
    TestCompositeHasBits('optional_foreign_message', 'c')
    TestCompositeHasBits('optional_import_message', 'd')

  def testReferencesToNestedMessage(self):
    proto = unittest_pb2.TestAllTypes()
    nested = proto.optional_nested_message
    del proto
    # A previous version had a bug where this would raise an exception when
    # hitting a now-dead weak reference.
    nested.bb = 23

  def testDisconnectingNestedMessageBeforeSettingField(self):
    proto = unittest_pb2.TestAllTypes()
    nested = proto.optional_nested_message
    proto.ClearField('optional_nested_message')  # Should disconnect from parent
    self.assertTrue(nested is not proto.optional_nested_message)
    nested.bb = 23
    self.assertTrue(not proto.HasField('optional_nested_message'))
    self.assertEqual(0, proto.optional_nested_message.bb)

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
    self.assertTrue(not proto.HasField('optional_nested_message'))
    proto.optional_nested_message.nestedmessage_repeated_foreignmessage.add()
    self.assertTrue(proto.HasField('optional_nested_message'))

  def testHasBitsForManyLevelsOfNesting(self):
    # Test nesting many levels deep.
    recursive_proto = unittest_pb2.TestMutualRecursionA()
    self.assertTrue(not recursive_proto.HasField('bb'))
    self.assertEqual(0, recursive_proto.bb.a.bb.a.bb.optional_int32)
    self.assertTrue(not recursive_proto.HasField('bb'))
    recursive_proto.bb.a.bb.a.bb.optional_int32 = 5
    self.assertEqual(5, recursive_proto.bb.a.bb.a.bb.optional_int32)
    self.assertTrue(recursive_proto.HasField('bb'))
    self.assertTrue(recursive_proto.bb.HasField('a'))
    self.assertTrue(recursive_proto.bb.a.HasField('bb'))
    self.assertTrue(recursive_proto.bb.a.bb.HasField('a'))
    self.assertTrue(recursive_proto.bb.a.bb.a.HasField('bb'))
    self.assertTrue(not recursive_proto.bb.a.bb.a.bb.HasField('a'))
    self.assertTrue(recursive_proto.bb.a.bb.a.bb.HasField('optional_int32'))

  def testSingularListFields(self):
    proto = unittest_pb2.TestAllTypes()
    proto.optional_fixed32 = 1
    proto.optional_int32 = 5
    proto.optional_string = 'foo'
    self.assertEqual(
      [ (proto.DESCRIPTOR.fields_by_name['optional_int32'  ], 5),
        (proto.DESCRIPTOR.fields_by_name['optional_fixed32'], 1),
        (proto.DESCRIPTOR.fields_by_name['optional_string' ], 'foo') ],
      proto.ListFields())

  def testRepeatedListFields(self):
    proto = unittest_pb2.TestAllTypes()
    proto.repeated_fixed32.append(1)
    proto.repeated_int32.append(5)
    proto.repeated_int32.append(11)
    proto.repeated_string.append('foo')
    proto.repeated_string.append('bar')
    proto.repeated_string.append('baz')
    proto.optional_int32 = 21
    self.assertEqual(
      [ (proto.DESCRIPTOR.fields_by_name['optional_int32'  ], 21),
        (proto.DESCRIPTOR.fields_by_name['repeated_int32'  ], [5, 11]),
        (proto.DESCRIPTOR.fields_by_name['repeated_fixed32'], [1]),
        (proto.DESCRIPTOR.fields_by_name['repeated_string' ],
          ['foo', 'bar', 'baz']) ],
      proto.ListFields())

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
    self.assertEqual('', proto.optional_bytes)

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
    self.assertEqual('world', proto.default_bytes)
    self.assertEqual(unittest_pb2.TestAllTypes.BAR, proto.default_nested_enum)
    self.assertEqual(unittest_pb2.FOREIGN_BAR, proto.default_foreign_enum)
    self.assertEqual(unittest_import_pb2.IMPORT_BAR,
                     proto.default_import_enum)

    proto = unittest_pb2.TestExtremeDefaultValues()
    self.assertEqual(u'\u1234', proto.utf8_string)

  def testHasFieldWithUnknownFieldName(self):
    proto = unittest_pb2.TestAllTypes()
    self.assertRaises(ValueError, proto.HasField, 'nonexistent_field')

  def testClearFieldWithUnknownFieldName(self):
    proto = unittest_pb2.TestAllTypes()
    self.assertRaises(ValueError, proto.ClearField, 'nonexistent_field')

  def testDisallowedAssignments(self):
    # It's illegal to assign values directly to repeated fields
    # or to nonrepeated composite fields.  Ensure that this fails.
    proto = unittest_pb2.TestAllTypes()
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

  # TODO(robinson): Add type-safety check for enums.
  def testSingleScalarTypeSafety(self):
    proto = unittest_pb2.TestAllTypes()
    self.assertRaises(TypeError, setattr, proto, 'optional_int32', 1.1)
    self.assertRaises(TypeError, setattr, proto, 'optional_int32', 'foo')
    self.assertRaises(TypeError, setattr, proto, 'optional_string', 10)
    self.assertRaises(TypeError, setattr, proto, 'optional_bytes', 10)

  def testSingleScalarBoundsChecking(self):
    def TestMinAndMaxIntegers(field_name, expected_min, expected_max):
      pb = unittest_pb2.TestAllTypes()
      setattr(pb, field_name, expected_min)
      setattr(pb, field_name, expected_max)
      self.assertRaises(ValueError, setattr, pb, field_name, expected_min - 1)
      self.assertRaises(ValueError, setattr, pb, field_name, expected_max + 1)

    TestMinAndMaxIntegers('optional_int32', -(1 << 31), (1 << 31) - 1)
    TestMinAndMaxIntegers('optional_uint32', 0, 0xffffffff)
    TestMinAndMaxIntegers('optional_int64', -(1 << 63), (1 << 63) - 1)
    TestMinAndMaxIntegers('optional_uint64', 0, 0xffffffffffffffff)
    TestMinAndMaxIntegers('optional_nested_enum', -(1 << 31), (1 << 31) - 1)

  def testRepeatedScalarTypeSafety(self):
    proto = unittest_pb2.TestAllTypes()
    self.assertRaises(TypeError, proto.repeated_int32.append, 1.1)
    self.assertRaises(TypeError, proto.repeated_int32.append, 'foo')
    self.assertRaises(TypeError, proto.repeated_string, 10)
    self.assertRaises(TypeError, proto.repeated_bytes, 10)

    proto.repeated_int32.append(10)
    proto.repeated_int32[0] = 23
    self.assertRaises(IndexError, proto.repeated_int32.__setitem__, 500, 23)
    self.assertRaises(TypeError, proto.repeated_int32.__setitem__, 0, 'abc')

  def testSingleScalarGettersAndSetters(self):
    proto = unittest_pb2.TestAllTypes()
    self.assertEqual(0, proto.optional_int32)
    proto.optional_int32 = 1
    self.assertEqual(1, proto.optional_int32)
    # TODO(robinson): Test all other scalar field types.

  def testSingleScalarClearField(self):
    proto = unittest_pb2.TestAllTypes()
    # Should be allowed to clear something that's not there (a no-op).
    proto.ClearField('optional_int32')
    proto.optional_int32 = 1
    self.assertTrue(proto.HasField('optional_int32'))
    proto.ClearField('optional_int32')
    self.assertEqual(0, proto.optional_int32)
    self.assertTrue(not proto.HasField('optional_int32'))
    # TODO(robinson): Test all other scalar field types.

  def testEnums(self):
    proto = unittest_pb2.TestAllTypes()
    self.assertEqual(1, proto.FOO)
    self.assertEqual(1, unittest_pb2.TestAllTypes.FOO)
    self.assertEqual(2, proto.BAR)
    self.assertEqual(2, unittest_pb2.TestAllTypes.BAR)
    self.assertEqual(3, proto.BAZ)
    self.assertEqual(3, unittest_pb2.TestAllTypes.BAZ)

  def testRepeatedScalars(self):
    proto = unittest_pb2.TestAllTypes()

    self.assertTrue(not proto.repeated_int32)
    self.assertEqual(0, len(proto.repeated_int32))
    proto.repeated_int32.append(5);
    proto.repeated_int32.append(10);
    self.assertTrue(proto.repeated_int32)
    self.assertEqual(2, len(proto.repeated_int32))

    self.assertEqual([5, 10], proto.repeated_int32)
    self.assertEqual(5, proto.repeated_int32[0])
    self.assertEqual(10, proto.repeated_int32[-1])
    # Test out-of-bounds indices.
    self.assertRaises(IndexError, proto.repeated_int32.__getitem__, 1234)
    self.assertRaises(IndexError, proto.repeated_int32.__getitem__, -1234)
    # Test incorrect types passed to __getitem__.
    self.assertRaises(TypeError, proto.repeated_int32.__getitem__, 'foo')
    self.assertRaises(TypeError, proto.repeated_int32.__getitem__, None)

    # Test that we can use the field as an iterator.
    result = []
    for i in proto.repeated_int32:
      result.append(i)
    self.assertEqual([5, 10], result)

    # Test clearing.
    proto.ClearField('repeated_int32')
    self.assertTrue(not proto.repeated_int32)
    self.assertEqual(0, len(proto.repeated_int32))

  def testRepeatedScalarsRemove(self):
    proto = unittest_pb2.TestAllTypes()

    self.assertTrue(not proto.repeated_int32)
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

  def testRepeatedComposites(self):
    proto = unittest_pb2.TestAllTypes()
    self.assertTrue(not proto.repeated_nested_message)
    self.assertEqual(0, len(proto.repeated_nested_message))
    m0 = proto.repeated_nested_message.add()
    m1 = proto.repeated_nested_message.add()
    self.assertTrue(proto.repeated_nested_message)
    self.assertEqual(2, len(proto.repeated_nested_message))
    self.assertTrue(m0 is proto.repeated_nested_message[0])
    self.assertTrue(m1 is proto.repeated_nested_message[1])
    self.assertTrue(isinstance(m0, unittest_pb2.TestAllTypes.NestedMessage))

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

    # Test that we can use the field as an iterator.
    result = []
    for i in proto.repeated_nested_message:
      result.append(i)
    self.assertEqual(2, len(result))
    self.assertTrue(m0 is result[0])
    self.assertTrue(m1 is result[1])

    # Test item deletion.
    del proto.repeated_nested_message[0]
    self.assertEqual(1, len(proto.repeated_nested_message))
    self.assertTrue(m1 is proto.repeated_nested_message[0])

    # Test clearing.
    proto.ClearField('repeated_nested_message')
    self.assertTrue(not proto.repeated_nested_message)
    self.assertEqual(0, len(proto.repeated_nested_message))

  def testHandWrittenReflection(self):
    # TODO(robinson): We probably need a better way to specify
    # protocol types by hand.  But then again, this isn't something
    # we expect many people to do.  Hmm.
    FieldDescriptor = descriptor.FieldDescriptor
    foo_field_descriptor = FieldDescriptor(
        name='foo_field', full_name='MyProto.foo_field',
        index=0, number=1, type=FieldDescriptor.TYPE_INT64,
        cpp_type=FieldDescriptor.CPPTYPE_INT64,
        label=FieldDescriptor.LABEL_OPTIONAL, default_value=0,
        containing_type=None, message_type=None, enum_type=None,
        is_extension=False, extension_scope=None,
        options=descriptor_pb2.FieldOptions())
    mydescriptor = descriptor.Descriptor(
        name='MyProto', full_name='MyProto', filename='ignored',
        containing_type=None, nested_types=[], enum_types=[],
        fields=[foo_field_descriptor], extensions=[],
        options=descriptor_pb2.MessageOptions())
    class MyProtoClass(message.Message):
      DESCRIPTOR = mydescriptor
      __metaclass__ = reflection.GeneratedProtocolMessageType
    myproto_instance = MyProtoClass()
    self.assertEqual(0, myproto_instance.foo_field)
    self.assertTrue(not myproto_instance.HasField('foo_field'))
    myproto_instance.foo_field = 23
    self.assertEqual(23, myproto_instance.foo_field)
    self.assertTrue(myproto_instance.HasField('foo_field'))

  def testTopLevelExtensionsForOptionalScalar(self):
    extendee_proto = unittest_pb2.TestAllExtensions()
    extension = unittest_pb2.optional_int32_extension
    self.assertTrue(not extendee_proto.HasExtension(extension))
    self.assertEqual(0, extendee_proto.Extensions[extension])
    # As with normal scalar fields, just doing a read doesn't actually set the
    # "has" bit.
    self.assertTrue(not extendee_proto.HasExtension(extension))
    # Actually set the thing.
    extendee_proto.Extensions[extension] = 23
    self.assertEqual(23, extendee_proto.Extensions[extension])
    self.assertTrue(extendee_proto.HasExtension(extension))
    # Ensure that clearing works as well.
    extendee_proto.ClearExtension(extension)
    self.assertEqual(0, extendee_proto.Extensions[extension])
    self.assertTrue(not extendee_proto.HasExtension(extension))

  def testTopLevelExtensionsForRepeatedScalar(self):
    extendee_proto = unittest_pb2.TestAllExtensions()
    extension = unittest_pb2.repeated_string_extension
    self.assertEqual(0, len(extendee_proto.Extensions[extension]))
    extendee_proto.Extensions[extension].append('foo')
    self.assertEqual(['foo'], extendee_proto.Extensions[extension])
    string_list = extendee_proto.Extensions[extension]
    extendee_proto.ClearExtension(extension)
    self.assertEqual(0, len(extendee_proto.Extensions[extension]))
    self.assertTrue(string_list is not extendee_proto.Extensions[extension])
    # Shouldn't be allowed to do Extensions[extension] = 'a'
    self.assertRaises(TypeError, operator.setitem, extendee_proto.Extensions,
                      extension, 'a')

  def testTopLevelExtensionsForOptionalMessage(self):
    extendee_proto = unittest_pb2.TestAllExtensions()
    extension = unittest_pb2.optional_foreign_message_extension
    self.assertTrue(not extendee_proto.HasExtension(extension))
    self.assertEqual(0, extendee_proto.Extensions[extension].c)
    # As with normal (non-extension) fields, merely reading from the
    # thing shouldn't set the "has" bit.
    self.assertTrue(not extendee_proto.HasExtension(extension))
    extendee_proto.Extensions[extension].c = 23
    self.assertEqual(23, extendee_proto.Extensions[extension].c)
    self.assertTrue(extendee_proto.HasExtension(extension))
    # Save a reference here.
    foreign_message = extendee_proto.Extensions[extension]
    extendee_proto.ClearExtension(extension)
    self.assertTrue(foreign_message is not extendee_proto.Extensions[extension])
    # Setting a field on foreign_message now shouldn't set
    # any "has" bits on extendee_proto.
    foreign_message.c = 42
    self.assertEqual(42, foreign_message.c)
    self.assertTrue(foreign_message.HasField('c'))
    self.assertTrue(not extendee_proto.HasExtension(extension))
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
    self.assertTrue(group_list is not extendee_proto.Extensions[extension])
    # Shouldn't be allowed to do Extensions[extension] = 'a'
    self.assertRaises(TypeError, operator.setitem, extendee_proto.Extensions,
                      extension, 'a')

  def testNestedExtensions(self):
    extendee_proto = unittest_pb2.TestAllExtensions()
    extension = unittest_pb2.TestRequired.single

    # We just test the non-repeated case.
    self.assertTrue(not extendee_proto.HasExtension(extension))
    required = extendee_proto.Extensions[extension]
    self.assertEqual(0, required.a)
    self.assertTrue(not extendee_proto.HasExtension(extension))
    required.a = 23
    self.assertEqual(23, extendee_proto.Extensions[extension].a)
    self.assertTrue(extendee_proto.HasExtension(extension))
    extendee_proto.ClearExtension(extension)
    self.assertTrue(required is not extendee_proto.Extensions[extension])
    self.assertTrue(not extendee_proto.HasExtension(extension))

  # If message A directly contains message B, and
  # a.HasField('b') is currently False, then mutating any
  # extension in B should change a.HasField('b') to True
  # (and so on up the object tree).
  def testHasBitsForAncestorsOfExtendedMessage(self):
    # Optional scalar extension.
    toplevel = more_extensions_pb2.TopLevelMessage()
    self.assertTrue(not toplevel.HasField('submessage'))
    self.assertEqual(0, toplevel.submessage.Extensions[
        more_extensions_pb2.optional_int_extension])
    self.assertTrue(not toplevel.HasField('submessage'))
    toplevel.submessage.Extensions[
        more_extensions_pb2.optional_int_extension] = 23
    self.assertEqual(23, toplevel.submessage.Extensions[
        more_extensions_pb2.optional_int_extension])
    self.assertTrue(toplevel.HasField('submessage'))

    # Repeated scalar extension.
    toplevel = more_extensions_pb2.TopLevelMessage()
    self.assertTrue(not toplevel.HasField('submessage'))
    self.assertEqual([], toplevel.submessage.Extensions[
        more_extensions_pb2.repeated_int_extension])
    self.assertTrue(not toplevel.HasField('submessage'))
    toplevel.submessage.Extensions[
        more_extensions_pb2.repeated_int_extension].append(23)
    self.assertEqual([23], toplevel.submessage.Extensions[
        more_extensions_pb2.repeated_int_extension])
    self.assertTrue(toplevel.HasField('submessage'))

    # Optional message extension.
    toplevel = more_extensions_pb2.TopLevelMessage()
    self.assertTrue(not toplevel.HasField('submessage'))
    self.assertEqual(0, toplevel.submessage.Extensions[
        more_extensions_pb2.optional_message_extension].foreign_message_int)
    self.assertTrue(not toplevel.HasField('submessage'))
    toplevel.submessage.Extensions[
        more_extensions_pb2.optional_message_extension].foreign_message_int = 23
    self.assertEqual(23, toplevel.submessage.Extensions[
        more_extensions_pb2.optional_message_extension].foreign_message_int)
    self.assertTrue(toplevel.HasField('submessage'))

    # Repeated message extension.
    toplevel = more_extensions_pb2.TopLevelMessage()
    self.assertTrue(not toplevel.HasField('submessage'))
    self.assertEqual(0, len(toplevel.submessage.Extensions[
        more_extensions_pb2.repeated_message_extension]))
    self.assertTrue(not toplevel.HasField('submessage'))
    foreign = toplevel.submessage.Extensions[
        more_extensions_pb2.repeated_message_extension].add()
    self.assertTrue(foreign is toplevel.submessage.Extensions[
        more_extensions_pb2.repeated_message_extension][0])
    self.assertTrue(toplevel.HasField('submessage'))

  def testDisconnectionAfterClearingEmptyMessage(self):
    toplevel = more_extensions_pb2.TopLevelMessage()
    extendee_proto = toplevel.submessage
    extension = more_extensions_pb2.optional_message_extension
    extension_proto = extendee_proto.Extensions[extension]
    extendee_proto.ClearExtension(extension)
    extension_proto.foreign_message_int = 23

    self.assertTrue(not toplevel.HasField('submessage'))
    self.assertTrue(extension_proto is not extendee_proto.Extensions[extension])

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
    unknown_handle = more_extensions_pb2.optional_int_extension
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

  def testMergeFromSingularField(self):
    # Test merge with just a singular field.
    proto1 = unittest_pb2.TestAllTypes()
    proto1.optional_int32 = 1

    proto2 = unittest_pb2.TestAllTypes()
    # This shouldn't get overwritten.
    proto2.optional_string = 'value'

    proto2.MergeFrom(proto1)
    self.assertEqual(1, proto2.optional_int32)
    self.assertEqual('value', proto2.optional_string)

  def testMergeFromRepeatedField(self):
    # Test merge with just a repeated field.
    proto1 = unittest_pb2.TestAllTypes()
    proto1.repeated_int32.append(1)
    proto1.repeated_int32.append(2)

    proto2 = unittest_pb2.TestAllTypes()
    proto2.repeated_int32.append(0)
    proto2.MergeFrom(proto1)

    self.assertEqual(0, proto2.repeated_int32[0])
    self.assertEqual(1, proto2.repeated_int32[1])
    self.assertEqual(2, proto2.repeated_int32[2])

  def testMergeFromOptionalGroup(self):
    # Test merge with an optional group.
    proto1 = unittest_pb2.TestAllTypes()
    proto1.optionalgroup.a = 12
    proto2 = unittest_pb2.TestAllTypes()
    proto2.MergeFrom(proto1)
    self.assertEqual(12, proto2.optionalgroup.a)

  def testMergeFromRepeatedNestedMessage(self):
    # Test merge with a repeated nested message.
    proto1 = unittest_pb2.TestAllTypes()
    m = proto1.repeated_nested_message.add()
    m.bb = 123
    m = proto1.repeated_nested_message.add()
    m.bb = 321

    proto2 = unittest_pb2.TestAllTypes()
    m = proto2.repeated_nested_message.add()
    m.bb = 999
    proto2.MergeFrom(proto1)
    self.assertEqual(999, proto2.repeated_nested_message[0].bb)
    self.assertEqual(123, proto2.repeated_nested_message[1].bb)
    self.assertEqual(321, proto2.repeated_nested_message[2].bb)

  def testMergeFromAllFields(self):
    # With all fields set.
    proto1 = unittest_pb2.TestAllTypes()
    test_util.SetAllFields(proto1)
    proto2 = unittest_pb2.TestAllTypes()
    proto2.MergeFrom(proto1)

    # Messages should be equal.
    self.assertEqual(proto2, proto1)

    # Serialized string should be equal too.
    string1 = proto1.SerializeToString()
    string2 = proto2.SerializeToString()
    self.assertEqual(string1, string2)

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

  def testCopyFromSingularField(self):
    # Test copy with just a singular field.
    proto1 = unittest_pb2.TestAllTypes()
    proto1.optional_int32 = 1
    proto1.optional_string = 'important-text'

    proto2 = unittest_pb2.TestAllTypes()
    proto2.optional_string = 'value'

    proto2.CopyFrom(proto1)
    self.assertEqual(1, proto2.optional_int32)
    self.assertEqual('important-text', proto2.optional_string)

  def testCopyFromRepeatedField(self):
    # Test copy with a repeated field.
    proto1 = unittest_pb2.TestAllTypes()
    proto1.repeated_int32.append(1)
    proto1.repeated_int32.append(2)

    proto2 = unittest_pb2.TestAllTypes()
    proto2.repeated_int32.append(0)
    proto2.CopyFrom(proto1)

    self.assertEqual(1, proto2.repeated_int32[0])
    self.assertEqual(2, proto2.repeated_int32[1])

  def testCopyFromAllFields(self):
    # With all fields set.
    proto1 = unittest_pb2.TestAllTypes()
    test_util.SetAllFields(proto1)
    proto2 = unittest_pb2.TestAllTypes()
    proto2.CopyFrom(proto1)

    # Messages should be equal.
    self.assertEqual(proto2, proto1)

    # Serialized string should be equal too.
    string1 = proto1.SerializeToString()
    string2 = proto2.SerializeToString()
    self.assertEqual(string1, string2)

  def testCopyFromSelf(self):
    proto1 = unittest_pb2.TestAllTypes()
    proto1.repeated_int32.append(1)
    proto1.optional_int32 = 2
    proto1.optional_string = 'important-text'

    proto1.CopyFrom(proto1)
    self.assertEqual(1, proto1.repeated_int32[0])
    self.assertEqual(2, proto1.optional_int32)
    self.assertEqual('important-text', proto1.optional_string)

  def testClear(self):
    proto = unittest_pb2.TestAllTypes()
    test_util.SetAllFields(proto)
    # Clear the message.
    proto.Clear()
    self.assertEquals(proto.ByteSize(), 0)
    empty_proto = unittest_pb2.TestAllTypes()
    self.assertEquals(proto, empty_proto)

    # Test if extensions which were set are cleared.
    proto = unittest_pb2.TestAllExtensions()
    test_util.SetAllExtensions(proto)
    # Clear the message.
    proto.Clear()
    self.assertEquals(proto.ByteSize(), 0)
    empty_proto = unittest_pb2.TestAllExtensions()
    self.assertEquals(proto, empty_proto)

  def testIsInitialized(self):
    # Trivial cases - all optional fields and extensions.
    proto = unittest_pb2.TestAllTypes()
    self.assertTrue(proto.IsInitialized())
    proto = unittest_pb2.TestAllExtensions()
    self.assertTrue(proto.IsInitialized())

    # The case of uninitialized required fields.
    proto = unittest_pb2.TestRequired()
    self.assertFalse(proto.IsInitialized())
    proto.a = proto.b = proto.c = 2
    self.assertTrue(proto.IsInitialized())

    # The case of uninitialized submessage.
    proto = unittest_pb2.TestRequiredForeign()
    self.assertTrue(proto.IsInitialized())
    proto.optional_message.a = 1
    self.assertFalse(proto.IsInitialized())
    proto.optional_message.b = 0
    proto.optional_message.c = 0
    self.assertTrue(proto.IsInitialized())

    # Uninitialized repeated submessage.
    message1 = proto.repeated_message.add()
    self.assertFalse(proto.IsInitialized())
    message1.a = message1.b = message1.c = 0
    self.assertTrue(proto.IsInitialized())

    # Uninitialized repeated group in an extension.
    proto = unittest_pb2.TestAllExtensions()
    extension = unittest_pb2.TestRequired.multi
    message1 = proto.Extensions[extension].add()
    message2 = proto.Extensions[extension].add()
    self.assertFalse(proto.IsInitialized())
    message1.a = 1
    message1.b = 1
    message1.c = 1
    self.assertFalse(proto.IsInitialized())
    message2.a = 2
    message2.b = 2
    message2.c = 2
    self.assertTrue(proto.IsInitialized())

    # Uninitialized nonrepeated message in an extension.
    proto = unittest_pb2.TestAllExtensions()
    extension = unittest_pb2.TestRequired.single
    proto.Extensions[extension].a = 1
    self.assertFalse(proto.IsInitialized())
    proto.Extensions[extension].b = 2
    proto.Extensions[extension].c = 3
    self.assertTrue(proto.IsInitialized())

  def testStringUTF8Encoding(self):
    proto = unittest_pb2.TestAllTypes()

    # Assignment of a unicode object to a field of type 'bytes' is not allowed.
    self.assertRaises(TypeError,
                      setattr, proto, 'optional_bytes', u'unicode object')

    # Check that the default value is of python's 'unicode' type.
    self.assertEqual(type(proto.optional_string), unicode)

    proto.optional_string = unicode('Testing')
    self.assertEqual(proto.optional_string, str('Testing'))

    # Assign a value of type 'str' which can be encoded in UTF-8.
    proto.optional_string = str('Testing')
    self.assertEqual(proto.optional_string, unicode('Testing'))

    # Values of type 'str' are also accepted as long as they can be encoded in
    # UTF-8.
    self.assertEqual(type(proto.optional_string), str)

    # Try to assign a 'str' value which contains bytes that aren't 7-bit ASCII.
    self.assertRaises(ValueError,
                      setattr, proto, 'optional_string', str('a\x80a'))
    # Assign a 'str' object which contains a UTF-8 encoded string.
    self.assertRaises(ValueError,
                      setattr, proto, 'optional_string', 'Тест')
    # No exception thrown.
    proto.optional_string = 'abc'

  def testStringUTF8Serialization(self):
    proto = unittest_mset_pb2.TestMessageSet()
    extension_message = unittest_mset_pb2.TestMessageSetExtension2
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
    raw.MergeFromString(serialized)

    message2 = unittest_mset_pb2.TestMessageSetExtension2()

    self.assertEqual(1, len(raw.item))
    # Check that the type_id is the same as the tag ID in the .proto file.
    self.assertEqual(raw.item[0].type_id, 1547769)

    # Check the actually bytes on the wire.
    self.assertTrue(
        raw.item[0].message.endswith(test_utf8_bytes))
    message2.MergeFromString(raw.item[0].message)

    self.assertEqual(type(message2.str), unicode)
    self.assertEqual(message2.str, test_utf8)

    # How about if the bytes on the wire aren't a valid UTF-8 encoded string.
    bytes = raw.item[0].message.replace(
        test_utf8_bytes, len(test_utf8_bytes) * '\xff')
    self.assertRaises(UnicodeDecodeError, message2.MergeFromString, bytes)


#  Since we had so many tests for protocol buffer equality, we broke these out
#  into separate TestCase classes.


class TestAllTypesEqualityTest(unittest.TestCase):

  def setUp(self):
    self.first_proto = unittest_pb2.TestAllTypes()
    self.second_proto = unittest_pb2.TestAllTypes()

  def testSelfEquality(self):
    self.assertEqual(self.first_proto, self.first_proto)

  def testEmptyProtosEqual(self):
    self.assertEqual(self.first_proto, self.second_proto)


class FullProtosEqualityTest(unittest.TestCase):

  """Equality tests using completely-full protos as a starting point."""

  def setUp(self):
    self.first_proto = unittest_pb2.TestAllTypes()
    self.second_proto = unittest_pb2.TestAllTypes()
    test_util.SetAllFields(self.first_proto)
    test_util.SetAllFields(self.second_proto)

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
    # TODO(robinson): Replace next two lines with method
    # to set the "has" bit without changing the value,
    # if/when such a method exists.
    self.first_proto.optional_nested_message.bb = 0
    self.first_proto.optional_nested_message.ClearField('bb')
    self.assertEqual(self.first_proto, self.second_proto)


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


class MutualRecursionEqualityTest(unittest.TestCase):

  def testEqualityWithMutualRecursion(self):
    first_proto = unittest_pb2.TestMutualRecursionA()
    second_proto = unittest_pb2.TestMutualRecursionA()
    self.assertEqual(first_proto, second_proto)
    first_proto.bb.a.bb.optional_int32 = 23
    self.assertNotEqual(first_proto, second_proto)
    second_proto.bb.a.bb.optional_int32 = 23
    self.assertEqual(first_proto, second_proto)


class ByteSizeTest(unittest.TestCase):

  def setUp(self):
    self.proto = unittest_pb2.TestAllTypes()
    self.extended_proto = more_extensions_pb2.ExtendedMessage()

  def Size(self):
    return self.proto.ByteSize()

  def testEmptyMessage(self):
    self.assertEqual(0, self.proto.ByteSize())

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


# TODO(robinson): We need cross-language serialization consistency tests.
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

class SerializationTest(unittest.TestCase):

  def testSerializeEmtpyMessage(self):
    first_proto = unittest_pb2.TestAllTypes()
    second_proto = unittest_pb2.TestAllTypes()
    serialized = first_proto.SerializeToString()
    self.assertEqual(first_proto.ByteSize(), len(serialized))
    second_proto.MergeFromString(serialized)
    self.assertEqual(first_proto, second_proto)

  def testSerializeAllFields(self):
    first_proto = unittest_pb2.TestAllTypes()
    second_proto = unittest_pb2.TestAllTypes()
    test_util.SetAllFields(first_proto)
    serialized = first_proto.SerializeToString()
    self.assertEqual(first_proto.ByteSize(), len(serialized))
    second_proto.MergeFromString(serialized)
    self.assertEqual(first_proto, second_proto)

  def testSerializeAllExtensions(self):
    first_proto = unittest_pb2.TestAllExtensions()
    second_proto = unittest_pb2.TestAllExtensions()
    test_util.SetAllExtensions(first_proto)
    serialized = first_proto.SerializeToString()
    second_proto.MergeFromString(serialized)
    self.assertEqual(first_proto, second_proto)

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
    d = decoder.Decoder(serialized)
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

    second_proto.MergeFromString(serialized)
    # Ensure that we append to repeated fields.
    self.assertEqual(['baz', 'foobar'], list(second_proto.repeated_string))
    # Ensure that we overwrite nonrepeatd scalars.
    self.assertEqual(23, second_proto.optional_int32)
    # Ensure that we recursively call MergeFromString() on
    # submessages.
    self.assertEqual(42, second_proto.optional_nested_message.bb)

  def testMessageSetWireFormat(self):
    proto = unittest_mset_pb2.TestMessageSet()
    extension_message1 = unittest_mset_pb2.TestMessageSetExtension1
    extension_message2 = unittest_mset_pb2.TestMessageSetExtension2
    extension1 = extension_message1.message_set_extension
    extension2 = extension_message2.message_set_extension
    proto.Extensions[extension1].i = 123
    proto.Extensions[extension2].str = 'foo'

    # Serialize using the MessageSet wire format (this is specified in the
    # .proto file).
    serialized = proto.SerializeToString()

    raw = unittest_mset_pb2.RawMessageSet()
    self.assertEqual(False,
                     raw.DESCRIPTOR.GetOptions().message_set_wire_format)
    raw.MergeFromString(serialized)
    self.assertEqual(2, len(raw.item))

    message1 = unittest_mset_pb2.TestMessageSetExtension1()
    message1.MergeFromString(raw.item[0].message)
    self.assertEqual(123, message1.i)

    message2 = unittest_mset_pb2.TestMessageSetExtension2()
    message2.MergeFromString(raw.item[1].message)
    self.assertEqual('foo', message2.str)

    # Deserialize using the MessageSet wire format.
    proto2 = unittest_mset_pb2.TestMessageSet()
    proto2.MergeFromString(serialized)
    self.assertEqual(123, proto2.Extensions[extension1].i)
    self.assertEqual('foo', proto2.Extensions[extension2].str)

    # Check byte size.
    self.assertEqual(proto2.ByteSize(), len(serialized))
    self.assertEqual(proto.ByteSize(), len(serialized))

  def testMessageSetWireFormatUnknownExtension(self):
    # Create a message using the message set wire format with an unknown
    # message.
    raw = unittest_mset_pb2.RawMessageSet()

    # Add an item.
    item = raw.item.add()
    item.type_id = 1545008
    extension_message1 = unittest_mset_pb2.TestMessageSetExtension1
    message1 = unittest_mset_pb2.TestMessageSetExtension1()
    message1.i = 12345
    item.message = message1.SerializeToString()

    # Add a second, unknown extension.
    item = raw.item.add()
    item.type_id = 1545009
    extension_message1 = unittest_mset_pb2.TestMessageSetExtension1
    message1 = unittest_mset_pb2.TestMessageSetExtension1()
    message1.i = 12346
    item.message = message1.SerializeToString()

    # Add another unknown extension.
    item = raw.item.add()
    item.type_id = 1545010
    message1 = unittest_mset_pb2.TestMessageSetExtension2()
    message1.str = 'foo'
    item.message = message1.SerializeToString()

    serialized = raw.SerializeToString()

    # Parse message using the message set wire format.
    proto = unittest_mset_pb2.TestMessageSet()
    proto.MergeFromString(serialized)

    # Check that the message parsed well.
    extension_message1 = unittest_mset_pb2.TestMessageSetExtension1
    extension1 = extension_message1.message_set_extension
    self.assertEquals(12345, proto.Extensions[extension1].i)

  def testUnknownFields(self):
    proto = unittest_pb2.TestAllTypes()
    test_util.SetAllFields(proto)

    serialized = proto.SerializeToString()

    # The empty message should be parsable with all of the fields
    # unknown.
    proto2 = unittest_pb2.TestEmptyMessage()

    # Parsing this message should succeed.
    proto2.MergeFromString(serialized)

    # Now test with a int64 field set.
    proto = unittest_pb2.TestAllTypes()
    proto.optional_int64 = 0x0fffffffffffffff
    serialized = proto.SerializeToString()
    # The empty message should be parsable with all of the fields
    # unknown.
    proto2 = unittest_pb2.TestEmptyMessage()
    # Parsing this message should succeed.
    proto2.MergeFromString(serialized)

  def _CheckRaises(self, exc_class, callable_obj, exception):
    """This method checks if the excpetion type and message are as expected."""
    try:
      callable_obj()
    except exc_class, ex:
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
        'Required field protobuf_unittest.TestRequired.a is not set.')
    # Shouldn't raise exceptions.
    partial = proto.SerializePartialToString()

    proto.a = 1
    self._CheckRaises(
        message.EncodeError,
        proto.SerializeToString,
        'Required field protobuf_unittest.TestRequired.b is not set.')
    # Shouldn't raise exceptions.
    partial = proto.SerializePartialToString()

    proto.b = 2
    self._CheckRaises(
        message.EncodeError,
        proto.SerializeToString,
        'Required field protobuf_unittest.TestRequired.c is not set.')
    # Shouldn't raise exceptions.
    partial = proto.SerializePartialToString()

    proto.c = 3
    serialized = proto.SerializeToString()
    # Shouldn't raise exceptions.
    partial = proto.SerializePartialToString()

    proto2 = unittest_pb2.TestRequired()
    proto2.MergeFromString(serialized)
    self.assertEqual(1, proto2.a)
    self.assertEqual(2, proto2.b)
    self.assertEqual(3, proto2.c)
    proto2.ParseFromString(partial)
    self.assertEqual(1, proto2.a)
    self.assertEqual(2, proto2.b)
    self.assertEqual(3, proto2.c)


class OptionsTest(unittest.TestCase):

  def testMessageOptions(self):
    proto = unittest_mset_pb2.TestMessageSet()
    self.assertEqual(True,
                     proto.DESCRIPTOR.GetOptions().message_set_wire_format)
    proto = unittest_pb2.TestAllTypes()
    self.assertEqual(False,
                     proto.DESCRIPTOR.GetOptions().message_set_wire_format)


class UtilityTest(unittest.TestCase):

  def testImergeSorted(self):
    ImergeSorted = reflection._ImergeSorted
    # Various types of emptiness.
    self.assertEqual([], list(ImergeSorted()))
    self.assertEqual([], list(ImergeSorted([])))
    self.assertEqual([], list(ImergeSorted([], [])))

    # One nonempty list.
    self.assertEqual([1, 2, 3], list(ImergeSorted([1, 2, 3])))
    self.assertEqual([1, 2, 3], list(ImergeSorted([1, 2, 3], [])))
    self.assertEqual([1, 2, 3], list(ImergeSorted([], [1, 2, 3])))

    # Merging some nonempty lists together.
    self.assertEqual([1, 2, 3], list(ImergeSorted([1, 3], [2])))
    self.assertEqual([1, 2, 3], list(ImergeSorted([1], [3], [2])))
    self.assertEqual([1, 2, 3], list(ImergeSorted([1], [3], [2], [])))

    # Elements repeated across component iterators.
    self.assertEqual([1, 2, 2, 3, 3],
                     list(ImergeSorted([1, 2], [3], [2, 3])))

    # Elements repeated within an iterator.
    self.assertEqual([1, 2, 2, 3, 3],
                     list(ImergeSorted([1, 2, 2], [3], [3])))


if __name__ == '__main__':
  unittest.main()
