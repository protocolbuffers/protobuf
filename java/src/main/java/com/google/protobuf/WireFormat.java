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

  public static final int WIRETYPE_VARINT           = 0;
  public static final int WIRETYPE_FIXED64          = 1;
  public static final int WIRETYPE_LENGTH_DELIMITED = 2;
  public static final int WIRETYPE_START_GROUP      = 3;
  public static final int WIRETYPE_END_GROUP        = 4;
  public static final int WIRETYPE_FIXED32          = 5;

  static final int TAG_TYPE_BITS = 3;
  static final int TAG_TYPE_MASK = (1 << TAG_TYPE_BITS) - 1;

  /** Given a tag value, determines the wire type (the lower 3 bits). */
  public static int getTagWireType(final int tag) {
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

  /**
   * Lite equivalent to {@link Descriptors.FieldDescriptor.JavaType}.  This is
   * only here to support the lite runtime and should not be used by users.
   */
  public enum JavaType {
    INT(0),
    LONG(0L),
    FLOAT(0F),
    DOUBLE(0D),
    BOOLEAN(false),
    STRING(""),
    BYTE_STRING(ByteString.EMPTY),
    ENUM(null),
    MESSAGE(null);

    JavaType(final Object defaultDefault) {
      this.defaultDefault = defaultDefault;
    }

    /**
     * The default default value for fields of this type, if it's a primitive
     * type.
     */
    Object getDefaultDefault() {
      return defaultDefault;
    }

    private final Object defaultDefault;
  }

  /**
   * Lite equivalent to {@link Descriptors.FieldDescriptor.Type}.  This is
   * only here to support the lite runtime and should not be used by users.
   */
  public enum FieldType {
    DOUBLE  (JavaType.DOUBLE     , WIRETYPE_FIXED64         ),
    FLOAT   (JavaType.FLOAT      , WIRETYPE_FIXED32         ),
    INT64   (JavaType.LONG       , WIRETYPE_VARINT          ),
    UINT64  (JavaType.LONG       , WIRETYPE_VARINT          ),
    INT32   (JavaType.INT        , WIRETYPE_VARINT          ),
    FIXED64 (JavaType.LONG       , WIRETYPE_FIXED64         ),
    FIXED32 (JavaType.INT        , WIRETYPE_FIXED32         ),
    BOOL    (JavaType.BOOLEAN    , WIRETYPE_VARINT          ),
    STRING  (JavaType.STRING     , WIRETYPE_LENGTH_DELIMITED) {
      public boolean isPackable() { return false; }
    },
    GROUP   (JavaType.MESSAGE    , WIRETYPE_START_GROUP     ) {
      public boolean isPackable() { return false; }
    },
    MESSAGE (JavaType.MESSAGE    , WIRETYPE_LENGTH_DELIMITED) {
      public boolean isPackable() { return false; }
    },
    BYTES   (JavaType.BYTE_STRING, WIRETYPE_LENGTH_DELIMITED) {
      public boolean isPackable() { return false; }
    },
    UINT32  (JavaType.INT        , WIRETYPE_VARINT          ),
    ENUM    (JavaType.ENUM       , WIRETYPE_VARINT          ),
    SFIXED32(JavaType.INT        , WIRETYPE_FIXED32         ),
    SFIXED64(JavaType.LONG       , WIRETYPE_FIXED64         ),
    SINT32  (JavaType.INT        , WIRETYPE_VARINT          ),
    SINT64  (JavaType.LONG       , WIRETYPE_VARINT          );

    FieldType(final JavaType javaType, final int wireType) {
      this.javaType = javaType;
      this.wireType = wireType;
    }

    private final JavaType javaType;
    private final int wireType;

    public JavaType getJavaType() { return javaType; }
    public int getWireType() { return wireType; }

    public boolean isPackable() { return true; }
  }

  // Field numbers for fields in MessageSet wire format.
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

  public static final int LITTLE_ENDIAN_32_SIZE = 4;
  public static final int LITTLE_ENDIAN_64_SIZE = 8;

  /** Compute the number of bytes that would be needed to encode a tag. */
  public static int computeTagSize(final int fieldNumber) {
    return computeRawVarint32Size(WireFormat.makeTag(fieldNumber, 0));
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code double} field, including tag.
   */
  public static int computeDoubleSize(final int fieldNumber,
                                      final double value) {
    return computeTagSize(fieldNumber) + computeDoubleSizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code float} field, including tag.
   */
  public static int computeFloatSize(final int fieldNumber, final float value) {
    return computeTagSize(fieldNumber) + computeFloatSizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code uint64} field, including tag.
   */
  public static int computeUInt64Size(final int fieldNumber, final long value) {
    return computeTagSize(fieldNumber) + computeUInt64SizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * {@code int64} field, including tag.
   */
  public static int computeInt64Size(final int fieldNumber, final long value) {
    return computeTagSize(fieldNumber) + computeInt64SizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * {@code int32} field, including tag.
   */
  public static int computeInt32Size(final int fieldNumber, final int value) {
    return computeTagSize(fieldNumber) + computeInt32SizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code fixed64} field, including tag.
   */
  public static int computeFixed64Size(final int fieldNumber,
                                       final long value) {
    return computeTagSize(fieldNumber) + computeFixed64SizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code fixed32} field, including tag.
   */
  public static int computeFixed32Size(final int fieldNumber,
                                       final int value) {
    return computeTagSize(fieldNumber) + computeFixed32SizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code bool} field, including tag.
   */
  public static int computeBoolSize(final int fieldNumber,
                                    final boolean value) {
    return computeTagSize(fieldNumber) + computeBoolSizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code string} field, including tag.
   */
  public static int computeStringSize(final int fieldNumber,
                                      final String value) {
    return computeTagSize(fieldNumber) + computeStringSizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code group} field, including tag.
   */
  public static int computeGroupSize(final int fieldNumber,
                                     final MessageLite value) {
    return computeTagSize(fieldNumber) * 2 + computeGroupSizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * embedded message field, including tag.
   */
  public static int computeMessageSize(final int fieldNumber,
                                       final MessageLite value) {
    return computeTagSize(fieldNumber) + computeMessageSizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code bytes} field, including tag.
   */
  public static int computeBytesSize(final int fieldNumber,
                                     final ByteString value) {
    return computeTagSize(fieldNumber) + computeBytesSizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code bytes} field, including tag.
   */
  public static int computeByteArraySize(final int fieldNumber,
                                         final byte[] value) {
    return WireFormat.computeTagSize(fieldNumber) + computeByteArraySizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * embedded message in lazy field, including tag.
   */
  public static int computeLazyFieldSize(final int fieldNumber,
                                         final LazyFieldLite value) {
    return computeTagSize(fieldNumber) + computeLazyFieldSizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code uint32} field, including tag.
   */
  public static int computeUInt32Size(final int fieldNumber, final int value) {
    return computeTagSize(fieldNumber) + computeUInt32SizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * enum field, including tag.  Caller is responsible for converting the
   * enum value to its numeric value.
   */
  public static int computeEnumSize(final int fieldNumber, final int value) {
    return computeTagSize(fieldNumber) + computeEnumSizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * {@code sfixed32} field, including tag.
   */
  public static int computeSFixed32Size(final int fieldNumber,
                                        final int value) {
    return computeTagSize(fieldNumber) + computeSFixed32SizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * {@code sfixed64} field, including tag.
   */
  public static int computeSFixed64Size(final int fieldNumber,
                                        final long value) {
    return computeTagSize(fieldNumber) + computeSFixed64SizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * {@code sint32} field, including tag.
   */
  public static int computeSInt32Size(final int fieldNumber, final int value) {
    return computeTagSize(fieldNumber) + computeSInt32SizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * {@code sint64} field, including tag.
   */
  public static int computeSInt64Size(final int fieldNumber, final long value) {
    return computeTagSize(fieldNumber) + computeSInt64SizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * MessageSet extension to the stream.  For historical reasons,
   * the wire format differs from normal fields.
   */
  public static int computeMessageSetExtensionSize(
          final int fieldNumber, final MessageLite value) {
    return computeTagSize(WireFormat.MESSAGE_SET_ITEM) * 2 +
            computeUInt32Size(WireFormat.MESSAGE_SET_TYPE_ID, fieldNumber) +
            computeMessageSize(WireFormat.MESSAGE_SET_MESSAGE, value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * unparsed MessageSet extension field to the stream.  For
   * historical reasons, the wire format differs from normal fields.
   */
  public static int computeRawMessageSetExtensionSize(
          final int fieldNumber, final ByteString value) {
    return computeTagSize(WireFormat.MESSAGE_SET_ITEM) * 2 +
            computeUInt32Size(WireFormat.MESSAGE_SET_TYPE_ID, fieldNumber) +
            computeBytesSize(WireFormat.MESSAGE_SET_MESSAGE, value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * lazily parsed MessageSet extension field to the stream.  For
   * historical reasons, the wire format differs from normal fields.
   */
  public static int computeLazyFieldMessageSetExtensionSize(
          final int fieldNumber, final LazyFieldLite value) {
    return computeTagSize(WireFormat.MESSAGE_SET_ITEM) * 2 +
            computeUInt32Size(WireFormat.MESSAGE_SET_TYPE_ID, fieldNumber) +
            computeLazyFieldSize(WireFormat.MESSAGE_SET_MESSAGE, value);
  }

  // -----------------------------------------------------------------

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code double} field, including tag.
   */
  public static int computeDoubleSizeNoTag(@SuppressWarnings("unused") final double value) {
    return LITTLE_ENDIAN_64_SIZE;
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code float} field, including tag.
   */
  public static int computeFloatSizeNoTag(@SuppressWarnings("unused") final float value) {
    return LITTLE_ENDIAN_32_SIZE;
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code uint64} field, including tag.
   */
  public static int computeUInt64SizeNoTag(final long value) {
    return computeRawVarint64Size(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * {@code int64} field, including tag.
   */
  public static int computeInt64SizeNoTag(final long value) {
    return computeRawVarint64Size(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * {@code int32} field, including tag.
   */
  public static int computeInt32SizeNoTag(final int value) {
    if (value >= 0) {
      return computeRawVarint32Size(value);
    } else {
      // Must sign-extend.
      return 10;
    }
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code fixed64} field.
   */
  public static int computeFixed64SizeNoTag(@SuppressWarnings("unused") final long value) {
    return LITTLE_ENDIAN_64_SIZE;
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code fixed32} field.
   */
  public static int computeFixed32SizeNoTag(@SuppressWarnings("unused") final int value) {
    return LITTLE_ENDIAN_32_SIZE;
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code bool} field.
   */
  public static int computeBoolSizeNoTag(@SuppressWarnings("unused") final boolean value) {
    return 1;
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code string} field.
   */
  public static int computeStringSizeNoTag(final String value) {
    int length;
    try {
      length = Utf8.encodedLength(value);
    } catch (Utf8.UnpairedSurrogateException e) {
      // TODO(dweis): Consider using nio Charset methods instead.
      final byte[] bytes = value.getBytes(Internal.UTF_8);
      length = bytes.length;
    }

    return computeRawVarint32Size(length) + length;
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code group} field.
   */
  public static int computeGroupSizeNoTag(final MessageLite value) {
    return value.getSerializedSize();
  }

  /**
   * Compute the number of bytes that would be needed to encode an embedded
   * message field.
   */
  public static int computeMessageSizeNoTag(final MessageLite value) {
    final int size = value.getSerializedSize();
    return computeRawVarint32Size(size) + size;
  }

  /**
   * Compute the number of bytes that would be needed to encode an embedded
   * message stored in lazy field.
   */
  public static int computeLazyFieldSizeNoTag(final LazyFieldLite value) {
    final int size = value.getSerializedSize();
    return computeRawVarint32Size(size) + size;
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code bytes} field.
   */
  public static int computeBytesSizeNoTag(final ByteString value) {
    return computeRawVarint32Size(value.size()) +
            value.size();
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code bytes} field.
   */
  public static int computeByteArraySizeNoTag(final byte[] value) {
    return WireFormat.computeRawVarint32Size(value.length) + value.length;
  }

  /**
   * Compute the number of bytes that would be needed to encode a
   * {@code uint32} field.
   */
  public static int computeUInt32SizeNoTag(final int value) {
    return computeRawVarint32Size(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an enum field.
   * Caller is responsible for converting the enum value to its numeric value.
   */
  public static int computeEnumSizeNoTag(final int value) {
    return computeInt32SizeNoTag(value);
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * {@code sfixed32} field.
   */
  public static int computeSFixed32SizeNoTag(@SuppressWarnings("unused") final int value) {
    return LITTLE_ENDIAN_32_SIZE;
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * {@code sfixed64} field.
   */
  public static int computeSFixed64SizeNoTag(@SuppressWarnings("unused") final long value) {
    return LITTLE_ENDIAN_64_SIZE;
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * {@code sint32} field.
   */
  public static int computeSInt32SizeNoTag(final int value) {
    return computeRawVarint32Size(encodeZigZag32(value));
  }

  /**
   * Compute the number of bytes that would be needed to encode an
   * {@code sint64} field.
   */
  public static int computeSInt64SizeNoTag(final long value) {
    return computeRawVarint64Size(encodeZigZag64(value));
  }

  /**
   * Compute the number of bytes that would be needed to encode a varint. {@code value} is treated
   * as unsigned, so it won't be sign-extended if negative.
   */
  public static int computeRawVarint32Size(final int value) {
    if ((value & (0xffffffff << 7)) == 0) return 1;
    if ((value & (0xffffffff << 14)) == 0) return 2;
    if ((value & (0xffffffff << 21)) == 0) return 3;
    if ((value & (0xffffffff << 28)) == 0) return 4;
    return 5;
  }

  public static int computeRawVarint64Size(final long value) {
    if ((value & (0xffffffffffffffffL <<  7)) == 0) return 1;
    if ((value & (0xffffffffffffffffL << 14)) == 0) return 2;
    if ((value & (0xffffffffffffffffL << 21)) == 0) return 3;
    if ((value & (0xffffffffffffffffL << 28)) == 0) return 4;
    if ((value & (0xffffffffffffffffL << 35)) == 0) return 5;
    if ((value & (0xffffffffffffffffL << 42)) == 0) return 6;
    if ((value & (0xffffffffffffffffL << 49)) == 0) return 7;
    if ((value & (0xffffffffffffffffL << 56)) == 0) return 8;
    if ((value & (0xffffffffffffffffL << 63)) == 0) return 9;
    return 10;
  }

  /**
   * Encode a ZigZag-encoded 32-bit value.  ZigZag encodes signed integers into values that can be
   * efficiently encoded with varint.  (Otherwise, negative values must be sign-extended to 64 bits
   * to be varint encoded, thus always taking 10 bytes on the wire.)
   *
   * @param n A signed 32-bit integer.
   * @return An unsigned 32-bit integer, stored in a signed int because Java has no explicit
   * unsigned support.
   */
  public static int encodeZigZag32(final int n) {
    // Note:  the right-shift must be arithmetic
    return (n << 1) ^ (n >> 31);
  }

  /**
   * Encode a ZigZag-encoded 64-bit value.  ZigZag encodes signed integers into values that can be
   * efficiently encoded with varint.  (Otherwise, negative values must be sign-extended to 64 bits
   * to be varint encoded, thus always taking 10 bytes on the wire.)
   *
   * @param n A signed 64-bit integer.
   * @return An unsigned 64-bit integer, stored in a signed int because Java has no explicit
   * unsigned support.
   */
  public static long encodeZigZag64(final long n) {
    // Note:  the right-shift must be arithmetic
    return (n << 1) ^ (n >> 63);
  }

  /**
   * Validation level for handling incoming string field data which potentially
   * contain non-UTF8 bytes.
   */
  enum Utf8Validation {
    /** Eagerly parses to String; silently accepts invalid UTF8 bytes. */
    LOOSE {
      Object readString(CodedInputStream input) throws IOException {
        return input.readString();
      }
    },
    /** Eagerly parses to String; throws an IOException on invalid bytes. */
    STRICT {
      Object readString(CodedInputStream input) throws IOException {
        return input.readStringRequireUtf8();
      }
    },
    /** Keep data as ByteString; validation/conversion to String is lazy. */
    LAZY {
      Object readString(CodedInputStream input) throws IOException {
        return input.readBytes();
      }
    };

    /** Read a string field from the input with the proper UTF8 validation. */
    abstract Object readString(CodedInputStream input) throws IOException;
  }

  /**
   * Read a field of any primitive type for immutable messages from a
   * CodedInputStream. Enums, groups, and embedded messages are not handled by
   * this method.
   *
   * @param input The stream from which to read.
   * @param type Declared type of the field.
   * @param utf8Validation Different string UTF8 validation level for handling
   *                       string fields.
   * @return An object representing the field's value, of the exact
   *         type which would be returned by
   *         {@link Message#getField(Descriptors.FieldDescriptor)} for
   *         this field.
   */
  static Object readPrimitiveField(
      CodedInputStream input,
      FieldType type,
      Utf8Validation utf8Validation) throws IOException {
    switch (type) {
      case DOUBLE  : return input.readDouble  ();
      case FLOAT   : return input.readFloat   ();
      case INT64   : return input.readInt64   ();
      case UINT64  : return input.readUInt64  ();
      case INT32   : return input.readInt32   ();
      case FIXED64 : return input.readFixed64 ();
      case FIXED32 : return input.readFixed32 ();
      case BOOL    : return input.readBool    ();
      case BYTES   : return input.readBytes   ();
      case UINT32  : return input.readUInt32  ();
      case SFIXED32: return input.readSFixed32();
      case SFIXED64: return input.readSFixed64();
      case SINT32  : return input.readSInt32  ();
      case SINT64  : return input.readSInt64  ();

      case STRING  : return utf8Validation.readString(input);
      case GROUP:
        throw new IllegalArgumentException(
          "readPrimitiveField() cannot handle nested groups.");
      case MESSAGE:
        throw new IllegalArgumentException(
          "readPrimitiveField() cannot handle embedded messages.");
      case ENUM:
        // We don't handle enums because we don't know what to do if the
        // value is not recognized.
        throw new IllegalArgumentException(
          "readPrimitiveField() cannot handle enums.");
    }

    throw new RuntimeException(
      "There is no way to get here, but the compiler thinks otherwise.");
  }
}
