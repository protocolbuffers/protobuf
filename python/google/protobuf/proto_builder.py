# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
"""Dynamic Protobuf class creator."""

import collections
import hashlib
import os

from google.protobuf import descriptor
from google.protobuf import descriptor_pb2
from google.protobuf import descriptor_pool
from google.protobuf import message_factory


def _GetMessageFromFactory(pool, full_name):
  """Get a proto class from the MessageFactory by name.

  Args:
    pool: a descriptor pool.
    full_name: str, the fully qualified name of the proto type.

  Returns:
    A class, for the type identified by full_name.
  Raises:
    KeyError, if the proto is not found in the factory's descriptor pool.
  """
  proto_descriptor = pool.FindMessageTypeByName(full_name)
  proto_cls = message_factory.GetMessageClass(proto_descriptor)
  return proto_cls


def _DynamicProtoName(field_items, full_name):
  """Generate a dynamic proto file name and full name."""
  # Use a consistent file name that is unlikely to conflict with any imported
  # proto files.
  fields_hash = hashlib.sha1()
  for f_name, f_type in field_items:
    fields_hash.update(f_name.encode('utf-8'))
    fields_hash.update(str(f_type).encode('utf-8'))
  proto_file_name = fields_hash.hexdigest() + '.proto'

  # If the proto is anonymous, use the same hash to name it.
  if full_name is None:
    full_name = (
        'net.proto2.python.public.proto_builder.AnonymousProto_'
        + fields_hash.hexdigest()
    )
  return proto_file_name, full_name


def MakeSimpleProtoClass(fields, full_name=None, pool=None):
  """Create a Protobuf class whose fields are basic types.

  Note: this doesn't validate field names!

  Args:
    fields: dict of {name: field_type} mappings for each field in the proto. If
      this is an OrderedDict the order will be maintained, otherwise the fields
      will be sorted by name.
    full_name: optional str, the fully-qualified name of the proto type.
    pool: optional DescriptorPool instance.

  Returns:
    a class, the new protobuf class with a FileDescriptor.
  """
  pool_instance = pool or descriptor_pool.DescriptorPool()
  if full_name is not None:
    try:
      proto_cls = _GetMessageFromFactory(pool_instance, full_name)
      return proto_cls
    except KeyError:
      # The factory's DescriptorPool doesn't know about this class yet.
      pass

  # Get a list of (name, field_type) tuples from the fields dict. If fields was
  # an OrderedDict we keep the order, but otherwise we sort the field to ensure
  # consistent ordering.
  field_items = fields.items()
  if not isinstance(fields, collections.OrderedDict):
    field_items = sorted(field_items)

  # Use a consistent file name that is unlikely to conflict with any imported
  # proto files.
  fields_hash = hashlib.sha1(usedforsecurity=False)
  for f_name, f_type in field_items:
    fields_hash.update(f_name.encode('utf-8'))
    fields_hash.update(str(f_type).encode('utf-8'))
  proto_file_name = fields_hash.hexdigest() + '.proto'

  # If the proto is anonymous, use the same hash to name it.
  if full_name is None:
    full_name = (
        'net.proto2.python.public.proto_builder.AnonymousProto_'
        + fields_hash.hexdigest()
    )
    try:
      proto_cls = _GetMessageFromFactory(pool_instance, full_name)
      return proto_cls
    except KeyError:
      # The factory's DescriptorPool doesn't know about this class yet.
      pass

  # This is the first time we see this proto: add a new descriptor to the pool.
  pool_instance.Add(
      _MakeFileDescriptorProto(proto_file_name, full_name, field_items)
  )
  return _GetMessageFromFactory(pool_instance, full_name)


