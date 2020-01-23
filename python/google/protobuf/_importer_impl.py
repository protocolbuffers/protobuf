# Lint as: python3

# Protocol Buffers - Google's data interchange format
# Copyright 2020 Google Inc.  All rights reserved.
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
"""Enables instantiation of Messages directly from a ".proto" file."""

__author__ = 'rbellevi@google.com (Richard Belleville)'


import contextlib
import importlib
import importlib.machinery
import os
import sys

from google.protobuf.pyext import _message as _pyext_message
from google.protobuf import message as _message
from google.protobuf.internal import enum_type_wrapper
from google.protobuf import reflection as _reflection
from google.protobuf import symbol_database as _symbol_database

_sym_db = _symbol_database.Default()

_PROTO_MODULE_SUFFIX = '_pb2'

def _module_name_to_proto_file(module_name):
  components = module_name.split('.')
  proto_name = components[-1][:-1 * len(_PROTO_MODULE_SUFFIX)]
  return os.path.sep.join(components[:-1] + [proto_name + '.proto'])

def _proto_file_to_module_name(proto_file):
  components = proto_file.split(os.path.sep)
  proto_base_name = os.path.splitext(components[-1])[0]
  return '.'.join(components[:-1] +
                  [proto_base_name + _PROTO_MODULE_SUFFIX])

@contextlib.contextmanager
def _augmented_syspath(new_paths):
  original_sys_path = sys.path
  if new_paths is not None:
    sys.path = sys.path + new_paths
  try:
    yield
  finally:
    sys.path = original_sys_path

class ProtoLoader(importlib.abc.Loader):
  """Instantiates a module equivalent to a _pb2.py file from a .proto."""

  def __init__(self, module_name, protobuf_path):
    self._module_name = module_name
    self._protobuf_path = protobuf_path

  def create_module(self, spec):
    return None

  def _generated_file_to_module_name(self, filepath):
    components = filepath.split(os.path.sep)
    return '.'.join(components[:-1] + [os.path.splitext(components[-1])[0]])

  @staticmethod
  def _register_message(sym_db, message_descriptor, module_name):
    """Registers a message in the symbol database."""
    nested_dict = {
        '__module__': module_name,
        'DESCRIPTOR': message_descriptor,
    }
    for nested_type_desc in message_descriptor.nested_types:
      nested_type = ProtoLoader._register_message(sym_db, nested_type_desc,
                                                  module_name)
      nested_dict[nested_type_desc.name] = nested_type
    for nested_enum_desc in message_descriptor.enum_types:
      sym_db.RegisterEnumDescriptor(nested_enum_desc)
    message_type = (
        _reflection.GeneratedProtocolMessageType(message_descriptor.name,
                                                 (_message.Message,),
                                                 nested_dict))
    sym_db.RegisterMessageDescriptor(message_descriptor)
    sym_db.RegisterMessage(message_type)
    return message_type

  def exec_module(self, module):
    """Instantiate a module identical to the generated version."""
    file_descriptor = _pyext_message.FileDescriptor.FromFile(
        self._protobuf_path.encode('ascii'),
        [path.encode('ascii') for path in sys.path])
    _sym_db.RegisterFileDescriptor(file_descriptor)
    for dependency in file_descriptor.dependencies:
      module_name = _proto_file_to_module_name(dependency.name)
      if module_name not in sys.modules:
        importlib.import_module(module_name)
    setattr(module, 'DESCRIPTOR', file_descriptor)
    for enum_name, enum_descriptor in file_descriptor.enum_types_by_name.iteritems(
    ):
      _sym_db.RegisterEnumDescriptor(enum_descriptor)
      setattr(module, enum_name,
              enum_type_wrapper.EnumTypeWrapper(enum_descriptor))
      for enum_value in enum_descriptor.values:
        setattr(module, enum_value.name, enum_value.number)
    for name, message_descriptor in file_descriptor.message_types_by_name.iteritems(
    ):
      message_type = ProtoLoader._register_message(_sym_db,
                                                   message_descriptor,
                                                   module.__name__)
      _sym_db.RegisterMessageDescriptor(message_descriptor)
      _sym_db.RegisterMessage(message_type)
      setattr(module, name, message_type)

class ProtoFinder(importlib.abc.MetaPathFinder):
  """MetaPathFinder that locates .proto files."""

  def find_spec(self, fullname, path, target=None):
    """Determines whether a .proto file can be loaded."""
    del path
    del target
    filepath = _module_name_to_proto_file(fullname)
    for search_path in sys.path:
      try:
        prospective_path = os.path.join(search_path, filepath)
        os.stat(prospective_path)
      except (FileNotFoundError, NotADirectoryError):
        continue
      else:
        return importlib.machinery.ModuleSpec(
            fullname, ProtoLoader(fullname, filepath))

def protos(protobuf_path, include_paths=None):  # pylint: disable=g-function-redefined, function-redefined
  """Loads a module from a .proto file on disk.

    This function is idempotent. Invoking it a second time will simply
    return a module that has already been loaded. If the desired proto has
    already been loaded from a "_pb2.py" file, this function will simply
    return that module.

    This function guarantees pointer equality for all types in the
    dependency tree of an imported type. For example, suppose we have three
    .proto files: A.proto, B.proto, and C.proto with the following contents:

    A.proto:
    ```
    import "C.proto"

    message AMessage {
        CMessage c = 1;
    }
    ```

    B.proto:
    ```
    import "C.proto"

    message BMessage {
        CMessage c = 1;
    }
    ```

    C.proto:
    ```
    message CMessage {
        int64 foo = 1;
    }
    ```

    Then we guarantee that the following assertion will hold regardless of
    the combination of generated code and runtime import used to load the
    three modules:

    ```
    assert AMessage().c.__class__ is BMessage().c.__class__
    ```

  Args:
    protobuf_path: A string representing the file path to the .proto file
      on disk. (e.g. "google/protobuf/any.proto"). By default, all the
      paths on sys.paths are searched for this file.
    include_paths: An optional sequence of strings representing paths on
      which to search for the .proto file to search in addition to the
      entries in sys.paths.

  Returns:
    A module object identical to one that would be generated by protoc
    and loaded with an `import foo_pb2` statement.
  """
  with _augmented_syspath(include_paths):
    module_name = _proto_file_to_module_name(protobuf_path)
    module = importlib.import_module(module_name)
    return module

sys.meta_path.extend([ProtoFinder()])
