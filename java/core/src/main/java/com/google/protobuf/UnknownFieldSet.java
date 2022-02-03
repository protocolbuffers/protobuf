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

import com.google.protobuf.AbstractMessageLite.Builder.LimitedInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.ListIterator;
import java.util.Map;
import java.util.TreeMap;

/**
 * {@code UnknownFieldSet} keeps track of fields which were seen when parsing a protocol
 * message but whose field numbers or types are unrecognized. This most frequently occurs when new
 * fields are added to a message type and then messages containing those fields are read by old
 * software that was compiled before the new types were added.
 *
 * <p>Every {@link Message} contains an {@code UnknownFieldSet} (and every {@link Message.Builder}
 * contains a {@link Builder}).
 *
 * <p>Most users will never need to use this class.
 *
 * @author kenton@google.com Kenton Varda
 */
public final class UnknownFieldSet implements MessageLite {

  private final TreeMap<Integer, Field> fields;

  /**
   * Construct an {@code UnknownFieldSet} around the given map.
   */
  UnknownFieldSet(TreeMap<Integer, Field> fields) {
    this.fields = fields;
  }

  /** Create a new {@link Builder}. */
  public static Builder newBuilder() {
    return Builder.create();
  }

  /** Create a new {@link Builder} and initialize it to be a copy of {@code copyFrom}. */
  public static Builder newBuilder(UnknownFieldSet copyFrom) {
    return newBuilder().mergeFrom(copyFrom);
  }

  /** Get an empty {@code UnknownFieldSet}. */
  public static UnknownFieldSet getDefaultInstance() {
    return defaultInstance;
  }

  @Override
  public UnknownFieldSet getDefaultInstanceForType() {
    return defaultInstance;
  }

  private static final UnknownFieldSet defaultInstance =
      new UnknownFieldSet(new TreeMap<Integer, Field>());


  @Override
  public boolean equals(Object other) {
    if (this == other) {
      return true;
    }
    return (other instanceof UnknownFieldSet) && fields.equals(((UnknownFieldSet) other).fields);
  }

  @Override
  public int hashCode() {
    if (fields.isEmpty()) { // avoid allocation of iterator.
      // This optimization may not be helpful but it is needed for the allocation tests to pass.
      return 0;
    }
    return fields.hashCode();
  }

  /** Get a map of fields in the set by number. */
  public Map<Integer, Field> asMap() {
    return (Map<Integer, Field>) fields.clone();
  }

  /** Check if the given field number is present in the set. */
  public boolean hasField(int number) {
    return fields.containsKey(number);
  }

  /** Get a field by number. Returns an empty field if not present. Never returns {@code null}. */
  public Field getField(int number) {
    Field result = fields.get(number);
    return (result == null) ? Field.getDefaultInstance() : result;
  }

  /** Serializes the set and writes it to {@code output}. */
  @Override
  public void writeTo(CodedOutputStream output) throws IOException {
    for (Map.Entry<Integer, Field> entry : fields.entrySet()) {
      Field field = entry.getValue();
      field.writeTo(entry.getKey(), output);
    }
  }

  /**
   * Converts the set to a string in protocol buffer text format. This is just a trivial wrapper
   * around {@link TextFormat.Printer#printToString(UnknownFieldSet)}.
   */
  @Override
  public String toString() {
    return TextFormat.printer().printToString(this);
  }

  /**
   * Serializes the message to a {@code ByteString} and returns it. This is just a trivial wrapper
   * around {@link #writeTo(CodedOutputStream)}.
   */
  @Override
  public ByteString toByteString() {
    try {
      ByteString.CodedBuilder out = ByteString.newCodedBuilder(getSerializedSize());
      writeTo(out.getCodedOutput());
      return out.build();
    } catch (IOException e) {
      throw new RuntimeException(
          "Serializing to a ByteString threw an IOException (should never happen).", e);
    }
  }

  /**
   * Serializes the message to a {@code byte} array and returns it. This is just a trivial wrapper
   * around {@link #writeTo(CodedOutputStream)}.
   */
  @Override
  public byte[] toByteArray() {
    try {
      byte[] result = new byte[getSerializedSize()];
      CodedOutputStream output = CodedOutputStream.newInstance(result);
      writeTo(output);
      output.checkNoSpaceLeft();
      return result;
    } catch (IOException e) {
      throw new RuntimeException(
          "Serializing to a byte array threw an IOException (should never happen).", e);
    }
  }

