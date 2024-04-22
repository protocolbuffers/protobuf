// Copyright 2009 Google Inc. All Rights Reserved.

package com.google.protobuf;

import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.Descriptors.OneofDescriptor;
import com.google.protobuf.Message.Builder;
import java.io.IOException;
import java.io.InputStream;
import java.util.List;
import java.util.Map;

/**
 * An instance of this class wraps a proto1 ProtocolMessage behind the proto2 {@link
 * com.google.protobuf.MessageLite.Builder} API, allowing the user to treat (update) a proto1
 * message in proto2 style.
 *
 * @author dmitriev@google.com (Misha Dmitriev)
 */
public final class Proto2BuilderAdapter implements Builder {

  // Non-final, since it needs to be set to null after build() method is called
  private MutableMessage proto;

  private static final String PARSING_ERROR = "Error parsing protocol message";

  public Proto2BuilderAdapter(MutableMessage proto) {
    this.proto = proto;
  }

  @Override
  public Message getDefaultInstanceForType() {
    return proto.getDefaultInstanceForType();
  }

  @Override
  public Builder clear() {
    proto.clear();
    return this;
  }

  @Override
  public Message build() {
    if (!proto.isInitialized()) {
      throw new UninitializedMessageException(proto.findInitializationErrors());
    }
    Message result = proto;
    proto = null;
    return result;
  }

  @Override
  public Message buildPartial() {
    return proto;
  }

  @Override
  public Builder clone() {
    return new Proto2BuilderAdapter(proto.clone());
  }

  @Override
  public boolean isInitialized() {
    return proto.isInitialized();
  }

  /**
   * See {@link com.google.protobuf.MessageLite.Builder#
   * mergeFrom(com.google.protobuf.CodedInputStream)}.
   *
   * This method implements the above functionality, but its memory usage may
   * be suboptimal, since it reads the whole raw byte array for the message
   * from input stream before parsing it. Its performance will also be
   * suboptimal due to byte-by-byte reading, unless input has the message size
   * set properly using pushLimit().
   */
  @Override
  public Builder mergeFrom(CodedInputStream input)
      throws IOException {
    int size = input.getBytesUntilLimit();
    if (size == -1) {
      throw new InvalidProtocolBufferException("CodedInputStream had no limit");
    }
    byte[] bytes = input.readRawBytes(size);
    if (!proto.mergeFrom(bytes)) {
      throw new InvalidProtocolBufferException("MergeFrom failed");
    }
    return this;
  }

  /**
   * See {@link com.google.protobuf.MessageLite.Builder#
   * mergeFrom(com.google.protobuf.CodedInputStream,
   *           com.google.protobuf.ExtensionRegistryLite)}.
   *
   * This method implements the above functionality, but its memory usage may
   * be suboptimal, since it makes a copy of the whole raw byte array containing
   * the message before parsing it. Also, it ignores any data related with
   * proto extensions.
   */
  @Override
  public Builder mergeFrom(CodedInputStream input,
      ExtensionRegistryLite extensionRegistry) throws IOException {
    return mergeFrom(input);
  }

  /**
   * See {@link com.google.protobuf.MessageLite.Builder#
   * mergeFrom(com.google.protobuf.ByteString)}.
   *
   * This method implements the above functionality, but its memory usage may
   * be suboptimal, since it may make a copy of the whole raw byte array
   * containing the message before parsing it.
   */
  @Override
  public Builder mergeFrom(ByteString data)
      throws InvalidProtocolBufferException {
    try {
      final CodedInputStream input = data.newCodedInput();
      Builder builder = mergeFrom(input);
      input.checkLastTagWas(0);
      return builder;
    } catch (InvalidProtocolBufferException ex) {
      throw ex;
    } catch (IOException ex) {
      // Should never happen in this case
      throw new IllegalStateException(ex);
    }
  }

