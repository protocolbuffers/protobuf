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

import static com.google.protobuf.Internal.checkNotNull;

import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.EnumValueDescriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.Descriptors.OneofDescriptor;
import java.io.IOException;
import java.io.InputStream;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.Map;

/**
 * An implementation of {@link Message} that can represent arbitrary types, given a {@link
 * Descriptors.Descriptor}.
 *
 * @author kenton@google.com Kenton Varda
 */
public final class DynamicMessage extends AbstractMessage {
  private final Descriptor type;
  private final FieldSet<FieldDescriptor> fields;
  private final FieldDescriptor[] oneofCases;
  private final UnknownFieldSet unknownFields;
  private int memoizedSize = -1;

  /**
   * Construct a {@code DynamicMessage} using the given {@code FieldSet}. oneofCases stores the
   * FieldDescriptor for each oneof to indicate which field is set. Caller should make sure the
   * array is immutable.
   *
   * <p>This constructor is package private and will be used in {@code DynamicMutableMessage} to
   * convert a mutable message to an immutable message.
   */
  DynamicMessage(
      Descriptor type,
      FieldSet<FieldDescriptor> fields,
      FieldDescriptor[] oneofCases,
      UnknownFieldSet unknownFields) {
    this.type = type;
    this.fields = fields;
    this.oneofCases = oneofCases;
    this.unknownFields = unknownFields;
  }

  /** Get a {@code DynamicMessage} representing the default instance of the given type. */
  public static DynamicMessage getDefaultInstance(Descriptor type) {
    int oneofDeclCount = type.toProto().getOneofDeclCount();
    FieldDescriptor[] oneofCases = new FieldDescriptor[oneofDeclCount];
    return new DynamicMessage(
        type,
        FieldSet.<FieldDescriptor>emptySet(),
        oneofCases,
        UnknownFieldSet.getDefaultInstance());
  }

  /** Parse a message of the given type from the given input stream. */
  public static DynamicMessage parseFrom(Descriptor type, CodedInputStream input)
      throws IOException {
    return newBuilder(type).mergeFrom(input).buildParsed();
  }

  /** Parse a message of the given type from the given input stream. */
  public static DynamicMessage parseFrom(
      Descriptor type, CodedInputStream input, ExtensionRegistry extensionRegistry)
      throws IOException {
    return newBuilder(type).mergeFrom(input, extensionRegistry).buildParsed();
  }

  /** Parse {@code data} as a message of the given type and return it. */
  public static DynamicMessage parseFrom(Descriptor type, ByteString data)
      throws InvalidProtocolBufferException {
    return newBuilder(type).mergeFrom(data).buildParsed();
  }

  /** Parse {@code data} as a message of the given type and return it. */
  public static DynamicMessage parseFrom(
      Descriptor type, ByteString data, ExtensionRegistry extensionRegistry)
      throws InvalidProtocolBufferException {
    return newBuilder(type).mergeFrom(data, extensionRegistry).buildParsed();
  }

  /** Parse {@code data} as a message of the given type and return it. */
  public static DynamicMessage parseFrom(Descriptor type, byte[] data)
      throws InvalidProtocolBufferException {
    return newBuilder(type).mergeFrom(data).buildParsed();
  }

  /** Parse {@code data} as a message of the given type and return it. */
  public static DynamicMessage parseFrom(
      Descriptor type, byte[] data, ExtensionRegistry extensionRegistry)
      throws InvalidProtocolBufferException {
    return newBuilder(type).mergeFrom(data, extensionRegistry).buildParsed();
  }

  /** Parse a message of the given type from {@code input} and return it. */
  public static DynamicMessage parseFrom(Descriptor type, InputStream input) throws IOException {
    return newBuilder(type).mergeFrom(input).buildParsed();
  }

  /** Parse a message of the given type from {@code input} and return it. */
  public static DynamicMessage parseFrom(
      Descriptor type, InputStream input, ExtensionRegistry extensionRegistry) throws IOException {
    return newBuilder(type).mergeFrom(input, extensionRegistry).buildParsed();
  }

  /** Construct a {@link Message.Builder} for the given type. */
  public static Builder newBuilder(Descriptor type) {
    return new Builder(type);
  }

  /**
   * Construct a {@link Message.Builder} for a message of the same type as {@code prototype}, and
   * initialize it with {@code prototype}'s contents.
   */
  public static Builder newBuilder(Message prototype) {
    return new Builder(prototype.getDescriptorForType()).mergeFrom(prototype);
  }

