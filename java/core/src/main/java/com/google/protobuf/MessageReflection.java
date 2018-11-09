// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

package com.google.protobuf;

import com.google.protobuf.Descriptors.FieldDescriptor;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

/**
 * Reflection utility methods shared by both mutable and immutable messages.
 *
 * @author liujisi@google.com (Pherl Liu)
 */
class MessageReflection {

  static void writeMessageTo(
      Message message,
      Map<FieldDescriptor, Object> fields,
      CodedOutputStream output,
      boolean alwaysWriteRequiredFields)
      throws IOException {
    final boolean isMessageSet =
        message.getDescriptorForType().getOptions().getMessageSetWireFormat();
    if (alwaysWriteRequiredFields) {
      fields = new TreeMap<FieldDescriptor, Object>(fields);
      for (final FieldDescriptor field : message.getDescriptorForType().getFields()) {
        if (field.isRequired() && !fields.containsKey(field)) {
          fields.put(field, message.getField(field));
        }
      }
    }
    for (final Map.Entry<Descriptors.FieldDescriptor, Object> entry : fields.entrySet()) {
      final Descriptors.FieldDescriptor field = entry.getKey();
      final Object value = entry.getValue();
      if (isMessageSet
          && field.isExtension()
          && field.getType() == Descriptors.FieldDescriptor.Type.MESSAGE
          && !field.isRepeated()) {
        output.writeMessageSetExtension(field.getNumber(), (Message) value);
      } else {
        FieldSet.writeField(field, value, output);
      }
    }

    final UnknownFieldSet unknownFields = message.getUnknownFields();
    if (isMessageSet) {
      unknownFields.writeAsMessageSetTo(output);
    } else {
      unknownFields.writeTo(output);
    }
  }

  static int getSerializedSize(Message message, Map<FieldDescriptor, Object> fields) {
    int size = 0;
    final boolean isMessageSet =
        message.getDescriptorForType().getOptions().getMessageSetWireFormat();

    for (final Map.Entry<Descriptors.FieldDescriptor, Object> entry : fields.entrySet()) {
      final Descriptors.FieldDescriptor field = entry.getKey();
      final Object value = entry.getValue();
      if (isMessageSet
          && field.isExtension()
          && field.getType() == Descriptors.FieldDescriptor.Type.MESSAGE
          && !field.isRepeated()) {
        size +=
            CodedOutputStream.computeMessageSetExtensionSize(field.getNumber(), (Message) value);
      } else {
        size += FieldSet.computeFieldSize(field, value);
      }
    }

    final UnknownFieldSet unknownFields = message.getUnknownFields();
    if (isMessageSet) {
      size += unknownFields.getSerializedSizeAsMessageSet();
    } else {
      size += unknownFields.getSerializedSize();
    }
    return size;
  }

  static String delimitWithCommas(List<String> parts) {
    StringBuilder result = new StringBuilder();
    for (String part : parts) {
      if (result.length() > 0) {
        result.append(", ");
      }
      result.append(part);
    }
    return result.toString();
  }

  @SuppressWarnings("unchecked")
  static boolean isInitialized(MessageOrBuilder message) {
    // Check that all required fields are present.
    for (final Descriptors.FieldDescriptor field : message.getDescriptorForType().getFields()) {
      if (field.isRequired()) {
        if (!message.hasField(field)) {
          return false;
        }
      }
    }

    // Check that embedded messages are initialized.
    for (final Map.Entry<Descriptors.FieldDescriptor, Object> entry :
        message.getAllFields().entrySet()) {
      final Descriptors.FieldDescriptor field = entry.getKey();
      if (field.getJavaType() == Descriptors.FieldDescriptor.JavaType.MESSAGE) {
        if (field.isRepeated()) {
          for (final Message element : (List<Message>) entry.getValue()) {
            if (!element.isInitialized()) {
              return false;
            }
          }
        } else {
          if (!((Message) entry.getValue()).isInitialized()) {
            return false;
          }
        }
      }
    }

    return true;
  }

