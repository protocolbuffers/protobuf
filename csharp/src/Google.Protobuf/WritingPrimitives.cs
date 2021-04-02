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
using System.Buffers.Binary;
using System.Diagnostics;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
#if GOOGLE_PROTOBUF_SIMD
using System.Runtime.Intrinsics;
using System.Runtime.Intrinsics.Arm;
using System.Runtime.Intrinsics.X86;
#endif
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
#if NET5_0
        internal static Encoding Utf8Encoding => Encoding.UTF8; // allows JIT to devirtualize
#else
        internal static readonly Encoding Utf8Encoding = Encoding.UTF8; // "Local" copy of Encoding.UTF8, for efficiency. (Yes, it makes a difference.)
#endif

        #region Writing of values (not including tags)

        /// <summary>
        /// Writes a double field value, without a tag, to the stream.
        /// </summary>
        public static void WriteDouble(ref Span<byte> buffer, ref WriterInternalState state, double value)
        {
            WriteRawLittleEndian64(ref buffer, ref state, (ulong)BitConverter.DoubleToInt64Bits(value));
        }

        /// <summary>
        /// Writes a float field value, without a tag, to the stream.
        /// </summary>
        public static unsafe void WriteFloat(ref Span<byte> buffer, ref WriterInternalState state, float value)
        {
            const int length = sizeof(float);
            if (buffer.Length - state.position >= length)
            {
                // if there's enough space in the buffer, write the float directly into the buffer
                var floatSpan = buffer.Slice(state.position, length);
                Unsafe.WriteUnaligned(ref MemoryMarshal.GetReference(floatSpan), value);

                if (!BitConverter.IsLittleEndian)
                {
                    floatSpan.Reverse();
                }
                state.position += length;
            }
            else
            {
                WriteFloatSlowPath(ref buffer, ref state, value);
            }
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        private static unsafe void WriteFloatSlowPath(ref Span<byte> buffer, ref WriterInternalState state, float value)
        {
            const int length = sizeof(float);

            // TODO(jtattermusch): deduplicate the code. Populating the span is the same as for the fastpath.
            Span<byte> floatSpan = stackalloc byte[length];
            Unsafe.WriteUnaligned(ref MemoryMarshal.GetReference(floatSpan), value);
            if (!BitConverter.IsLittleEndian)
            {
                floatSpan.Reverse();
            }

            WriteRawByte(ref buffer, ref state, floatSpan[0]);
            WriteRawByte(ref buffer, ref state, floatSpan[1]);
            WriteRawByte(ref buffer, ref state, floatSpan[2]);
            WriteRawByte(ref buffer, ref state, floatSpan[3]);
        }

        /// <summary>
        /// Writes a uint64 field value, without a tag, to the stream.
        /// </summary>
        public static void WriteUInt64(ref Span<byte> buffer, ref WriterInternalState state, ulong value)
        {
            WriteRawVarint64(ref buffer, ref state, value);
        }

        /// <summary>
        /// Writes an int64 field value, without a tag, to the stream.
        /// </summary>
        public static void WriteInt64(ref Span<byte> buffer, ref WriterInternalState state, long value)
        {
            WriteRawVarint64(ref buffer, ref state, (ulong)value);
        }

        /// <summary>
        /// Writes an int32 field value, without a tag, to the stream.
        /// </summary>
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
        public static void WriteFixed64(ref Span<byte> buffer, ref WriterInternalState state, ulong value)
        {
            WriteRawLittleEndian64(ref buffer, ref state, value);
        }

        /// <summary>
        /// Writes a fixed32 field value, without a tag, to the stream.
        /// </summary>
        public static void WriteFixed32(ref Span<byte> buffer, ref WriterInternalState state, uint value)
        {
            WriteRawLittleEndian32(ref buffer, ref state, value);
        }

        /// <summary>
        /// Writes a bool field value, without a tag, to the stream.
        /// </summary>
        public static void WriteBool(ref Span<byte> buffer, ref WriterInternalState state, bool value)
        {
            WriteRawByte(ref buffer, ref state, value ? (byte)1 : (byte)0);
        }

        /// <summary>
        /// Writes a string field value, without a tag, to the stream.
        /// The data is length-prefixed.
        /// </summary>
        public static void WriteString(ref Span<byte> buffer, ref WriterInternalState state, string value)
        {
            const int MaxBytesPerChar = 3;
            const int MaxSmallStringLength = 128 / MaxBytesPerChar;

            // The string is small enough that the length will always be a 1 byte varint.
            // Also there is enough space to write length + bytes to buffer.
            // Write string directly to the buffer, and then write length.
            // This saves calling GetByteCount on the string. We get the string length from GetBytes.
            if (value.Length <= MaxSmallStringLength && buffer.Length - state.position - 1 >= value.Length * MaxBytesPerChar)
            {
                int indexOfLengthDelimiter = state.position++;
                buffer[indexOfLengthDelimiter] = (byte)WriteStringToBuffer(buffer, ref state, value);
                return;
            }

            int length = Utf8Encoding.GetByteCount(value);
            WriteLength(ref buffer, ref state, length);

            // Optimise the case where we have enough space to write
            // the string directly to the buffer, which should be common.
            if (buffer.Length - state.position >= length)
            {
                if (length == value.Length) // Must be all ASCII...
                {
                    WriteAsciiStringToBuffer(buffer, ref state, value, length);
                }
                else
                {
                    WriteStringToBuffer(buffer, ref state, value);
                }
            }
            else
            {
                // Opportunity for future optimization:
                // Large strings that don't fit into the current buffer segment
                // can probably be optimized by using Utf8Encoding.GetEncoder()
                // but more benchmarks would need to be added as evidence.
                byte[] bytes = Utf8Encoding.GetBytes(value);
                WriteRawBytes(ref buffer, ref state, bytes);
            }
        }

        // Calling this method with non-ASCII content will break.
        // Content must be verified to be all ASCII before using this method.
        private static void WriteAsciiStringToBuffer(Span<byte> buffer, ref WriterInternalState state, string value, int length)
        {
            ref char sourceChars = ref MemoryMarshal.GetReference(value.AsSpan());
            ref byte destinationBytes = ref MemoryMarshal.GetReference(buffer.Slice(state.position));

            int currentIndex = 0;
            // If 64bit, process 4 chars at a time.
            // The logic inside this check will be elided by JIT in 32bit programs.
            if (IntPtr.Size == 8)
            {
                // Need at least 4 chars available to use this optimization. 
                if (length >= 4)
                {
                    ref byte sourceBytes = ref Unsafe.As<char, byte>(ref sourceChars);

                    // Process 4 chars at a time until there are less than 4 remaining.
                    // We already know all characters are ASCII so there is no need to validate the source.
                    int lastIndexWhereCanReadFourChars = value.Length - 4;
                    do
                    {
                        NarrowFourUtf16CharsToAsciiAndWriteToBuffer(
                            ref Unsafe.AddByteOffset(ref destinationBytes, (IntPtr)currentIndex),
                            Unsafe.ReadUnaligned<ulong>(ref Unsafe.AddByteOffset(ref sourceBytes, (IntPtr)(currentIndex * 2))));

                    } while ((currentIndex += 4) <= lastIndexWhereCanReadFourChars);
                }
            }

            // Process any remaining, 1 char at a time.
            // Avoid bounds checking with ref + Unsafe
            for (; currentIndex < length; currentIndex++)
            {
                Unsafe.AddByteOffset(ref destinationBytes, (IntPtr)currentIndex) = (byte)Unsafe.AddByteOffset(ref sourceChars, (IntPtr)(currentIndex * 2));
            }

            state.position += length;
        }

        // Copied with permission from https://github.com/dotnet/runtime/blob/1cdafd27e4afd2c916af5df949c13f8b373c4335/src/libraries/System.Private.CoreLib/src/System/Text/ASCIIUtility.cs#L1119-L1171
        //
        /// <summary>
        /// Given a QWORD which represents a buffer of 4 ASCII chars in machine-endian order,
        /// narrows each WORD to a BYTE, then writes the 4-byte result to the output buffer
        /// also in machine-endian order.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private static void NarrowFourUtf16CharsToAsciiAndWriteToBuffer(ref byte outputBuffer, ulong value)
        {
#if GOOGLE_PROTOBUF_SIMD
            if (Sse2.X64.IsSupported)
            {
                // Narrows a vector of words [ w0 w1 w2 w3 ] to a vector of bytes
                // [ b0 b1 b2 b3 b0 b1 b2 b3 ], then writes 4 bytes (32 bits) to the destination.

                Vector128<short> vecWide = Sse2.X64.ConvertScalarToVector128UInt64(value).AsInt16();
                Vector128<uint> vecNarrow = Sse2.PackUnsignedSaturate(vecWide, vecWide).AsUInt32();
                Unsafe.WriteUnaligned<uint>(ref outputBuffer, Sse2.ConvertToUInt32(vecNarrow));
            }
            else if (AdvSimd.IsSupported)
            {
                // Narrows a vector of words [ w0 w1 w2 w3 ] to a vector of bytes
                // [ b0 b1 b2 b3 * * * * ], then writes 4 bytes (32 bits) to the destination.

                Vector128<short> vecWide = Vector128.CreateScalarUnsafe(value).AsInt16();
                Vector64<byte> lower = AdvSimd.ExtractNarrowingSaturateUnsignedLower(vecWide);
                Unsafe.WriteUnaligned<uint>(ref outputBuffer, lower.AsUInt32().ToScalar());
            }
            else
#endif
            {
                // Fallback to non-SIMD approach when SIMD is not available.
                // This could happen either because the APIs are not available, or hardware doesn't support it.
                // Processing 4 chars at a time in this fallback is still faster than casting one char at a time.
                if (BitConverter.IsLittleEndian)
                {
                    outputBuffer = (byte)value;
                    value >>= 16;
                    Unsafe.Add(ref outputBuffer, 1) = (byte)value;
                    value >>= 16;
                    Unsafe.Add(ref outputBuffer, 2) = (byte)value;
                    value >>= 16;
                    Unsafe.Add(ref outputBuffer, 3) = (byte)value;
                }
                else
                {
                    Unsafe.Add(ref outputBuffer, 3) = (byte)value;
                    value >>= 16;
                    Unsafe.Add(ref outputBuffer, 2) = (byte)value;
                    value >>= 16;
                    Unsafe.Add(ref outputBuffer, 1) = (byte)value;
                    value >>= 16;
                    outputBuffer = (byte)value;
                }
            }
        }

        private static int WriteStringToBuffer(Span<byte> buffer, ref WriterInternalState state, string value)
        {
#if NETSTANDARD1_1
            // slowpath when Encoding.GetBytes(Char*, Int32, Byte*, Int32) is not available
            byte[] bytes = Utf8Encoding.GetBytes(value);
            WriteRawBytes(ref buffer, ref state, bytes);
            return bytes.Length;
#else
            ReadOnlySpan<char> source = value.AsSpan();
            int bytesUsed;
            unsafe
            {
                fixed (char* sourceChars = &MemoryMarshal.GetReference(source))
                fixed (byte* destinationBytes = &MemoryMarshal.GetReference(buffer))
                {
                    bytesUsed = Utf8Encoding.GetBytes(
                        sourceChars,
                        source.Length,
                        destinationBytes + state.position,
                        buffer.Length - state.position);
                }
            }
            state.position += bytesUsed;
            return bytesUsed;
#endif
        }

        /// <summary>
        /// Write a byte string, without a tag, to the stream.
        /// The data is length-prefixed.
        /// </summary>
        public static void WriteBytes(ref Span<byte> buffer, ref WriterInternalState state, ByteString value)
        {
            WriteLength(ref buffer, ref state, value.Length);
            WriteRawBytes(ref buffer, ref state, value.Span);
        }

        /// <summary>
        /// Writes a uint32 value, without a tag, to the stream.
        /// </summary>
        public static void WriteUInt32(ref Span<byte> buffer, ref WriterInternalState state, uint value)
        {
            WriteRawVarint32(ref buffer, ref state, value);
        }

        /// <summary>
        /// Writes an enum value, without a tag, to the stream.
        /// </summary>
        public static void WriteEnum(ref Span<byte> buffer, ref WriterInternalState state, int value)
        {
            WriteInt32(ref buffer, ref state, value);
        }

        /// <summary>
        /// Writes an sfixed32 value, without a tag, to the stream.
        /// </summary>
        public static void WriteSFixed32(ref Span<byte> buffer, ref WriterInternalState state, int value)
        {
            WriteRawLittleEndian32(ref buffer, ref state, (uint)value);
        }

        /// <summary>
        /// Writes an sfixed64 value, without a tag, to the stream.
        /// </summary>
        public static void WriteSFixed64(ref Span<byte> buffer, ref WriterInternalState state, long value)
        {
            WriteRawLittleEndian64(ref buffer, ref state, (ulong)value);
        }

        /// <summary>
        /// Writes an sint32 value, without a tag, to the stream.
        /// </summary>
        public static void WriteSInt32(ref Span<byte> buffer, ref WriterInternalState state, int value)
        {
            WriteRawVarint32(ref buffer, ref state, EncodeZigZag32(value));
        }

        /// <summary>
        /// Writes an sint64 value, without a tag, to the stream.
        /// </summary>
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
        public static void WriteRawVarint32(ref Span<byte> buffer, ref WriterInternalState state, uint value)
        {
            // Optimize for the common case of a single byte value
            if (value < 128 && state.position < buffer.Length)
            {
                buffer[state.position++] = (byte)value;
                return;
            }

            // Fast path when capacity is available
            while (state.position < buffer.Length)
            {
                if (value > 127)
                {
                    buffer[state.position++] = (byte)((value & 0x7F) | 0x80);
                    value >>= 7;
                }
                else
                {
                    buffer[state.position++] = (byte)value;
                    return;
                }
            }

            while (value > 127)
            {
                WriteRawByte(ref buffer, ref state, (byte)((value & 0x7F) | 0x80));
                value >>= 7;
            }

            WriteRawByte(ref buffer, ref state, (byte)value);
        }

        public static void WriteRawVarint64(ref Span<byte> buffer, ref WriterInternalState state, ulong value)
        {
            // Optimize for the common case of a single byte value
            if (value < 128 && state.position < buffer.Length)
            {
                buffer[state.position++] = (byte)value;
                return;
            }

            // Fast path when capacity is available
            while (state.position < buffer.Length)
            {
                if (value > 127)
                {
                    buffer[state.position++] = (byte)((value & 0x7F) | 0x80);
                    value >>= 7;
                }
                else
                {
                    buffer[state.position++] = (byte)value;
                    return;
                }
            }

            while (value > 127)
            {
                WriteRawByte(ref buffer, ref state, (byte)((value & 0x7F) | 0x80));
                value >>= 7;
            }

            WriteRawByte(ref buffer, ref state, (byte)value);
        }

        public static void WriteRawLittleEndian32(ref Span<byte> buffer, ref WriterInternalState state, uint value)
        {
            const int length = sizeof(uint);
            if (state.position + length > buffer.Length)
            {
                WriteRawLittleEndian32SlowPath(ref buffer, ref state, value);
            }
            else
            {
                BinaryPrimitives.WriteUInt32LittleEndian(buffer.Slice(state.position), value);
                state.position += length;
            }
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        private static void WriteRawLittleEndian32SlowPath(ref Span<byte> buffer, ref WriterInternalState state, uint value)
        {
            WriteRawByte(ref buffer, ref state, (byte)value);
            WriteRawByte(ref buffer, ref state, (byte)(value >> 8));
            WriteRawByte(ref buffer, ref state, (byte)(value >> 16));
            WriteRawByte(ref buffer, ref state, (byte)(value >> 24));
        }

        public static void WriteRawLittleEndian64(ref Span<byte> buffer, ref WriterInternalState state, ulong value)
        {
            const int length = sizeof(ulong);
            if (state.position + length > buffer.Length)
            {
                WriteRawLittleEndian64SlowPath(ref buffer, ref state, value);
            }
            else
            {
                BinaryPrimitives.WriteUInt64LittleEndian(buffer.Slice(state.position), value);
                state.position += length;
            }
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        public static void WriteRawLittleEndian64SlowPath(ref Span<byte> buffer, ref WriterInternalState state, ulong value)
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

        private static void WriteRawByte(ref Span<byte> buffer, ref WriterInternalState state, byte value)
        {
            if (state.position == buffer.Length)
            {
                WriteBufferHelper.RefreshBuffer(ref buffer, ref state);
            }

            buffer[state.position++] = value;
        }

        /// <summary>
        /// Writes out an array of bytes.
        /// </summary>
        public static void WriteRawBytes(ref Span<byte> buffer, ref WriterInternalState state, byte[] value)
        {
            WriteRawBytes(ref buffer, ref state, new ReadOnlySpan<byte>(value));
        }

        /// <summary>
        /// Writes out part of an array of bytes.
        /// </summary>
        public static void WriteRawBytes(ref Span<byte> buffer, ref WriterInternalState state, byte[] value, int offset, int length)
        {
            WriteRawBytes(ref buffer, ref state, new ReadOnlySpan<byte>(value, offset, length));
        }

        /// <summary>
        /// Writes out part of an array of bytes.
        /// </summary>
        public static void WriteRawBytes(ref Span<byte> buffer, ref WriterInternalState state, ReadOnlySpan<byte> value)
        {
            if (buffer.Length - state.position >= value.Length)
            {
                // We have room in the current buffer.    
                value.CopyTo(buffer.Slice(state.position, value.Length));
                state.position += value.Length;
            }
            else
            {
                // When writing to a CodedOutputStream backed by a Stream, we could avoid
                // copying the data twice (first copying to the current buffer and
                // and later writing from the current buffer to the underlying Stream)
                // in some circumstances by writing the data directly to the underlying Stream.
                // Current this is not being done to avoid specialcasing the code for
                // CodedOutputStream vs IBufferWriter<byte>.
                int bytesWritten = 0;
                while (buffer.Length - state.position < value.Length - bytesWritten)
                {
                    int length = buffer.Length - state.position;
                    value.Slice(bytesWritten, length).CopyTo(buffer.Slice(state.position, length));
                    bytesWritten += length;
                    state.position += length;
                    WriteBufferHelper.RefreshBuffer(ref buffer, ref state);
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
        public static void WriteTag(ref Span<byte> buffer, ref WriterInternalState state, int fieldNumber, WireFormat.WireType type)
        {
            WriteRawVarint32(ref buffer, ref state, WireFormat.MakeTag(fieldNumber, type));
        }

        /// <summary>
        /// Writes an already-encoded tag.
        /// </summary>
        public static void WriteTag(ref Span<byte> buffer, ref WriterInternalState state, uint tag)
        {
            WriteRawVarint32(ref buffer, ref state, tag);
        }

        /// <summary>
        /// Writes the given single-byte tag directly to the stream.
        /// </summary>
        public static void WriteRawTag(ref Span<byte> buffer, ref WriterInternalState state, byte b1)
        {
            WriteRawByte(ref buffer, ref state, b1);
        }

        /// <summary>
        /// Writes the given two-byte tag directly to the stream.
        /// </summary>
        public static void WriteRawTag(ref Span<byte> buffer, ref WriterInternalState state, byte b1, byte b2)
        {
            if (state.position + 2 > buffer.Length)
            {
                WriteRawTagSlowPath(ref buffer, ref state, b1, b2);
            }
            else
            {
                buffer[state.position++] = b1;
                buffer[state.position++] = b2;
            }
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        private static void WriteRawTagSlowPath(ref Span<byte> buffer, ref WriterInternalState state, byte b1, byte b2)
        {
            WriteRawByte(ref buffer, ref state, b1);
            WriteRawByte(ref buffer, ref state, b2);
        }

        /// <summary>
        /// Writes the given three-byte tag directly to the stream.
        /// </summary>
        public static void WriteRawTag(ref Span<byte> buffer, ref WriterInternalState state, byte b1, byte b2, byte b3)
        {
            if (state.position + 3 > buffer.Length)
            {
                WriteRawTagSlowPath(ref buffer, ref state, b1, b2, b3);
            }
            else
            {
                buffer[state.position++] = b1;
                buffer[state.position++] = b2;
                buffer[state.position++] = b3;
            }
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        private static void WriteRawTagSlowPath(ref Span<byte> buffer, ref WriterInternalState state, byte b1, byte b2, byte b3)
        {
            WriteRawByte(ref buffer, ref state, b1);
            WriteRawByte(ref buffer, ref state, b2);
            WriteRawByte(ref buffer, ref state, b3);
        }

        /// <summary>
        /// Writes the given four-byte tag directly to the stream.
        /// </summary>
        public static void WriteRawTag(ref Span<byte> buffer, ref WriterInternalState state, byte b1, byte b2, byte b3, byte b4)
        {
            if (state.position + 4 > buffer.Length)
            {
                WriteRawTagSlowPath(ref buffer, ref state, b1, b2, b3, b4);
            }
            else
            {
                buffer[state.position++] = b1;
                buffer[state.position++] = b2;
                buffer[state.position++] = b3;
                buffer[state.position++] = b4;
            }
        }

        [MethodImpl(MethodImplOptions.NoInlining)]

        private static void WriteRawTagSlowPath(ref Span<byte> buffer, ref WriterInternalState state, byte b1, byte b2, byte b3, byte b4)
        {
            WriteRawByte(ref buffer, ref state, b1);
            WriteRawByte(ref buffer, ref state, b2);
            WriteRawByte(ref buffer, ref state, b3);
            WriteRawByte(ref buffer, ref state, b4);
        }

        /// <summary>
        /// Writes the given five-byte tag directly to the stream.
        /// </summary>
        public static void WriteRawTag(ref Span<byte> buffer, ref WriterInternalState state, byte b1, byte b2, byte b3, byte b4, byte b5)
        {
            if (state.position + 5 > buffer.Length)
            {
                WriteRawTagSlowPath(ref buffer, ref state, b1, b2, b3, b4, b5);
            }
            else
            {
                buffer[state.position++] = b1;
                buffer[state.position++] = b2;
                buffer[state.position++] = b3;
                buffer[state.position++] = b4;
                buffer[state.position++] = b5;
            }
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        private static void WriteRawTagSlowPath(ref Span<byte> buffer, ref WriterInternalState state, byte b1, byte b2, byte b3, byte b4, byte b5)
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