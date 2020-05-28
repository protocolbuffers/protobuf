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
using System.Runtime.CompilerServices;
using System.Security;
using System.Text;

namespace Google.Protobuf
{
    /// <summary>
    /// Primitives for encoding protobuf wire format.
    /// </summary>
    [SecuritySafeCritical]
    internal static class WritingPrimitives
    {
        // "Local" copy of Encoding.UTF8, for efficiency. (Yes, it makes a difference.)
        internal static readonly Encoding Utf8Encoding = Encoding.UTF8;
     
        // TODO: computing size....

        #region Writing of values (not including tags)

        /// <summary>
        /// Writes a double field value, without a tag, to the stream.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void WriteDouble(ref Span<byte> buffer, ref WriterInternalState state, double value)
        {
            WriteRawLittleEndian64(ref buffer, ref state, (ulong)BitConverter.DoubleToInt64Bits(value));
        }

        /// <summary>
        /// Writes a float field value, without a tag, to the stream.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void WriteFloat(ref Span<byte> buffer, ref WriterInternalState state, float value)
        {
            byte[] rawBytes = BitConverter.GetBytes(value);
            if (!BitConverter.IsLittleEndian)
            {
                ByteArray.Reverse(rawBytes);
            }

            if (state.limit - state.position >= 4)
            {
                buffer[state.position++] = rawBytes[0];
                buffer[state.position++] = rawBytes[1];
                buffer[state.position++] = rawBytes[2];
                buffer[state.position++] = rawBytes[3];
            }
            else
            {
                WriteRawBytes(ref buffer, ref state, rawBytes, 0, 4);
            }
        }

        /// <summary>
        /// Writes a uint64 field value, without a tag, to the stream.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void WriteUInt64(ref Span<byte> buffer, ref WriterInternalState state, ulong value)
        {
            WriteRawVarint64(ref buffer, ref state, value);
        }

        /// <summary>
        /// Writes an int64 field value, without a tag, to the stream.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void WriteInt64(ref Span<byte> buffer, ref WriterInternalState state, long value)
        {
            WriteRawVarint64(ref buffer, ref state, (ulong)value);
        }

        /// <summary>
        /// Writes an int32 field value, without a tag, to the stream.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void WriteInt32(ref Span<byte> buffer, ref WriterInternalState state, int value)
        {
            if (value >= 0)
            {
                WriteRawVarint32(ref buffer, ref state, (uint)value);
            }
            else
            {
                // Must sign-extend.
                WriteRawVarint64(ref buffer, ref state, (ulong)value);
            }
        }

        /// <summary>
        /// Writes a fixed64 field value, without a tag, to the stream.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void WriteFixed64(ref Span<byte> buffer, ref WriterInternalState state, ulong value)
        {
            WriteRawLittleEndian64(ref buffer, ref state, value);
        }

        /// <summary>
        /// Writes a fixed32 field value, without a tag, to the stream.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void WriteFixed32(ref Span<byte> buffer, ref WriterInternalState state, uint value)
        {
            WriteRawLittleEndian32(ref buffer, ref state, value);
        }

        /// <summary>
        /// Writes a bool field value, without a tag, to the stream.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void WriteBool(ref Span<byte> buffer, ref WriterInternalState state, bool value)
        {
            WriteRawByte(ref buffer, ref state, value ? (byte)1 : (byte)0);
        }

        /// <summary>
        /// Writes a string field value, without a tag, to the stream.
        /// The data is length-prefixed.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void WriteString(ref Span<byte> buffer, ref WriterInternalState state, string value)
        {
            // Optimise the case where we have enough space to write
            // the string directly to the buffer, which should be common.
            int length = Utf8Encoding.GetByteCount(value);
            WriteLength(ref buffer, ref state, length);
            if (state.limit - state.position >= length)
            {
                if (length == value.Length) // Must be all ASCII...
                {
                    for (int i = 0; i < length; i++)
                    {
                        buffer[state.position + i] = (byte)value[i];
                    }
                    state.position += length;
                }
                else
                {
                    // TODO: optimize this part!!!!
                    byte[] bytes = Utf8Encoding.GetBytes(value);
                    WriteRawBytes(ref buffer, ref state, bytes);
                    // TODO: we need to write to a span...
                    //Utf8Encoding.GetBytes(value, 0, value.Length, buffer, state.position);
                    //state.position += length;
                }
            }
            else
            {
                // TODO: do this more efficiently
                byte[] bytes = Utf8Encoding.GetBytes(value);
                WriteRawBytes(ref buffer, ref state, bytes);
            }
        }

