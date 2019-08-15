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

import com.google.protobuf.LazyField.LazyIterator;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

/**
 * A class which represents an arbitrary set of fields of some message type. This is used to
 * implement {@link DynamicMessage}, and also to represent extensions in {@link GeneratedMessage}.
 * This class is package-private, since outside users should probably be using {@link
 * DynamicMessage}.
 *
 * @author kenton@google.com Kenton Varda
 */
final class FieldSet<T extends FieldSet.FieldDescriptorLite<T>> {
  /**
   * Interface for a FieldDescriptor or lite extension descriptor. This prevents FieldSet from
   * depending on {@link Descriptors.FieldDescriptor}.
   */
  public interface FieldDescriptorLite<T extends FieldDescriptorLite<T>> extends Comparable<T> {
    int getNumber();

    WireFormat.FieldType getLiteType();

    WireFormat.JavaType getLiteJavaType();

    boolean isRepeated();

    boolean isPacked();

    Internal.EnumLiteMap<?> getEnumType();

    // If getLiteJavaType() == MESSAGE, this merges a message object of the
    // type into a builder of the type.  Returns {@code to}.
    MessageLite.Builder internalMergeFrom(MessageLite.Builder to, MessageLite from);
  }

  private static final int DEFAULT_FIELD_MAP_ARRAY_SIZE = 16;

  private final SmallSortedMap<T, Object> fields;
  private boolean isImmutable;
  private boolean hasLazyField;

  /** Construct a new FieldSet. */
  private FieldSet() {
    this.fields = SmallSortedMap.newFieldMap(DEFAULT_FIELD_MAP_ARRAY_SIZE);
  }

  /** Construct an empty FieldSet. This is only used to initialize DEFAULT_INSTANCE. */
  @SuppressWarnings("unused")
  private FieldSet(final boolean dummy) {
    this(SmallSortedMap.<T>newFieldMap(0));
    makeImmutable();
  }

  private FieldSet(SmallSortedMap<T, Object> fields) {
    this.fields = fields;
    makeImmutable();
  }

  /** Construct a new FieldSet. */
  public static <T extends FieldSet.FieldDescriptorLite<T>> FieldSet<T> newFieldSet() {
    return new FieldSet<T>();
  }

  /** Get an immutable empty FieldSet. */
  @SuppressWarnings("unchecked")
  public static <T extends FieldSet.FieldDescriptorLite<T>> FieldSet<T> emptySet() {
    return DEFAULT_INSTANCE;
  }

  /** Construct a new Builder. */
  public static <T extends FieldDescriptorLite<T>> Builder<T> newBuilder() {
    return new Builder<T>();
  }

  @SuppressWarnings("rawtypes")
  private static final FieldSet DEFAULT_INSTANCE = new FieldSet(true);

  /** Returns {@code true} if empty, {@code false} otherwise. */
  boolean isEmpty() {
    return fields.isEmpty();
  }

  /** Make this FieldSet immutable from this point forward. */
  @SuppressWarnings("unchecked")
  public void makeImmutable() {
    if (isImmutable) {
      return;
    }
    fields.makeImmutable();
    isImmutable = true;
  }

  /**
   * Returns whether the FieldSet is immutable. This is true if it is the {@link #emptySet} or if
   * {@link #makeImmutable} were called.
   *
   * @return whether the FieldSet is immutable.
   */
  public boolean isImmutable() {
    return isImmutable;
  }

  @Override
  public boolean equals(Object o) {
    if (this == o) {
      return true;
    }

    if (!(o instanceof FieldSet)) {
      return false;
    }

    FieldSet<?> other = (FieldSet<?>) o;
    return fields.equals(other.fields);
  }

  @Override
  public int hashCode() {
    return fields.hashCode();
  }

  /**
   * Clones the FieldSet. The returned FieldSet will be mutable even if the original FieldSet was
   * immutable.
   *
   * @return the newly cloned FieldSet
   */
  @Override
  public FieldSet<T> clone() {
    // We can't just call fields.clone because List objects in the map
    // should not be shared.
    FieldSet<T> clone = FieldSet.newFieldSet();
    for (int i = 0; i < fields.getNumArrayEntries(); i++) {
      Map.Entry<T, Object> entry = fields.getArrayEntryAt(i);
      clone.setField(entry.getKey(), entry.getValue());
    }
    for (Map.Entry<T, Object> entry : fields.getOverflowEntries()) {
      clone.setField(entry.getKey(), entry.getValue());
    }
    clone.hasLazyField = hasLazyField;
    return clone;
  }


  // =================================================================

  /** See {@link Message.Builder#clear()}. */
  public void clear() {
    fields.clear();
    hasLazyField = false;
  }

  /** Get a simple map containing all the fields. */
  public Map<T, Object> getAllFields() {
    if (hasLazyField) {
      SmallSortedMap<T, Object> result = cloneAllFieldsMap(fields, /* copyList */ false);
      if (fields.isImmutable()) {
        result.makeImmutable();
      }
      return result;
    }
    return fields.isImmutable() ? fields : Collections.unmodifiableMap(fields);
  }