  private static String subMessagePrefix(
      final String prefix, final Descriptors.FieldDescriptor field, final int index) {
    final StringBuilder result = new StringBuilder(prefix);
    if (field.isExtension()) {
      result.append('(').append(field.getFullName()).append(')');
    } else {
      result.append(field.getName());
    }
    if (index != -1) {
      result.append('[').append(index).append(']');
    }
    result.append('.');
    return result.toString();
  }

  private static void findMissingFields(
      final MessageOrBuilder message, final String prefix, final List<String> results) {
    for (final Descriptors.FieldDescriptor field : message.getDescriptorForType().getFields()) {
      if (field.isRequired() && !message.hasField(field)) {
        results.add(prefix + field.getName());
      }
    }

    for (final Map.Entry<Descriptors.FieldDescriptor, Object> entry :
        message.getAllFields().entrySet()) {
      final Descriptors.FieldDescriptor field = entry.getKey();
      final Object value = entry.getValue();

      if (field.getJavaType() == Descriptors.FieldDescriptor.JavaType.MESSAGE) {
        if (field.isRepeated()) {
          int i = 0;
          for (final Object element : (List) value) {
            findMissingFields(
                (MessageOrBuilder) element, subMessagePrefix(prefix, field, i++), results);
          }
        } else {
          if (message.hasField(field)) {
            findMissingFields(
                (MessageOrBuilder) value, subMessagePrefix(prefix, field, -1), results);
          }
        }
      }
    }
  }

  /**
   * Populates {@code this.missingFields} with the full "path" of each missing required field in the
   * given message.
   */
  static List<String> findMissingFields(final MessageOrBuilder message) {
    final List<String> results = new ArrayList<String>();
    findMissingFields(message, "", results);
    return results;
  }

  static interface MergeTarget {
    enum ContainerType {
      MESSAGE,
      EXTENSION_SET
    }

    /** Returns the descriptor for the target. */
    public Descriptors.Descriptor getDescriptorForType();

    public ContainerType getContainerType();

    public ExtensionRegistry.ExtensionInfo findExtensionByName(
        ExtensionRegistry registry, String name);

    public ExtensionRegistry.ExtensionInfo findExtensionByNumber(
        ExtensionRegistry registry, Descriptors.Descriptor containingType, int fieldNumber);

    /**
     * Obtains the value of the given field, or the default value if it is not set. For primitive
     * fields, the boxed primitive value is returned. For enum fields, the EnumValueDescriptor for
     * the value is returned. For embedded message fields, the sub-message is returned. For repeated
     * fields, a java.util.List is returned.
     */
    public Object getField(Descriptors.FieldDescriptor field);

    /**
     * Returns true if the given field is set. This is exactly equivalent to calling the generated
     * "has" accessor method corresponding to the field.
     *
     * @throws IllegalArgumentException The field is a repeated field, or {@code
     *     field.getContainingType() != getDescriptorForType()}.
     */
    boolean hasField(Descriptors.FieldDescriptor field);

    /**
     * Sets a field to the given value. The value must be of the correct type for this field, i.e.
     * the same type that {@link Message#getField(Descriptors.FieldDescriptor)} would return.
     */
    MergeTarget setField(Descriptors.FieldDescriptor field, Object value);

    /**
     * Clears the field. This is exactly equivalent to calling the generated "clear" accessor method
     * corresponding to the field.
     */
    MergeTarget clearField(Descriptors.FieldDescriptor field);

    /**
     * Sets an element of a repeated field to the given value. The value must be of the correct type
     * for this field, i.e. the same type that {@link
     * Message#getRepeatedField(Descriptors.FieldDescriptor, int)} would return.
     *
     * @throws IllegalArgumentException The field is not a repeated field, or {@code
     *     field.getContainingType() != getDescriptorForType()}.
     */
    MergeTarget setRepeatedField(Descriptors.FieldDescriptor field, int index, Object value);

