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
using System.Collections.Generic;
using System.IO;
using System.Threading.Tasks;
using System.Threading;

namespace Google.Protobuf
{
    public sealed partial class CodedInputStream : IDisposable
    {
        #region Reading of tags etc

        /// <summary>
        /// Peeks at the next field tag. This is like calling <see cref="ReadTag"/>, but the
        /// tag is not consumed. (So a subsequent call to <see cref="ReadTag"/> will return the
        /// same value.)
        /// </summary>
        public async Task<uint> PeekTagAsync(CancellationToken cancellationToken)
        {
            if (hasNextTag)
            {
                return nextTag;
            }

            uint savedLast = lastTag;
            nextTag = await ReadTagAsync(cancellationToken).ConfigureAwait(false);
            hasNextTag = true;
            lastTag = savedLast; // Undo the side effect of ReadTag
            return nextTag;
        }

        /// <summary>
        /// Reads a field tag, returning the tag of 0 for "end of stream".
        /// </summary>
        /// <remarks>
        /// If this method returns 0, it doesn't necessarily mean the end of all
        /// the data in this CodedInputStream; it may be the end of the logical stream
        /// for an embedded message, for example.
        /// </remarks>
        /// <returns>The next field tag, or 0 for end of stream. (0 is never a valid tag.)</returns>
        public async Task<uint> ReadTagAsync(CancellationToken cancellationToken)
        {
            if (hasNextTag)
            {
                lastTag = nextTag;
                hasNextTag = false;
                return lastTag;
            }

            // Optimize for the incredibly common case of having at least two bytes left in the buffer,
            // and those two bytes being enough to get the tag. This will be true for fields up to 4095.
            if (bufferPos + 2 <= bufferSize)
            {
                int tmp = buffer[bufferPos++];
                if (tmp < 128)
                {
                    lastTag = (uint)tmp;
                }
                else
                {
                    int result = tmp & 0x7f;
                    if ((tmp = buffer[bufferPos++]) < 128)
                    {
                        result |= tmp << 7;
                        lastTag = (uint)result;
                    }
                    else
                    {
                        // Nope, rewind and go the potentially slow route.
                        bufferPos -= 2;
                        lastTag = await ReadRawVarint32Async(cancellationToken).ConfigureAwait(false);
                    }
                }
            }
            else
            {
                if (await IsAtEndAsync(cancellationToken).ConfigureAwait(false))
                {
                    lastTag = 0;
                    return 0; // This is the only case in which we return 0.
                }

                lastTag = await ReadRawVarint32Async(cancellationToken).ConfigureAwait(false);
            }
            if (lastTag == 0)
            {
                // If we actually read zero, that's not a valid tag.
                throw InvalidProtocolBufferException.InvalidTag();
            }
            return lastTag;
        }

        /// <summary>
        /// Skips the data for the field with the tag we've just read.
        /// This should be called directly after <see cref="ReadTag"/>, when
        /// the caller wishes to skip an unknown field.
        /// </summary>
        /// <remarks>
        /// This method throws <see cref="InvalidProtocolBufferException"/> if the last-read tag was an end-group tag.
        /// If a caller wishes to skip a group, they should skip the whole group, by calling this method after reading the
        /// start-group tag. This behavior allows callers to call this method on any field they don't understand, correctly
        /// resulting in an error if an end-group tag has not been paired with an earlier start-group tag.
        /// </remarks>
        /// <exception cref="InvalidProtocolBufferException">The last tag was an end-group tag</exception>
        /// <exception cref="InvalidOperationException">The last read operation read to the end of the logical stream</exception>
        public async Task SkipLastFieldAsync(CancellationToken cancellationToken)
        {
            if (lastTag == 0)
            {
                throw new InvalidOperationException("SkipLastField cannot be called at the end of a stream");
            }
            switch (WireFormat.GetTagWireType(lastTag))
            {
                case WireFormat.WireType.StartGroup:
                    await SkipGroupAsync(lastTag, cancellationToken).ConfigureAwait(false);
                    break;
                case WireFormat.WireType.EndGroup:
                    throw new InvalidProtocolBufferException(
                        "SkipLastField called on an end-group tag, indicating that the corresponding start-group was missing");
                case WireFormat.WireType.Fixed32:
                    await ReadFixed32Async(cancellationToken).ConfigureAwait(false);
                    break;
                case WireFormat.WireType.Fixed64:
                    await ReadFixed64Async(cancellationToken).ConfigureAwait(false);
                    break;
                case WireFormat.WireType.LengthDelimited:
                    var length = await ReadLengthAsync(cancellationToken).ConfigureAwait(false);
                    await SkipRawBytesAsync(length, cancellationToken).ConfigureAwait(false);
                    break;
                case WireFormat.WireType.Varint:
                    await ReadRawVarint32Async(cancellationToken).ConfigureAwait(false);
                    break;
            }
        }

