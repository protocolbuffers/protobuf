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

# This code is meant to work on Python 2.4 and above only.
#
# TODO(robinson): Helpers for verbose, common checks like seeing if a
# descriptor's cpp_type is CPPTYPE_MESSAGE.

"""Contains a metaclass and helper functions used to create
protocol message classes from Descriptor objects at runtime.

Recall that a metaclass is the "type" of a class.
(A class is to a metaclass what an instance is to a class.)

In this case, we use the GeneratedProtocolMessageType metaclass
to inject all the useful functionality into the classes
output by the protocol compiler at compile-time.

The upshot of all this is that the real implementation
details for ALL pure-Python protocol buffers are *here in
this file*.
"""

__author__ = 'robinson@google.com (Will Robinson)'

import heapq
import threading
import weakref
# We use "as" to avoid name collisions with variables.
from google.protobuf.internal import decoder
from google.protobuf.internal import encoder
from google.protobuf.internal import message_listener as message_listener_mod
from google.protobuf.internal import wire_format
from google.protobuf import descriptor as descriptor_mod
from google.protobuf import message as message_mod

_FieldDescriptor = descriptor_mod.FieldDescriptor


class GeneratedProtocolMessageType(type):

  """Metaclass for protocol message classes created at runtime from Descriptors.

  We add implementations for all methods described in the Message class.  We
  also create properties to allow getting/setting all fields in the protocol
  message.  Finally, we create slots to prevent users from accidentally
  "setting" nonexistent fields in the protocol message, which then wouldn't get
  serialized / deserialized properly.

  The protocol compiler currently uses this metaclass to create protocol
  message classes at runtime.  Clients can also manually create their own
  classes at runtime, as in this example:

  mydescriptor = Descriptor(.....)
  class MyProtoClass(Message):
    __metaclass__ = GeneratedProtocolMessageType
    DESCRIPTOR = mydescriptor
  myproto_instance = MyProtoClass()
  myproto.foo_field = 23
  ...
  """

  # Must be consistent with the protocol-compiler code in
  # proto2/compiler/internal/generator.*.
  _DESCRIPTOR_KEY = 'DESCRIPTOR'

  def __new__(cls, name, bases, dictionary):
    """Custom allocation for runtime-generated class types.

    We override __new__ because this is apparently the only place
    where we can meaningfully set __slots__ on the class we're creating(?).
    (The interplay between metaclasses and slots is not very well-documented).

    Args:
      name: Name of the class (ignored, but required by the
        metaclass protocol).
      bases: Base classes of the class we're constructing.
        (Should be message.Message).  We ignore this field, but
        it's required by the metaclass protocol
      dictionary: The class dictionary of the class we're
        constructing.  dictionary[_DESCRIPTOR_KEY] must contain
        a Descriptor object describing this protocol message
        type.

    Returns:
      Newly-allocated class.
    """
    descriptor = dictionary[GeneratedProtocolMessageType._DESCRIPTOR_KEY]
    _AddSlots(descriptor, dictionary)
    _AddClassAttributesForNestedExtensions(descriptor, dictionary)
    superclass = super(GeneratedProtocolMessageType, cls)
    return superclass.__new__(cls, name, bases, dictionary)

  def __init__(cls, name, bases, dictionary):
    """Here we perform the majority of our work on the class.
    We add enum getters, an __init__ method, implementations
    of all Message methods, and properties for all fields
    in the protocol type.

    Args:
      name: Name of the class (ignored, but required by the
        metaclass protocol).
      bases: Base classes of the class we're constructing.
        (Should be message.Message).  We ignore this field, but
        it's required by the metaclass protocol
      dictionary: The class dictionary of the class we're
        constructing.  dictionary[_DESCRIPTOR_KEY] must contain
        a Descriptor object describing this protocol message
        type.
    """
    descriptor = dictionary[GeneratedProtocolMessageType._DESCRIPTOR_KEY]
    # We act as a "friend" class of the descriptor, setting
    # its _concrete_class attribute the first time we use a
    # given descriptor to initialize a concrete protocol message
    # class.
    concrete_class_attr_name = '_concrete_class'
    if not hasattr(descriptor, concrete_class_attr_name):
      setattr(descriptor, concrete_class_attr_name, cls)
    cls._known_extensions = []
    _AddEnumValues(descriptor, cls)
    _AddInitMethod(descriptor, cls)
    _AddPropertiesForFields(descriptor, cls)
    _AddStaticMethods(cls)
    _AddMessageMethods(descriptor, cls)
    _AddPrivateHelperMethods(cls)
    superclass = super(GeneratedProtocolMessageType, cls)
    superclass.__init__(cls, name, bases, dictionary)


# Stateless helpers for GeneratedProtocolMessageType below.
# Outside clients should not access these directly.
#
# I opted not to make any of these methods on the metaclass, to make it more
# clear that I'm not really using any state there and to keep clients from
# thinking that they have direct access to these construction helpers.


def _PropertyName(proto_field_name):
  """Returns the name of the public property attribute which
  clients can use to get and (in some cases) set the value
  of a protocol message field.

  Args:
    proto_field_name: The protocol message field name, exactly
      as it appears (or would appear) in a .proto file.
  """
  # TODO(robinson): Escape Python keywords (e.g., yield), and test this support.
  # nnorwitz makes my day by writing:
  # """
  # FYI.  See the keyword module in the stdlib. This could be as simple as:
  #
  # if keyword.iskeyword(proto_field_name):
  #   return proto_field_name + "_"
  # return proto_field_name
  # """
  return proto_field_name


def _ValueFieldName(proto_field_name):
  """Returns the name of the (internal) instance attribute which objects
  should use to store the current value for a given protocol message field.

  Args:
    proto_field_name: The protocol message field name, exactly
      as it appears (or would appear) in a .proto file.
  """
  return '_value_' + proto_field_name


def _HasFieldName(proto_field_name):
  """Returns the name of the (internal) instance attribute which
  objects should use to store a boolean telling whether this field
  is explicitly set or not.

  Args:
    proto_field_name: The protocol message field name, exactly
      as it appears (or would appear) in a .proto file.
  """
  return '_has_' + proto_field_name


def _AddSlots(message_descriptor, dictionary):
  """Adds a __slots__ entry to dictionary, containing the names of all valid
  attributes for this message type.

  Args:
    message_descriptor: A Descriptor instance describing this message type.
    dictionary: Class dictionary to which we'll add a '__slots__' entry.
  """
  field_names = [_ValueFieldName(f.name) for f in message_descriptor.fields]
  field_names.extend(_HasFieldName(f.name) for f in message_descriptor.fields
                     if f.label != _FieldDescriptor.LABEL_REPEATED)
  field_names.extend(('Extensions',
                      '_cached_byte_size',
                      '_cached_byte_size_dirty',
                      '_called_transition_to_nonempty',
                      '_listener',
                      '_lock', '__weakref__'))
  dictionary['__slots__'] = field_names


def _AddClassAttributesForNestedExtensions(descriptor, dictionary):
  extension_dict = descriptor.extensions_by_name
  for extension_name, extension_field in extension_dict.iteritems():
    assert extension_name not in dictionary
    dictionary[extension_name] = extension_field


def _AddEnumValues(descriptor, cls):
  """Sets class-level attributes for all enum fields defined in this message.

  Args:
    descriptor: Descriptor object for this message type.
    cls: Class we're constructing for this message type.
  """
  for enum_type in descriptor.enum_types:
    for enum_value in enum_type.values:
      setattr(cls, enum_value.name, enum_value.number)


