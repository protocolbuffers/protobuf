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

  private static final UnknownFieldSetLite DEFAULT_INSTANCE =
      new UnknownFieldSetLite(ByteString.EMPTY);

  /**
   * Get an empty {@code UnknownFieldSetLite}.
   *
   * <p>For use by generated code only.
   */
  public static UnknownFieldSetLite getDefaultInstance() {
    return DEFAULT_INSTANCE;
  }

  /**
   * Create a new {@link Builder}.
   *
   * <p>For use by generated code only.
   */
  public static Builder newBuilder() {
    return new Builder();
  }

  /**
   * Returns an {@code UnknownFieldSetLite} that is the composite of {@code first} and
   * {@code second}.
   */
  static UnknownFieldSetLite concat(UnknownFieldSetLite first, UnknownFieldSetLite second) {
    return new UnknownFieldSetLite(first.byteString.concat(second.byteString));
  }

  /**
   * The internal representation of the unknown fields.
   */
  private final ByteString byteString;

  /**
   * Constructs the {@code UnknownFieldSetLite} as a thin wrapper around {@link ByteString}.
   */
  private UnknownFieldSetLite(ByteString byteString) {
    this.byteString = byteString;
  }

  /**
   * Serializes the set and writes it to {@code output}.
   *
   * <p>For use by generated code only.
   */
  public void writeTo(CodedOutputStream output) throws IOException {
    output.writeRawBytes(byteString);
  }


  /**
   * Get the number of bytes required to encode this set.
   *
   * <p>For use by generated code only.
   */
  public int getSerializedSize() {
    return byteString.size();
  }

  @Override
  public boolean equals(Object obj) {
    if (this == obj) {
      return true;
    }

    if (obj instanceof UnknownFieldSetLite) {
      return byteString.equals(((UnknownFieldSetLite) obj).byteString);
    }

    return false;
  }

  @Override
  public int hashCode() {
    return byteString.hashCode();
  }

  /**
   * Builder for {@link UnknownFieldSetLite}s.
   *
   * <p>Use {@link UnknownFieldSet#newBuilder()} to construct a {@code Builder}.
   *
   * <p>For use by generated code only.
   */
  public static final class Builder {

    private ByteString.Output byteStringOutput;
    private CodedOutputStream output;
    private boolean built;

    /**
     * Constructs a {@code Builder}. Lazily initialized by
     * {@link #ensureInitializedButNotBuilt()}.
     */
    private Builder() {}

    /**
     * Ensures internal state is initialized for use.
     */
    private void ensureInitializedButNotBuilt() {
      if (built) {
        throw new IllegalStateException("Do not reuse UnknownFieldSetLite Builders.");
      }

      if (output == null && byteStringOutput == null) {
          byteStringOutput = ByteString.newOutput(100 /* initialCapacity */);
          output = CodedOutputStream.newInstance(byteStringOutput);
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
    public boolean mergeFieldFrom(final int tag, final CodedInputStream input)
                                  throws IOException {
      ensureInitializedButNotBuilt();

      final int fieldNumber = WireFormat.getTagFieldNumber(tag);
      switch (WireFormat.getTagWireType(tag)) {
        case WireFormat.WIRETYPE_VARINT:
          output.writeUInt64(fieldNumber, input.readInt64());
          return true;
        case WireFormat.WIRETYPE_FIXED32:
          output.writeFixed32(fieldNumber, input.readFixed32());
          return true;
        case WireFormat.WIRETYPE_FIXED64:
          output.writeFixed64(fieldNumber, input.readFixed64());
          return true;
        case WireFormat.WIRETYPE_LENGTH_DELIMITED:
          output.writeBytes(fieldNumber, input.readBytes());
          return true;
        case WireFormat.WIRETYPE_START_GROUP:
          final Builder subBuilder = newBuilder();
          subBuilder.mergeFrom(input);
          input.checkLastTagWas(
              WireFormat.makeTag(fieldNumber, WireFormat.WIRETYPE_END_GROUP));

          output.writeTag(fieldNumber, WireFormat.WIRETYPE_START_GROUP);
          subBuilder.build().writeTo(output);
          output.writeTag(fieldNumber, WireFormat.WIRETYPE_END_GROUP);
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
    public Builder mergeVarintField(int fieldNumber, int value) {
      if (fieldNumber == 0) {
        throw new IllegalArgumentException("Zero is not a valid field number.");
      }
      ensureInitializedButNotBuilt();
      try {
        output.writeUInt64(fieldNumber, value);
      } catch (IOException e) {
        // Should never happen.
      }
      return this;
    }

    /**
     * Convenience method for merging a length-delimited field.
     *
     * <p>For use by generated code only.
     */
    public Builder mergeLengthDelimitedField(
        final int fieldNumber, final ByteString value) {  
      if (fieldNumber == 0) {
        throw new IllegalArgumentException("Zero is not a valid field number.");
      }
      ensureInitializedButNotBuilt();
      try {
        output.writeBytes(fieldNumber, value);
      } catch (IOException e) {
        // Should never happen.
      }
      return this;
    }

    /**
     * Build the {@link UnknownFieldSetLite} and return it.
     *
     * <p>Once {@code build()} has been called, the {@code Builder} will no
     * longer be usable.  Calling any method after {@code build()} will result
     * in undefined behavior and can cause a {@code IllegalStateException} to be
     * thrown.
     *
     * <p>For use by generated code only.
     */
    public UnknownFieldSetLite build() {
      if (built) {
        throw new IllegalStateException("Do not reuse UnknownFieldSetLite Builders.");
      }

      built = true;

      final UnknownFieldSetLite result;
      // If we were never initialized, no data was written.
      if (output == null) {
        result = getDefaultInstance();
      } else {
        try {
          output.flush();
        } catch (IOException e) {
          // Should never happen.
        }
        ByteString byteString = byteStringOutput.toByteString();
        if (byteString.isEmpty()) {
          result = getDefaultInstance();
        } else {
          result = new UnknownFieldSetLite(byteString);
        }
      }

      // Allow for garbage collection.
      output = null;
      byteStringOutput = null;
      return result;
    }

    /**
     * Parse an entire message from {@code input} and merge its fields into
     * this set.
     */
    private Builder mergeFrom(final CodedInputStream input) throws IOException {
      // Ensures initialization in mergeFieldFrom.
      while (true) {
        final int tag = input.readTag();
        if (tag == 0 || !mergeFieldFrom(tag, input)) {
          break;
        }
      }
      return this;
    }
  }
}