        private async Task SkipGroupAsync(uint startGroupTag, CancellationToken cancellationToken)
        {
            // Note: Currently we expect this to be the way that groups are read. We could put the recursion
            // depth changes into the ReadTag method instead, potentially...
            recursionDepth++;
            if (recursionDepth >= recursionLimit)
            {
                throw InvalidProtocolBufferException.RecursionLimitExceeded();
            }
            uint tag;
            while (true)
            {
                tag = await ReadTagAsync(cancellationToken).ConfigureAwait(false);
                if (tag == 0)
                {
                    throw InvalidProtocolBufferException.TruncatedMessage();
                }
                // Can't call SkipLastField for this case- that would throw.
                if (WireFormat.GetTagWireType(tag) == WireFormat.WireType.EndGroup)
                {
                    break;
                }
                // This recursion will allow us to handle nested groups.
                SkipLastField();
            }
            int startField = WireFormat.GetTagFieldNumber(startGroupTag);
            int endField = WireFormat.GetTagFieldNumber(tag);
            if (startField != endField)
            {
                throw new InvalidProtocolBufferException(
                    $"Mismatched end-group tag. Started with field {startField}; ended with field {endField}");
            }
            recursionDepth--;
        }

        /// <summary>
        /// Reads a double field from the stream.
        /// </summary>
        public async Task<double> ReadDoubleAsync(CancellationToken cancellationToken) => BitConverter.Int64BitsToDouble((long)await ReadRawLittleEndian64Async(cancellationToken).ConfigureAwait(false));

        /// <summary>
        /// Reads a float field from the stream.
        /// </summary>
        public async Task<float> ReadFloatAsync(CancellationToken cancellationToken)
        {
            if (BitConverter.IsLittleEndian && 4 <= bufferSize - bufferPos)
            {
                float ret = BitConverter.ToSingle(buffer, bufferPos);
                bufferPos += 4;
                return ret;
            }
            else
            {
                byte[] rawBytes = await ReadRawBytesAsync(4, cancellationToken).ConfigureAwait(false);
                if (!BitConverter.IsLittleEndian)
                {
                    ByteArray.Reverse(rawBytes);
                }
                return BitConverter.ToSingle(rawBytes, 0);
            }
        }

        /// <summary>
        /// Reads a uint64 field from the stream.
        /// </summary>
        public Task<ulong> ReadUInt64Async(CancellationToken cancellationToken) => ReadRawVarint64Async(cancellationToken);

        /// <summary>
        /// Reads an int64 field from the stream.
        /// </summary>
        public async Task<long> ReadInt64Async(CancellationToken cancellationToken)
        {
            return (long)await ReadRawVarint64Async(cancellationToken).ConfigureAwait(false);
        }

        /// <summary>
        /// Reads an int32 field from the stream.
        /// </summary>
        public async Task<int> ReadInt32Async(CancellationToken cancellationToken)
        {
            return (int)await ReadRawVarint32Async(cancellationToken).ConfigureAwait(false);
        }

        /// <summary>
        /// Reads a fixed64 field from the stream.
        /// </summary>
        public Task<ulong> ReadFixed64Async(CancellationToken cancellationToken) => ReadRawLittleEndian64Async(cancellationToken);

        /// <summary>
        /// Reads a fixed32 field from the stream.
        /// </summary>
        public Task<uint> ReadFixed32Async(CancellationToken cancellationToken) => ReadRawLittleEndian32Async(cancellationToken);

        /// <summary>
        /// Reads a bool field from the stream.
        /// </summary>
        public async Task<bool> ReadBoolAsync(CancellationToken cancellationToken)
        {
            return await ReadRawVarint32Async(cancellationToken).ConfigureAwait(false) != 0;
        }