  private static <T extends FieldDescriptorLite<T>> SmallSortedMap<T, Object> cloneAllFieldsMap(
      SmallSortedMap<T, Object> fields, boolean copyList) {
    SmallSortedMap<T, Object> result = SmallSortedMap.newFieldMap(DEFAULT_FIELD_MAP_ARRAY_SIZE);
    for (int i = 0; i < fields.getNumArrayEntries(); i++) {
      cloneFieldEntry(result, fields.getArrayEntryAt(i), copyList);
    }
    for (Map.Entry<T, Object> entry : fields.getOverflowEntries()) {
      cloneFieldEntry(result, entry, copyList);
    }
    return result;
  }

  private static <T extends FieldDescriptorLite<T>> void cloneFieldEntry(
      Map<T, Object> map, Map.Entry<T, Object> entry, boolean copyList) {
    T key = entry.getKey();
    Object value = entry.getValue();
    if (value instanceof LazyField) {
      map.put(key, ((LazyField) value).getValue());
    } else if (copyList && value instanceof List) {
      map.put(key, new ArrayList<>((List<?>) value));
    } else {
      map.put(key, value);
    }
  }

  /**
   * Get an iterator to the field map. This iterator should not be leaked out of the protobuf
   * library as it is not protected from mutation when fields is not immutable.
   */
  public Iterator<Map.Entry<T, Object>> iterator() {
    if (hasLazyField) {
      return new LazyIterator<T>(fields.entrySet().iterator());
    }
    return fields.entrySet().iterator();
  }

  /**
   * Get an iterator over the fields in the map in descending (i.e. reverse) order. This iterator
   * should not be leaked out of the protobuf library as it is not protected from mutation when
   * fields is not immutable.
   */
  Iterator<Map.Entry<T, Object>> descendingIterator() {
    if (hasLazyField) {
      return new LazyIterator<T>(fields.descendingEntrySet().iterator());
    }
    return fields.descendingEntrySet().iterator();
  }

  /** Useful for implementing {@link Message#hasField(Descriptors.FieldDescriptor)}. */
  public boolean hasField(final T descriptor) {
    if (descriptor.isRepeated()) {
      throw new IllegalArgumentException("hasField() can only be called on non-repeated fields.");
    }

    return fields.get(descriptor) != null;
  }

  /**
   * Useful for implementing {@link Message#getField(Descriptors.FieldDescriptor)}. This method
   * returns {@code null} if the field is not set; in this case it is up to the caller to fetch the
   * field's default value.
   */
  public Object getField(final T descriptor) {
    Object o = fields.get(descriptor);
    if (o instanceof LazyField) {
      return ((LazyField) o).getValue();
    }
    return o;
  }

  /**
   * Useful for implementing {@link Message.Builder#setField(Descriptors.FieldDescriptor,Object)}.
   */
  @SuppressWarnings({"unchecked", "rawtypes"})
  public void setField(final T descriptor, Object value) {
    if (descriptor.isRepeated()) {
      if (!(value instanceof List)) {
        throw new IllegalArgumentException(
            "Wrong object type used with protocol message reflection.");
      }

      // Wrap the contents in a new list so that the caller cannot change
      // the list's contents after setting it.
      final List newList = new ArrayList();
      newList.addAll((List) value);
      for (final Object element : newList) {
        verifyType(descriptor.getLiteType(), element);
      }
      value = newList;
    } else {
      verifyType(descriptor.getLiteType(), value);
    }

    if (value instanceof LazyField) {
      hasLazyField = true;
    }
    fields.put(descriptor, value);
  }

  /** Useful for implementing {@link Message.Builder#clearField(Descriptors.FieldDescriptor)}. */
  public void clearField(final T descriptor) {
    fields.remove(descriptor);
    if (fields.isEmpty()) {
      hasLazyField = false;
    }
  }

  /** Useful for implementing {@link Message#getRepeatedFieldCount(Descriptors.FieldDescriptor)}. */
  public int getRepeatedFieldCount(final T descriptor) {
    if (!descriptor.isRepeated()) {
      throw new IllegalArgumentException(
          "getRepeatedField() can only be called on repeated fields.");
    }

    final Object value = getField(descriptor);
    if (value == null) {
      return 0;
    } else {
      return ((List<?>) value).size();
    }
  }

  /** Useful for implementing {@link Message#getRepeatedField(Descriptors.FieldDescriptor,int)}. */
  public Object getRepeatedField(final T descriptor, final int index) {
    if (!descriptor.isRepeated()) {
      throw new IllegalArgumentException(
          "getRepeatedField() can only be called on repeated fields.");
    }

    final Object value = getField(descriptor);

    if (value == null) {
      throw new IndexOutOfBoundsException();
    } else {
      return ((List<?>) value).get(index);
    }
  }

  /**
   * Useful for implementing {@link
   * Message.Builder#setRepeatedField(Descriptors.FieldDescriptor,int,Object)}.
   */
  @SuppressWarnings("unchecked")
  public void setRepeatedField(final T descriptor, final int index, final Object value) {
    if (!descriptor.isRepeated()) {
      throw new IllegalArgumentException(
          "getRepeatedField() can only be called on repeated fields.");
    }

    final Object list = getField(descriptor);
    if (list == null) {
      throw new IndexOutOfBoundsException();
    }

    verifyType(descriptor.getLiteType(), value);
    ((List<Object>) list).set(index, value);
  }

