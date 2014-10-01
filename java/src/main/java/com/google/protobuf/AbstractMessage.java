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
import com.google.protobuf.Descriptors.OneofDescriptor;
import com.google.protobuf.Internal.EnumLite;

import java.io.IOException;
import java.io.InputStream;
import java.util.Arrays;
import java.util.List;
import java.util.Map;

/**
 * A partial implementation of the {@link Message} interface which implements
 * as many methods of that interface as possible in terms of other methods.
 *
 * @author kenton@google.com Kenton Varda
 */
public abstract class AbstractMessage extends AbstractMessageLite
                                      implements Message {
  public boolean isInitialized() {
    return MessageReflection.isInitialized(this);
  }


  public List<String> findInitializationErrors() {
    return MessageReflection.findMissingFields(this);
  }

  public String getInitializationErrorString() {
    return MessageReflection.delimitWithCommas(findInitializationErrors());
  }

  /** TODO(jieluo): Clear it when all subclasses have implemented this method. */
  @Override
  public boolean hasOneof(OneofDescriptor oneof) {
    throw new UnsupportedOperationException("hasOneof() is not implemented.");
  }

  /** TODO(jieluo): Clear it when all subclasses have implemented this method. */
  @Override
  public FieldDescriptor getOneofFieldDescriptor(OneofDescriptor oneof) {
    throw new UnsupportedOperationException(
        "getOneofFieldDescriptor() is not implemented.");
  }

  @Override
  public final String toString() {
    return TextFormat.printToString(this);
  }

  public void writeTo(final CodedOutputStream output) throws IOException {
    MessageReflection.writeMessageTo(this, output, false);
  }

  private int memoizedSize = -1;

  public int getSerializedSize() {
    int size = memoizedSize;
    if (size != -1) {
      return size;
    }

    memoizedSize = MessageReflection.getSerializedSize(this);
    return memoizedSize;
  }

  @Override
  public boolean equals(final Object other) {
    if (other == this) {
      return true;
    }
    if (!(other instanceof Message)) {
      return false;
    }
    final Message otherMessage = (Message) other;
    if (getDescriptorForType() != otherMessage.getDescriptorForType()) {
      return false;
    }
    return compareFields(getAllFields(), otherMessage.getAllFields()) &&
        getUnknownFields().equals(otherMessage.getUnknownFields());
  }

  @Override
  public int hashCode() {
    int hash = memoizedHashCode;
    if (hash == 0) {
      hash = 41;
      hash = (19 * hash) + getDescriptorForType().hashCode();
      hash = hashFields(hash, getAllFields());
      hash = (29 * hash) + getUnknownFields().hashCode();
      memoizedHashCode = hash;
    }
    return hash;
  }
  
  private static ByteString toByteString(Object value) {
    if (value instanceof byte[]) {
      return ByteString.copyFrom((byte[]) value);
    } else {
      return (ByteString) value;
    }
  }
 
  /**
   * Compares two bytes fields. The parameters must be either a byte array or a
   * ByteString object. They can be of different type though.
   */
  private static boolean compareBytes(Object a, Object b) {
    if (a instanceof byte[] && b instanceof byte[]) {
      return Arrays.equals((byte[])a, (byte[])b);
    }
    return toByteString(a).equals(toByteString(b));
  }
  
  /**
   * Compares two set of fields.
   * This method is used to implement {@link AbstractMessage#equals(Object)}
   * and {@link AbstractMutableMessage#equals(Object)}. It takes special care
   * of bytes fields because immutable messages and mutable messages use
   * different Java type to reprensent a bytes field and this method should be
   * able to compare immutable messages, mutable messages and also an immutable
   * message to a mutable message.
   */
  static boolean compareFields(Map<FieldDescriptor, Object> a,
      Map<FieldDescriptor, Object> b) {
    if (a.size() != b.size()) {
      return false;
    }
    for (FieldDescriptor descriptor : a.keySet()) {
      if (!b.containsKey(descriptor)) {
        return false;
      }
      Object value1 = a.get(descriptor);
      Object value2 = b.get(descriptor);
      if (descriptor.getType() == FieldDescriptor.Type.BYTES) {
        if (descriptor.isRepeated()) {
          List list1 = (List) value1;
          List list2 = (List) value2;
          if (list1.size() != list2.size()) {
            return false;
          }
          for (int i = 0; i < list1.size(); i++) {
            if (!compareBytes(list1.get(i), list2.get(i))) {
              return false;
            }
          }
        } else {
          // Compares a singular bytes field.
          if (!compareBytes(value1, value2)) {
            return false;
          }
        }
      } else {
        // Compare non-bytes fields.
        if (!value1.equals(value2)) {
          return false;
        }
      }
    }
    return true;
  }

  /** Get a hash code for given fields and values, using the given seed. */
  @SuppressWarnings("unchecked")
  protected static int hashFields(int hash, Map<FieldDescriptor, Object> map) {
    for (Map.Entry<FieldDescriptor, Object> entry : map.entrySet()) {
      FieldDescriptor field = entry.getKey();
      Object value = entry.getValue();
      hash = (37 * hash) + field.getNumber();
      if (field.getType() != FieldDescriptor.Type.ENUM){
        hash = (53 * hash) + value.hashCode();
      } else if (field.isRepeated()) {
        List<? extends EnumLite> list = (List<? extends EnumLite>) value;
        hash = (53 * hash) + Internal.hashEnumList(list);
      } else {
        hash = (53 * hash) + Internal.hashEnum((EnumLite) value);
      }
    }
    return hash;
  }

  /**
   * Package private helper method for AbstractParser to create
   * UninitializedMessageException with missing field information.
   */
  @Override
  UninitializedMessageException newUninitializedMessageException() {
    return Builder.newUninitializedMessageException(this);
  }

  // =================================================================

  /**
   * A partial implementation of the {@link Message.Builder} interface which
   * implements as many methods of that interface as possible in terms of
   * other methods.
   */
  @SuppressWarnings("unchecked")
  public static abstract class Builder<BuilderType extends Builder>
      extends AbstractMessageLite.Builder<BuilderType>
      implements Message.Builder {
    // The compiler produces an error if this is not declared explicitly.
    @Override
    public abstract BuilderType clone();

    /** TODO(jieluo): Clear it when all subclasses have implemented this method. */
    @Override
    public boolean hasOneof(OneofDescriptor oneof) {
      throw new UnsupportedOperationException("hasOneof() is not implemented.");
    }

    /** TODO(jieluo): Clear it when all subclasses have implemented this method. */
    @Override
    public FieldDescriptor getOneofFieldDescriptor(OneofDescriptor oneof) {
      throw new UnsupportedOperationException(
          "getOneofFieldDescriptor() is not implemented.");
    }

    /** TODO(jieluo): Clear it when all subclasses have implemented this method. */
    @Override
    public BuilderType clearOneof(OneofDescriptor oneof) {
      throw new UnsupportedOperationException("clearOneof() is not implemented.");
    }

    public BuilderType clear() {
      for (final Map.Entry<FieldDescriptor, Object> entry :
           getAllFields().entrySet()) {
        clearField(entry.getKey());
      }
      return (BuilderType) this;
    }

    public List<String> findInitializationErrors() {
      return MessageReflection.findMissingFields(this);
    }

    public String getInitializationErrorString() {
      return MessageReflection.delimitWithCommas(findInitializationErrors());
    }

    public BuilderType mergeFrom(final Message other) {
      if (other.getDescriptorForType() != getDescriptorForType()) {
        throw new IllegalArgumentException(
          "mergeFrom(Message) can only merge messages of the same type.");
      }

      // Note:  We don't attempt to verify that other's fields have valid
      //   types.  Doing so would be a losing battle.  We'd have to verify
      //   all sub-messages as well, and we'd have to make copies of all of
      //   them to insure that they don't change after verification (since
      //   the Message interface itself cannot enforce immutability of
      //   implementations).
      // TODO(kenton):  Provide a function somewhere called makeDeepCopy()
      //   which allows people to make secure deep copies of messages.

      for (final Map.Entry<FieldDescriptor, Object> entry :
           other.getAllFields().entrySet()) {
        final FieldDescriptor field = entry.getKey();
        if (field.isRepeated()) {
          for (final Object element : (List)entry.getValue()) {
            addRepeatedField(field, element);
          }
        } else if (field.getJavaType() == FieldDescriptor.JavaType.MESSAGE) {
          final Message existingValue = (Message)getField(field);
          if (existingValue == existingValue.getDefaultInstanceForType()) {
            setField(field, entry.getValue());
          } else {
            setField(field,
              existingValue.newBuilderForType()
                .mergeFrom(existingValue)
                .mergeFrom((Message)entry.getValue())
                .build());
          }
        } else {
          setField(field, entry.getValue());
        }
      }

      mergeUnknownFields(other.getUnknownFields());

      return (BuilderType) this;
    }

    @Override
    public BuilderType mergeFrom(final CodedInputStream input)
                                 throws IOException {
      return mergeFrom(input, ExtensionRegistry.getEmptyRegistry());
    }

    @Override
    public BuilderType mergeFrom(
        final CodedInputStream input,
        final ExtensionRegistryLite extensionRegistry)
        throws IOException {
      final UnknownFieldSet.Builder unknownFields =
        UnknownFieldSet.newBuilder(getUnknownFields());
      while (true) {
        final int tag = input.readTag();
        if (tag == 0) {
          break;
        }

        MessageReflection.BuilderAdapter builderAdapter =
            new MessageReflection.BuilderAdapter(this);
        if (!MessageReflection.mergeFieldFrom(input, unknownFields,
                                              extensionRegistry,
                                              getDescriptorForType(),
                                              builderAdapter,
                                              tag)) {
          // end group tag
          break;
        }
      }
      setUnknownFields(unknownFields.build());
      return (BuilderType) this;
    }

    public BuilderType mergeUnknownFields(final UnknownFieldSet unknownFields) {
      setUnknownFields(
        UnknownFieldSet.newBuilder(getUnknownFields())
                       .mergeFrom(unknownFields)
                       .build());
      return (BuilderType) this;
    }

    public Message.Builder getFieldBuilder(final FieldDescriptor field) {
      throw new UnsupportedOperationException(
          "getFieldBuilder() called on an unsupported message type.");
    }

    public String toString() {
      return TextFormat.printToString(this);
    }

    /**
     * Construct an UninitializedMessageException reporting missing fields in
     * the given message.
     */
    protected static UninitializedMessageException
        newUninitializedMessageException(Message message) {
      return new UninitializedMessageException(
          MessageReflection.findMissingFields(message));
    }

    // ===============================================================
    // The following definitions seem to be required in order to make javac
    // not produce weird errors like:
    //
    // java/com/google/protobuf/DynamicMessage.java:203: types
    //   com.google.protobuf.AbstractMessage.Builder<
    //     com.google.protobuf.DynamicMessage.Builder> and
    //   com.google.protobuf.AbstractMessage.Builder<
    //     com.google.protobuf.DynamicMessage.Builder> are incompatible; both
    //   define mergeFrom(com.google.protobuf.ByteString), but with unrelated
    //   return types.
    //
    // Strangely, these lines are only needed if javac is invoked separately
    // on AbstractMessage.java and AbstractMessageLite.java.  If javac is
    // invoked on both simultaneously, it works.  (Or maybe the important
    // point is whether or not DynamicMessage.java is compiled together with
    // AbstractMessageLite.java -- not sure.)  I suspect this is a compiler
    // bug.

    @Override
    public BuilderType mergeFrom(final ByteString data)
        throws InvalidProtocolBufferException {
      return super.mergeFrom(data);
    }

    @Override
    public BuilderType mergeFrom(
        final ByteString data,
        final ExtensionRegistryLite extensionRegistry)
        throws InvalidProtocolBufferException {
      return super.mergeFrom(data, extensionRegistry);
    }

    @Override
    public BuilderType mergeFrom(final byte[] data)
        throws InvalidProtocolBufferException {
      return super.mergeFrom(data);
    }

    @Override
    public BuilderType mergeFrom(
        final byte[] data, final int off, final int len)
        throws InvalidProtocolBufferException {
      return super.mergeFrom(data, off, len);
    }

    @Override
    public BuilderType mergeFrom(
        final byte[] data,
        final ExtensionRegistryLite extensionRegistry)
        throws InvalidProtocolBufferException {
      return super.mergeFrom(data, extensionRegistry);
    }

    @Override
    public BuilderType mergeFrom(
        final byte[] data, final int off, final int len,
        final ExtensionRegistryLite extensionRegistry)
        throws InvalidProtocolBufferException {
      return super.mergeFrom(data, off, len, extensionRegistry);
    }

    @Override
    public BuilderType mergeFrom(final InputStream input)
        throws IOException {
      return super.mergeFrom(input);
    }

    @Override
    public BuilderType mergeFrom(
        final InputStream input,
        final ExtensionRegistryLite extensionRegistry)
        throws IOException {
      return super.mergeFrom(input, extensionRegistry);
    }

    @Override
    public boolean mergeDelimitedFrom(final InputStream input)
        throws IOException {
      return super.mergeDelimitedFrom(input);
    }

    @Override
    public boolean mergeDelimitedFrom(
        final InputStream input,
        final ExtensionRegistryLite extensionRegistry)
        throws IOException {
      return super.mergeDelimitedFrom(input, extensionRegistry);
    }
  }
}
