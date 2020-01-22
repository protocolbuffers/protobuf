#! /usr/bin/env python
#
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

__author__ = 'rbellevi@google.com (Richard Belleville)'

# pylint: disable=g-statement-before-imports,g-import-not-at-top
try:
  import unittest2 as unittest  # PY26
except ImportError:
  import unittest
# pylint: enable=g-statement-before-imports,g-import-not-at-top

import collections
import functools
import multiprocessing
import sys

from google import protobuf

from google.protobuf.internal import api_implementation

_TEST_PROTO_FILE = 'google/protobuf/internal/test_messages_proto3_dynamic.proto'
_BROKEN_PROTO = 'google/protobuf/internal/broken_test.proto'

_UNINTENTIONAL_PUBLIC_SYMBOLS = ('enum_type_wrapper',)

# These classes allow the collection of the information pertaining to a file
# descriptor as a dumb bag of data. This acts as a proxy for comparing the
# contents of two modules directly. If two FileDescriptor objects result in the
# equal ProtoFile objects, then they will present the same interface to
# user-space applications.


class ProtoFile(
    collections.namedtuple(
        'ProtoFile',
        ('package', 'messages', 'enums', 'dependencies', 'public_dependencies'))
):

  @staticmethod
  def from_file_descriptor(file_descriptor):
    return ProtoFile(file_descriptor.package, [
        MessageType.from_descriptor(descriptor)
        for descriptor in file_descriptor.message_types_by_name.values()
    ], [
        EnumType.from_enum_descriptor(descriptor)
        for descriptor in file_descriptor.enum_types_by_name.values()
    ], [dep.name for dep in file_descriptor.dependencies],
                     [dep.name for dep in file_descriptor.public_dependencies])


class EnumType(
    collections.namedtuple('Enumtype', ('name', 'full_name', 'values'))):

  @staticmethod
  def from_enum_descriptor(enum_descriptor):
    return EnumType(enum_descriptor.name, enum_descriptor.full_name,
                    [(value_descriptor.name, value_descriptor.number)
                     for value_descriptor in enum_descriptor.values])


class MessageType(
    collections.namedtuple(
        'MessageType',
        ('name', 'full_name', 'fields', 'nested_types', 'is_extendable'))):

  @staticmethod
  def from_descriptor(descriptor):
    return MessageType(descriptor.name, descriptor.full_name, [
        MessageField.from_field_descriptor(field_desc)
        for field_desc in descriptor.fields_by_name.values()
    ], [
        MessageType.from_descriptor(nested_desc)
        for nested_desc in descriptor.nested_types
    ], descriptor.is_extendable)


class EnumField(
    collections.namedtuple(
        'Enumfield', ('name', 'full_name', 'number', 'type', 'has_options'))):

  @staticmethod
  def from_enum_field_descriptor(descriptor):
    return EnumField(descriptor.name, descriptor.full_name, descriptor.number,
                     descriptor.type, descriptor.has_options)


class MessageField(
    collections.namedtuple(
        'MessageField',
        ('name', 'full_name', 'number', 'type', 'cpp_type', 'has_default_value',
         'default_value', 'is_extension'))):

  @staticmethod
  def from_field_descriptor(descriptor):
    return MessageField(descriptor.name, descriptor.full_name,
                        descriptor.number, descriptor.type, descriptor.cpp_type,
                        descriptor.has_default_value, descriptor.default_value,
                        descriptor.is_extension)


def _wrap_in_subprocess(result_queue, error_queue, fn):

  @functools.wraps(fn)
  def _wrapped():
    try:
      result_queue.put(fn())
    except Exception as e:
      error_queue.put(e)
      raise

  return _wrapped


def _run_in_subprocess(test_case):
  error_queue = multiprocessing.Queue()
  result_queue = multiprocessing.Queue()
  wrapped_case = _wrap_in_subprocess(result_queue, error_queue, test_case)
  proc = multiprocessing.Process(target=wrapped_case)
  proc.start()
  proc.join()
  if not error_queue.empty():
    raise error_queue.get()
  assert proc.exitcode == 0, 'Process exited with code {}'.format(proc.exitcode)
  if result_queue.empty():
    raise RuntimeError('Wrapped function returned no result.')
  return result_queue.get()


def _is_public_symbol(symbol):
  return (not symbol.startswith('_') and not symbol.endswith('__pb2') and
          symbol not in _UNINTENTIONAL_PUBLIC_SYMBOLS)


