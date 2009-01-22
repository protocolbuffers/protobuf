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
public final class WireFormat {
  // Do not allow instantiation.
  private WireFormat() {}

  static final int WIRETYPE_VARINT           = 0;
  static final int WIRETYPE_FIXED64          = 1;
  static final int WIRETYPE_LENGTH_DELIMITED = 2;
  static final int WIRETYPE_START_GROUP      = 3;
  static final int WIRETYPE_END_GROUP        = 4;
  static final int WIRETYPE_FIXED32          = 5;

  static final int TAG_TYPE_BITS = 3;
  static final int TAG_TYPE_MASK = (1 << TAG_TYPE_BITS) - 1;

  /** Given a tag value, determines the wire type (the lower 3 bits). */
  static int getTagWireType(int tag) {
    return tag & TAG_TYPE_MASK;
  }

  /** Given a tag value, determines the field number (the upper 29 bits). */
  public static int getTagFieldNumber(int tag) {
    return tag >>> TAG_TYPE_BITS;
  }

  /** Makes a tag value given a field number and wire type. */
  static int makeTag(int fieldNumber, int wireType) {
    return (fieldNumber << TAG_TYPE_BITS) | wireType;
  }

  static int getWireFormatForFieldType(Descriptors.FieldDescriptor.Type type) {
    switch (type) {
      case DOUBLE  : return WIRETYPE_FIXED64;
      case FLOAT   : return WIRETYPE_FIXED32;
      case INT64   : return WIRETYPE_VARINT;
      case UINT64  : return WIRETYPE_VARINT;
      case INT32   : return WIRETYPE_VARINT;
      case FIXED64 : return WIRETYPE_FIXED64;
      case FIXED32 : return WIRETYPE_FIXED32;
      case BOOL    : return WIRETYPE_VARINT;
      case STRING  : return WIRETYPE_LENGTH_DELIMITED;
      case GROUP   : return WIRETYPE_START_GROUP;
      case MESSAGE : return WIRETYPE_LENGTH_DELIMITED;
      case BYTES   : return WIRETYPE_LENGTH_DELIMITED;
      case UINT32  : return WIRETYPE_VARINT;
      case ENUM    : return WIRETYPE_VARINT;
      case SFIXED32: return WIRETYPE_FIXED32;
      case SFIXED64: return WIRETYPE_FIXED64;
      case SINT32  : return WIRETYPE_VARINT;
      case SINT64  : return WIRETYPE_VARINT;
    }

    throw new RuntimeException(
      "There is no way to get here, but the compiler thinks otherwise.");
  }

  /** Given a field descriptor, returns the wire type. This differs from
   * getWireFormatForFieldType for packed repeated fields. */
  static int getWireFormatForField(Descriptors.FieldDescriptor descriptor) {
    if (descriptor.getOptions().getPacked()) {
      return WIRETYPE_LENGTH_DELIMITED;
    } else {
      return getWireFormatForFieldType(descriptor.getType());
    }
  }

  // Field numbers for feilds in MessageSet wire format.
  static final int MESSAGE_SET_ITEM    = 1;
  static final int MESSAGE_SET_TYPE_ID = 2;
  static final int MESSAGE_SET_MESSAGE = 3;

  // Tag numbers.
  static final int MESSAGE_SET_ITEM_TAG =
    makeTag(MESSAGE_SET_ITEM, WIRETYPE_START_GROUP);
  static final int MESSAGE_SET_ITEM_END_TAG =
    makeTag(MESSAGE_SET_ITEM, WIRETYPE_END_GROUP);
  static final int MESSAGE_SET_TYPE_ID_TAG =
    makeTag(MESSAGE_SET_TYPE_ID, WIRETYPE_VARINT);
  static final int MESSAGE_SET_MESSAGE_TAG =
    makeTag(MESSAGE_SET_MESSAGE, WIRETYPE_LENGTH_DELIMITED);
}