        /// <summary>
        /// Reads a string field from the stream.
        /// </summary>
        public async Task<string> ReadStringAsync(CancellationToken cancellationToken)
        {
            int length = await ReadLengthAsync(cancellationToken).ConfigureAwait(false);
            // No need to read any data for an empty string.
            if (length == 0)
            {
                return "";
            }
            if (length <= bufferSize - bufferPos)
            {
                // Fast path:  We already have the bytes in a contiguous buffer, so
                //   just copy directly from it.
                String result = CodedOutputStream.Utf8Encoding.GetString(buffer, bufferPos, length);
                bufferPos += length;
                return result;
            }
            // Slow path: Build a byte array first then copy it.
            return CodedOutputStream.Utf8Encoding.GetString(await ReadRawBytesAsync(length, cancellationToken).ConfigureAwait(false), 0, length);
        }

        /// <summary>
        /// Reads an embedded message field value from the stream.
        /// </summary>   
        public async Task ReadMessageAsync(IAsyncMessage builder, CancellationToken cancellationToken)
        {
            int length = await ReadLengthAsync(cancellationToken).ConfigureAwait(false);
            if (recursionDepth >= recursionLimit)
            {
                throw InvalidProtocolBufferException.RecursionLimitExceeded();
            }
            int oldLimit = PushLimit(length);
            ++recursionDepth;

            await builder.MergeFromAsync(this, cancellationToken).ConfigureAwait(false);

            CheckReadEndOfStreamTag();
            // Check that we've read exactly as much data as expected.
            if (!ReachedLimit)
            {
                throw InvalidProtocolBufferException.TruncatedMessage();
            }
            --recursionDepth;
            PopLimit(oldLimit);
        }

        /// <summary>
        /// Reads a bytes field value from the stream.
        /// </summary>   
        public async Task<ByteString> ReadBytesAsync(CancellationToken cancellationToken)
        {
            int length = await ReadLengthAsync(cancellationToken).ConfigureAwait(false);
            if (length <= bufferSize - bufferPos && length > 0)
            {
                // Fast path:  We already have the bytes in a contiguous buffer, so
                //   just copy directly from it.
                ByteString result = ByteString.CopyFrom(buffer, bufferPos, length);
                bufferPos += length;
                return result;
            }
            else
            {
                // Slow path:  Build a byte array and attach it to a new ByteString.
                return ByteString.AttachBytes(await ReadRawBytesAsync(length, cancellationToken).ConfigureAwait(false));
            }
        }

        /// <summary>
        /// Reads a uint32 field value from the stream.
        /// </summary>   
        public Task<uint> ReadUInt32Async(CancellationToken cancellationToken) => ReadRawVarint32Async(cancellationToken);

        /// <summary>
        /// Reads an enum field value from the stream.
        /// </summary>   
        public async Task<int> ReadEnumAsync(CancellationToken cancellationToken)
        {
            // Currently just a pass-through, but it's nice to separate it logically from WriteInt32.
            return (int)await ReadRawVarint32Async(cancellationToken).ConfigureAwait(false);
        }

        /// <summary>
        /// Reads an sfixed32 field value from the stream.
        /// </summary>   
        public async Task<int> ReadSFixed32Async(CancellationToken cancellationToken)
        {
            return (int)await ReadRawLittleEndian32Async(cancellationToken).ConfigureAwait(false);
        }

        /// <summary>
        /// Reads an sfixed64 field value from the stream.
        /// </summary>   
        public async Task<long> ReadSFixed64Async(CancellationToken cancellationToken)
        {
            return (long)await ReadRawLittleEndian64Async(cancellationToken).ConfigureAwait(false);
        }

        /// <summary>
        /// Reads an sint32 field value from the stream.
        /// </summary>   
        public async Task<int> ReadSInt32Async(CancellationToken cancellationToken)
        {
            return DecodeZigZag32(await ReadRawVarint32Async(cancellationToken).ConfigureAwait(false));
        }

        /// <summary>
        /// Reads an sint64 field value from the stream.
        /// </summary>   
        public async Task<long> ReadSInt64Async(CancellationToken cancellationToken)
        {
            return DecodeZigZag64(await ReadRawVarint64Async(cancellationToken).ConfigureAwait(false));
        }

