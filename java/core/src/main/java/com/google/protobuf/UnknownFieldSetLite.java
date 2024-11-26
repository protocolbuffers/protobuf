// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import java.io.IOException;
import java.util.Arrays;

/**
 * {@code UnknownFieldSetLite} is used to keep track of fields which were seen when parsing a
 * protocol message but whose field numbers or types are unrecognized. This most frequently occurs
 * when new fields are added to a message type and then messages containing those fields are read by
 * old software that was compiled before the new types were added.
 *
 * <p>For use by generated code only.
 *
 * @author dweis@google.com (Daniel Weis)
 */
public final class UnknownFieldSetLite {

  // Arbitrarily chosen.
  // TODO: Tune this number?
  private static final int MIN_CAPACITY = 8;

  private static final UnknownFieldSetLite DEFAULT_INSTANCE =
      new UnknownFieldSetLite(0, new int[0], new Object[0], /* isMutable= */ false);

  /**
   * Get an empty {@code UnknownFieldSetLite}.
   *
   * <p>For use by generated code only.
   */
  public static UnknownFieldSetLite getDefaultInstance() {
    return DEFAULT_INSTANCE;
  }

  /** Returns a new mutable instance. */
  static UnknownFieldSetLite newInstance() {
    return new UnknownFieldSetLite();
  }

  /**
   * Returns a mutable {@code UnknownFieldSetLite} that is the composite of {@code first} and {@code
   * second}.
   */
  static UnknownFieldSetLite mutableCopyOf(UnknownFieldSetLite first, UnknownFieldSetLite second) {
    int count = first.count + second.count;
    int[] tags = Arrays.copyOf(first.tags, count);
    System.arraycopy(second.tags, 0, tags, first.count, second.count);
    Object[] objects = Arrays.copyOf(first.objects, count);
    System.arraycopy(second.objects, 0, objects, first.count, second.count);
    return new UnknownFieldSetLite(count, tags, objects, /* isMutable= */ true);
  }

  /** The number of elements in the set. */
  private int count;

  /** The tag numbers for the elements in the set. */
  private int[] tags;

  /** The boxed values of the elements in the set. */
  private Object[] objects;

  /** The lazily computed serialized size of the set. */
  private int memoizedSerializedSize = -1;

  /** Indicates that this object is mutable. */
  private boolean isMutable;

  /** Constructs a mutable {@code UnknownFieldSetLite}. */
  private UnknownFieldSetLite() {
    this(0, new int[MIN_CAPACITY], new Object[MIN_CAPACITY], /* isMutable= */ true);
  }

