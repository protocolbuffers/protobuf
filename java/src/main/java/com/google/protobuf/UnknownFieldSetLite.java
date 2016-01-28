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

import java.io.IOException;
import java.util.Arrays;

/**
 * {@code UnknownFieldSetLite} is used to keep track of fields which were seen
 * when parsing a protocol message but whose field numbers or types are
 * unrecognized. This most frequently occurs when new fields are added to a
 * message type and then messages containing those fields are read by old
 * software that was compiled before the new types were added.
 *
 * <p>For use by generated code only.
 *
 * @author dweis@google.com (Daniel Weis)
 */
public final class UnknownFieldSetLite {
  
  // Arbitrarily chosen.
  // TODO(dweis): Tune this number?
  private static final int MIN_CAPACITY = 8;

  private static final UnknownFieldSetLite DEFAULT_INSTANCE =
      new UnknownFieldSetLite(0, new int[0], new Object[0], false /* isMutable */);

  /**
   * Get an empty {@code UnknownFieldSetLite}.
   *
   * <p>For use by generated code only.
   */
  public static UnknownFieldSetLite getDefaultInstance() {
    return DEFAULT_INSTANCE;
  }

  /**
   * Returns an empty {@code UnknownFieldSetLite.Builder}.
   *
   * <p>For use by generated code only.
   */
  public static Builder newBuilder() {
    return new Builder();
  }
  
  /**
   * Returns a new mutable instance.
   */
  static UnknownFieldSetLite newInstance() {
    return new UnknownFieldSetLite();
  }

  /**
   * Returns a mutable {@code UnknownFieldSetLite} that is the composite of {@code first} and
   * {@code second}.
   */
  static UnknownFieldSetLite mutableCopyOf(UnknownFieldSetLite first, UnknownFieldSetLite second) {
    int count = first.count + second.count;
    int[] tags = Arrays.copyOf(first.tags, count);
    System.arraycopy(second.tags, 0, tags, first.count, second.count);
    Object[] objects = Arrays.copyOf(first.objects, count);
    System.arraycopy(second.objects, 0, objects, first.count, second.count);
    return new UnknownFieldSetLite(count, tags, objects, true /* isMutable */);
  }
  
  /**
   * The number of elements in the set.
   */
  private int count;
  
  /**
   * The tag numbers for the elements in the set.
   */
  private int[] tags;
  
  /**
   * The boxed values of the elements in the set.
   */
  private Object[] objects;
  
  /**
   * The lazily computed serialized size of the set.
   */
  private int memoizedSerializedSize = -1;
  
  /**
   * Indicates that this object is mutable. 
   */
  private boolean isMutable;

  /**
   * Constructs a mutable {@code UnknownFieldSetLite}.
   */
  private UnknownFieldSetLite() {
    this(0, new int[MIN_CAPACITY], new Object[MIN_CAPACITY], true /* isMutable */);
  }
  
  /**
   * Constructs the {@code UnknownFieldSetLite}.
   */
  private UnknownFieldSetLite(int count, int[] tags, Object[] objects, boolean isMutable) {
    this.count = count;
    this.tags = tags;
    this.objects = objects;
    this.isMutable = isMutable;
  }
  
  /**
   * Marks this object as immutable.
   * 
   * <p>Future calls to methods that attempt to modify this object will throw.
   */
  public void makeImmutable() {
    this.isMutable = false;
  }
  
  /**
   * Throws an {@link UnsupportedOperationException} if immutable.
   */
  void checkMutable() {
    if (!isMutable) {
      throw new UnsupportedOperationException(); 
    }
  }

  /**
   * Serializes the set and writes it to {@code output}.
   *
   * <p>For use by generated code only.
   */
  public void writeTo(CodedOutputStream output) throws IOException {
    for (int i = 0; i < count; i++) {
      int tag = tags[i];
      int fieldNumber = WireFormat.getTagFieldNumber(tag);
      switch (WireFormat.getTagWireType(tag)) {
        case WireFormat.WIRETYPE_VARINT:
          output.writeUInt64(fieldNumber, (Long) objects[i]);
          break;
        case WireFormat.WIRETYPE_FIXED32:
          output.writeFixed32(fieldNumber, (Integer) objects[i]);
          break;
        case WireFormat.WIRETYPE_FIXED64:
          output.writeFixed64(fieldNumber, (Long) objects[i]);
          break;
        case WireFormat.WIRETYPE_LENGTH_DELIMITED:
          output.writeBytes(fieldNumber, (ByteString) objects[i]);
          break;
        case WireFormat.WIRETYPE_START_GROUP:
          output.writeTag(fieldNumber, WireFormat.WIRETYPE_START_GROUP);
          ((UnknownFieldSetLite) objects[i]).writeTo(output);
          output.writeTag(fieldNumber, WireFormat.WIRETYPE_END_GROUP);
          break;
        default:
          throw InvalidProtocolBufferException.invalidWireType();
      }
    }
  }