def _DefaultValueForField(message, field):
  """Returns a default value for a field.

  Args:
    message: Message instance containing this field, or a weakref proxy
      of same.
    field: FieldDescriptor object for this field.

  Returns: A default value for this field.  May refer back to |message|
    via a weak reference.
  """
  # TODO(robinson): Only the repeated fields need a reference to 'message' (so
  # that they can set the 'has' bit on the containing Message when someone
  # append()s a value).  We could special-case this, and avoid an extra
  # function call on __init__() and Clear() for non-repeated fields.

  # TODO(robinson): Find a better place for the default value assertion in this
  # function.  No need to repeat them every time the client calls Clear('foo').
  # (We should probably just assert these things once and as early as possible,
  # by tightening checking in the descriptor classes.)
  if field.label == _FieldDescriptor.LABEL_REPEATED:
    if field.default_value != []:
      raise ValueError('Repeated field default value not empty list: %s' % (
          field.default_value))
    listener = _Listener(message, None)
    if field.cpp_type == _FieldDescriptor.CPPTYPE_MESSAGE:
      # We can't look at _concrete_class yet since it might not have
      # been set.  (Depends on order in which we initialize the classes).
      return _RepeatedCompositeFieldContainer(listener, field.message_type)
    else:
      return _RepeatedScalarFieldContainer(listener,
                                           _VALUE_CHECKERS[field.cpp_type])

  if field.cpp_type == _FieldDescriptor.CPPTYPE_MESSAGE:
    assert field.default_value is None

  return field.default_value


def _AddInitMethod(message_descriptor, cls):
  """Adds an __init__ method to cls."""
  fields = message_descriptor.fields
  def init(self):
    self._cached_byte_size = 0
    self._cached_byte_size_dirty = False
    self._listener = message_listener_mod.NullMessageListener()
    self._called_transition_to_nonempty = False
    # TODO(robinson): We should only create a lock if we really need one
    # in this class.
    self._lock = threading.Lock()
    for field in fields:
      default_value = _DefaultValueForField(self, field)
      python_field_name = _ValueFieldName(field.name)
      setattr(self, python_field_name, default_value)
      if field.label != _FieldDescriptor.LABEL_REPEATED:
        setattr(self, _HasFieldName(field.name), False)
    self.Extensions = _ExtensionDict(self, cls._known_extensions)

  init.__module__ = None
  init.__doc__ = None
  cls.__init__ = init


def _AddPropertiesForFields(descriptor, cls):
  """Adds properties for all fields in this protocol message type."""
  for field in descriptor.fields:
    _AddPropertiesForField(field, cls)


def _AddPropertiesForField(field, cls):
  """Adds a public property for a protocol message field.
  Clients can use this property to get and (in the case
  of non-repeated scalar fields) directly set the value
  of a protocol message field.

  Args:
    field: A FieldDescriptor for this field.
    cls: The class we're constructing.
  """
  # Catch it if we add other types that we should
  # handle specially here.
  assert _FieldDescriptor.MAX_CPPTYPE == 10

  if field.label == _FieldDescriptor.LABEL_REPEATED:
    _AddPropertiesForRepeatedField(field, cls)
  elif field.cpp_type == _FieldDescriptor.CPPTYPE_MESSAGE:
    _AddPropertiesForNonRepeatedCompositeField(field, cls)
  else:
    _AddPropertiesForNonRepeatedScalarField(field, cls)


def _AddPropertiesForRepeatedField(field, cls):
  """Adds a public property for a "repeated" protocol message field.  Clients
  can use this property to get the value of the field, which will be either a
  _RepeatedScalarFieldContainer or _RepeatedCompositeFieldContainer (see
  below).

  Note that when clients add values to these containers, we perform
  type-checking in the case of repeated scalar fields, and we also set any
  necessary "has" bits as a side-effect.

  Args:
    field: A FieldDescriptor for this field.
    cls: The class we're constructing.
  """
  proto_field_name = field.name
  python_field_name = _ValueFieldName(proto_field_name)
  property_name = _PropertyName(proto_field_name)

  def getter(self):
    return getattr(self, python_field_name)
  getter.__module__ = None
  getter.__doc__ = 'Getter for %s.' % proto_field_name

  # We define a setter just so we can throw an exception with a more
  # helpful error message.
  def setter(self, new_value):
    raise AttributeError('Assignment not allowed to repeated field '
                         '"%s" in protocol message object.' % proto_field_name)

  doc = 'Magic attribute generated for "%s" proto field.' % proto_field_name
  setattr(cls, property_name, property(getter, setter, doc=doc))


def _AddPropertiesForNonRepeatedScalarField(field, cls):
  """Adds a public property for a nonrepeated, scalar protocol message field.
  Clients can use this property to get and directly set the value of the field.
  Note that when the client sets the value of a field by using this property,
  all necessary "has" bits are set as a side-effect, and we also perform
  type-checking.

  Args:
    field: A FieldDescriptor for this field.
    cls: The class we're constructing.
  """
  proto_field_name = field.name
  python_field_name = _ValueFieldName(proto_field_name)
  has_field_name = _HasFieldName(proto_field_name)
  property_name = _PropertyName(proto_field_name)
  type_checker = _VALUE_CHECKERS[field.cpp_type]

  def getter(self):
    return getattr(self, python_field_name)
  getter.__module__ = None
  getter.__doc__ = 'Getter for %s.' % proto_field_name
  def setter(self, new_value):
    type_checker.CheckValue(new_value)
    setattr(self, has_field_name, True)
    self._MarkByteSizeDirty()
    self._MaybeCallTransitionToNonemptyCallback()
    setattr(self, python_field_name, new_value)
  setter.__module__ = None
  setter.__doc__ = 'Setter for %s.' % proto_field_name

  # Add a property to encapsulate the getter/setter.
  doc = 'Magic attribute generated for "%s" proto field.' % proto_field_name
  setattr(cls, property_name, property(getter, setter, doc=doc))


def _AddPropertiesForNonRepeatedCompositeField(field, cls):
  """Adds a public property for a nonrepeated, composite protocol message field.
  A composite field is a "group" or "message" field.

  Clients can use this property to get the value of the field, but cannot
  assign to the property directly.

  Args:
    field: A FieldDescriptor for this field.
    cls: The class we're constructing.
  """
  # TODO(robinson): Remove duplication with similar method
  # for non-repeated scalars.
  proto_field_name = field.name
  python_field_name = _ValueFieldName(proto_field_name)
  has_field_name = _HasFieldName(proto_field_name)
  property_name = _PropertyName(proto_field_name)
  message_type = field.message_type

  def getter(self):
    # TODO(robinson): Appropriately scary note about double-checked locking.
    field_value = getattr(self, python_field_name)
    if field_value is None:
      self._lock.acquire()
      try:
        field_value = getattr(self, python_field_name)
        if field_value is None:
          field_class = message_type._concrete_class
          field_value = field_class()
          field_value._SetListener(_Listener(self, has_field_name))
          setattr(self, python_field_name, field_value)
      finally:
        self._lock.release()
    return field_value
  getter.__module__ = None
  getter.__doc__ = 'Getter for %s.' % proto_field_name

  # We define a setter just so we can throw an exception with a more
  # helpful error message.
  def setter(self, new_value):
    raise AttributeError('Assignment not allowed to composite field '
                         '"%s" in protocol message object.' % proto_field_name)

  # Add a property to encapsulate the getter.
  doc = 'Magic attribute generated for "%s" proto field.' % proto_field_name
  setattr(cls, property_name, property(getter, setter, doc=doc))


def _AddStaticMethods(cls):
  # TODO(robinson): This probably needs to be thread-safe(?)
  def RegisterExtension(extension_handle):
    extension_handle.containing_type = cls.DESCRIPTOR
    cls._known_extensions.append(extension_handle)
  cls.RegisterExtension = staticmethod(RegisterExtension)