  // -----------------------------------------------------------------
  // Implementation of Message interface.

  @Override
  public Descriptor getDescriptorForType() {
    return type;
  }

  @Override
  public DynamicMessage getDefaultInstanceForType() {
    return getDefaultInstance(type);
  }

  @Override
  public Map<FieldDescriptor, Object> getAllFields() {
    return fields.getAllFields();
  }

  @Override
  public boolean hasOneof(OneofDescriptor oneof) {
    verifyOneofContainingType(oneof);
    FieldDescriptor field = oneofCases[oneof.getIndex()];
    if (field == null) {
      return false;
    }
    return true;
  }

  @Override
  public FieldDescriptor getOneofFieldDescriptor(OneofDescriptor oneof) {
    verifyOneofContainingType(oneof);
    return oneofCases[oneof.getIndex()];
  }

  @Override
  public boolean hasField(FieldDescriptor field) {
    verifyContainingType(field);
    return fields.hasField(field);
  }

  @Override
  public Object getField(FieldDescriptor field) {
    verifyContainingType(field);
    Object result = fields.getField(field);
    if (result == null) {
      if (field.isRepeated()) {
        result = Collections.emptyList();
      } else if (field.getJavaType() == FieldDescriptor.JavaType.MESSAGE) {
        result = getDefaultInstance(field.getMessageType());
      } else {
        result = field.getDefaultValue();
      }
    }
    return result;
  }

  @Override
  public int getRepeatedFieldCount(FieldDescriptor field) {
    verifyContainingType(field);
    return fields.getRepeatedFieldCount(field);
  }

  @Override
  public Object getRepeatedField(FieldDescriptor field, int index) {
    verifyContainingType(field);
    return fields.getRepeatedField(field, index);
  }

  @Override
  public UnknownFieldSet getUnknownFields() {
    return unknownFields;
  }

  static boolean isInitialized(Descriptor type, FieldSet<FieldDescriptor> fields) {
    // Check that all required fields are present.
    for (final FieldDescriptor field : type.getFields()) {
      if (field.isRequired()) {
        if (!fields.hasField(field)) {
          return false;
        }
      }
    }

    // Check that embedded messages are initialized.
    return fields.isInitialized();
  }

  @Override
  public boolean isInitialized() {
    return isInitialized(type, fields);
  }

  @Override
  public void writeTo(CodedOutputStream output) throws IOException {
    if (type.getOptions().getMessageSetWireFormat()) {
      fields.writeMessageSetTo(output);
      unknownFields.writeAsMessageSetTo(output);
    } else {
      fields.writeTo(output);
      unknownFields.writeTo(output);
    }
  }

  @Override
  public int getSerializedSize() {
    int size = memoizedSize;
    if (size != -1) return size;

    if (type.getOptions().getMessageSetWireFormat()) {
      size = fields.getMessageSetSerializedSize();
      size += unknownFields.getSerializedSizeAsMessageSet();
    } else {
      size = fields.getSerializedSize();
      size += unknownFields.getSerializedSize();
    }

    memoizedSize = size;
    return size;
  }

  @Override
  public Builder newBuilderForType() {
    return new Builder(type);
  }

  @Override
  public Builder toBuilder() {
    return newBuilderForType().mergeFrom(this);
  }

  @Override
  public Parser<DynamicMessage> getParserForType() {
    return new AbstractParser<DynamicMessage>() {
      @Override
      public DynamicMessage parsePartialFrom(
          CodedInputStream input, ExtensionRegistryLite extensionRegistry)
          throws InvalidProtocolBufferException {
        Builder builder = newBuilder(type);
        try {
          builder.mergeFrom(input, extensionRegistry);
        } catch (InvalidProtocolBufferException e) {
          throw e.setUnfinishedMessage(builder.buildPartial());
        } catch (IOException e) {
          throw new InvalidProtocolBufferException(e).setUnfinishedMessage(builder.buildPartial());
        }
        return builder.buildPartial();
      }
    };
  }

  /** Verifies that the field is a field of this message. */
  private void verifyContainingType(FieldDescriptor field) {
    if (field.getContainingType() != type) {
      throw new IllegalArgumentException("FieldDescriptor does not match message type.");
    }
  }

  /** Verifies that the oneof is an oneof of this message. */
  private void verifyOneofContainingType(OneofDescriptor oneof) {
    if (oneof.getContainingType() != type) {
      throw new IllegalArgumentException("OneofDescriptor does not match message type.");
    }
  }

  // =================================================================

