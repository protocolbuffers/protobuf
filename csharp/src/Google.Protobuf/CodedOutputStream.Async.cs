#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2017 Google Inc.  All rights reserved.
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

#if !PROTOBUF_NO_ASYNC
using System;
using System.Threading;
using System.Threading.Tasks;

namespace Google.Protobuf
{
    public sealed partial class CodedOutputStream
    {
        #region Writing of values (not including tags)

        /// <summary>
        /// Writes a double field value, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        public Task WriteDoubleAsync(double value, CancellationToken cancellationToken) => WriteRawLittleEndian64Async((ulong)BitConverter.DoubleToInt64Bits(value), cancellationToken);

        /// <summary>
        /// Writes a float field value, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        public async Task WriteFloatAsync(float value, CancellationToken cancellationToken)
        {
            byte[] rawBytes = BitConverter.GetBytes(value);
            if (!BitConverter.IsLittleEndian)
            {
                ByteArray.Reverse(rawBytes);
            }

            if (limit - position >= 4)
            {
                buffer[position++] = rawBytes[0];
                buffer[position++] = rawBytes[1];
                buffer[position++] = rawBytes[2];
                buffer[position++] = rawBytes[3];
            }
            else
            {
                await WriteRawBytesAsync(rawBytes, 0, 4, cancellationToken).ConfigureAwait(false);
            }
        }

        /// <summary>
        /// Writes a uint64 field value, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        public Task WriteUInt64Async(ulong value, CancellationToken cancellationToken) => WriteRawVarint64Async(value, cancellationToken);

        /// <summary>
        /// Writes an int64 field value, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        public Task WriteInt64Async(long value, CancellationToken cancellationToken) => WriteRawVarint64Async((ulong)value, cancellationToken);

        /// <summary>
        /// Writes an int32 field value, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        public Task WriteInt32Async(int value, CancellationToken cancellationToken)
        {
            if (value >= 0)
            {
                return WriteRawVarint32Async((uint)value, cancellationToken);
            }
            else
            {
                // Must sign-extend.
                return WriteRawVarint64Async((ulong)value, cancellationToken);
            }
        }

        /// <summary>
        /// Writes a fixed64 field value, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        public Task WriteFixed64Async(ulong value, CancellationToken cancellationToken) => WriteRawLittleEndian64Async(value, cancellationToken);

        /// <summary>
        /// Writes a fixed32 field value, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        public Task WriteFixed32Async(uint value, CancellationToken cancellationToken) => WriteRawLittleEndian32Async(value, cancellationToken);

        /// <summary>
        /// Writes a bool field value, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        public Task WriteBoolAsync(bool value, CancellationToken cancellationToken) => WriteRawByteAsync(value ? (byte)1 : (byte)0, cancellationToken);

        /// <summary>
        /// Writes a string field value, without a tag, to the stream.
        /// The data is length-prefixed.
        /// </summary>
        /// <param name="value">The value to write</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        public async Task WriteStringAsync(string value, CancellationToken cancellationToken)
        {
            // Optimise the case where we have enough space to write
            // the string directly to the buffer, which should be common.
            int length = Utf8Encoding.GetByteCount(value);
            await WriteLengthAsync(length, cancellationToken).ConfigureAwait(false);
            if (limit - position >= length)
            {
                if (length == value.Length) // Must be all ASCII...
                {
                    for (int i = 0; i < length; i++)
                    {
                        buffer[position + i] = (byte)value[i];
                    }
                }
                else
                {
                    Utf8Encoding.GetBytes(value, 0, value.Length, buffer, position);
                }
                position += length;
            }
            else
            {
                byte[] bytes = Utf8Encoding.GetBytes(value);
                await WriteRawBytesAsync(bytes, cancellationToken).ConfigureAwait(false);
            }
        }

        /// <summary>
        /// Writes a message, without a tag, to the stream.
        /// The data is length-prefixed.
        /// </summary>
        /// <param name="value">The value to write</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        public async Task WriteMessageAsync(IMessage value, CancellationToken cancellationToken)
        {
            await WriteLengthAsync(value.CalculateSize(), cancellationToken).ConfigureAwait(false);
            value.WriteTo(this);
        }