def _MakeFileDescriptorProto(proto_file_name, full_name, field_items):
  """Populate FileDescriptorProto for MessageFactory's DescriptorPool."""
  package, name = full_name.rsplit('.', 1)
  file_proto = descriptor_pb2.FileDescriptorProto()
  file_proto.name = os.path.join(package.replace('.', '/'), proto_file_name)
  file_proto.package = package
  desc_proto = file_proto.message_type.add()
  desc_proto.name = name
  for f_number, (f_name, f_type) in enumerate(field_items, 1):
    field_proto = desc_proto.field.add()
    field_proto.name = f_name
    # # If the number falls in the reserved range, reassign it to the correct
    # # number after the range.
    if f_number >= descriptor.FieldDescriptor.FIRST_RESERVED_FIELD_NUMBER:
      f_number += (
          descriptor.FieldDescriptor.LAST_RESERVED_FIELD_NUMBER
          - descriptor.FieldDescriptor.FIRST_RESERVED_FIELD_NUMBER
          + 1
      )
    field_proto.number = f_number
    is_repeated = isinstance(f_type, list)
    if is_repeated:
      field_proto.label = descriptor_pb2.FieldDescriptorProto.LABEL_REPEATED
      actual_f_type = next((t for t in f_type if t is not None), None)
    else:
      field_proto.label = descriptor_pb2.FieldDescriptorProto.LABEL_OPTIONAL
      actual_f_type = f_type

    if hasattr(actual_f_type, 'DESCRIPTOR') and isinstance(
        actual_f_type.DESCRIPTOR, descriptor.Descriptor
    ):
      field_proto.type = descriptor_pb2.FieldDescriptorProto.TYPE_MESSAGE
      field_proto.type_name = '.' + actual_f_type.DESCRIPTOR.full_name
      dep_name = actual_f_type.DESCRIPTOR.file.name
      if dep_name not in file_proto.dependency:
        file_proto.dependency.append(dep_name)
    else:
      field_proto.type = actual_f_type
  return file_proto


def _AddDependentFileDescriptorProtos(pool_instance, file_descriptor):
  """Adds dependent file descriptors to the pool."""
  try:
    pool_instance.FindFileByName(file_descriptor.name)
    return
  except KeyError:
    pass

  for dependency in file_descriptor.dependencies:
    _AddDependentFileDescriptorProtos(pool_instance, dependency)

  file_descriptor_proto = descriptor_pb2.FileDescriptorProto()
  file_descriptor.CopyToProto(file_descriptor_proto)
  pool_instance.Add(file_descriptor_proto)


def _DynamicProtoEq(self, other):
  if not hasattr(other, 'SerializePartialToString'):
    return False
  return self.SerializePartialToString() == other.SerializePartialToString()


def MakeProtoClassWithProtoFields(fields, full_name=None, pool=None):
  """Create a Protobuf class whose fields can be proto types."""
  pool_instance = pool or descriptor_pool.DescriptorPool()

  # Get a list of (name, field_type) tuples from the fields dict.
  field_items = fields.items()
  if not isinstance(fields, collections.OrderedDict):
    field_items = sorted(field_items)

  proto_file_name, full_name = _DynamicProtoName(field_items, full_name)

  try:
    return _GetMessageFromFactory(pool_instance, full_name)
  except KeyError:
    # The factory's DescriptorPool doesn't know about this class yet.
    pass

  for _, f_type in field_items:
    actual_f_type = (
        next((t for t in f_type if t is not None), None)
        if isinstance(f_type, list)
        else f_type
    )
    if hasattr(actual_f_type, 'DESCRIPTOR') and isinstance(
        actual_f_type.DESCRIPTOR, descriptor.Descriptor
    ):
      _AddDependentFileDescriptorProtos(
          pool_instance, actual_f_type.DESCRIPTOR.file
      )

  # This is the first time we see this proto: add a new descriptor to the pool.
  pool_instance.Add(
      _MakeFileDescriptorProto(proto_file_name, full_name, field_items)
  )
  proto_cls = _GetMessageFromFactory(pool_instance, full_name)
  # try:
  #   proto_cls.SerializeToString = proto_cls.SerializePartialToString
  #   proto_cls.__eq__ = _DynamicProtoEq
  # except (AttributeError, TypeError):
  #   pass
  return proto_cls
