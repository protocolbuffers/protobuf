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

"""Contains helper functions used to create protocol message classes from
Descriptor objects at runtime backed by the protocol buffer C++ API.
"""

__author__ = 'petar@google.com (Petar Petrov)'

import operator
from google.protobuf.internal import _net_proto2___python
from google.protobuf import message


_LABEL_REPEATED = _net_proto2___python.LABEL_REPEATED
_LABEL_OPTIONAL = _net_proto2___python.LABEL_OPTIONAL
_CPPTYPE_MESSAGE = _net_proto2___python.CPPTYPE_MESSAGE
_TYPE_MESSAGE = _net_proto2___python.TYPE_MESSAGE


def GetDescriptorPool():
  """Creates a new DescriptorPool C++ object."""
  return _net_proto2___python.NewCDescriptorPool()


_pool = GetDescriptorPool()


def GetFieldDescriptor(full_field_name):
  """Searches for a field descriptor given a full field name."""
  return _pool.FindFieldByName(full_field_name)


def BuildFile(content):
  """Registers a new proto file in the underlying C++ descriptor pool."""
  _net_proto2___python.BuildFile(content)


def GetExtensionDescriptor(full_extension_name):
  """Searches for extension descriptor given a full field name."""
  return _pool.FindExtensionByName(full_extension_name)


def NewCMessage(full_message_name):
  """Creates a new C++ protocol message by its name."""
  return _net_proto2___python.NewCMessage(full_message_name)


def ScalarProperty(cdescriptor):
  """Returns a scalar property for the given descriptor."""

  def Getter(self):
    return self._cmsg.GetScalar(cdescriptor)

  def Setter(self, value):
    self._cmsg.SetScalar(cdescriptor, value)

  return property(Getter, Setter)


def CompositeProperty(cdescriptor, message_type):
  """Returns a Python property the given composite field."""

  def Getter(self):
    sub_message = self._composite_fields.get(cdescriptor.name, None)
    if sub_message is None:
      cmessage = self._cmsg.NewSubMessage(cdescriptor)
      sub_message = message_type._concrete_class(__cmessage=cmessage)
      self._composite_fields[cdescriptor.name] = sub_message
    return sub_message

  return property(Getter)


class RepeatedScalarContainer(object):
  """Container for repeated scalar fields."""

  __slots__ = ['_message', '_cfield_descriptor', '_cmsg']

  def __init__(self, msg, cfield_descriptor):
    self._message = msg
    self._cmsg = msg._cmsg
    self._cfield_descriptor = cfield_descriptor

  def append(self, value):
    self._cmsg.AddRepeatedScalar(
        self._cfield_descriptor, value)

  def extend(self, sequence):
    for element in sequence:
      self.append(element)

  def insert(self, key, value):
    values = self[slice(None, None, None)]
    values.insert(key, value)
    self._cmsg.AssignRepeatedScalar(self._cfield_descriptor, values)

  def remove(self, value):
    values = self[slice(None, None, None)]
    values.remove(value)
    self._cmsg.AssignRepeatedScalar(self._cfield_descriptor, values)

  def __setitem__(self, key, value):
    values = self[slice(None, None, None)]
    values[key] = value
    self._cmsg.AssignRepeatedScalar(self._cfield_descriptor, values)

  def __getitem__(self, key):
    return self._cmsg.GetRepeatedScalar(self._cfield_descriptor, key)

  def __delitem__(self, key):
    self._cmsg.DeleteRepeatedField(self._cfield_descriptor, key)

  def __len__(self):
    return len(self[slice(None, None, None)])

  def __eq__(self, other):
    if self is other:
      return True
    if not operator.isSequenceType(other):
      raise TypeError(
          'Can only compare repeated scalar fields against sequences.')
    # We are presumably comparing against some other sequence type.
    return other == self[slice(None, None, None)]

  def __ne__(self, other):
    return not self == other

  def __hash__(self):
    raise TypeError('unhashable object')

  def sort(self, sort_function=cmp):
    values = self[slice(None, None, None)]
    values.sort(sort_function)
    self._cmsg.AssignRepeatedScalar(self._cfield_descriptor, values)


