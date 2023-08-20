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

"""Tests for google.protobuf.descriptor_pool."""

__author__ = 'matthewtoia@google.com (Matt Toia)'

import copy
import warnings

import pytest

from google.protobuf import descriptor_pb2
from google.protobuf.internal import api_implementation
from google.protobuf.internal import descriptor_pool_test1_pb2
from google.protobuf.internal import descriptor_pool_test2_pb2
from google.protobuf.internal import factory_test1_pb2
from google.protobuf.internal import factory_test2_pb2
from google.protobuf.internal import file_options_test_pb2
from google.protobuf.internal import more_messages_pb2
from google.protobuf.internal import no_package_pb2
from google.protobuf.internal import testing_refleaks
from google.protobuf import descriptor
from google.protobuf import descriptor_database
from google.protobuf import descriptor_pool
from google.protobuf import message_factory
from google.protobuf import symbol_database
from google.protobuf import unittest_import_pb2
from google.protobuf import unittest_import_public_pb2
from google.protobuf import unittest_pb2


warnings.simplefilter('error', DeprecationWarning)


class DescriptorPoolTestBase:
    def test_find_file_by_name(self):
        name1 = 'google/protobuf/internal/factory_test1.proto'
        file_desc1 = self.pool.FindFileByName(name1)
        assert isinstance(file_desc1, descriptor.FileDescriptor)
        assert name1 == file_desc1.name
        assert 'google.protobuf.python.internal' == file_desc1.package
        assert 'Factory1Message' in file_desc1.message_types_by_name

        name2 = 'google/protobuf/internal/factory_test2.proto'
        file_desc2 = self.pool.FindFileByName(name2)
        assert isinstance(file_desc2, descriptor.FileDescriptor)
        assert name2 == file_desc2.name
        assert 'google.protobuf.python.internal' == file_desc2.package
        assert 'Factory2Message' in file_desc2.message_types_by_name

    def test_find_file_by_name_failure(self):
        with pytest.raises(KeyError):
            self.pool.FindFileByName('Does not exist')

    def test_find_file_containing_symbol(self):
        file_desc1 = self.pool.FindFileContainingSymbol(
            'google.protobuf.python.internal.Factory1Message')
        assert isinstance(file_desc1, descriptor.FileDescriptor)
        assert 'google/protobuf/internal/factory_test1.proto' == file_desc1.name
        assert 'google.protobuf.python.internal' == file_desc1.package
        assert 'Factory1Message' in file_desc1.message_types_by_name

        file_desc2 = self.pool.FindFileContainingSymbol(
            'google.protobuf.python.internal.Factory2Message')
        assert isinstance(file_desc2, descriptor.FileDescriptor)
        assert 'google/protobuf/internal/factory_test2.proto' == file_desc2.name
        assert 'google.protobuf.python.internal' == file_desc2.package
        assert 'Factory2Message' in file_desc2.message_types_by_name

        # Tests top level extension.
        file_desc3 = self.pool.FindFileContainingSymbol(
            'google.protobuf.python.internal.another_field')
        assert isinstance(file_desc3, descriptor.FileDescriptor)
        assert 'google/protobuf/internal/factory_test2.proto' == file_desc3.name

        # Tests nested extension inside a message.
        file_desc4 = self.pool.FindFileContainingSymbol(
            'google.protobuf.python.internal.Factory2Message.one_more_field')
        assert isinstance(file_desc4, descriptor.FileDescriptor)
        assert 'google/protobuf/internal/factory_test2.proto' == file_desc4.name

        file_desc5 = self.pool.FindFileContainingSymbol(
            'protobuf_unittest.TestService')
        assert isinstance(file_desc5, descriptor.FileDescriptor)
        assert 'google/protobuf/unittest.proto' == file_desc5.name
        # Tests the generated pool.
        assert descriptor_pool.Default().FindFileContainingSymbol(
            'google.protobuf.python.internal.Factory2Message.one_more_field')
        assert descriptor_pool.Default().FindFileContainingSymbol(
            'google.protobuf.python.internal.another_field')
        assert descriptor_pool.Default().FindFileContainingSymbol(
            'protobuf_unittest.TestService')

        # Can find field.
        file_desc6 = self.pool.FindFileContainingSymbol(
            'google.protobuf.python.internal.Factory1Message.list_value')
        assert isinstance(file_desc6, descriptor.FileDescriptor)
        assert 'google/protobuf/internal/factory_test1.proto' == file_desc6.name

        # Can find top level Enum value.
        file_desc7 = self.pool.FindFileContainingSymbol(
            'google.protobuf.python.internal.FACTORY_1_VALUE_0')
        assert isinstance(file_desc7, descriptor.FileDescriptor)
        assert 'google/protobuf/internal/factory_test1.proto' == file_desc7.name

        # Can find nested Enum value.
        file_desc8 = self.pool.FindFileContainingSymbol(
            'protobuf_unittest.TestAllTypes.FOO')
        assert isinstance(file_desc8, descriptor.FileDescriptor)
        assert 'google/protobuf/unittest.proto' == file_desc8.name

        # TODO(jieluo): Add tests for no package when b/13860351 is fixed.

        pytest.raises(KeyError, self.pool.FindFileContainingSymbol,
                          'google.protobuf.python.internal.Factory1Message.none_field')

    def test_find_file_containing_symbol_failure(self):
        with pytest.raises(KeyError):
            self.pool.FindFileContainingSymbol('Does not exist')

    def test_find_message_type_by_name(self):
        msg1 = self.pool.FindMessageTypeByName(
            'google.protobuf.python.internal.Factory1Message')
        assert isinstance(msg1, descriptor.Descriptor)
        assert 'Factory1Message' == msg1.name
        assert 'google.protobuf.python.internal.Factory1Message' == msg1.full_name
        assert msg1.containing_type is None
        assert not msg1.has_options

        nested_msg1 = msg1.nested_types[0]
        assert 'NestedFactory1Message' == nested_msg1.name
        assert msg1 == nested_msg1.containing_type

        nested_enum1 = msg1.enum_types[0]
        assert 'NestedFactory1Enum' == nested_enum1.name
        assert msg1 == nested_enum1.containing_type

        assert nested_msg1 == msg1.fields_by_name['nested_factory_1_message'].message_type
        assert nested_enum1 == msg1.fields_by_name['nested_factory_1_enum'].enum_type

        msg2 = self.pool.FindMessageTypeByName(
            'google.protobuf.python.internal.Factory2Message')
        assert isinstance(msg2, descriptor.Descriptor)
        assert 'Factory2Message' == msg2.name
        assert 'google.protobuf.python.internal.Factory2Message' == msg2.full_name
        assert msg2.containing_type is None

        nested_msg2 = msg2.nested_types[0]
        assert 'NestedFactory2Message' == nested_msg2.name
        assert msg2 == nested_msg2.containing_type

        nested_enum2 = msg2.enum_types[0]
        assert 'NestedFactory2Enum' == nested_enum2.name
        assert msg2 == nested_enum2.containing_type

        assert nested_msg2 == msg2.fields_by_name['nested_factory_2_message'].message_type
        assert nested_enum2 == msg2.fields_by_name['nested_factory_2_enum'].enum_type

        assert msg2.fields_by_name['int_with_default'].has_default_value
        assert 1776 == msg2.fields_by_name['int_with_default'].default_value

        assert msg2.fields_by_name['double_with_default'].has_default_value
        assert 9.99 == msg2.fields_by_name['double_with_default'].default_value

        assert msg2.fields_by_name['string_with_default'].has_default_value
        assert 'hello world' == msg2.fields_by_name['string_with_default'].default_value

        assert msg2.fields_by_name['bool_with_default'].has_default_value
        assert not msg2.fields_by_name['bool_with_default'].default_value

        assert msg2.fields_by_name['enum_with_default'].has_default_value
        assert 1 == msg2.fields_by_name['enum_with_default'].default_value

        msg3 = self.pool.FindMessageTypeByName(
            'google.protobuf.python.internal.Factory2Message.NestedFactory2Message')
        assert nested_msg2 == msg3

        assert msg2.fields_by_name['bytes_with_default'].has_default_value
        assert b'a\xfb\x00c' == msg2.fields_by_name['bytes_with_default'].default_value

        assert 1 == len(msg2.oneofs)
        assert 1 == len(msg2.oneofs_by_name)
        assert 2 == len(msg2.oneofs[0].fields)
        for name in ['oneof_int', 'oneof_string']:
            assert msg2.oneofs[0] == msg2.fields_by_name[name].containing_oneof
            assert msg2.fields_by_name[name] in msg2.oneofs[0].fields

    def test_find_type_errors(self):
        pytest.raises(TypeError, self.pool.FindExtensionByNumber, '')
        pytest.raises(KeyError, self.pool.FindMethodByName, '')

        # TODO(jieluo): Fix python to raise correct errors.
        if api_implementation.Type() == 'python':
            error_type = AttributeError
        else:
            error_type = TypeError
        pytest.raises(error_type, self.pool.FindMessageTypeByName, 0)
        pytest.raises(error_type, self.pool.FindFieldByName, 0)
        pytest.raises(error_type, self.pool.FindExtensionByName, 0)
        pytest.raises(error_type, self.pool.FindEnumTypeByName, 0)
        pytest.raises(error_type, self.pool.FindOneofByName, 0)
        pytest.raises(error_type, self.pool.FindServiceByName, 0)
        pytest.raises(error_type, self.pool.FindMethodByName, 0)
        pytest.raises(error_type, self.pool.FindFileContainingSymbol, 0)
        if api_implementation.Type() == 'python':
            error_type = KeyError
        pytest.raises(error_type, self.pool.FindFileByName, 0)

    def test_find_message_type_by_name_failure(self):
        with pytest.raises(KeyError):
            self.pool.FindMessageTypeByName('Does not exist')

    def test_find_enum_type_by_name(self):
        enum1 = self.pool.FindEnumTypeByName(
            'google.protobuf.python.internal.Factory1Enum')
        assert isinstance(enum1, descriptor.EnumDescriptor)
        assert 0 == enum1.values_by_name['FACTORY_1_VALUE_0'].number
        assert 1 == enum1.values_by_name['FACTORY_1_VALUE_1'].number
        assert not enum1.has_options

        nested_enum1 = self.pool.FindEnumTypeByName(
            'google.protobuf.python.internal.Factory1Message.NestedFactory1Enum')
        assert isinstance(nested_enum1, descriptor.EnumDescriptor)
        assert 0 == nested_enum1.values_by_name['NESTED_FACTORY_1_VALUE_0'].number
        assert 1 == nested_enum1.values_by_name['NESTED_FACTORY_1_VALUE_1'].number

        enum2 = self.pool.FindEnumTypeByName(
            'google.protobuf.python.internal.Factory2Enum')
        assert isinstance(enum2, descriptor.EnumDescriptor)
        assert 0 == enum2.values_by_name['FACTORY_2_VALUE_0'].number
        assert 1 == enum2.values_by_name['FACTORY_2_VALUE_1'].number

        nested_enum2 = self.pool.FindEnumTypeByName(
            'google.protobuf.python.internal.Factory2Message.NestedFactory2Enum')
        assert isinstance(nested_enum2, descriptor.EnumDescriptor)
        assert 0 == nested_enum2.values_by_name['NESTED_FACTORY_2_VALUE_0'].number
        assert 1 == nested_enum2.values_by_name['NESTED_FACTORY_2_VALUE_1'].number

    def test_find_enum_type_by_name_failure(self):
        with pytest.raises(KeyError):
            self.pool.FindEnumTypeByName('Does not exist')

    def test_find_field_by_name(self):
        field = self.pool.FindFieldByName(
            'google.protobuf.python.internal.Factory1Message.list_value')
        assert field.name == 'list_value'
        assert field.label == field.LABEL_REPEATED
        assert not field.has_options

        with pytest.raises(KeyError):
            self.pool.FindFieldByName('Does not exist')

    def testFindOneofByName(self):
        oneof = self.pool.FindOneofByName(
            'google.protobuf.python.internal.Factory2Message.oneof_field')
        assert oneof.name == 'oneof_field'
        with pytest.raises(KeyError):
            self.pool.FindOneofByName('Does not exist')

    def test_find_extension_by_name(self):
        # An extension defined in a message.
        extension = self.pool.FindExtensionByName(
            'google.protobuf.python.internal.Factory2Message.one_more_field')
        assert extension.name == 'one_more_field'
        # An extension defined at file scope.
        extension = self.pool.FindExtensionByName(
            'google.protobuf.python.internal.another_field')
        assert extension.name == 'another_field'
        assert extension.number == 1002
        with pytest.raises(KeyError):
            self.pool.FindFieldByName('Does not exist')

    def test_find_all_extensions(self):
        factory1_message = self.pool.FindMessageTypeByName(
            'google.protobuf.python.internal.Factory1Message')
        factory2_message = self.pool.FindMessageTypeByName(
            'google.protobuf.python.internal.Factory2Message')
        # An extension defined in a message.
        one_more_field = factory2_message.extensions_by_name['one_more_field']
        # An extension defined at file scope.
        factory_test2 = self.pool.FindFileByName(
            'google/protobuf/internal/factory_test2.proto')
        another_field = factory_test2.extensions_by_name['another_field']

        extensions = self.pool.FindAllExtensions(factory1_message)
        expected_extension_numbers = set([one_more_field, another_field])
        assert expected_extension_numbers == set(extensions)
        # Verify that mutating the returned list does not affect the pool.
        extensions.append('unexpected_element')
        # Get the extensions again, the returned value does not contain the
        # 'unexpected_element'.
        extensions = self.pool.FindAllExtensions(factory1_message)
        assert expected_extension_numbers == set(extensions)

    def test_find_extension_by_number(self):
        factory1_message = self.pool.FindMessageTypeByName(
            'google.protobuf.python.internal.Factory1Message')
        # Build factory_test2.proto which will put extensions to the pool
        self.pool.FindFileByName('google/protobuf/internal/factory_test2.proto')

        # An extension defined in a message.
        extension = self.pool.FindExtensionByNumber(factory1_message, 1001)
        assert extension.name == 'one_more_field'
        # An extension defined at file scope.
        extension = self.pool.FindExtensionByNumber(factory1_message, 1002)
        assert extension.name == 'another_field'
        with pytest.raises(KeyError):
            extension = self.pool.FindExtensionByNumber(factory1_message, 1234567)

    def test_extensions_are_not_fields(self):
        with pytest.raises(KeyError):
            self.pool.FindFieldByName('google.protobuf.python.internal.another_field')
        with pytest.raises(KeyError):
            self.pool.FindFieldByName(
                'google.protobuf.python.internal.Factory2Message.one_more_field')
        with pytest.raises(KeyError):
            self.pool.FindExtensionByName(
                'google.protobuf.python.internal.Factory1Message.list_value')

    def test_find_service(self):
        service = self.pool.FindServiceByName('protobuf_unittest.TestService')
        assert service.full_name == 'protobuf_unittest.TestService'
        with pytest.raises(KeyError):
            self.pool.FindServiceByName('Does not exist')

        method = self.pool.FindMethodByName('protobuf_unittest.TestService.Foo')
        assert method.containing_service is service
        with pytest.raises(KeyError):
            self.pool.FindMethodByName('protobuf_unittest.TestService.Doesnotexist')

    def test_user_defined_db(self):
        db = descriptor_database.DescriptorDatabase()
        self.pool = descriptor_pool.DescriptorPool(db)
        db.Add(self.factory_test1_fd)
        db.Add(self.factory_test2_fd)
        self.test_find_message_type_by_name()

    def test_add_serialized_file(self):
        if isinstance(self, SecondaryDescriptorFromDescriptorDB):
            if api_implementation.Type() != 'python':
                # Cpp extension cannot call Add on a DescriptorPool
                # that uses a DescriptorDatabase.
                # TODO(jieluo): Fix python and cpp extension diff.
                return
        self.pool = descriptor_pool.DescriptorPool()
        file1 = self.pool.AddSerializedFile(
            self.factory_test1_fd.SerializeToString())
        file2 = self.pool.AddSerializedFile(
            self.factory_test2_fd.SerializeToString())
        assert file1.name == 'google/protobuf/internal/factory_test1.proto'
        assert file2.name == 'google/protobuf/internal/factory_test2.proto'
        self.test_find_message_type_by_name()
        file_json = self.pool.AddSerializedFile(
            more_messages_pb2.DESCRIPTOR.serialized_pb)
        field = file_json.message_types_by_name['class'].fields_by_name['int_field']
        assert field.json_name == 'json_int'

    def test_add_serialized_file_twice(self):
        if isinstance(self, SecondaryDescriptorFromDescriptorDB):
            if api_implementation.Type() != 'python':
                # Cpp extension cannot call Add on a DescriptorPool
                # that uses a DescriptorDatabase.
                # TODO(jieluo): Fix python and cpp extension diff.
                return
        self.pool = descriptor_pool.DescriptorPool()
        file1_first = self.pool.AddSerializedFile(
            self.factory_test1_fd.SerializeToString())
        file1_again = self.pool.AddSerializedFile(
            self.factory_test1_fd.SerializeToString())
        assert file1_first is file1_again

    def test_enum_default_value(self):
        """Test the default value of enums which don't start at zero."""
        def _check_default_value(file_descriptor):
            default_value = (file_descriptor
                            .message_types_by_name['DescriptorPoolTest1']
                            .fields_by_name['nested_enum']
                            .default_value)
            assert default_value == descriptor_pool_test1_pb2.DescriptorPoolTest1.BETA
        # First check what the generated descriptor contains.
        _check_default_value(descriptor_pool_test1_pb2.DESCRIPTOR)
        # Then check the generated pool. Normally this is the same descriptor.
        file_descriptor = symbol_database.Default().pool.FindFileByName(
            'google/protobuf/internal/descriptor_pool_test1.proto')
        assert file_descriptor is descriptor_pool_test1_pb2.DESCRIPTOR
        _check_default_value(file_descriptor)

        if isinstance(self, SecondaryDescriptorFromDescriptorDB):
            if api_implementation.Type() != 'python':
                # Cpp extension cannot call Add on a DescriptorPool
                # that uses a DescriptorDatabase.
                # TODO(jieluo): Fix python and cpp extension diff.
                return
        # Then check the dynamic pool and its internal DescriptorDatabase.
        descriptor_proto = descriptor_pb2.FileDescriptorProto.FromString(
            descriptor_pool_test1_pb2.DESCRIPTOR.serialized_pb)
        self.pool.Add(descriptor_proto)
        # And do the same check as above
        file_descriptor = self.pool.FindFileByName(
            'google/protobuf/internal/descriptor_pool_test1.proto')
        _check_default_value(file_descriptor)

    def test_default_value_for_custom_messages(self):
        """Check the value returned by non-existent fields."""
        def _check_value_and_type(value, expected_value, expected_type):
            assert value == expected_value
            assert isinstance(value, expected_type)

        def _check_default_values(msg):
            try:
                int64 = long
            except NameError:  # Python3
                int64 = int
            try:
                unicode_type = unicode
            except NameError:  # Python3
                unicode_type = str
            _check_value_and_type(msg.optional_int32, 0, int)
            _check_value_and_type(msg.optional_uint64, 0, (int64, int))
            _check_value_and_type(msg.optional_float, 0, (float, int))
            _check_value_and_type(msg.optional_double, 0, (float, int))
            _check_value_and_type(msg.optional_bool, False, bool)
            _check_value_and_type(msg.optional_string, '', unicode_type)
            _check_value_and_type(msg.optional_bytes, b'', bytes)
            _check_value_and_type(msg.optional_nested_enum, msg.FOO, int)
        # First for the generated message
        _check_default_values(unittest_pb2.TestAllTypes())
        # Then for a message built with from the DescriptorPool.
        pool = descriptor_pool.DescriptorPool()
        pool.Add(descriptor_pb2.FileDescriptorProto.FromString(
            unittest_import_public_pb2.DESCRIPTOR.serialized_pb))
        pool.Add(descriptor_pb2.FileDescriptorProto.FromString(
            unittest_import_pb2.DESCRIPTOR.serialized_pb))
        pool.Add(descriptor_pb2.FileDescriptorProto.FromString(
            unittest_pb2.DESCRIPTOR.serialized_pb))
        message_class = message_factory.GetMessageClass(
            pool.FindMessageTypeByName(
                unittest_pb2.TestAllTypes.DESCRIPTOR.full_name))
        _check_default_values(message_class())

    def test_add_file_descriptor(self):
        if isinstance(self, SecondaryDescriptorFromDescriptorDB):
            if api_implementation.Type() != 'python':
                # Cpp extension cannot call Add on a DescriptorPool
                # that uses a DescriptorDatabase.
                # TODO(jieluo): Fix python and cpp extension diff.
                return
        file_desc = descriptor_pb2.FileDescriptorProto(name='some/file.proto')
        self.pool.Add(file_desc)
        self.pool.AddSerializedFile(file_desc.SerializeToString())

    def test_complex_nesting(self):
        if isinstance(self, SecondaryDescriptorFromDescriptorDB):
            if api_implementation.Type() != 'python':
                # Cpp extension cannot call Add on a DescriptorPool
                # that uses a DescriptorDatabase.
                # TODO(jieluo): Fix python and cpp extension diff.
                return
        more_messages_desc = descriptor_pb2.FileDescriptorProto.FromString(
            more_messages_pb2.DESCRIPTOR.serialized_pb)
        test1_desc = descriptor_pb2.FileDescriptorProto.FromString(
            descriptor_pool_test1_pb2.DESCRIPTOR.serialized_pb)
        test2_desc = descriptor_pb2.FileDescriptorProto.FromString(
            descriptor_pool_test2_pb2.DESCRIPTOR.serialized_pb)
        self.pool.Add(more_messages_desc)
        self.pool.Add(test1_desc)
        self.pool.Add(test2_desc)
        TEST1_FILE.CheckFile(self, self.pool)
        TEST2_FILE.CheckFile(self, self.pool)

    def test_conflict_register(self):
        if isinstance(self, SecondaryDescriptorFromDescriptorDB):
            if api_implementation.Type() != 'python':
                # Cpp extension cannot call Add on a DescriptorPool
                # that uses a DescriptorDatabase.
                # TODO(jieluo): Fix python and cpp extension diff.
                return
        unittest_fd = descriptor_pb2.FileDescriptorProto.FromString(
            unittest_pb2.DESCRIPTOR.serialized_pb)
        conflict_fd = copy.deepcopy(unittest_fd)
        conflict_fd.name = 'other_file'
        if api_implementation.Type() != 'python':
            pass
        else:
          pool = copy.deepcopy(self.pool)
          file_descriptor = unittest_pb2.DESCRIPTOR
          pool._AddDescriptor(
              file_descriptor.message_types_by_name['TestAllTypes'])
          pool._AddEnumDescriptor(
              file_descriptor.enum_types_by_name['ForeignEnum'])
          pool._AddServiceDescriptor(
              file_descriptor.services_by_name['TestService'])
          pool._AddExtensionDescriptor(
              file_descriptor.extensions_by_name['optional_int32_extension'])
          pool.Add(unittest_fd)
          with warnings.catch_warnings(record=True) as w:
              warnings.simplefilter('always')
              pool.Add(conflict_fd)
              assert len(w)
              assert w[0].category is RuntimeWarning
              assert 'Conflict register for file "other_file": ' in str(w[0].message)
          pool.FindFileByName(unittest_fd.name)
          with pytest.raises(TypeError):
              pool.FindFileByName(conflict_fd.name)

    def test_type_not_set(self):
        f = descriptor_pb2.FileDescriptorProto(
            name='google/protobuf/internal/not_type.proto',
            package='google.protobuf.python.internal',
            syntax='proto3')
        f.enum_type.add(name='TestEnum').value.add(name='DEFAULTVALUE',
                                                  number=0)
        msg_proto = f.message_type.add(name='TestMessage')
        msg_proto.nested_type.add(name='Nested')
        # type may not set if type_name is set in FieldDescriptorProto
        msg_proto.field.add(name='nested_field',
                            number=1,
                            label=descriptor.FieldDescriptor.LABEL_OPTIONAL,
                            type_name='Nested')
        msg_proto.field.add(name='enum_field',
                            number=2,
                            label=descriptor.FieldDescriptor.LABEL_REPEATED,
                            type_name='TestEnum')
        pool = descriptor_pool.DescriptorPool()
        pool.Add(f)
        file_des = pool.FindFileByName('google/protobuf/internal/not_type.proto')
        msg = file_des.message_types_by_name['TestMessage']
        nested_field = msg.fields_by_name['nested_field']
        assert nested_field.has_presence
        # cpp extension and upb do not provide is_packed on FieldDescriptor
        if api_implementation.Type() == 'python':
          assert not nested_field.is_packed
        enum_field = msg.fields_by_name['enum_field']
        assert not enum_field.has_presence
        if api_implementation.Type() == 'python':
          assert enum_field.is_packed