    /**
     * Like {@code setRepeatedField}, but appends the value as a new element.
     *
     * @throws IllegalArgumentException The field is not a repeated field, or {@code
     *     field.getContainingType() != getDescriptorForType()}.
     */
    MergeTarget addRepeatedField(Descriptors.FieldDescriptor field, Object value);

    /**
     * Returns true if the given oneof is set.
     *
     * @throws IllegalArgumentException if {@code oneof.getContainingType() !=
     *     getDescriptorForType()}.
     */
    boolean hasOneof(Descriptors.OneofDescriptor oneof);

    /**
     * Clears the oneof. This is exactly equivalent to calling the generated "clear" accessor method
     * corresponding to the oneof.
     */
    MergeTarget clearOneof(Descriptors.OneofDescriptor oneof);

    /** Obtains the FieldDescriptor if the given oneof is set. Returns null if no field is set. */
    Descriptors.FieldDescriptor getOneofFieldDescriptor(Descriptors.OneofDescriptor oneof);

    /**
     * Parse the input stream into a sub field group defined based on either FieldDescriptor or the
     * default instance.
     */
    Object parseGroup(
        CodedInputStream input,
        ExtensionRegistryLite registry,
        Descriptors.FieldDescriptor descriptor,
        Message defaultInstance)
        throws IOException;

    /**
     * Parse the input stream into a sub field message defined based on either FieldDescriptor or
     * the default instance.
     */
    Object parseMessage(
        CodedInputStream input,
        ExtensionRegistryLite registry,
        Descriptors.FieldDescriptor descriptor,
        Message defaultInstance)
        throws IOException;

    /**
     * Parse from a ByteString into a sub field message defined based on either FieldDescriptor or
     * the default instance. There isn't a varint indicating the length of the message at the
     * beginning of the input ByteString.
     */
    Object parseMessageFromBytes(
        ByteString bytes,
        ExtensionRegistryLite registry,
        Descriptors.FieldDescriptor descriptor,
        Message defaultInstance)
        throws IOException;

    /** Returns the UTF8 validation level for the field. */
    WireFormat.Utf8Validation getUtf8Validation(Descriptors.FieldDescriptor descriptor);

    /**
     * Returns a new merge target for a sub-field. When defaultInstance is provided, it indicates
     * the descriptor is for an extension type, and implementations should create a new instance
     * from the defaultInstance prototype directly.
     */
    MergeTarget newMergeTargetForField(
        Descriptors.FieldDescriptor descriptor, Message defaultInstance);

    /**
     * Returns an empty merge target for a sub-field. When defaultInstance is provided, it indicates
     * the descriptor is for an extension type, and implementations should create a new instance
     * from the defaultInstance prototype directly.
     */
    MergeTarget newEmptyTargetForField(
        Descriptors.FieldDescriptor descriptor, Message defaultInstance);

    /** Finishes the merge and returns the underlying object. */
    Object finish();
  }

  static class BuilderAdapter implements MergeTarget {

    private final Message.Builder builder;

    @Override
    public Descriptors.Descriptor getDescriptorForType() {
      return builder.getDescriptorForType();
    }

    public BuilderAdapter(Message.Builder builder) {
      this.builder = builder;
    }

    @Override
    public Object getField(Descriptors.FieldDescriptor field) {
      return builder.getField(field);
    }

    @Override
    public boolean hasField(Descriptors.FieldDescriptor field) {
      return builder.hasField(field);
    }

    @Override
    public MergeTarget setField(Descriptors.FieldDescriptor field, Object value) {
      builder.setField(field, value);
      return this;
    }

    @Override
    public MergeTarget clearField(Descriptors.FieldDescriptor field) {
      builder.clearField(field);
      return this;
    }

    @Override
    public MergeTarget setRepeatedField(
        Descriptors.FieldDescriptor field, int index, Object value) {
      builder.setRepeatedField(field, index, value);
      return this;
    }