  /**
   * Serializes the message and writes it to {@code output}. This is just a trivial wrapper around
   * {@link #writeTo(CodedOutputStream)}.
   */
  @Override
  public void writeTo(OutputStream output) throws IOException {
    CodedOutputStream codedOutput = CodedOutputStream.newInstance(output);
    writeTo(codedOutput);
    codedOutput.flush();
  }

  @Override
  public void writeDelimitedTo(OutputStream output) throws IOException {
    CodedOutputStream codedOutput = CodedOutputStream.newInstance(output);
    codedOutput.writeUInt32NoTag(getSerializedSize());
    writeTo(codedOutput);
    codedOutput.flush();
  }

  /** Get the number of bytes required to encode this set. */
  @Override
  public int getSerializedSize() {
    int result = 0;
    if (!fields.isEmpty()) {
      for (Map.Entry<Integer, Field> entry : fields.entrySet()) {
        result += entry.getValue().getSerializedSize(entry.getKey());
      }
    }
    return result;
  }

  /** Serializes the set and writes it to {@code output} using {@code MessageSet} wire format. */
  public void writeAsMessageSetTo(CodedOutputStream output) throws IOException {
    for (Map.Entry<Integer, Field> entry : fields.entrySet()) {
      entry.getValue().writeAsMessageSetExtensionTo(entry.getKey(), output);
    }
  }

  /** Serializes the set and writes it to {@code writer}. */
  void writeTo(Writer writer) throws IOException {
    if (writer.fieldOrder() == Writer.FieldOrder.DESCENDING) {
      // Write fields in descending order.
      for (Map.Entry<Integer, Field> entry : fields.descendingMap().entrySet()) {
        entry.getValue().writeTo(entry.getKey(), writer);
      }
    } else {
      // Write fields in ascending order.
      for (Map.Entry<Integer, Field> entry : fields.entrySet()) {
        entry.getValue().writeTo(entry.getKey(), writer);
      }
    }
  }

  /** Serializes the set and writes it to {@code writer} using {@code MessageSet} wire format. */
  void writeAsMessageSetTo(Writer writer) throws IOException {
    if (writer.fieldOrder() == Writer.FieldOrder.DESCENDING) {
      // Write fields in descending order.
      for (Map.Entry<Integer, Field> entry : fields.descendingMap().entrySet()) {
        entry.getValue().writeAsMessageSetExtensionTo(entry.getKey(), writer);
      }
    } else {
      // Write fields in ascending order.
      for (Map.Entry<Integer, Field> entry : fields.entrySet()) {
        entry.getValue().writeAsMessageSetExtensionTo(entry.getKey(), writer);
      }
    }
  }

  /** Get the number of bytes required to encode this set using {@code MessageSet} wire format. */
  public int getSerializedSizeAsMessageSet() {
    int result = 0;
    for (Map.Entry<Integer, Field> entry : fields.entrySet()) {
      result += entry.getValue().getSerializedSizeAsMessageSetExtension(entry.getKey());
    }
    return result;
  }

  @Override
  public boolean isInitialized() {
    // UnknownFieldSets do not have required fields, so they are always
    // initialized.
    return true;
  }

  /** Parse an {@code UnknownFieldSet} from the given input stream. */
  public static UnknownFieldSet parseFrom(CodedInputStream input) throws IOException {
    return newBuilder().mergeFrom(input).build();
  }

  /** Parse {@code data} as an {@code UnknownFieldSet} and return it. */
  public static UnknownFieldSet parseFrom(ByteString data)
      throws InvalidProtocolBufferException {
    return newBuilder().mergeFrom(data).build();
  }

  /** Parse {@code data} as an {@code UnknownFieldSet} and return it. */
  public static UnknownFieldSet parseFrom(byte[] data) throws InvalidProtocolBufferException {
    return newBuilder().mergeFrom(data).build();
  }