def _AddListFieldsMethod(message_descriptor, cls):
  """Helper for _AddMessageMethods()."""

  # Ensure that we always list in ascending field-number order.
  # For non-extension fields, we can do the sort once, here, at import-time.
  # For extensions, we sort on each ListFields() call, though
  # we could do better if we have to.
  fields = sorted(message_descriptor.fields, key=lambda f: f.number)
  has_field_names = (_HasFieldName(f.name) for f in fields)
  value_field_names = (_ValueFieldName(f.name) for f in fields)
  triplets = zip(has_field_names, value_field_names, fields)

  def ListFields(self):
    # We need to list all extension and non-extension fields
    # together, in sorted order by field number.

    # Step 0: Get an iterator over all "set" non-extension fields,
    # sorted by field number.
    # This iterator yields (field_number, field_descriptor, value) tuples.
    def SortedSetFieldsIter():
      # Note that triplets is already sorted by field number.
      for has_field_name, value_field_name, field_descriptor in triplets:
        if field_descriptor.label == _FieldDescriptor.LABEL_REPEATED:
          value = getattr(self, _ValueFieldName(field_descriptor.name))
          if len(value) > 0:
            yield (field_descriptor.number, field_descriptor, value)
        elif getattr(self, _HasFieldName(field_descriptor.name)):
          value = getattr(self, _ValueFieldName(field_descriptor.name))
          yield (field_descriptor.number, field_descriptor, value)
    sorted_fields = SortedSetFieldsIter()

    # Step 1: Get an iterator over all "set" extension fields,
    # sorted by field number.
    # This iterator ALSO yields (field_number, field_descriptor, value) tuples.
    # TODO(robinson): It's not necessary to repeat this with each
    # serialization call.  We can do better.
    sorted_extension_fields = sorted(
        [(f.number, f, v) for f, v in self.Extensions._ListSetExtensions()])

    # Step 2: Create a composite iterator that merges the extension-
    # and non-extension fields, and that still yields fields in
    # sorted order.
    all_set_fields = _ImergeSorted(sorted_fields, sorted_extension_fields)

    # Step 3: Strip off the field numbers and return.
    return [field[1:] for field in all_set_fields]

  cls.ListFields = ListFields

def _AddHasFieldMethod(cls):
  """Helper for _AddMessageMethods()."""
  def HasField(self, field_name):
    try:
      return getattr(self, _HasFieldName(field_name))
    except AttributeError:
      raise ValueError('Protocol message has no "%s" field.' % field_name)
  cls.HasField = HasField


def _AddClearFieldMethod(cls):
  """Helper for _AddMessageMethods()."""
  def ClearField(self, field_name):
    try:
      field = self.DESCRIPTOR.fields_by_name[field_name]
    except KeyError:
      raise ValueError('Protocol message has no "%s" field.' % field_name)
    proto_field_name = field.name
    python_field_name = _ValueFieldName(proto_field_name)
    has_field_name = _HasFieldName(proto_field_name)
    default_value = _DefaultValueForField(self, field)
    if field.label == _FieldDescriptor.LABEL_REPEATED:
      self._MarkByteSizeDirty()
    else:
      if field.cpp_type == _FieldDescriptor.CPPTYPE_MESSAGE:
        old_field_value = getattr(self, python_field_name)
        if old_field_value is not None:
          # Snip the old object out of the object tree.
          old_field_value._SetListener(None)
      if getattr(self, has_field_name):
        setattr(self, has_field_name, False)
        # Set dirty bit on ourself and parents only if
        # we're actually changing state.
        self._MarkByteSizeDirty()
    setattr(self, python_field_name, default_value)
  cls.ClearField = ClearField


def _AddClearExtensionMethod(cls):
  """Helper for _AddMessageMethods()."""
  def ClearExtension(self, extension_handle):
    self.Extensions._ClearExtension(extension_handle)
  cls.ClearExtension = ClearExtension


def _AddClearMethod(cls):
  """Helper for _AddMessageMethods()."""
  def Clear(self):
    # Clear fields.
    fields = self.DESCRIPTOR.fields
    for field in fields:
      self.ClearField(field.name)
    # Clear extensions.
    extensions = self.Extensions._ListSetExtensions()
    for extension in extensions:
      self.ClearExtension(extension[0])
  cls.Clear = Clear


def _AddHasExtensionMethod(cls):
  """Helper for _AddMessageMethods()."""
  def HasExtension(self, extension_handle):
    return self.Extensions._HasExtension(extension_handle)
  cls.HasExtension = HasExtension


def _AddEqualsMethod(message_descriptor, cls):
  """Helper for _AddMessageMethods()."""
  def __eq__(self, other):
    if self is other:
      return True

    # Compare all fields contained directly in this message.
    for field_descriptor in message_descriptor.fields:
      label = field_descriptor.label
      property_name = _PropertyName(field_descriptor.name)
      # Non-repeated field equality requires matching "has" bits as well
      # as having an equal value.
      if label != _FieldDescriptor.LABEL_REPEATED:
        self_has = self.HasField(property_name)
        other_has = other.HasField(property_name)
        if self_has != other_has:
          return False
        if not self_has:
          # If the "has" bit for this field is False, we must stop here.
          # Otherwise we will recurse forever on recursively-defined protos.
          continue
      if getattr(self, property_name) != getattr(other, property_name):
        return False

    # Compare the extensions present in both messages.
    return self.Extensions == other.Extensions
  cls.__eq__ = __eq__


def _AddSetListenerMethod(cls):
  """Helper for _AddMessageMethods()."""
  def SetListener(self, listener):
    if listener is None:
      self._listener = message_listener_mod.NullMessageListener()
    else:
      self._listener = listener
  cls._SetListener = SetListener


def _BytesForNonRepeatedElement(value, field_number, field_type):
  """Returns the number of bytes needed to serialize a non-repeated element.
  The returned byte count includes space for tag information and any
  other additional space associated with serializing value.

  Args:
    value: Value we're serializing.
    field_number: Field number of this value.  (Since the field number
      is stored as part of a varint-encoded tag, this has an impact
      on the total bytes required to serialize the value).
    field_type: The type of the field.  One of the TYPE_* constants
      within FieldDescriptor.
  """
  try:
    fn = _TYPE_TO_BYTE_SIZE_FN[field_type]
    return fn(field_number, value)
  except KeyError:
    raise message_mod.EncodeError('Unrecognized field type: %d' % field_type)


def _AddByteSizeMethod(message_descriptor, cls):
  """Helper for _AddMessageMethods()."""

  def BytesForField(message, field, value):
    """Returns the number of bytes required to serialize a single field
    in message.  The field may be repeated or not, composite or not.

    Args:
      message: The Message instance containing a field of the given type.
      field: A FieldDescriptor describing the field of interest.
      value: The value whose byte size we're interested in.

    Returns: The number of bytes required to serialize the current value
      of "field" in "message", including space for tags and any other
      necessary information.
    """

    if _MessageSetField(field):
      return wire_format.MessageSetItemByteSize(field.number, value)

    field_number, field_type = field.number, field.type

    # Repeated fields.
    if field.label == _FieldDescriptor.LABEL_REPEATED:
      elements = value
    else:
      elements = [value]

    size = sum(_BytesForNonRepeatedElement(element, field_number, field_type)
               for element in elements)
    return size

  fields = message_descriptor.fields
  has_field_names = (_HasFieldName(f.name) for f in fields)
  zipped = zip(has_field_names, fields)

  def ByteSize(self):
    if not self._cached_byte_size_dirty:
      return self._cached_byte_size

    size = 0
    # Hardcoded fields first.
    for has_field_name, field in zipped:
      if (field.label == _FieldDescriptor.LABEL_REPEATED
          or getattr(self, has_field_name)):
        value = getattr(self, _ValueFieldName(field.name))
        size += BytesForField(self, field, value)
    # Extensions next.
    for field, value in self.Extensions._ListSetExtensions():
      size += BytesForField(self, field, value)

    self._cached_byte_size = size
    self._cached_byte_size_dirty = False
    return size
  cls.ByteSize = ByteSize


def _MessageSetField(field_descriptor):
  """Checks if a field should be serialized using the message set wire format.

  Args:
    field_descriptor: Descriptor of the field.

  Returns:
    True if the field should be serialized using the message set wire format,
    false otherwise.
  """
  return (field_descriptor.is_extension and
          field_descriptor.label != _FieldDescriptor.LABEL_REPEATED and
          field_descriptor.cpp_type == _FieldDescriptor.CPPTYPE_MESSAGE and
          field_descriptor.containing_type.GetOptions().message_set_wire_format)


def _SerializeValueToEncoder(value, field_number, field_descriptor, encoder):
  """Appends the serialization of a single value to encoder.

  Args:
    value: Value to serialize.
    field_number: Field number of this value.
    field_descriptor: Descriptor of the field to serialize.
    encoder: encoder.Encoder object to which we should serialize this value.
  """
  if _MessageSetField(field_descriptor):
    encoder.AppendMessageSetItem(field_number, value)
    return

  try:
    method = _TYPE_TO_SERIALIZE_METHOD[field_descriptor.type]
    method(encoder, field_number, value)
  except KeyError:
    raise message_mod.EncodeError('Unrecognized field type: %d' %
                                  field_descriptor.type)