  /**
   * Useful for implementing {@link
   * Message.Builder#addRepeatedField(Descriptors.FieldDescriptor,Object)}.
   */
  @SuppressWarnings("unchecked")
  public void addRepeatedField(final T descriptor, final Object value) {
    if (!descriptor.isRepeated()) {
      throw new IllegalArgumentException(
          "addRepeatedField() can only be called on repeated fields.");
    }

    verifyType(descriptor.getLiteType(), value);

    final Object existingValue = getField(descriptor);
    List<Object> list;
    if (existingValue == null) {
      list = new ArrayList<Object>();
      fields.put(descriptor, list);
    } else {
      list = (List<Object>) existingValue;
    }

    list.add(value);
  }

  /**
   * Verifies that the given object is of the correct type to be a valid value for the given field.
   * (For repeated fields, this checks if the object is the right type to be one element of the
   * field.)
   *
   * @throws IllegalArgumentException The value is not of the right type.
   */
  private void verifyType(final WireFormat.FieldType type, final Object value) {
    if (!isValidType(type, value)) {
      // TODO(kenton):  When chaining calls to setField(), it can be hard to
      //   tell from the stack trace which exact call failed, since the whole
      //   chain is considered one line of code.  It would be nice to print
      //   more information here, e.g. naming the field.  We used to do that.
      //   But we can't now that FieldSet doesn't use descriptors.  Maybe this
      //   isn't a big deal, though, since it would only really apply when using
      //   reflection and generally people don't chain reflection setters.
      throw new IllegalArgumentException(
          "Wrong object type used with protocol message reflection.");
    }
  }

  private static boolean isValidType(final WireFormat.FieldType type, final Object value) {
    checkNotNull(value);
    switch (type.getJavaType()) {
      case INT:
        return value instanceof Integer;
      case LONG:
        return value instanceof Long;
      case FLOAT:
        return value instanceof Float;
      case DOUBLE:
        return value instanceof Double;
      case BOOLEAN:
        return value instanceof Boolean;
      case STRING:
        return value instanceof String;
      case BYTE_STRING:
        return value instanceof ByteString || value instanceof byte[];
      case ENUM:
        // TODO(kenton):  Caller must do type checking here, I guess.
        return (value instanceof Integer || value instanceof Internal.EnumLite);
      case MESSAGE:
        // TODO(kenton):  Caller must do type checking here, I guess.
        return (value instanceof MessageLite) || (value instanceof LazyField);
    }
    return false;
  }

  // =================================================================
  // Parsing and serialization

  /**
   * See {@link Message#isInitialized()}. Note: Since {@code FieldSet} itself does not have any way
   * of knowing about required fields that aren't actually present in the set, it is up to the
   * caller to check that all required fields are present.
   */
  public boolean isInitialized() {
    for (int i = 0; i < fields.getNumArrayEntries(); i++) {
      if (!isInitialized(fields.getArrayEntryAt(i))) {
        return false;
      }
    }
    for (final Map.Entry<T, Object> entry : fields.getOverflowEntries()) {
      if (!isInitialized(entry)) {
        return false;
      }
    }
    return true;
  }

  @SuppressWarnings("unchecked")
  private static <T extends FieldDescriptorLite<T>> boolean isInitialized(
      final Map.Entry<T, Object> entry) {
    final T descriptor = entry.getKey();
    if (descriptor.getLiteJavaType() == WireFormat.JavaType.MESSAGE) {
      if (descriptor.isRepeated()) {
        for (final MessageLite element : (List<MessageLite>) entry.getValue()) {
          if (!element.isInitialized()) {
            return false;
          }
        }
      } else {
        Object value = entry.getValue();
        if (value instanceof MessageLite) {
          if (!((MessageLite) value).isInitialized()) {
            return false;
          }
        } else if (value instanceof LazyField) {
          return true;
        } else {
          throw new IllegalArgumentException(
              "Wrong object type used with protocol message reflection.");
        }
      }
    }
    return true;
  }

  /**
   * Given a field type, return the wire type.
   *
   * @return One of the {@code WIRETYPE_} constants defined in {@link WireFormat}.
   */
  static int getWireFormatForFieldType(final WireFormat.FieldType type, boolean isPacked) {
    if (isPacked) {
      return WireFormat.WIRETYPE_LENGTH_DELIMITED;
    } else {
      return type.getWireType();
    }
  }

  /** Like {@link Message.Builder#mergeFrom(Message)}, but merges from another {@link FieldSet}. */
  public void mergeFrom(final FieldSet<T> other) {
    for (int i = 0; i < other.fields.getNumArrayEntries(); i++) {
      mergeFromField(other.fields.getArrayEntryAt(i));
    }
    for (final Map.Entry<T, Object> entry : other.fields.getOverflowEntries()) {
      mergeFromField(entry);
    }
  }

  private static Object cloneIfMutable(Object value) {
    if (value instanceof byte[]) {
      byte[] bytes = (byte[]) value;
      byte[] copy = new byte[bytes.length];
      System.arraycopy(bytes, 0, copy, 0, bytes.length);
      return copy;
    } else {
      return value;
    }
  }