  /** Parse an {@code UnknownFieldSet} from {@code input} and return it. */
  public static UnknownFieldSet parseFrom(InputStream input) throws IOException {
    return newBuilder().mergeFrom(input).build();
  }

  @Override
  public Builder newBuilderForType() {
    return newBuilder();
  }

  @Override
  public Builder toBuilder() {
    return newBuilder().mergeFrom(this);
  }

  /**
   * Builder for {@link UnknownFieldSet}s.
   *
   * <p>Note that this class maintains {@link Field.Builder}s for all fields in the set. Thus,
   * adding one element to an existing {@link Field} does not require making a copy. This is
   * important for efficient parsing of unknown repeated fields. However, it implies that {@link
   * Field}s cannot be constructed independently, nor can two {@link UnknownFieldSet}s share the
   * same {@code Field} object.
   *
   * <p>Use {@link UnknownFieldSet#newBuilder()} to construct a {@code Builder}.
   */
  public static final class Builder implements MessageLite.Builder {
    // This constructor should never be called directly (except from 'create').
    private Builder() {}

    private TreeMap<Integer, Field.Builder> fieldBuilders = new TreeMap<>();

    private static Builder create() {
      return new Builder();
    }

    /**
     * Get a field builder for the given field number which includes any values that already exist.
     */
    private Field.Builder getFieldBuilder(int number) {
      if (number == 0) {
        return null;
      } else {
        Field.Builder builder = fieldBuilders.get(number);
        if (builder == null) {
          builder = Field.newBuilder();
          fieldBuilders.put(number, builder);
        }
        return builder;
      }
    }

    /**
     * Build the {@link UnknownFieldSet} and return it.
     */
    @Override
    public UnknownFieldSet build() {
      UnknownFieldSet result;
      if (fieldBuilders.isEmpty()) {
        result = getDefaultInstance();
      } else {
        TreeMap<Integer, Field> fields = new TreeMap<>();
        for (Map.Entry<Integer, Field.Builder> entry : fieldBuilders.entrySet()) {
          fields.put(entry.getKey(), entry.getValue().build());
        }
        result = new UnknownFieldSet(fields);
      }
      return result;
    }

    @Override
    public UnknownFieldSet buildPartial() {
      // No required fields, so this is the same as build().
      return build();
    }

    @Override
    public Builder clone() {
      Builder clone = UnknownFieldSet.newBuilder();
      for (Map.Entry<Integer, Field.Builder> entry : fieldBuilders.entrySet()) {
        Integer key = entry.getKey();
        Field.Builder value = entry.getValue();
        clone.fieldBuilders.put(key, value.clone());
      }
      return clone;
    }

    @Override
    public UnknownFieldSet getDefaultInstanceForType() {
      return UnknownFieldSet.getDefaultInstance();
    }

    /** Reset the builder to an empty set. */
    @Override
    public Builder clear() {
      fieldBuilders = new TreeMap<>();
      return this;
    }

    /**
     * Clear fields from the set with a given field number.
     *
     * @throws IllegalArgumentException if number is not positive
     */
    public Builder clearField(int number) {
      if (number <= 0) {
        throw new IllegalArgumentException(number + " is not a valid field number.");
      }
      if (fieldBuilders.containsKey(number)) {
        fieldBuilders.remove(number);
      }
      return this;
    }

    /**
     * Merge the fields from {@code other} into this set. If a field number exists in both sets,
     * {@code other}'s values for that field will be appended to the values in this set.
     */
    public Builder mergeFrom(UnknownFieldSet other) {
      if (other != getDefaultInstance()) {
        for (Map.Entry<Integer, Field> entry : other.fields.entrySet()) {
          mergeField(entry.getKey(), entry.getValue());
        }
      }
      return this;
    }

    /**
     * Add a field to the {@code UnknownFieldSet}. If a field with the same number already exists,
     * the two are merged.
     *
     * @throws IllegalArgumentException if number is not positive
     */
    public Builder mergeField(int number, final Field field) {
      if (number <= 0) {
        throw new IllegalArgumentException(number + " is not a valid field number.");
      }
      if (hasField(number)) {
        getFieldBuilder(number).mergeFrom(field);
      } else {
        // Optimization:  We could call getFieldBuilder(number).mergeFrom(field)
        // in this case, but that would create a copy of the Field object.
        // We'd rather reuse the one passed to us, so call addField() instead.
        addField(number, field);
      }
      return this;
    }