def _get_public_symbols(module):
  return [symbol for symbol in dir(module) if _is_public_symbol(symbol)]


def _get_static_module_record():
  from google.protobuf import test_messages_proto3_pb2  # pylint: disable=g-import-not-at-top
  return ProtoFile.from_file_descriptor(test_messages_proto3_pb2.DESCRIPTOR)


def _get_static_module_symbols():
  from google.protobuf import test_messages_proto3_pb2  # pylint: disable=g-import-not-at-top
  return _get_public_symbols(test_messages_proto3_pb2)


def _get_dynamic_module_record():
  protos = protobuf.protos(_TEST_PROTO_FILE, include_paths=['../src/'])
  return ProtoFile.from_file_descriptor(protos.DESCRIPTOR)


def _get_dynamic_module_symbols():
  protos = protobuf.protos(_TEST_PROTO_FILE, include_paths=['../src/'])
  return _get_public_symbols(protos)


def _test_proto_module_imported_once():
  protos = protobuf.protos('google/protobuf/internal/simple_test.proto')
  complicated_protos = protobuf.protos(
      'google/protobuf/internal/complicated_test.proto')
  simple_message = protos.SimpleMessage()
  complicated_message = complicated_protos.ComplicatedMessage()
  assert (simple_message.simpler_message.simplest_message.__class__ is
          complicated_message.simplest_message.__class__)


def _test_static_dynamic_combo():
  from google.protobuf.internal import complicated_test_pb2  # pylint: disable=g-import-not-at-top
  protos = protobuf.protos('google/protobuf/internal/simple_test.proto')
  static_message = complicated_test_pb2.ComplicatedMessage()
  dynamic_message = protos.SimpleMessage()
  assert (dynamic_message.simpler_message.simplest_message.__class__ is
          static_message.simplest_message.__class__)


def _test_source_code_info():
  from google.protobuf import descriptor_pb2  # pylint: disable=g-import-not-at-top
  protos = protobuf.protos('google/protobuf/internal/simple_test.proto')
  file_descriptor_proto = descriptor_pb2.FileDescriptorProto()
  protos.DESCRIPTOR.CopyToProto(file_descriptor_proto)
  protos.DESCRIPTOR.CopySourceCodeInfoToProto(file_descriptor_proto)
  assert file_descriptor_proto.HasField('source_code_info')
  assert file_descriptor_proto.source_code_info.location


@unittest.skipIf(sys.version_info.major < 3, 'Not supported on Python 2.')
@unittest.skipIf(api_implementation.Type() != 'cpp',
                 'Not supported on pure Python implementation.')
class RuntimeImportTest(unittest.TestCase):

  def test_file_descriptor_contents_identical(self):
    static_record = _run_in_subprocess(_get_static_module_record)
    dynamic_record = _run_in_subprocess(_get_dynamic_module_record)
    self.assertEqual(static_record, dynamic_record)

  def test_module_contents_identical(self):
    static_symbols = _run_in_subprocess(_get_static_module_symbols)
    dynamic_symbols = _run_in_subprocess(_get_dynamic_module_symbols)
    self.assertSequenceEqual(set(static_symbols), set(dynamic_symbols))

  def test_syntax_error(self):
    with self.assertRaises(SyntaxError) as cm:
      unused_protos = protobuf.protos(_BROKEN_PROTO)
    self.assertIn(_BROKEN_PROTO, str(cm.exception))
    # Line number of first error.
    self.assertIn('35', str(cm.exception))
    # Line number of second error.
    self.assertIn('39', str(cm.exception))

  def test_proto_module_imported_once(self):
    _run_in_subprocess(_test_proto_module_imported_once)

  def test_static_dynamic_combo(self):
    _run_in_subprocess(_test_static_dynamic_combo)

  def test_source_code_info(self):
    _run_in_subprocess(_test_source_code_info)


@unittest.skipIf(sys.version_info.major != 2, 'Not supported on Python 2.')
@unittest.skipIf(api_implementation.Type() == 'cpp',
                 'Not supported on pure Python implementation.')
class RuntimeImportGracefulFailureTest(unittest.TestCase):

  def test_graceful_failure(self):
    with self.assertRaises(NotImplementedError):
      unused_protos = protobuf.protos(_TEST_PROTO_FILE)


if __name__ == '__main__':
  unittest.main()