    @Override
    public MergeTarget addRepeatedField(Descriptors.FieldDescriptor field, Object value) {
      builder.addRepeatedField(field, value);
      return this;
    }

    @Override
    public boolean hasOneof(Descriptors.OneofDescriptor oneof) {
      return builder.hasOneof(oneof);
    }

    @Override
    public MergeTarget clearOneof(Descriptors.OneofDescriptor oneof) {
      builder.clearOneof(oneof);
      return this;
    }

    @Override
    public Descriptors.FieldDescriptor getOneofFieldDescriptor(Descriptors.OneofDescriptor oneof) {
      return builder.getOneofFieldDescriptor(oneof);
    }

    @Override
    public ContainerType getContainerType() {
      return ContainerType.MESSAGE;
    }

    @Override
    public ExtensionRegistry.ExtensionInfo findExtensionByName(
        ExtensionRegistry registry, String name) {
      return registry.findImmutableExtensionByName(name);
    }

    @Override
    public ExtensionRegistry.ExtensionInfo findExtensionByNumber(
        ExtensionRegistry registry, Descriptors.Descriptor containingType, int fieldNumber) {
      return registry.findImmutableExtensionByNumber(containingType, fieldNumber);
    }

    @Override
    public Object parseGroup(
        CodedInputStream input,
        ExtensionRegistryLite extensionRegistry,
        Descriptors.FieldDescriptor field,
        Message defaultInstance)
        throws IOException {
      Message.Builder subBuilder;
      // When default instance is not null. The field is an extension field.
      if (defaultInstance != null) {
        subBuilder = defaultInstance.newBuilderForType();
      } else {
        subBuilder = builder.newBuilderForField(field);
      }
      if (!field.isRepeated()) {
        Message originalMessage = (Message) getField(field);
        if (originalMessage != null) {
          subBuilder.mergeFrom(originalMessage);
        }
      }
      input.readGroup(field.getNumber(), subBuilder, extensionRegistry);
      return subBuilder.buildPartial();
    }

    @Override
    public Object parseMessage(
        CodedInputStream input,
        ExtensionRegistryLite extensionRegistry,
        Descriptors.FieldDescriptor field,
        Message defaultInstance)
        throws IOException {
      Message.Builder subBuilder;
      // When default instance is not null. The field is an extension field.
      if (defaultInstance != null) {
        subBuilder = defaultInstance.newBuilderForType();
      } else {
        subBuilder = builder.newBuilderForField(field);
      }
      if (!field.isRepeated()) {
        Message originalMessage = (Message) getField(field);
        if (originalMessage != null) {
          subBuilder.mergeFrom(originalMessage);
        }
      }
      input.readMessage(subBuilder, extensionRegistry);
      return subBuilder.buildPartial();
    }

    @Override
    public Object parseMessageFromBytes(
        ByteString bytes,
        ExtensionRegistryLite extensionRegistry,
        Descriptors.FieldDescriptor field,
        Message defaultInstance)
        throws IOException {
      Message.Builder subBuilder;
      // When default instance is not null. The field is an extension field.
      if (defaultInstance != null) {
        subBuilder = defaultInstance.newBuilderForType();
      } else {
        subBuilder = builder.newBuilderForField(field);
      }
      if (!field.isRepeated()) {
        Message originalMessage = (Message) getField(field);
        if (originalMessage != null) {
          subBuilder.mergeFrom(originalMessage);
        }
      }
      subBuilder.mergeFrom(bytes, extensionRegistry);
      return subBuilder.buildPartial();
    }

    @Override
    public MergeTarget newMergeTargetForField(
        Descriptors.FieldDescriptor field, Message defaultInstance) {
      Message.Builder subBuilder;
      if (defaultInstance != null) {
        subBuilder = defaultInstance.newBuilderForType();
      } else {
        subBuilder = builder.newBuilderForField(field);
      }
      if (!field.isRepeated()) {
        Message originalMessage = (Message) getField(field);
        if (originalMessage != null) {
          subBuilder.mergeFrom(originalMessage);
        }
      }
      return new BuilderAdapter(subBuilder);
    }