  /** Builder for {@link DynamicMessage}s. */
  public static final class Builder extends AbstractMessage.Builder<Builder> {
    private final Descriptor type;
    private FieldSet.Builder<FieldDescriptor> fields;
    private final FieldDescriptor[] oneofCases;
    private UnknownFieldSet unknownFields;

    /** Construct a {@code Builder} for the given type. */
    private Builder(Descriptor type) {
      this.type = type;
      this.fields = FieldSet.newBuilder();
      this.unknownFields = UnknownFieldSet.getDefaultInstance();
      this.oneofCases = new FieldDescriptor[type.toProto().getOneofDeclCount()];
    }

    // ---------------------------------------------------------------
    // Implementation of Message.Builder interface.

    @Override
    public Builder clear() {
      fields = FieldSet.newBuilder();
      unknownFields = UnknownFieldSet.getDefaultInstance();
      return this;
    }

    @Override
    public Builder mergeFrom(Message other) {
      if (other instanceof DynamicMessage) {
        // This should be somewhat faster than calling super.mergeFrom().
        DynamicMessage otherDynamicMessage = (DynamicMessage) other;
        if (otherDynamicMessage.type != type) {
          throw new IllegalArgumentException(
              "mergeFrom(Message) can only merge messages of the same type.");
        }
        fields.mergeFrom(otherDynamicMessage.fields);
        mergeUnknownFields(otherDynamicMessage.unknownFields);
        for (int i = 0; i < oneofCases.length; i++) {
          if (oneofCases[i] == null) {
            oneofCases[i] = otherDynamicMessage.oneofCases[i];
          } else {
            if ((otherDynamicMessage.oneofCases[i] != null)
                && (oneofCases[i] != otherDynamicMessage.oneofCases[i])) {
              fields.clearField(oneofCases[i]);
              oneofCases[i] = otherDynamicMessage.oneofCases[i];
            }
          }
        }
        return this;
      } else {
        return super.mergeFrom(other);
      }
    }

    @Override
    public DynamicMessage build() {
      if (!isInitialized()) {
        throw newUninitializedMessageException(
            new DynamicMessage(
                type, fields.build(), Arrays.copyOf(oneofCases, oneofCases.length), unknownFields));
      }
      return buildPartial();
    }

    /**
     * Helper for DynamicMessage.parseFrom() methods to call. Throws {@link
     * InvalidProtocolBufferException} instead of {@link UninitializedMessageException}.
     */
    private DynamicMessage buildParsed() throws InvalidProtocolBufferException {
      if (!isInitialized()) {
        throw newUninitializedMessageException(
                new DynamicMessage(
                    type,
                    fields.build(),
                    Arrays.copyOf(oneofCases, oneofCases.length),
                    unknownFields))
            .asInvalidProtocolBufferException();
      }
      return buildPartial();
    }

    @Override
    public DynamicMessage buildPartial() {
      // Set default values for all fields in a MapEntry.
      if (type.getOptions().getMapEntry()) {
        for (FieldDescriptor field : type.getFields()) {
          if (field.isOptional() && !fields.hasField(field)) {
            if (field.getJavaType() == FieldDescriptor.JavaType.MESSAGE) {
              fields.setField(field, getDefaultInstance(field.getMessageType()));
            } else {
              fields.setField(field, field.getDefaultValue());
            }
          }
        }
      }

      DynamicMessage result =
          new DynamicMessage(
              type,
              fields.buildPartial(),
              Arrays.copyOf(oneofCases, oneofCases.length),
              unknownFields);
      return result;
    }

    @Override
    public Builder clone() {
      Builder result = new Builder(type);
      result.fields.mergeFrom(fields.build());
      result.mergeUnknownFields(unknownFields);
      System.arraycopy(oneofCases, 0, result.oneofCases, 0, oneofCases.length);
      return result;
    }

    @Override
    public boolean isInitialized() {
      // Check that all required fields are present.
      for (FieldDescriptor field : type.getFields()) {
        if (field.isRequired()) {
          if (!fields.hasField(field)) {
            return false;
          }
        }
      }

      // Check that embedded messages are initialized.
      return fields.isInitialized();
    }

    @Override
    public Descriptor getDescriptorForType() {
      return type;
    }

    @Override
    public DynamicMessage getDefaultInstanceForType() {
      return getDefaultInstance(type);
    }

    @Override
    public Map<FieldDescriptor, Object> getAllFields() {
      return fields.getAllFields();
    }