  @SuppressWarnings({"unchecked", "rawtypes"})
  private void mergeFromField(final Map.Entry<T, Object> entry) {
    final T descriptor = entry.getKey();
    Object otherValue = entry.getValue();
    if (otherValue instanceof LazyField) {
      otherValue = ((LazyField) otherValue).getValue();
    }

    if (descriptor.isRepeated()) {
      Object value = getField(descriptor);
      if (value == null) {
        value = new ArrayList();
      }
      for (Object element : (List) otherValue) {
        ((List) value).add(cloneIfMutable(element));
      }
      fields.put(descriptor, value);
    } else if (descriptor.getLiteJavaType() == WireFormat.JavaType.MESSAGE) {
      Object value = getField(descriptor);
      if (value == null) {
        fields.put(descriptor, cloneIfMutable(otherValue));
      } else {
        // Merge the messages.
          value =
              descriptor
                  .internalMergeFrom(((MessageLite) value).toBuilder(), (MessageLite) otherValue)
                  .build();
        fields.put(descriptor, value);
      }
    } else {
      fields.put(descriptor, cloneIfMutable(otherValue));
    }
  }

  // TODO(kenton):  Move static parsing and serialization methods into some
  //   other class.  Probably WireFormat.

  /**
   * Read a field of any primitive type for immutable messages from a CodedInputStream. Enums,
   * groups, and embedded messages are not handled by this method.
   *
   * @param input The stream from which to read.
   * @param type Declared type of the field.
   * @param checkUtf8 When true, check that the input is valid utf8.
   * @return An object representing the field's value, of the exact type which would be returned by
   *     {@link Message#getField(Descriptors.FieldDescriptor)} for this field.
   */
  public static Object readPrimitiveField(
      CodedInputStream input, final WireFormat.FieldType type, boolean checkUtf8)
      throws IOException {
    if (checkUtf8) {
      return WireFormat.readPrimitiveField(input, type, WireFormat.Utf8Validation.STRICT);
    } else {
      return WireFormat.readPrimitiveField(input, type, WireFormat.Utf8Validation.LOOSE);
    }
  }


  /** See {@link Message#writeTo(CodedOutputStream)}. */
  public void writeTo(final CodedOutputStream output) throws IOException {
    for (int i = 0; i < fields.getNumArrayEntries(); i++) {
      final Map.Entry<T, Object> entry = fields.getArrayEntryAt(i);
      writeField(entry.getKey(), entry.getValue(), output);
    }
    for (final Map.Entry<T, Object> entry : fields.getOverflowEntries()) {
      writeField(entry.getKey(), entry.getValue(), output);
    }
  }

  /** Like {@link #writeTo} but uses MessageSet wire format. */
  public void writeMessageSetTo(final CodedOutputStream output) throws IOException {
    for (int i = 0; i < fields.getNumArrayEntries(); i++) {
      writeMessageSetTo(fields.getArrayEntryAt(i), output);
    }
    for (final Map.Entry<T, Object> entry : fields.getOverflowEntries()) {
      writeMessageSetTo(entry, output);
    }
  }

  private void writeMessageSetTo(final Map.Entry<T, Object> entry, final CodedOutputStream output)
      throws IOException {
    final T descriptor = entry.getKey();
    if (descriptor.getLiteJavaType() == WireFormat.JavaType.MESSAGE
        && !descriptor.isRepeated()
        && !descriptor.isPacked()) {
      Object value = entry.getValue();
      if (value instanceof LazyField) {
        value = ((LazyField) value).getValue();
      }
      output.writeMessageSetExtension(entry.getKey().getNumber(), (MessageLite) value);
    } else {
      writeField(descriptor, entry.getValue(), output);
    }
  }

  /**
   * Write a single tag-value pair to the stream.
   *
   * @param output The output stream.
   * @param type The field's type.
   * @param number The field's number.
   * @param value Object representing the field's value. Must be of the exact type which would be
   *     returned by {@link Message#getField(Descriptors.FieldDescriptor)} for this field.
   */
  static void writeElement(
      final CodedOutputStream output,
      final WireFormat.FieldType type,
      final int number,
      final Object value)
      throws IOException {
    // Special case for groups, which need a start and end tag; other fields
    // can just use writeTag() and writeFieldNoTag().
    if (type == WireFormat.FieldType.GROUP) {
        output.writeGroup(number, (MessageLite) value);
    } else {
      output.writeTag(number, getWireFormatForFieldType(type, false));
      writeElementNoTag(output, type, value);
    }
  }

  /**
   * Write a field of arbitrary type, without its tag, to the stream.
   *
   * @param output The output stream.
   * @param type The field's type.
   * @param value Object representing the field's value. Must be of the exact type which would be
   *     returned by {@link Message#getField(Descriptors.FieldDescriptor)} for this field.
   */
  static void writeElementNoTag(
      final CodedOutputStream output, final WireFormat.FieldType type, final Object value)
      throws IOException {
    switch (type) {
      case DOUBLE:
        output.writeDoubleNoTag((Double) value);
        break;
      case FLOAT:
        output.writeFloatNoTag((Float) value);
        break;
      case INT64:
        output.writeInt64NoTag((Long) value);
        break;
      case UINT64:
        output.writeUInt64NoTag((Long) value);
        break;
      case INT32:
        output.writeInt32NoTag((Integer) value);
        break;
      case FIXED64:
        output.writeFixed64NoTag((Long) value);
        break;
      case FIXED32:
        output.writeFixed32NoTag((Integer) value);
        break;
      case BOOL:
        output.writeBoolNoTag((Boolean) value);
        break;
      case GROUP:
        output.writeGroupNoTag((MessageLite) value);
        break;
      case MESSAGE:
        output.writeMessageNoTag((MessageLite) value);
        break;
      case STRING:
        if (value instanceof ByteString) {
          output.writeBytesNoTag((ByteString) value);
        } else {
          output.writeStringNoTag((String) value);
        }
        break;
      case BYTES:
        if (value instanceof ByteString) {
          output.writeBytesNoTag((ByteString) value);
        } else {
          output.writeByteArrayNoTag((byte[]) value);
        }
        break;
      case UINT32:
        output.writeUInt32NoTag((Integer) value);
        break;
      case SFIXED32:
        output.writeSFixed32NoTag((Integer) value);
        break;
      case SFIXED64:
        output.writeSFixed64NoTag((Long) value);
        break;
      case SINT32:
        output.writeSInt32NoTag((Integer) value);
        break;
      case SINT64:
        output.writeSInt64NoTag((Long) value);
        break;

      case ENUM:
        if (value instanceof Internal.EnumLite) {
          output.writeEnumNoTag(((Internal.EnumLite) value).getNumber());
        } else {
          output.writeEnumNoTag(((Integer) value).intValue());
        }
        break;
    }
  }

