// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.
// http://code.google.com/p/protobuf/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package com.google.protobuf;

/**
 * This class is used internally by the Protocol Buffer library and generated
 * message implementations.  It is public only because those generated messages
 * do not reside in the {@code protocol2} package.  Others should not use this
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