def _ImergeSorted(*streams):
  """Merges N sorted iterators into a single sorted iterator.
  Each element in streams must be an iterable that yields
  its elements in sorted order, and the elements contained
  in each stream must all be comparable.

  There may be repeated elements in the component streams or
  across the streams; the repeated elements will all be repeated
  in the merged iterator as well.

  I believe that the heapq module at HEAD in the Python
  sources has a method like this, but for now we roll our own.
  """
  iters = [iter(stream) for stream in streams]
  heap = []
  for index, it in enumerate(iters):
    try:
      heap.append((it.next(), index))
    except StopIteration:
      pass
  heapq.heapify(heap)

  while heap:
    smallest_value, idx = heap[0]
    yield smallest_value
    try:
      next_element = iters[idx].next()
      heapq.heapreplace(heap, (next_element, idx))
    except StopIteration:
      heapq.heappop(heap)


def _AddSerializeToStringMethod(message_descriptor, cls):
  """Helper for _AddMessageMethods()."""
  Encoder = encoder.Encoder

  def SerializeToString(self):
    encoder = Encoder()
    # We need to serialize all extension and non-extension fields
    # together, in sorted order by field number.

    # Step 3: Iterate over all extension and non-extension fields, sorted
    # in order of tag number, and serialize each one to the wire.
    for field_descriptor, field_value in self.ListFields():
      if field_descriptor.label == _FieldDescriptor.LABEL_REPEATED:
        repeated_value = field_value
      else:
        repeated_value = [field_value]
      for element in repeated_value:
        _SerializeValueToEncoder(element, field_descriptor.number,
                                 field_descriptor, encoder)
    return encoder.ToString()
  cls.SerializeToString = SerializeToString


def _WireTypeForFieldType(field_type):
  """Given a field type, returns the expected wire type."""
  try:
    return _FIELD_TYPE_TO_WIRE_TYPE[field_type]
  except KeyError:
    raise message_mod.DecodeError('Unknown field type: %d' % field_type)


def _RecursivelyMerge(field_number, field_type, decoder, message):
  """Decodes a message from decoder into message.
  message is either a group or a nested message within some containing
  protocol message.  If it's a group, we use the group protocol to
  deserialize, and if it's a nested message, we use the nested-message
  protocol.

  Args:
    field_number: The field number of message in its enclosing protocol buffer.
    field_type: The field type of message.  Must be either TYPE_MESSAGE
      or TYPE_GROUP.
    decoder: Decoder to read from.
    message: Message to deserialize into.
  """
  if field_type == _FieldDescriptor.TYPE_MESSAGE:
    decoder.ReadMessageInto(message)
  elif field_type == _FieldDescriptor.TYPE_GROUP:
    decoder.ReadGroupInto(field_number, message)
  else:
    raise message_mod.DecodeError('Unexpected field type: %d' % field_type)


def _DeserializeScalarFromDecoder(field_type, decoder):
  """Deserializes a scalar of the requested type from decoder.  field_type must
  be a scalar (non-group, non-message) FieldDescriptor.FIELD_* constant.
  """
  try:
    method = _TYPE_TO_DESERIALIZE_METHOD[field_type]
    return method(decoder)
  except KeyError:
    raise message_mod.DecodeError('Unrecognized field type: %d' % field_type)


def _SkipField(field_number, wire_type, decoder):
  """Skips a field with the specified wire type.

  Args:
    field_number: Tag number of the field to skip.
    wire_type: Wire type of the field to skip.
    decoder: Decoder used to deserialize the messsage. It must be positioned
      just after reading the the tag and wire type of the field.
  """
  if wire_type == wire_format.WIRETYPE_VARINT:
    decoder.ReadInt32()
  elif wire_type == wire_format.WIRETYPE_FIXED64:
    decoder.ReadFixed64()
  elif wire_type == wire_format.WIRETYPE_LENGTH_DELIMITED:
    decoder.SkipBytes(decoder.ReadInt32())
  elif wire_type == wire_format.WIRETYPE_START_GROUP:
    _SkipGroup(field_number, decoder)
  elif wire_type == wire_format.WIRETYPE_END_GROUP:
    pass
  elif wire_type == wire_format.WIRETYPE_FIXED32:
    decoder.ReadFixed32()
  else:
    raise message_mod.DecodeError('Unexpected wire type: %d' % wire_type)


def _SkipGroup(group_number, decoder):
  """Skips a nested group from the decoder.

  Args:
    group_number: Tag number of the group to skip.
    decoder: Decoder used to deserialize the message. It must be positioned
      exactly at the beginning of the message that should be skipped.
  """
  while True:
    field_number, wire_type = decoder.ReadFieldNumberAndWireType()
    if (wire_type == wire_format.WIRETYPE_END_GROUP and
        field_number == group_number):
      return
    _SkipField(field_number, wire_type, decoder)


def _DeserializeMessageSetItem(message, decoder):
  """Deserializes a message using the message set wire format.

  Args:
    message: Message to be parsed to.
    decoder: The decoder to be used to deserialize encoded data. Note that the
      decoder should be positioned just after reading the START_GROUP tag that
      began the messageset item.
  """
  field_number, wire_type = decoder.ReadFieldNumberAndWireType()
  if wire_type != wire_format.WIRETYPE_VARINT or field_number != 2:
    raise message_mod.DecodeError(
        'Incorrect message set wire format. '
        'wire_type: %d, field_number: %d' % (wire_type, field_number))

  type_id = decoder.ReadInt32()
  field_number, wire_type = decoder.ReadFieldNumberAndWireType()
  if wire_type != wire_format.WIRETYPE_LENGTH_DELIMITED or field_number != 3:
    raise message_mod.DecodeError(
        'Incorrect message set wire format. '
        'wire_type: %d, field_number: %d' % (wire_type, field_number))

  extension_dict = message.Extensions
  extensions_by_number = extension_dict._AllExtensionsByNumber()
  if type_id not in extensions_by_number:
    _SkipField(field_number, wire_type, decoder)
    return

  field_descriptor = extensions_by_number[type_id]
  value = extension_dict[field_descriptor]
  decoder.ReadMessageInto(value)
  # Read the END_GROUP tag.
  field_number, wire_type = decoder.ReadFieldNumberAndWireType()
  if wire_type != wire_format.WIRETYPE_END_GROUP or field_number != 1:
    raise message_mod.DecodeError(
        'Incorrect message set wire format. '
        'wire_type: %d, field_number: %d' % (wire_type, field_number))