        /// <summary>
        /// Reads a length for length-delimited data.
        /// </summary>
        /// <remarks>
        /// This is internally just reading a varint, but this method exists
        /// to make the calling code clearer.
        /// </remarks>
        public async Task<int> ReadLengthAsync(CancellationToken cancellationToken) => (int)await ReadRawVarint32Async(cancellationToken).ConfigureAwait(false);

        /// <summary>
        /// Peeks at the next tag in the stream. If it matches <paramref name="tag"/>,
        /// the tag is consumed and the method returns <c>true</c>; otherwise, the
        /// stream is left in the original position and the method returns <c>false</c>.
        /// </summary>
        public async Task<bool> MaybeConsumeTagAsync(uint tag, CancellationToken cancellationToken)
        {
            if (await PeekTagAsync(cancellationToken).ConfigureAwait(false) == tag)
            {
                hasNextTag = false;
                return true;
            }
            return false;
        }

        #endregion

        #region Underlying reading primitives

        /// <summary>
        /// Same code as ReadRawVarint32, but read each byte individually, checking for
        /// buffer overflow.
        /// </summary>
        private async Task<uint> SlowReadRawVarint32Async(CancellationToken cancellationToken)
        {
            int tmp = await ReadRawByteAsync(cancellationToken).ConfigureAwait(false);
            if (tmp < 128)
            {
                return (uint)tmp;
            }
            int result = tmp & 0x7f;
            if ((tmp = await ReadRawByteAsync(cancellationToken).ConfigureAwait(false)) < 128)
            {
                result |= tmp << 7;
            }
            else
            {
                result |= (tmp & 0x7f) << 7;
                if ((tmp = await ReadRawByteAsync(cancellationToken).ConfigureAwait(false)) < 128)
                {
                    result |= tmp << 14;
                }
                else
                {
                    result |= (tmp & 0x7f) << 14;
                    if ((tmp = await ReadRawByteAsync(cancellationToken).ConfigureAwait(false)) < 128)
                    {
                        result |= tmp << 21;
                    }
                    else
                    {
                        result |= (tmp & 0x7f) << 21;
                        result |= (tmp = await ReadRawByteAsync(cancellationToken).ConfigureAwait(false)) << 28;
                        if (tmp >= 128)
                        {
                            // Discard upper 32 bits.
                            for (int i = 0; i < 5; i++)
                            {
                                if (await ReadRawByteAsync(cancellationToken).ConfigureAwait(false) < 128)
                                {
                                    return (uint)result;
                                }
                            }
                            throw InvalidProtocolBufferException.MalformedVarint();
                        }
                    }
                }
            }
            return (uint)result;
        }

        /// <summary>
        /// Reads a raw Varint from the stream.  If larger than 32 bits, discard the upper bits.
        /// This method is optimised for the case where we've got lots of data in the buffer.
        /// That means we can check the size just once, then just read directly from the buffer
        /// without constant rechecking of the buffer length.
        /// </summary>
        internal async Task<uint> ReadRawVarint32Async(CancellationToken cancellationToken)
        {
            if (bufferPos + 5 > bufferSize)
            {
                return await SlowReadRawVarint32Async(cancellationToken).ConfigureAwait(false);
            }

            int tmp = buffer[bufferPos++];
            if (tmp < 128)
            {
                return (uint)tmp;
            }
            int result = tmp & 0x7f;
            if ((tmp = buffer[bufferPos++]) < 128)
            {
                result |= tmp << 7;
            }
            else
            {
                result |= (tmp & 0x7f) << 7;
                if ((tmp = buffer[bufferPos++]) < 128)
                {
                    result |= tmp << 14;
                }
                else
                {
                    result |= (tmp & 0x7f) << 14;
                    if ((tmp = buffer[bufferPos++]) < 128)
                    {
                        result |= tmp << 21;
                    }
                    else
                    {
                        result |= (tmp & 0x7f) << 21;
                        result |= (tmp = buffer[bufferPos++]) << 28;
                        if (tmp >= 128)
                        {
                            // Discard upper 32 bits.
                            // Note that this has to use ReadRawByte() as we only ensure we've
                            // got at least 5 bytes at the start of the method. This lets us
                            // use the fast path in more cases, and we rarely hit this section of code.
                            for (int i = 0; i < 5; i++)
                            {
                                if (await ReadRawByteAsync(cancellationToken).ConfigureAwait(false) < 128)
                                {
                                    return (uint)result;
                                }
                            }
                            throw InvalidProtocolBufferException.MalformedVarint();
                        }
                    }
                }
            }
            return (uint)result;
        }

