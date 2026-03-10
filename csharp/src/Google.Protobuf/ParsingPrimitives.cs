#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System;
using System.Buffers;
using System.Buffers.Binary;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Security;
using System.Text;

namespace Google.Protobuf
{
    /// <summary>
    /// Primitives for parsing protobuf wire format.
    /// </summary>
    [SecuritySafeCritical]
    internal static class ParsingPrimitives
    {
        internal static readonly Encoding Utf8Encoding =
            new UTF8Encoding(encoderShouldEmitUTF8Identifier: false, throwOnInvalidBytes: true);

        private const int StackallocThreshold = 256;

        /// <summary>
        /// Reads a length for length-delimited data.
        /// </summary>
        /// <remarks>
        /// This is internally just reading a varint, but this method exists
        /// to make the calling code clearer.
        /// </remarks>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static int ParseLength(ref ReadOnlySpan<byte> buffer, ref ParserInternalState state)
        {
            return (int)ParseRawVarint32(ref buffer, ref state);
        }

        /// <summary>
        /// Parses the next tag.
        /// If the end of logical stream was reached, an invalid tag of 0 is returned.
        /// </summary>
        public static uint ParseTag(ref ReadOnlySpan<byte> buffer, ref ParserInternalState state)
        {
            // The "nextTag" logic is there only as an optimization for reading non-packed repeated / map
            // fields and is strictly speaking not necessary.
            // TODO: look into simplifying the ParseTag logic.
            if (state.hasNextTag)
            {
                state.lastTag = state.nextTag;
                state.hasNextTag = false;
                return state.lastTag;
            }

            // Optimize for the incredibly common case of having at least two bytes left in the buffer,
            // and those two bytes being enough to get the tag. This will be true for fields up to 4095.
            if (state.bufferPos + 2 <= state.bufferSize)
            {
                int tmp = buffer[state.bufferPos++];
                if (tmp < 128)
                {
                    state.lastTag = (uint)tmp;
                }
                else
                {
                    int result = tmp & 0x7f;
                    if ((tmp = buffer[state.bufferPos++]) < 128)
                    {
                        result |= tmp << 7;
                        state.lastTag = (uint) result;
                    }
                    else
                    {
                        // Nope, rewind and go the potentially slow route.
                        state.bufferPos -= 2;
                        state.lastTag = ParsingPrimitives.ParseRawVarint32(ref buffer, ref state);
                    }
                }
            }
            else
            {
                if (SegmentedBufferHelper.IsAtEnd(ref buffer, ref state))
                {
                    state.lastTag = 0;
                    return 0;
                }

                state.lastTag = ParsingPrimitives.ParseRawVarint32(ref buffer, ref state);
            }
            if (WireFormat.GetTagFieldNumber(state.lastTag) == 0)
            {
                // If we actually read a tag with a field of 0, that's not a valid tag.
                throw InvalidProtocolBufferException.InvalidTag();
            }
            return state.lastTag;
        }

        /// <summary>
        /// Peeks at the next tag in the stream. If it matches <paramref name="tag"/>,
        /// the tag is consumed and the method returns <c>true</c>; otherwise, the
        /// stream is left in the original position and the method returns <c>false</c>.
        /// </summary>
        public static bool MaybeConsumeTag(ref ReadOnlySpan<byte> buffer, ref ParserInternalState state, uint tag)
        {
            if (PeekTag(ref buffer, ref state) == tag)
            {
                state.hasNextTag = false;
                return true;
            }
            return false;
        }

        /// <summary>
        /// Peeks at the next field tag. This is like calling <see cref="ParseTag"/>, but the
        /// tag is not consumed. (So a subsequent call to <see cref="ParseTag"/> will return the
        /// same value.)
        /// </summary>
        public static uint PeekTag(ref ReadOnlySpan<byte> buffer, ref ParserInternalState state)
        {
            if (state.hasNextTag)
            {
                return state.nextTag;
            }

            uint savedLast = state.lastTag;
            state.nextTag = ParseTag(ref buffer, ref state);
            state.hasNextTag = true;
            state.lastTag = savedLast; // Undo the side effect of ReadTag
            return state.nextTag;
        }