        /// <summary>
        /// Write a byte string, without a tag, to the stream.
        /// The data is length-prefixed.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void WriteBytes(ref Span<byte> buffer, ref WriterInternalState state, ByteString value)
        {
            WriteLength(ref buffer, ref state, value.Length);
            WriteRawBytes(ref buffer, ref state, value.Span);
        }

        /// <summary>
        /// Writes a uint32 value, without a tag, to the stream.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void WriteUInt32(ref Span<byte> buffer, ref WriterInternalState state, uint value)
        {
            WriteRawVarint32(ref buffer, ref state, value);
        }

        /// <summary>
        /// Writes an enum value, without a tag, to the stream.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void WriteEnum(ref Span<byte> buffer, ref WriterInternalState state, int value)
        {
            WriteInt32(ref buffer, ref state, value);
        }

        /// <summary>
        /// Writes an sfixed32 value, without a tag, to the stream.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void WriteSFixed32(ref Span<byte> buffer, ref WriterInternalState state, int value)
        {
            WriteRawLittleEndian32(ref buffer, ref state, (uint)value);
        }

        /// <summary>
        /// Writes an sfixed64 value, without a tag, to the stream.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void WriteSFixed64(ref Span<byte> buffer, ref WriterInternalState state, long value)
        {
            WriteRawLittleEndian64(ref buffer, ref state, (ulong)value);
        }

        /// <summary>
        /// Writes an sint32 value, without a tag, to the stream.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void WriteSInt32(ref Span<byte> buffer, ref WriterInternalState state, int value)
        {
            WriteRawVarint32(ref buffer, ref state, EncodeZigZag32(value));
        }

        /// <summary>
        /// Writes an sint64 value, without a tag, to the stream.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void WriteSInt64(ref Span<byte> buffer, ref WriterInternalState state, long value)
        {
            WriteRawVarint64(ref buffer, ref state, EncodeZigZag64(value));
        }

        /// <summary>
        /// Writes a length (in bytes) for length-delimited data.
        /// </summary>
        /// <remarks>
        /// This method simply writes a rawint, but exists for clarity in calling code.
        /// </remarks>

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void WriteLength(ref Span<byte> buffer, ref WriterInternalState state, int length)
        {
            WriteRawVarint32(ref buffer, ref state, (uint)length);
        }

        #endregion