    @Override
    public MergeTarget newEmptyTargetForField(
        Descriptors.FieldDescriptor field, Message defaultInstance) {
      Message.Builder subBuilder;
      if (defaultInstance != null) {
        subBuilder = defaultInstance.newBuilderForType();
      } else {
        subBuilder = builder.newBuilderForField(field);
      }
      return new BuilderAdapter(subBuilder);
    }

    @Override
    public WireFormat.Utf8Validation getUtf8Validation(Descriptors.FieldDescriptor descriptor) {
      if (descriptor.needsUtf8Check()) {
        return WireFormat.Utf8Validation.STRICT;
      }
      // TODO(liujisi): support lazy strings for repeated fields.
      if (!descriptor.isRepeated() && builder instanceof GeneratedMessage.Builder) {
        return WireFormat.Utf8Validation.LAZY;
      }
      return WireFormat.Utf8Validation.LOOSE;
    }

    @Override
    public Object finish() {
      return builder.buildPartial();
    }
  }


  static class ExtensionAdapter implements MergeTarget {

    private final FieldSet<Descriptors.FieldDescriptor> extensions;

    ExtensionAdapter(FieldSet<Descriptors.FieldDescriptor> extensions) {
      this.extensions = extensions;
    }

    @Override
    public Descriptors.Descriptor getDescriptorForType() {
      throw new UnsupportedOperationException("getDescriptorForType() called on FieldSet object");
    }

    @Override
    public Object getField(Descriptors.FieldDescriptor field) {
      return extensions.getField(field);
    }

    @Override
    public boolean hasField(Descriptors.FieldDescriptor field) {
      return extensions.hasField(field);
    }

    @Override
    public MergeTarget setField(Descriptors.FieldDescriptor field, Object value) {
      extensions.setField(field, value);
      return this;
    }

    @Override
    public MergeTarget clearField(Descriptors.FieldDescriptor field) {
      extensions.clearField(field);
      return this;
    }

    @Override
    public MergeTarget setRepeatedField(
        Descriptors.FieldDescriptor field, int index, Object value) {
      extensions.setRepeatedField(field, index, value);
      return this;
    }

    @Override
    public MergeTarget addRepeatedField(Descriptors.FieldDescriptor field, Object value) {
      extensions.addRepeatedField(field, value);
      return this;
    }

    @Override
    public boolean hasOneof(Descriptors.OneofDescriptor oneof) {
      return false;
    }

    @Override
    public MergeTarget clearOneof(Descriptors.OneofDescriptor oneof) {
      // Nothing to clear.
      return this;
    }

    @Override
    public Descriptors.FieldDescriptor getOneofFieldDescriptor(Descriptors.OneofDescriptor oneof) {
      return null;
    }

    @Override
    public ContainerType getContainerType() {
      return ContainerType.EXTENSION_SET;
    }

    @Override
    public ExtensionRegistry.ExtensionInfo findExtensionByName(
        ExtensionRegistry registry, String name) {
      return registry.findImmutableExtensionByName(name);
    }

    @Override
    public ExtensionRegistry.ExtensionInfo findExtensionByNumber(
        ExtensionRegistry registry, Descriptors.Descriptor containingType, int fieldNumber) {
      return registry.findImmutableExtensionByNumber(containingType, fieldNumber);
    }

    @Override
    public Object parseGroup(
        CodedInputStream input,
        ExtensionRegistryLite registry,
        Descriptors.FieldDescriptor field,
        Message defaultInstance)
        throws IOException {
      Message.Builder subBuilder = defaultInstance.newBuilderForType();
      if (!field.isRepeated()) {
        Message originalMessage = (Message) getField(field);
        if (originalMessage != null) {
          subBuilder.mergeFrom(originalMessage);
        }
      }
      input.readGroup(field.getNumber(), subBuilder, registry);
      return subBuilder.buildPartial();
    }