        /// <summary>
        /// Reads a varint from the input one byte at a time, so that it does not
        /// read any bytes after the end of the varint. If you simply wrapped the
        /// stream in a CodedInputStream and used ReadRawVarint32(Stream)
        /// then you would probably end up reading past the end of the varint since
        /// CodedInputStream buffers its input.
        /// </summary>
        /// <param name="input"></param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns></returns>
        internal static async Task<uint> ReadRawVarint32Async(Stream input, CancellationToken cancellationToken)
        {
            int result = 0;
            int offset = 0;
            for (; offset < 32; offset += 7)
            {
                var buff = new byte[1];
                if (await input.ReadAsync(buff, 0, 1, cancellationToken).ConfigureAwait(false) == 0)
                {
                    throw InvalidProtocolBufferException.TruncatedMessage();
                }
                int b = buff[0];
                result |= (b & 0x7f) << offset;
                if ((b & 0x80) == 0)
                {
                    return (uint)result;
                }
            }
            // Keep reading up to 64 bits.
            for (; offset < 64; offset += 7)
            {
                var buff = new byte[1];
                if (await input.ReadAsync(buff, 0, 1, cancellationToken).ConfigureAwait(false) == 0)
                {
                    throw InvalidProtocolBufferException.TruncatedMessage();
                }
                int b = buff[0];
                if ((b & 0x80) == 0)
                {
                    return (uint)result;
                }
            }
            throw InvalidProtocolBufferException.MalformedVarint();
        }

        /// <summary>
        /// Reads a raw varint from the stream.
        /// </summary>
        internal async Task<ulong> ReadRawVarint64Async(CancellationToken cancellationToken)
        {
            int shift = 0;
            ulong result = 0;
            while (shift < 64)
            {
                byte b = await ReadRawByteAsync(cancellationToken).ConfigureAwait(false);
                result |= (ulong)(b & 0x7F) << shift;
                if ((b & 0x80) == 0)
                {
                    return result;
                }
                shift += 7;
            }
            throw InvalidProtocolBufferException.MalformedVarint();
        }

        /// <summary>
        /// Reads a 32-bit little-endian integer from the stream.
        /// </summary>
        internal async Task<uint> ReadRawLittleEndian32Async(CancellationToken cancellationToken)
        {
            uint b1 = await ReadRawByteAsync(cancellationToken).ConfigureAwait(false);
            uint b2 = await ReadRawByteAsync(cancellationToken).ConfigureAwait(false);
            uint b3 = await ReadRawByteAsync(cancellationToken).ConfigureAwait(false);
            uint b4 = await ReadRawByteAsync(cancellationToken).ConfigureAwait(false);
            return b1 | (b2 << 8) | (b3 << 16) | (b4 << 24);
        }

        /// <summary>
        /// Reads a 64-bit little-endian integer from the stream.
        /// </summary>
        internal async Task<ulong> ReadRawLittleEndian64Async(CancellationToken cancellationToken)
        {
            ulong b1 = await ReadRawByteAsync(cancellationToken).ConfigureAwait(false);
            ulong b2 = await ReadRawByteAsync(cancellationToken).ConfigureAwait(false);
            ulong b3 = await ReadRawByteAsync(cancellationToken).ConfigureAwait(false);
            ulong b4 = await ReadRawByteAsync(cancellationToken).ConfigureAwait(false);
            ulong b5 = await ReadRawByteAsync(cancellationToken).ConfigureAwait(false);
            ulong b6 = await ReadRawByteAsync(cancellationToken).ConfigureAwait(false);
            ulong b7 = await ReadRawByteAsync(cancellationToken).ConfigureAwait(false);
            ulong b8 = await ReadRawByteAsync(cancellationToken).ConfigureAwait(false);
            return b1 | (b2 << 8) | (b3 << 16) | (b4 << 24)
                   | (b5 << 32) | (b6 << 40) | (b7 << 48) | (b8 << 56);
        }

        #endregion

        #region Internal reading and buffer management

