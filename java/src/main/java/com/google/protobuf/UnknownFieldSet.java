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

import com.google.protobuf.Internal.ThreadLocalQuickQueue;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

/**
 * {@code UnknownFieldSet} is used to keep track of fields which were seen when
 * parsing a protocol message but whose field numbers or types are unrecognized.
 * This most frequently occurs when new fields are added to a message type
 * and then messages containing those feilds are read by old software that was
 * compiled before the new types were added.
 *
 * <p>Every {@link Message} contains an {@code UnknownFieldSet} (and every
 * {@link Message.Builder} contains an {@link Builder}).
 *
 * <p>Most users will never need to use this class.
 *
 * @author kenton@google.com Kenton Varda
 */
public final class UnknownFieldSet implements MessageLite {
  private UnknownFieldSet() {}

  /** Create a new {@link Builder}. */
  public static Builder newBuilder() {
    return Builder.create();
  }

  /**
   * Create a new {@link Builder} and initialize it to be a copy
   * of {@code copyFrom}.
   */
  public static Builder newBuilder(final UnknownFieldSet copyFrom) {
    return newBuilder().mergeFrom(copyFrom);
  }

  /** Get an empty {@code UnknownFieldSet}. */
  public static UnknownFieldSet getDefaultInstance() {
    return defaultInstance;
  }
  public UnknownFieldSet getDefaultInstanceForType() {
    return defaultInstance;
  }
  private static final UnknownFieldSet defaultInstance =
    new UnknownFieldSet(Collections.<Integer, Field>emptyMap());

  /**
   * Construct an {@code UnknownFieldSet} around the given map.  The map is
   * expected to be immutable.
   */
  private UnknownFieldSet(final Map<Integer, Field> fields) {
    this.fields = fields;
  }
  private Map<Integer, Field> fields;

  @Override
  public boolean equals(final Object other) {
    if (this == other) {
      return true;
    }
    return (other instanceof UnknownFieldSet) &&
           fields.equals(((UnknownFieldSet) other).fields);
  }

  @Override
  public int hashCode() {
    return fields.hashCode();
  }

  /** Get a map of fields in the set by number. */
  public Map<Integer, Field> asMap() {
    return fields;
  }

  /** Check if the given field number is present in the set. */
  public boolean hasField(final int number) {
    return fields.containsKey(number);
  }

  /**
   * Get a field by number.  Returns an empty field if not present.  Never
   * returns {@code null}.
   */
  public Field getField(final int number) {
    final Field result = fields.get(number);
    return (result == null) ? Field.getDefaultInstance() : result;
  }

  /** Serializes the set and writes it to {@code output}. */
  public void writeTo(final CodedOutputStream output) throws IOException {
    for (final Map.Entry<Integer, Field> entry : fields.entrySet()) {
      entry.getValue().writeTo(entry.getKey(), output);
    }
  }

  /**
   * Converts the set to a string in protocol buffer text format. This is
   * just a trivial wrapper around
   * {@link TextFormat#printToString(UnknownFieldSet)}.
   */
  @Override
  public String toString() {
    return TextFormat.printToString(this);
  }

  /**
   * Serializes the message to a {@code ByteString} and returns it. This is
   * just a trivial wrapper around {@link #writeTo(CodedOutputStream)}.
   */
  public ByteString toByteString() {
    try {
      final ByteString.CodedBuilder out =
        ByteString.newCodedBuilder(getSerializedSize());
      writeTo(out.getCodedOutput());
      return out.build();
    } catch (final IOException e) {
      throw new RuntimeException(
        "Serializing to a ByteString threw an IOException (should " +
        "never happen).", e);
    }
  }

  /**
   * Serializes the message to a {@code byte} array and returns it.  This is
   * just a trivial wrapper around {@link #writeTo(CodedOutputStream)}.
   */
  public byte[] toByteArray() {
    try {
      final byte[] result = new byte[getSerializedSize()];
      final CodedOutputStream output = CodedOutputStream.newInstance(result);
      writeTo(output);
      output.checkNoSpaceLeft();
      return result;
    } catch (final IOException e) {
      throw new RuntimeException(
        "Serializing to a byte array threw an IOException " +
        "(should never happen).", e);
    }
  }