        /// <summary>
        /// Writes a message, without a tag, to the stream.
        /// The data is length-prefixed.
        /// </summary>
        /// <param name="value">The value to write</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        public async Task WriteMessageAsync(IAsyncMessage value, CancellationToken cancellationToken)
        {
            await WriteLengthAsync(value.CalculateSize(), cancellationToken).ConfigureAwait(false);
            await value.WriteToAsync(this, cancellationToken).ConfigureAwait(false);
        }

        /// <summary>
        /// Write a byte string, without a tag, to the stream.
        /// The data is length-prefixed.
        /// </summary>
        /// <param name="value">The value to write</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        public async Task WriteBytesAsync(ByteString value, CancellationToken cancellationToken)
        {
            await WriteLengthAsync(value.Length, cancellationToken).ConfigureAwait(false);
            await value.WriteRawBytesToAsync(this, cancellationToken).ConfigureAwait(false);
        }

        /// <summary>
        /// Writes a uint32 value, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        public Task WriteUInt32Async(uint value, CancellationToken cancellationToken) => WriteRawVarint32Async(value, cancellationToken);

        /// <summary>
        /// Writes an enum value, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        public Task WriteEnumAsync(int value, CancellationToken cancellationToken) => WriteInt32Async(value, cancellationToken);

        /// <summary>
        /// Writes an sfixed32 value, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        public Task WriteSFixed32Async(int value, CancellationToken cancellationToken) => WriteRawLittleEndian32Async((uint)value, cancellationToken);

        /// <summary>
        /// Writes an sfixed64 value, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        public Task WriteSFixed64Async(long value, CancellationToken cancellationToken) => WriteRawLittleEndian64Async((ulong)value, cancellationToken);

        /// <summary>
        /// Writes an sint32 value, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        public Task WriteSInt32Async(int value, CancellationToken cancellationToken) => WriteRawVarint32Async(EncodeZigZag32(value), cancellationToken);

        /// <summary>
        /// Writes an sint64 value, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        public Task WriteSInt64Async(long value, CancellationToken cancellationToken) => WriteRawVarint64Async(EncodeZigZag64(value), cancellationToken);

        /// <summary>
        /// Writes a length (in bytes) for length-delimited data.
        /// </summary>
        /// <remarks>
        /// This method simply writes a rawint, but exists for clarity in calling code.
        /// </remarks>
        /// <param name="length">Length value, in bytes.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        public Task WriteLengthAsync(int length, CancellationToken cancellationToken) => WriteRawVarint32Async((uint)length, cancellationToken);

        #endregion

        #region Raw tag writing
        /// <summary>
        /// Encodes and writes a tag.
        /// </summary>
        /// <param name="fieldNumber">The number of the field to write the tag for</param>
        /// <param name="type">The wire format type of the tag to write</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        public Task WriteTagAsync(int fieldNumber, WireFormat.WireType type, CancellationToken cancellationToken) => WriteRawVarint32Async(WireFormat.MakeTag(fieldNumber, type), cancellationToken);

        /// <summary>
        /// Writes an already-encoded tag.
        /// </summary>
        /// <param name="tag">The encoded tag</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        public Task WriteTagAsync(uint tag, CancellationToken cancellationToken) => WriteRawVarint32Async(tag, cancellationToken);

        /// <summary>
        /// Writes the given single-byte tag directly to the stream.
        /// </summary>
        /// <param name="b1">The encoded tag</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        public Task WriteRawTagAsync(byte b1, CancellationToken cancellationToken) => WriteRawByteAsync(b1, cancellationToken);

        /// <summary>
        /// Writes the given two-byte tag directly to the stream.
        /// </summary>
        /// <param name="b1">The first byte of the encoded tag</param>
        /// <param name="b2">The second byte of the encoded tag</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        public async Task WriteRawTagAsync(byte b1, byte b2, CancellationToken cancellationToken)
        {
            await WriteRawByteAsync(b1, cancellationToken).ConfigureAwait(false);
            await WriteRawByteAsync(b2, cancellationToken).ConfigureAwait(false);
        }