@testing_refleaks.TestCase
class TestDefaultDescriptorPool(DescriptorPoolTestBase):
    def setup_method(self, method):
        self.pool = descriptor_pool.Default()
        self.factory_test1_fd = descriptor_pb2.FileDescriptorProto.FromString(
            factory_test1_pb2.DESCRIPTOR.serialized_pb)
        self.factory_test2_fd = descriptor_pb2.FileDescriptorProto.FromString(
            factory_test2_pb2.DESCRIPTOR.serialized_pb)

    def test_find_methods(self):
      assert (
          self.pool.FindFileByName('google/protobuf/unittest.proto') is
          unittest_pb2.DESCRIPTOR)
      assert (
          self.pool.FindMessageTypeByName('protobuf_unittest.TestAllTypes') is
          unittest_pb2.TestAllTypes.DESCRIPTOR)
      assert (
          self.pool.FindFieldByName(
              'protobuf_unittest.TestAllTypes.optional_int32') is
          unittest_pb2.TestAllTypes.DESCRIPTOR.fields_by_name['optional_int32'])
      assert (
          self.pool.FindEnumTypeByName('protobuf_unittest.ForeignEnum') is
          unittest_pb2.ForeignEnum.DESCRIPTOR)
      assert (
          self.pool.FindExtensionByName(
              'protobuf_unittest.optional_int32_extension') is
          unittest_pb2.DESCRIPTOR.extensions_by_name['optional_int32_extension'])
      assert (
          self.pool.FindOneofByName('protobuf_unittest.TestAllTypes.oneof_field') is
          unittest_pb2.TestAllTypes.DESCRIPTOR.oneofs_by_name['oneof_field'])
      assert (
          self.pool.FindServiceByName('protobuf_unittest.TestService') is
          unittest_pb2.DESCRIPTOR.services_by_name['TestService'])


