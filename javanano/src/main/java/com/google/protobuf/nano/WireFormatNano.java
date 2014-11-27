// Protocol Buffers - Google's data interchange format
// Copyright 2013 Google Inc.  All rights reserved.
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

package com.google.protobuf.nano;

import java.io.IOException;

/**
 * This class is used internally by the Protocol Buffer library and generated
 * message implementations.  It is public only because those generated messages
 * do not reside in the {@code protobuf} package.  Others should not use this
 * class directly.
 *
 * This class contains constants and helper functions useful for dealing with
 * the Protocol Buffer wire format.
 *
 * @author kenton@google.com Kenton Varda
 */
public final class WireFormatNano {
  // Do not allow instantiation.
  private WireFormatNano() {}

  static final int WIRETYPE_VARINT           = 0;
  static final int WIRETYPE_FIXED64          = 1;
  static final int WIRETYPE_LENGTH_DELIMITED = 2;
  static final int WIRETYPE_START_GROUP      = 3;
  static final int WIRETYPE_END_GROUP        = 4;
  static final int WIRETYPE_FIXED32          = 5;

  static final int TAG_TYPE_BITS = 3;
  static final int TAG_TYPE_MASK = (1 << TAG_TYPE_BITS) - 1;

  /** Given a tag value, determines the wire type (the lower 3 bits). */
  static int getTagWireType(final int tag) {
    return tag & TAG_TYPE_MASK;
  }

  /** Given a tag value, determines the field number (the upper 29 bits). */
  public static int getTagFieldNumber(final int tag) {
    return tag >>> TAG_TYPE_BITS;
  }

  /** Makes a tag value given a field number and wire type. */
  static int makeTag(final int fieldNumber, final int wireType) {
    return (fieldNumber << TAG_TYPE_BITS) | wireType;
  }

  public static final int EMPTY_INT_ARRAY[] = {};
  public static final long EMPTY_LONG_ARRAY[] = {};
  public static final float EMPTY_FLOAT_ARRAY[] = {};
  public static final double EMPTY_DOUBLE_ARRAY[] = {};
  public static final boolean EMPTY_BOOLEAN_ARRAY[] = {};
  public static final String EMPTY_STRING_ARRAY[] = {};
  public static final byte[] EMPTY_BYTES_ARRAY[] = {};
  public static final byte[] EMPTY_BYTES = {};

  /**
   * Parses an unknown field. This implementation skips the field.
   *
   * <p>Generated messages will call this for unknown fields if the store_unknown_fields
   * option is off.
   *
   * @return {@literal true} unless the tag is an end-group tag.
   */
  public static boolean parseUnknownField(
      final CodedInputByteBufferNano input,
      final int tag) throws IOException {
    return input.skipField(tag);
  }

  /**
   * Computes the array length of a repeated field. We assume that in the common case repeated
   * fields are contiguously serialized but we still correctly handle interspersed values of a
   * repeated field (but with extra allocations).
   *
   * Rewinds to current input position before returning.
   *
   * @param input stream input, pointing to the byte after the first tag
   * @param tag repeated field tag just read
   * @return length of array
   * @throws IOException
   */
  public static final int getRepeatedFieldArrayLength(
      final CodedInputByteBufferNano input,
      final int tag) throws IOException {
    int arrayLength = 1;
    int startPos = input.getPosition();
    input.skipField(tag);
    while (input.readTag() == tag) {
      input.skipField(tag);
      arrayLength++;
    }
    input.rewindToPosition(startPos);
    return arrayLength;
  }

}