    @Override
    public Object parseMessage(
        CodedInputStream input,
        ExtensionRegistryLite registry,
        Descriptors.FieldDescriptor field,
        Message defaultInstance)
        throws IOException {
      Message.Builder subBuilder = defaultInstance.newBuilderForType();
      if (!field.isRepeated()) {
        Message originalMessage = (Message) getField(field);
        if (originalMessage != null) {
          subBuilder.mergeFrom(originalMessage);
        }
      }
      input.readMessage(subBuilder, registry);
      return subBuilder.buildPartial();
    }

    @Override
    public Object parseMessageFromBytes(
        ByteString bytes,
        ExtensionRegistryLite registry,
        Descriptors.FieldDescriptor field,
        Message defaultInstance)
        throws IOException {
      Message.Builder subBuilder = defaultInstance.newBuilderForType();
      if (!field.isRepeated()) {
        Message originalMessage = (Message) getField(field);
        if (originalMessage != null) {
          subBuilder.mergeFrom(originalMessage);
        }
      }
      subBuilder.mergeFrom(bytes, registry);
      return subBuilder.buildPartial();
    }

    @Override
    public MergeTarget newMergeTargetForField(
        Descriptors.FieldDescriptor descriptor, Message defaultInstance) {
      throw new UnsupportedOperationException("newMergeTargetForField() called on FieldSet object");
    }

    @Override
    public MergeTarget newEmptyTargetForField(
        Descriptors.FieldDescriptor descriptor, Message defaultInstance) {
      throw new UnsupportedOperationException("newEmptyTargetForField() called on FieldSet object");
    }

    @Override
    public WireFormat.Utf8Validation getUtf8Validation(Descriptors.FieldDescriptor descriptor) {
      if (descriptor.needsUtf8Check()) {
        return WireFormat.Utf8Validation.STRICT;
      }
      // TODO(liujisi): support lazy strings for ExtesnsionSet.
      return WireFormat.Utf8Validation.LOOSE;
    }

    @Override
    public Object finish() {
      throw new UnsupportedOperationException("finish() called on FieldSet object");
    }
  }