def _DeserializeOneEntity(message_descriptor, message, decoder):
  """Deserializes the next wire entity from decoder into message.
  The next wire entity is either a scalar or a nested message,
  and may also be an element in a repeated field (the wire encoding
  is the same).

  Args:
    message_descriptor: A Descriptor instance describing all fields
      in message.
    message: The Message instance into which we're decoding our fields.
    decoder: The Decoder we're using to deserialize encoded data.

  Returns: The number of bytes read from decoder during this method.
  """
  initial_position = decoder.Position()
  field_number, wire_type = decoder.ReadFieldNumberAndWireType()
  extension_dict = message.Extensions
  extensions_by_number = extension_dict._AllExtensionsByNumber()
  if field_number in message_descriptor.fields_by_number:
    # Non-extension field.
    field_descriptor = message_descriptor.fields_by_number[field_number]
    value = getattr(message, _PropertyName(field_descriptor.name))
    def nonextension_setter_fn(scalar):
      setattr(message, _PropertyName(field_descriptor.name), scalar)
    scalar_setter_fn = nonextension_setter_fn
  elif field_number in extensions_by_number:
    # Extension field.
    field_descriptor = extensions_by_number[field_number]
    value = extension_dict[field_descriptor]
    def extension_setter_fn(scalar):
      extension_dict[field_descriptor] = scalar
    scalar_setter_fn = extension_setter_fn
  elif wire_type == wire_format.WIRETYPE_END_GROUP:
    # We assume we're being parsed as the group that's ended.
    return 0
  elif (wire_type == wire_format.WIRETYPE_START_GROUP and
        field_number == 1 and
        message_descriptor.GetOptions().message_set_wire_format):
    # A Message Set item.
    _DeserializeMessageSetItem(message, decoder)
    return decoder.Position() - initial_position
  else:
    _SkipField(field_number, wire_type, decoder)
    return decoder.Position() - initial_position

  # If we reach this point, we've identified the field as either
  # hardcoded or extension, and set |field_descriptor|, |scalar_setter_fn|,
  # and |value| appropriately.  Now actually deserialize the thing.
  #
  # field_descriptor: Describes the field we're deserializing.
  # value: The value currently stored in the field to deserialize.
  #   Used only if the field is composite and/or repeated.
  # scalar_setter_fn: A function F such that F(scalar) will
  #   set a nonrepeated scalar value for this field.  Used only
  #   if this field is a nonrepeated scalar.

  field_number = field_descriptor.number
  field_type = field_descriptor.type
  expected_wire_type = _WireTypeForFieldType(field_type)
  if wire_type != expected_wire_type:
    # Need to fill in uninterpreted_bytes.  Work for the next CL.
    raise RuntimeError('TODO(robinson): Wiretype mismatches not handled.')

  property_name = _PropertyName(field_descriptor.name)
  label = field_descriptor.label
  cpp_type = field_descriptor.cpp_type

  # Nonrepeated scalar.  Just set the field directly.
  if (label != _FieldDescriptor.LABEL_REPEATED
      and cpp_type != _FieldDescriptor.CPPTYPE_MESSAGE):
    scalar_setter_fn(_DeserializeScalarFromDecoder(field_type, decoder))
    return decoder.Position() - initial_position

  # Nonrepeated composite.  Recursively deserialize.
  if label != _FieldDescriptor.LABEL_REPEATED:
    composite = value
    _RecursivelyMerge(field_number, field_type, decoder, composite)
    return decoder.Position() - initial_position

  # Now we know we're dealing with a repeated field of some kind.
  element_list = value

  if cpp_type != _FieldDescriptor.CPPTYPE_MESSAGE:
    # Repeated scalar.
    element_list.append(_DeserializeScalarFromDecoder(field_type, decoder))
    return decoder.Position() - initial_position
  else:
    # Repeated composite.
    composite = element_list.add()
    _RecursivelyMerge(field_number, field_type, decoder, composite)
    return decoder.Position() - initial_position


def _FieldOrExtensionValues(message, field_or_extension):
  """Retrieves the list of values for the specified field or extension.

  The target field or extension can be optional, required or repeated, but it
  must have value(s) set. The assumption is that the target field or extension
  is set (e.g. _HasFieldOrExtension holds true).

  Args:
    message: Message which contains the target field or extension.
    field_or_extension: Field or extension for which the list of values is
      required. Must be an instance of FieldDescriptor.

  Returns:
    A list of values for the specified field or extension. This list will only
    contain a single element if the field is non-repeated.
  """
  if field_or_extension.is_extension:
    value = message.Extensions[field_or_extension]
  else:
    value = getattr(message, _ValueFieldName(field_or_extension.name))
  if field_or_extension.label != _FieldDescriptor.LABEL_REPEATED:
    return [value]
  else:
    # In this case value is a list or repeated values.
    return value


def _HasFieldOrExtension(message, field_or_extension):
  """Checks if a message has the specified field or extension set.

  The field or extension specified can be optional, required or repeated. If
  it is repeated, this function returns True. Otherwise it checks the has bit
  of the field or extension.

  Args:
    message: Message which contains the target field or extension.
    field_or_extension: Field or extension to check. This must be a
      FieldDescriptor instance.

  Returns:
    True if the message has a value set for the specified field or extension,
    or if the field or extension is repeated.
  """
  if field_or_extension.label == _FieldDescriptor.LABEL_REPEATED:
    return True
  if field_or_extension.is_extension:
    return message.HasExtension(field_or_extension)
  else:
    return message.HasField(field_or_extension.name)


def _IsFieldOrExtensionInitialized(message, field):
  """Checks if a message field or extension is initialized.

  Args:
    message: The message which contains the field or extension.
    field: Field or extension to check. This must be a FieldDescriptor instance.

  Returns:
    True if the field/extension can be considered initialized.
  """
  # If the field is required and is not set, it isn't initialized.
  if field.label == _FieldDescriptor.LABEL_REQUIRED:
    if not _HasFieldOrExtension(message, field):
      return False

  # If the field is optional and is not set, or if it
  # isn't a submessage then the field is initialized.
  if field.label == _FieldDescriptor.LABEL_OPTIONAL:
    if not _HasFieldOrExtension(message, field):
      return True
  if field.cpp_type != _FieldDescriptor.CPPTYPE_MESSAGE:
    return True

  # The field is set and is either a single or a repeated submessage.
  messages = _FieldOrExtensionValues(message, field)
  # If all submessages in this field are initialized, the field is
  # considered initialized.
  for message in messages:
    if not message.IsInitialized():
      return False
  return True


def _AddMergeFromStringMethod(message_descriptor, cls):
  """Helper for _AddMessageMethods()."""
  Decoder = decoder.Decoder
  def MergeFromString(self, serialized):
    decoder = Decoder(serialized)
    byte_count = 0
    while not decoder.EndOfStream():
      bytes_read = _DeserializeOneEntity(message_descriptor, self, decoder)
      if not bytes_read:
        break
      byte_count += bytes_read
    return byte_count
  cls.MergeFromString = MergeFromString


def _AddIsInitializedMethod(message_descriptor, cls):
  """Adds the IsInitialized method to the protocol message class."""
  def IsInitialized(self):
    fields_and_extensions = []
    fields_and_extensions.extend(message_descriptor.fields)
    fields_and_extensions.extend(
        self.Extensions._AllExtensionsByNumber().values())
    for field_or_extension in fields_and_extensions:
      if not _IsFieldOrExtensionInitialized(self, field_or_extension):
        return False
    return True
  cls.IsInitialized = IsInitialized


def _AddMessageMethods(message_descriptor, cls):
  """Adds implementations of all Message methods to cls."""

  # TODO(robinson): Add support for remaining Message methods.

  _AddListFieldsMethod(message_descriptor, cls)
  _AddHasFieldMethod(cls)
  _AddClearFieldMethod(cls)
  _AddClearExtensionMethod(cls)
  _AddClearMethod(cls)
  _AddHasExtensionMethod(cls)
  _AddEqualsMethod(message_descriptor, cls)
  _AddSetListenerMethod(cls)
  _AddByteSizeMethod(message_descriptor, cls)
  _AddSerializeToStringMethod(message_descriptor, cls)
  _AddMergeFromStringMethod(message_descriptor, cls)
  _AddIsInitializedMethod(message_descriptor, cls)


def _AddPrivateHelperMethods(cls):
  """Adds implementation of private helper methods to cls."""

  def MaybeCallTransitionToNonemptyCallback(self):
    """Calls self._listener.TransitionToNonempty() the first time this
    method is called.  On all subsequent calls, this is a no-op.
    """
    if not self._called_transition_to_nonempty:
      self._listener.TransitionToNonempty()
      self._called_transition_to_nonempty = True
  cls._MaybeCallTransitionToNonemptyCallback = (
      MaybeCallTransitionToNonemptyCallback)

  def MarkByteSizeDirty(self):
    """Sets the _cached_byte_size_dirty bit to true,
    and propagates this to our listener iff this was a state change.
    """
    if not self._cached_byte_size_dirty:
      self._cached_byte_size_dirty = True
      self._listener.ByteSizeDirty()
  cls._MarkByteSizeDirty = MarkByteSizeDirty