@testing_refleaks.TestCase
class TestCreateDescriptorPool(DescriptorPoolTestBase):
    def setup_method(self, method):
        self.pool = descriptor_pool.DescriptorPool()
        self.factory_test1_fd = descriptor_pb2.FileDescriptorProto.FromString(
            factory_test1_pb2.DESCRIPTOR.serialized_pb)
        self.factory_test2_fd = descriptor_pb2.FileDescriptorProto.FromString(
            factory_test2_pb2.DESCRIPTOR.serialized_pb)
        self.pool.Add(self.factory_test1_fd)
        self.pool.Add(self.factory_test2_fd)

        self.pool.Add(descriptor_pb2.FileDescriptorProto.FromString(
            unittest_import_public_pb2.DESCRIPTOR.serialized_pb))
        self.pool.Add(descriptor_pb2.FileDescriptorProto.FromString(
            unittest_import_pb2.DESCRIPTOR.serialized_pb))
        self.pool.Add(descriptor_pb2.FileDescriptorProto.FromString(
            unittest_pb2.DESCRIPTOR.serialized_pb))
        self.pool.Add(descriptor_pb2.FileDescriptorProto.FromString(
            no_package_pb2.DESCRIPTOR.serialized_pb))


@testing_refleaks.TestCase
class SecondaryDescriptorFromDescriptorDB(DescriptorPoolTestBase):
    def setup_method(self, method):
        self.factory_test1_fd = descriptor_pb2.FileDescriptorProto.FromString(
            factory_test1_pb2.DESCRIPTOR.serialized_pb)
        self.factory_test2_fd = descriptor_pb2.FileDescriptorProto.FromString(
            factory_test2_pb2.DESCRIPTOR.serialized_pb)
        self.db = descriptor_database.DescriptorDatabase()
        self.db.Add(self.factory_test1_fd)
        self.db.Add(self.factory_test2_fd)
        self.db.Add(descriptor_pb2.FileDescriptorProto.FromString(
            unittest_import_public_pb2.DESCRIPTOR.serialized_pb))
        self.db.Add(descriptor_pb2.FileDescriptorProto.FromString(
            unittest_import_pb2.DESCRIPTOR.serialized_pb))
        self.db.Add(descriptor_pb2.FileDescriptorProto.FromString(
            unittest_pb2.DESCRIPTOR.serialized_pb))
        self.db.Add(descriptor_pb2.FileDescriptorProto.FromString(
            no_package_pb2.DESCRIPTOR.serialized_pb))
        self.pool = descriptor_pool.DescriptorPool(descriptor_db=self.db)

    def test_error_collector(self):
        file_proto = descriptor_pb2.FileDescriptorProto()
        file_proto.package = 'collector'
        file_proto.name = 'error_file'
        message_type = file_proto.message_type.add()
        message_type.name = 'ErrorMessage'
        field = message_type.field.add()
        field.number = 1
        field.name = 'nested_message_field'
        field.label = descriptor.FieldDescriptor.LABEL_OPTIONAL
        field.type = descriptor.FieldDescriptor.TYPE_MESSAGE
        field.type_name = 'SubMessage'
        oneof = message_type.oneof_decl.add()
        oneof.name = 'MyOneof'
        enum_type = file_proto.enum_type.add()
        enum_type.name = 'MyEnum'
        enum_value = enum_type.value.add()
        enum_value.name = 'MyEnumValue'
        enum_value.number = 0
        self.db.Add(file_proto)

        pytest.raises(KeyError, 'SubMessage',
                            self.pool.FindMessageTypeByName,
                            'collector.ErrorMessage')
        pytest.raises(KeyError, 'SubMessage', self.pool.FindFileByName,
                            'error_file')
        with pytest.raises(KeyError) as exc:
            self.pool.FindFileByName('none_file')
        assert (str(exc.exception) in ('\'none_file\'',
                                        '\"Couldn\'t find file none_file\"'))

        # Pure python _ConvertFileProtoToFileDescriptor() method has side effect
        # that all the symbols found in the file will load into the pool even the
        # file can not build. So when FindMessageTypeByName('ErrorMessage') was
        # called the first time, a KeyError will be raised but call the find
        # method later will return a descriptor which is not build.
        # TODO(jieluo): fix pure python to revert the load if file can not be build
        if api_implementation.Type() != 'python':
            error_msg = ('Invalid proto descriptor for file "error_file":\\n  '
                        'collector.ErrorMessage.nested_message_field: "SubMessage" '
                        'is not defined.\\n  collector.ErrorMessage.MyOneof: Oneof '
                        'must have at least one field.\\n\'')
            with pytest.raises(KeyError) as exc:
                    self.pool.FindMessageTypeByName('collector.ErrorMessage')
            assert str(exc.exception) == '\'Couldn\\\'t build file for message collector.ErrorMessage\\n' + error_msg

            with pytest.raises(KeyError) as exc:
                self.pool.FindFieldByName('collector.ErrorMessage.nested_message_field')
            assert str(exc.exception) == '\'Couldn\\\'t build file for field collector.ErrorMessage.nested_message_field\\n' + error_msg

            with pytest.raises(KeyError) as exc:
                self.pool.FindEnumTypeByName('collector.MyEnum')
            assert str(exc.exception) == '\'Couldn\\\'t build file for enum collector.MyEnum\\n' + error_msg

            with pytest.raises(KeyError) as exc:
                self.pool.FindFileContainingSymbol('collector.MyEnumValue')
            assert str(exc.exception) == '\'Couldn\\\'t build file for symbol collector.MyEnumValue\\n' + error_msg

            with pytest.raises(KeyError) as exc:
                self.pool.FindOneofByName('collector.ErrorMessage.MyOneof')
            assert str(exc.exception) == '\'Couldn\\\'t build file for oneof collector.ErrorMessage.MyOneof\\n' + error_msg


