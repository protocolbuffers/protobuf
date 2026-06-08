// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.util.Collection;

/**
 * A partial implementation of the {@link MutableMessageLite} interface which implements as many
 * methods of that interface as possible in terms of other methods
 *
 * <p>This class is intended to only be extended by protoc created gencode. It is not intended or
 * supported to extend this class, and any protected methods may be removed without it being
 * considered a breaking change as long as all supported gencode does not depend on the changed
 * methods.
 *
 * @deprecated Mutable API has been strongly deprecated for over a decade and should be migrated off
 *     of as soon as is practicable. Use the standard JavaProto (which provides a builder/immutable
 *     API) instead.
 * @author liujisi@google.com (Pherl Liu)
 */
@SuppressWarnings({"unchecked", "nullness"})
@CheckReturnValue
@Deprecated
public abstract class AbstractMutableMessageLite implements MutableMessageLite {
  /**
   * A flag indicating if this message is mutable. This flag is set to false only for default
   * instances.
   */
  private boolean isMutable = true;

  protected void makeImmutable() {
    isMutable = false;
  }

  protected void assertMutable() {
    if (!isMutable) {
      throw new IllegalStateException("Try to modify an immutable message.");
    }
  }

  @Override
  public Builder toBuilder() {
    throw new UnsupportedOperationException("toBuilder() is not supported in mutable messages.");
  }

  @Override
  public Builder newBuilderForType() {
    throw new UnsupportedOperationException(
        "newBuilderForType() is not supported in mutable messages.");
  }

  // The compiler produces an error if this is not declared explicitly.
  @Override
  public MutableMessageLite clone() {
    throw new UnsupportedOperationException("clone() should be implemented by subclasses.");
  }

  @Override
  public ByteString toByteString() {
    try {
      final ByteString.CodedBuilder out = ByteString.newCodedBuilder(getSerializedSize());
      writeTo(out.getCodedOutput());
      return out.build();
    } catch (IOException e) {
      throw new RuntimeException(
          "Serializing to a ByteString threw an IOException (should never happen).", e);
    }
  }

  @Override
  public byte[] toByteArray() {
    try {
      final byte[] result = new byte[getSerializedSize()];
      final CodedOutputStream output = CodedOutputStream.newInstance(result);
      writeTo(output);
      output.checkNoSpaceLeft();
      return result;
    } catch (IOException e) {
      throw new RuntimeException(
          "Serializing to a byte array threw an IOException (should never happen).", e);
    }
  }

  @Override
  public void writeTo(final OutputStream output) throws IOException {
    final int bufferSize = CodedOutputStream.computePreferredBufferSize(getSerializedSize());
    final CodedOutputStream codedOutput = CodedOutputStream.newInstance(output, bufferSize);
    writeTo(codedOutput);
    codedOutput.flush();
  }

  @Override
  public void writeDelimitedTo(final OutputStream output) throws IOException {
    final int serialized = getSerializedSize();
    final int bufferSize =
        CodedOutputStream.computePreferredBufferSize(
            CodedOutputStream.computeUInt32SizeNoTag(serialized) + serialized);
    final CodedOutputStream codedOutput = CodedOutputStream.newInstance(output, bufferSize);
    codedOutput.writeUInt32NoTag(serialized);
    writeTo(codedOutput);
    codedOutput.flush();
  }

  /** Package private helper method for AbstractParser to create UninitializedMessageException. */
  UninitializedMessageException newUninitializedMessageException() {
    return new UninitializedMessageException(this);
  }

  @Override
  public final int getCachedSize() {
    return getSerializedSize();
  }

  @Override
  public void writeTo(CodedOutputStream output) throws IOException {
    // getSerializedSize() is required for writeToWithCachedSizes to operate; do not remove!
    int unused = getSerializedSize();
    writeToWithCachedSizes(output);
  }

  @Override
  public boolean mergeFrom(CodedInputStream input) {
    return mergeFrom(input, ExtensionRegistryLite.getEmptyRegistry());
  }

  @Override
  public boolean mergeFrom(ByteString data) {
    final CodedInputStream input = data.newCodedInput();
    return mergeFrom(input) && input.getLastTag() == 0;
  }

  @Override
  public boolean mergeFrom(ByteString data, ExtensionRegistryLite extensionRegistry) {
    final CodedInputStream input = data.newCodedInput();
    return mergeFrom(input, extensionRegistry) && input.getLastTag() == 0;
  }

  @Override
  public boolean mergeFrom(byte[] data) {
    return mergeFrom(data, 0, data.length);
  }

  @Override
  public boolean mergeFrom(byte[] data, int off, int len) {
    final CodedInputStream input = CodedInputStream.newInstance(data, off, len);
    return mergeFrom(input) && input.getLastTag() == 0;
  }

  @Override
  public boolean mergeFrom(byte[] data, ExtensionRegistryLite extensionRegistry) {
    return mergeFrom(data, 0, data.length, extensionRegistry);
  }

  @Override
  public boolean mergeFrom(byte[] data, int off, int len, ExtensionRegistryLite extensionRegistry) {
    final CodedInputStream input = CodedInputStream.newInstance(data, off, len);
    return mergeFrom(input, extensionRegistry) && input.getLastTag() == 0;
  }