        /// <summary>
        /// Returns true if the stream has reached the end of the input. This is the
        /// case if either the end of the underlying input source has been reached or
        /// the stream has reached a limit created using PushLimit.
        /// </summary>
        public async Task<bool> IsAtEndAsync(CancellationToken cancellationToken)
        {
            return bufferPos == bufferSize && !await RefillBufferAsync(false, cancellationToken).ConfigureAwait(false);
        }

        /// <summary>
        /// Called when buffer is empty to read more bytes from the
        /// input.  If <paramref name="mustSucceed"/> is true, RefillBuffer() gurantees that
        /// either there will be at least one byte in the buffer when it returns
        /// or it will throw an exception.  If <paramref name="mustSucceed"/> is false,
        /// RefillBuffer() returns false if no more bytes were available.
        /// </summary>
        /// <param name="mustSucceed"></param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns></returns>
        private async Task<bool> RefillBufferAsync(bool mustSucceed, CancellationToken cancellationToken)
        {
            if (bufferPos < bufferSize)
            {
                throw new InvalidOperationException("RefillBuffer() called when buffer wasn't empty.");
            }

            if (totalBytesRetired + bufferSize == currentLimit)
            {
                // Oops, we hit a limit.
                if (mustSucceed)
                {
                    throw InvalidProtocolBufferException.TruncatedMessage();
                }
                else
                {
                    return false;
                }
            }

            totalBytesRetired += bufferSize;

            bufferPos = 0;
            bufferSize = (input == null) ? 0 : await input.ReadAsync(buffer, 0, buffer.Length, cancellationToken).ConfigureAwait(false);
            if (bufferSize < 0)
            {
                throw new InvalidOperationException("Stream.Read returned a negative count");
            }
            if (bufferSize == 0)
            {
                if (mustSucceed)
                {
                    throw InvalidProtocolBufferException.TruncatedMessage();
                }
                else
                {
                    return false;
                }
            }
            else
            {
                RecomputeBufferSizeAfterLimit();
                int totalBytesRead =
                    totalBytesRetired + bufferSize + bufferSizeAfterLimit;
                if (totalBytesRead > sizeLimit || totalBytesRead < 0)
                {
                    throw InvalidProtocolBufferException.SizeLimitExceeded();
                }
                return true;
            }
        }

        /// <summary>
        /// Read one byte from the input.
        /// </summary>
        /// <exception cref="InvalidProtocolBufferException">
        /// the end of the stream or the current limit was reached
        /// </exception>
        internal async Task<byte> ReadRawByteAsync(CancellationToken cancellationToken)
        {
            if (bufferPos == bufferSize)
            {
                await RefillBufferAsync(true, cancellationToken).ConfigureAwait(false);
            }
            return buffer[bufferPos++];
        }