class ProtoFile:
    def __init__(self, name, package, messages, dependencies=None,
                public_dependencies=None):
        self.name = name
        self.package = package
        self.messages = messages
        self.dependencies = dependencies or []
        self.public_dependencies = public_dependencies or []

    def CheckFile(self, test, pool):
        file_desc = pool.FindFileByName(self.name)
        assert self.name == file_desc.name
        assert self.package == file_desc.package
        dependencies_names = [f.name for f in file_desc.dependencies]
        assert self.dependencies == dependencies_names
        public_dependencies_names = [f.name for f in file_desc.public_dependencies]
        assert self.public_dependencies == public_dependencies_names
        for name, msg_type in self.messages.items():
            msg_type.CheckType(test, None, name, file_desc)


class EnumType:
    def __init__(self, values):
        self.values = values

    def CheckType(self, test, msg_desc, name, file_desc):
        enum_desc = msg_desc.enum_types_by_name[name]
        assert name == enum_desc.name
        expected_enum_full_name = '.'.join([msg_desc.full_name, name])
        assert expected_enum_full_name == enum_desc.full_name
        assert msg_desc == enum_desc.containing_type
        assert file_desc == enum_desc.file
        for index, (value, number) in enumerate(self.values):
            value_desc = enum_desc.values_by_name[value]
            assert value == value_desc.name
            assert index == value_desc.index
            assert number == value_desc.number
            assert enum_desc == value_desc.type
            assert value in msg_desc.enum_values_by_name