    @Override
    public Builder newBuilderForField(FieldDescriptor field) {
      verifyContainingType(field);

      if (field.getJavaType() != FieldDescriptor.JavaType.MESSAGE) {
        throw new IllegalArgumentException(
            "newBuilderForField is only valid for fields with message type.");
      }

      return new Builder(field.getMessageType());
    }

    @Override
    public boolean hasOneof(OneofDescriptor oneof) {
      verifyOneofContainingType(oneof);
      FieldDescriptor field = oneofCases[oneof.getIndex()];
      if (field == null) {
        return false;
      }
      return true;
    }

    @Override
    public FieldDescriptor getOneofFieldDescriptor(OneofDescriptor oneof) {
      verifyOneofContainingType(oneof);
      return oneofCases[oneof.getIndex()];
    }

    @Override
    public Builder clearOneof(OneofDescriptor oneof) {
      verifyOneofContainingType(oneof);
      FieldDescriptor field = oneofCases[oneof.getIndex()];
      if (field != null) {
        clearField(field);
      }
      return this;
    }

    @Override
    public boolean hasField(FieldDescriptor field) {
      verifyContainingType(field);
      return fields.hasField(field);
    }

    @Override
    public Object getField(FieldDescriptor field) {
      verifyContainingType(field);
      Object result = fields.getField(field);
      if (result == null) {
        if (field.isRepeated()) {
          result = Collections.emptyList();
        } else if (field.getJavaType() == FieldDescriptor.JavaType.MESSAGE) {
          result = getDefaultInstance(field.getMessageType());
        } else {
          result = field.getDefaultValue();
        }
      }
      return result;
    }

    @Override
    public Builder setField(FieldDescriptor field, Object value) {
      verifyContainingType(field);
      // TODO(xiaofeng): This check should really be put in FieldSet.setField()
      // where all other such checks are done. However, currently
      // FieldSet.setField() permits Integer value for enum fields probably
      // because of some internal features we support. Should figure it out
      // and move this check to a more appropriate place.
      verifyType(field, value);
      OneofDescriptor oneofDescriptor = field.getContainingOneof();
      if (oneofDescriptor != null) {
        int index = oneofDescriptor.getIndex();
        FieldDescriptor oldField = oneofCases[index];
        if ((oldField != null) && (oldField != field)) {
          fields.clearField(oldField);
        }
        oneofCases[index] = field;
      } else if (!field.hasPresence()) {
        if (!field.isRepeated() && value.equals(field.getDefaultValue())) {
          // Setting a field without presence to its default value is equivalent to clearing the
          // field.
          fields.clearField(field);
          return this;
        }
      }
      fields.setField(field, value);
      return this;
    }

    @Override
    public Builder clearField(FieldDescriptor field) {
      verifyContainingType(field);
      OneofDescriptor oneofDescriptor = field.getContainingOneof();
      if (oneofDescriptor != null) {
        int index = oneofDescriptor.getIndex();
        if (oneofCases[index] == field) {
          oneofCases[index] = null;
        }
      }
      fields.clearField(field);
      return this;
    }

    @Override
    public int getRepeatedFieldCount(FieldDescriptor field) {
      verifyContainingType(field);
      return fields.getRepeatedFieldCount(field);
    }

    @Override
    public Object getRepeatedField(FieldDescriptor field, int index) {
      verifyContainingType(field);
      return fields.getRepeatedField(field, index);
    }

    @Override
    public Builder setRepeatedField(FieldDescriptor field, int index, Object value) {
      verifyContainingType(field);
      verifySingularValueType(field, value);
      fields.setRepeatedField(field, index, value);
      return this;
    }

    @Override
    public Builder addRepeatedField(FieldDescriptor field, Object value) {
      verifyContainingType(field);
      verifySingularValueType(field, value);
      fields.addRepeatedField(field, value);
      return this;
    }

    @Override
    public UnknownFieldSet getUnknownFields() {
      return unknownFields;
    }

    @Override
    public Builder setUnknownFields(UnknownFieldSet unknownFields) {
      this.unknownFields = unknownFields;
      return this;
    }

    @Override
    public Builder mergeUnknownFields(UnknownFieldSet unknownFields) {
      this.unknownFields =
          UnknownFieldSet.newBuilder(this.unknownFields).mergeFrom(unknownFields).build();
      return this;
    }

    /** Verifies that the field is a field of this message. */
    private void verifyContainingType(FieldDescriptor field) {
      if (field.getContainingType() != type) {
        throw new IllegalArgumentException("FieldDescriptor does not match message type.");
      }
    }