  /**
   * See {@link com.google.protobuf.MessageLite.Builder#
   * mergeFrom(com.google.protobuf.ByteString,
   *           com.google.protobuf.ExtensionRegistryLite)}.
   *
   * This method implements the above functionality, but its memory usage may
   * be suboptimal, since it makes a copy of the whole raw byte array containing
   * the message before parsing it. Also, it ignores any data related to
   * proto extensions.
   */
  @Override
  public Builder mergeFrom(ByteString data,
      ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException {
    return mergeFrom(data);
  }

  @Override
  public Builder mergeFrom(byte[] data)
      throws InvalidProtocolBufferException {
    if (!proto.mergeFrom(data)) {
      throw new InvalidProtocolBufferException(PARSING_ERROR);
    }
    return this;
  }

  @Override
  public Builder mergeFrom(byte[] data, int off, int len)
      throws InvalidProtocolBufferException {
    if (!proto.mergeFrom(data, off, len)) {
      throw new InvalidProtocolBufferException(PARSING_ERROR);
    }
    return this;
  }

  /**
   * See {@link com.google.protobuf.MessageLite.Builder#
   * mergeFrom(byte[], com.google.protobuf.ExtensionRegistryLite)}.
   *
   * This method ignores any data related to proto extensions.
   */
  @Override
  public Builder mergeFrom(byte[] data,
      ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException {
    return mergeFrom(data);
  }

  /**
   * See {@link com.google.protobuf.MessageLite.Builder#
   * mergeFrom(byte[], int, int, com.google.protobuf.ExtensionRegistryLite)}.
   *
   * This method ignores any data related to proto extensions.
   */
  @Override
  public Builder mergeFrom(byte[] data, int off, int len,
      ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException {
    return mergeFrom(data, off, len);
  }

  @Override
  public Builder mergeFrom(InputStream input) throws IOException {
    if (!proto.mergeFrom(input)) {
      throw new IOException(PARSING_ERROR);
    }
    return this;
  }

  /**
   * See {@link com.google.protobuf.MessageLite.Builder#
   * mergeFrom(byte[], com.google.protobuf.ExtensionRegistryLite)}.
   *
   * This method ignores any data related to proto extensions.
   */
  @Override
  public Builder mergeFrom(InputStream input,
      ExtensionRegistryLite extensionRegistry) throws IOException {
    return mergeFrom(input);
  }

  /**
   * See {@link com.google.protobuf.MessageLite.Builder#
   * mergeDelimitedFrom(InputStream)}.
   *
   * This method implements the above functionality, but its memory usage may
   * be suboptimal, since it makes a copy of the whole raw byte array containing
   * the message before parsing it.
   */
  @Override
  public boolean mergeDelimitedFrom(InputStream input) throws IOException {
    // First check if we are already at the end of stream - we are required to
    // simply return false in that case.
    int firstByte = input.read();
    if (firstByte == -1) {
      return false;
    }

    final int size = CodedInputStream.readRawVarint32(firstByte, input);
    if (size < 0) {
      throw new IOException("Negative message size read from stream: " + size);
    }

    byte[] bytes = new byte[size];
    int remaining = size;
    int off = 0;
    while (remaining > 0) {
      int bytesRead = input.read(bytes, off, remaining);
      if (bytesRead == -1) {
        throw new IOException("EOF reached unexpectedly at position " + off);
      }
      remaining -= bytesRead;
      off += bytesRead;
    }

    mergeFrom(bytes);
    return true;
  }

  /**
   * See {@link com.google.protobuf.MessageLite.Builder#
   * mergeDelimitedFrom(InputStream, com.google.protobuf.ExtensionRegistryLite)}.
   *
   * This method implements the above functionality, but its memory usage may
   * be suboptimal, since it makes a copy of the whole raw byte array containing
   * the message before parsing it. This method also ignores any data related
   * to proto extensions.
   */
  @Override
  public boolean mergeDelimitedFrom(InputStream input,
      ExtensionRegistryLite extensionRegistry) throws IOException {
    return mergeDelimitedFrom(input);
  }

  /**
   * The following methods implement proto2 reflection API or depend on proto2
   * reflection API. They are not yet supported for proto1.
   * TODO: Implement proto2 reflection API for proto1.
   */
  @Override
  public List<String> findInitializationErrors() {
    throw new UnsupportedOperationException("not implemented for proto1");
  }

  @Override
  public String getInitializationErrorString() {
    throw new UnsupportedOperationException("not implemented for proto1");
  }

  @Override
  public Map<FieldDescriptor, Object> getAllFields() {
    return proto.getAllFields();
  }

  /** TODO: Add @Overrride to this function after oneof java is released */
  @Override
  public boolean hasOneof(OneofDescriptor oneof) {
    throw new UnsupportedOperationException("not implemented for proto1");
  }

  /** TODO: Add @Overrride to this function after oneof java is released */
  @Override
  public FieldDescriptor getOneofFieldDescriptor(OneofDescriptor oneof) {
    throw new UnsupportedOperationException("not implemented for proto1");
  }

  @Override
  public boolean hasField(FieldDescriptor field) {
    return proto.hasField(field);
  }

  @Override
  public Object getField(FieldDescriptor field) {
    return proto.getField(field);
  }

  @Override
  public int getRepeatedFieldCount(FieldDescriptor field) {
    return proto.getRepeatedFieldCount(field);
  }

  @Override
  public Object getRepeatedField(FieldDescriptor field, int index) {
    return proto.getRepeatedField(field, index);
  }

  @Override
  public UnknownFieldSet getUnknownFields() {
    return proto.getUnknownFields();
  }

  @Override
  public Builder mergeFrom(MessageLite other) {
    proto.mergeFrom((MutableMessage) other);
    return this;
  }

  @Override
  public Builder mergeFrom(Message other) {
    proto.mergeFrom((MutableMessage) other);
    return this;
  }

  @Override
  public Descriptor getDescriptorForType() {
    return proto.getDescriptorForType();
  }

  @Override
  public Builder newBuilderForField(FieldDescriptor field) {
    return proto.newMessageForField(field).newBuilderForType();
  }

  @Override
  public Builder getFieldBuilder(FieldDescriptor field) {
    throw new UnsupportedOperationException("not implemented for proto1");
  }

  @Override
  public Builder getRepeatedFieldBuilder(FieldDescriptor field, int index) {
    throw new UnsupportedOperationException("not implemented for proto1");
  }

  @Override
  public Builder setField(FieldDescriptor field, Object value) {
    proto.setField(field, value);
    return this;
  }

  @Override
  public Builder clearField(FieldDescriptor field) {
    proto.clearField(field);
    return this;
  }

  /** TODO: Add @Overrride to this function after oneof java is released */
  @Override
  public Builder clearOneof(OneofDescriptor oneof) {
    throw new UnsupportedOperationException("not implemented for proto1");
  }

  @Override
  public Builder setRepeatedField(FieldDescriptor field, int index, Object value) {
    proto.setRepeatedField(field, index, value);
    return this;
  }

  @Override
  public Builder addRepeatedField(FieldDescriptor field, Object value) {
    proto.addRepeatedField(field, value);
    return this;
  }

  @Override
  public Builder setUnknownFields(UnknownFieldSet unknownFields) {
    proto.setUnknownFields(unknownFields);
    return this;
  }

  @Override
  public Builder mergeUnknownFields(UnknownFieldSet unknownFields) {
    throw new UnsupportedOperationException("not implemented for proto1");
  }
}