class MessageType:
    def __init__(self, type_dict, field_list, is_extendable=False,
                extensions=None):
        self.type_dict = type_dict
        self.field_list = field_list
        self.is_extendable = is_extendable
        self.extensions = extensions or []

    def CheckType(self, test, containing_type_desc, name, file_desc):
        if containing_type_desc is None:
            desc = file_desc.message_types_by_name[name]
            expected_full_name = '.'.join([file_desc.package, name])
        else:
            desc = containing_type_desc.nested_types_by_name[name]
            expected_full_name = '.'.join([containing_type_desc.full_name, name])

        assert name == desc.name
        assert expected_full_name == desc.full_name
        assert containing_type_desc == desc.containing_type
        assert desc.file == file_desc
        assert self.is_extendable == desc.is_extendable
        for name, subtype in self.type_dict.items():
            subtype.CheckType(test, desc, name, file_desc)

        for index, (name, field) in enumerate(self.field_list):
            field.CheckField(test, desc, name, index, file_desc)

        for index, (name, field) in enumerate(self.extensions):
            field.CheckField(test, desc, name, index, file_desc)


class EnumField:
    def __init__(self, number, type_name, default_value):
        self.number = number
        self.type_name = type_name
        self.default_value = default_value

    def CheckField(self, test, msg_desc, name, index, file_desc):
        field_desc = msg_desc.fields_by_name[name]
        enum_desc = msg_desc.enum_types_by_name[self.type_name]
        assert name == field_desc.name
        expected_field_full_name = '.'.join([msg_desc.full_name, name])
        assert expected_field_full_name == field_desc.full_name
        assert index == field_desc.index
        assert self.number == field_desc.number
        assert descriptor.FieldDescriptor.TYPE_ENUM == field_desc.type
        assert descriptor.FieldDescriptor.CPPTYPE_ENUM == field_desc.cpp_type
        assert field_desc.has_default_value
        assert enum_desc.values_by_name[self.default_value].number == field_desc.default_value
        assert not enum_desc.values_by_name[self.default_value].has_options
        assert msg_desc == field_desc.containing_type
        assert enum_desc == field_desc.enum_type
        assert file_desc == enum_desc.file