def RepeatedScalarProperty(cdescriptor):
  """Returns a Python property the given repeated scalar field."""

  def Getter(self):
    container = self._composite_fields.get(cdescriptor.name, None)
    if container is None:
      container = RepeatedScalarContainer(self, cdescriptor)
      self._composite_fields[cdescriptor.name] = container
    return container

  def Setter(self, new_value):
    raise AttributeError('Assignment not allowed to repeated field '
                         '"%s" in protocol message object.' % cdescriptor.name)

  doc = 'Magic attribute generated for "%s" proto field.' % cdescriptor.name
  return property(Getter, Setter, doc=doc)


class RepeatedCompositeContainer(object):
  """Container for repeated composite fields."""

  __slots__ = ['_message', '_subclass', '_cfield_descriptor', '_cmsg']

  def __init__(self, msg, cfield_descriptor, subclass):
    self._message = msg
    self._cmsg = msg._cmsg
    self._subclass = subclass
    self._cfield_descriptor = cfield_descriptor

  def add(self, **kwargs):
    cmessage = self._cmsg.AddMessage(self._cfield_descriptor)
    return self._subclass(__cmessage=cmessage, __owner=self._message, **kwargs)

  def extend(self, elem_seq):
    """Extends by appending the given sequence of elements of the same type
    as this one, copying each individual message.
    """
    for message in elem_seq:
      self.add().MergeFrom(message)

  def MergeFrom(self, other):
    for message in other[:]:
      self.add().MergeFrom(message)

  def __getitem__(self, key):
    cmessages = self._cmsg.GetRepeatedMessage(
        self._cfield_descriptor, key)
    subclass = self._subclass
    if not isinstance(cmessages, list):
      return subclass(__cmessage=cmessages, __owner=self._message)

    return [subclass(__cmessage=m, __owner=self._message) for m in cmessages]

  def __delitem__(self, key):
    self._cmsg.DeleteRepeatedField(
        self._cfield_descriptor, key)

  def __len__(self):
    return self._cmsg.FieldLength(self._cfield_descriptor)

  def __eq__(self, other):
    """Compares the current instance with another one."""
    if self is other:
      return True
    if not isinstance(other, self.__class__):
      raise TypeError('Can only compare repeated composite fields against '
                      'other repeated composite fields.')
    messages = self[slice(None, None, None)]
    other_messages = other[slice(None, None, None)]
    return messages == other_messages

  def __hash__(self):
    raise TypeError('unhashable object')

  def sort(self, sort_function=cmp):
    messages = []
    for index in range(len(self)):
      # messages[i][0] is where the i-th element of the new array has to come
      # from.
      # messages[i][1] is where the i-th element of the old array has to go.
      messages.append([index, 0, self[index]])
    messages.sort(lambda x,y: sort_function(x[2], y[2]))

    # Remember which position each elements has to move to.
    for i in range(len(messages)):
      messages[messages[i][0]][1] = i

    # Apply the transposition.
    for i in range(len(messages)):
      from_position = messages[i][0]
      if i == from_position:
        continue
      self._cmsg.SwapRepeatedFieldElements(
          self._cfield_descriptor, i, from_position)
      messages[messages[i][1]][0] = from_position


def RepeatedCompositeProperty(cdescriptor, message_type):
  """Returns a Python property for the given repeated composite field."""

  def Getter(self):
    container = self._composite_fields.get(cdescriptor.name, None)
    if container is None:
      container = RepeatedCompositeContainer(
          self, cdescriptor, message_type._concrete_class)
      self._composite_fields[cdescriptor.name] = container
    return container

  def Setter(self, new_value):
    raise AttributeError('Assignment not allowed to repeated field '
                         '"%s" in protocol message object.' % cdescriptor.name)

  doc = 'Magic attribute generated for "%s" proto field.' % cdescriptor.name
  return property(Getter, Setter, doc=doc)