        /// <summary>
        /// Parses a raw varint.
        /// </summary>
        public static ulong ParseRawVarint64(ref ReadOnlySpan<byte> buffer, ref ParserInternalState state)
        {
            if (state.bufferPos + 10 > state.bufferSize)
            {
                return ParseRawVarint64SlowPath(ref buffer, ref state);
            }

            ulong result = buffer[state.bufferPos++];
            if (result < 128)
            {
                return result;
            }
            result &= 0x7f;
            int shift = 7;
            do
            {
                byte b = buffer[state.bufferPos++];
                result |= (ulong)(b & 0x7F) << shift;
                if (b < 0x80)
                {
                    return result;
                }
                shift += 7;
            }
            while (shift < 64);

            throw InvalidProtocolBufferException.MalformedVarint();
        }

        private static ulong ParseRawVarint64SlowPath(ref ReadOnlySpan<byte> buffer, ref ParserInternalState state)
        {
            int shift = 0;
            ulong result = 0;
            do
            {
                byte b = ReadRawByte(ref buffer, ref state);
                result |= (ulong)(b & 0x7F) << shift;
                if (b < 0x80)
                {
                    return result;
                }
                shift += 7;
            }
            while (shift < 64);

            throw InvalidProtocolBufferException.MalformedVarint();
        }

        /// <summary>
        /// Parses a raw Varint.  If larger than 32 bits, discard the upper bits.
        /// This method is optimised for the case where we've got lots of data in the buffer.
        /// That means we can check the size just once, then just read directly from the buffer
        /// without constant rechecking of the buffer length.
        /// </summary>
        public static uint ParseRawVarint32(ref ReadOnlySpan<byte> buffer, ref ParserInternalState state)
        {
            if (state.bufferPos + 5 > state.bufferSize)
            {
                return ParseRawVarint32SlowPath(ref buffer, ref state);
            }

            int tmp = buffer[state.bufferPos++];
            if (tmp < 128)
            {
                return (uint)tmp;
            }
            int result = tmp & 0x7f;
            if ((tmp = buffer[state.bufferPos++]) < 128)
            {
                result |= tmp << 7;
            }
            else
            {
                result |= (tmp & 0x7f) << 7;
                if ((tmp = buffer[state.bufferPos++]) < 128)
                {
                    result |= tmp << 14;
                }
                else
                {
                    result |= (tmp & 0x7f) << 14;
                    if ((tmp = buffer[state.bufferPos++]) < 128)
                    {
                        result |= tmp << 21;
                    }
                    else
                    {
                        result |= (tmp & 0x7f) << 21;
                        result |= (tmp = buffer[state.bufferPos++]) << 28;
                        if (tmp >= 128)
                        {
                            // Discard upper 32 bits.
                            // Note that this has to use ReadRawByte() as we only ensure we've
                            // got at least 5 bytes at the start of the method. This lets us
                            // use the fast path in more cases, and we rarely hit this section of code.
                            for (int i = 0; i < 5; i++)
                            {
                                if (ReadRawByte(ref buffer, ref state) < 128)
                                {
                                    return (uint) result;
                                }
                            }
                            throw InvalidProtocolBufferException.MalformedVarint();
                        }
                    }
                }
            }
            return (uint)result;
        }

        private static uint ParseRawVarint32SlowPath(ref ReadOnlySpan<byte> buffer, ref ParserInternalState state)
        {
            int tmp = ReadRawByte(ref buffer, ref state);
            if (tmp < 128)
            {
                return (uint) tmp;
            }
            int result = tmp & 0x7f;
            if ((tmp = ReadRawByte(ref buffer, ref state)) < 128)
            {
                result |= tmp << 7;
            }
            else
            {
                result |= (tmp & 0x7f) << 7;
                if ((tmp = ReadRawByte(ref buffer, ref state)) < 128)
                {
                    result |= tmp << 14;
                }
                else
                {
                    result |= (tmp & 0x7f) << 14;
                    if ((tmp = ReadRawByte(ref buffer, ref state)) < 128)
                    {
                        result |= tmp << 21;
                    }
                    else
                    {
                        result |= (tmp & 0x7f) << 21;
                        result |= (tmp = ReadRawByte(ref buffer, ref state)) << 28;
                        if (tmp >= 128)
                        {
                            // Discard upper 32 bits.
                            for (int i = 0; i < 5; i++)
                            {
                                if (ReadRawByte(ref buffer, ref state) < 128)
                                {
                                    return (uint) result;
                                }
                            }
                            throw InvalidProtocolBufferException.MalformedVarint();
                        }
                    }
                }
            }
            return (uint) result;
        }