        /// <summary>
        /// Reads a fixed size of bytes from the input.
        /// </summary>
        /// <exception cref="InvalidProtocolBufferException">
        /// the end of the stream or the current limit was reached
        /// </exception>
        internal async Task<byte[]> ReadRawBytesAsync(int size, CancellationToken cancellationToken)
        {
            if (size < 0)
            {
                throw InvalidProtocolBufferException.NegativeSize();
            }

            if (totalBytesRetired + bufferPos + size > currentLimit)
            {
                // Read to the end of the stream (up to the current limit) anyway.
                await SkipRawBytesAsync(currentLimit - totalBytesRetired - bufferPos, cancellationToken).ConfigureAwait(false);
                // Then fail.
                throw InvalidProtocolBufferException.TruncatedMessage();
            }

            if (size <= bufferSize - bufferPos)
            {
                // We have all the bytes we need already.
                byte[] bytes = new byte[size];
                ByteArray.Copy(buffer, bufferPos, bytes, 0, size);
                bufferPos += size;
                return bytes;
            }
            else if (size < buffer.Length)
            {
                // Reading more bytes than are in the buffer, but not an excessive number
                // of bytes.  We can safely allocate the resulting array ahead of time.

                // First copy what we have.
                byte[] bytes = new byte[size];
                int pos = bufferSize - bufferPos;
                ByteArray.Copy(buffer, bufferPos, bytes, 0, pos);
                bufferPos = bufferSize;

                // We want to use RefillBuffer() and then copy from the buffer into our
                // byte array rather than reading directly into our byte array because
                // the input may be unbuffered.
                await RefillBufferAsync(true, cancellationToken).ConfigureAwait(false);

                while (size - pos > bufferSize)
                {
                    Buffer.BlockCopy(buffer, 0, bytes, pos, bufferSize);
                    pos += bufferSize;
                    bufferPos = bufferSize;
                    await RefillBufferAsync(true, cancellationToken).ConfigureAwait(false);
                }

                ByteArray.Copy(buffer, 0, bytes, pos, size - pos);
                bufferPos = size - pos;

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

                // Remember the buffer markers since we'll have to copy the bytes out of
                // it later.
                int originalBufferPos = bufferPos;
                int originalBufferSize = bufferSize;

                // Mark the current buffer consumed.
                totalBytesRetired += bufferSize;
                bufferPos = 0;
                bufferSize = 0;

                // Read all the rest of the bytes we need.
                int sizeLeft = size - (originalBufferSize - originalBufferPos);
                List<byte[]> chunks = new List<byte[]>();

                while (sizeLeft > 0)
                {
                    byte[] chunk = new byte[Math.Min(sizeLeft, buffer.Length)];
                    int pos = 0;
                    while (pos < chunk.Length)
                    {
                        int n = (input == null) ? -1 : await input.ReadAsync(chunk, pos, chunk.Length - pos, cancellationToken).ConfigureAwait(false);
                        if (n <= 0)
                        {
                            throw InvalidProtocolBufferException.TruncatedMessage();
                        }
                        totalBytesRetired += n;
                        pos += n;
                    }
                    sizeLeft -= chunk.Length;
                    chunks.Add(chunk);
                }

                // OK, got everything.  Now concatenate it all into one buffer.
                byte[] bytes = new byte[size];

                // Start by copying the leftover bytes from this.buffer.
                int newPos = originalBufferSize - originalBufferPos;
                ByteArray.Copy(buffer, originalBufferPos, bytes, 0, newPos);

                // And now all the chunks.
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
        private async Task SkipRawBytesAsync(int size, CancellationToken cancellationToken)
        {
            if (size < 0)
            {
                throw InvalidProtocolBufferException.NegativeSize();
            }

            if (totalBytesRetired + bufferPos + size > currentLimit)
            {
                // Read to the end of the stream anyway.
                await SkipRawBytesAsync(currentLimit - totalBytesRetired - bufferPos, cancellationToken).ConfigureAwait(false);
                // Then fail.
                throw InvalidProtocolBufferException.TruncatedMessage();
            }

            if (size <= bufferSize - bufferPos)
            {
                // We have all the bytes we need already.
                bufferPos += size;
            }
            else
            {
                // Skipping more bytes than are in the buffer.  First skip what we have.
                int pos = bufferSize - bufferPos;

                // ROK 5/7/2013 Issue #54: should retire all bytes in buffer (bufferSize)
                // totalBytesRetired += pos;
                totalBytesRetired += bufferSize;

                bufferPos = 0;
                bufferSize = 0;

                // Then skip directly from the InputStream for the rest.
                if (pos < size)
                {
                    if (input == null)
                    {
                        throw InvalidProtocolBufferException.TruncatedMessage();
                    }
                    await SkipImplAsync(size - pos, cancellationToken).ConfigureAwait(false);
                    totalBytesRetired += size - pos;
                }
            }
        }

        /// <summary>
        /// Abstraction of skipping to cope with streams which can't really skip.
        /// </summary>
        private async Task SkipImplAsync(int amountToSkip, CancellationToken cancellationToken)
        {
            if (input.CanSeek)
            {
                long previousPosition = input.Position;
                input.Position += amountToSkip;
                if (input.Position != previousPosition + amountToSkip)
                {
                    throw InvalidProtocolBufferException.TruncatedMessage();
                }
            }
            else
            {
                byte[] skipBuffer = new byte[Math.Min(1024, amountToSkip)];
                while (amountToSkip > 0)
                {
                    int bytesRead = await input.ReadAsync(skipBuffer, 0, Math.Min(skipBuffer.Length, amountToSkip), cancellationToken).ConfigureAwait(false);
                    if (bytesRead <= 0)
                    {
                        throw InvalidProtocolBufferException.TruncatedMessage();
                    }
                    amountToSkip -= bytesRead;
                }
            }
        }

        #endregion
    }
}
#endif