  /**
   * Get the number of bytes required to encode this set.
   *
   * <p>For use by generated code only.
   */
  public int getSerializedSize() {
    int size = memoizedSerializedSize;
    if (size != -1) {
      return size;
    }
    
    size = 0;
    for (int i = 0; i < count; i++) {
      int tag = tags[i];
      int fieldNumber = WireFormat.getTagFieldNumber(tag);
      switch (WireFormat.getTagWireType(tag)) {
        case WireFormat.WIRETYPE_VARINT:
          size += CodedOutputStream.computeUInt64Size(fieldNumber, (Long) objects[i]);
          break;
        case WireFormat.WIRETYPE_FIXED32:
          size += CodedOutputStream.computeFixed32Size(fieldNumber, (Integer) objects[i]);
          break;
        case WireFormat.WIRETYPE_FIXED64:
          size += CodedOutputStream.computeFixed64Size(fieldNumber, (Long) objects[i]);
          break;
        case WireFormat.WIRETYPE_LENGTH_DELIMITED:
          size += CodedOutputStream.computeBytesSize(fieldNumber, (ByteString) objects[i]);
          break;
        case WireFormat.WIRETYPE_START_GROUP:
          size +=  CodedOutputStream.computeTagSize(fieldNumber) * 2
              + ((UnknownFieldSetLite) objects[i]).getSerializedSize();
          break;
        default:
          throw new IllegalStateException(InvalidProtocolBufferException.invalidWireType());
      }
    }
    
    memoizedSerializedSize = size;
    
    return size;
  }

  @Override
  public boolean equals(Object obj) {
    if (this == obj) {
      return true;
    }

    if (obj == null) {
      return false;
    }

    if (!(obj instanceof UnknownFieldSetLite)) {
      return false;
    }
    
    UnknownFieldSetLite other = (UnknownFieldSetLite) obj;    
    if (count != other.count
        // TODO(dweis): Only have to compare up to count but at worst 2x worse than we need to do.
        || !Arrays.equals(tags, other.tags)
        || !Arrays.deepEquals(objects, other.objects)) {
      return false;
    }

    return true;
  }

  @Override
  public int hashCode() {
    int hashCode = 17;
    
    hashCode = 31 * hashCode + count;
    hashCode = 31 * hashCode + Arrays.hashCode(tags);
    hashCode = 31 * hashCode + Arrays.deepHashCode(objects);
    
    return hashCode;
  }

  private void storeField(int tag, Object value) {
    ensureCapacity();
    
    tags[count] = tag;
    objects[count] = value;
    count++;
  }
  
  /**
   * Ensures that our arrays are long enough to store more metadata.
   */
  private void ensureCapacity() {
    if (count == tags.length) {        
      int increment = count < (MIN_CAPACITY / 2) ? MIN_CAPACITY : count >> 1;
      int newLength = count + increment;
        
      tags = Arrays.copyOf(tags, newLength);
      objects = Arrays.copyOf(objects, newLength);
    }
  }
  
  /**
   * Parse a single field from {@code input} and merge it into this set.
   *
   * <p>For use by generated code only.
   *
   * @param tag The field's tag number, which was already parsed.
   * @return {@code false} if the tag is an end group tag.
   */
  boolean mergeFieldFrom(final int tag, final CodedInputStream input) throws IOException {
    checkMutable();
    final int fieldNumber = WireFormat.getTagFieldNumber(tag);
    switch (WireFormat.getTagWireType(tag)) {
      case WireFormat.WIRETYPE_VARINT:
        storeField(tag, input.readInt64());
        return true;
      case WireFormat.WIRETYPE_FIXED32:
        storeField(tag, input.readFixed32());
        return true;
      case WireFormat.WIRETYPE_FIXED64:
        storeField(tag, input.readFixed64());
        return true;
      case WireFormat.WIRETYPE_LENGTH_DELIMITED:
        storeField(tag, input.readBytes());
        return true;
      case WireFormat.WIRETYPE_START_GROUP:
        final UnknownFieldSetLite subFieldSet = new UnknownFieldSetLite();
        subFieldSet.mergeFrom(input);
        input.checkLastTagWas(
            WireFormat.makeTag(fieldNumber, WireFormat.WIRETYPE_END_GROUP));
        storeField(tag, subFieldSet);
        return true;
      case WireFormat.WIRETYPE_END_GROUP:
        return false;
      default:
        throw InvalidProtocolBufferException.invalidWireType();
    }
  }

