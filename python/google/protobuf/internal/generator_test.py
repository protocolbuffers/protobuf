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

# TODO(robinson): Flesh this out considerably.  We focused on reflection_test.py
# first, since it's testing the subtler code, and since it provides decent
# indirect testing of the protocol compiler output.

"""Unittest that directly tests the output of the pure-Python protocol
compiler.  See //google/protobuf/internal/reflection_test.py for a test which
further ensures that we can use Python protocol message objects as we expect.
"""

__author__ = 'robinson@google.com (Will Robinson)'

import math

import pytest

from google.protobuf.internal import test_bad_identifiers_pb2
from google.protobuf import service
from google.protobuf import symbol_database
from google.protobuf import unittest_import_pb2
from google.protobuf import unittest_import_public_pb2
from google.protobuf import unittest_mset_pb2
from google.protobuf import unittest_mset_wire_format_pb2
from google.protobuf import unittest_pb2
from google.protobuf import unittest_retention_pb2
from google.protobuf import unittest_custom_options_pb2
from google.protobuf import unittest_no_generic_services_pb2

MAX_EXTENSION = 536870912


class TestGenerator:
    def test_nested_message_descriptor(self):
        field_name = 'optional_nested_message'
        proto_type = unittest_pb2.TestAllTypes
        assert proto_type.NestedMessage.DESCRIPTOR == proto_type.DESCRIPTOR.fields_by_name[field_name].message_type

    def test_enums(self):
        # We test only module-level enums here.
        # TODO(robinson): Examine descriptors directly to check
        # enum descriptor output.
        assert 4 == unittest_pb2.FOREIGN_FOO
        assert 5 == unittest_pb2.FOREIGN_BAR
        assert 6 == unittest_pb2.FOREIGN_BAZ

        proto = unittest_pb2.TestAllTypes()
        assert 1 == proto.FOO
        assert 1 == unittest_pb2.TestAllTypes.FOO
        assert 2 == proto.BAR
        assert 2 == unittest_pb2.TestAllTypes.BAR
        assert 3 == proto.BAZ
        assert 3 == unittest_pb2.TestAllTypes.BAZ

    def test_extreme_default_values(self):
        message = unittest_pb2.TestExtremeDefaultValues()

        assert math.isinf(message.inf_double)
        assert message.inf_double > 0
        assert math.isinf(message.neg_inf_double)
        assert message.neg_inf_double < 0
        assert math.isnan(message.nan_double)

        assert math.isinf(message.inf_float)
        assert message.inf_float > 0
        assert math.isinf(message.neg_inf_float)
        assert message.neg_inf_float < 0
        assert math.isnan(message.nan_float)
        assert "? ? ?? ?? ??? ??/ ??-" == message.cpp_trigraph

    def test_has_default_values(self):
        desc = unittest_pb2.TestAllTypes.DESCRIPTOR

        expected_has_default_by_name = {
            'optional_int32': False,
            'repeated_int32': False,
            'optional_nested_message': False,
            'default_int32': True,
        }

        has_default_by_name = dict(
            [(f.name, f.has_default_value)
            for f in desc.fields
            if f.name in expected_has_default_by_name])
        assert expected_has_default_by_name == has_default_by_name

    def test_containing_type_behavior_for_extensions(self):
        assert unittest_pb2.optional_int32_extension.containing_type == unittest_pb2.TestAllExtensions.DESCRIPTOR
        assert unittest_pb2.TestRequired.single.containing_type == unittest_pb2.TestAllExtensions.DESCRIPTOR

    def test_extension_scope(self):
        assert unittest_pb2.optional_int32_extension.extension_scope == None
        assert unittest_pb2.TestRequired.single.extension_scope == unittest_pb2.TestRequired.DESCRIPTOR

    def test_is_extension(self):
        assert unittest_pb2.optional_int32_extension.is_extension
        assert unittest_pb2.TestRequired.single.is_extension

        message_descriptor = unittest_pb2.TestRequired.DESCRIPTOR
        non_extension_descriptor = message_descriptor.fields_by_name['a']
        assert not non_extension_descriptor.is_extension

    def test_options(self):
        proto = unittest_mset_wire_format_pb2.TestMessageSet()
        assert proto.DESCRIPTOR.GetOptions().message_set_wire_format

    def test_message_with_custom_options(self):
        proto = unittest_custom_options_pb2.TestMessageWithCustomOptions()
        enum_options = proto.DESCRIPTOR.enum_types_by_name['AnEnum'].GetOptions()
        assert enum_options is not None
        # TODO(gps): We really should test for the presence of the enum_opt1
        # extension and for its value to be set to -789.

    # Options that are explicitly marked RETENTION_SOURCE should not be present
    # in the descriptors in the binary.
    def test_option_retention(self):
        # Direct options
        options = unittest_retention_pb2.DESCRIPTOR.GetOptions()
        assert options.HasExtension(unittest_retention_pb2.plain_option)
        assert options.HasExtension(unittest_retention_pb2.runtime_retention_option)
        assert not options.HasExtension(unittest_retention_pb2.source_retention_option)

        def check_options_message_is_stripped_correctly(options):
            assert options.plain_field == 1
            assert options.runtime_retention_field == 2
            assert not options.HasField('source_retention_field')
            assert options.source_retention_field == 0

        # Verify that our test OptionsMessage is stripped correctly on all
        # different entity types.
        check_options_message_is_stripped_correctly(
            options.Extensions[unittest_retention_pb2.file_option]
        )
        check_options_message_is_stripped_correctly(
            unittest_retention_pb2.TopLevelMessage.DESCRIPTOR.GetOptions().Extensions[
                unittest_retention_pb2.message_option
            ]
        )
        check_options_message_is_stripped_correctly(
            unittest_retention_pb2.TopLevelMessage.NestedMessage.DESCRIPTOR.GetOptions().Extensions[
                unittest_retention_pb2.message_option
            ]
        )
        check_options_message_is_stripped_correctly(
            unittest_retention_pb2._TOPLEVELENUM.GetOptions().Extensions[
                unittest_retention_pb2.enum_option
            ]
        )
        check_options_message_is_stripped_correctly(
            unittest_retention_pb2._TOPLEVELMESSAGE_NESTEDENUM.GetOptions().Extensions[
                unittest_retention_pb2.enum_option
            ]
        )
        check_options_message_is_stripped_correctly(
            unittest_retention_pb2._TOPLEVELENUM.values[0]
            .GetOptions()
            .Extensions[unittest_retention_pb2.enum_entry_option]
        )
        check_options_message_is_stripped_correctly(
            unittest_retention_pb2.DESCRIPTOR.extensions_by_name['i']
            .GetOptions()
            .Extensions[unittest_retention_pb2.field_option]
        )
        check_options_message_is_stripped_correctly(
            unittest_retention_pb2.TopLevelMessage.DESCRIPTOR.fields[0]
            .GetOptions()
            .Extensions[unittest_retention_pb2.field_option]
        )
        check_options_message_is_stripped_correctly(
            unittest_retention_pb2.TopLevelMessage.DESCRIPTOR.oneofs[0]
            .GetOptions()
            .Extensions[unittest_retention_pb2.oneof_option]
        )
        check_options_message_is_stripped_correctly(
            unittest_retention_pb2.DESCRIPTOR.services_by_name['Service']
            .GetOptions()
            .Extensions[unittest_retention_pb2.service_option]
        )
        check_options_message_is_stripped_correctly(
            unittest_retention_pb2.DESCRIPTOR.services_by_name['Service']
            .methods[0]
            .GetOptions()
            .Extensions[unittest_retention_pb2.method_option]
        )

    def test_nested_types(self):
        assert (
            set(unittest_pb2.TestAllTypes.DESCRIPTOR.nested_types)
            == set([
                unittest_pb2.TestAllTypes.NestedMessage.DESCRIPTOR,
                unittest_pb2.TestAllTypes.OptionalGroup.DESCRIPTOR,
                unittest_pb2.TestAllTypes.RepeatedGroup.DESCRIPTOR,
            ]))
        assert unittest_pb2.TestEmptyMessage.DESCRIPTOR.nested_types == []
        assert unittest_pb2.TestAllTypes.NestedMessage.DESCRIPTOR.nested_types == []

    def test_containing_type(self):
        assert unittest_pb2.TestEmptyMessage.DESCRIPTOR.containing_type is None
        assert unittest_pb2.TestAllTypes.DESCRIPTOR.containing_type is None
        assert unittest_pb2.TestAllTypes.NestedMessage.DESCRIPTOR.containing_type == unittest_pb2.TestAllTypes.DESCRIPTOR
        assert unittest_pb2.TestAllTypes.NestedMessage.DESCRIPTOR.containing_type == unittest_pb2.TestAllTypes.DESCRIPTOR
        assert unittest_pb2.TestAllTypes.RepeatedGroup.DESCRIPTOR.containing_type == unittest_pb2.TestAllTypes.DESCRIPTOR

    def test_containing_type_in_enum_descriptor(self):
        assert unittest_pb2._FOREIGNENUM.containing_type is None
        assert unittest_pb2._TESTALLTYPES_NESTEDENUM.containing_type == unittest_pb2.TestAllTypes.DESCRIPTOR

    def test_package(self):
        assert unittest_pb2.TestAllTypes.DESCRIPTOR.file.package == 'protobuf_unittest'
        desc = unittest_pb2.TestAllTypes.NestedMessage.DESCRIPTOR
        assert desc.file.package == 'protobuf_unittest'
        assert unittest_import_pb2.ImportMessage.DESCRIPTOR.file.package == 'protobuf_unittest_import'

        assert unittest_pb2._FOREIGNENUM.file.package == 'protobuf_unittest'
        assert unittest_pb2._TESTALLTYPES_NESTEDENUM.file.package == 'protobuf_unittest'
        assert unittest_import_pb2._IMPORTENUM.file.package == 'protobuf_unittest_import'

    def test_extension_range(self):
        assert unittest_pb2.TestAllTypes.DESCRIPTOR.extension_ranges == []
        assert unittest_pb2.TestAllExtensions.DESCRIPTOR.extension_ranges, [(1 == MAX_EXTENSION)]
        assert unittest_pb2.TestMultipleExtensionRanges.DESCRIPTOR.extension_ranges, [(42, 43), (4143, 4244), (65536 == MAX_EXTENSION)]

    def test_file_descriptor(self):
        assert unittest_pb2.DESCRIPTOR.name == 'google/protobuf/unittest.proto'
        assert unittest_pb2.DESCRIPTOR.package == 'protobuf_unittest'
        assert unittest_pb2.DESCRIPTOR.serialized_pb is not None
        assert unittest_pb2.DESCRIPTOR.dependencies == [unittest_import_pb2.DESCRIPTOR]
        assert unittest_import_pb2.DESCRIPTOR.dependencies == [unittest_import_public_pb2.DESCRIPTOR]
        assert unittest_import_pb2.DESCRIPTOR.public_dependencies == [unittest_import_public_pb2.DESCRIPTOR]

    def test_no_generic_services(self):
        assert hasattr(unittest_no_generic_services_pb2, "TestMessage")
        assert hasattr(unittest_no_generic_services_pb2, "FOO")
        assert hasattr(unittest_no_generic_services_pb2, "test_extension")

        # Make sure unittest_no_generic_services_pb2 has no services subclassing
        # Proto2 Service class.
        if hasattr(unittest_no_generic_services_pb2, "TestService"):
            assert not issubclass(unittest_no_generic_services_pb2.TestService, service.Service)

    def test_message_types_by_name(self):
        file_type = unittest_pb2.DESCRIPTOR
        assert unittest_pb2._TESTALLTYPES == file_type.message_types_by_name[unittest_pb2._TESTALLTYPES.name]

        # Nested messages shouldn't be included in the message_types_by_name
        # dictionary (like in the C++ API).
        assert unittest_pb2._TESTALLTYPES_NESTEDMESSAGE.name not in file_type.message_types_by_name

    def test_enum_types_by_name(self):
        file_type = unittest_pb2.DESCRIPTOR
        assert unittest_pb2._FOREIGNENUM == file_type.enum_types_by_name[unittest_pb2._FOREIGNENUM.name]

    def test_extensions_by_name(self):
        file_type = unittest_pb2.DESCRIPTOR
        assert unittest_pb2.my_extension_string == file_type.extensions_by_name[unittest_pb2.my_extension_string.name]

    def test_public_imports(self):
        # Test public imports as embedded message.
        all_type_proto = unittest_pb2.TestAllTypes()
        assert 0 == all_type_proto.optional_public_import_message.e

        # PublicImportMessage is actually defined in unittest_import_public_pb2
        # module, and is public imported by unittest_import_pb2 module.
        public_import_proto = unittest_import_pb2.PublicImportMessage()
        assert 0 == public_import_proto.e
        assert unittest_import_public_pb2.PublicImportMessage is unittest_import_pb2.PublicImportMessage

    def test_bad_identifiers(self):
        # We're just testing that the code was imported without problems.
        message = test_bad_identifiers_pb2.TestBadIdentifiers()
        assert message.Extensions[test_bad_identifiers_pb2.message] == "foo"
        assert message.Extensions[test_bad_identifiers_pb2.descriptor] == "bar"
        assert message.Extensions[test_bad_identifiers_pb2.reflection] == "baz"
        assert message.Extensions[test_bad_identifiers_pb2.service] == "qux"

    def test_oneof(self):
        desc = unittest_pb2.TestAllTypes.DESCRIPTOR
        assert 1 == len(desc.oneofs)
        assert 'oneof_field' == desc.oneofs[0].name
        assert 0 == desc.oneofs[0].index
        assert desc is desc.oneofs[0].containing_type
        assert desc.oneofs[0] is desc.oneofs_by_name['oneof_field']
        nested_names = set(['oneof_uint32', 'oneof_nested_message',
                            'oneof_string', 'oneof_bytes'])
        assert (
            nested_names
            == set([field.name for field in desc.oneofs[0].fields]))
        for field_name, field_desc in desc.fields_by_name.items():
            if field_name in nested_names:
                assert desc.oneofs[0] is field_desc.containing_oneof
            else:
                assert field_desc.containing_oneof is None

    def test_enum_with_dup_value(self):
        assert 'FOO1' == unittest_pb2.TestEnumWithDupValue.Name(unittest_pb2.FOO1)
        assert 'FOO1' == unittest_pb2.TestEnumWithDupValue.Name(unittest_pb2.FOO2)
        assert 'BAR1' == unittest_pb2.TestEnumWithDupValue.Name(unittest_pb2.BAR1)
        assert 'BAR1' == unittest_pb2.TestEnumWithDupValue.Name(unittest_pb2.BAR2)