class MessageField:
    def __init__(self, number, type_name):
        self.number = number
        self.type_name = type_name

    def CheckField(self, test, msg_desc, name, index, file_desc):
        field_desc = msg_desc.fields_by_name[name]
        field_type_desc = msg_desc.nested_types_by_name[self.type_name]
        assert name == field_desc.name
        expected_field_full_name = '.'.join([msg_desc.full_name, name])
        assert expected_field_full_name == field_desc.full_name
        assert index == field_desc.index
        assert self.number == field_desc.number
        assert descriptor.FieldDescriptor.TYPE_MESSAGE == field_desc.type
        assert descriptor.FieldDescriptor.CPPTYPE_MESSAGE == field_desc.cpp_type
        assert not field_desc.has_default_value
        assert msg_desc == field_desc.containing_type
        assert field_type_desc == field_desc.message_type
        assert file_desc == field_desc.file
        assert field_desc.default_value == None


class StringField:
    def __init__(self, number, default_value):
        self.number = number
        self.default_value = default_value

    def CheckField(self, test, msg_desc, name, index, file_desc):
        field_desc = msg_desc.fields_by_name[name]
        assert name == field_desc.name
        expected_field_full_name = '.'.join([msg_desc.full_name, name])
        assert expected_field_full_name == field_desc.full_name
        assert index == field_desc.index
        assert self.number == field_desc.number
        assert descriptor.FieldDescriptor.TYPE_STRING == field_desc.type
        assert descriptor.FieldDescriptor.CPPTYPE_STRING == field_desc.cpp_type
        assert field_desc.has_default_value
        assert self.default_value == field_desc.default_value
        assert file_desc == field_desc.file


