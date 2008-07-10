# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.
# http://code.google.com/p/protobuf/
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# TODO(robinson): We probably need to provide deep-copy methods for
# descriptor types.  When a FieldDescriptor is passed into
# Descriptor.__init__(), we should make a deep copy and then set
# containing_type on it.  Alternatively, we could just get
# rid of containing_type (iit's not needed for reflection.py, at least).
#
# TODO(robinson): Print method?
#
# TODO(robinson): Useful __repr__?

"""Descriptors essentially contain exactly the information found in a .proto
file, in types that make this information accessible in Python.
"""

__author__ = 'robinson@google.com (Will Robinson)'

class DescriptorBase(object):

  """Descriptors base class.

  This class is the base of all descriptor classes. It provides common options
  related functionaility.
  """

  def __init__(self, options, options_class_name):
    """Initialize the descriptor given its options message and the name of the
    class of the options message. The name of the class is required in case
    the options message is None and has to be created.
    """
    self._options = options
    self._options_class_name = options_class_name

  def GetOptions(self):
    """Retrieves descriptor options.

    This method returns the options set or creates the default options for the
    descriptor.
    """
    if self._options:
      return self._options
    from google.protobuf import descriptor_pb2
    try:
      options_class = getattr(descriptor_pb2, self._options_class_name)
    except AttributeError:
      raise RuntimeError('Unknown options class name %s!' %
                         (self._options_class_name))
    self._options = options_class()
    return self._options


class Descriptor(DescriptorBase):

  """Descriptor for a protocol message type.

  A Descriptor instance has the following attributes:

    name: (str) Name of this protocol message type.
    full_name: (str) Fully-qualified name of this protocol message type,
      which will include protocol "package" name and the name of any
      enclosing types.

    filename: (str) Name of the .proto file containing this message.

    containing_type: (Descriptor) Reference to the descriptor of the
      type containing us, or None if we have no containing type.

    fields: (list of FieldDescriptors) Field descriptors for all
      fields in this type.
    fields_by_number: (dict int -> FieldDescriptor) Same FieldDescriptor
      objects as in |fields|, but indexed by "number" attribute in each
      FieldDescriptor.
    fields_by_name: (dict str -> FieldDescriptor) Same FieldDescriptor
      objects as in |fields|, but indexed by "name" attribute in each
      FieldDescriptor.

    nested_types: (list of Descriptors) Descriptor references
      for all protocol message types nested within this one.
    nested_types_by_name: (dict str -> Descriptor) Same Descriptor
      objects as in |nested_types|, but indexed by "name" attribute
      in each Descriptor.

    enum_types: (list of EnumDescriptors) EnumDescriptor references
      for all enums contained within this type.
    enum_types_by_name: (dict str ->EnumDescriptor) Same EnumDescriptor
      objects as in |enum_types|, but indexed by "name" attribute
      in each EnumDescriptor.
    enum_values_by_name: (dict str -> EnumValueDescriptor) Dict mapping
      from enum value name to EnumValueDescriptor for that value.

    extensions: (list of FieldDescriptor) All extensions defined directly
      within this message type (NOT within a nested type).
    extensions_by_name: (dict, string -> FieldDescriptor) Same FieldDescriptor
      objects as |extensions|, but indexed by "name" attribute of each
      FieldDescriptor.

    options: (descriptor_pb2.MessageOptions) Protocol message options or None
      to use default message options.
  """

  def __init__(self, name, full_name, filename, containing_type,
               fields, nested_types, enum_types, extensions, options=None):
    """Arguments to __init__() are as described in the description
    of Descriptor fields above.
    """
    super(Descriptor, self).__init__(options, 'MessageOptions')
    self.name = name
    self.full_name = full_name
    self.filename = filename
    self.containing_type = containing_type

    # We have fields in addition to fields_by_name and fields_by_number,
    # so that:
    #   1. Clients can index fields by "order in which they're listed."
    #   2. Clients can easily iterate over all fields with the terse
    #      syntax: for f in descriptor.fields: ...
    self.fields = fields
    for field in self.fields:
      field.containing_type = self
    self.fields_by_number = dict((f.number, f) for f in fields)
    self.fields_by_name = dict((f.name, f) for f in fields)

    self.nested_types = nested_types
    self.nested_types_by_name = dict((t.name, t) for t in nested_types)

    self.enum_types = enum_types
    for enum_type in self.enum_types:
      enum_type.containing_type = self
    self.enum_types_by_name = dict((t.name, t) for t in enum_types)
    self.enum_values_by_name = dict(
        (v.name, v) for t in enum_types for v in t.values)

    self.extensions = extensions
    for extension in self.extensions:
      extension.extension_scope = self
    self.extensions_by_name = dict((f.name, f) for f in extensions)


