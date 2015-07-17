#region Copyright notice and license
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
#endregion

using System;

namespace Google.Protobuf
{
    // This part of CodedOutputStream provides all the static entry points that are used
    // by generated code and internally to compute the size of messages prior to being
    // written to an instance of CodedOutputStream.
    public sealed partial class CodedOutputStream
    {
        private const int LittleEndian64Size = 8;
        private const int LittleEndian32Size = 4;        

        /// <summary>
        /// Computes the number of bytes that would be needed to encode a
        /// double field, including the tag.
        /// </summary>
        public static int ComputeDoubleSize(double value)
        {
            return LittleEndian64Size;
        }

        /// <summary>
        /// Computes the number of bytes that would be needed to encode a
        /// float field, including the tag.
        /// </summary>
        public static int ComputeFloatSize(float value)
        {
            return LittleEndian32Size;
        }

        /// <summary>
        /// Computes the number of bytes that would be needed to encode a
        /// uint64 field, including the tag.
        /// </summary>
        public static int ComputeUInt64Size(ulong value)
        {
            return ComputeRawVarint64Size(value);
        }

        /// <summary>
        /// Computes the number of bytes that would be needed to encode an
        /// int64 field, including the tag.
        /// </summary>
        public static int ComputeInt64Size(long value)
        {
            return ComputeRawVarint64Size((ulong) value);
        }

        /// <summary>
        /// Computes the number of bytes that would be needed to encode an
        /// int32 field, including the tag.
        /// </summary>
        public static int ComputeInt32Size(int value)
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
        /// Computes the number of bytes that would be needed to encode a
        /// fixed64 field, including the tag.
        /// </summary>
        public static int ComputeFixed64Size(ulong value)
        {
            return LittleEndian64Size;
        }

        /// <summary>
        /// Computes the number of bytes that would be needed to encode a
        /// fixed32 field, including the tag.
        /// </summary>
        public static int ComputeFixed32Size(uint value)
        {
            return LittleEndian32Size;
        }

        /// <summary>
        /// Computes the number of bytes that would be needed to encode a
        /// bool field, including the tag.
        /// </summary>
        public static int ComputeBoolSize(bool value)
        {
            return 1;
        }

        /// <summary>
        /// Computes the number of bytes that would be needed to encode a
        /// string field, including the tag.
        /// </summary>
        public static int ComputeStringSize(String value)
        {
            int byteArraySize = Utf8Encoding.GetByteCount(value);
            return ComputeLengthSize(byteArraySize) + byteArraySize;
        }

        /// <summary>
        /// Computes the number of bytes that would be needed to encode a
        /// group field, including the tag.
        /// </summary>
        public static int ComputeGroupSize(IMessage value)
        {
            return value.CalculateSize();
        }

        /// <summary>
        /// Computes the number of bytes that would be needed to encode an
        /// embedded message field, including the tag.
        /// </summary>
        public static int ComputeMessageSize(IMessage value)
        {
            int size = value.CalculateSize();
            return ComputeLengthSize(size) + size;
        }

        /// <summary>
        /// Computes the number of bytes that would be needed to encode a
        /// bytes field, including the tag.
        /// </summary>
        public static int ComputeBytesSize(ByteString value)
        {
            return ComputeLengthSize(value.Length) + value.Length;
        }

        /// <summary>
        /// Computes the number of bytes that would be needed to encode a
        /// uint32 field, including the tag.
        /// </summary>
        public static int ComputeUInt32Size(uint value)
        {
            return ComputeRawVarint32Size(value);
        }

        /// <summary>
        /// Computes the number of bytes that would be needed to encode a
        /// enum field, including the tag. The caller is responsible for
        /// converting the enum value to its numeric value.
        /// </summary>
        public static int ComputeEnumSize(int value)
        {
            // Currently just a pass-through, but it's nice to separate it logically.
            return ComputeInt32Size(value);
        }

        /// <summary>
        /// Computes the number of bytes that would be needed to encode an
        /// sfixed32 field, including the tag.
        /// </summary>
        public static int ComputeSFixed32Size(int value)
        {
            return LittleEndian32Size;
        }

        /// <summary>
        /// Computes the number of bytes that would be needed to encode an
        /// sfixed64 field, including the tag.
        /// </summary>
        public static int ComputeSFixed64Size(long value)
        {
            return LittleEndian64Size;
        }

        /// <summary>
        /// Computes the number of bytes that would be needed to encode an
        /// sint32 field, including the tag.
        /// </summary>
        public static int ComputeSInt32Size(int value)
        {
            return ComputeRawVarint32Size(EncodeZigZag32(value));
        }

        /// <summary>
        /// Computes the number of bytes that would be needed to encode an
        /// sint64 field, including the tag.
        /// </summary>
        public static int ComputeSInt64Size(long value)
        {
            return ComputeRawVarint64Size(EncodeZigZag64(value));
        }

        /// <summary>
        /// Computes the number of bytes that would be needed to encode a length,
        /// as written by <see cref="WriteLength"/>.
        /// </summary>
        public static int ComputeLengthSize(int length)
        {
            return ComputeRawVarint32Size((uint) length);
        }

        /// <summary>
        /// Computes the number of bytes that would be needed to encode a varint.
        /// </summary>
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
        /// Computes the number of bytes that would be needed to encode a varint.
        /// </summary>
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
        /// Computes the number of bytes that would be needed to encode a tag.
        /// </summary>
        public static int ComputeTagSize(int fieldNumber)
        {
            return ComputeRawVarint32Size(WireFormat.MakeTag(fieldNumber, 0));
        }
    }
}