class ExtensionField:
    def __init__(self, number, extended_type):
        self.number = number
        self.extended_type = extended_type

    def CheckField(self, test, msg_desc, name, index, file_desc):
        field_desc = msg_desc.extensions_by_name[name]
        assert name == field_desc.name
        expected_field_full_name = '.'.join([msg_desc.full_name, name])
        assert expected_field_full_name == field_desc.full_name
        assert self.number == field_desc.number
        assert index == field_desc.index
        assert descriptor.FieldDescriptor.TYPE_MESSAGE == field_desc.type
        assert descriptor.FieldDescriptor.CPPTYPE_MESSAGE == field_desc.cpp_type
        assert not field_desc.has_default_value
        assert field_desc.is_extension
        assert msg_desc == field_desc.extension_scope
        assert msg_desc == field_desc.message_type
        assert self.extended_type == field_desc.containing_type.name
        assert file_desc == field_desc.file


@testing_refleaks.TestCase
class TestAddDescriptor:
    def _test_message(self, prefix):
        pool = descriptor_pool.DescriptorPool()
        pool._AddDescriptor(unittest_pb2.TestAllTypes.DESCRIPTOR)
        assert (
            'protobuf_unittest.TestAllTypes'
            == pool.FindMessageTypeByName(
                prefix + 'protobuf_unittest.TestAllTypes').full_name)

        # AddDescriptor is not recursive.
        with pytest.raises(KeyError):
            pool.FindMessageTypeByName(
                prefix + 'protobuf_unittest.TestAllTypes.NestedMessage')

        pool._AddDescriptor(unittest_pb2.TestAllTypes.NestedMessage.DESCRIPTOR)
        assert (
            'protobuf_unittest.TestAllTypes.NestedMessage'
            == pool.FindMessageTypeByName(
                prefix + 'protobuf_unittest.TestAllTypes.NestedMessage').full_name)

        # Files are implicitly also indexed when messages are added.
        assert (
            'google/protobuf/unittest.proto'
            == pool.FindFileByName('google/protobuf/unittest.proto').name)

        assert (
            'google/protobuf/unittest.proto'
            == pool.FindFileContainingSymbol(
                prefix + 'protobuf_unittest.TestAllTypes.NestedMessage').name)

    @pytest.mark.skipif(
        api_implementation.Type() != 'python',
        reason='Only pure python allows _Add*()')
    def test_message(self):
        self._test_message('')
        self._test_message('.')

    def _test_enum(self, prefix):
        pool = descriptor_pool.DescriptorPool()
        pool.AddSerializedFile(unittest_import_public_pb2.DESCRIPTOR.serialized_pb)
        pool.AddSerializedFile(unittest_import_pb2.DESCRIPTOR.serialized_pb)
        pool.AddSerializedFile(unittest_pb2.DESCRIPTOR.serialized_pb)
        assert (
            'protobuf_unittest.ForeignEnum'
            == pool.FindEnumTypeByName(
                prefix + 'protobuf_unittest.ForeignEnum').full_name)

        # AddEnumDescriptor is not recursive.
        with pytest.raises(KeyError):
          pool.FindEnumTypeByName(
              prefix + 'protobuf_unittest.ForeignEnum.NestedEnum')

        assert (
            'protobuf_unittest.TestAllTypes.NestedEnum'
            == pool.FindEnumTypeByName(
                prefix + 'protobuf_unittest.TestAllTypes.NestedEnum').full_name)

        # Files are implicitly also indexed when enums are added.
        assert (
            'google/protobuf/unittest.proto'
            == pool.FindFileByName('google/protobuf/unittest.proto').name)

        assert (
            'google/protobuf/unittest.proto'
            == pool.FindFileContainingSymbol(
                prefix + 'protobuf_unittest.TestAllTypes.NestedEnum').name)

    @pytest.mark.skipif(
        api_implementation.Type() != 'python',
        reason='Only pure python allows _Add*()')
    def test_enum(self):
        self._test_enum('')
        self._test_enum('.')

    @pytest.mark.skipif(
        api_implementation.Type() != 'python',
        reason='Only pure python allows _Add*()')
    def test_service(self):
        pool = descriptor_pool.DescriptorPool()
        with pytest.raises(KeyError):
            pool.FindServiceByName('protobuf_unittest.TestService')
        pool._AddServiceDescriptor(unittest_pb2._TESTSERVICE)
        assert (
            'protobuf_unittest.TestService'
            == pool.FindServiceByName('protobuf_unittest.TestService').full_name)

    @pytest.mark.skipif(
        api_implementation.Type() != 'python',
        reason='Only pure python allows _Add*()')
    def test_file(self):
        pool = descriptor_pool.DescriptorPool()
        pool._AddFileDescriptor(unittest_pb2.DESCRIPTOR)
        assert (
            'google/protobuf/unittest.proto'
            == pool.FindFileByName('google/protobuf/unittest.proto').name)

        # AddFileDescriptor is not recursive; messages and enums within files must
        # be explicitly registered.
        with pytest.raises(KeyError):
            pool.FindFileContainingSymbol(
                'protobuf_unittest.TestAllTypes')

    def test_empty_descriptor_pool(self):
        # Check that an empty DescriptorPool() contains no messages.
        pool = descriptor_pool.DescriptorPool()
        proto_file_name = descriptor_pb2.DESCRIPTOR.name
        pytest.raises(KeyError, pool.FindFileByName, proto_file_name)
        # Add the above file to the pool
        file_descriptor = descriptor_pb2.FileDescriptorProto()
        descriptor_pb2.DESCRIPTOR.CopyToProto(file_descriptor)
        pool.Add(file_descriptor)
        # Now it exists.
        assert pool.FindFileByName(proto_file_name)

    def test_custom_descriptor_pool(self):
        # Create a new pool, and add a file descriptor.
        pool = descriptor_pool.DescriptorPool()
        file_desc = descriptor_pb2.FileDescriptorProto(
            name='some/file.proto', package='package')
        file_desc.message_type.add(name='Message')
        pool.Add(file_desc)
        assert pool.FindFileByName('some/file.proto').name == 'some/file.proto'
        assert pool.FindMessageTypeByName('package.Message').name == 'Message'
        # Test no package
        file_proto = descriptor_pb2.FileDescriptorProto(
            name='some/filename/container.proto')
        message_proto = file_proto.message_type.add(
            name='TopMessage')
        message_proto.field.add(
            name='bb',
            number=1,
            type=descriptor_pb2.FieldDescriptorProto.TYPE_INT32,
            label=descriptor_pb2.FieldDescriptorProto.LABEL_OPTIONAL)
        enum_proto = file_proto.enum_type.add(name='TopEnum')
        enum_proto.value.add(name='FOREIGN_FOO', number=4)
        file_proto.service.add(name='TopService')
        pool = descriptor_pool.DescriptorPool()
        pool.Add(file_proto)
        assert 'TopMessage' == pool.FindMessageTypeByName('TopMessage').name
        assert 'TopEnum' == pool.FindEnumTypeByName('TopEnum').name
        assert 'TopService' == pool.FindServiceByName('TopService').name

    def test_file_descriptor_options_with_custom_descriptor_pool(self):
        # Create a descriptor pool, and add a new FileDescriptorProto to it.
        pool = descriptor_pool.DescriptorPool()
        file_name = 'file_descriptor_options_with_custom_descriptor_pool.proto'
        file_descriptor_proto = descriptor_pb2.FileDescriptorProto(name=file_name)
        extension_id = file_options_test_pb2.foo_options
        file_descriptor_proto.options.Extensions[extension_id].foo_name = 'foo'
        pool.Add(file_descriptor_proto)
        # The options set on the FileDescriptorProto should be available in the
        # descriptor even if they contain extensions that cannot be deserialized
        # using the pool.
        file_descriptor = pool.FindFileByName(file_name)
        options = file_descriptor.GetOptions()
        assert 'foo' == options.Extensions[extension_id].foo_name
        # The object returned by GetOptions() is cached.
        assert options is file_descriptor.GetOptions()

    def test_add_type_error(self):
        pool = descriptor_pool.DescriptorPool()
        if api_implementation.Type() == 'python':
            with pytest.raises(TypeError):
                pool._AddDescriptor(0)
            with pytest.raises(TypeError):
                pool._AddEnumDescriptor(0)
            with pytest.raises(TypeError):
                pool._AddServiceDescriptor(0)
            with pytest.raises(TypeError):
                pool._AddExtensionDescriptor(0)
            with pytest.raises(TypeError):
                pool._AddFileDescriptor(0)