        /// <summary>
        /// Parses a 32-bit little-endian integer.
        /// </summary>
        public static uint ParseRawLittleEndian32(ref ReadOnlySpan<byte> buffer, ref ParserInternalState state)
        {
            const int uintLength = sizeof(uint);
            const int ulongLength = sizeof(ulong);
            if (state.bufferPos + ulongLength > state.bufferSize)
            {
                return ParseRawLittleEndian32SlowPath(ref buffer, ref state);
            }
            // ReadUInt32LittleEndian is many times slower than ReadUInt64LittleEndian (at least on some runtimes)
            // so it's faster better to use ReadUInt64LittleEndian and truncate the result.
            uint result = (uint) BinaryPrimitives.ReadUInt64LittleEndian(buffer.Slice(state.bufferPos, ulongLength));
            state.bufferPos += uintLength;
            return result;
        }

        private static uint ParseRawLittleEndian32SlowPath(ref ReadOnlySpan<byte> buffer, ref ParserInternalState state)
        {
            uint b1 = ReadRawByte(ref buffer, ref state);
            uint b2 = ReadRawByte(ref buffer, ref state);
            uint b3 = ReadRawByte(ref buffer, ref state);
            uint b4 = ReadRawByte(ref buffer, ref state);
            return b1 | (b2 << 8) | (b3 << 16) | (b4 << 24);
        }

        /// <summary>
        /// Parses a 64-bit little-endian integer.
        /// </summary>
        public static ulong ParseRawLittleEndian64(ref ReadOnlySpan<byte> buffer, ref ParserInternalState state)
        {
            const int length = sizeof(ulong);
            if (state.bufferPos + length > state.bufferSize)
            {
                return ParseRawLittleEndian64SlowPath(ref buffer, ref state);
            }
            ulong result = BinaryPrimitives.ReadUInt64LittleEndian(buffer.Slice(state.bufferPos, length));
            state.bufferPos += length;
            return result;
        }

        private static ulong ParseRawLittleEndian64SlowPath(ref ReadOnlySpan<byte> buffer, ref ParserInternalState state)
        {
            ulong b1 = ReadRawByte(ref buffer, ref state);
            ulong b2 = ReadRawByte(ref buffer, ref state);
            ulong b3 = ReadRawByte(ref buffer, ref state);
            ulong b4 = ReadRawByte(ref buffer, ref state);
            ulong b5 = ReadRawByte(ref buffer, ref state);
            ulong b6 = ReadRawByte(ref buffer, ref state);
            ulong b7 = ReadRawByte(ref buffer, ref state);
            ulong b8 = ReadRawByte(ref buffer, ref state);
            return b1 | (b2 << 8) | (b3 << 16) | (b4 << 24)
                    | (b5 << 32) | (b6 << 40) | (b7 << 48) | (b8 << 56);
        }

        /// <summary>
        /// Parses a double value.
        /// </summary>
        public static double ParseDouble(ref ReadOnlySpan<byte> buffer, ref ParserInternalState state)
        {
            const int length = sizeof(double);
            if (!BitConverter.IsLittleEndian || state.bufferPos + length > state.bufferSize)
            {
                return BitConverter.Int64BitsToDouble((long)ParseRawLittleEndian64(ref buffer, ref state));
            }
            // ReadUnaligned uses processor architecture for endianness.
            double result = Unsafe.ReadUnaligned<double>(ref MemoryMarshal.GetReference(buffer.Slice(state.bufferPos, length)));
            state.bufferPos += length;
            return result;
        }