  /**
   * Serializes the message and writes it to {@code output}.  This is just a
   * trivial wrapper around {@link #writeTo(CodedOutputStream)}.
   */
  public void writeTo(final OutputStream output) throws IOException {
    final CodedOutputStream codedOutput = CodedOutputStream.newInstance(output);
    writeTo(codedOutput);
    codedOutput.flush();
  }

  public void writeDelimitedTo(OutputStream output) throws IOException {
    final CodedOutputStream codedOutput = CodedOutputStream.newInstance(output);
    codedOutput.writeRawVarint32(getSerializedSize());
    writeTo(codedOutput);
    codedOutput.flush();
  }

  /** Get the number of bytes required to encode this set. */
  public int getSerializedSize() {
    int result = 0;
    for (final Map.Entry<Integer, Field> entry : fields.entrySet()) {
      result += entry.getValue().getSerializedSize(entry.getKey());
    }
    return result;
  }

  /**
   * Serializes the set and writes it to {@code output} using
   * {@code MessageSet} wire format.
   */
  public void writeAsMessageSetTo(final CodedOutputStream output)
      throws IOException {
    for (final Map.Entry<Integer, Field> entry : fields.entrySet()) {
      entry.getValue().writeAsMessageSetExtensionTo(
        entry.getKey(), output);
    }
  }

  /**
   * Get the number of bytes required to encode this set using
   * {@code MessageSet} wire format.
   */
  public int getSerializedSizeAsMessageSet() {
    int result = 0;
    for (final Map.Entry<Integer, Field> entry : fields.entrySet()) {
      result += entry.getValue().getSerializedSizeAsMessageSetExtension(
        entry.getKey());
    }
    return result;
  }

  public boolean isInitialized() {
    // UnknownFieldSets do not have required fields, so they are always
    // initialized.
    return true;
  }

  /** Parse an {@code UnknownFieldSet} from the given input stream. */
  public static UnknownFieldSet parseFrom(final CodedInputStream input)
                                          throws IOException {
    return newBuilder().mergeFrom(input).build();
  }

  /** Parse {@code data} as an {@code UnknownFieldSet} and return it. */
  public static UnknownFieldSet parseFrom(final ByteString data)
      throws InvalidProtocolBufferException {
    return newBuilder().mergeFrom(data).build();
  }

  /** Parse {@code data} as an {@code UnknownFieldSet} and return it. */
  public static UnknownFieldSet parseFrom(final byte[] data)
      throws InvalidProtocolBufferException {
    return newBuilder().mergeFrom(data).build();
  }

  /** Parse an {@code UnknownFieldSet} from {@code input} and return it. */
  public static UnknownFieldSet parseFrom(final InputStream input)
                                          throws IOException {
    return newBuilder().mergeFrom(input).build();
  }

  public Builder newBuilderForType() {
    return newBuilder();
  }

  public Builder toBuilder() {
    return newBuilder().mergeFrom(this);
  }

  /**
   * Builder for {@link UnknownFieldSet}s.
   *
   * <p>Note that this class maintains {@link Field.Builder}s for all fields
   * in the set.  Thus, adding one element to an existing {@link Field} does not
   * require making a copy.  This is important for efficient parsing of
   * unknown repeated fields.  However, it implies that {@link Field}s cannot
   * be constructed independently, nor can two {@link UnknownFieldSet}s share
   * the same {@code Field} object.
   *
   * <p>Use {@link UnknownFieldSet#newBuilder()} to construct a {@code Builder}.
   */
  public static final class Builder implements MessageLite.Builder {
    private static ThreadLocalQuickQueue<Builder> builders =
      new ThreadLocalQuickQueue<Builder>();

    // This constructor should never be called directly (except from 'create').
    private Builder() {}

    private Map<Integer, Field> fields;

    // Optimization:  We keep around a builder for the last field that was
    //   modified so that we can efficiently add to it multiple times in a
    //   row (important when parsing an unknown repeated field).
    private int lastFieldNumber;
    private Field.Builder lastField;

    private static Builder create() {
      Builder builder = builders.get().poll();
      if (builder == null) {
        builder = new Builder();
      }
      builder.reinitialize();
      return builder;
    }