  /**
   * Parses a single field into MergeTarget. The target can be Message.Builder, FieldSet or
   * MutableMessage.
   *
   * <p>Package-private because it is used by GeneratedMessage.ExtendableMessage.
   *
   * @param tag The tag, which should have already been read.
   * @param unknownFields If not null, unknown fields will be merged to this {@link
   *     UnknownFieldSet}, otherwise unknown fields will be discarded.
   * @return {@code true} unless the tag is an end-group tag.
   */
  static boolean mergeFieldFrom(
      CodedInputStream input,
      UnknownFieldSet.Builder unknownFields,
      ExtensionRegistryLite extensionRegistry,
      Descriptors.Descriptor type,
      MergeTarget target,
      int tag)
      throws IOException {
    if (type.getOptions().getMessageSetWireFormat() && tag == WireFormat.MESSAGE_SET_ITEM_TAG) {
      mergeMessageSetExtensionFromCodedStream(
          input, unknownFields, extensionRegistry, type, target);
      return true;
    }

    final int wireType = WireFormat.getTagWireType(tag);
    final int fieldNumber = WireFormat.getTagFieldNumber(tag);

    final Descriptors.FieldDescriptor field;
    Message defaultInstance = null;

    if (type.isExtensionNumber(fieldNumber)) {
      // extensionRegistry may be either ExtensionRegistry or
      // ExtensionRegistryLite.  Since the type we are parsing is a full
      // message, only a full ExtensionRegistry could possibly contain
      // extensions of it.  Otherwise we will treat the registry as if it
      // were empty.
      if (extensionRegistry instanceof ExtensionRegistry) {
        final ExtensionRegistry.ExtensionInfo extension =
            target.findExtensionByNumber((ExtensionRegistry) extensionRegistry, type, fieldNumber);
        if (extension == null) {
          field = null;
        } else {
          field = extension.descriptor;
          defaultInstance = extension.defaultInstance;
          if (defaultInstance == null
              && field.getJavaType() == Descriptors.FieldDescriptor.JavaType.MESSAGE) {
            throw new IllegalStateException(
                "Message-typed extension lacked default instance: " + field.getFullName());
          }
        }
      } else {
        field = null;
      }
    } else if (target.getContainerType() == MergeTarget.ContainerType.MESSAGE) {
      field = type.findFieldByNumber(fieldNumber);
    } else {
      field = null;
    }

    boolean unknown = false;
    boolean packed = false;
    if (field == null) {
      unknown = true; // Unknown field.
    } else if (wireType
        == FieldSet.getWireFormatForFieldType(field.getLiteType(), /* isPacked= */ false)) {
      packed = false;
    } else if (field.isPackable()
        && wireType
            == FieldSet.getWireFormatForFieldType(field.getLiteType(), /* isPacked= */ true)) {
      packed = true;
    } else {
      unknown = true; // Unknown wire type.
    }

    if (unknown) { // Unknown field or wrong wire type.  Skip.
      if (unknownFields != null) {
        return unknownFields.mergeFieldFrom(tag, input);
      } else {
        return input.skipField(tag);
      }
    }

    if (packed) {
      final int length = input.readRawVarint32();
      final int limit = input.pushLimit(length);
      if (field.getLiteType() == WireFormat.FieldType.ENUM) {
        while (input.getBytesUntilLimit() > 0) {
          final int rawValue = input.readEnum();
          if (field.getFile().supportsUnknownEnumValue()) {
            target.addRepeatedField(
                field, field.getEnumType().findValueByNumberCreatingIfUnknown(rawValue));
          } else {
            final Object value = field.getEnumType().findValueByNumber(rawValue);
            if (value == null) {
              // If the number isn't recognized as a valid value for this
              // enum, drop it (don't even add it to unknownFields).
              return true;
            }
            target.addRepeatedField(field, value);
          }
        }
      } else {
        while (input.getBytesUntilLimit() > 0) {
          final Object value =
              WireFormat.readPrimitiveField(
                  input, field.getLiteType(), target.getUtf8Validation(field));
          target.addRepeatedField(field, value);
        }
      }
      input.popLimit(limit);
    } else {
      final Object value;
      switch (field.getType()) {
        case GROUP:
          {
            value = target.parseGroup(input, extensionRegistry, field, defaultInstance);
            break;
          }
        case MESSAGE:
          {
            value = target.parseMessage(input, extensionRegistry, field, defaultInstance);
            break;
          }
        case ENUM:
          final int rawValue = input.readEnum();
          if (field.getFile().supportsUnknownEnumValue()) {
            value = field.getEnumType().findValueByNumberCreatingIfUnknown(rawValue);
          } else {
            value = field.getEnumType().findValueByNumber(rawValue);
            // If the number isn't recognized as a valid value for this enum,
            // drop it.
            if (value == null) {
              if (unknownFields != null) {
                unknownFields.mergeVarintField(fieldNumber, rawValue);
              }
              return true;
            }
          }
          break;
        default:
          value =
              WireFormat.readPrimitiveField(
                  input, field.getLiteType(), target.getUtf8Validation(field));
          break;
      }

      if (field.isRepeated()) {
        target.addRepeatedField(field, value);
      } else {
        target.setField(field, value);
      }
    }

    return true;
  }

