#region Copyright notice and license

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://github.com/jskeet/dotnet-protobufs/
// Original C++/Java/Python code:
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

#endregion

using System;
using System.Globalization;
using System.Text;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers
{
    // This part of CodedOutputStream provides all the static entry points that are used
    // by generated code and internally to compute the size of messages prior to being
    // written to an instance of CodedOutputStream.
    public sealed partial class CodedOutputStream
    {
        private const int LittleEndian64Size = 8;
        private const int LittleEndian32Size = 4;

        /// <summary>
        /// Compute the number of bytes that would be needed to encode a
        /// double field, including the tag.
        /// </summary>
        public static int ComputeDoubleSize(int fieldNumber, double value)
        {
            return ComputeTagSize(fieldNumber) + LittleEndian64Size;
        }

        /// <summary>
        /// Compute the number of bytes that would be needed to encode a
        /// float field, including the tag.
        /// </summary>
        public static int ComputeFloatSize(int fieldNumber, float value)
        {
            return ComputeTagSize(fieldNumber) + LittleEndian32Size;
        }

        /// <summary>
        /// Compute the number of bytes that would be needed to encode a
        /// uint64 field, including the tag.
        /// </summary>
        [CLSCompliant(false)]
        public static int ComputeUInt64Size(int fieldNumber, ulong value)
        {
            return ComputeTagSize(fieldNumber) + ComputeRawVarint64Size(value);
        }

        /// <summary>
        /// Compute the number of bytes that would be needed to encode an
        /// int64 field, including the tag.
        /// </summary>
        public static int ComputeInt64Size(int fieldNumber, long value)
        {
            return ComputeTagSize(fieldNumber) + ComputeRawVarint64Size((ulong) value);
        }

        /// <summary>
        /// Compute the number of bytes that would be needed to encode an
        /// int32 field, including the tag.
        /// </summary>
        public static int ComputeInt32Size(int fieldNumber, int value)
        {
            if (value >= 0)
            {
                return ComputeTagSize(fieldNumber) + ComputeRawVarint32Size((uint) value);
            }
            else
            {
                // Must sign-extend.
                return ComputeTagSize(fieldNumber) + 10;
            }
        }

        /// <summary>
        /// Compute the number of bytes that would be needed to encode a
        /// fixed64 field, including the tag.
        /// </summary>
        [CLSCompliant(false)]
        public static int ComputeFixed64Size(int fieldNumber, ulong value)
        {
            return ComputeTagSize(fieldNumber) + LittleEndian64Size;
        }

        /// <summary>
        /// Compute the number of bytes that would be needed to encode a
        /// fixed32 field, including the tag.
        /// </summary>
        [CLSCompliant(false)]
        public static int ComputeFixed32Size(int fieldNumber, uint value)
        {
            return ComputeTagSize(fieldNumber) + LittleEndian32Size;
        }

        /// <summary>
        /// Compute the number of bytes that would be needed to encode a
        /// bool field, including the tag.
        /// </summary>
        public static int ComputeBoolSize(int fieldNumber, bool value)
        {
            return ComputeTagSize(fieldNumber) + 1;
        }

        /// <summary>
        /// Compute the number of bytes that would be needed to encode a
        /// string field, including the tag.
        /// </summary>
        public static int ComputeStringSize(int fieldNumber, String value)
        {
            int byteArraySize = Encoding.UTF8.GetByteCount(value);
            return ComputeTagSize(fieldNumber) +
                   ComputeRawVarint32Size((uint) byteArraySize) +
                   byteArraySize;
        }

        /// <summary>
        /// Compute the number of bytes that would be needed to encode a
        /// group field, including the tag.
        /// </summary>
        public static int ComputeGroupSize(int fieldNumber, IMessageLite value)
        {
            return ComputeTagSize(fieldNumber)*2 + value.SerializedSize;
        }

        /// <summary>
        /// Compute the number of bytes that would be needed to encode a
        /// group field represented by an UnknownFieldSet, including the tag.
        /// </summary>
        [Obsolete]
        public static int ComputeUnknownGroupSize(int fieldNumber,
                                                  IMessageLite value)
        {
            return ComputeTagSize(fieldNumber)*2 + value.SerializedSize;
        }

        /// <summary>
        /// Compute the number of bytes that would be needed to encode an
        /// embedded message field, including the tag.
        /// </summary>
        public static int ComputeMessageSize(int fieldNumber, IMessageLite value)
        {
            int size = value.SerializedSize;
            return ComputeTagSize(fieldNumber) + ComputeRawVarint32Size((uint) size) + size;
        }

        /// <summary>
        /// Compute the number of bytes that would be needed to encode a
        /// bytes field, including the tag.
        /// </summary>
        public static int ComputeBytesSize(int fieldNumber, ByteString value)
        {
            return ComputeTagSize(fieldNumber) +
                   ComputeRawVarint32Size((uint) value.Length) +
                   value.Length;
        }

        /// <summary>
        /// Compute the number of bytes that would be needed to encode a
        /// uint32 field, including the tag.
        /// </summary>
        [CLSCompliant(false)]
        public static int ComputeUInt32Size(int fieldNumber, uint value)
        {
            return ComputeTagSize(fieldNumber) + ComputeRawVarint32Size(value);
        }

        /// <summary>
        /// Compute the number of bytes that would be needed to encode a
        /// enum field, including the tag. The caller is responsible for
        /// converting the enum value to its numeric value.
        /// </summary>
        public static int ComputeEnumSize(int fieldNumber, int value)
        {
            return ComputeTagSize(fieldNumber) + ComputeEnumSizeNoTag(value);
        }

        /// <summary>
        /// Compute the number of bytes that would be needed to encode an
        /// sfixed32 field, including the tag.
        /// </summary>
        public static int ComputeSFixed32Size(int fieldNumber, int value)
        {
            return ComputeTagSize(fieldNumber) + LittleEndian32Size;
        }

        /// <summary>
        /// Compute the number of bytes that would be needed to encode an
        /// sfixed64 field, including the tag.
        /// </summary>
        public static int ComputeSFixed64Size(int fieldNumber, long value)
        {
            return ComputeTagSize(fieldNumber) + LittleEndian64Size;
        }

        /// <summary>
        /// Compute the number of bytes that would be needed to encode an
        /// sint32 field, including the tag.
        /// </summary>
        public static int ComputeSInt32Size(int fieldNumber, int value)
        {
            return ComputeTagSize(fieldNumber) + ComputeRawVarint32Size(EncodeZigZag32(value));
        }

        /// <summary>
        /// Compute the number of bytes that would be needed to encode an
        /// sint64 field, including the tag.
        /// </summary>
        public static int ComputeSInt64Size(int fieldNumber, long value)
        {
            return ComputeTagSize(fieldNumber) + ComputeRawVarint64Size(EncodeZigZag64(value));
        }

        /// <summary>
        /// Compute the number of bytes that would be needed to encode a
        /// double field, including the tag.
        /// </summary>
        public static int ComputeDoubleSizeNoTag(double value)
        {
            return LittleEndian64Size;
        }

        /// <summary>
        /// Compute the number of bytes that would be needed to encode a
        /// float field, including the tag.
        /// </summary>
        public static int ComputeFloatSizeNoTag(float value)
        {
            return LittleEndian32Size;
        }

        /// <summary>
        /// Compute the number of bytes that would be needed to encode a
        /// uint64 field, including the tag.
        /// </summary>
        [CLSCompliant(false)]
        public static int ComputeUInt64SizeNoTag(ulong value)
        {
            return ComputeRawVarint64Size(value);
        }

        /// <summary>
        /// Compute the number of bytes that would be needed to encode an
        /// int64 field, including the tag.
        /// </summary>
        public static int ComputeInt64SizeNoTag(long value)
        {
            return ComputeRawVarint64Size((ulong) value);
        }

        /// <summary>
        /// Compute the number of bytes that would be needed to encode an
        /// int32 field, including the tag.
        /// </summary>
        public static int ComputeInt32SizeNoTag(int value)
        {
            if (value >= 0)
            {
                return ComputeRawVarint32Size((uint) value);
            }
            else
            {
                // Must sign-extend.
                return 10;
            }
        }

        /// <summary>
        /// Compute the number of bytes that would be needed to encode a
        /// fixed64 field, including the tag.
        /// </summary>
        [CLSCompliant(false)]
        public static int ComputeFixed64SizeNoTag(ulong value)
        {
            return LittleEndian64Size;
        }

        /// <summary>
        /// Compute the number of bytes that would be needed to encode a
        /// fixed32 field, including the tag.
        /// </summary>
        [CLSCompliant(false)]
        public static int ComputeFixed32SizeNoTag(uint value)
        {
            return LittleEndian32Size;
        }

        /// <summary>
        /// Compute the number of bytes that would be needed to encode a
        /// bool field, including the tag.
        /// </summary>
        public static int ComputeBoolSizeNoTag(bool value)
        {
            return 1;
        }

        /// <summary>
        /// Compute the number of bytes that would be needed to encode a
        /// string field, including the tag.
        /// </summary>
        public static int ComputeStringSizeNoTag(String value)
        {
            int byteArraySize = Encoding.UTF8.GetByteCount(value);
            return ComputeRawVarint32Size((uint) byteArraySize) +
                   byteArraySize;
        }

        /// <summary>
        /// Compute the number of bytes that would be needed to encode a
        /// group field, including the tag.
        /// </summary>
        public static int ComputeGroupSizeNoTag(IMessageLite value)
        {
            return value.SerializedSize;
        }

        /// <summary>
        /// Compute the number of bytes that would be needed to encode a
        /// group field represented by an UnknownFieldSet, including the tag.
        /// </summary>
        [Obsolete]
        public static int ComputeUnknownGroupSizeNoTag(IMessageLite value)
        {
            return value.SerializedSize;
        }

        /// <summary>
        /// Compute the number of bytes that would be needed to encode an
        /// embedded message field, including the tag.
        /// </summary>
        public static int ComputeMessageSizeNoTag(IMessageLite value)
        {
            int size = value.SerializedSize;
            return ComputeRawVarint32Size((uint) size) + size;
        }

        /// <summary>
        /// Compute the number of bytes that would be needed to encode a
        /// bytes field, including the tag.
        /// </summary>
        public static int ComputeBytesSizeNoTag(ByteString value)
        {
            return ComputeRawVarint32Size((uint) value.Length) +
                   value.Length;
        }

        /// <summary>
        /// Compute the number of bytes that would be needed to encode a
        /// uint32 field, including the tag.
        /// </summary>
        [CLSCompliant(false)]
        public static int ComputeUInt32SizeNoTag(uint value)
        {
            return ComputeRawVarint32Size(value);
        }

        /// <summary>
        /// Compute the number of bytes that would be needed to encode a
        /// enum field, including the tag. The caller is responsible for
        /// converting the enum value to its numeric value.
        /// </summary>
        public static int ComputeEnumSizeNoTag(int value)
        {
            return ComputeInt32SizeNoTag(value);
        }

        /// <summary>
        /// Compute the number of bytes that would be needed to encode an
        /// sfixed32 field, including the tag.
        /// </summary>
        public static int ComputeSFixed32SizeNoTag(int value)
        {
            return LittleEndian32Size;
        }

        /// <summary>
        /// Compute the number of bytes that would be needed to encode an
        /// sfixed64 field, including the tag.
        /// </summary>
        public static int ComputeSFixed64SizeNoTag(long value)
        {
            return LittleEndian64Size;
        }

        /// <summary>
        /// Compute the number of bytes that would be needed to encode an
        /// sint32 field, including the tag.
        /// </summary>
        public static int ComputeSInt32SizeNoTag(int value)
        {
            return ComputeRawVarint32Size(EncodeZigZag32(value));
        }

        /// <summary>
        /// Compute the number of bytes that would be needed to encode an
        /// sint64 field, including the tag.
        /// </summary>
        public static int ComputeSInt64SizeNoTag(long value)
        {
            return ComputeRawVarint64Size(EncodeZigZag64(value));
        }

        /*
     * Compute the number of bytes that would be needed to encode a
     * MessageSet extension to the stream.  For historical reasons,
     * the wire format differs from normal fields.
     */

        /// <summary>
        /// Compute the number of bytes that would be needed to encode a
        /// MessageSet extension to the stream. For historical reasons,
        /// the wire format differs from normal fields.
        /// </summary>
        public static int ComputeMessageSetExtensionSize(int fieldNumber, IMessageLite value)
        {
            return ComputeTagSize(WireFormat.MessageSetField.Item)*2 +
                   ComputeUInt32Size(WireFormat.MessageSetField.TypeID, (uint) fieldNumber) +
                   ComputeMessageSize(WireFormat.MessageSetField.Message, value);
        }

        /// <summary>
        /// Compute the number of bytes that would be needed to encode an
        /// unparsed MessageSet extension field to the stream. For
        /// historical reasons, the wire format differs from normal fields.
        /// </summary>
        public static int ComputeRawMessageSetExtensionSize(int fieldNumber, ByteString value)
        {
            return ComputeTagSize(WireFormat.MessageSetField.Item)*2 +
                   ComputeUInt32Size(WireFormat.MessageSetField.TypeID, (uint) fieldNumber) +
                   ComputeBytesSize(WireFormat.MessageSetField.Message, value);
        }

        /// <summary>
        /// Compute the number of bytes that would be needed to encode a varint.
        /// </summary>
        [CLSCompliant(false)]
        public static int ComputeRawVarint32Size(uint value)
        {
            if ((value & (0xffffffff << 7)) == 0)
            {
                return 1;
            }
            if ((value & (0xffffffff << 14)) == 0)
            {
                return 2;
            }
            if ((value & (0xffffffff << 21)) == 0)
            {
                return 3;
            }
            if ((value & (0xffffffff << 28)) == 0)
            {
                return 4;
            }
            return 5;
        }

        /// <summary>
        /// Compute the number of bytes that would be needed to encode a varint.
        /// </summary>
        [CLSCompliant(false)]
        public static int ComputeRawVarint64Size(ulong value)
        {
            if ((value & (0xffffffffffffffffL << 7)) == 0)
            {
                return 1;
            }
            if ((value & (0xffffffffffffffffL << 14)) == 0)
            {
                return 2;
            }
            if ((value & (0xffffffffffffffffL << 21)) == 0)
            {
                return 3;
            }
            if ((value & (0xffffffffffffffffL << 28)) == 0)
            {
                return 4;
            }
            if ((value & (0xffffffffffffffffL << 35)) == 0)
            {
                return 5;
            }
            if ((value & (0xffffffffffffffffL << 42)) == 0)
            {
                return 6;
            }
            if ((value & (0xffffffffffffffffL << 49)) == 0)
            {
                return 7;
            }
            if ((value & (0xffffffffffffffffL << 56)) == 0)
            {
                return 8;
            }
            if ((value & (0xffffffffffffffffL << 63)) == 0)
            {
                return 9;
            }
            return 10;
        }

        /// <summary>
        /// Compute the number of bytes that would be needed to encode a
        /// field of arbitrary type, including the tag, to the stream.
        /// </summary>
        public static int ComputeFieldSize(FieldType fieldType, int fieldNumber, Object value)
        {
            switch (fieldType)
            {
                case FieldType.Double:
                    return ComputeDoubleSize(fieldNumber, (double) value);
                case FieldType.Float:
                    return ComputeFloatSize(fieldNumber, (float) value);
                case FieldType.Int64:
                    return ComputeInt64Size(fieldNumber, (long) value);
                case FieldType.UInt64:
                    return ComputeUInt64Size(fieldNumber, (ulong) value);
                case FieldType.Int32:
                    return ComputeInt32Size(fieldNumber, (int) value);
                case FieldType.Fixed64:
                    return ComputeFixed64Size(fieldNumber, (ulong) value);
                case FieldType.Fixed32:
                    return ComputeFixed32Size(fieldNumber, (uint) value);
                case FieldType.Bool:
                    return ComputeBoolSize(fieldNumber, (bool) value);
                case FieldType.String:
                    return ComputeStringSize(fieldNumber, (string) value);
                case FieldType.Group:
                    return ComputeGroupSize(fieldNumber, (IMessageLite) value);
                case FieldType.Message:
                    return ComputeMessageSize(fieldNumber, (IMessageLite) value);
                case FieldType.Bytes:
                    return ComputeBytesSize(fieldNumber, (ByteString) value);
                case FieldType.UInt32:
                    return ComputeUInt32Size(fieldNumber, (uint) value);
                case FieldType.SFixed32:
                    return ComputeSFixed32Size(fieldNumber, (int) value);
                case FieldType.SFixed64:
                    return ComputeSFixed64Size(fieldNumber, (long) value);
                case FieldType.SInt32:
                    return ComputeSInt32Size(fieldNumber, (int) value);
                case FieldType.SInt64:
                    return ComputeSInt64Size(fieldNumber, (long) value);
                case FieldType.Enum:
                    if (value is Enum)
                    {
                        return ComputeEnumSize(fieldNumber, ((IConvertible) value).ToInt32(CultureInfo.InvariantCulture));
                    }
                    else
                    {
                        return ComputeEnumSize(fieldNumber, ((IEnumLite) value).Number);
                    }
                default:
                    throw new ArgumentOutOfRangeException("Invalid field type " + fieldType);
            }
        }

        /// <summary>
        /// Compute the number of bytes that would be needed to encode a
        /// field of arbitrary type, excluding the tag, to the stream.
        /// </summary>
        public static int ComputeFieldSizeNoTag(FieldType fieldType, Object value)
        {
            switch (fieldType)
            {
                case FieldType.Double:
                    return ComputeDoubleSizeNoTag((double) value);
                case FieldType.Float:
                    return ComputeFloatSizeNoTag((float) value);
                case FieldType.Int64:
                    return ComputeInt64SizeNoTag((long) value);
                case FieldType.UInt64:
                    return ComputeUInt64SizeNoTag((ulong) value);
                case FieldType.Int32:
                    return ComputeInt32SizeNoTag((int) value);
                case FieldType.Fixed64:
                    return ComputeFixed64SizeNoTag((ulong) value);
                case FieldType.Fixed32:
                    return ComputeFixed32SizeNoTag((uint) value);
                case FieldType.Bool:
                    return ComputeBoolSizeNoTag((bool) value);
                case FieldType.String:
                    return ComputeStringSizeNoTag((string) value);
                case FieldType.Group:
                    return ComputeGroupSizeNoTag((IMessageLite) value);
                case FieldType.Message:
                    return ComputeMessageSizeNoTag((IMessageLite) value);
                case FieldType.Bytes:
                    return ComputeBytesSizeNoTag((ByteString) value);
                case FieldType.UInt32:
                    return ComputeUInt32SizeNoTag((uint) value);
                case FieldType.SFixed32:
                    return ComputeSFixed32SizeNoTag((int) value);
                case FieldType.SFixed64:
                    return ComputeSFixed64SizeNoTag((long) value);
                case FieldType.SInt32:
                    return ComputeSInt32SizeNoTag((int) value);
                case FieldType.SInt64:
                    return ComputeSInt64SizeNoTag((long) value);
                case FieldType.Enum:
                    if (value is Enum)
                    {
                        return ComputeEnumSizeNoTag(((IConvertible) value).ToInt32(CultureInfo.InvariantCulture));
                    }
                    else
                    {
                        return ComputeEnumSizeNoTag(((IEnumLite) value).Number);
                    }
                default:
                    throw new ArgumentOutOfRangeException("Invalid field type " + fieldType);
            }
        }

        /// <summary>
        /// Compute the number of bytes that would be needed to encode a tag.
        /// </summary>
        public static int ComputeTagSize(int fieldNumber)
        {
            return ComputeRawVarint32Size(WireFormat.MakeTag(fieldNumber, 0));
        }
    }
}