    /**
     * Get a field builder for the given field number which includes any
     * values that already exist.
     */
    private Field.Builder getFieldBuilder(final int number) {
      if (lastField != null) {
        if (number == lastFieldNumber) {
          return lastField;
        }
        // Note:  addField() will reset lastField and lastFieldNumber.
        addField(lastFieldNumber, lastField.build());
      }
      if (number == 0) {
        return null;
      } else {
        final Field existing = fields.get(number);
        lastFieldNumber = number;
        lastField = Field.newBuilder();
        if (existing != null) {
          lastField.mergeFrom(existing);
        }
        return lastField;
      }
    }

    /**
     * Build the {@link UnknownFieldSet} and return it.
     *
     * <p>Once {@code build()} has been called, the {@code Builder} will no
     * longer be usable.  Calling any method after {@code build()} will result
     * in undefined behavior and can cause a {@code NullPointerException} to be
     * thrown.
     */
    public UnknownFieldSet build() {
      getFieldBuilder(0);  // Force lastField to be built.
      final UnknownFieldSet result;
      if (fields.isEmpty()) {
        result = getDefaultInstance();
      } else {
        result = new UnknownFieldSet(Collections.unmodifiableMap(fields));
      }
      fields = null;
      builders.get().offer(this);
      return result;
    }

    public UnknownFieldSet buildPartial() {
      // No required fields, so this is the same as build().
      return build();
    }

    @Override
    public Builder clone() {
      getFieldBuilder(0);  // Force lastField to be built.
      return UnknownFieldSet.newBuilder().mergeFrom(
          new UnknownFieldSet(fields));
    }

    public UnknownFieldSet getDefaultInstanceForType() {
      return UnknownFieldSet.getDefaultInstance();
    }

    private void reinitialize() {
      fields = Collections.emptyMap();
      lastFieldNumber = 0;
      lastField = null;
    }

    /** Reset the builder to an empty set. */
    public Builder clear() {
      reinitialize();
      return this;
    }

    /**
     * Merge the fields from {@code other} into this set.  If a field number
     * exists in both sets, {@code other}'s values for that field will be
     * appended to the values in this set.
     */
    public Builder mergeFrom(final UnknownFieldSet other) {
      if (other != getDefaultInstance()) {
        for (final Map.Entry<Integer, Field> entry : other.fields.entrySet()) {
          mergeField(entry.getKey(), entry.getValue());
        }
      }
      return this;
    }