  /** Called by {@code #mergeFieldFrom()} to parse a MessageSet extension into MergeTarget. */
  private static void mergeMessageSetExtensionFromCodedStream(
      CodedInputStream input,
      UnknownFieldSet.Builder unknownFields,
      ExtensionRegistryLite extensionRegistry,
      Descriptors.Descriptor type,
      MergeTarget target)
      throws IOException {

    // The wire format for MessageSet is:
    //   message MessageSet {
    //     repeated group Item = 1 {
    //       required int32 typeId = 2;
    //       required bytes message = 3;
    //     }
    //   }
    // "typeId" is the extension's field number.  The extension can only be
    // a message type, where "message" contains the encoded bytes of that
    // message.
    //
    // In practice, we will probably never see a MessageSet item in which
    // the message appears before the type ID, or where either field does not
    // appear exactly once.  However, in theory such cases are valid, so we
    // should be prepared to accept them.

    int typeId = 0;
    ByteString rawBytes = null; // If we encounter "message" before "typeId"
    ExtensionRegistry.ExtensionInfo extension = null;

    // Read bytes from input, if we get it's type first then parse it eagerly,
    // otherwise we store the raw bytes in a local variable.
    while (true) {
      final int tag = input.readTag();
      if (tag == 0) {
        break;
      }

      if (tag == WireFormat.MESSAGE_SET_TYPE_ID_TAG) {
        typeId = input.readUInt32();
        if (typeId != 0) {
          // extensionRegistry may be either ExtensionRegistry or
          // ExtensionRegistryLite. Since the type we are parsing is a full
          // message, only a full ExtensionRegistry could possibly contain
          // extensions of it. Otherwise we will treat the registry as if it
          // were empty.
          if (extensionRegistry instanceof ExtensionRegistry) {
            extension =
                target.findExtensionByNumber((ExtensionRegistry) extensionRegistry, type, typeId);
          }
        }

      } else if (tag == WireFormat.MESSAGE_SET_MESSAGE_TAG) {
        if (typeId != 0) {
          if (extension != null && ExtensionRegistryLite.isEagerlyParseMessageSets()) {
            // We already know the type, so we can parse directly from the
            // input with no copying.  Hooray!
            eagerlyMergeMessageSetExtension(input, extension, extensionRegistry, target);
            rawBytes = null;
            continue;
          }
        }
        // We haven't seen a type ID yet or we want parse message lazily.
        rawBytes = input.readBytes();

      } else { // Unknown tag. Skip it.
        if (!input.skipField(tag)) {
          break; // End of group
        }
      }
    }
    input.checkLastTagWas(WireFormat.MESSAGE_SET_ITEM_END_TAG);

    // Process the raw bytes.
    if (rawBytes != null && typeId != 0) { // Zero is not a valid type ID.
      if (extension != null) { // We known the type
        mergeMessageSetExtensionFromBytes(rawBytes, extension, extensionRegistry, target);
      } else { // We don't know how to parse this. Ignore it.
        if (rawBytes != null && unknownFields != null) {
          unknownFields.mergeField(
              typeId, UnknownFieldSet.Field.newBuilder().addLengthDelimited(rawBytes).build());
        }
      }
    }
  }

  private static void mergeMessageSetExtensionFromBytes(
      ByteString rawBytes,
      ExtensionRegistry.ExtensionInfo extension,
      ExtensionRegistryLite extensionRegistry,
      MergeTarget target)
      throws IOException {

    Descriptors.FieldDescriptor field = extension.descriptor;
    boolean hasOriginalValue = target.hasField(field);

    if (hasOriginalValue || ExtensionRegistryLite.isEagerlyParseMessageSets()) {
      // If the field already exists, we just parse the field.
      Object value =
          target.parseMessageFromBytes(
              rawBytes, extensionRegistry, field, extension.defaultInstance);
      target.setField(field, value);
    } else {
      // Use LazyField to load MessageSet lazily.
      LazyField lazyField = new LazyField(extension.defaultInstance, extensionRegistry, rawBytes);
      target.setField(field, lazyField);
    }
  }

  private static void eagerlyMergeMessageSetExtension(
      CodedInputStream input,
      ExtensionRegistry.ExtensionInfo extension,
      ExtensionRegistryLite extensionRegistry,
      MergeTarget target)
      throws IOException {
    Descriptors.FieldDescriptor field = extension.descriptor;
    Object value = target.parseMessage(input, extensionRegistry, field, extension.defaultInstance);
    target.setField(field, value);
  }
}