        /// <summary>
        /// Writes the given three-byte tag directly to the stream.
        /// </summary>
        /// <param name="b1">The first byte of the encoded tag</param>
        /// <param name="b2">The second byte of the encoded tag</param>
        /// <param name="b3">The third byte of the encoded tag</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        public async Task WriteRawTagAsync(byte b1, byte b2, byte b3, CancellationToken cancellationToken)
        {
            await WriteRawByteAsync(b1, cancellationToken).ConfigureAwait(false);
            await WriteRawByteAsync(b2, cancellationToken).ConfigureAwait(false);
            await WriteRawByteAsync(b3, cancellationToken).ConfigureAwait(false);
        }

        /// <summary>
        /// Writes the given four-byte tag directly to the stream.
        /// </summary>
        /// <param name="b1">The first byte of the encoded tag</param>
        /// <param name="b2">The second byte of the encoded tag</param>
        /// <param name="b3">The third byte of the encoded tag</param>
        /// <param name="b4">The fourth byte of the encoded tag</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        public async Task WriteRawTagAsync(byte b1, byte b2, byte b3, byte b4, CancellationToken cancellationToken)
        {
            await WriteRawByteAsync(b1, cancellationToken).ConfigureAwait(false);
            await WriteRawByteAsync(b2, cancellationToken).ConfigureAwait(false);
            await WriteRawByteAsync(b3, cancellationToken).ConfigureAwait(false);
            await WriteRawByteAsync(b4, cancellationToken).ConfigureAwait(false);
        }

        /// <summary>
        /// Writes the given five-byte tag directly to the stream.
        /// </summary>
        /// <param name="b1">The first byte of the encoded tag</param>
        /// <param name="b2">The second byte of the encoded tag</param>
        /// <param name="b3">The third byte of the encoded tag</param>
        /// <param name="b4">The fourth byte of the encoded tag</param>
        /// <param name="b5">The fifth byte of the encoded tag</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        public async Task WriteRawTagAsync(byte b1, byte b2, byte b3, byte b4, byte b5, CancellationToken cancellationToken)
        {
            await WriteRawByteAsync(b1, cancellationToken).ConfigureAwait(false);
            await WriteRawByteAsync(b2, cancellationToken).ConfigureAwait(false);
            await WriteRawByteAsync(b3, cancellationToken).ConfigureAwait(false);
            await WriteRawByteAsync(b4, cancellationToken).ConfigureAwait(false);
            await WriteRawByteAsync(b5, cancellationToken).ConfigureAwait(false);
        }
        #endregion

        #region Underlying writing primitives
        /// <summary>
        /// Writes a 32 bit value as a varint. The fast route is taken when
        /// there's enough buffer space left to whizz through without checking
        /// for each byte; otherwise, we resort to calling WriteRawByte each time.
        /// </summary>
        internal async Task WriteRawVarint32Async(uint value, CancellationToken cancellationToken)
        {
            // Optimize for the common case of a single byte value
            if (value < 128 && position < limit)
            {
                buffer[position++] = (byte)value;
                return;
            }

            while (value > 127 && position < limit)
            {
                buffer[position++] = (byte)((value & 0x7F) | 0x80);
                value >>= 7;
            }
            while (value > 127)
            {
                await WriteRawByteAsync((byte)((value & 0x7F) | 0x80), cancellationToken).ConfigureAwait(false);
                value >>= 7;
            }
            if (position < limit)
            {
                buffer[position++] = (byte)value;
            }
            else
            {
                await WriteRawByteAsync((byte)value, cancellationToken).ConfigureAwait(false);
            }
        }

        internal async Task WriteRawVarint64Async(ulong value, CancellationToken cancellationToken)
        {
            while (value > 127 && position < limit)
            {
                buffer[position++] = (byte)((value & 0x7F) | 0x80);
                value >>= 7;
            }
            while (value > 127)
            {
                await WriteRawByteAsync((byte)((value & 0x7F) | 0x80), cancellationToken).ConfigureAwait(false);
                value >>= 7;
            }
            if (position < limit)
            {
                buffer[position++] = (byte)value;
            }
            else
            {
                await WriteRawByteAsync((byte)value, cancellationToken).ConfigureAwait(false);
            }
        }