TEST1_FILE = ProtoFile(
    'google/protobuf/internal/descriptor_pool_test1.proto',
    'google.protobuf.python.internal',
    {
        'DescriptorPoolTest1': MessageType({
            'NestedEnum': EnumType([('ALPHA', 1), ('BETA', 2)]),
            'NestedMessage': MessageType({
                'NestedEnum': EnumType([('EPSILON', 5), ('ZETA', 6)]),
                'DeepNestedMessage': MessageType({
                    'NestedEnum': EnumType([('ETA', 7), ('THETA', 8)]),
                }, [
                    ('nested_enum', EnumField(1, 'NestedEnum', 'ETA')),
                    ('nested_field', StringField(2, 'theta')),
                ]),
            }, [
                ('nested_enum', EnumField(1, 'NestedEnum', 'ZETA')),
                ('nested_field', StringField(2, 'beta')),
                ('deep_nested_message', MessageField(3, 'DeepNestedMessage')),
            ])
        }, [
            ('nested_enum', EnumField(1, 'NestedEnum', 'BETA')),
            ('nested_message', MessageField(2, 'NestedMessage')),
        ], is_extendable=True),

        'DescriptorPoolTest2': MessageType({
            'NestedEnum': EnumType([('GAMMA', 3), ('DELTA', 4)]),
            'NestedMessage': MessageType({
                'NestedEnum': EnumType([('IOTA', 9), ('KAPPA', 10)]),
                'DeepNestedMessage': MessageType({
                    'NestedEnum': EnumType([('LAMBDA', 11), ('MU', 12)]),
                }, [
                    ('nested_enum', EnumField(1, 'NestedEnum', 'MU')),
                    ('nested_field', StringField(2, 'lambda')),
                ]),
            }, [
                ('nested_enum', EnumField(1, 'NestedEnum', 'IOTA')),
                ('nested_field', StringField(2, 'delta')),
                ('deep_nested_message', MessageField(3, 'DeepNestedMessage')),
            ])
        }, [
            ('nested_enum', EnumField(1, 'NestedEnum', 'GAMMA')),
            ('nested_message', MessageField(2, 'NestedMessage')),
        ]),
    })


TEST2_FILE = ProtoFile(
    'google/protobuf/internal/descriptor_pool_test2.proto',
    'google.protobuf.python.internal', {
        'DescriptorPoolTest3':
            MessageType(
                {
                    'NestedEnum':
                        EnumType([('NU', 13), ('XI', 14)]),
                    'NestedMessage':
                        MessageType(
                            {
                                'NestedEnum':
                                    EnumType([('OMICRON', 15), ('PI', 16)]),
                                'DeepNestedMessage':
                                    MessageType(
                                        {
                                            'NestedEnum':
                                                EnumType([('RHO', 17),
                                                          ('SIGMA', 18)]),
                                        }, [
                                            ('nested_enum',
                                             EnumField(1, 'NestedEnum', 'RHO')),
                                            ('nested_field',
                                             StringField(2, 'sigma')),
                                        ]),
                            }, [
                                ('nested_enum', EnumField(
                                    1, 'NestedEnum', 'PI')),
                                ('nested_field', StringField(2, 'nu')),
                                ('deep_nested_message',
                                 MessageField(3, 'DeepNestedMessage')),
                            ])
                }, [
                    ('nested_enum', EnumField(1, 'NestedEnum', 'XI')),
                    ('nested_message', MessageField(2, 'NestedMessage')),
                ],
                extensions=[
                    ('descriptor_pool_test',
                     ExtensionField(1001, 'DescriptorPoolTest1')),
                ]),
    },
    dependencies=[
        'google/protobuf/internal/more_messages.proto',
        'google/protobuf/internal/descriptor_pool_test1.proto',
    ],
    public_dependencies=[
        'google/protobuf/internal/more_messages.proto'])