class _Listener(object):

  """MessageListener implementation that a parent message registers with its
  child message.

  In order to support semantics like:

    foo.bar.baz = 23
    assert foo.HasField('bar')

  ...child objects must have back references to their parents.
  This helper class is at the heart of this support.
  """

  def __init__(self, parent_message, has_field_name):
    """Args:
      parent_message: The message whose _MaybeCallTransitionToNonemptyCallback()
        and _MarkByteSizeDirty() methods we should call when we receive
        TransitionToNonempty() and ByteSizeDirty() messages.
      has_field_name: The name of the "has" field that we should set in
        the parent message when we receive a TransitionToNonempty message,
        or None if there's no "has" field to set.  (This will be the case
        for child objects in "repeated" fields).
    """
    # This listener establishes a back reference from a child (contained) object
    # to its parent (containing) object.  We make this a weak reference to avoid
    # creating cyclic garbage when the client finishes with the 'parent' object
    # in the tree.
    if isinstance(parent_message, weakref.ProxyType):
      self._parent_message_weakref = parent_message
    else:
      self._parent_message_weakref = weakref.proxy(parent_message)
    self._has_field_name = has_field_name

  def TransitionToNonempty(self):
    try:
      if self._has_field_name is not None:
        setattr(self._parent_message_weakref, self._has_field_name, True)
      # Propagate the signal to our parents iff this is the first field set.
      self._parent_message_weakref._MaybeCallTransitionToNonemptyCallback()
    except ReferenceError:
      # We can get here if a client has kept a reference to a child object,
      # and is now setting a field on it, but the child's parent has been
      # garbage-collected.  This is not an error.
      pass

  def ByteSizeDirty(self):
    try:
      self._parent_message_weakref._MarkByteSizeDirty()
    except ReferenceError:
      # Same as above.
      pass


# TODO(robinson): Move elsewhere?
# TODO(robinson): Provide a clear() method here in addition to ClearField()?
class _RepeatedScalarFieldContainer(object):

  """Simple, type-checked, list-like container for holding repeated scalars.
  """

  def __init__(self, message_listener, type_checker):
    """
    Args:
      message_listener: A MessageListener implementation.
        The _RepeatedScalarFieldContaininer will call this object's
        TransitionToNonempty() method when it transitions from being empty to
        being nonempty.
      type_checker: A _ValueChecker instance to run on elements inserted
        into this container.
    """
    self._message_listener = message_listener
    self._type_checker = type_checker
    self._values = []

  def append(self, elem):
    self._type_checker.CheckValue(elem)
    self._values.append(elem)
    self._message_listener.ByteSizeDirty()
    if len(self._values) == 1:
      self._message_listener.TransitionToNonempty()

  # List-like __getitem__() support also makes us iterable (via "iter(foo)"
  # or implicitly via "for i in mylist:") for free.
  def __getitem__(self, key):
    return self._values[key]

  def __setitem__(self, key, value):
    # No need to call TransitionToNonempty(), since if we're able to
    # set the element at this index, we were already nonempty before
    # this method was called.
    self._message_listener.ByteSizeDirty()
    self._type_checker.CheckValue(value)
    self._values[key] = value

  def __len__(self):
    return len(self._values)

  def __eq__(self, other):
    if self is other:
      return True
    # Special case for the same type which should be common and fast.
    if isinstance(other, self.__class__):
      return other._values == self._values
    # We are presumably comparing against some other sequence type.
    return other == self._values

  def __ne__(self, other):
    # Can't use != here since it would infinitely recurse.
    return not self == other


# TODO(robinson): Move elsewhere?
# TODO(robinson): Provide a clear() method here in addition to ClearField()?
# TODO(robinson): Unify common functionality with
# _RepeatedScalarFieldContaininer?
class _RepeatedCompositeFieldContainer(object):

  """Simple, list-like container for holding repeated composite fields.
  """

  def __init__(self, message_listener, message_descriptor):
    """Note that we pass in a descriptor instead of the generated directly,
    since at the time we construct a _RepeatedCompositeFieldContainer we
    haven't yet necessarily initialized the type that will be contained in the
    container.

    Args:
      message_listener: A MessageListener implementation.
        The _RepeatedCompositeFieldContainer will call this object's
        TransitionToNonempty() method when it transitions from being empty to
        being nonempty.
      message_descriptor: A Descriptor instance describing the protocol type
        that should be present in this container.  We'll use the
        _concrete_class field of this descriptor when the client calls add().
    """
    self._message_listener = message_listener
    self._message_descriptor = message_descriptor
    self._values = []

  def add(self):
    new_element = self._message_descriptor._concrete_class()
    new_element._SetListener(self._message_listener)
    self._values.append(new_element)
    self._message_listener.ByteSizeDirty()
    self._message_listener.TransitionToNonempty()
    return new_element

  # List-like __getitem__() support also makes us iterable (via "iter(foo)"
  # or implicitly via "for i in mylist:") for free.
  def __getitem__(self, key):
    return self._values[key]

  def __len__(self):
    return len(self._values)

  def __eq__(self, other):
    if self is other:
      return True
    if not isinstance(other, self.__class__):
      raise TypeError('Can only compare repeated composite fields against '
                      'other repeated composite fields.')
    return self._values == other._values

  def __ne__(self, other):
    # Can't use != here since it would infinitely recurse.
    return not self == other

  # TODO(robinson): Implement, document, and test slicing support.