    /**
     * Add a field to the {@code UnknownFieldSet}.  If a field with the same
     * number already exists, the two are merged.
     */
    public Builder mergeField(final int number, final Field field) {
      if (number == 0) {
        throw new IllegalArgumentException("Zero is not a valid field number.");
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
     * Convenience method for merging a new field containing a single varint
     * value.  This is used in particular when an unknown enum value is
     * encountered.
     */
    public Builder mergeVarintField(final int number, final int value) {
      if (number == 0) {
        throw new IllegalArgumentException("Zero is not a valid field number.");
      }
      getFieldBuilder(number).addVarint(value);
      return this;
    }

    /** Check if the given field number is present in the set. */
    public boolean hasField(final int number) {
      if (number == 0) {
        throw new IllegalArgumentException("Zero is not a valid field number.");
      }
      return number == lastFieldNumber || fields.containsKey(number);
    }

    /**
     * Add a field to the {@code UnknownFieldSet}.  If a field with the same
     * number already exists, it is removed.
     */
    public Builder addField(final int number, final Field field) {
      if (number == 0) {
        throw new IllegalArgumentException("Zero is not a valid field number.");
      }
      if (lastField != null && lastFieldNumber == number) {
        // Discard this.
        lastField = null;
        lastFieldNumber = 0;
      }
      if (fields.isEmpty()) {
        fields = new TreeMap<Integer,Field>();
      }
      fields.put(number, field);
      return this;
    }

    /**
     * Get all present {@code Field}s as an immutable {@code Map}.  If more
     * fields are added, the changes may or may not be reflected in this map.
     */
    public Map<Integer, Field> asMap() {
      getFieldBuilder(0);  // Force lastField to be built.
      return Collections.unmodifiableMap(fields);
    }

    /**
     * Parse an entire message from {@code input} and merge its fields into
     * this set.
     */
    public Builder mergeFrom(final CodedInputStream input) throws IOException {
      while (true) {
        final int tag = input.readTag();
        if (tag == 0 || !mergeFieldFrom(tag, input)) {
          break;
        }
      }
      return this;
    }

    /**
     * Parse a single field from {@code input} and merge it into this set.
     * @param tag The field's tag number, which was already parsed.
     * @return {@code false} if the tag is an engroup tag.
     */
    public boolean mergeFieldFrom(final int tag, final CodedInputStream input)
                                  throws IOException {
      final int number = WireFormat.getTagFieldNumber(tag);
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
          final Builder subBuilder = newBuilder();
          input.readGroup(number, subBuilder,
                          ExtensionRegistry.getEmptyRegistry());
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
     * Parse {@code data} as an {@code UnknownFieldSet} and merge it with the
     * set being built.  This is just a small wrapper around
     * {@link #mergeFrom(CodedInputStream)}.
     */
    public Builder mergeFrom(final ByteString data)
        throws InvalidProtocolBufferException {
      try {
        final CodedInputStream input = data.newCodedInput();
        mergeFrom(input);
        input.checkLastTagWas(0);
        return this;
      } catch (final InvalidProtocolBufferException e) {
        throw e;
      } catch (final IOException e) {
        throw new RuntimeException(
          "Reading from a ByteString threw an IOException (should " +
          "never happen).", e);
      }
    }

    /**
     * Parse {@code data} as an {@code UnknownFieldSet} and merge it with the
     * set being built.  This is just a small wrapper around
     * {@link #mergeFrom(CodedInputStream)}.
     */
    public Builder mergeFrom(final byte[] data)
        throws InvalidProtocolBufferException {
      try {
        final CodedInputStream input = CodedInputStream.newInstance(data);
        mergeFrom(input);
        input.checkLastTagWas(0);
        return this;
      } catch (final InvalidProtocolBufferException e) {
        throw e;
      } catch (final IOException e) {
        throw new RuntimeException(
          "Reading from a byte array threw an IOException (should " +
          "never happen).", e);
      }
    }

    /**
     * Parse an {@code UnknownFieldSet} from {@code input} and merge it with the
     * set being built.  This is just a small wrapper around
     * {@link #mergeFrom(CodedInputStream)}.
     */
    public Builder mergeFrom(final InputStream input) throws IOException {
      final CodedInputStream codedInput = CodedInputStream.newInstance(input);
      mergeFrom(codedInput);
      codedInput.checkLastTagWas(0);
      return this;
    }

    public Builder mergeDelimitedFrom(InputStream input)
        throws IOException {
      final int size = CodedInputStream.readRawVarint32(input);
      final InputStream limitedInput =
        new AbstractMessage.Builder.LimitedInputStream(input, size);
      return mergeFrom(limitedInput, null);
    }

    public Builder mergeDelimitedFrom(
        InputStream input,
        ExtensionRegistryLite extensionRegistry) throws IOException {
      // UnknownFieldSet has no extensions.
      return mergeFrom(input);
    }

    public Builder mergeFrom(
        CodedInputStream input,
        ExtensionRegistryLite extensionRegistry) throws IOException {
      // UnknownFieldSet has no extensions.
      return mergeFrom(input);
    }

    public Builder mergeFrom(
        ByteString data,
        ExtensionRegistryLite extensionRegistry)
        throws InvalidProtocolBufferException {
      // UnknownFieldSet has no extensions.
      return mergeFrom(data);
    }

    public Builder mergeFrom(byte[] data, int off, int len)
        throws InvalidProtocolBufferException {
      try {
        final CodedInputStream input =
            CodedInputStream.newInstance(data, off, len);
        mergeFrom(input);
        input.checkLastTagWas(0);
        return this;
      } catch (InvalidProtocolBufferException e) {
        throw e;
      } catch (IOException e) {
        throw new RuntimeException(
          "Reading from a byte array threw an IOException (should " +
          "never happen).", e);
      }
    }

    public Builder mergeFrom(
        byte[] data,
        ExtensionRegistryLite extensionRegistry)
        throws InvalidProtocolBufferException {
      // UnknownFieldSet has no extensions.
      return mergeFrom(data);
    }

    public Builder mergeFrom(
        byte[] data, int off, int len,
        ExtensionRegistryLite extensionRegistry)
        throws InvalidProtocolBufferException {
      // UnknownFieldSet has no extensions.
      return mergeFrom(data, off, len);
    }

    public Builder mergeFrom(
        InputStream input,
        ExtensionRegistryLite extensionRegistry) throws IOException {
      // UnknownFieldSet has no extensions.
      return mergeFrom(input);
    }

    public boolean isInitialized() {
      // UnknownFieldSets do not have required fields, so they are always
      // initialized.
      return true;
    }
  }

  /**
   * Represents a single field in an {@code UnknownFieldSet}.
   *
   * <p>A {@code Field} consists of five lists of values.  The lists correspond
   * to the five "wire types" used in the protocol buffer binary format.
   * The wire type of each field can be determined from the encoded form alone,
   * without knowing the field's declared type.  So, we are able to parse
   * unknown values at least this far and separate them.  Normally, only one
   * of the five lists will contain any values, since it is impossible to
   * define a valid message type that declares two different types for the
   * same field number.  However, the code is designed to allow for the case
   * where the same unknown field number is encountered using multiple different
   * wire types.
   *
   * <p>{@code Field} is an immutable class.  To construct one, you must use a
   * {@link Builder}.
   *
   * @see UnknownFieldSet
   */
  public static final class Field {
    private Field() {}

    /** Construct a new {@link Builder}. */
    public static Builder newBuilder() {
      return Builder.create();
    }

    /**
     * Construct a new {@link Builder} and initialize it to a copy of
     * {@code copyFrom}.
     */
    public static Builder newBuilder(final Field copyFrom) {
      return newBuilder().mergeFrom(copyFrom);
    }

    /** Get an empty {@code Field}. */
    public static Field getDefaultInstance() {
      return fieldDefaultInstance;
    }
    private static final Field fieldDefaultInstance = newBuilder().build();

    /** Get the list of varint values for this field. */
    public List<Long> getVarintList()               { return varint;          }

    /** Get the list of fixed32 values for this field. */
    public List<Integer> getFixed32List()           { return fixed32;         }

    /** Get the list of fixed64 values for this field. */
    public List<Long> getFixed64List()              { return fixed64;         }

    /** Get the list of length-delimited values for this field. */
    public List<ByteString> getLengthDelimitedList() { return lengthDelimited; }

    /**
     * Get the list of embedded group values for this field.  These are
     * represented using {@link UnknownFieldSet}s rather than {@link Message}s
     * since the group's type is presumably unknown.
     */
    public List<UnknownFieldSet> getGroupList()      { return group;           }

    @Override
    public boolean equals(final Object other) {
      if (this == other) {
        return true;
      }
      if (!(other instanceof Field)) {
        return false;
      }
      return Arrays.equals(getIdentityArray(),
          ((Field) other).getIdentityArray());
    }

    @Override
    public int hashCode() {
      return Arrays.hashCode(getIdentityArray());
    }

    /**
     * Returns the array of objects to be used to uniquely identify this
     * {@link Field} instance.
     */
    private Object[] getIdentityArray() {
      return new Object[] {
          varint,
          fixed32,
          fixed64,
          lengthDelimited,
          group};
    }

    /**
     * Serializes the field, including field number, and writes it to
     * {@code output}.
     */
    public void writeTo(final int fieldNumber, final CodedOutputStream output)
                        throws IOException {
      for (final long value : varint) {
        output.writeUInt64(fieldNumber, value);
      }
      for (final int value : fixed32) {
        output.writeFixed32(fieldNumber, value);
      }
      for (final long value : fixed64) {
        output.writeFixed64(fieldNumber, value);
      }
      for (final ByteString value : lengthDelimited) {
        output.writeBytes(fieldNumber, value);
      }
      for (final UnknownFieldSet value : group) {
        output.writeGroup(fieldNumber, value);
      }
    }

    /**
     * Get the number of bytes required to encode this field, including field
     * number.
     */
    public int getSerializedSize(final int fieldNumber) {
      int result = 0;
      for (final long value : varint) {
        result += CodedOutputStream.computeUInt64Size(fieldNumber, value);
      }
      for (final int value : fixed32) {
        result += CodedOutputStream.computeFixed32Size(fieldNumber, value);
      }
      for (final long value : fixed64) {
        result += CodedOutputStream.computeFixed64Size(fieldNumber, value);
      }
      for (final ByteString value : lengthDelimited) {
        result += CodedOutputStream.computeBytesSize(fieldNumber, value);
      }
      for (final UnknownFieldSet value : group) {
        result += CodedOutputStream.computeGroupSize(fieldNumber, value);
      }
      return result;
    }

    /**
     * Serializes the field, including field number, and writes it to
     * {@code output}, using {@code MessageSet} wire format.
     */
    public void writeAsMessageSetExtensionTo(
        final int fieldNumber,
        final CodedOutputStream output)
        throws IOException {
      for (final ByteString value : lengthDelimited) {
        output.writeRawMessageSetExtension(fieldNumber, value);
      }
    }

    /**
     * Get the number of bytes required to encode this field, including field
     * number, using {@code MessageSet} wire format.
     */
    public int getSerializedSizeAsMessageSetExtension(final int fieldNumber) {
      int result = 0;
      for (final ByteString value : lengthDelimited) {
        result += CodedOutputStream.computeRawMessageSetExtensionSize(
          fieldNumber, value);
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
      private static ThreadLocalQuickQueue<Builder> builders =
        new ThreadLocalQuickQueue<Builder>();

      // This constructor should never be called directly (except from 'create').
      private Builder() {}

      private static Builder create() {
        Builder builder = builders.get().poll();
        if (builder == null) {
          builder = new Builder();
        }

        builder.result = new Field();
        return builder;
      }

      private Field result;

      /**
       * Build the field.  After {@code build()} has been called, the
       * {@code Builder} is no longer usable.  Calling any other method will
       * result in undefined behavior and can cause a
       * {@code NullPointerException} to be thrown.
       */
      public Field build() {
        if (result.varint == null) {
          result.varint = Collections.emptyList();
        } else {
          result.varint = Collections.unmodifiableList(result.varint);
        }
        if (result.fixed32 == null) {
          result.fixed32 = Collections.emptyList();
        } else {
          result.fixed32 = Collections.unmodifiableList(result.fixed32);
        }
        if (result.fixed64 == null) {
          result.fixed64 = Collections.emptyList();
        } else {
          result.fixed64 = Collections.unmodifiableList(result.fixed64);
        }
        if (result.lengthDelimited == null) {
          result.lengthDelimited = Collections.emptyList();
        } else {
          result.lengthDelimited =
            Collections.unmodifiableList(result.lengthDelimited);
        }
        if (result.group == null) {
          result.group = Collections.emptyList();
        } else {
          result.group = Collections.unmodifiableList(result.group);
        }

        final Field returnMe = result;
        result = null;
        builders.get().offer(this);
        return returnMe;
      }

      /** Discard the field's contents. */
      public Builder clear() {
        result = new Field();
        return this;
      }

      /**
       * Merge the values in {@code other} into this field.  For each list
       * of values, {@code other}'s values are append to the ones in this
       * field.
       */
      public Builder mergeFrom(final Field other) {
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
            result.fixed64 = new ArrayList<Long>();
          }
          result.fixed64.addAll(other.fixed64);
        }
        if (!other.lengthDelimited.isEmpty()) {
          if (result.lengthDelimited == null) {
            result.lengthDelimited = new ArrayList<ByteString>();
          }
          result.lengthDelimited.addAll(other.lengthDelimited);
        }
        if (!other.group.isEmpty()) {
          if (result.group == null) {
            result.group = new ArrayList<UnknownFieldSet>();
          }
          result.group.addAll(other.group);
        }
        return this;
      }

      /** Add a varint value. */
      public Builder addVarint(final long value) {
        if (result.varint == null) {
          result.varint = new ArrayList<Long>();
        }
        result.varint.add(value);
        return this;
      }

      /** Add a fixed32 value. */
      public Builder addFixed32(final int value) {
        if (result.fixed32 == null) {
          result.fixed32 = new ArrayList<Integer>();
        }
        result.fixed32.add(value);
        return this;
      }

      /** Add a fixed64 value. */
      public Builder addFixed64(final long value) {
        if (result.fixed64 == null) {
          result.fixed64 = new ArrayList<Long>();
        }
        result.fixed64.add(value);
        return this;
      }

      /** Add a length-delimited value. */
      public Builder addLengthDelimited(final ByteString value) {
        if (result.lengthDelimited == null) {
          result.lengthDelimited = new ArrayList<ByteString>();
        }
        result.lengthDelimited.add(value);
        return this;
      }

      /** Add an embedded group. */
      public Builder addGroup(final UnknownFieldSet value) {
        if (result.group == null) {
          result.group = new ArrayList<UnknownFieldSet>();
        }
        result.group.add(value);
        return this;
      }
    }
  }
}