    /**
     * Convenience method for merging a new field containing a single varint value. This is used in
     * particular when an unknown enum value is encountered.
     *
     * @throws IllegalArgumentException if number is not positive
     */
    public Builder mergeVarintField(int number, int value) {
      if (number <= 0) {
        throw new IllegalArgumentException(number + " is not a valid field number.");
      }
      getFieldBuilder(number).addVarint(value);
      return this;
    }

    /**
     * Convenience method for merging a length-delimited field.
     *
     * <p>For use by generated code only.
     *
     * @throws IllegalArgumentException if number is not positive
     */
    public Builder mergeLengthDelimitedField(int number, ByteString value) {
      if (number <= 0) {
        throw new IllegalArgumentException(number + " is not a valid field number.");
      }
      getFieldBuilder(number).addLengthDelimited(value);
      return this;
    }

    /** Check if the given field number is present in the set. */
    public boolean hasField(int number) {
      return fieldBuilders.containsKey(number);
    }

    /**
     * Add a field to the {@code UnknownFieldSet}. If a field with the same number already exists,
     * it is removed.
     *
     * @throws IllegalArgumentException if number is not positive
     */
    public Builder addField(int number, Field field) {
      if (number <= 0) {
        throw new IllegalArgumentException(number + " is not a valid field number.");
      }
      fieldBuilders.put(number, Field.newBuilder(field));
      return this;
    }

    /**
     * Get all present {@code Field}s as an immutable {@code Map}. If more fields are added, the
     * changes may or may not be reflected in this map.
     */
    public Map<Integer, Field> asMap() {
      TreeMap<Integer, Field> fields = new TreeMap<>();
      for (Map.Entry<Integer, Field.Builder> entry : fieldBuilders.entrySet()) {
        fields.put(entry.getKey(), entry.getValue().build());
      }
      return Collections.unmodifiableMap(fields);
    }

    /** Parse an entire message from {@code input} and merge its fields into this set. */
    @Override
    public Builder mergeFrom(CodedInputStream input) throws IOException {
      while (true) {
        int tag = input.readTag();
        if (tag == 0 || !mergeFieldFrom(tag, input)) {
          break;
        }
      }
      return this;
    }

    /**
     * Parse a single field from {@code input} and merge it into this set.
     *
     * @param tag The field's tag number, which was already parsed.
     * @return {@code false} if the tag is an end group tag.
     */
    public boolean mergeFieldFrom(int tag, CodedInputStream input) throws IOException {
      int number = WireFormat.getTagFieldNumber(tag);
      switch (WireFormat.getTagWireType(tag)) {
        case WireFormat.WIRETYPE_VARINT:
          getFieldBuilder(number).addVarint(input.readInt64());
          return true;
        case WireFormat.WIRETYPE_FIXED64:
          getFieldBuilder(number).addFixed64(input.readFixed64());
          return true;
        case WireFormat.WIRETYPE_LENGTH_DELIMITED:
          getFieldBuilder(number).addLengthDelimited(input.readBytes());
          return true;
        case WireFormat.WIRETYPE_START_GROUP:
          Builder subBuilder = newBuilder();
          input.readGroup(number, subBuilder, ExtensionRegistry.getEmptyRegistry());
          getFieldBuilder(number).addGroup(subBuilder.build());
          return true;
        case WireFormat.WIRETYPE_END_GROUP:
          return false;
        case WireFormat.WIRETYPE_FIXED32:
          getFieldBuilder(number).addFixed32(input.readFixed32());
          return true;
        default:
          throw InvalidProtocolBufferException.invalidWireType();
      }
    }

    /**
     * Parse {@code data} as an {@code UnknownFieldSet} and merge it with the set being built. This
     * is just a small wrapper around {@link #mergeFrom(CodedInputStream)}.
     */
    @Override
    public Builder mergeFrom(ByteString data) throws InvalidProtocolBufferException {
      try {
        CodedInputStream input = data.newCodedInput();
        mergeFrom(input);
        input.checkLastTagWas(0);
        return this;
      } catch (InvalidProtocolBufferException e) {
        throw e;
      } catch (IOException e) {
        throw new RuntimeException(
            "Reading from a ByteString threw an IOException (should never happen).", e);
      }
    }