        /// <summary>
        /// Parses a float value.
        /// </summary>
        public static float ParseFloat(ref ReadOnlySpan<byte> buffer, ref ParserInternalState state)
        {
            const int length = sizeof(float);
            if (!BitConverter.IsLittleEndian || state.bufferPos + length > state.bufferSize)
            {
                return ParseFloatSlow(ref buffer, ref state);
            }
            // ReadUnaligned uses processor architecture for endianness.
            float result = Unsafe.ReadUnaligned<float>(ref MemoryMarshal.GetReference(buffer.Slice(state.bufferPos, length)));
            state.bufferPos += length;
            return result;
        }

        private static unsafe float ParseFloatSlow(ref ReadOnlySpan<byte> buffer, ref ParserInternalState state)
        {
            const int length = sizeof(float);
            byte* stackBuffer = stackalloc byte[length];
            Span<byte> tempSpan = new Span<byte>(stackBuffer, length);
            for (int i = 0; i < length; i++)
            {
                tempSpan[i] = ReadRawByte(ref buffer, ref state);
            }

            // Content is little endian. Reverse if needed to match endianness of architecture.
            if (!BitConverter.IsLittleEndian)
            {
                tempSpan.Reverse();
            }
            return Unsafe.ReadUnaligned<float>(ref MemoryMarshal.GetReference(tempSpan));
        }

        /// <summary>
        /// Reads a fixed size of bytes from the input.
        /// </summary>
        /// <exception cref="InvalidProtocolBufferException">
        /// the end of the stream or the current limit was reached
        /// </exception>
        public static byte[] ReadRawBytes(ref ReadOnlySpan<byte> buffer, ref ParserInternalState state, int size)
        {
            if (size < 0)
            {
                throw InvalidProtocolBufferException.NegativeSize();
            }

            if (size <= state.bufferSize - state.bufferPos)
            {
                // We have all the bytes we need already.
                byte[] bytes = new byte[size];
                buffer.Slice(state.bufferPos, size).CopyTo(bytes);
                state.bufferPos += size;
                return bytes;
            }

            return ReadRawBytesSlow(ref buffer, ref state, size);
        }

        private static byte[] ReadRawBytesSlow(ref ReadOnlySpan<byte> buffer, ref ParserInternalState state, int size)
        {
            ValidateCurrentLimit(ref buffer, ref state, size);

            if ((!state.segmentedBufferHelper.TotalLength.HasValue && size < buffer.Length) ||
                IsDataAvailableInSource(ref state, size))
            {
                // Reading more bytes than are in the buffer, but not an excessive number
                // of bytes.  We can safely allocate the resulting array ahead of time.

                byte[] bytes = new byte[size];
                ReadRawBytesIntoSpan(ref buffer, ref state, size, bytes);
                return bytes;
            }
            else
            {
                // The size is very large.  For security reasons, we can't allocate the
                // entire byte array yet.  The size comes directly from the input, so a
                // maliciously-crafted message could provide a bogus very large size in
                // order to trick the app into allocating a lot of memory.  We avoid this
                // by allocating and reading only a small chunk at a time, so that the
                // malicious message must actually *be* extremely large to cause
                // problems.  Meanwhile, we limit the allowed size of a message elsewhere.

                List<byte[]> chunks = new List<byte[]>();

                int pos = state.bufferSize - state.bufferPos;
                byte[] firstChunk = new byte[pos];
                buffer.Slice(state.bufferPos, pos).CopyTo(firstChunk);
                chunks.Add(firstChunk);
                state.bufferPos = state.bufferSize;

                // Read all the rest of the bytes we need.
                int sizeLeft = size - pos;
                while (sizeLeft > 0)
                {
                    state.segmentedBufferHelper.RefillBuffer(ref buffer, ref state, true);
                    byte[] chunk = new byte[Math.Min(sizeLeft, state.bufferSize)];

                    buffer.Slice(0, chunk.Length)
                        .CopyTo(chunk);
                    state.bufferPos += chunk.Length;
                    sizeLeft -= chunk.Length;
                    chunks.Add(chunk);
                }

                // OK, got everything.  Now concatenate it all into one buffer.
                byte[] bytes = new byte[size];
                int newPos = 0;
                foreach (byte[] chunk in chunks)
                {
                    Buffer.BlockCopy(chunk, 0, bytes, newPos, chunk.Length);
                    newPos += chunk.Length;
                }

                // Done.
                return bytes;
            }
        }

