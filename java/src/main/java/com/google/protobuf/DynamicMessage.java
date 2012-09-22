// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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

import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;

import java.io.InputStream;
import java.io.IOException;
import java.util.Collections;
import java.util.Map;

/**
 * An implementation of {@link Message} that can represent arbitrary types,
 * given a {@link Descriptors.Descriptor}.
 *
 * @author kenton@google.com Kenton Varda
 */
public final class DynamicMessage extends AbstractMessage {
  private final Descriptor type;
  private final FieldSet<FieldDescriptor> fields;
  private final UnknownFieldSet unknownFields;
  private int memoizedSize = -1;

  /**
   * Construct a {@code DynamicMessage} using the given {@code FieldSet}.
   */
  private DynamicMessage(Descriptor type, FieldSet<FieldDescriptor> fields,
                         UnknownFieldSet unknownFields) {
    this.type = type;
    this.fields = fields;
    this.unknownFields = unknownFields;
  }

  /**
   * Get a {@code DynamicMessage} representing the default instance of the
   * given type.
   */
  public static DynamicMessage getDefaultInstance(Descriptor type) {
    return new DynamicMessage(type, FieldSet.<FieldDescriptor>emptySet(),
                              UnknownFieldSet.getDefaultInstance());
  }

  /** Parse a message of the given type from the given input stream. */
  public static DynamicMessage parseFrom(Descriptor type,
                                         CodedInputStream input)
                                         throws IOException {
    return newBuilder(type).mergeFrom(input).buildParsed();
  }

  /** Parse a message of the given type from the given input stream. */
  public static DynamicMessage parseFrom(
      Descriptor type,
      CodedInputStream input,
      ExtensionRegistry extensionRegistry)
      throws IOException {
    return newBuilder(type).mergeFrom(input, extensionRegistry).buildParsed();
  }

  /** Parse {@code data} as a message of the given type and return it. */
  public static DynamicMessage parseFrom(Descriptor type, ByteString data)
                                         throws InvalidProtocolBufferException {
    return newBuilder(type).mergeFrom(data).buildParsed();
  }

  /** Parse {@code data} as a message of the given type and return it. */
  public static DynamicMessage parseFrom(Descriptor type, ByteString data,
                                         ExtensionRegistry extensionRegistry)
                                         throws InvalidProtocolBufferException {
    return newBuilder(type).mergeFrom(data, extensionRegistry).buildParsed();
  }

  /** Parse {@code data} as a message of the given type and return it. */
  public static DynamicMessage parseFrom(Descriptor type, byte[] data)
                                         throws InvalidProtocolBufferException {
    return newBuilder(type).mergeFrom(data).buildParsed();
  }

  /** Parse {@code data} as a message of the given type and return it. */
  public static DynamicMessage parseFrom(Descriptor type, byte[] data,
                                         ExtensionRegistry extensionRegistry)
                                         throws InvalidProtocolBufferException {
    return newBuilder(type).mergeFrom(data, extensionRegistry).buildParsed();
  }

  /** Parse a message of the given type from {@code input} and return it. */
  public static DynamicMessage parseFrom(Descriptor type, InputStream input)
                                         throws IOException {
    return newBuilder(type).mergeFrom(input).buildParsed();
  }

  /** Parse a message of the given type from {@code input} and return it. */
  public static DynamicMessage parseFrom(Descriptor type, InputStream input,
                                         ExtensionRegistry extensionRegistry)
                                         throws IOException {
    return newBuilder(type).mergeFrom(input, extensionRegistry).buildParsed();
  }

  /** Construct a {@link Message.Builder} for the given type. */
  public static Builder newBuilder(Descriptor type) {
    return new Builder(type);
  }

  /**
   * Construct a {@link Message.Builder} for a message of the same type as
   * {@code prototype}, and initialize it with {@code prototype}'s contents.
   */
  public static Builder newBuilder(Message prototype) {
    return new Builder(prototype.getDescriptorForType()).mergeFrom(prototype);
  }

  // -----------------------------------------------------------------
  // Implementation of Message interface.

  public Descriptor getDescriptorForType() {
    return type;
  }

  public DynamicMessage getDefaultInstanceForType() {
    return getDefaultInstance(type);
  }

  public Map<FieldDescriptor, Object> getAllFields() {
    return fields.getAllFields();
  }

  public boolean hasField(FieldDescriptor field) {
    verifyContainingType(field);
    return fields.hasField(field);
  }

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

  public int getRepeatedFieldCount(FieldDescriptor field) {
    verifyContainingType(field);
    return fields.getRepeatedFieldCount(field);
  }

  public Object getRepeatedField(FieldDescriptor field, int index) {
    verifyContainingType(field);
    return fields.getRepeatedField(field, index);
  }

  public UnknownFieldSet getUnknownFields() {
    return unknownFields;
  }