    /** Verifies that the oneof is an oneof of this message. */
    private void verifyOneofContainingType(OneofDescriptor oneof) {
      if (oneof.getContainingType() != type) {
        throw new IllegalArgumentException("OneofDescriptor does not match message type.");
      }
    }

    /**
     * Verifies that {@code value} is of the appropriate type, in addition to the checks already
     * performed by {@link FieldSet.Builder}.
     */
    private void verifySingularValueType(FieldDescriptor field, Object value) {
      // Most type checks are performed by FieldSet.Builder, but FieldSet.Builder is more permissive
      // than generated Message.Builder subclasses, so we perform extra checks in this class so that
      // DynamicMessage.Builder's semantics more closely match the semantics of generated builders.
      switch (field.getType()) {
        case ENUM:
          checkNotNull(value);
          // FieldSet.Builder accepts Integer values for enum fields.
          if (!(value instanceof EnumValueDescriptor)) {
            throw new IllegalArgumentException(
                "DynamicMessage should use EnumValueDescriptor to set Enum Value.");
          }
          // TODO(xiaofeng): Re-enable this check after Orgstore is fixed to not
          // set incorrect EnumValueDescriptors.
          // EnumDescriptor fieldType = field.getEnumType();
          // EnumDescriptor fieldValueType = ((EnumValueDescriptor) value).getType();
          // if (fieldType != fieldValueType) {
          //  throw new IllegalArgumentException(String.format(
          //      "EnumDescriptor %s of field doesn't match EnumDescriptor %s of field value",
          //      fieldType.getFullName(), fieldValueType.getFullName()));
          // }
          break;
        case MESSAGE:
          // FieldSet.Builder accepts Message.Builder values for message fields.
          if (value instanceof Message.Builder) {
            throw new IllegalArgumentException(
                String.format(
                    "Wrong object type used with protocol message reflection.\n"
                        + "Field number: %d, field java type: %s, value type: %s\n",
                    field.getNumber(),
                    field.getLiteType().getJavaType(),
                    value.getClass().getName()));
          }
          break;
        default:
          break;
      }
    }

    /**
     * Verifies that {@code value} is of the appropriate type, in addition to the checks already
     * performed by {@link FieldSet.Builder}.
     */
    private void verifyType(FieldDescriptor field, Object value) {
      if (field.isRepeated()) {
        for (Object item : (List<?>) value) {
          verifySingularValueType(field, item);
        }
      } else {
        verifySingularValueType(field, value);
      }
    }

    @Override
    public com.google.protobuf.Message.Builder getFieldBuilder(FieldDescriptor field) {
      verifyContainingType(field);
      // Error messages chosen for parity with GeneratedMessage.getFieldBuilder.
      if (field.isMapField()) {
        throw new UnsupportedOperationException("Nested builder not supported for map fields.");
      }
      if (field.getJavaType() != FieldDescriptor.JavaType.MESSAGE) {
        throw new UnsupportedOperationException("getFieldBuilder() called on a non-Message type.");
      }

      Object existingValue = fields.getFieldAllowBuilders(field);
      Message.Builder builder =
          existingValue == null
              ? new Builder(field.getMessageType())
              : toMessageBuilder(existingValue);
      fields.setField(field, builder);
      return builder;
    }

    @Override
    public com.google.protobuf.Message.Builder getRepeatedFieldBuilder(
        FieldDescriptor field, int index) {
      verifyContainingType(field);
      // Error messages chosen for parity with GeneratedMessage.getRepeatedFieldBuilder.
      if (field.isMapField()) {
        throw new UnsupportedOperationException("Map fields cannot be repeated");
      }
      if (field.getJavaType() != FieldDescriptor.JavaType.MESSAGE) {
        throw new UnsupportedOperationException(
            "getRepeatedFieldBuilder() called on a non-Message type.");
      }

      Message.Builder builder =
          toMessageBuilder(fields.getRepeatedFieldAllowBuilders(field, index));
      fields.setRepeatedField(field, index, builder);
      return builder;
    }

    private static Message.Builder toMessageBuilder(Object o) {
      if (o instanceof Message.Builder) {
        return (Message.Builder) o;
      }

      if (o instanceof LazyField) {
        o = ((LazyField) o).getValue();
      }
      if (o instanceof Message) {
        return ((Message) o).toBuilder();
      }

      throw new IllegalArgumentException(
          String.format("Cannot convert %s to Message.Builder", o.getClass()));
    }
  }
}