        /// <summary>
        /// Reads and discards <paramref name="size"/> bytes.
        /// </summary>
        /// <exception cref="InvalidProtocolBufferException">the end of the stream
        /// or the current limit was reached</exception>
        public static void SkipRawBytes(ref ReadOnlySpan<byte> buffer, ref ParserInternalState state, int size)
        {
            if (size < 0)
            {
                throw InvalidProtocolBufferException.NegativeSize();
            }

            ValidateCurrentLimit(ref buffer, ref state, size);

            if (size <= state.bufferSize - state.bufferPos)
            {
                // We have all the bytes we need already.
                state.bufferPos += size;
            }
            else
            {
                // Skipping more bytes than are in the buffer.  First skip what we have.
                int pos = state.bufferSize - state.bufferPos;
                state.bufferPos = state.bufferSize;

                // TODO: If our segmented buffer is backed by a Stream that is seekable, we could skip the bytes more efficiently
                // by simply updating stream's Position property. This used to be supported in the past, but the support was dropped
                // because it would make the segmentedBufferHelper more complex. Support can be reintroduced if needed.
                state.segmentedBufferHelper.RefillBuffer(ref buffer, ref state, true);

                while (size - pos > state.bufferSize)
                {
                    pos += state.bufferSize;
                    state.bufferPos = state.bufferSize;
                    state.segmentedBufferHelper.RefillBuffer(ref buffer, ref state, true);
                }

                state.bufferPos = size - pos;
            }
        }

        /// <summary>
        /// Reads a string field value from the input.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static string ReadString(ref ReadOnlySpan<byte> buffer, ref ParserInternalState state)
        {
            int length = ParsingPrimitives.ParseLength(ref buffer, ref state);
            return ParsingPrimitives.ReadRawString(ref buffer, ref state, length);
        }

        /// <summary>
        /// Reads a bytes field value from the input.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static ByteString ReadBytes(ref ReadOnlySpan<byte> buffer, ref ParserInternalState state)
        {
            int length = ParsingPrimitives.ParseLength(ref buffer, ref state);
            return ByteString.AttachBytes(ParsingPrimitives.ReadRawBytes(ref buffer, ref state, length));
        }

        /// <summary>
        /// Reads a UTF-8 string from the next "length" bytes.
        /// </summary>
        /// <exception cref="InvalidProtocolBufferException">
        /// the end of the stream or the current limit was reached
        /// </exception>
        [SecuritySafeCritical]
        public static string ReadRawString(ref ReadOnlySpan<byte> buffer, ref ParserInternalState state, int length)
        {
            // No need to read any data for an empty string.
            if (length == 0)
            {
                return string.Empty;
            }

            if (length < 0)
            {
                throw InvalidProtocolBufferException.NegativeSize();
            }

#if GOOGLE_PROTOBUF_SUPPORT_FAST_STRING
            if (length <= state.bufferSize - state.bufferPos)
            {
                // Fast path: all bytes to decode appear in the same span.
                ReadOnlySpan<byte> data = buffer.Slice(state.bufferPos, length);

                string value;
                unsafe
                {
                    fixed (byte* sourceBytes = &MemoryMarshal.GetReference(data))
                    {
                        try
                        {
                            value = Utf8Encoding.GetString(sourceBytes, length);
                        }
                        catch (DecoderFallbackException e)
                        {
                            throw InvalidProtocolBufferException.InvalidUtf8(e);
                        }
                    }
                }

                state.bufferPos += length;
                return value;
            }
#endif

            return ReadStringSlow(ref buffer, ref state, length);
        }