class TestSymbolDatabaseRegistration:
    """Checks that messages, enums and files are correctly registered."""

    def test_get_symbol(self):
        assert unittest_pb2.TestAllTypes == symbol_database.Default().GetSymbol('protobuf_unittest.TestAllTypes')
        assert unittest_pb2.TestAllTypes.NestedMessage == symbol_database.Default().GetSymbol('protobuf_unittest.TestAllTypes.NestedMessage')
        with pytest.raises(KeyError):
            symbol_database.Default().GetSymbol('protobuf_unittest.NestedMessage')
        assert unittest_pb2.TestAllTypes.OptionalGroup == symbol_database.Default().GetSymbol('protobuf_unittest.TestAllTypes.OptionalGroup')
        assert unittest_pb2.TestAllTypes.RepeatedGroup == symbol_database.Default().GetSymbol('protobuf_unittest.TestAllTypes.RepeatedGroup')

    def test_enums(self):
        assert 'protobuf_unittest.ForeignEnum' == symbol_database.Default().pool.FindEnumTypeByName('protobuf_unittest.ForeignEnum').full_name
        assert 'protobuf_unittest.TestAllTypes.NestedEnum' == symbol_database.Default().pool.FindEnumTypeByName('protobuf_unittest.TestAllTypes.NestedEnum').full_name

    def test_find_file_by_name(self):
        assert 'google/protobuf/unittest.proto' == symbol_database.Default().pool.FindFileByName('google/protobuf/unittest.proto').name
