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

import com.google.protobuf.Descriptors.EnumValueDescriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.Descriptors.OneofDescriptor;
import com.google.protobuf.Internal.EnumLite;
import java.io.IOException;
import java.io.InputStream;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

/**
 * A partial implementation of the {@link Message} interface which implements
 * as many methods of that interface as possible in terms of other methods.
 *
 * @author kenton@google.com Kenton Varda
 */
public abstract class AbstractMessage
    // TODO(dweis): Update GeneratedMessage to parameterize with MessageType and BuilderType.
    extends AbstractMessageLite
    implements Message {

  @Override
  public boolean isInitialized() {
    return MessageReflection.isInitialized(this);
  }

  /**
   * Interface for the parent of a Builder that allows the builder to
   * communicate invalidations back to the parent for use when using nested
   * builders.
   */
  protected interface BuilderParent {

    /**
     * A builder becomes dirty whenever a field is modified -- including fields
     * in nested builders -- and becomes clean when build() is called.  Thus,
     * when a builder becomes dirty, all its parents become dirty as well, and
     * when it becomes clean, all its children become clean.  The dirtiness
     * state is used to invalidate certain cached values.
     * <br>
     * To this end, a builder calls markDirty() on its parent whenever it
     * transitions from clean to dirty.  The parent must propagate this call to
     * its own parent, unless it was already dirty, in which case the
     * grandparent must necessarily already be dirty as well.  The parent can
     * only transition back to "clean" after calling build() on all children.
     */
    void markDirty();
  }

  /** Create a nested builder. */
  protected Message.Builder newBuilderForType(BuilderParent parent) {
    throw new UnsupportedOperationException("Nested builder is not supported for this type.");
  }


  @Override
  public List<String> findInitializationErrors() {
    return MessageReflection.findMissingFields(this);
  }

  @Override
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

  @Override
  public void writeTo(final CodedOutputStream output) throws IOException {
    MessageReflection.writeMessageTo(this, getAllFields(), output, false);
  }

  protected int memoizedSize = -1;

  @Override
  public int getSerializedSize() {
    int size = memoizedSize;
    if (size != -1) {
      return size;
    }

    memoizedSize = MessageReflection.getSerializedSize(this, getAllFields());
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
   * Converts a list of MapEntry messages into a Map used for equals() and
   * hashCode().
   */
  @SuppressWarnings({"rawtypes", "unchecked"})
  private static Map convertMapEntryListToMap(List list) {
    if (list.isEmpty()) {
      return Collections.emptyMap();
    }
    Map result = new HashMap();
    Iterator iterator = list.iterator();
    Message entry = (Message) iterator.next();
    Descriptors.Descriptor descriptor = entry.getDescriptorForType();
    Descriptors.FieldDescriptor key = descriptor.findFieldByName("key");
    Descriptors.FieldDescriptor value = descriptor.findFieldByName("value");
    Object fieldValue = entry.getField(value);
    if (fieldValue instanceof EnumValueDescriptor) {
      fieldValue = ((EnumValueDescriptor) fieldValue).getNumber();
    }
    result.put(entry.getField(key), fieldValue);
    while (iterator.hasNext()) {
      entry = (Message) iterator.next();
      fieldValue = entry.getField(value);
      if (fieldValue instanceof EnumValueDescriptor) {
        fieldValue = ((EnumValueDescriptor) fieldValue).getNumber();
      }
      result.put(entry.getField(key), fieldValue);
    }
    return result;
  }
  
  /**
   * Compares two map fields. The parameters must be a list of MapEntry
   * messages.
   */
  @SuppressWarnings({"rawtypes", "unchecked"})
  private static boolean compareMapField(Object a, Object b) {
    Map ma = convertMapEntryListToMap((List) a);
    Map mb = convertMapEntryListToMap((List) b);
    return MapFieldLite.equals(ma, mb);
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
      } else if (descriptor.isMapField()) {
        if (!compareMapField(value1, value2)) {
          return false;
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
  
  /**
   * Calculates the hash code of a map field. {@code value} must be a list of
   * MapEntry messages.
   */
  @SuppressWarnings("unchecked")
  private static int hashMapField(Object value) {
    return MapFieldLite.calculateHashCodeForMap(convertMapEntryListToMap((List) value));
  }

  /** Get a hash code for given fields and values, using the given seed. */
  @SuppressWarnings("unchecked")
  protected static int hashFields(int hash, Map<FieldDescriptor, Object> map) {
    for (Map.Entry<FieldDescriptor, Object> entry : map.entrySet()) {
      FieldDescriptor field = entry.getKey();
      Object value = entry.getValue();
      hash = (37 * hash) + field.getNumber();
      if (field.isMapField()) {
        hash = (53 * hash) + hashMapField(value);
      } else if (field.getType() != FieldDescriptor.Type.ENUM){
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
  public static abstract class Builder<BuilderType extends Builder<BuilderType>>
      extends AbstractMessageLite.Builder
      implements Message.Builder {
    // The compiler produces an error if this is not declared explicitly.
    // Method isn't abstract to bypass Java 1.6 compiler issue:
    //     http://bugs.java.com/view_bug.do?bug_id=6908259
    @Override
    public BuilderType clone() {
      throw new UnsupportedOperationException("clone() should be implemented in subclasses.");
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

    /** TODO(jieluo): Clear it when all subclasses have implemented this method. */
    @Override
    public BuilderType clearOneof(OneofDescriptor oneof) {
      throw new UnsupportedOperationException("clearOneof() is not implemented.");
    }

    @Override
    public BuilderType clear() {
      for (final Map.Entry<FieldDescriptor, Object> entry :
           getAllFields().entrySet()) {
        clearField(entry.getKey());
      }
      return (BuilderType) this;
    }

    @Override
    public List<String> findInitializationErrors() {
      return MessageReflection.findMissingFields(this);
    }

    @Override
    public String getInitializationErrorString() {
      return MessageReflection.delimitWithCommas(findInitializationErrors());
    }
    
    @Override
    protected BuilderType internalMergeFrom(AbstractMessageLite other) {
      return mergeFrom((Message) other);
    }

    @Override
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

    @Override
    public BuilderType mergeUnknownFields(final UnknownFieldSet unknownFields) {
      setUnknownFields(
        UnknownFieldSet.newBuilder(getUnknownFields())
                       .mergeFrom(unknownFields)
                       .build());
      return (BuilderType) this;
    }

    @Override
    public Message.Builder getFieldBuilder(final FieldDescriptor field) {
      throw new UnsupportedOperationException(
          "getFieldBuilder() called on an unsupported message type.");
    }

    @Override
    public Message.Builder getRepeatedFieldBuilder(final FieldDescriptor field, int index) {
      throw new UnsupportedOperationException(
          "getRepeatedFieldBuilder() called on an unsupported message type.");
    }

    @Override
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

    /**
     * Used to support nested builders and called to mark this builder as clean.
     * Clean builders will propagate the {@link BuilderParent#markDirty()} event
     * to their parent builders, while dirty builders will not, as their parents
     * should be dirty already.
     *
     * NOTE: Implementations that don't support nested builders don't need to
     * override this method.
     */
    void markClean() {
      throw new IllegalStateException("Should be overridden by subclasses.");
    }

    /**
     * Used to support nested builders and called when this nested builder is
     * no longer used by its parent builder and should release the reference
     * to its parent builder.
     *
     * NOTE: Implementations that don't support nested builders don't need to
     * override this method.
     */
    void dispose() {
      throw new IllegalStateException("Should be overridden by subclasses.");
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
      return (BuilderType) super.mergeFrom(data);
    }

    @Override
    public BuilderType mergeFrom(
        final ByteString data,
        final ExtensionRegistryLite extensionRegistry)
        throws InvalidProtocolBufferException {
      return (BuilderType) super.mergeFrom(data, extensionRegistry);
    }

    @Override
    public BuilderType mergeFrom(final byte[] data)
        throws InvalidProtocolBufferException {
      return (BuilderType) super.mergeFrom(data);
    }

    @Override
    public BuilderType mergeFrom(
        final byte[] data, final int off, final int len)
        throws InvalidProtocolBufferException {
      return (BuilderType) super.mergeFrom(data, off, len);
    }

    @Override
    public BuilderType mergeFrom(
        final byte[] data,
        final ExtensionRegistryLite extensionRegistry)
        throws InvalidProtocolBufferException {
      return (BuilderType) super.mergeFrom(data, extensionRegistry);
    }

    @Override
    public BuilderType mergeFrom(
        final byte[] data, final int off, final int len,
        final ExtensionRegistryLite extensionRegistry)
        throws InvalidProtocolBufferException {
      return (BuilderType) super.mergeFrom(data, off, len, extensionRegistry);
    }

    @Override
    public BuilderType mergeFrom(final InputStream input)
        throws IOException {
      return (BuilderType) super.mergeFrom(input);
    }

    @Override
    public BuilderType mergeFrom(
        final InputStream input,
        final ExtensionRegistryLite extensionRegistry)
        throws IOException {
      return (BuilderType) super.mergeFrom(input, extensionRegistry);
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

  /**
   * @deprecated from v3.0.0-beta-3+, for compatibility with v2.5.0 and v2.6.1
   * generated code.
   */
  @Deprecated
  protected static int hashLong(long n) {
    return (int) (n ^ (n >>> 32));
  }
  //
  /**
   * @deprecated from v3.0.0-beta-3+, for compatibility with v2.5.0 and v2.6.1
   * generated code.
   */
  @Deprecated
  protected static int hashBoolean(boolean b) {
    return b ? 1231 : 1237;
  }
  //
  /**
   * @deprecated from v3.0.0-beta-3+, for compatibility with v2.5.0 and v2.6.1
   * generated code.
   */
  @Deprecated
  protected static int hashEnum(EnumLite e) {
    return e.getNumber();
  }
  //
  /**
   * @deprecated from v3.0.0-beta-3+, for compatibility with v2.5.0 and v2.6.1
   * generated code.
   */
  @Deprecated
  protected static int hashEnumList(List<? extends EnumLite> list) {
    int hash = 1;
    for (EnumLite e : list) {
      hash = 31 * hash + hashEnum(e);
    }
    return hash;
  }
}