# TODO(robinson): We should have aggressive checking here,
# for example:
#   * If you specify a repeated field, you should not be allowed
#     to specify a default value.
#   * [Other examples here as needed].
#
# TODO(robinson): for this and other *Descriptor classes, we
# might also want to lock things down aggressively (e.g.,
# prevent clients from setting the attributes).  Having
# stronger invariants here in general will reduce the number
# of runtime checks we must do in reflection.py...
class FieldDescriptor(DescriptorBase):

  """Descriptor for a single field in a .proto file.

  A FieldDescriptor instance has the following attriubtes:

    name: (str) Name of this field, exactly as it appears in .proto.
    full_name: (str) Name of this field, including containing scope.  This is
      particularly relevant for extensions.
    index: (int) Dense, 0-indexed index giving the order that this
      field textually appears within its message in the .proto file.
    number: (int) Tag number declared for this field in the .proto file.

    type: (One of the TYPE_* constants below) Declared type.
    cpp_type: (One of the CPPTYPE_* constants below) C++ type used to
      represent this field.

    label: (One of the LABEL_* constants below) Tells whether this
      field is optional, required, or repeated.
    default_value: (Varies) Default value of this field.  Only
      meaningful for non-repeated scalar fields.  Repeated fields
      should always set this to [], and non-repeated composite
      fields should always set this to None.

    containing_type: (Descriptor) Descriptor of the protocol message
      type that contains this field.  Set by the Descriptor constructor
      if we're passed into one.
      Somewhat confusingly, for extension fields, this is the
      descriptor of the EXTENDED message, not the descriptor
      of the message containing this field.  (See is_extension and
      extension_scope below).
    message_type: (Descriptor) If a composite field, a descriptor
      of the message type contained in this field.  Otherwise, this is None.
    enum_type: (EnumDescriptor) If this field contains an enum, a
      descriptor of that enum.  Otherwise, this is None.

    is_extension: True iff this describes an extension field.
    extension_scope: (Descriptor) Only meaningful if is_extension is True.
      Gives the message that immediately contains this extension field.
      Will be None iff we're a top-level (file-level) extension field.

    options: (descriptor_pb2.FieldOptions) Protocol message field options or
      None to use default field options.
  """

  # Must be consistent with C++ FieldDescriptor::Type enum in
  # descriptor.h.
  #
  # TODO(robinson): Find a way to eliminate this repetition.
  TYPE_DOUBLE         = 1
  TYPE_FLOAT          = 2
  TYPE_INT64          = 3
  TYPE_UINT64         = 4
  TYPE_INT32          = 5
  TYPE_FIXED64        = 6
  TYPE_FIXED32        = 7
  TYPE_BOOL           = 8
  TYPE_STRING         = 9
  TYPE_GROUP          = 10
  TYPE_MESSAGE        = 11
  TYPE_BYTES          = 12
  TYPE_UINT32         = 13
  TYPE_ENUM           = 14
  TYPE_SFIXED32       = 15
  TYPE_SFIXED64       = 16
  TYPE_SINT32         = 17
  TYPE_SINT64         = 18
  MAX_TYPE            = 18

  # Must be consistent with C++ FieldDescriptor::CppType enum in
  # descriptor.h.
  #
  # TODO(robinson): Find a way to eliminate this repetition.
  CPPTYPE_INT32       = 1
  CPPTYPE_INT64       = 2
  CPPTYPE_UINT32      = 3
  CPPTYPE_UINT64      = 4
  CPPTYPE_DOUBLE      = 5
  CPPTYPE_FLOAT       = 6
  CPPTYPE_BOOL        = 7
  CPPTYPE_ENUM        = 8
  CPPTYPE_STRING      = 9
  CPPTYPE_MESSAGE     = 10
  MAX_CPPTYPE         = 10

  # Must be consistent with C++ FieldDescriptor::Label enum in
  # descriptor.h.
  #
  # TODO(robinson): Find a way to eliminate this repetition.
  LABEL_OPTIONAL      = 1
  LABEL_REQUIRED      = 2
  LABEL_REPEATED      = 3
  MAX_LABEL           = 3

  def __init__(self, name, full_name, index, number, type, cpp_type, label,
               default_value, message_type, enum_type, containing_type,
               is_extension, extension_scope, options=None):
    """The arguments are as described in the description of FieldDescriptor
    attributes above.

    Note that containing_type may be None, and may be set later if necessary
    (to deal with circular references between message types, for example).
    Likewise for extension_scope.
    """
    super(FieldDescriptor, self).__init__(options, 'FieldOptions')
    self.name = name
    self.full_name = full_name
    self.index = index
    self.number = number
    self.type = type
    self.cpp_type = cpp_type
    self.label = label
    self.default_value = default_value
    self.containing_type = containing_type
    self.message_type = message_type
    self.enum_type = enum_type
    self.is_extension = is_extension
    self.extension_scope = extension_scope