class ExtensionDict(object):
  """Extension dictionary added to each protocol message."""

  def __init__(self, msg):
    self._message = msg
    self._cmsg = msg._cmsg
    self._values = {}

  def __setitem__(self, extension, value):
    from google.protobuf import descriptor
    if not isinstance(extension, descriptor.FieldDescriptor):
      raise KeyError('Bad extension %r.' % (extension,))
    cdescriptor = extension._cdescriptor
    if (cdescriptor.label != _LABEL_OPTIONAL or
        cdescriptor.cpp_type == _CPPTYPE_MESSAGE):
      raise TypeError('Extension %r is repeated and/or a composite type.' % (
          extension.full_name,))
    self._cmsg.SetScalar(cdescriptor, value)
    self._values[extension] = value

  def __getitem__(self, extension):
    from google.protobuf import descriptor
    if not isinstance(extension, descriptor.FieldDescriptor):
      raise KeyError('Bad extension %r.' % (extension,))

    cdescriptor = extension._cdescriptor
    if (cdescriptor.label != _LABEL_REPEATED and
        cdescriptor.cpp_type != _CPPTYPE_MESSAGE):
      return self._cmsg.GetScalar(cdescriptor)

    ext = self._values.get(extension, None)
    if ext is not None:
      return ext

    ext = self._CreateNewHandle(extension)
    self._values[extension] = ext
    return ext

  def ClearExtension(self, extension):
    from google.protobuf import descriptor
    if not isinstance(extension, descriptor.FieldDescriptor):
      raise KeyError('Bad extension %r.' % (extension,))
    self._cmsg.ClearFieldByDescriptor(extension._cdescriptor)
    if extension in self._values:
      del self._values[extension]

  def HasExtension(self, extension):
    from google.protobuf import descriptor
    if not isinstance(extension, descriptor.FieldDescriptor):
      raise KeyError('Bad extension %r.' % (extension,))
    return self._cmsg.HasFieldByDescriptor(extension._cdescriptor)

  def _FindExtensionByName(self, name):
    """Tries to find a known extension with the specified name.

    Args:
      name: Extension full name.

    Returns:
      Extension field descriptor.
    """
    return self._message._extensions_by_name.get(name, None)

  def _CreateNewHandle(self, extension):
    cdescriptor = extension._cdescriptor
    if (cdescriptor.label != _LABEL_REPEATED and
        cdescriptor.cpp_type == _CPPTYPE_MESSAGE):
      cmessage = self._cmsg.NewSubMessage(cdescriptor)
      return extension.message_type._concrete_class(__cmessage=cmessage)

    if cdescriptor.label == _LABEL_REPEATED:
      if cdescriptor.cpp_type == _CPPTYPE_MESSAGE:
        return RepeatedCompositeContainer(
            self._message, cdescriptor, extension.message_type._concrete_class)
      else:
        return RepeatedScalarContainer(self._message, cdescriptor)
    # This shouldn't happen!
    assert False
    return None


def NewMessage(message_descriptor, dictionary):
  """Creates a new protocol message *class*."""
  _AddClassAttributesForNestedExtensions(message_descriptor, dictionary)
  _AddEnumValues(message_descriptor, dictionary)
  _AddDescriptors(message_descriptor, dictionary)


def InitMessage(message_descriptor, cls):
  """Constructs a new message instance (called before instance's __init__)."""
  cls._extensions_by_name = {}
  _AddInitMethod(message_descriptor, cls)
  _AddMessageMethods(message_descriptor, cls)
  _AddPropertiesForExtensions(message_descriptor, cls)