  @Override
  public boolean mergeFrom(ByteBuffer data) {
    final CodedInputStream input = CodedInputStream.newInstance(data);
    return mergeFrom(input) && input.getLastTag() == 0;
  }

  @Override
  public boolean mergeFrom(ByteBuffer data, ExtensionRegistryLite extensionRegistry) {
    final CodedInputStream input = CodedInputStream.newInstance(data);
    return mergeFrom(input, extensionRegistry) && input.getLastTag() == 0;
  }

  @Override
  public boolean mergeFrom(InputStream input) {
    final CodedInputStream codedInput = CodedInputStream.newInstance(input);
    return mergeFrom(codedInput) && codedInput.getLastTag() == 0;
  }

  @Override
  public boolean mergeFrom(InputStream input, ExtensionRegistryLite extensionRegistry) {
    final CodedInputStream codedInput = CodedInputStream.newInstance(input);
    return mergeFrom(codedInput, extensionRegistry) && codedInput.getLastTag() == 0;
  }

  @Override
  public boolean mergeDelimitedFrom(InputStream input) {
    return mergeDelimitedFrom(input, ExtensionRegistryLite.getEmptyRegistry());
  }

  @Override
  public boolean mergeDelimitedFrom(InputStream input, ExtensionRegistryLite extensionRegistry) {
    try {
      final int firstByte = input.read();
      if (firstByte == -1) {
        return false;
      }
      final int size = CodedInputStream.readRawVarint32(firstByte, input);
      final InputStream limitedInput =
          new AbstractMessageLite.Builder.LimitedInputStream(input, size);
      return mergeFrom(limitedInput, extensionRegistry);
    } catch (IOException e) {
      return false;
    }
  }

  // Parse methods.

  public boolean parseFrom(CodedInputStream input) {
    clear();
    return mergeFrom(input);
  }

  public boolean parseFrom(CodedInputStream input, ExtensionRegistryLite extensionRegistry) {
    clear();
    return mergeFrom(input, extensionRegistry);
  }

  @Override
  public boolean parseFrom(ByteString data) {
    clear();
    return mergeFrom(data);
  }

  @Override
  public boolean parseFrom(ByteString data, ExtensionRegistryLite extensionRegistry) {
    clear();
    return mergeFrom(data, extensionRegistry);
  }

  @Override
  public boolean parseFrom(byte[] data) {
    clear();
    return mergeFrom(data, 0, data.length);
  }

  @Override
  public boolean parseFrom(byte[] data, int off, int len) {
    clear();
    return mergeFrom(data, off, len);
  }

  @Override
  public boolean parseFrom(byte[] data, ExtensionRegistryLite extensionRegistry) {
    clear();
    return mergeFrom(data, 0, data.length, extensionRegistry);
  }

  @Override
  public boolean parseFrom(byte[] data, int off, int len, ExtensionRegistryLite extensionRegistry) {
    clear();
    return mergeFrom(data, off, len, extensionRegistry);
  }

  @Override
  public boolean parseFrom(ByteBuffer data) {
    clear();
    return mergeFrom(data);
  }

  @Override
  public boolean parseFrom(ByteBuffer data, ExtensionRegistryLite extensionRegistry) {
    clear();
    return mergeFrom(data, extensionRegistry);
  }

  @Override
  public boolean parseFrom(InputStream input) {
    clear();
    return mergeFrom(input);
  }

  @Override
  public boolean parseFrom(InputStream input, ExtensionRegistryLite extensionRegistry) {
    clear();
    return mergeFrom(input, extensionRegistry);
  }

  @Override
  public boolean parseDelimitedFrom(InputStream input) {
    clear();
    return mergeDelimitedFrom(input);
  }

  @Override
  public boolean parseDelimitedFrom(InputStream input, ExtensionRegistryLite extensionRegistry) {
    clear();
    return mergeDelimitedFrom(input, extensionRegistry);
  }

  protected static UninitializedMessageException newUninitializedMessageException(
      MessageLite message) {
    return new UninitializedMessageException(message);
  }

  protected static <T extends MutableMessageLite> Parser<T> internalNewParserForType(
      final T defaultInstance) {
    return new AbstractParser<T>() {
      @Override
      @SuppressWarnings("unchecked")
      public T parsePartialFrom(CodedInputStream input, ExtensionRegistryLite extensionRegistry)
          throws InvalidProtocolBufferException {
        T message = (T) defaultInstance.newMessageForType();
        if (!message.mergeFrom(input, extensionRegistry)) {
          throw InvalidProtocolBufferException.parseFailure().setUnfinishedMessage(message);
        }
        return message;
      }
    };
  }

  /**
   * Adds the {@code values} to the {@code list}. This is a helper method used by generated code.
   * Users should ignore it.
   *
   * @throws NullPointerException if any of the elements of {@code values} is null.
   */
  protected static <T> void addAll(final Iterable<T> values, final Collection<? super T> list) {
    AbstractMessageLite.Builder.addAll(values, list);
  }

  /**
   * Whether this message is a proto1 group.
   *
   * <p>Protobuf internal. Users should not use this method.
   */
  protected boolean isProto1Group() {
    return false;
  }
}