class EnumDescriptor(DescriptorBase):

  """Descriptor for an enum defined in a .proto file.

  An EnumDescriptor instance has the following attributes:

    name: (str) Name of the enum type.
    full_name: (str) Full name of the type, including package name
      and any enclosing type(s).
    filename: (str) Name of the .proto file in which this appears.

    values: (list of EnumValueDescriptors) List of the values
      in this enum.
    values_by_name: (dict str -> EnumValueDescriptor) Same as |values|,
      but indexed by the "name" field of each EnumValueDescriptor.
    values_by_number: (dict int -> EnumValueDescriptor) Same as |values|,
      but indexed by the "number" field of each EnumValueDescriptor.
    containing_type: (Descriptor) Descriptor of the immediate containing
      type of this enum, or None if this is an enum defined at the
      top level in a .proto file.  Set by Descriptor's constructor
      if we're passed into one.
    options: (descriptor_pb2.EnumOptions) Enum options message or
      None to use default enum options.
  """

  def __init__(self, name, full_name, filename, values,
               containing_type=None, options=None):
    """Arguments are as described in the attribute description above."""
    super(EnumDescriptor, self).__init__(options, 'EnumOptions')
    self.name = name
    self.full_name = full_name
    self.filename = filename
    self.values = values
    for value in self.values:
      value.type = self
    self.values_by_name = dict((v.name, v) for v in values)
    self.values_by_number = dict((v.number, v) for v in values)
    self.containing_type = containing_type


class EnumValueDescriptor(DescriptorBase):

  """Descriptor for a single value within an enum.

    name: (str) Name of this value.
    index: (int) Dense, 0-indexed index giving the order that this
      value appears textually within its enum in the .proto file.
    number: (int) Actual number assigned to this enum value.
    type: (EnumDescriptor) EnumDescriptor to which this value
      belongs.  Set by EnumDescriptor's constructor if we're
      passed into one.
    options: (descriptor_pb2.EnumValueOptions) Enum value options message or
      None to use default enum value options options.
  """

  def __init__(self, name, index, number, type=None, options=None):
    """Arguments are as described in the attribute description above."""
    super(EnumValueDescriptor, self).__init__(options, 'EnumValueOptions')
    self.name = name
    self.index = index
    self.number = number
    self.type = type


class ServiceDescriptor(DescriptorBase):

  """Descriptor for a service.

    name: (str) Name of the service.
    full_name: (str) Full name of the service, including package name.
    index: (int) 0-indexed index giving the order that this services
      definition appears withing the .proto file.
    methods: (list of MethodDescriptor) List of methods provided by this
      service.
    options: (descriptor_pb2.ServiceOptions) Service options message or
      None to use default service options.
  """

  def __init__(self, name, full_name, index, methods, options=None):
    super(ServiceDescriptor, self).__init__(options, 'ServiceOptions')
    self.name = name
    self.full_name = full_name
    self.index = index
    self.methods = methods
    # Set the containing service for each method in this service.
    for method in self.methods:
      method.containing_service = self

  def FindMethodByName(self, name):
    """Searches for the specified method, and returns its descriptor."""
    for method in self.methods:
      if name == method.name:
        return method
    return None


class MethodDescriptor(DescriptorBase):

  """Descriptor for a method in a service.

  name: (str) Name of the method within the service.
  full_name: (str) Full name of method.
  index: (int) 0-indexed index of the method inside the service.
  containing_service: (ServiceDescriptor) The service that contains this
    method.
  input_type: The descriptor of the message that this method accepts.
  output_type: The descriptor of the message that this method returns.
  options: (descriptor_pb2.MethodOptions) Method options message or
    None to use default method options.
  """

  def __init__(self, name, full_name, index, containing_service,
               input_type, output_type, options=None):
    """The arguments are as described in the description of MethodDescriptor
    attributes above.

    Note that containing_service may be None, and may be set later if necessary.
    """
    super(MethodDescriptor, self).__init__(options, 'MethodOptions')
    self.name = name
    self.full_name = full_name
    self.index = index
    self.containing_service = containing_service
    self.input_type = input_type
    self.output_type = output_type


def _ParseOptions(message, string):
  """Parses serialized options.

  This helper function is used to parse serialized options in generated
  proto2 files. It must not be used outside proto2.
  """
  message.ParseFromString(string)
  return message;