    /**
     * Parse {@code data} as an {@code UnknownFieldSet} and merge it with the set being built. This
     * is just a small wrapper around {@link #mergeFrom(CodedInputStream)}.
     */
    @Override
    public Builder mergeFrom(byte[] data) throws InvalidProtocolBufferException {
      try {
        CodedInputStream input = CodedInputStream.newInstance(data);
        mergeFrom(input);
        input.checkLastTagWas(0);
        return this;
      } catch (InvalidProtocolBufferException e) {
        throw e;
      } catch (IOException e) {
        throw new RuntimeException(
            "Reading from a byte array threw an IOException (should never happen).", e);
      }
    }

    /**
     * Parse an {@code UnknownFieldSet} from {@code input} and merge it with the set being built.
     * This is just a small wrapper around {@link #mergeFrom(CodedInputStream)}.
     */
    @Override
    public Builder mergeFrom(InputStream input) throws IOException {
      CodedInputStream codedInput = CodedInputStream.newInstance(input);
      mergeFrom(codedInput);
      codedInput.checkLastTagWas(0);
      return this;
    }

    @Override
    public boolean mergeDelimitedFrom(InputStream input) throws IOException {
      int firstByte = input.read();
      if (firstByte == -1) {
        return false;
      }
      int size = CodedInputStream.readRawVarint32(firstByte, input);
      InputStream limitedInput = new LimitedInputStream(input, size);
      mergeFrom(limitedInput);
      return true;
    }

    @Override
    public boolean mergeDelimitedFrom(InputStream input, ExtensionRegistryLite extensionRegistry)
        throws IOException {
      // UnknownFieldSet has no extensions.
      return mergeDelimitedFrom(input);
    }

    @Override
    public Builder mergeFrom(CodedInputStream input, ExtensionRegistryLite extensionRegistry)
        throws IOException {
      // UnknownFieldSet has no extensions.
      return mergeFrom(input);
    }

    @Override
    public Builder mergeFrom(ByteString data, ExtensionRegistryLite extensionRegistry)
        throws InvalidProtocolBufferException {
      // UnknownFieldSet has no extensions.
      return mergeFrom(data);
    }

    @Override
    public Builder mergeFrom(byte[] data, int off, int len) throws InvalidProtocolBufferException {
      try {
        CodedInputStream input = CodedInputStream.newInstance(data, off, len);
        mergeFrom(input);
        input.checkLastTagWas(0);
        return this;
      } catch (InvalidProtocolBufferException e) {
        throw e;
      } catch (IOException e) {
        throw new RuntimeException(
            "Reading from a byte array threw an IOException (should never happen).", e);
      }
    }

    @Override
    public Builder mergeFrom(byte[] data, ExtensionRegistryLite extensionRegistry)
        throws InvalidProtocolBufferException {
      // UnknownFieldSet has no extensions.
      return mergeFrom(data);
    }

    @Override
    public Builder mergeFrom(byte[] data, int off, int len, ExtensionRegistryLite extensionRegistry)
        throws InvalidProtocolBufferException {
      // UnknownFieldSet has no extensions.
      return mergeFrom(data, off, len);
    }

    @Override
    public Builder mergeFrom(InputStream input, ExtensionRegistryLite extensionRegistry)
        throws IOException {
      // UnknownFieldSet has no extensions.
      return mergeFrom(input);
    }

    @Override
    public Builder mergeFrom(MessageLite m) {
      if (m instanceof UnknownFieldSet) {
        return mergeFrom((UnknownFieldSet) m);
      }
      throw new IllegalArgumentException(
          "mergeFrom(MessageLite) can only merge messages of the same type.");
    }

    @Override
    public boolean isInitialized() {
      // UnknownFieldSets do not have required fields, so they are always
      // initialized.
      return true;
    }
  }