  /**
   * Convenience method for merging a new field containing a single varint
   * value. This is used in particular when an unknown enum value is
   * encountered.
   *
   * <p>For use by generated code only.
   */
  UnknownFieldSetLite mergeVarintField(int fieldNumber, int value) {
    checkMutable();
    if (fieldNumber == 0) {
      throw new IllegalArgumentException("Zero is not a valid field number.");
    }

    storeField(WireFormat.makeTag(fieldNumber, WireFormat.WIRETYPE_VARINT), (long) value);
    
    return this;
  }

  /**
   * Convenience method for merging a length-delimited field.
   *
   * <p>For use by generated code only.
   */
  UnknownFieldSetLite mergeLengthDelimitedField(final int fieldNumber, final ByteString value) {  
    checkMutable();
    if (fieldNumber == 0) {
      throw new IllegalArgumentException("Zero is not a valid field number.");
    }

    storeField(WireFormat.makeTag(fieldNumber, WireFormat.WIRETYPE_LENGTH_DELIMITED), value);
    
    return this;
  }
  
  /**
   * Parse an entire message from {@code input} and merge its fields into
   * this set.
   */
  private UnknownFieldSetLite mergeFrom(final CodedInputStream input) throws IOException {
    // Ensures initialization in mergeFieldFrom.
    while (true) {
      final int tag = input.readTag();
      if (tag == 0 || !mergeFieldFrom(tag, input)) {
        break;
      }
    }
    return this;
  }
  
  /**
   * Builder for {@link UnknownFieldSetLite}s.
   *
   * <p>Use {@link UnknownFieldSet#newBuilder()} to construct a {@code Builder}.
   *
   * <p>For use by generated code only.
   */
  // TODO(dweis): Update the mutable API to no longer need this builder and delete.
  public static final class Builder {

    private UnknownFieldSetLite set;
    
    private Builder() {
      this.set = null;
    }

    /**
     * Ensures internal state is initialized for use.
     */
    private void ensureNotBuilt() {
      if (set == null) {
        set = new UnknownFieldSetLite();
      }
      
      set.checkMutable();
    }
    
    /**
     * Parse a single field from {@code input} and merge it into this set.
     *
     * <p>For use by generated code only.
     *
     * @param tag The field's tag number, which was already parsed.
     * @return {@code false} if the tag is an end group tag.
     */
    boolean mergeFieldFrom(final int tag, final CodedInputStream input) throws IOException {
      ensureNotBuilt();
      return set.mergeFieldFrom(tag, input);
    }

    /**
     * Convenience method for merging a new field containing a single varint
     * value. This is used in particular when an unknown enum value is
     * encountered.
     *
     * <p>For use by generated code only.
     */
    Builder mergeVarintField(int fieldNumber, int value) {
      ensureNotBuilt();
      set.mergeVarintField(fieldNumber, value);
      return this;
    }
    
    /**
     * Convenience method for merging a length-delimited field.
     *
     * <p>For use by generated code only.
     */
    public Builder mergeLengthDelimitedField(final int fieldNumber, final ByteString value) {
      ensureNotBuilt();  
      set.mergeLengthDelimitedField(fieldNumber, value);
      return this;
    }
    
    /**
     * Build the {@link UnknownFieldSetLite} and return it.
     *
     * <p>Once {@code build()} has been called, the {@code Builder} will no
     * longer be usable.  Calling any method after {@code build()} will result
     * in undefined behavior and can cause an
     * {@code UnsupportedOperationException} to be thrown.
     *
     * <p>For use by generated code only.
     */
    public UnknownFieldSetLite build() {
      if (set == null) {
        return DEFAULT_INSTANCE;
      }
      
      set.checkMutable();
      set.makeImmutable();

      return set;
    }
  }
}