  /** Write a single field. */
  public static void writeField(
      final FieldDescriptorLite<?> descriptor, final Object value, final CodedOutputStream output)
      throws IOException {
    WireFormat.FieldType type = descriptor.getLiteType();
    int number = descriptor.getNumber();
    if (descriptor.isRepeated()) {
      final List<?> valueList = (List<?>) value;
      if (descriptor.isPacked()) {
        output.writeTag(number, WireFormat.WIRETYPE_LENGTH_DELIMITED);
        // Compute the total data size so the length can be written.
        int dataSize = 0;
        for (final Object element : valueList) {
          dataSize += computeElementSizeNoTag(type, element);
        }
        output.writeRawVarint32(dataSize);
        // Write the data itself, without any tags.
        for (final Object element : valueList) {
          writeElementNoTag(output, type, element);
        }
      } else {
        for (final Object element : valueList) {
          writeElement(output, type, number, element);
        }
      }
    } else {
      if (value instanceof LazyField) {
        writeElement(output, type, number, ((LazyField) value).getValue());
      } else {
        writeElement(output, type, number, value);
      }
    }
  }

  /**
   * See {@link Message#getSerializedSize()}. It's up to the caller to cache the resulting size if
   * desired.
   */
  public int getSerializedSize() {
    int size = 0;
    for (int i = 0; i < fields.getNumArrayEntries(); i++) {
      final Map.Entry<T, Object> entry = fields.getArrayEntryAt(i);
      size += computeFieldSize(entry.getKey(), entry.getValue());
    }
    for (final Map.Entry<T, Object> entry : fields.getOverflowEntries()) {
      size += computeFieldSize(entry.getKey(), entry.getValue());
    }
    return size;
  }

  /** Like {@link #getSerializedSize} but uses MessageSet wire format. */
  public int getMessageSetSerializedSize() {
    int size = 0;
    for (int i = 0; i < fields.getNumArrayEntries(); i++) {
      size += getMessageSetSerializedSize(fields.getArrayEntryAt(i));
    }
    for (final Map.Entry<T, Object> entry : fields.getOverflowEntries()) {
      size += getMessageSetSerializedSize(entry);
    }
    return size;
  }

  private int getMessageSetSerializedSize(final Map.Entry<T, Object> entry) {
    final T descriptor = entry.getKey();
    Object value = entry.getValue();
    if (descriptor.getLiteJavaType() == WireFormat.JavaType.MESSAGE
        && !descriptor.isRepeated()
        && !descriptor.isPacked()) {
      if (value instanceof LazyField) {
        return CodedOutputStream.computeLazyFieldMessageSetExtensionSize(
            entry.getKey().getNumber(), (LazyField) value);
      } else {
        return CodedOutputStream.computeMessageSetExtensionSize(
            entry.getKey().getNumber(), (MessageLite) value);
      }
    } else {
      return computeFieldSize(descriptor, value);
    }
  }