def _AddDescriptors(message_descriptor, dictionary):
  """Sets up a new protocol message class dictionary.

  Args:
    message_descriptor: A Descriptor instance describing this message type.
    dictionary: Class dictionary to which we'll add a '__slots__' entry.
  """
  dictionary['__descriptors'] = {}
  for field in message_descriptor.fields:
    dictionary['__descriptors'][field.name] = GetFieldDescriptor(
        field.full_name)

  dictionary['__slots__'] = list(dictionary['__descriptors'].iterkeys()) + [
      '_cmsg', '_owner', '_composite_fields', 'Extensions']


def _AddEnumValues(message_descriptor, dictionary):
  """Sets class-level attributes for all enum fields defined in this message.

  Args:
    message_descriptor: Descriptor object for this message type.
    dictionary: Class dictionary that should be populated.
  """
  for enum_type in message_descriptor.enum_types:
    for enum_value in enum_type.values:
      dictionary[enum_value.name] = enum_value.number


def _AddClassAttributesForNestedExtensions(message_descriptor, dictionary):
  """Adds class attributes for the nested extensions."""
  extension_dict = message_descriptor.extensions_by_name
  for extension_name, extension_field in extension_dict.iteritems():
    assert extension_name not in dictionary
    dictionary[extension_name] = extension_field


def _AddInitMethod(message_descriptor, cls):
  """Adds an __init__ method to cls."""

  # Create and attach message field properties to the message class.
  # This can be done just once per message class, since property setters and
  # getters are passed the message instance.
  # This makes message instantiation extremely fast, and at the same time it
  # doesn't require the creation of property objects for each message instance,
  # which saves a lot of memory.
  for field in message_descriptor.fields:
    field_cdescriptor = cls.__descriptors[field.name]
    if field.label == _LABEL_REPEATED:
      if field.cpp_type == _CPPTYPE_MESSAGE:
        value = RepeatedCompositeProperty(field_cdescriptor, field.message_type)
      else:
        value = RepeatedScalarProperty(field_cdescriptor)
    elif field.cpp_type == _CPPTYPE_MESSAGE:
      value = CompositeProperty(field_cdescriptor, field.message_type)
    else:
      value = ScalarProperty(field_cdescriptor)
    setattr(cls, field.name, value)

    # Attach a constant with the field number.
    constant_name = field.name.upper() + '_FIELD_NUMBER'
    setattr(cls, constant_name, field.number)

  def Init(self, **kwargs):
    """Message constructor."""
    cmessage = kwargs.pop('__cmessage', None)
    if cmessage is None:
      self._cmsg = NewCMessage(message_descriptor.full_name)
    else:
      self._cmsg = cmessage

    # Keep a reference to the owner, as the owner keeps a reference to the
    # underlying protocol buffer message.
    owner = kwargs.pop('__owner', None)
    if owner is not None:
      self._owner = owner

    self.Extensions = ExtensionDict(self)
    self._composite_fields = {}

    for field_name, field_value in kwargs.iteritems():
      field_cdescriptor = self.__descriptors.get(field_name, None)
      if field_cdescriptor is None:
        raise ValueError('Protocol message has no "%s" field.' % field_name)
      if field_cdescriptor.label == _LABEL_REPEATED:
        if field_cdescriptor.cpp_type == _CPPTYPE_MESSAGE:
          for val in field_value:
            getattr(self, field_name).add().MergeFrom(val)
        else:
          getattr(self, field_name).extend(field_value)
      elif field_cdescriptor.cpp_type == _CPPTYPE_MESSAGE:
        getattr(self, field_name).MergeFrom(field_value)
      else:
        setattr(self, field_name, field_value)

  Init.__module__ = None
  Init.__doc__ = None
  cls.__init__ = Init


def _IsMessageSetExtension(field):
  """Checks if a field is a message set extension."""
  return (field.is_extension and
          field.containing_type.has_options and
          field.containing_type.GetOptions().message_set_wire_format and
          field.type == _TYPE_MESSAGE and
          field.message_type == field.extension_scope and
          field.label == _LABEL_OPTIONAL)