  /**
   * Represents a single field in an {@code UnknownFieldSet}.
   *
   * <p>A {@code Field} consists of five lists of values. The lists correspond to the five "wire
   * types" used in the protocol buffer binary format. The wire type of each field can be determined
   * from the encoded form alone, without knowing the field's declared type. So, we are able to
   * parse unknown values at least this far and separate them. Normally, only one of the five lists
   * will contain any values, since it is impossible to define a valid message type that declares
   * two different types for the same field number. However, the code is designed to allow for the
   * case where the same unknown field number is encountered using multiple different wire types.
   *
   * <p>{@code Field} is an immutable class. To construct one, you must use a {@link Builder}.
   *
   * @see UnknownFieldSet
   */
  public static final class Field {
    private Field() {}

    /** Construct a new {@link Builder}. */
    public static Builder newBuilder() {
      return Builder.create();
    }

    /** Construct a new {@link Builder} and initialize it to a copy of {@code copyFrom}. */
    public static Builder newBuilder(Field copyFrom) {
      return newBuilder().mergeFrom(copyFrom);
    }

    /** Get an empty {@code Field}. */
    public static Field getDefaultInstance() {
      return fieldDefaultInstance;
    }

    private static final Field fieldDefaultInstance = newBuilder().build();

    /** Get the list of varint values for this field. */
    public List<Long> getVarintList() {
      return varint;
    }

    /** Get the list of fixed32 values for this field. */
    public List<Integer> getFixed32List() {
      return fixed32;
    }

    /** Get the list of fixed64 values for this field. */
    public List<Long> getFixed64List() {
      return fixed64;
    }

    /** Get the list of length-delimited values for this field. */
    public List<ByteString> getLengthDelimitedList() {
      return lengthDelimited;
    }

    /**
     * Get the list of embedded group values for this field. These are represented using {@link
     * UnknownFieldSet}s rather than {@link Message}s since the group's type is presumably unknown.
     */
    public List<UnknownFieldSet> getGroupList() {
      return group;
    }

    @Override
    public boolean equals(Object other) {
      if (this == other) {
        return true;
      }
      if (!(other instanceof Field)) {
        return false;
      }
      return Arrays.equals(getIdentityArray(), ((Field) other).getIdentityArray());
    }

    @Override
    public int hashCode() {
      return Arrays.hashCode(getIdentityArray());
    }

    /** Returns the array of objects to be used to uniquely identify this {@link Field} instance. */
    private Object[] getIdentityArray() {
      return new Object[] {varint, fixed32, fixed64, lengthDelimited, group};
    }

    /**
     * Serializes the message to a {@code ByteString} and returns it. This is just a trivial wrapper
     * around {@link #writeTo(int, CodedOutputStream)}.
     */
    public ByteString toByteString(int fieldNumber) {
      try {
        // TODO(lukes): consider caching serialized size in a volatile long
        ByteString.CodedBuilder out =
            ByteString.newCodedBuilder(getSerializedSize(fieldNumber));
        writeTo(fieldNumber, out.getCodedOutput());
        return out.build();
      } catch (IOException e) {
        throw new RuntimeException(
            "Serializing to a ByteString should never fail with an IOException", e);
      }
    }

    /** Serializes the field, including field number, and writes it to {@code output}. */
    public void writeTo(int fieldNumber, CodedOutputStream output) throws IOException {
      for (long value : varint) {
        output.writeUInt64(fieldNumber, value);
      }
      for (int value : fixed32) {
        output.writeFixed32(fieldNumber, value);
      }
      for (long value : fixed64) {
        output.writeFixed64(fieldNumber, value);
      }
      for (ByteString value : lengthDelimited) {
        output.writeBytes(fieldNumber, value);
      }
      for (UnknownFieldSet value : group) {
        output.writeGroup(fieldNumber, value);
      }
    }

    /** Get the number of bytes required to encode this field, including field number. */
    public int getSerializedSize(int fieldNumber) {
      int result = 0;
      for (long value : varint) {
        result += CodedOutputStream.computeUInt64Size(fieldNumber, value);
      }
      for (int value : fixed32) {
        result += CodedOutputStream.computeFixed32Size(fieldNumber, value);
      }
      for (long value : fixed64) {
        result += CodedOutputStream.computeFixed64Size(fieldNumber, value);
      }
      for (ByteString value : lengthDelimited) {
        result += CodedOutputStream.computeBytesSize(fieldNumber, value);
      }
      for (UnknownFieldSet value : group) {
        result += CodedOutputStream.computeGroupSize(fieldNumber, value);
      }
      return result;
    }