        #region Writing primitives
        /// <summary>
        /// Writes a 32 bit value as a varint. The fast route is taken when
        /// there's enough buffer space left to whizz through without checking
        /// for each byte; otherwise, we resort to calling WriteRawByte each time.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void WriteRawVarint32(ref Span<byte> buffer, ref WriterInternalState state, uint value)
        {
            // Optimize for the common case of a single byte value
            if (value < 128 && state.position < state.limit)
            {
                buffer[state.position++] = (byte)value;
                return;
            }

            while (value > 127 && state.position < state.limit)
            {
                buffer[state.position++] = (byte)((value & 0x7F) | 0x80);
                value >>= 7;
            }
            while (value > 127)
            {
                WriteRawByte(ref buffer, ref state, (byte)((value & 0x7F) | 0x80));
                value >>= 7;
            }
            if (state.position < state.limit)
            {
                buffer[state.position++] = (byte)value;
            }
            else
            {
                WriteRawByte(ref buffer, ref state, (byte)value);
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void WriteRawVarint64(ref Span<byte> buffer, ref WriterInternalState state, ulong value)
        {
            while (value > 127 && state.position < state.limit)
            {
                buffer[state.position++] = (byte)((value & 0x7F) | 0x80);
                value >>= 7;
            }
            while (value > 127)
            {
                WriteRawByte(ref buffer, ref state, (byte)((value & 0x7F) | 0x80));
                value >>= 7;
            }
            if (state.position < state.limit)
            {
                buffer[state.position++] = (byte)value;
            }
            else
            {
                WriteRawByte(ref buffer, ref state, (byte)value);
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void WriteRawLittleEndian32(ref Span<byte> buffer, ref WriterInternalState state, uint value)
        {
            if (state.position + 4 > state.limit)
            {
                WriteRawByte(ref buffer, ref state, (byte)value);
                WriteRawByte(ref buffer, ref state, (byte)(value >> 8));
                WriteRawByte(ref buffer, ref state, (byte)(value >> 16));
                WriteRawByte(ref buffer, ref state, (byte)(value >> 24));
            }
            else
            {
                buffer[state.position++] = ((byte)value);
                buffer[state.position++] = ((byte)(value >> 8));
                buffer[state.position++] = ((byte)(value >> 16));
                buffer[state.position++] = ((byte)(value >> 24));
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void WriteRawLittleEndian64(ref Span<byte> buffer, ref WriterInternalState state, ulong value)
        {
            if (state.position + 8 > state.limit)
            {
                WriteRawByte(ref buffer, ref state, (byte)value);
                WriteRawByte(ref buffer, ref state, (byte)(value >> 8));
                WriteRawByte(ref buffer, ref state, (byte)(value >> 16));
                WriteRawByte(ref buffer, ref state, (byte)(value >> 24));
                WriteRawByte(ref buffer, ref state, (byte)(value >> 32));
                WriteRawByte(ref buffer, ref state, (byte)(value >> 40));
                WriteRawByte(ref buffer, ref state, (byte)(value >> 48));
                WriteRawByte(ref buffer, ref state, (byte)(value >> 56));
            }
            else
            {
                buffer[state.position++] = ((byte)value);
                buffer[state.position++] = ((byte)(value >> 8));
                buffer[state.position++] = ((byte)(value >> 16));
                buffer[state.position++] = ((byte)(value >> 24));
                buffer[state.position++] = ((byte)(value >> 32));
                buffer[state.position++] = ((byte)(value >> 40));
                buffer[state.position++] = ((byte)(value >> 48));
                buffer[state.position++] = ((byte)(value >> 56));
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void WriteRawByte(ref Span<byte> buffer, ref WriterInternalState state, byte value)
        {
            if (state.position == state.limit)
            {
                state.writeBufferHelper.RefreshBuffer(ref buffer, ref state);
            }

            buffer[state.position++] = value;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void WriteRawByte(ref Span<byte> buffer, ref WriterInternalState state, uint value)
        {
            WriteRawByte(ref buffer, ref state, (byte)value);
        }

        /// <summary>
        /// Writes out an array of bytes.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void WriteRawBytes(ref Span<byte> buffer, ref WriterInternalState state, byte[] value)
        {
            WriteRawBytes(ref buffer, ref state, new ReadOnlySpan<byte>(value));
        }

        /// <summary>
        /// Writes out part of an array of bytes.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void WriteRawBytes(ref Span<byte> buffer, ref WriterInternalState state, byte[] value, int offset, int length)
        {
            WriteRawBytes(ref buffer, ref state, new ReadOnlySpan<byte>(value, offset, length));
        }

        /// <summary>
        /// Writes out part of an array of bytes.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void WriteRawBytes(ref Span<byte> buffer, ref WriterInternalState state, ReadOnlySpan<byte> value)
        {
            if (state.limit - state.position >= value.Length)
            {
                // We have room in the current buffer.    
                value.CopyTo(buffer.Slice(state.position, value.Length));
                state.position += value.Length;
            }
            else
            {
                // TODO: save copies when using coded output stream and there's a lot of data to write...

                int bytesWritten = 0;
                while (state.limit - state.position < value.Length - bytesWritten)
                {
                    int length = state.limit - state.position;
                    value.Slice(bytesWritten, length).CopyTo(buffer.Slice(state.position, length));
                    bytesWritten += length;
                    state.position += length;
                    state.writeBufferHelper.RefreshBuffer(ref buffer, ref state);
                }

                // copy the remaining data
                int remainderLength = value.Length - bytesWritten;
                value.Slice(bytesWritten, remainderLength).CopyTo(buffer.Slice(state.position, remainderLength));
                state.position += remainderLength;
            }
        }
        #endregion

        #region Raw tag writing
        /// <summary>
        /// Encodes and writes a tag.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void WriteTag(ref Span<byte> buffer, ref WriterInternalState state, int fieldNumber, WireFormat.WireType type)
        {
            WriteRawVarint32(ref buffer, ref state, WireFormat.MakeTag(fieldNumber, type));
        }

        /// <summary>
        /// Writes an already-encoded tag.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void WriteTag(ref Span<byte> buffer, ref WriterInternalState state, uint tag)
        {
            WriteRawVarint32(ref buffer, ref state, tag);
        }

        /// <summary>
        /// Writes the given single-byte tag directly to the stream.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void WriteRawTag(ref Span<byte> buffer, ref WriterInternalState state, byte b1)
        {
            WriteRawByte(ref buffer, ref state, b1);
        }

        /// <summary>
        /// Writes the given two-byte tag directly to the stream.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void WriteRawTag(ref Span<byte> buffer, ref WriterInternalState state, byte b1, byte b2)
        {
            WriteRawByte(ref buffer, ref state, b1);
            WriteRawByte(ref buffer, ref state, b2);
        }

        /// <summary>
        /// Writes the given three-byte tag directly to the stream.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void WriteRawTag(ref Span<byte> buffer, ref WriterInternalState state, byte b1, byte b2, byte b3)
        {
            WriteRawByte(ref buffer, ref state, b1);
            WriteRawByte(ref buffer, ref state, b2);
            WriteRawByte(ref buffer, ref state, b3);
        }

        /// <summary>
        /// Writes the given four-byte tag directly to the stream.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void WriteRawTag(ref Span<byte> buffer, ref WriterInternalState state, byte b1, byte b2, byte b3, byte b4)
        {
            WriteRawByte(ref buffer, ref state, b1);
            WriteRawByte(ref buffer, ref state, b2);
            WriteRawByte(ref buffer, ref state, b3);
            WriteRawByte(ref buffer, ref state, b4);
        }

        /// <summary>
        /// Writes the given five-byte tag directly to the stream.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void WriteRawTag(ref Span<byte> buffer, ref WriterInternalState state, byte b1, byte b2, byte b3, byte b4, byte b5)
        {
            WriteRawByte(ref buffer, ref state, b1);
            WriteRawByte(ref buffer, ref state, b2);
            WriteRawByte(ref buffer, ref state, b3);
            WriteRawByte(ref buffer, ref state, b4);
            WriteRawByte(ref buffer, ref state, b5);
        }
        #endregion

        /// <summary>
        /// Encode a 32-bit value with ZigZag encoding.
        /// </summary>
        /// <remarks>
        /// ZigZag encodes signed integers into values that can be efficiently
        /// encoded with varint.  (Otherwise, negative values must be 
        /// sign-extended to 64 bits to be varint encoded, thus always taking
        /// 10 bytes on the wire.)
        /// </remarks>
        public static uint EncodeZigZag32(int n)
        {
            // Note:  the right-shift must be arithmetic
            return (uint)((n << 1) ^ (n >> 31));
        }

        /// <summary>
        /// Encode a 64-bit value with ZigZag encoding.
        /// </summary>
        /// <remarks>
        /// ZigZag encodes signed integers into values that can be efficiently
        /// encoded with varint.  (Otherwise, negative values must be 
        /// sign-extended to 64 bits to be varint encoded, thus always taking
        /// 10 bytes on the wire.)
        /// </remarks>
        public static ulong EncodeZigZag64(long n)
        {
            return (ulong)((n << 1) ^ (n >> 63));
        }
    }
}