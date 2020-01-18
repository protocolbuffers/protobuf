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

"""Contains functions enabling instantiation of Messages directly from a
".proto" file.
"""

__author__ = 'rbellevi@google.com (Richard Belleville)'

import sys

def protos(*args, **kwargs):
  raise NotImplementedError("The protos function is only available when using the cpp implementation"
                            "on a 3.X interpreter.")

from google.protobuf.internal import api_implementation as _api_implementation

if sys.version_info[0] > 2 and _api_implementation.Type() == 'cpp':
  try:
    from google.protobuf.pyext import _message as _pyext_message
  except ImportError:
    pass
  else:
    from google.protobuf import message as _message
    import contextlib
    import importlib
    import importlib.machinery
    import threading
    import os

    from google.protobuf.internal import enum_type_wrapper
    from google.protobuf import reflection as _reflection

    _PROTO_MODULE_SUFFIX = "_pb2"

    def _module_name_to_proto_file(suffix, module_name):
        components = module_name.split(".")
        proto_name = components[-1][:-1 * len(suffix)]
        return os.path.sep.join(components[:-1] + [proto_name + ".proto"])

    def _proto_file_to_module_name(suffix, proto_file):
        components = proto_file.split(os.path.sep)
        proto_base_name = os.path.splitext(components[-1])[0]
        return ".".join(components[:-1] + [proto_base_name + suffix])

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

        def __init__(self, suffix, module_name, protobuf_path):
            self._suffix = suffix
            self._module_name = module_name
            self._protobuf_path = protobuf_path

        def create_module(self, spec):
            return None

        def _generated_file_to_module_name(self, filepath):
            components = filepath.split(os.path.sep)
            return ".".join(
                components[:-1] + [os.path.splitext(components[-1])[0]])

        @staticmethod
        def _register_message(message_descriptor, module_name):
            nested_dict = {
                '__module__': module_name,
                'DESCRIPTOR': message_descriptor,
            }
            for nested_type_desc in message_descriptor.nested_types:
                nested_type = ProtoLoader._register_message(nested_type_desc, module_name)
                nested_dict[nested_type_desc.name] = nested_type
            message_type = (
                    _reflection.GeneratedProtocolMessageType(
                            message_descriptor.name, (_message.Message,), nested_dict))
            return message_type

        def exec_module(self, module):
            """Instantiate a module identical to the generated version.

            This method does not register symbols in the symbol database, as
            this feature is not available for the Python implementation.
            """
            file_descriptor = _pyext_message.FileDescriptor.FromFile(self._protobuf_path.encode('ascii'),
                                                               [path.encode('ascii') for path in sys.path])
            for dependency in file_descriptor.dependencies:
                module_name = _proto_file_to_module_name(_PROTO_MODULE_SUFFIX, dependency.name)
                if module_name not in sys.modules:
                    importlib.import_module(module_name)
            setattr(module, 'DESCRIPTOR', file_descriptor)
            for enum_name, enum_descriptor in file_descriptor.enum_types_by_name.iteritems():
                setattr(module, enum_name, enum_type_wrapper.EnumTypeWrapper(enum_descriptor))
                for enum_value in enum_descriptor.values:
                    setattr(module, enum_value.name, enum_value.number)
            for name, message_descriptor in file_descriptor.message_types_by_name.iteritems():
                message_type = ProtoLoader._register_message(message_descriptor, module.__name__)
                setattr(module, name, message_type)

    class ProtoFinder(importlib.abc.MetaPathFinder):

        def __init__(self, suffix):
            self._suffix = suffix

        def find_spec(self, fullname, path, target=None):
            filepath = _module_name_to_proto_file(self._suffix, fullname)
            for search_path in sys.path:
                try:
                    prospective_path = os.path.join(search_path, filepath)
                    os.stat(prospective_path)
                except (FileNotFoundError, NotADirectoryError):
                    continue
                else:
                    # TODO: ProtoLoader singleton?
                    return importlib.machinery.ModuleSpec(
                        fullname,
                        ProtoLoader(self._suffix, fullname, filepath))

    def _import(module_suffix):
      def protos_impl(protobuf_path, include_paths=None):
        with _augmented_syspath(include_paths):
          module_name = _proto_file_to_module_name(module_suffix,
                                                   protobuf_path)
          module = importlib.import_module(module_name)
          return module
      return protos_impl

    # TODO: Take another look at the docstrings.
    def get_import_machinery(module_suffix):
      """Get the objects needed to create message types at runtime.
      Args:
        module_suffix: A Text object appended to the proto's name in order to
          form a module name. For example, for vanilla protocol buffers, this
          will be "_pb2" and for gRPC it will be "_pb2_grpc".
      Returns:
        A tuple with two components:
         1. An instance of importlib.abc.MetaPathFinder capable of importing
           .proto files with module names derived using the module_suffix
           argument. (e.g. capable of importing "foo.proto" as foo_pb2). It is
           expected that this object be addeed to sys.meta_path.
         2. A function with the same signature as google.protobuf.protos      """
      return ProtoFinder(module_suffix), _import(module_suffix)

    # TODO: Now that we no longer depend on a code_generator parameter, make
    # this signature easier to grok.
    finder, protos = get_import_machinery(_PROTO_MODULE_SUFFIX)

    sys.meta_path.extend([finder])