        /// <summary>
        /// Reads a string assuming that it is spread across multiple spans in a <see cref="ReadOnlySequence{T}"/>.
        /// </summary>
        private static string ReadStringSlow(ref ReadOnlySpan<byte> buffer, ref ParserInternalState state, int length)
        {
            ValidateCurrentLimit(ref buffer, ref state, length);

#if GOOGLE_PROTOBUF_SUPPORT_FAST_STRING
            if (IsDataAvailable(ref state, length))
            {
                // Read string data into a temporary buffer, either stackalloc'ed or from ArrayPool
                // Once all data is read then call Encoding.GetString on buffer and return to pool if needed.

                byte[] byteArray = null;
                Span<byte> byteSpan = length <= StackallocThreshold ?
                    stackalloc byte[length] :
                    (byteArray = ArrayPool<byte>.Shared.Rent(length));

                try
                {
                    unsafe
                    {
                        fixed (byte* pByteSpan = &MemoryMarshal.GetReference(byteSpan))
                        {
                            // Compiler doesn't like that a potentially stackalloc'd Span<byte> is being used
                            // in a method with a "ref Span<byte> buffer" argument. If the stackalloc'd span was assigned
                            // to the ref argument then bad things would happen. We'll never do that so it is ok.
                            // Make compiler happy by passing a new span created from pointer.
                            var tempSpan = new Span<byte>(pByteSpan, byteSpan.Length);
                            ReadRawBytesIntoSpan(ref buffer, ref state, length, tempSpan);
                            try
                            {
                                return Utf8Encoding.GetString(pByteSpan, length);
                            }
                            catch (DecoderFallbackException e)
                            {
                                throw InvalidProtocolBufferException.InvalidUtf8(e);
                            }
                        }
                    }
                }
                finally
                {
                    if (byteArray != null)
                    {
                        ArrayPool<byte>.Shared.Return(byteArray);
                    }
                }
            }
#endif

            // Slow path: Build a byte array first then copy it.
            // This will be called when reading from a Stream because we don't know the length of the stream,
            // or there is not enough data in the sequence. If there is not enough data then ReadRawBytes will
            // throw an exception.
            byte[] bytes = ReadRawBytes(ref buffer, ref state, length);
            try
            {
                return Utf8Encoding.GetString(bytes, 0, length);
            }
            catch (DecoderFallbackException e)
            {
                throw InvalidProtocolBufferException.InvalidUtf8(e);
            }
        }

        /// <summary>
        /// Validates that the specified size doesn't exceed the current limit. If it does then remaining bytes
        /// are skipped and an error is thrown.
        /// </summary>
        private static void ValidateCurrentLimit(ref ReadOnlySpan<byte> buffer, ref ParserInternalState state, int size)
        {
            if (state.totalBytesRetired + state.bufferPos + size > state.currentLimit)
            {
                // Read to the end of the stream (up to the current limit) anyway.
                SkipRawBytes(ref buffer, ref state, state.currentLimit - state.totalBytesRetired - state.bufferPos);
                // Then fail.
                throw InvalidProtocolBufferException.TruncatedMessage();
            }
        }

        [SecuritySafeCritical]
        private static byte ReadRawByte(ref ReadOnlySpan<byte> buffer, ref ParserInternalState state)
        {
            if (state.bufferPos == state.bufferSize)
            {
                state.segmentedBufferHelper.RefillBuffer(ref buffer, ref state, true);
            }
            return buffer[state.bufferPos++];
        }

        /// <summary>
        /// Reads a varint from the input one byte at a time, so that it does not
        /// read any bytes after the end of the varint. If you simply wrapped the
        /// stream in a CodedInputStream and used ReadRawVarint32(Stream)
        /// then you would probably end up reading past the end of the varint since
        /// CodedInputStream buffers its input.
        /// </summary>
        /// <param name="input"></param>
        /// <returns></returns>
        public static uint ReadRawVarint32(Stream input)
        {
            int result = 0;
            int offset = 0;
            for (; offset < 32; offset += 7)
            {
                int b = input.ReadByte();
                if (b == -1)
                {
                    throw InvalidProtocolBufferException.TruncatedMessage();
                }
                result |= (b & 0x7f) << offset;
                if ((b & 0x80) == 0)
                {
                    return (uint) result;
                }
            }
            // Keep reading up to 64 bits.
            for (; offset < 64; offset += 7)
            {
                int b = input.ReadByte();
                if (b == -1)
                {
                    throw InvalidProtocolBufferException.TruncatedMessage();
                }
                if ((b & 0x80) == 0)
                {
                    return (uint) result;
                }
            }
            throw InvalidProtocolBufferException.MalformedVarint();
        }