    /**
     * Serializes the field, including field number, and writes it to {@code output}, using {@code
     * MessageSet} wire format.
     */
    public void writeAsMessageSetExtensionTo(int fieldNumber, CodedOutputStream output)
        throws IOException {
      for (ByteString value : lengthDelimited) {
        output.writeRawMessageSetExtension(fieldNumber, value);
      }
    }

    /** Serializes the field, including field number, and writes it to {@code writer}. */
    void writeTo(int fieldNumber, Writer writer) throws IOException {
      writer.writeInt64List(fieldNumber, varint, false);
      writer.writeFixed32List(fieldNumber, fixed32, false);
      writer.writeFixed64List(fieldNumber, fixed64, false);
      writer.writeBytesList(fieldNumber, lengthDelimited);

      if (writer.fieldOrder() == Writer.FieldOrder.ASCENDING) {
        for (int i = 0; i < group.size(); i++) {
          writer.writeStartGroup(fieldNumber);
          group.get(i).writeTo(writer);
          writer.writeEndGroup(fieldNumber);
        }
      } else {
        for (int i = group.size() - 1; i >= 0; i--) {
          writer.writeEndGroup(fieldNumber);
          group.get(i).writeTo(writer);
          writer.writeStartGroup(fieldNumber);
        }
      }
    }

    /**
     * Serializes the field, including field number, and writes it to {@code writer}, using {@code
     * MessageSet} wire format.
     */
    private void writeAsMessageSetExtensionTo(int fieldNumber, Writer writer)
        throws IOException {
      if (writer.fieldOrder() == Writer.FieldOrder.DESCENDING) {
        // Write in descending field order.
        ListIterator<ByteString> iter = lengthDelimited.listIterator(lengthDelimited.size());
        while (iter.hasPrevious()) {
          writer.writeMessageSetItem(fieldNumber, iter.previous());
        }
      } else {
        // Write in ascending field order.
        for (ByteString value : lengthDelimited) {
          writer.writeMessageSetItem(fieldNumber, value);
        }
      }
    }

    /**
     * Get the number of bytes required to encode this field, including field number, using {@code
     * MessageSet} wire format.
     */
    public int getSerializedSizeAsMessageSetExtension(int fieldNumber) {
      int result = 0;
      for (ByteString value : lengthDelimited) {
        result += CodedOutputStream.computeRawMessageSetExtensionSize(fieldNumber, value);
      }
      return result;
    }

    private List<Long> varint;
    private List<Integer> fixed32;
    private List<Long> fixed64;
    private List<ByteString> lengthDelimited;
    private List<UnknownFieldSet> group;

    /**
     * Used to build a {@link Field} within an {@link UnknownFieldSet}.
     *
     * <p>Use {@link Field#newBuilder()} to construct a {@code Builder}.
     */
    public static final class Builder {
      // This constructor should only be called directly from 'create' and 'clone'.
      private Builder() {
        result = new Field();
      }

      private static Builder create() {
        Builder builder = new Builder();
        return builder;
      }

      private Field result;

      @Override
      public Builder clone() {
        Field copy = new Field();
        if (result.varint == null) {
          copy.varint = null;
        } else {
          copy.varint = new ArrayList<>(result.varint);
        }
        if (result.fixed32 == null) {
          copy.fixed32 = null;
        } else {
          copy.fixed32 = new ArrayList<>(result.fixed32);
        }
        if (result.fixed64 == null) {
          copy.fixed64 = null;
        } else {
          copy.fixed64 = new ArrayList<>(result.fixed64);
        }
        if (result.lengthDelimited == null) {
          copy.lengthDelimited = null;
        } else {
          copy.lengthDelimited = new ArrayList<>(result.lengthDelimited);
        }
        if (result.group == null) {
          copy.group = null;
        } else {
          copy.group = new ArrayList<>(result.group);
        }

        Builder clone = new Builder();
        clone.result = copy;
        return clone;
      }