# TODO(robinson): Move elsewhere?  This file is getting pretty ridiculous...
# TODO(robinson): Unify error handling of "unknown extension" crap.
# TODO(robinson): There's so much similarity between the way that
# extensions behave and the way that normal fields behave that it would
# be really nice to unify more code.  It's not immediately obvious
# how to do this, though, and I'd rather get the full functionality
# implemented (and, crucially, get all the tests and specs fleshed out
# and passing), and then come back to this thorny unification problem.
# TODO(robinson): Support iteritems()-style iteration over all
# extensions with the "has" bits turned on?
class _ExtensionDict(object):

  """Dict-like container for supporting an indexable "Extensions"
  field on proto instances.

  Note that in all cases we expect extension handles to be
  FieldDescriptors.
  """

  class _ExtensionListener(object):

    """Adapts an _ExtensionDict to behave as a MessageListener."""

    def __init__(self, extension_dict, handle_id):
      self._extension_dict = extension_dict
      self._handle_id = handle_id

    def TransitionToNonempty(self):
      self._extension_dict._SubmessageTransitionedToNonempty(self._handle_id)

    def ByteSizeDirty(self):
      self._extension_dict._SubmessageByteSizeBecameDirty()

  # TODO(robinson): Somewhere, we need to blow up if people
  # try to register two extensions with the same field number.
  # (And we need a test for this of course).

  def __init__(self, extended_message, known_extensions):
    """extended_message: Message instance for which we are the Extensions dict.
      known_extensions: Iterable of known extension handles.
        These must be FieldDescriptors.
    """
    # We keep a weak reference to extended_message, since
    # it has a reference to this instance in turn.
    self._extended_message = weakref.proxy(extended_message)
    # We make a deep copy of known_extensions to avoid any
    # thread-safety concerns, since the argument passed in
    # is the global (class-level) dict of known extensions for
    # this type of message, which could be modified at any time
    # via a RegisterExtension() call.
    #
    # This dict maps from handle id to handle (a FieldDescriptor).
    #
    # XXX
    # TODO(robinson): This isn't good enough.  The client could
    # instantiate an object in module A, then afterward import
    # module B and pass the instance to B.Foo().  If B imports
    # an extender of this proto and then tries to use it, B
    # will get a KeyError, even though the extension *is* registered
    # at the time of use.
    # XXX
    self._known_extensions = dict((id(e), e) for e in known_extensions)
    # Read lock around self._values, which may be modified by multiple
    # concurrent readers in the conceptually "const" __getitem__ method.
    # So, we grab this lock in every "read-only" method to ensure
    # that concurrent read access is safe without external locking.
    self._lock = threading.Lock()
    # Maps from extension handle ID to current value of that extension.
    self._values = {}
    # Maps from extension handle ID to a boolean "has" bit, but only
    # for non-repeated extension fields.
    keys = (id for id, extension in self._known_extensions.iteritems()
            if extension.label != _FieldDescriptor.LABEL_REPEATED)
    self._has_bits = dict.fromkeys(keys, False)

  def __getitem__(self, extension_handle):
    """Returns the current value of the given extension handle."""
    # We don't care as much about keeping critical sections short in the
    # extension support, since it's presumably much less of a common case.
    self._lock.acquire()
    try:
      handle_id = id(extension_handle)
      if handle_id not in self._known_extensions:
        raise KeyError('Extension not known to this class')
      if handle_id not in self._values:
        self._AddMissingHandle(extension_handle, handle_id)
      return self._values[handle_id]
    finally:
      self._lock.release()

  def __eq__(self, other):
    # We have to grab read locks since we're accessing _values
    # in a "const" method.  See the comment in the constructor.
    if self is other:
      return True
    self._lock.acquire()
    try:
      other._lock.acquire()
      try:
        if self._has_bits != other._has_bits:
          return False
        # If there's a "has" bit, then only compare values where it is true.
        for k, v in self._values.iteritems():
          if self._has_bits.get(k, False) and v != other._values[k]:
            return False
        return True
      finally:
        other._lock.release()
    finally:
      self._lock.release()

  def __ne__(self, other):
    return not self == other

  # Note that this is only meaningful for non-repeated, scalar extension
  # fields.  Note also that we may have to call
  # MaybeCallTransitionToNonemptyCallback() when we do successfully set a field
  # this way, to set any necssary "has" bits in the ancestors of the extended
  # message.
  def __setitem__(self, extension_handle, value):
    """If extension_handle specifies a non-repeated, scalar extension
    field, sets the value of that field.
    """
    handle_id = id(extension_handle)
    if handle_id not in self._known_extensions:
      raise KeyError('Extension not known to this class')
    field = extension_handle  # Just shorten the name.
    if (field.label == _FieldDescriptor.LABEL_OPTIONAL
        and field.cpp_type != _FieldDescriptor.CPPTYPE_MESSAGE):
      # It's slightly wasteful to lookup the type checker each time,
      # but we expect this to be a vanishingly uncommon case anyway.
      type_checker = _VALUE_CHECKERS[field.cpp_type]
      type_checker.CheckValue(value)
      self._values[handle_id] = value
      self._has_bits[handle_id] = True
      self._extended_message._MarkByteSizeDirty()
      self._extended_message._MaybeCallTransitionToNonemptyCallback()
    else:
      raise TypeError('Extension is repeated and/or a composite type.')

  def _AddMissingHandle(self, extension_handle, handle_id):
    """Helper internal to ExtensionDict."""
    # Special handling for non-repeated message extensions, which (like
    # normal fields of this kind) are initialized lazily.
    # REQUIRES: _lock already held.
    cpp_type = extension_handle.cpp_type
    label = extension_handle.label
    if (cpp_type == _FieldDescriptor.CPPTYPE_MESSAGE
        and label != _FieldDescriptor.LABEL_REPEATED):
      self._AddMissingNonRepeatedCompositeHandle(extension_handle, handle_id)
    else:
      self._values[handle_id] = _DefaultValueForField(
          self._extended_message, extension_handle)

  def _AddMissingNonRepeatedCompositeHandle(self, extension_handle, handle_id):
    """Helper internal to ExtensionDict."""
    # REQUIRES: _lock already held.
    value = extension_handle.message_type._concrete_class()
    value._SetListener(_ExtensionDict._ExtensionListener(self, handle_id))
    self._values[handle_id] = value

  def _SubmessageTransitionedToNonempty(self, handle_id):
    """Called when a submessage with a given handle id first transitions to
    being nonempty.  Called by _ExtensionListener.
    """
    assert handle_id in self._has_bits
    self._has_bits[handle_id] = True
    self._extended_message._MaybeCallTransitionToNonemptyCallback()

  def _SubmessageByteSizeBecameDirty(self):
    """Called whenever a submessage's cached byte size becomes invalid
    (goes from being "clean" to being "dirty").  Called by _ExtensionListener.
    """
    self._extended_message._MarkByteSizeDirty()

  # We may wish to widen the public interface of Message.Extensions
  # to expose some of this private functionality in the future.
  # For now, we make all this functionality module-private and just
  # implement what we need for serialization/deserialization,
  # HasField()/ClearField(), etc.

  def _HasExtension(self, extension_handle):
    """Method for internal use by this module.
    Returns true iff we "have" this extension in the sense of the
    "has" bit being set.
    """
    handle_id = id(extension_handle)
    # Note that this is different from the other checks.
    if handle_id not in self._has_bits:
      raise KeyError('Extension not known to this class, or is repeated field.')
    return self._has_bits[handle_id]

  # Intentionally pretty similar to ClearField() above.
  def _ClearExtension(self, extension_handle):
    """Method for internal use by this module.
    Clears the specified extension, unsetting its "has" bit.
    """
    handle_id = id(extension_handle)
    if handle_id not in self._known_extensions:
      raise KeyError('Extension not known to this class')
    default_value = _DefaultValueForField(self._extended_message,
                                          extension_handle)
    if extension_handle.label == _FieldDescriptor.LABEL_REPEATED:
      self._extended_message._MarkByteSizeDirty()
    else:
      cpp_type = extension_handle.cpp_type
      if cpp_type == _FieldDescriptor.CPPTYPE_MESSAGE:
        if handle_id in self._values:
          # Future modifications to this object shouldn't set any
          # "has" bits here.
          self._values[handle_id]._SetListener(None)
      if self._has_bits[handle_id]:
        self._has_bits[handle_id] = False
        self._extended_message._MarkByteSizeDirty()
    if handle_id in self._values:
      del self._values[handle_id]

  def _ListSetExtensions(self):
    """Method for internal use by this module.

    Returns an sequence of all extensions that are currently "set"
    in this extension dict.  A "set" extension is a repeated extension,
    or a non-repeated extension with its "has" bit set.

    The returned sequence contains (field_descriptor, value) pairs,
    where value is the current value of the extension with the given
    field descriptor.

    The sequence values are in arbitrary order.
    """
    self._lock.acquire()  # Read-only methods must lock around self._values.
    try:
      set_extensions = []
      for handle_id, value in self._values.iteritems():
        handle = self._known_extensions[handle_id]
        if (handle.label == _FieldDescriptor.LABEL_REPEATED
            or self._has_bits[handle_id]):
          set_extensions.append((handle, value))
      return set_extensions
    finally:
      self._lock.release()

  def _AllExtensionsByNumber(self):
    """Method for internal use by this module.

    Returns: A dict mapping field_number to (handle, field_descriptor),
      for *all* registered extensions for this dict.
    """
    # TODO(robinson): Precompute and store this away.  Note that we'll have to
    # be careful when we move away from having _known_extensions as a
    # deep-copied member of this object.
    return dict((f.number, f) for f in self._known_extensions.itervalues())


# None of the typecheckers below make any attempt to guard against people
# subclassing builtin types and doing weird things.  We're not trying to
# protect against malicious clients here, just people accidentally shooting
# themselves in the foot in obvious ways.

class _TypeChecker(object):

  """Type checker used to catch type errors as early as possible
  when the client is setting scalar fields in protocol messages.
  """

  def __init__(self, *acceptable_types):
    self._acceptable_types = acceptable_types

  def CheckValue(self, proposed_value):
    if not isinstance(proposed_value, self._acceptable_types):
      message = ('%.1024r has type %s, but expected one of: %s' %
                 (proposed_value, type(proposed_value), self._acceptable_types))
      raise TypeError(message)


# _IntValueChecker and its subclasses perform integer type-checks
# and bounds-checks.
class _IntValueChecker(object):

  """Checker used for integer fields.  Performs type-check and range check."""

  def CheckValue(self, proposed_value):
    if not isinstance(proposed_value, (int, long)):
      message = ('%.1024r has type %s, but expected one of: %s' %
                 (proposed_value, type(proposed_value), (int, long)))
      raise TypeError(message)
    if not self._MIN <= proposed_value <= self._MAX:
      raise ValueError('Value out of range: %d' % proposed_value)

class _Int32ValueChecker(_IntValueChecker):
  # We're sure to use ints instead of longs here since comparison may be more
  # efficient.
  _MIN = -2147483648
  _MAX = 2147483647

class _Uint32ValueChecker(_IntValueChecker):
  _MIN = 0
  _MAX = (1 << 32) - 1

class _Int64ValueChecker(_IntValueChecker):
  _MIN = -(1 << 63)
  _MAX = (1 << 63) - 1

class _Uint64ValueChecker(_IntValueChecker):
  _MIN = 0
  _MAX = (1 << 64) - 1