        /// <summary>
        /// Decode a 32-bit value with ZigZag encoding.
        /// </summary>
        /// <remarks>
        /// ZigZag encodes signed integers into values that can be efficiently
        /// encoded with varint.  (Otherwise, negative values must be
        /// sign-extended to 32 bits to be varint encoded, thus always taking
        /// 5 bytes on the wire.)
        /// </remarks>
        public static int DecodeZigZag32(uint n)
        {
            return (int)(n >> 1) ^ -(int)(n & 1);
        }

        /// <summary>
        /// Decode a 64-bit value with ZigZag encoding.
        /// </summary>
        /// <remarks>
        /// ZigZag encodes signed integers into values that can be efficiently
        /// encoded with varint.  (Otherwise, negative values must be
        /// sign-extended to 64 bits to be varint encoded, thus always taking
        /// 10 bytes on the wire.)
        /// </remarks>
        public static long DecodeZigZag64(ulong n)
        {
            return (long)(n >> 1) ^ -(long)(n & 1);
        }

        /// <summary>
        /// Checks whether there is known data available of the specified size remaining to parse.
        /// When parsing from a Stream this can return false because we have no knowledge of the amount
        /// of data remaining in the stream until it is read.
        /// </summary>
        public static bool IsDataAvailable(ref ParserInternalState state, int size)
        {
            // Data fits in remaining buffer
            if (size <= state.bufferSize - state.bufferPos)
            {
                return true;
            }

            return IsDataAvailableInSource(ref state, size);
        }

        /// <summary>
        /// Checks whether there is known data available of the specified size remaining to parse
        /// in the underlying data source.
        /// When parsing from a Stream this will return false because we have no knowledge of the amount
        /// of data remaining in the stream until it is read.
        /// </summary>
        private static bool IsDataAvailableInSource(ref ParserInternalState state, int size)
        {
            // Data fits in remaining source data.
            // Note that this will never be true when reading from a stream as the total length is unknown.
            return size <= state.segmentedBufferHelper.TotalLength - state.totalBytesRetired - state.bufferPos;
        }

        /// <summary>
        /// Read raw bytes of the specified length into a span. The amount of data available and the current limit should
        /// be checked before calling this method.
        /// </summary>
        private static void ReadRawBytesIntoSpan(ref ReadOnlySpan<byte> buffer, ref ParserInternalState state, int length, Span<byte> byteSpan)
        {
            int remainingByteLength = length;
            while (remainingByteLength > 0)
            {
                if (state.bufferSize - state.bufferPos == 0)
                {
                    state.segmentedBufferHelper.RefillBuffer(ref buffer, ref state, true);
                }

                ReadOnlySpan<byte> unreadSpan = buffer.Slice(state.bufferPos, Math.Min(remainingByteLength, state.bufferSize - state.bufferPos));
                unreadSpan.CopyTo(byteSpan.Slice(length - remainingByteLength));

                remainingByteLength -= unreadSpan.Length;
                state.bufferPos += unreadSpan.Length;
            }
        }

        /// <summary>
        /// Read LittleEndian packed field from buffer of specified length into a span.
        /// The amount of data available and the current limit should be checked before calling this method.
        /// </summary>
        internal static void ReadPackedFieldLittleEndian(ref ReadOnlySpan<byte> buffer, ref ParserInternalState state, int length, Span<byte> outBuffer)
        {
            Debug.Assert(BitConverter.IsLittleEndian);

            if (length <= state.bufferSize - state.bufferPos)
            {
                buffer.Slice(state.bufferPos, length).CopyTo(outBuffer);
                state.bufferPos += length;
            }
            else
            {
                ReadRawBytesIntoSpan(ref buffer, ref state, length, outBuffer);
            }
        }

    }
}
