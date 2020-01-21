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

#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
using System;
using System.Buffers;
using System.Buffers.Binary;
using System.IO;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Security;
using System.Text;

namespace Google.Protobuf
{
    /// <summary>
    /// Encodes and writes protocol message fields.
    /// Note: experimental API that can change or be removed without any prior notice.
    /// </summary>
    /// <remarks>
    /// <para>
    /// This class is generally used by generated code to write appropriate
    /// primitives to the stream. It effectively encapsulates the lowest
    /// levels of protocol buffer format. Unlike some other implementations,
    /// this does not include combined "write tag and value" methods. Generated
    /// code knows the exact byte representations of the tags they're going to write,
    /// so there's no need to re-encode them each time. Manually-written code calling
    /// this class should just call one of the <c>WriteTag</c> overloads before each value.
    /// </para>
    /// <para>
    /// Repeated fields and map fields are not handled by this class; use <c>RepeatedField&lt;T&gt;</c>
    /// and <c>MapField&lt;TKey, TValue&gt;</c> to serialize such fields.
    /// </para>
    /// </remarks>
    [SecuritySafeCritical]
    public ref struct CodedOutputWriter
    {
        private Encoder encoder;

        /// <summary>
        /// The underlying <see cref="IBufferWriter{T}"/>.
        /// </summary>
        private IBufferWriter<byte> output;

        /// <summary>
        /// The cached span.
        /// </summary>
        private Span<byte> span;

        /// <summary>
        /// The number of uncommitted bytes in the cached span.
        /// </summary>
        private int buffered;

        /// <summary>
        /// Creates a new CodedOutputWriter that writes to the given
        /// <see cref="IBufferWriter{T}"/>.
        /// </summary>
        /// <param name="writer">The <see cref="IBufferWriter{T}"/> to write to.</param>
        public CodedOutputWriter(IBufferWriter<byte> writer)
        {
            buffered = 0;

            span = writer.GetSpan();

            this.output = writer;
            this.encoder = null;
        }

        #region Writing of values (not including tags)

        /// <summary>
        /// Writes a double field value, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteDouble(double value)
        {
            WriteRawLittleEndian64((ulong) BitConverter.DoubleToInt64Bits(value));
        }

        /// <summary>
        /// Writes a float field value, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteFloat(float value)
        {
            const int length = sizeof(float);

            Ensure(length);

            var floatSpan = span.Slice(buffered, length);

            Unsafe.WriteUnaligned(ref MemoryMarshal.GetReference(floatSpan), value);

            if (!BitConverter.IsLittleEndian)
            {
                floatSpan.Reverse();
            }

            buffered += length;
        }

        /// <summary>
        /// Writes a uint64 field value, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteUInt64(ulong value)
        {
            WriteRawVarint64(value);
        }

        /// <summary>
        /// Writes an int64 field value, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteInt64(long value)
        {
            WriteRawVarint64((ulong) value);
        }

        /// <summary>
        /// Writes an int32 field value, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteInt32(int value)
        {
            if (value >= 0)
            {
                WriteRawVarint32((uint) value);
            }
            else
            {
                // Must sign-extend.
                WriteRawVarint64((ulong) value);
            }
        }

        /// <summary>
        /// Writes a fixed64 field value, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteFixed64(ulong value)
        {
            WriteRawLittleEndian64(value);
        }

        /// <summary>
        /// Writes a fixed32 field value, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteFixed32(uint value)
        {
            WriteRawLittleEndian32(value);
        }

        /// <summary>
        /// Writes a bool field value, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteBool(bool value)
        {
            WriteRawByte(value ? (byte) 1 : (byte) 0);
        }

        /// <summary>
        /// Writes a string field value, without a tag, to the stream.
        /// The data is length-prefixed.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteString(string value)
        {
            int length = Encoding.UTF8.GetByteCount(value);
            WriteLength(length);

            if (span.Length < length + buffered)
            {
                // String doesn't fit in the remaining buffer. Refresh buffer without specifying a size
                // to get the default sized buffer
                EnsureMore();

                if (span.Length < length + buffered)
                {
                    // String doesn't fit in refreshed buffer. Write across multiple
                    WriteStringMultiBuffer(value);
                }
                else
                {
                    // String now fits in refreshed buffer
                    WriteStringSingleBuffer(value, length);
                }
            }
            else
            {
                // String fits in remaining buffer space
                WriteStringSingleBuffer(value, length);
            }
        }

        private void WriteStringSingleBuffer(string value, int length)
        {
            // Can write string to a single buffer
            Span<byte> buffer = span.Slice(buffered);

            if (length == value.Length)
            {
                // Fast path when all content is ASCII
                for (int i = 0; i < length; i++)
                {
                    buffer[i] = (byte)value[i];
                }

                buffered += length;
            }
            else
            {
                ReadOnlySpan<char> source = value.AsSpan();

                int bytesUsed;

                unsafe
                {
                    fixed (char* sourceChars = &MemoryMarshal.GetReference(source))
                    fixed (byte* destinationBytes = &MemoryMarshal.GetReference(buffer))
                    {
                        bytesUsed = Encoding.UTF8.GetBytes(sourceChars, source.Length, destinationBytes, buffer.Length);
                    }
                }

                buffered += bytesUsed;
            }
        }

        private void WriteStringMultiBuffer(string value)
        {
            // The destination byte array might not be large enough so multiple writes are sometimes required
            if (encoder == null)
            {
                encoder = Encoding.UTF8.GetEncoder();
            }

            ReadOnlySpan<char> source = value.AsSpan();
            int written = 0;

            while (true)
            {
                int bytesUsed;
                int charsUsed;

                unsafe
                {
                    fixed (char* sourceChars = &MemoryMarshal.GetReference(source))
                    fixed (byte* destinationBytes = &MemoryMarshal.GetReference(span))
                    {
                        encoder.Convert(sourceChars, source.Length, destinationBytes, span.Length, false, out charsUsed, out bytesUsed, out _);
                    }
                }

                source = source.Slice(charsUsed);
                written += bytesUsed;

                buffered += bytesUsed;

                if (source.Length == 0)
                {
                    break;
                }

                EnsureMore();
            }
        }

        /// <summary>
        /// Writes a message, without a tag, to the stream.
        /// The data is length-prefixed.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteMessage(IBufferMessage value)
        {
            WriteLength(value.CalculateSize());
            value.WriteTo(ref this);
        }

        /// <summary>
        /// Writes a group, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteGroup(IBufferMessage value)
        {
            value.WriteTo(ref this);
        }

        /// <summary>
        /// Write a byte string, without a tag, to the stream.
        /// The data is length-prefixed.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteBytes(ByteString value)
        {
            WriteLength(value.Length);

            Write(value.Span);
        }

        /// <summary>
        /// Writes a uint32 value, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteUInt32(uint value)
        {
            WriteRawVarint32(value);
        }

        /// <summary>
        /// Writes an enum value, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteEnum(int value)
        {
            WriteInt32(value);
        }

        /// <summary>
        /// Writes an sfixed32 value, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write.</param>
        public void WriteSFixed32(int value)
        {
            WriteRawLittleEndian32((uint) value);
        }

        /// <summary>
        /// Writes an sfixed64 value, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteSFixed64(long value)
        {
            WriteRawLittleEndian64((ulong) value);
        }

        /// <summary>
        /// Writes an sint32 value, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteSInt32(int value)
        {
            WriteRawVarint32(CodedOutputStream.EncodeZigZag32(value));
        }

        /// <summary>
        /// Writes an sint64 value, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteSInt64(long value)
        {
            WriteRawVarint64(CodedOutputStream.EncodeZigZag64(value));
        }

        /// <summary>
        /// Writes a length (in bytes) for length-delimited data.
        /// </summary>
        /// <remarks>
        /// This method simply writes a rawint, but exists for clarity in calling code.
        /// </remarks>
        /// <param name="length">Length value, in bytes.</param>
        public void WriteLength(int length)
        {
            WriteRawVarint32((uint) length);
        }

        #endregion

        #region Raw tag writing
        /// <summary>
        /// Encodes and writes a tag.
        /// </summary>
        /// <param name="fieldNumber">The number of the field to write the tag for</param>
        /// <param name="type">The wire format type of the tag to write</param>
        public void WriteTag(int fieldNumber, WireFormat.WireType type)
        {
            WriteRawVarint32(WireFormat.MakeTag(fieldNumber, type));
        }

        /// <summary>
        /// Writes an already-encoded tag.
        /// </summary>
        /// <param name="tag">The encoded tag</param>
        public void WriteTag(uint tag)
        {
            WriteRawVarint32(tag);
        }

        /// <summary>
        /// Writes the given single-byte tag directly to the stream.
        /// </summary>
        /// <param name="b1">The encoded tag</param>
        public void WriteRawTag(byte b1)
        {
            if (buffered > span.Length)
            {
                EnsureMore(1);
            }

            span[buffered++] = b1;
        }

        /// <summary>
        /// Writes the given two-byte tag directly to the stream.
        /// </summary>
        /// <param name="b1">The first byte of the encoded tag</param>
        /// <param name="b2">The second byte of the encoded tag</param>
        public void WriteRawTag(byte b1, byte b2)
        {
            if (buffered + 1 > span.Length)
            {
                EnsureMore(2);
            }

            span[buffered++] = b1;
            span[buffered++] = b2;
        }

        /// <summary>
        /// Writes the given three-byte tag directly to the stream.
        /// </summary>
        /// <param name="b1">The first byte of the encoded tag</param>
        /// <param name="b2">The second byte of the encoded tag</param>
        /// <param name="b3">The third byte of the encoded tag</param>
        public void WriteRawTag(byte b1, byte b2, byte b3)
        {
            if (buffered + 2 > span.Length)
            {
                EnsureMore(3);
            }

            span[buffered++] = b1;
            span[buffered++] = b2;
            span[buffered++] = b3;
        }

        /// <summary>
        /// Writes the given four-byte tag directly to the stream.
        /// </summary>
        /// <param name="b1">The first byte of the encoded tag</param>
        /// <param name="b2">The second byte of the encoded tag</param>
        /// <param name="b3">The third byte of the encoded tag</param>
        /// <param name="b4">The fourth byte of the encoded tag</param>
        public void WriteRawTag(byte b1, byte b2, byte b3, byte b4)
        {
            if (buffered + 3 > span.Length)
            {
                EnsureMore(4);
            }

            span[buffered++] = b1;
            span[buffered++] = b2;
            span[buffered++] = b3;
            span[buffered++] = b4;
        }

        /// <summary>
        /// Writes the given five-byte tag directly to the stream.
        /// </summary>
        /// <param name="b1">The first byte of the encoded tag</param>
        /// <param name="b2">The second byte of the encoded tag</param>
        /// <param name="b3">The third byte of the encoded tag</param>
        /// <param name="b4">The fourth byte of the encoded tag</param>
        /// <param name="b5">The fifth byte of the encoded tag</param>
        public void WriteRawTag(byte b1, byte b2, byte b3, byte b4, byte b5)
        {
            if (buffered + 4 > span.Length)
            {
                EnsureMore(5);
            }

            span[buffered++] = b1;
            span[buffered++] = b2;
            span[buffered++] = b3;
            span[buffered++] = b4;
            span[buffered++] = b5;
        }
        #endregion

        #region Underlying writing primitives
        internal void WriteRawVarint32(uint value)
        {
            // Optimize for the common case of a single byte value
            if (value < 128 && buffered < span.Length)
            {
                span[buffered++] = (byte)value;
            }
            else
            {
                while (value > 127)
                {
                    Ensure(1);
                    span[buffered++] = (byte)((value & 0x7F) | 0x80);
                    value >>= 7;
                }

                Ensure(1);
                span[buffered++] = (byte)value;
            }
        }

        internal void WriteRawVarint64(ulong value)
        {
            // Optimize for the common case of a single byte value
            if (value < 128 && buffered < span.Length)
            {
                span[buffered++] = (byte)value;
            }
            else
            {
                while (value > 127)
                {
                    Ensure(1);
                    span[buffered++] = (byte)((value & 0x7F) | 0x80);
                    value >>= 7;
                }

                Ensure(1);
                span[buffered++] = (byte)value;
            }
        }

        internal void WriteRawLittleEndian32(uint value)
        {
            const int length = 4;

            Ensure(length);

            BinaryPrimitives.WriteUInt32LittleEndian(span.Slice(buffered), value);

            buffered += length;
        }

        internal void WriteRawLittleEndian64(ulong value)
        {
            const int length = 8;

            Ensure(length);

            BinaryPrimitives.WriteUInt64LittleEndian(span.Slice(buffered), value);

            buffered += length;
        }

        internal void WriteRawByte(byte value)
        {
            if (buffered > span.Length)
            {
                EnsureMore(1);
            }

            span[buffered++] = value;
        }

        /// <summary>
        /// Writes out an array of bytes.
        /// </summary>
        internal void WriteRawBytes(byte[] value)
        {
            WriteRawBytes(value, 0, value.Length);
        }

        /// <summary>
        /// Writes out part of an array of bytes.
        /// </summary>
        internal void WriteRawBytes(byte[] value, int offset, int length)
        {
            Write(value.AsSpan(offset, length));
        }

        #endregion

        #region Buffer
        /// <summary>
        /// Copies the caller's buffer into the writer.
        /// </summary>
        /// <param name="source">The buffer to copy in.</param>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private void Write(ReadOnlySpan<byte> source)
        {
            if (span.Length - buffered >= source.Length)
            {
                source.CopyTo(span.Slice(buffered));
                buffered += source.Length;
            }
            else
            {
                WriteMultiBuffer(source);
            }
        }

        /// <summary>
        /// Acquires a new buffer if necessary to ensure that some given number of bytes can be written to a single buffer.
        /// </summary>
        /// <param name="count">The number of bytes that must be allocated in a single buffer.</param>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private void Ensure(int count = 1)
        {
            if (span.Length < count + buffered)
            {
                EnsureMore(count);
            }
        }

        /// <summary>
        /// Gets a fresh span to write to, with an optional minimum size.
        /// </summary>
        /// <param name="count">The minimum size for the next requested buffer.</param>
        [MethodImpl(MethodImplOptions.NoInlining)]
        private void EnsureMore(int count = 0)
        {
            // This method should only be called when the end of the current span is reached.
            // We don't want it inlined because it is called infrequently and it is used by
            // methods that we want to be as inlinable as possible.
            if (buffered > 0)
            {
                Flush();
            }

            span = output.GetSpan(count);
        }

        /// <summary>
        /// Copies the caller's buffer into this writer, potentially across multiple buffers from the underlying writer.
        /// </summary>
        /// <param name="source">The buffer to copy into this writer.</param>
        private void WriteMultiBuffer(ReadOnlySpan<byte> source)
        {
            Span<byte> buffer = span.Slice(buffered);

            if (buffer.Length >= source.Length)
            {
                source.CopyTo(buffer);
                buffered += source.Length;
            }
            else
            {
                while (source.Length > 0)
                {
                    EnsureMore();

                    var writable = Math.Min(source.Length, span.Length);
                    source.Slice(0, writable).CopyTo(span);
                    source = source.Slice(writable);
                    buffered += writable;
                }
            }
        }

        /// <summary>
        /// Flush uncommited data to the output writer.
        /// </summary>
        public void Flush()
        {
            var buffered = this.buffered;
            if (buffered > 0)
            {
                this.buffered = 0;
                output.Advance(buffered);
                span = default;
            }
        }
        #endregion
    }
}
#endif