# Type-checkers for all scalar CPPTYPEs.
_VALUE_CHECKERS = {
    _FieldDescriptor.CPPTYPE_INT32: _Int32ValueChecker(),
    _FieldDescriptor.CPPTYPE_INT64: _Int64ValueChecker(),
    _FieldDescriptor.CPPTYPE_UINT32: _Uint32ValueChecker(),
    _FieldDescriptor.CPPTYPE_UINT64: _Uint64ValueChecker(),
    _FieldDescriptor.CPPTYPE_DOUBLE: _TypeChecker(
        float, int, long),
    _FieldDescriptor.CPPTYPE_FLOAT: _TypeChecker(
        float, int, long),
    _FieldDescriptor.CPPTYPE_BOOL: _TypeChecker(bool, int),
    _FieldDescriptor.CPPTYPE_ENUM: _Int32ValueChecker(),
    _FieldDescriptor.CPPTYPE_STRING: _TypeChecker(str),
    }


# Map from field type to a function F, such that F(field_num, value)
# gives the total byte size for a value of the given type.  This
# byte size includes tag information and any other additional space
# associated with serializing "value".
_TYPE_TO_BYTE_SIZE_FN = {
    _FieldDescriptor.TYPE_DOUBLE: wire_format.DoubleByteSize,
    _FieldDescriptor.TYPE_FLOAT: wire_format.FloatByteSize,
    _FieldDescriptor.TYPE_INT64: wire_format.Int64ByteSize,
    _FieldDescriptor.TYPE_UINT64: wire_format.UInt64ByteSize,
    _FieldDescriptor.TYPE_INT32: wire_format.Int32ByteSize,
    _FieldDescriptor.TYPE_FIXED64: wire_format.Fixed64ByteSize,
    _FieldDescriptor.TYPE_FIXED32: wire_format.Fixed32ByteSize,
    _FieldDescriptor.TYPE_BOOL: wire_format.BoolByteSize,
    _FieldDescriptor.TYPE_STRING: wire_format.StringByteSize,
    _FieldDescriptor.TYPE_GROUP: wire_format.GroupByteSize,
    _FieldDescriptor.TYPE_MESSAGE: wire_format.MessageByteSize,
    _FieldDescriptor.TYPE_BYTES: wire_format.BytesByteSize,
    _FieldDescriptor.TYPE_UINT32: wire_format.UInt32ByteSize,
    _FieldDescriptor.TYPE_ENUM: wire_format.EnumByteSize,
    _FieldDescriptor.TYPE_SFIXED32: wire_format.SFixed32ByteSize,
    _FieldDescriptor.TYPE_SFIXED64: wire_format.SFixed64ByteSize,
    _FieldDescriptor.TYPE_SINT32: wire_format.SInt32ByteSize,
    _FieldDescriptor.TYPE_SINT64: wire_format.SInt64ByteSize
    }

# Maps from field type to an unbound Encoder method F, such that
# F(encoder, field_number, value) will append the serialization
# of a value of this type to the encoder.
_Encoder = encoder.Encoder
_TYPE_TO_SERIALIZE_METHOD = {
    _FieldDescriptor.TYPE_DOUBLE: _Encoder.AppendDouble,
    _FieldDescriptor.TYPE_FLOAT: _Encoder.AppendFloat,
    _FieldDescriptor.TYPE_INT64: _Encoder.AppendInt64,
    _FieldDescriptor.TYPE_UINT64: _Encoder.AppendUInt64,
    _FieldDescriptor.TYPE_INT32: _Encoder.AppendInt32,
    _FieldDescriptor.TYPE_FIXED64: _Encoder.AppendFixed64,
    _FieldDescriptor.TYPE_FIXED32: _Encoder.AppendFixed32,
    _FieldDescriptor.TYPE_BOOL: _Encoder.AppendBool,
    _FieldDescriptor.TYPE_STRING: _Encoder.AppendString,
    _FieldDescriptor.TYPE_GROUP: _Encoder.AppendGroup,
    _FieldDescriptor.TYPE_MESSAGE: _Encoder.AppendMessage,
    _FieldDescriptor.TYPE_BYTES: _Encoder.AppendBytes,
    _FieldDescriptor.TYPE_UINT32: _Encoder.AppendUInt32,
    _FieldDescriptor.TYPE_ENUM: _Encoder.AppendEnum,
    _FieldDescriptor.TYPE_SFIXED32: _Encoder.AppendSFixed32,
    _FieldDescriptor.TYPE_SFIXED64: _Encoder.AppendSFixed64,
    _FieldDescriptor.TYPE_SINT32: _Encoder.AppendSInt32,
    _FieldDescriptor.TYPE_SINT64: _Encoder.AppendSInt64,
    }

# Maps from field type to expected wiretype.
_FIELD_TYPE_TO_WIRE_TYPE = {
    _FieldDescriptor.TYPE_DOUBLE: wire_format.WIRETYPE_FIXED64,
    _FieldDescriptor.TYPE_FLOAT: wire_format.WIRETYPE_FIXED32,
    _FieldDescriptor.TYPE_INT64: wire_format.WIRETYPE_VARINT,
    _FieldDescriptor.TYPE_UINT64: wire_format.WIRETYPE_VARINT,
    _FieldDescriptor.TYPE_INT32: wire_format.WIRETYPE_VARINT,
    _FieldDescriptor.TYPE_FIXED64: wire_format.WIRETYPE_FIXED64,
    _FieldDescriptor.TYPE_FIXED32: wire_format.WIRETYPE_FIXED32,
    _FieldDescriptor.TYPE_BOOL: wire_format.WIRETYPE_VARINT,
    _FieldDescriptor.TYPE_STRING:
      wire_format.WIRETYPE_LENGTH_DELIMITED,
    _FieldDescriptor.TYPE_GROUP: wire_format.WIRETYPE_START_GROUP,
    _FieldDescriptor.TYPE_MESSAGE:
      wire_format.WIRETYPE_LENGTH_DELIMITED,
    _FieldDescriptor.TYPE_BYTES:
      wire_format.WIRETYPE_LENGTH_DELIMITED,
    _FieldDescriptor.TYPE_UINT32: wire_format.WIRETYPE_VARINT,
    _FieldDescriptor.TYPE_ENUM: wire_format.WIRETYPE_VARINT,
    _FieldDescriptor.TYPE_SFIXED32: wire_format.WIRETYPE_FIXED32,
    _FieldDescriptor.TYPE_SFIXED64: wire_format.WIRETYPE_FIXED64,
    _FieldDescriptor.TYPE_SINT32: wire_format.WIRETYPE_VARINT,
    _FieldDescriptor.TYPE_SINT64: wire_format.WIRETYPE_VARINT,
    }

# Maps from field type to an unbound Decoder method F,
# such that F(decoder) will read a field of the requested type.
#
# Note that Message and Group are intentionally missing here.
# They're handled by _RecursivelyMerge().
_Decoder = decoder.Decoder
_TYPE_TO_DESERIALIZE_METHOD = {
    _FieldDescriptor.TYPE_DOUBLE: _Decoder.ReadDouble,
    _FieldDescriptor.TYPE_FLOAT: _Decoder.ReadFloat,
    _FieldDescriptor.TYPE_INT64: _Decoder.ReadInt64,
    _FieldDescriptor.TYPE_UINT64: _Decoder.ReadUInt64,
    _FieldDescriptor.TYPE_INT32: _Decoder.ReadInt32,
    _FieldDescriptor.TYPE_FIXED64: _Decoder.ReadFixed64,
    _FieldDescriptor.TYPE_FIXED32: _Decoder.ReadFixed32,
    _FieldDescriptor.TYPE_BOOL: _Decoder.ReadBool,
    _FieldDescriptor.TYPE_STRING: _Decoder.ReadString,
    _FieldDescriptor.TYPE_BYTES: _Decoder.ReadBytes,
    _FieldDescriptor.TYPE_UINT32: _Decoder.ReadUInt32,
    _FieldDescriptor.TYPE_ENUM: _Decoder.ReadEnum,
    _FieldDescriptor.TYPE_SFIXED32: _Decoder.ReadSFixed32,
    _FieldDescriptor.TYPE_SFIXED64: _Decoder.ReadSFixed64,
    _FieldDescriptor.TYPE_SINT32: _Decoder.ReadSInt32,
    _FieldDescriptor.TYPE_SINT64: _Decoder.ReadSInt64,
    }