def _AddMessageMethods(message_descriptor, cls):
  """Adds the methods to a protocol message class."""
  if message_descriptor.is_extendable:

    def ClearExtension(self, extension):
      self.Extensions.ClearExtension(extension)

    def HasExtension(self, extension):
      return self.Extensions.HasExtension(extension)

  def HasField(self, field_name):
    return self._cmsg.HasField(field_name)

  def ClearField(self, field_name):
    if field_name in self._composite_fields:
      del self._composite_fields[field_name]
    self._cmsg.ClearField(field_name)

  def Clear(self):
    return self._cmsg.Clear()

  def IsInitialized(self, errors=None):
    if self._cmsg.IsInitialized():
      return True
    if errors is not None:
      errors.extend(self.FindInitializationErrors());
    return False

  def SerializeToString(self):
    if not self.IsInitialized():
      raise message.EncodeError(
          'Message is missing required fields: ' +
          ','.join(self.FindInitializationErrors()))
    return self._cmsg.SerializeToString()

  def SerializePartialToString(self):
    return self._cmsg.SerializePartialToString()

  def ParseFromString(self, serialized):
    self.Clear()
    self.MergeFromString(serialized)

  def MergeFromString(self, serialized):
    byte_size = self._cmsg.MergeFromString(serialized)
    if byte_size < 0:
      raise message.DecodeError('Unable to merge from string.')
    return byte_size

  def MergeFrom(self, msg):
    if not isinstance(msg, cls):
      raise TypeError(
          "Parameter to MergeFrom() must be instance of same class.")
    self._cmsg.MergeFrom(msg._cmsg)

  def CopyFrom(self, msg):
    self._cmsg.CopyFrom(msg._cmsg)

  def ByteSize(self):
    return self._cmsg.ByteSize()

  def SetInParent(self):
    return self._cmsg.SetInParent()

  def ListFields(self):
    all_fields = []
    field_list = self._cmsg.ListFields()
    fields_by_name = cls.DESCRIPTOR.fields_by_name
    for is_extension, field_name in field_list:
      if is_extension:
        extension = cls._extensions_by_name[field_name]
        all_fields.append((extension, self.Extensions[extension]))
      else:
        field_descriptor = fields_by_name[field_name]
        all_fields.append(
            (field_descriptor, getattr(self, field_name)))
    all_fields.sort(key=lambda item: item[0].number)
    return all_fields

  def FindInitializationErrors(self):
    return self._cmsg.FindInitializationErrors()

  def __str__(self):
    return self._cmsg.DebugString()

  def __eq__(self, other):
    if self is other:
      return True
    if not isinstance(other, self.__class__):
      return False
    return self.ListFields() == other.ListFields()

  def __ne__(self, other):
    return not self == other

  def __hash__(self):
    raise TypeError('unhashable object')

  def __unicode__(self):
    return text_format.MessageToString(self, as_utf8=True).decode('utf-8')

  # Attach the local methods to the message class.
  for key, value in locals().copy().iteritems():
    if key not in ('key', 'value', '__builtins__', '__name__', '__doc__'):
      setattr(cls, key, value)

  # Static methods:

  def RegisterExtension(extension_handle):
    extension_handle.containing_type = cls.DESCRIPTOR
    cls._extensions_by_name[extension_handle.full_name] = extension_handle

    if _IsMessageSetExtension(extension_handle):
      # MessageSet extension.  Also register under type name.
      cls._extensions_by_name[
          extension_handle.message_type.full_name] = extension_handle
  cls.RegisterExtension = staticmethod(RegisterExtension)

  def FromString(string):
    msg = cls()
    msg.MergeFromString(string)
    return msg
  cls.FromString = staticmethod(FromString)



def _AddPropertiesForExtensions(message_descriptor, cls):
  """Adds properties for all fields in this protocol message type."""
  extension_dict = message_descriptor.extensions_by_name
  for extension_name, extension_field in extension_dict.iteritems():
    constant_name = extension_name.upper() + '_FIELD_NUMBER'
    setattr(cls, constant_name, extension_field.number)