  private static boolean isInitialized(Descriptor type,
                                       FieldSet<FieldDescriptor> fields) {
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

  public Builder newBuilderForType() {
    return new Builder(type);
  }

  public Builder toBuilder() {
    return newBuilderForType().mergeFrom(this);
  }

  public Parser<DynamicMessage> getParserForType() {
    return new AbstractParser<DynamicMessage>() {
      public DynamicMessage parsePartialFrom(
          CodedInputStream input,
          ExtensionRegistryLite extensionRegistry)
          throws InvalidProtocolBufferException {
        Builder builder = newBuilder(type);
        try {
          builder.mergeFrom(input, extensionRegistry);
        } catch (InvalidProtocolBufferException e) {
          throw e.setUnfinishedMessage(builder.buildPartial());
        } catch (IOException e) {
          throw new InvalidProtocolBufferException(e.getMessage())
              .setUnfinishedMessage(builder.buildPartial());
        }
        return builder.buildPartial();
      }
    };
  }

  /** Verifies that the field is a field of this message. */
  private void verifyContainingType(FieldDescriptor field) {
    if (field.getContainingType() != type) {
      throw new IllegalArgumentException(
        "FieldDescriptor does not match message type.");
    }
  }

  // =================================================================

  /**
   * Builder for {@link DynamicMessage}s.
   */
  public static final class Builder extends AbstractMessage.Builder<Builder> {
    private final Descriptor type;
    private FieldSet<FieldDescriptor> fields;
    private UnknownFieldSet unknownFields;

    /** Construct a {@code Builder} for the given type. */
    private Builder(Descriptor type) {
      this.type = type;
      this.fields = FieldSet.newFieldSet();
      this.unknownFields = UnknownFieldSet.getDefaultInstance();
    }

    // ---------------------------------------------------------------
    // Implementation of Message.Builder interface.

    @Override
    public Builder clear() {
      if (fields.isImmutable()) {
        fields = FieldSet.newFieldSet();
      } else {
        fields.clear();
      }
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
        ensureIsMutable();
        fields.mergeFrom(otherDynamicMessage.fields);
        mergeUnknownFields(otherDynamicMessage.unknownFields);
        return this;
      } else {
        return super.mergeFrom(other);
      }
    }

    public DynamicMessage build() {
      if (!isInitialized()) {
        throw newUninitializedMessageException(
          new DynamicMessage(type, fields, unknownFields));
      }
      return buildPartial();
    }

    /**
     * Helper for DynamicMessage.parseFrom() methods to call.  Throws
     * {@link InvalidProtocolBufferException} instead of
     * {@link UninitializedMessageException}.
     */
    private DynamicMessage buildParsed() throws InvalidProtocolBufferException {
      if (!isInitialized()) {
        throw newUninitializedMessageException(
            new DynamicMessage(type, fields, unknownFields))
          .asInvalidProtocolBufferException();
      }
      return buildPartial();
    }

    public DynamicMessage buildPartial() {
      fields.makeImmutable();
      DynamicMessage result =
        new DynamicMessage(type, fields, unknownFields);
      return result;
    }

    @Override
    public Builder clone() {
      Builder result = new Builder(type);
      result.fields.mergeFrom(fields);
      result.mergeUnknownFields(unknownFields);
      return result;
    }

    public boolean isInitialized() {
      return DynamicMessage.isInitialized(type, fields);
    }

    public Descriptor getDescriptorForType() {
      return type;
    }

    public DynamicMessage getDefaultInstanceForType() {
      return getDefaultInstance(type);
    }

    public Map<FieldDescriptor, Object> getAllFields() {
      return fields.getAllFields();
    }

    public Builder newBuilderForField(FieldDescriptor field) {
      verifyContainingType(field);

      if (field.getJavaType() != FieldDescriptor.JavaType.MESSAGE) {
        throw new IllegalArgumentException(
          "newBuilderForField is only valid for fields with message type.");
      }

      return new Builder(field.getMessageType());
    }

    public boolean hasField(FieldDescriptor field) {
      verifyContainingType(field);
      return fields.hasField(field);
    }

    public Object getField(FieldDescriptor field) {
      verifyContainingType(field);
      Object result = fields.getField(field);
      if (result == null) {
        if (field.getJavaType() == FieldDescriptor.JavaType.MESSAGE) {
          result = getDefaultInstance(field.getMessageType());
        } else {
          result = field.getDefaultValue();
        }
      }
      return result;
    }

    public Builder setField(FieldDescriptor field, Object value) {
      verifyContainingType(field);
      ensureIsMutable();
      fields.setField(field, value);
      return this;
    }

    public Builder clearField(FieldDescriptor field) {
      verifyContainingType(field);
      ensureIsMutable();
      fields.clearField(field);
      return this;
    }

    public int getRepeatedFieldCount(FieldDescriptor field) {
      verifyContainingType(field);
      return fields.getRepeatedFieldCount(field);
    }

    public Object getRepeatedField(FieldDescriptor field, int index) {
      verifyContainingType(field);
      return fields.getRepeatedField(field, index);
    }

    public Builder setRepeatedField(FieldDescriptor field,
                                    int index, Object value) {
      verifyContainingType(field);
      ensureIsMutable();
      fields.setRepeatedField(field, index, value);
      return this;
    }

    public Builder addRepeatedField(FieldDescriptor field, Object value) {
      verifyContainingType(field);
      ensureIsMutable();
      fields.addRepeatedField(field, value);
      return this;
    }

    public UnknownFieldSet getUnknownFields() {
      return unknownFields;
    }

    public Builder setUnknownFields(UnknownFieldSet unknownFields) {
      this.unknownFields = unknownFields;
      return this;
    }

    @Override
    public Builder mergeUnknownFields(UnknownFieldSet unknownFields) {
      this.unknownFields =
        UnknownFieldSet.newBuilder(this.unknownFields)
                       .mergeFrom(unknownFields)
                       .build();
      return this;
    }

    /** Verifies that the field is a field of this message. */
    private void verifyContainingType(FieldDescriptor field) {
      if (field.getContainingType() != type) {
        throw new IllegalArgumentException(
          "FieldDescriptor does not match message type.");
      }
    }

    private void ensureIsMutable() {
      if (fields.isImmutable()) {
        fields = fields.clone();
      }
    }

    @Override
    public com.google.protobuf.Message.Builder getFieldBuilder(FieldDescriptor field) {
      // TODO(xiangl): need implementation for dynamic message
      throw new UnsupportedOperationException(
        "getFieldBuilder() called on a dynamic message type.");
    }
  }
}