  /** Constructs the {@code UnknownFieldSetLite}. */
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
    if (this.isMutable) {
      this.isMutable = false;
    }
  }

  /** Throws an {@link UnsupportedOperationException} if immutable. */
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
   * Serializes the set and writes it to {@code output} using {@code MessageSet} wire format.
   *
   * <p>For use by generated code only.
   */
  public void writeAsMessageSetTo(CodedOutputStream output) throws IOException {
    for (int i = 0; i < count; i++) {
      int fieldNumber = WireFormat.getTagFieldNumber(tags[i]);
      output.writeRawMessageSetExtension(fieldNumber, (ByteString) objects[i]);
    }
  }

  /** Serializes the set and writes it to {@code writer} using {@code MessageSet} wire format. */
  void writeAsMessageSetTo(Writer writer) throws IOException {
    if (writer.fieldOrder() == Writer.FieldOrder.DESCENDING) {
      // Write fields in descending order.
      for (int i = count - 1; i >= 0; i--) {
        int fieldNumber = WireFormat.getTagFieldNumber(tags[i]);
        writer.writeMessageSetItem(fieldNumber, objects[i]);
      }
    } else {
      // Write fields in ascending order.
      for (int i = 0; i < count; i++) {
        int fieldNumber = WireFormat.getTagFieldNumber(tags[i]);
        writer.writeMessageSetItem(fieldNumber, objects[i]);
      }
    }
  }

  /** Serializes the set and writes it to {@code writer}. */
  public void writeTo(Writer writer) throws IOException {
    if (count == 0) {
      return;
    }

    // TODO: tags are not sorted, so there's no write order guarantees
    if (writer.fieldOrder() == Writer.FieldOrder.ASCENDING) {
      for (int i = 0; i < count; ++i) {
        writeField(tags[i], objects[i], writer);
      }
    } else {
      for (int i = count - 1; i >= 0; --i) {
        writeField(tags[i], objects[i], writer);
      }
    }
  }

  private static void writeField(int tag, Object object, Writer writer) throws IOException {
    int fieldNumber = WireFormat.getTagFieldNumber(tag);
    switch (WireFormat.getTagWireType(tag)) {
      case WireFormat.WIRETYPE_VARINT:
        writer.writeInt64(fieldNumber, (Long) object);
        break;
      case WireFormat.WIRETYPE_FIXED32:
        writer.writeFixed32(fieldNumber, (Integer) object);
        break;
      case WireFormat.WIRETYPE_FIXED64:
        writer.writeFixed64(fieldNumber, (Long) object);
        break;
      case WireFormat.WIRETYPE_LENGTH_DELIMITED:
        writer.writeBytes(fieldNumber, (ByteString) object);
        break;
      case WireFormat.WIRETYPE_START_GROUP:
        if (writer.fieldOrder() == Writer.FieldOrder.ASCENDING) {
          writer.writeStartGroup(fieldNumber);
          ((UnknownFieldSetLite) object).writeTo(writer);
          writer.writeEndGroup(fieldNumber);
        } else {
          writer.writeEndGroup(fieldNumber);
          ((UnknownFieldSetLite) object).writeTo(writer);
          writer.writeStartGroup(fieldNumber);
        }
        break;
      default:
        // TODO: Change writeTo to throw IOException?
        throw new RuntimeException(InvalidProtocolBufferException.invalidWireType());
    }
  }

  /**
   * Get the number of bytes required to encode this field, including field number, using {@code
   * MessageSet} wire format.
   */
  public int getSerializedSizeAsMessageSet() {
    int size = memoizedSerializedSize;
    if (size != -1) {
      return size;
    }

    size = 0;
    for (int i = 0; i < count; i++) {
      int tag = tags[i];
      int fieldNumber = WireFormat.getTagFieldNumber(tag);
      size +=
          CodedOutputStream.computeRawMessageSetExtensionSize(fieldNumber, (ByteString) objects[i]);
    }

    memoizedSerializedSize = size;

    return size;
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
          size +=
              CodedOutputStream.computeTagSize(fieldNumber) * 2
                  + ((UnknownFieldSetLite) objects[i]).getSerializedSize();
          break;
        default:
          throw new IllegalStateException(InvalidProtocolBufferException.invalidWireType());
      }
    }

    memoizedSerializedSize = size;

    return size;
  }

  private static boolean tagsEquals(int[] tags1, int[] tags2, int count) {
    for (int i = 0; i < count; ++i) {
      if (tags1[i] != tags2[i]) {
        return false;
      }
    }
    return true;
  }

  private static boolean objectsEquals(Object[] objects1, Object[] objects2, int count) {
    for (int i = 0; i < count; ++i) {
      if (!objects1[i].equals(objects2[i])) {
        return false;
      }
    }
    return true;
  }

  @Override
  public boolean equals(
          Object obj) {
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
        || !tagsEquals(tags, other.tags, count)
        || !objectsEquals(objects, other.objects, count)) {
      return false;
    }

    return true;
  }

  private static int hashCode(int[] tags, int count) {
    int hashCode = 17;
    for (int i = 0; i < count; ++i) {
      hashCode = 31 * hashCode + tags[i];
    }
    return hashCode;
  }

  private static int hashCode(Object[] objects, int count) {
    int hashCode = 17;
    for (int i = 0; i < count; ++i) {
      hashCode = 31 * hashCode + objects[i].hashCode();
    }
    return hashCode;
  }

  @Override
  public int hashCode() {
    int hashCode = 17;

    hashCode = 31 * hashCode + count;
    hashCode = 31 * hashCode + hashCode(tags, count);
    hashCode = 31 * hashCode + hashCode(objects, count);

    return hashCode;
  }

  /**
   * Prints a String representation of the unknown field set.
   *
   * <p>For use by generated code only.
   *
   * @param buffer the buffer to write to
   * @param indent the number of spaces the fields should be indented by
   */
  final void printWithIndent(StringBuilder buffer, int indent) {
    for (int i = 0; i < count; i++) {
      int fieldNumber = WireFormat.getTagFieldNumber(tags[i]);
      MessageLiteToString.printField(buffer, indent, String.valueOf(fieldNumber), objects[i]);
    }
  }

  // Package private for unsafe experimental runtime.
  void storeField(int tag, Object value) {
    checkMutable();
    ensureCapacity(count + 1);

    tags[count] = tag;
    objects[count] = value;
    count++;
  }

  /** Ensures that our arrays are long enough to store more metadata. */
  private void ensureCapacity(int minCapacity) {
    if (minCapacity > this.tags.length) {
      // Increase by at least 50%
      int newCapacity = count + count / 2;

      // Or new capacity if higher
      if (newCapacity < minCapacity) {
        newCapacity = minCapacity;
      }

      // And never less than MIN_CAPACITY
      if (newCapacity < MIN_CAPACITY) {
        newCapacity = MIN_CAPACITY;
      }

      this.tags = Arrays.copyOf(this.tags, newCapacity);
      this.objects = Arrays.copyOf(this.objects, newCapacity);
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
        input.checkLastTagWas(WireFormat.makeTag(fieldNumber, WireFormat.WIRETYPE_END_GROUP));
        storeField(tag, subFieldSet);
        return true;
      case WireFormat.WIRETYPE_END_GROUP:
        return false;
      default:
        throw InvalidProtocolBufferException.invalidWireType();
    }
  }

  /**
   * Convenience method for merging a new field containing a single varint value. This is used in
   * particular when an unknown enum value is encountered.
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

  /** Parse an entire message from {@code input} and merge its fields into this set. */
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

  @CanIgnoreReturnValue
  UnknownFieldSetLite mergeFrom(UnknownFieldSetLite other) {
    if (other.equals(getDefaultInstance())) {
      return this;
    }

    checkMutable();
    int newCount = this.count + other.count;
    ensureCapacity(newCount);
    System.arraycopy(other.tags, 0, tags, this.count, other.count);
    System.arraycopy(other.objects, 0, objects, this.count, other.count);
    this.count = newCount;
    return this;
  }
}