        internal async Task WriteRawLittleEndian32Async(uint value, CancellationToken cancellationToken)
        {
            if (position + 4 > limit)
            {
                await WriteRawByteAsync((byte)value, cancellationToken).ConfigureAwait(false);
                await WriteRawByteAsync((byte)(value >> 8), cancellationToken).ConfigureAwait(false);
                await WriteRawByteAsync((byte)(value >> 16), cancellationToken).ConfigureAwait(false);
                await WriteRawByteAsync((byte)(value >> 24), cancellationToken).ConfigureAwait(false);
            }
            else
            {
                buffer[position++] = ((byte)value);
                buffer[position++] = ((byte)(value >> 8));
                buffer[position++] = ((byte)(value >> 16));
                buffer[position++] = ((byte)(value >> 24));
            }
        }

        internal async Task WriteRawLittleEndian64Async(ulong value, CancellationToken cancellationToken)
        {
            if (position + 8 > limit)
            {
                await WriteRawByteAsync((byte)value, cancellationToken).ConfigureAwait(false);
                await WriteRawByteAsync((byte)(value >> 8), cancellationToken).ConfigureAwait(false);
                await WriteRawByteAsync((byte)(value >> 16), cancellationToken).ConfigureAwait(false);
                await WriteRawByteAsync((byte)(value >> 24), cancellationToken).ConfigureAwait(false);
                await WriteRawByteAsync((byte)(value >> 32), cancellationToken).ConfigureAwait(false);
                await WriteRawByteAsync((byte)(value >> 40), cancellationToken).ConfigureAwait(false);
                await WriteRawByteAsync((byte)(value >> 48), cancellationToken).ConfigureAwait(false);
                await WriteRawByteAsync((byte)(value >> 56), cancellationToken).ConfigureAwait(false);
            }
            else
            {
                buffer[position++] = ((byte)value);
                buffer[position++] = ((byte)(value >> 8));
                buffer[position++] = ((byte)(value >> 16));
                buffer[position++] = ((byte)(value >> 24));
                buffer[position++] = ((byte)(value >> 32));
                buffer[position++] = ((byte)(value >> 40));
                buffer[position++] = ((byte)(value >> 48));
                buffer[position++] = ((byte)(value >> 56));
            }
        }

        internal async Task WriteRawByteAsync(byte value, CancellationToken cancellationToken)
        {
            if (position == limit)
            {
                await RefreshBufferAsync(cancellationToken).ConfigureAwait(false);
            }

            buffer[position++] = value;
        }

        /// <summary>
        /// Writes out an array of bytes.
        /// </summary>
        internal Task WriteRawBytesAsync(byte[] value, CancellationToken cancellationToken) => WriteRawBytesAsync(value, 0, value.Length, cancellationToken);

        /// <summary>
        /// Writes out part of an array of bytes.
        /// </summary>
        internal async Task WriteRawBytesAsync(byte[] value, int offset, int length, CancellationToken cancellationToken)
        {
            if (limit - position >= length)
            {
                ByteArray.Copy(value, offset, buffer, position, length);
                // We have room in the current buffer.
                position += length;
            }
            else
            {
                // Write extends past current buffer.  Fill the rest of this buffer and
                // flush.
                int bytesWritten = limit - position;
                ByteArray.Copy(value, offset, buffer, position, bytesWritten);
                offset += bytesWritten;
                length -= bytesWritten;
                position = limit;
                await RefreshBufferAsync(cancellationToken).ConfigureAwait(false);

                // Now deal with the rest.
                // Since we have an output stream, this is our buffer
                // and buffer offset == 0
                if (length <= limit)
                {
                    // Fits in new buffer.
                    ByteArray.Copy(value, offset, buffer, 0, length);
                    position = length;
                }
                else
                {
                    // Write is very big.  Let's do it all at once.
                    await output.WriteAsync(value, offset, length, cancellationToken).ConfigureAwait(false);
                }
            }
        }

        #endregion

        private async Task RefreshBufferAsync(CancellationToken cancellationToken)
        {
            if (output == null)
            {
                // We're writing to a single buffer.
                throw new OutOfSpaceException();
            }

            // Since we have an output stream, this is our buffer
            // and buffer offset == 0
            await output.WriteAsync(buffer, 0, position, cancellationToken).ConfigureAwait(false);
            position = 0;
        }

        /// <summary>
        /// Flushes any buffered data to the underlying stream (if there is one).
        /// </summary>
        public async Task FlushAsync(CancellationToken cancellationToken)
        {
            if (output != null)
            {
                await RefreshBufferAsync(cancellationToken).ConfigureAwait(false);
            }
        }
    }
}
#endif