      /**
       * Build the field.
       */
      public Field build() {
        Field built = new Field();
        if (result.varint == null) {
          built.varint = Collections.emptyList();
        } else {
          built.varint = Collections.unmodifiableList(new ArrayList<>(result.varint));
        }
        if (result.fixed32 == null) {
          built.fixed32 = Collections.emptyList();
        } else {
          built.fixed32 = Collections.unmodifiableList(new ArrayList<>(result.fixed32));
        }
        if (result.fixed64 == null) {
          built.fixed64 = Collections.emptyList();
        } else {
          built.fixed64 = Collections.unmodifiableList(new ArrayList<>(result.fixed64));
        }
        if (result.lengthDelimited == null) {
          built.lengthDelimited = Collections.emptyList();
        } else {
          built.lengthDelimited = Collections.unmodifiableList(
              new ArrayList<>(result.lengthDelimited));
        }
        if (result.group == null) {
          built.group = Collections.emptyList();
        } else {
          built.group = Collections.unmodifiableList(new ArrayList<>(result.group));
        }

        return built;
      }

      /** Discard the field's contents. */
      public Builder clear() {
        result = new Field();
        return this;
      }

      /**
       * Merge the values in {@code other} into this field. For each list of values, {@code other}'s
       * values are append to the ones in this field.
       */
      public Builder mergeFrom(Field other) {
        if (!other.varint.isEmpty()) {
          if (result.varint == null) {
            result.varint = new ArrayList<Long>();
          }
          result.varint.addAll(other.varint);
        }
        if (!other.fixed32.isEmpty()) {
          if (result.fixed32 == null) {
            result.fixed32 = new ArrayList<Integer>();
          }
          result.fixed32.addAll(other.fixed32);
        }
        if (!other.fixed64.isEmpty()) {
          if (result.fixed64 == null) {
            result.fixed64 = new ArrayList<>();
          }
          result.fixed64.addAll(other.fixed64);
        }
        if (!other.lengthDelimited.isEmpty()) {
          if (result.lengthDelimited == null) {
            result.lengthDelimited = new ArrayList<>();
          }
          result.lengthDelimited.addAll(other.lengthDelimited);
        }
        if (!other.group.isEmpty()) {
          if (result.group == null) {
            result.group = new ArrayList<>();
          }
          result.group.addAll(other.group);
        }
        return this;
      }

      /** Add a varint value. */
      public Builder addVarint(long value) {
        if (result.varint == null) {
          result.varint = new ArrayList<>();
        }
        result.varint.add(value);
        return this;
      }

      /** Add a fixed32 value. */
      public Builder addFixed32(int value) {
        if (result.fixed32 == null) {
          result.fixed32 = new ArrayList<>();
        }
        result.fixed32.add(value);
        return this;
      }

      /** Add a fixed64 value. */
      public Builder addFixed64(long value) {
        if (result.fixed64 == null) {
          result.fixed64 = new ArrayList<>();
        }
        result.fixed64.add(value);
        return this;
      }

      /** Add a length-delimited value. */
      public Builder addLengthDelimited(ByteString value) {
        if (result.lengthDelimited == null) {
          result.lengthDelimited = new ArrayList<>();
        }
        result.lengthDelimited.add(value);
        return this;
      }

      /** Add an embedded group. */
      public Builder addGroup(UnknownFieldSet value) {
        if (result.group == null) {
          result.group = new ArrayList<>();
        }
        result.group.add(value);
        return this;
      }
    }
  }

  /** Parser to implement MessageLite interface. */
  public static final class Parser extends AbstractParser<UnknownFieldSet> {
    @Override
    public UnknownFieldSet parsePartialFrom(
        CodedInputStream input, ExtensionRegistryLite extensionRegistry)
        throws InvalidProtocolBufferException {
      Builder builder = newBuilder();
      try {
        builder.mergeFrom(input);
      } catch (InvalidProtocolBufferException e) {
        throw e.setUnfinishedMessage(builder.buildPartial());
      } catch (IOException e) {
        throw new InvalidProtocolBufferException(e).setUnfinishedMessage(builder.buildPartial());
      }
      return builder.buildPartial();
    }
  }

  private static final Parser PARSER = new Parser();

  @Override
  public final Parser getParserForType() {
    return PARSER;
  }
}