  /**
   * Compute the number of bytes that would be needed to encode a single tag/value pair of arbitrary
   * type.
   *
   * @param type The field's type.
   * @param number The field's number.
   * @param value Object representing the field's value. Must be of the exact type which would be
   *     returned by {@link Message#getField(Descriptors.FieldDescriptor)} for this field.
   */
  static int computeElementSize(
      final WireFormat.FieldType type, final int number, final Object value) {
    int tagSize = CodedOutputStream.computeTagSize(number);
    if (type == WireFormat.FieldType.GROUP) {
      // Only count the end group tag for proto2 messages as for proto1 the end
      // group tag will be counted as a part of getSerializedSize().
        tagSize *= 2;
    }
    return tagSize + computeElementSizeNoTag(type, value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a particular value of arbitrary
   * type, excluding tag.
   *
   * @param type The field's type.
   * @param value Object representing the field's value. Must be of the exact type which would be
   *     returned by {@link Message#getField(Descriptors.FieldDescriptor)} for this field.
   */
  static int computeElementSizeNoTag(final WireFormat.FieldType type, final Object value) {
    switch (type) {
        // Note:  Minor violation of 80-char limit rule here because this would
        //   actually be harder to read if we wrapped the lines.
      case DOUBLE:
        return CodedOutputStream.computeDoubleSizeNoTag((Double) value);
      case FLOAT:
        return CodedOutputStream.computeFloatSizeNoTag((Float) value);
      case INT64:
        return CodedOutputStream.computeInt64SizeNoTag((Long) value);
      case UINT64:
        return CodedOutputStream.computeUInt64SizeNoTag((Long) value);
      case INT32:
        return CodedOutputStream.computeInt32SizeNoTag((Integer) value);
      case FIXED64:
        return CodedOutputStream.computeFixed64SizeNoTag((Long) value);
      case FIXED32:
        return CodedOutputStream.computeFixed32SizeNoTag((Integer) value);
      case BOOL:
        return CodedOutputStream.computeBoolSizeNoTag((Boolean) value);
      case GROUP:
        return CodedOutputStream.computeGroupSizeNoTag((MessageLite) value);
      case BYTES:
        if (value instanceof ByteString) {
          return CodedOutputStream.computeBytesSizeNoTag((ByteString) value);
        } else {
          return CodedOutputStream.computeByteArraySizeNoTag((byte[]) value);
        }
      case STRING:
        if (value instanceof ByteString) {
          return CodedOutputStream.computeBytesSizeNoTag((ByteString) value);
        } else {
          return CodedOutputStream.computeStringSizeNoTag((String) value);
        }
      case UINT32:
        return CodedOutputStream.computeUInt32SizeNoTag((Integer) value);
      case SFIXED32:
        return CodedOutputStream.computeSFixed32SizeNoTag((Integer) value);
      case SFIXED64:
        return CodedOutputStream.computeSFixed64SizeNoTag((Long) value);
      case SINT32:
        return CodedOutputStream.computeSInt32SizeNoTag((Integer) value);
      case SINT64:
        return CodedOutputStream.computeSInt64SizeNoTag((Long) value);

      case MESSAGE:
        if (value instanceof LazyField) {
          return CodedOutputStream.computeLazyFieldSizeNoTag((LazyField) value);
        } else {
          return CodedOutputStream.computeMessageSizeNoTag((MessageLite) value);
        }

      case ENUM:
        if (value instanceof Internal.EnumLite) {
          return CodedOutputStream.computeEnumSizeNoTag(((Internal.EnumLite) value).getNumber());
        } else {
          return CodedOutputStream.computeEnumSizeNoTag((Integer) value);
        }
    }

    throw new RuntimeException("There is no way to get here, but the compiler thinks otherwise.");
  }

  /** Compute the number of bytes needed to encode a particular field. */
  public static int computeFieldSize(final FieldDescriptorLite<?> descriptor, final Object value) {
    WireFormat.FieldType type = descriptor.getLiteType();
    int number = descriptor.getNumber();
    if (descriptor.isRepeated()) {
      if (descriptor.isPacked()) {
        int dataSize = 0;
        for (final Object element : (List<?>) value) {
          dataSize += computeElementSizeNoTag(type, element);
        }
        return dataSize
            + CodedOutputStream.computeTagSize(number)
            + CodedOutputStream.computeRawVarint32Size(dataSize);
      } else {
        int size = 0;
        for (final Object element : (List<?>) value) {
          size += computeElementSize(type, number, element);
        }
        return size;
      }
    } else {
      return computeElementSize(type, number, value);
    }
  }

  /**
   * A FieldSet Builder that accept a {@link MessageLite.Builder} as a field value. This is useful
   * for implementing methods in {@link MessageLite.Builder}.
   */
  static final class Builder<T extends FieldDescriptorLite<T>> {

    private SmallSortedMap<T, Object> fields;
    private boolean hasLazyField;
    private boolean isMutable;
    private boolean hasNestedBuilders;

    private Builder() {
      this(SmallSortedMap.<T>newFieldMap(DEFAULT_FIELD_MAP_ARRAY_SIZE));
    }

    private Builder(SmallSortedMap<T, Object> fields) {
      this.fields = fields;
      this.isMutable = true;
    }

    /** Creates the FieldSet */
    public FieldSet<T> build() {
      if (fields.isEmpty()) {
        return FieldSet.emptySet();
      }
      isMutable = false;
      SmallSortedMap<T, Object> fieldsForBuild = fields;
      if (hasNestedBuilders) {
        // Make a copy of the fields map with all Builders replaced by Message.
        fieldsForBuild = cloneAllFieldsMap(fields, /* copyList */ false);
        replaceBuilders(fieldsForBuild);
      }
      FieldSet<T> fieldSet = new FieldSet<>(fieldsForBuild);
      fieldSet.hasLazyField = hasLazyField;
      return fieldSet;
    }

    private static <T extends FieldDescriptorLite<T>> void replaceBuilders(
        SmallSortedMap<T, Object> fieldMap) {
      for (int i = 0; i < fieldMap.getNumArrayEntries(); i++) {
        replaceBuilders(fieldMap.getArrayEntryAt(i));
      }
      for (Map.Entry<T, Object> entry : fieldMap.getOverflowEntries()) {
        replaceBuilders(entry);
      }
    }

    private static <T extends FieldDescriptorLite<T>> void replaceBuilders(
        Map.Entry<T, Object> entry) {
      entry.setValue(replaceBuilders(entry.getKey(), entry.getValue()));
    }

    private static <T extends FieldDescriptorLite<T>> Object replaceBuilders(
        T descriptor, Object value) {
      if (value == null) {
        return value;
      }
      if (descriptor.getLiteJavaType() == WireFormat.JavaType.MESSAGE) {
        if (descriptor.isRepeated()) {
          if (!(value instanceof List)) {
            throw new IllegalStateException(
                "Repeated field should contains a List but actually contains type: "
                    + value.getClass());
          }
          @SuppressWarnings("unchecked")  // We just check that value is an instance of List above.
          List<Object> list = (List<Object>) value;
          for (int i = 0; i < list.size(); i++) {
            Object oldElement = list.get(i);
            Object newElement = replaceBuilder(oldElement);
            if (newElement != oldElement) {
              // If the list contains a Message.Builder, then make a copy of that list and then
              // modify the Message.Builder into a Message and return the new list. This way, the
              // existing Message.Builder will still be able to modify the inner fields of the
              // original FieldSet.Builder.
              if (list == value) {
                list = new ArrayList<>(list);
              }
              list.set(i, newElement);
            }
          }
          return list;
        } else {
          return replaceBuilder(value);
        }
      }
      return value;
    }

    private static Object replaceBuilder(Object value) {
      return (value instanceof MessageLite.Builder) ? ((MessageLite.Builder) value).build() : value;
    }

    /** Returns a new Builder using the fields from {@code fieldSet}. */
    public static <T extends FieldDescriptorLite<T>> Builder<T> fromFieldSet(FieldSet<T> fieldSet) {
      Builder<T> builder = new Builder<T>(cloneAllFieldsMap(fieldSet.fields, /* copyList */ true));
      builder.hasLazyField = fieldSet.hasLazyField;
      return builder;
    }

    // =================================================================

    /** Get a simple map containing all the fields. */
    public Map<T, Object> getAllFields() {
      if (hasLazyField) {
        SmallSortedMap<T, Object> result = cloneAllFieldsMap(fields, /* copyList */ false);
        if (fields.isImmutable()) {
          result.makeImmutable();
        } else {
          replaceBuilders(result);
        }
        return result;
      }
      return fields.isImmutable() ? fields : Collections.unmodifiableMap(fields);
    }

    /** Useful for implementing {@link Message#hasField(Descriptors.FieldDescriptor)}. */
    public boolean hasField(final T descriptor) {
      if (descriptor.isRepeated()) {
        throw new IllegalArgumentException("hasField() can only be called on non-repeated fields.");
      }

      return fields.get(descriptor) != null;
    }

    /**
     * Useful for implementing {@link Message#getField(Descriptors.FieldDescriptor)}. This method
     * returns {@code null} if the field is not set; in this case it is up to the caller to fetch
     * the field's default value.
     */
    public Object getField(final T descriptor) {
      Object value = getFieldAllowBuilders(descriptor);
      return replaceBuilders(descriptor, value);
    }

    /** Same as {@link #getField(F)}, but allow a {@link MessageLite.Builder} to be returned. */
    Object getFieldAllowBuilders(final T descriptor) {
      Object o = fields.get(descriptor);
      if (o instanceof LazyField) {
        return ((LazyField) o).getValue();
      }
      return o;
    }

    private void ensureIsMutable() {
      if (!isMutable) {
        fields = cloneAllFieldsMap(fields, /* copyList */ true);
        isMutable = true;
      }
    }

    /**
     * Useful for implementing {@link Message.Builder#setField(Descriptors.FieldDescriptor,
     * Object)}.
     */
    @SuppressWarnings({"unchecked", "rawtypes"})
    public void setField(final T descriptor, Object value) {
      ensureIsMutable();
      if (descriptor.isRepeated()) {
        if (!(value instanceof List)) {
          throw new IllegalArgumentException(
              "Wrong object type used with protocol message reflection.");
        }

        // Wrap the contents in a new list so that the caller cannot change
        // the list's contents after setting it.
        final List newList = new ArrayList();
        newList.addAll((List) value);
        for (final Object element : newList) {
          verifyType(descriptor.getLiteType(), element);
          hasNestedBuilders = hasNestedBuilders || element instanceof MessageLite.Builder;
        }
        value = newList;
      } else {
        verifyType(descriptor.getLiteType(), value);
      }

      if (value instanceof LazyField) {
        hasLazyField = true;
      }
      hasNestedBuilders = hasNestedBuilders || value instanceof MessageLite.Builder;

      fields.put(descriptor, value);
    }

    /** Useful for implementing {@link Message.Builder#clearField(Descriptors.FieldDescriptor)}. */
    public void clearField(final T descriptor) {
      ensureIsMutable();
      fields.remove(descriptor);
      if (fields.isEmpty()) {
        hasLazyField = false;
      }
    }

    /**
     * Useful for implementing {@link Message#getRepeatedFieldCount(Descriptors.FieldDescriptor)}.
     */
    public int getRepeatedFieldCount(final T descriptor) {
      if (!descriptor.isRepeated()) {
        throw new IllegalArgumentException(
            "getRepeatedField() can only be called on repeated fields.");
      }

      final Object value = getField(descriptor);
      if (value == null) {
        return 0;
      } else {
        return ((List<?>) value).size();
      }
    }

    /**
     * Useful for implementing {@link Message#getRepeatedField(Descriptors.FieldDescriptor, int)}.
     */
    public Object getRepeatedField(final T descriptor, final int index) {
      if (hasNestedBuilders) {
        ensureIsMutable();
      }
      Object value = getRepeatedFieldAllowBuilders(descriptor, index);
      return replaceBuilder(value);
    }

    /**
     * Same as {@link #getRepeatedField(F, int)}, but allow a {@link MessageLite.Builder} to be
     * returned.
     */
    Object getRepeatedFieldAllowBuilders(final T descriptor, final int index) {
      if (!descriptor.isRepeated()) {
        throw new IllegalArgumentException(
            "getRepeatedField() can only be called on repeated fields.");
      }

      final Object value = getFieldAllowBuilders(descriptor);

      if (value == null) {
        throw new IndexOutOfBoundsException();
      } else {
        return ((List<?>) value).get(index);
      }
    }

    /**
     * Useful for implementing {@link Message.Builder#setRepeatedField(Descriptors.FieldDescriptor,
     * int, Object)}.
     */
    @SuppressWarnings("unchecked")
    public void setRepeatedField(final T descriptor, final int index, final Object value) {
      ensureIsMutable();
      if (!descriptor.isRepeated()) {
        throw new IllegalArgumentException(
            "getRepeatedField() can only be called on repeated fields.");
      }

      hasNestedBuilders = hasNestedBuilders || value instanceof MessageLite.Builder;

      final Object list = getField(descriptor);
      if (list == null) {
        throw new IndexOutOfBoundsException();
      }

      verifyType(descriptor.getLiteType(), value);
      ((List<Object>) list).set(index, value);
    }

    /**
     * Useful for implementing {@link Message.Builder#addRepeatedField(Descriptors.FieldDescriptor,
     * Object)}.
     */
    @SuppressWarnings("unchecked")
    public void addRepeatedField(final T descriptor, final Object value) {
      ensureIsMutable();
      if (!descriptor.isRepeated()) {
        throw new IllegalArgumentException(
            "addRepeatedField() can only be called on repeated fields.");
      }

      hasNestedBuilders = hasNestedBuilders || value instanceof MessageLite.Builder;

      verifyType(descriptor.getLiteType(), value);

      final Object existingValue = getField(descriptor);
      List<Object> list;
      if (existingValue == null) {
        list = new ArrayList<>();
        fields.put(descriptor, list);
      } else {
        list = (List<Object>) existingValue;
      }

      list.add(value);
    }

    /**
     * Verifies that the given object is of the correct type to be a valid value for the given
     * field. (For repeated fields, this checks if the object is the right type to be one element of
     * the field.)
     *
     * @throws IllegalArgumentException The value is not of the right type.
     */
    private static void verifyType(final WireFormat.FieldType type, final Object value) {
      if (!FieldSet.isValidType(type, value)) {
        // Builder can accept Message.Builder values even though FieldSet will reject.
        if (type.getJavaType() == WireFormat.JavaType.MESSAGE
            && value instanceof MessageLite.Builder) {
          return;
        }
        throw new IllegalArgumentException(
            "Wrong object type used with protocol message reflection.");
      }
    }

    /**
     * See {@link Message#isInitialized()}. Note: Since {@code FieldSet} itself does not have any
     * way of knowing about required fields that aren't actually present in the set, it is up to the
     * caller to check that all required fields are present.
     */
    public boolean isInitialized() {
      for (int i = 0; i < fields.getNumArrayEntries(); i++) {
        if (!FieldSet.isInitialized(fields.getArrayEntryAt(i))) {
          return false;
        }
      }
      for (final Map.Entry<T, Object> entry : fields.getOverflowEntries()) {
        if (!FieldSet.isInitialized(entry)) {
          return false;
        }
      }
      return true;
    }

    /**
     * Like {@link Message.Builder#mergeFrom(Message)}, but merges from another {@link FieldSet}.
     */
    public void mergeFrom(final FieldSet<T> other) {
      ensureIsMutable();
      for (int i = 0; i < other.fields.getNumArrayEntries(); i++) {
        mergeFromField(other.fields.getArrayEntryAt(i));
      }
      for (final Map.Entry<T, Object> entry : other.fields.getOverflowEntries()) {
        mergeFromField(entry);
      }
    }

    @SuppressWarnings({"unchecked", "rawtypes"})
    private void mergeFromField(final Map.Entry<T, Object> entry) {
      final T descriptor = entry.getKey();
      Object otherValue = entry.getValue();
      if (otherValue instanceof LazyField) {
        otherValue = ((LazyField) otherValue).getValue();
      }

      if (descriptor.isRepeated()) {
        Object value = getField(descriptor);
        if (value == null) {
          value = new ArrayList();
        }
        for (Object element : (List) otherValue) {
          ((List) value).add(FieldSet.cloneIfMutable(element));
        }
        fields.put(descriptor, value);
      } else if (descriptor.getLiteJavaType() == WireFormat.JavaType.MESSAGE) {
        Object value = getField(descriptor);
        if (value == null) {
          fields.put(descriptor, FieldSet.cloneIfMutable(otherValue));
        } else {
          // Merge the messages.
          if (value instanceof MessageLite.Builder) {
            descriptor.internalMergeFrom((MessageLite.Builder) value, (MessageLite) otherValue);
          } else {
            value =
                descriptor
                    .internalMergeFrom(((MessageLite) value).toBuilder(), (MessageLite) otherValue)
                    .build();
            fields.put(descriptor, value);
          }
        }
      } else {
        fields.put(descriptor, cloneIfMutable(otherValue));
      }
    }
  }
}
