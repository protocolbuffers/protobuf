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

using Google.Protobuf.Collections;
using System;
using System.Buffers;
using System.Buffers.Binary;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Runtime.CompilerServices;
using System.Security;

namespace Google.Protobuf
{
    /// <summary>
    /// Reads and decodes protocol message fields.
    /// </summary>
    /// <remarks>
    /// <para>
    /// This class is generally used by generated code to read appropriate
    /// primitives from the stream. It effectively encapsulates the lowest
    /// levels of protocol buffer format.
    /// </para>
    /// <para>
    /// Repeated fields and map fields are not handled by this class; use <see cref="RepeatedField{T}"/>
    /// and <see cref="MapField{TKey, TValue}"/> to serialize such fields.
    /// </para>
    /// </remarks>
    public sealed class CodedInputStream : IDisposable
    {
        /// <summary>
        /// Whether to leave the underlying stream open when disposing of this stream.
        /// This is always true when there's no stream.
        /// </summary>
        private readonly bool leaveOpen;

        /// <summary>
        /// Buffer of data read from the stream or provided at construction time.
        /// </summary>
        private readonly byte[] buffer;

        /// <summary>
        /// The index of the buffer at which we need to refill from the stream (if there is one).
        /// </summary>
        private int bufferSize;

        private int bufferSizeAfterLimit = 0;
        /// <summary>
        /// The position within the current buffer (i.e. the next byte to read)
        /// </summary>
        private int bufferPos = 0;

        /// <summary>
        /// The stream to read further input from, or null if the byte array buffer was provided
        /// directly on construction, with no further data available.
        /// </summary>
        private readonly Stream input;

        private ReadOnlySequence<byte>.Enumerator nativeInput;
        private bool hasNativeInput;
        private long nativeInputPastBuffersLength;
        private ReadOnlyMemory<byte> currentNativeBuffer;

        /// <summary>
        /// The last tag we read. 0 indicates we've read to the end of the stream
        /// (or haven't read anything yet).
        /// </summary>
        private uint lastTag = 0;

        /// <summary>
        /// The next tag, used to store the value read by PeekTag.
        /// </summary>
        private uint nextTag = 0;
        private bool hasNextTag = false;

        internal const int DefaultRecursionLimit = 64;
        internal const int DefaultSizeLimit = Int32.MaxValue;
        internal const int BufferSize = 4096;

        /// <summary>
        /// The total number of bytes read before the current buffer. The
        /// total bytes read up to the current position can be computed as
        /// totalBytesRetired + bufferPos.
        /// </summary>
        private int totalBytesRetired = 0;

        /// <summary>
        /// The absolute position of the end of the current message.
        /// </summary> 
        private int currentLimit = int.MaxValue;

        private int recursionDepth = 0;

        private readonly int recursionLimit;
        private readonly int sizeLimit;

        #region Construction
        // Note that the checks are performed such that we don't end up checking obviously-valid things
        // like non-null references for arrays we've just created.

        /// <summary>
        /// Creates a new CodedInputStream reading data from the given byte array.
        /// </summary>
        [SecuritySafeCritical]
        public CodedInputStream(byte[] buffer) : this(null, ProtoPreconditions.CheckNotNull(buffer, "buffer"), null, null, 0, buffer.Length, true)
        {
        }

        /// <summary>
        /// Creates a new CodedInputStream reading data from the given contiguous memory segment.
        /// </summary>
        [SecurityCritical]
        public CodedInputStream(ReadOnlyMemory<byte> buffer) : this(null, null, null, buffer, 0, buffer.Length, true)
        {
        }

        /// <summary>
        /// Creates a new CodedInputStream reading data from the given non-contiguous memory sequence.
        /// </summary>
        [SecurityCritical]
        public CodedInputStream(ReadOnlySequence<byte> buffer)
            : this(null, null, buffer.IsSingleSegment ? default(ReadOnlySequence<byte>?) : buffer, buffer.First, 0, buffer.First.Length, true)
        {
        }

        /// <summary>
        /// Creates a new <see cref="CodedInputStream"/> that reads from the given byte array slice.
        /// </summary>
        [SecuritySafeCritical]
        public CodedInputStream(byte[] buffer, int offset, int length)
            : this(null, ProtoPreconditions.CheckNotNull(buffer, "buffer"), null, null, offset, offset + length, true)
        {
            if (offset < 0 || offset > buffer.Length)
            {
                throw new ArgumentOutOfRangeException("offset", "Offset must be within the buffer");
            }
            if (length < 0 || offset + length > buffer.Length)
            {
                throw new ArgumentOutOfRangeException("length", "Length must be non-negative and within the buffer");
            }
        }

        /// <summary>
        /// Creates a new <see cref="CodedInputStream"/> reading data from the given stream, which will be disposed
        /// when the returned object is disposed.
        /// </summary>
        /// <param name="input">The stream to read from.</param>
        [SecuritySafeCritical]
        public CodedInputStream(Stream input) : this(input, false)
        {
        }

        /// <summary>
        /// Creates a new <see cref="CodedInputStream"/> reading data from the given stream.
        /// </summary>
        /// <param name="input">The stream to read from.</param>
        /// <param name="leaveOpen"><c>true</c> to leave <paramref name="input"/> open when the returned
        /// <c cref="CodedInputStream"/> is disposed; <c>false</c> to dispose of the given stream when the
        /// returned object is disposed.</param>
        [SecuritySafeCritical]
        public CodedInputStream(Stream input, bool leaveOpen)
            : this(ProtoPreconditions.CheckNotNull(input, "input"), new byte[BufferSize], null, null, 0, 0, leaveOpen)
        {
        }

        /// <summary>
        /// Creates a new CodedInputStream reading data from the given
        /// stream and buffer, using the default limits.
        /// </summary>
        [SecurityCritical]
        internal CodedInputStream(Stream input, byte[] buffer, ReadOnlySequence<byte>? nativeInput, ReadOnlyMemory<byte>? nativeBuffer, int bufferPos, int bufferSize, bool leaveOpen)
        {
            this.input = input;
            this.buffer = buffer;
            if (nativeInput.HasValue)
            {
                var en = nativeInput.Value.GetEnumerator();
                en.MoveNext();
                this.nativeInput = en;
                hasNativeInput = true;
            }
            if (nativeBuffer.HasValue)
            {
                this.currentNativeBuffer = nativeBuffer.Value;
            }
            this.bufferPos = bufferPos;
            this.bufferSize = bufferSize;
            this.sizeLimit = DefaultSizeLimit;
            this.recursionLimit = DefaultRecursionLimit;
            this.leaveOpen = leaveOpen;
        }

        /// <summary>
        /// Creates a new CodedInputStream reading data from the given
        /// stream and buffer, using the specified limits.
        /// </summary>
        /// <remarks>
        /// This chains to the version with the default limits instead of vice versa to avoid
        /// having to check that the default values are valid every time.
        /// </remarks>
        [SecuritySafeCritical]
        internal CodedInputStream(Stream input, byte[] buffer, int bufferPos, int bufferSize, int sizeLimit, int recursionLimit, bool leaveOpen)
            : this(input, buffer, null, null, bufferPos, bufferSize, leaveOpen)
        {
            if (sizeLimit <= 0)
            {
                throw new ArgumentOutOfRangeException("sizeLimit", "Size limit must be positive");
            }
            if (recursionLimit <= 0)
            {
                throw new ArgumentOutOfRangeException("recursionLimit!", "Recursion limit must be positive");
            }
            this.sizeLimit = sizeLimit;
            this.recursionLimit = recursionLimit;
        }
        #endregion

        /// <summary>
        /// Creates a <see cref="CodedInputStream"/> with the specified size and recursion limits, reading
        /// from an input stream.
        /// </summary>
        /// <remarks>
        /// This method exists separately from the constructor to reduce the number of constructor overloads.
        /// It is likely to be used considerably less frequently than the constructors, as the default limits
        /// are suitable for most use cases.
        /// </remarks>
        /// <param name="input">The input stream to read from</param>
        /// <param name="sizeLimit">The total limit of data to read from the stream.</param>
        /// <param name="recursionLimit">The maximum recursion depth to allow while reading.</param>
        /// <returns>A <c>CodedInputStream</c> reading from <paramref name="input"/> with the specified size
        /// and recursion limits.</returns>
        public static CodedInputStream CreateWithLimits(Stream input, int sizeLimit, int recursionLimit)
        {
            // Note: we may want an overload accepting leaveOpen
            return new CodedInputStream(input, new byte[BufferSize], 0, 0, sizeLimit, recursionLimit, false);
        }

        /// <summary>
        /// Returns the current position in the input stream, or the position in the input buffer
        /// </summary>
        public long Position
        {
            get
            {
                if (input != null)
                {
                    return input.Position - ((bufferSize + bufferSizeAfterLimit) - bufferPos);
                }
                else if (hasNativeInput)
                {
                    return nativeInputPastBuffersLength + bufferPos;
                }
                else
                {
                    return bufferPos;
                }
            }
        }

        /// <summary>
        /// Returns the last tag read, or 0 if no tags have been read or we've read beyond
        /// the end of the stream.
        /// </summary>
        internal uint LastTag { get { return lastTag; } }

        internal ReadOnlySpan<byte> ImmediateBuffer
        {
            [SecurityCritical]
            get => buffer != null ? buffer : currentNativeBuffer.Span;
        }

        /// <summary>
        /// Returns the size limit for this stream.
        /// </summary>
        /// <remarks>
        /// This limit is applied when reading from the underlying stream, as a sanity check. It is
        /// not applied when reading from a byte array data source without an underlying stream.
        /// The default value is Int32.MaxValue.
        /// </remarks>
        /// <value>
        /// The size limit.
        /// </value>
        public int SizeLimit { get { return sizeLimit; } }

        /// <summary>
        /// Returns the recursion limit for this stream. This limit is applied whilst reading messages,
        /// to avoid maliciously-recursive data.
        /// </summary>
        /// <remarks>
        /// The default limit is 64.
        /// </remarks>
        /// <value>
        /// The recursion limit for this stream.
        /// </value>
        public int RecursionLimit { get { return recursionLimit; } }

        /// <summary>
        /// Internal-only property; when set to true, unknown fields will be discarded while parsing.
        /// </summary>
        internal bool DiscardUnknownFields { get; set; }

        /// <summary>
        /// Disposes of this instance, potentially closing any underlying stream.
        /// </summary>
        /// <remarks>
        /// As there is no flushing to perform here, disposing of a <see cref="CodedInputStream"/> which
        /// was constructed with the <c>leaveOpen</c> option parameter set to <c>true</c> (or one which
        /// was constructed to read from a byte array) has no effect.
        /// </remarks>
        public void Dispose()
        {
            if (!leaveOpen)
            {
                input.Dispose();
            }
        }

        #region Validation
        /// <summary>
        /// Verifies that the last call to ReadTag() returned tag 0 - in other words,
        /// we've reached the end of the stream when we expected to.
        /// </summary>
        /// <exception cref="InvalidProtocolBufferException">The 
        /// tag read was not the one specified</exception>
        internal void CheckReadEndOfStreamTag()
        {
            if (lastTag != 0)
            {
                throw InvalidProtocolBufferException.MoreDataAvailable();
            }
        }
        #endregion

        #region Reading of tags etc

        /// <summary>
        /// Peeks at the next field tag. This is like calling <see cref="ReadTag"/>, but the
        /// tag is not consumed. (So a subsequent call to <see cref="ReadTag"/> will return the
        /// same value.)
        /// </summary>
        public uint PeekTag()
        {
            if (hasNextTag)
            {
                return nextTag;
            }

            uint savedLast = lastTag;
            nextTag = ReadTag();
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
        [SecuritySafeCritical]
        public uint ReadTag()
        {
            var immediateBuffer = ImmediateBuffer;
            return ReadTag(ref immediateBuffer);
        }

        /// <summary>
        /// This supports the Protocol Buffers infrastructure and is not meant to be used directly from your code.
        /// </summary>
        [EditorBrowsable(EditorBrowsableState.Never)]
        [SecurityCritical]
        public uint ReadTag(ref ReadOnlySpan<byte> immediateBuffer)
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
                int tmp = immediateBuffer[bufferPos++];
                if (tmp < 128)
                {
                    lastTag = (uint)tmp;
                }
                else
                {
                    int result = tmp & 0x7f;
                    if ((tmp = immediateBuffer[bufferPos++]) < 128)
                    {
                        result |= tmp << 7;
                        lastTag = (uint)result;
                    }
                    else
                    {
                        // Nope, rewind and go the potentially slow route.
                        bufferPos -= 2;
                        lastTag = ReadRawVarint32(ref immediateBuffer);
                    }
                }
            }
            else
            {
                if (IsAtEndCore(ref immediateBuffer))
                {
                    lastTag = 0;
                    return 0; // This is the only case in which we return 0.
                }

                lastTag = ReadRawVarint32(ref immediateBuffer);
            }
            if (WireFormat.GetTagFieldNumber(lastTag) == 0)
            {
                // If we actually read a tag with a field of 0, that's not a valid tag.
                throw InvalidProtocolBufferException.InvalidTag();
            }
            return lastTag;
        }

        /// <summary>
        /// Skips the data for the field with the tag we've just read.
        /// This should be called directly after <see cref="ReadTag()"/>, when
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
        [SecuritySafeCritical]
        public void SkipLastField()
        {
            var immediateBuffer = ImmediateBuffer;
            SkipLastField(ref immediateBuffer);
        }

        /// <summary>
        /// This supports the Protocol Buffers infrastructure and is not meant to be used directly from your code.
        /// </summary>
        [EditorBrowsable(EditorBrowsableState.Never)]
        [SecurityCritical]
        public void SkipLastField(ref ReadOnlySpan<byte> immediateBuffer)
        {
            if (lastTag == 0)
            {
                throw new InvalidOperationException("SkipLastField cannot be called at the end of a stream");
            }
            switch (WireFormat.GetTagWireType(lastTag))
            {
                case WireFormat.WireType.StartGroup:
                    SkipGroup(lastTag, ref immediateBuffer);
                    break;
                case WireFormat.WireType.EndGroup:
                    throw new InvalidProtocolBufferException(
                        "SkipLastField called on an end-group tag, indicating that the corresponding start-group was missing");
                case WireFormat.WireType.Fixed32:
                    ReadFixed32(ref immediateBuffer);
                    break;
                case WireFormat.WireType.Fixed64:
                    ReadFixed64(ref immediateBuffer);
                    break;
                case WireFormat.WireType.LengthDelimited:
                    var length = ReadLength(ref immediateBuffer);
                    SkipRawBytes(length, ref immediateBuffer);
                    break;
                case WireFormat.WireType.Varint:
                    ReadRawVarint32(ref immediateBuffer);
                    break;
            }
        }

        /// <summary>
        /// Skip a group.
        /// </summary>
        internal void SkipGroup(uint startGroupTag)
        {
            var immediateBuffer = ImmediateBuffer;
            SkipGroup(startGroupTag, ref immediateBuffer);
        }

        [EditorBrowsable(EditorBrowsableState.Never)]
        [SecurityCritical]
        internal void SkipGroup(uint startGroupTag, ref ReadOnlySpan<byte> immediateBuffer)
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
                tag = ReadTag(ref immediateBuffer);
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
                SkipLastField(ref immediateBuffer);
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
        [SecuritySafeCritical]
        public double ReadDouble()
        {
            var immediateBuffer = ImmediateBuffer;
            return ReadDouble(ref immediateBuffer);
        }

        /// <summary>
        /// This supports the Protocol Buffers infrastructure and is not meant to be used directly from your code.
        /// </summary>
        [EditorBrowsable(EditorBrowsableState.Never)]
        [SecurityCritical]
        public double ReadDouble(ref ReadOnlySpan<byte> immediateBuffer) => BitConverter.Int64BitsToDouble((long)ReadRawLittleEndian64(ref immediateBuffer));

        /// <summary>
        /// This supports the Protocol Buffers infrastructure and is not meant to be used directly from your code.
        /// </summary>
        [EditorBrowsable(EditorBrowsableState.Never)]
        [SecurityCritical]
        public double ReadWrappedDouble(ref ReadOnlySpan<byte> immediateBuffer)
        {
            int oldLimit = BeginReadNested(ref immediateBuffer);
            uint tag;
            double value = default(double);
            while ((tag = ReadTag(ref immediateBuffer)) != 0)
            {
                if (tag == WellKnownTypes.WrappersReflection.WrapperValueFixed64Tag)
                {
                    value = ReadDouble(ref immediateBuffer);
                }
                else
                {
                    SkipLastField(ref immediateBuffer);
                }
            }
            EndReadNested(oldLimit);

            return value;
        }

        /// <summary>
        /// Reads a float field from the stream.
        /// </summary>
        [SecuritySafeCritical]
        public float ReadFloat()
        {
            var immediateBuffer = ImmediateBuffer;
            return ReadFloat(ref immediateBuffer);
        }

        /// <summary>
        /// This supports the Protocol Buffers infrastructure and is not meant to be used directly from your code.
        /// </summary>
        [EditorBrowsable(EditorBrowsableState.Never)]
        [SecurityCritical]
        //NOTE: We cannot use BitConverter.Int32BitsToSingle because it requires .NET Core 2.1
        public float ReadFloat(ref ReadOnlySpan<byte> immediateBuffer) => Int32BitsToSingleSlow((int)ReadRawLittleEndian32(ref immediateBuffer));

        private static float Int32BitsToSingleSlow(int value) => BitConverter.ToSingle(BitConverter.GetBytes(value), 0);

        /// <summary>
        /// This supports the Protocol Buffers infrastructure and is not meant to be used directly from your code.
        /// </summary>
        [EditorBrowsable(EditorBrowsableState.Never)]
        [SecurityCritical]
        public float ReadWrappedFloat(ref ReadOnlySpan<byte> immediateBuffer)
        {
            int oldLimit = BeginReadNested(ref immediateBuffer);
            uint tag;
            float value = default(float);
            while ((tag = ReadTag(ref immediateBuffer)) != 0)
            {
                if (tag == WellKnownTypes.WrappersReflection.WrapperValueFixed32Tag)
                {
                    value = ReadFloat(ref immediateBuffer);
                }
                else
                {
                    SkipLastField(ref immediateBuffer);
                }
            }
            EndReadNested(oldLimit);

            return value;
        }

        /// <summary>
        /// Reads a uint64 field from the stream.
        /// </summary>
        [SecuritySafeCritical]
        public ulong ReadUInt64()
        {
            var immediateBuffer = ImmediateBuffer;
            return ReadUInt64(ref immediateBuffer);
        }

        /// <summary>
        /// This supports the Protocol Buffers infrastructure and is not meant to be used directly from your code.
        /// </summary>
        [EditorBrowsable(EditorBrowsableState.Never)]
        [SecurityCritical]
        public ulong ReadUInt64(ref ReadOnlySpan<byte> immediateBuffer) => ReadRawVarint64(ref immediateBuffer);

        /// <summary>
        /// This supports the Protocol Buffers infrastructure and is not meant to be used directly from your code.
        /// </summary>
        [EditorBrowsable(EditorBrowsableState.Never)]
        [SecurityCritical]
        public ulong ReadWrappedUInt64(ref ReadOnlySpan<byte> immediateBuffer) => ReadRawWrappedVarint64(ref immediateBuffer);

        /// <summary>
        /// Reads an int64 field from the stream.
        /// </summary>
        [SecuritySafeCritical]
        public long ReadInt64()
        {
            var immediateBuffer = ImmediateBuffer;
            return ReadInt64(ref immediateBuffer);
        }

        /// <summary>
        /// This supports the Protocol Buffers infrastructure and is not meant to be used directly from your code.
        /// </summary>
        [EditorBrowsable(EditorBrowsableState.Never)]
        [SecurityCritical]
        public long ReadInt64(ref ReadOnlySpan<byte> immediateBuffer) => (long)ReadRawVarint64(ref immediateBuffer);

        /// <summary>
        /// This supports the Protocol Buffers infrastructure and is not meant to be used directly from your code.
        /// </summary>
        [EditorBrowsable(EditorBrowsableState.Never)]
        [SecurityCritical]
        public long ReadWrappedInt64(ref ReadOnlySpan<byte> immediateBuffer) => (long)ReadRawWrappedVarint64(ref immediateBuffer);

        /// <summary>
        /// Reads an int32 field from the stream.
        /// </summary>
        [SecuritySafeCritical]
        public int ReadInt32()
        {
            var immediateBuffer = ImmediateBuffer;
            return ReadInt32(ref immediateBuffer);
        }

        /// <summary>
        /// This supports the Protocol Buffers infrastructure and is not meant to be used directly from your code.
        /// </summary>
        [EditorBrowsable(EditorBrowsableState.Never)]
        [SecurityCritical]
        public int ReadInt32(ref ReadOnlySpan<byte> immediateBuffer) => (int)ReadRawVarint32(ref immediateBuffer);

        /// <summary>
        /// This supports the Protocol Buffers infrastructure and is not meant to be used directly from your code.
        /// </summary>
        [EditorBrowsable(EditorBrowsableState.Never)]
        [SecurityCritical]
        public int ReadWrappedInt32(ref ReadOnlySpan<byte> immediateBuffer) => (int)ReadRawWrappedVarint32(ref immediateBuffer);

        /// <summary>
        /// Reads a fixed64 field from the stream.
        /// </summary>
        [SecuritySafeCritical]
        public ulong ReadFixed64()
        {
            var immediateBuffer = ImmediateBuffer;
            return ReadFixed64(ref immediateBuffer);
        }

        /// <summary>
        /// This supports the Protocol Buffers infrastructure and is not meant to be used directly from your code.
        /// </summary>
        [EditorBrowsable(EditorBrowsableState.Never)]
        [SecurityCritical]
        public ulong ReadFixed64(ref ReadOnlySpan<byte> immediateBuffer) => ReadRawLittleEndian64(ref immediateBuffer);

        /// <summary>
        /// Reads a fixed32 field from the stream.
        /// </summary>
        [SecuritySafeCritical]
        public uint ReadFixed32()
        {
            var immediateBuffer = ImmediateBuffer;
            return ReadFixed32(ref immediateBuffer);
        }

        /// <summary>
        /// This supports the Protocol Buffers infrastructure and is not meant to be used directly from your code.
        /// </summary>
        [EditorBrowsable(EditorBrowsableState.Never)]
        [SecurityCritical]
        public uint ReadFixed32(ref ReadOnlySpan<byte> immediateBuffer) => ReadRawLittleEndian32(ref immediateBuffer);

        /// <summary>
        /// Reads a bool field from the stream.
        /// </summary>
        [SecuritySafeCritical]
        public bool ReadBool()
        {
            var immediateBuffer = ImmediateBuffer;
            return ReadBool(ref immediateBuffer);
        }

        /// <summary>
        /// This supports the Protocol Buffers infrastructure and is not meant to be used directly from your code.
        /// </summary>
        [EditorBrowsable(EditorBrowsableState.Never)]
        [SecurityCritical]
        public bool ReadBool(ref ReadOnlySpan<byte> immediateBuffer) => ReadRawVarint32(ref immediateBuffer) != 0;

        /// <summary>
        /// This supports the Protocol Buffers infrastructure and is not meant to be used directly from your code.
        /// </summary>
        [EditorBrowsable(EditorBrowsableState.Never)]
        [SecurityCritical]
        public bool ReadWrappedBool(ref ReadOnlySpan<byte> immediateBuffer) => ReadRawWrappedVarint32(ref immediateBuffer) != 0;

        /// <summary>
        /// Reads a string field from the stream.
        /// </summary>
        [SecuritySafeCritical]
        public string ReadString()
        {
            var immediateBuffer = ImmediateBuffer;
            return ReadString(ref immediateBuffer);
        }

        /// <summary>
        /// This supports the Protocol Buffers infrastructure and is not meant to be used directly from your code.
        /// </summary>
        [EditorBrowsable(EditorBrowsableState.Never)]
        [SecurityCritical]
        public string ReadString(ref ReadOnlySpan<byte> immediateBuffer)
        {
            int length = ReadLength(ref immediateBuffer);
            // No need to read any data for an empty string.
            if (length == 0)
            {
                return "";
            }
            if (length <= bufferSize - bufferPos)
            {
                // Fast path:  We already have the bytes in a contiguous buffer, so
                //   just copy directly from it.
                //NOTE: We cannot use span-based overload because it is available only in .NET Core 2.1
                String result = buffer != null ? CodedOutputStream.Utf8Encoding.GetString(buffer, bufferPos, length) : CodedOutputStream.Utf8Encoding.GetString(immediateBuffer.Slice(bufferPos, length).ToArray(), 0, length);
                bufferPos += length;
                return result;
            }
            // Slow path: Build a byte array first then copy it.
            return CodedOutputStream.Utf8Encoding.GetString(ReadRawBytes(length, ref immediateBuffer), 0, length);
        }

        /// <summary>
        /// This supports the Protocol Buffers infrastructure and is not meant to be used directly from your code.
        /// </summary>
        [EditorBrowsable(EditorBrowsableState.Never)]
        [SecurityCritical]
        public string ReadWrappedString(ref ReadOnlySpan<byte> immediateBuffer)
        {
            int oldLimit = BeginReadNested(ref immediateBuffer);
            uint tag;
            string value = string.Empty;
            while ((tag = ReadTag(ref immediateBuffer)) != 0)
            {
                if (tag == WellKnownTypes.WrappersReflection.WrapperValueLengthDelimitedTag)
                {
                    value = ReadString(ref immediateBuffer);
                }
                else
                {
                    SkipLastField(ref immediateBuffer);
                }
            }
            EndReadNested(oldLimit);

            return value;
        }

        /// <summary>
        /// Reads an embedded message field value from the stream.
        /// </summary>
        [SecuritySafeCritical]
        public void ReadMessage(IMessage builder)
        {
            var immediateBuffer = ImmediateBuffer;
            var oldLimit = BeginReadNested(ref immediateBuffer);
            //NOTE: IMessage does not support MergeFrom with immediateBuffer yet - so we are using legacy path for reverse compat
            builder.MergeFrom(this);
            EndReadNested(oldLimit);
        }

        /// <summary>
        /// This supports the Protocol Buffers infrastructure and is not meant to be used directly from your code.
        /// </summary>
        [EditorBrowsable(EditorBrowsableState.Never)]
        [SecurityCritical]
        public int BeginReadNested(ref ReadOnlySpan<byte> immediateBuffer)
        {
            int length = ReadLength(ref immediateBuffer);
            if (recursionDepth >= recursionLimit)
            {
                throw InvalidProtocolBufferException.RecursionLimitExceeded();
            }
            int oldLimit = PushLimit(length);
            ++recursionDepth;
            return oldLimit;
        }

        /// <summary>
        /// This supports the Protocol Buffers infrastructure and is not meant to be used directly from your code.
        /// </summary>
        [EditorBrowsable(EditorBrowsableState.Never)]
        public void EndReadNested(int oldLimit)
        {
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
        [SecuritySafeCritical]
        public ByteString ReadBytes()
        {
            var immediateBuffer = ImmediateBuffer;
            return ReadBytes(ref immediateBuffer);
        }

        /// <summary>
        /// This supports the Protocol Buffers infrastructure and is not meant to be used directly from your code.
        /// </summary>
        [EditorBrowsable(EditorBrowsableState.Never)]
        [SecurityCritical]
        public ByteString ReadBytes(ref ReadOnlySpan<byte> immediateBuffer)
        {
            int length = ReadLength(ref immediateBuffer);
            if (length <= bufferSize - bufferPos && length > 0)
            {
                // Fast path:  We already have the bytes in a contiguous buffer, so
                //   just copy directly from it.
                ByteString result = ByteString.CopyFrom(immediateBuffer.Slice(bufferPos, length));
                bufferPos += length;
                return result;
            }
            else
            {
                // Slow path:  Build a byte array and attach it to a new ByteString.
                return ByteString.AttachBytes(ReadRawBytes(length, ref immediateBuffer));
            }
        }

        /// <summary>
        /// This supports the Protocol Buffers infrastructure and is not meant to be used directly from your code.
        /// </summary>
        [EditorBrowsable(EditorBrowsableState.Never)]
        [SecurityCritical]
        public ByteString ReadWrappedBytes(ref ReadOnlySpan<byte> immediateBuffer)
        {
            int oldLimit = BeginReadNested(ref immediateBuffer);
            uint tag;
            ByteString value = ByteString.Empty;
            while ((tag = ReadTag(ref immediateBuffer)) != 0)
            {
                if (tag == WellKnownTypes.WrappersReflection.WrapperValueLengthDelimitedTag)
                {
                    value = ReadBytes(ref immediateBuffer);
                }
                else
                {
                    SkipLastField(ref immediateBuffer);
                }
            }
            EndReadNested(oldLimit);

            return value;
        }

        /// <summary>
        /// Reads a uint32 field value from the stream.
        /// </summary>
        [SecuritySafeCritical]
        public uint ReadUInt32()
        {
            var immediateBuffer = ImmediateBuffer;
            return ReadUInt32(ref immediateBuffer);
        }

        /// <summary>
        /// This supports the Protocol Buffers infrastructure and is not meant to be used directly from your code.
        /// </summary>
        [EditorBrowsable(EditorBrowsableState.Never)]
        [SecurityCritical]
        public uint ReadUInt32(ref ReadOnlySpan<byte> immediateBuffer) => ReadRawVarint32(ref immediateBuffer);

        /// <summary>
        /// This supports the Protocol Buffers infrastructure and is not meant to be used directly from your code.
        /// </summary>
        [EditorBrowsable(EditorBrowsableState.Never)]
        [SecurityCritical]
        public uint ReadWrappedUInt32(ref ReadOnlySpan<byte> immediateBuffer) => ReadRawWrappedVarint32(ref immediateBuffer);

        /// <summary>
        /// Reads an enum field value from the stream.
        /// </summary>
        [SecuritySafeCritical]
        public int ReadEnum()
        {
            var immediateBuffer = ImmediateBuffer;
            return ReadEnum(ref immediateBuffer);
        }

        /// <summary>
        /// This supports the Protocol Buffers infrastructure and is not meant to be used directly from your code.
        /// </summary>
        [EditorBrowsable(EditorBrowsableState.Never)]
        [SecurityCritical]
        public int ReadEnum(ref ReadOnlySpan<byte> immediateBuffer) => (int)ReadRawVarint32(ref immediateBuffer);

        /// <summary>
        /// Reads an sfixed32 field value from the stream.
        /// </summary>
        [SecuritySafeCritical]
        public int ReadSFixed32()
        {
            var immediateBuffer = ImmediateBuffer;
            return ReadSFixed32(ref immediateBuffer);
        }

        /// <summary>
        /// This supports the Protocol Buffers infrastructure and is not meant to be used directly from your code.
        /// </summary>
        [EditorBrowsable(EditorBrowsableState.Never)]
        [SecurityCritical]
        public int ReadSFixed32(ref ReadOnlySpan<byte> immediateBuffer) => (int)ReadRawLittleEndian32(ref immediateBuffer);

        /// <summary>
        /// Reads an sfixed64 field value from the stream.
        /// </summary>
        [SecuritySafeCritical]
        public long ReadSFixed64()
        {
            var immediateBuffer = ImmediateBuffer;
            return ReadSFixed64(ref immediateBuffer);
        }

        /// <summary>
        /// This supports the Protocol Buffers infrastructure and is not meant to be used directly from your code.
        /// </summary>
        [EditorBrowsable(EditorBrowsableState.Never)]
        [SecurityCritical]
        public long ReadSFixed64(ref ReadOnlySpan<byte> immediateBuffer) => (long)ReadRawLittleEndian64(ref immediateBuffer);

        /// <summary>
        /// Reads an sint32 field value from the stream.
        /// </summary>
        [SecuritySafeCritical]
        public int ReadSInt32()
        {
            var immediateBuffer = ImmediateBuffer;
            return ReadSInt32(ref immediateBuffer);
        }

        /// <summary>
        /// This supports the Protocol Buffers infrastructure and is not meant to be used directly from your code.
        /// </summary>
        [EditorBrowsable(EditorBrowsableState.Never)]
        [SecurityCritical]
        public int ReadSInt32(ref ReadOnlySpan<byte> immediateBuffer) => DecodeZigZag32(ReadRawVarint32(ref immediateBuffer));

        /// <summary>
        /// Reads an sint64 field value from the stream.
        /// </summary>   
        [SecuritySafeCritical]
        public long ReadSInt64()
        {
            var immediateBuffer = ImmediateBuffer;
            return ReadSInt64(ref immediateBuffer);
        }

        /// <summary>
        /// This supports the Protocol Buffers infrastructure and is not meant to be used directly from your code.
        /// </summary>
        [EditorBrowsable(EditorBrowsableState.Never)]
        [SecurityCritical]
        public long ReadSInt64(ref ReadOnlySpan<byte> immediateBuffer) => DecodeZigZag64(ReadRawVarint64(ref immediateBuffer));

        /// <summary>
        /// Reads a length for length-delimited data.
        /// </summary>
        [SecuritySafeCritical]
        public int ReadLength()
        {
            var immediateBuffer = ImmediateBuffer;
            return ReadLength(ref immediateBuffer);
        }

        /// <summary>
        /// This supports the Protocol Buffers infrastructure and is not meant to be used directly from your code.
        /// </summary>
        [EditorBrowsable(EditorBrowsableState.Never)]
        [SecurityCritical]
        public int ReadLength(ref ReadOnlySpan<byte> immediateBuffer) => (int)ReadRawVarint32(ref immediateBuffer);

        /// <summary>
        /// Peeks at the next tag in the stream. If it matches <paramref name="tag"/>,
        /// the tag is consumed and the method returns <c>true</c>; otherwise, the
        /// stream is left in the original position and the method returns <c>false</c>.
        /// </summary>
        public bool MaybeConsumeTag(uint tag)
        {
            if (PeekTag() == tag)
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
        [SecurityCritical]
        private uint SlowReadRawVarint32(ref ReadOnlySpan<byte> immediateBuffer)
        {
            int tmp = ReadRawByte(ref immediateBuffer);
            if (tmp < 128)
            {
                return (uint)tmp;
            }
            int result = tmp & 0x7f;
            if ((tmp = ReadRawByte(ref immediateBuffer)) < 128)
            {
                result |= tmp << 7;
            }
            else
            {
                result |= (tmp & 0x7f) << 7;
                if ((tmp = ReadRawByte(ref immediateBuffer)) < 128)
                {
                    result |= tmp << 14;
                }
                else
                {
                    result |= (tmp & 0x7f) << 14;
                    if ((tmp = ReadRawByte(ref immediateBuffer)) < 128)
                    {
                        result |= tmp << 21;
                    }
                    else
                    {
                        result |= (tmp & 0x7f) << 21;
                        result |= (tmp = ReadRawByte(ref immediateBuffer)) << 28;
                        if (tmp >= 128)
                        {
                            // Discard upper 32 bits.
                            for (int i = 0; i < 5; i++)
                            {
                                if (ReadRawByte(ref immediateBuffer) < 128)
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

        [SecurityCritical]
        private uint ReadRawWrappedVarint32(ref ReadOnlySpan<byte> immediateBuffer)
        {
            int oldLimit = BeginReadNested(ref immediateBuffer);
            uint tag;
            uint value = default(uint);
            while ((tag = ReadTag(ref immediateBuffer)) != 0)
            {
                if (tag == WellKnownTypes.WrappersReflection.WrapperValueVarintTag)
                {
                    value = ReadRawVarint32(ref immediateBuffer);
                }
                else
                {
                    SkipLastField(ref immediateBuffer);
                }
            }
            EndReadNested(oldLimit);

            return value;
        }

        /// <summary>
        /// Reads a raw Varint from the stream.  If larger than 32 bits, discard the upper bits.
        /// This method is optimised for the case where we've got lots of data in the buffer.
        /// That means we can check the size just once, then just read directly from the buffer
        /// without constant rechecking of the buffer length.
        /// </summary>
        [SecuritySafeCritical]
        internal uint ReadRawVarint32()
        {
            var immediateBuffer = ImmediateBuffer;
            return ReadRawVarint32(ref immediateBuffer);
        }

        /// <summary>
        /// This supports the Protocol Buffers infrastructure and is not meant to be used directly from your code.
        /// </summary>
        [EditorBrowsable(EditorBrowsableState.Never)]
        [SecurityCritical]
        private uint ReadRawVarint32(ref ReadOnlySpan<byte> immediateBuffer)
        {
            if (bufferPos + 5 > bufferSize)
            {
                return SlowReadRawVarint32(ref immediateBuffer);
            }

            int tmp = immediateBuffer[bufferPos++];
            if (tmp < 128)
            {
                return (uint)tmp;
            }
            int result = tmp & 0x7f;
            if ((tmp = immediateBuffer[bufferPos++]) < 128)
            {
                result |= tmp << 7;
            }
            else
            {
                result |= (tmp & 0x7f) << 7;
                if ((tmp = immediateBuffer[bufferPos++]) < 128)
                {
                    result |= tmp << 14;
                }
                else
                {
                    result |= (tmp & 0x7f) << 14;
                    if ((tmp = immediateBuffer[bufferPos++]) < 128)
                    {
                        result |= tmp << 21;
                    }
                    else
                    {
                        result |= (tmp & 0x7f) << 21;
                        result |= (tmp = immediateBuffer[bufferPos++]) << 28;
                        if (tmp >= 128)
                        {
                            // Discard upper 32 bits.
                            // Note that this has to use ReadRawByte() as we only ensure we've
                            // got at least 5 bytes at the start of the method. This lets us
                            // use the fast path in more cases, and we rarely hit this section of code.
                            for (int i = 0; i < 5; i++)
                            {
                                if (ReadRawByte(ref immediateBuffer) < 128)
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
        /// <returns></returns>
        internal static uint ReadRawVarint32(Stream input)
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
                    return (uint)result;
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
                    return (uint)result;
                }
            }
            throw InvalidProtocolBufferException.MalformedVarint();
        }

        /// <summary>
        /// Reads a raw varint from the stream.
        /// </summary>
        [SecuritySafeCritical]
        internal ulong ReadRawVarint64()
        {
            var immediateBuffer = ImmediateBuffer;
            return ReadRawVarint64(ref immediateBuffer);
        }

        /// <summary>
        /// This supports the Protocol Buffers infrastructure and is not meant to be used directly from your code.
        /// </summary>
        [EditorBrowsable(EditorBrowsableState.Never)]
        [SecurityCritical]
        internal ulong ReadRawVarint64(ref ReadOnlySpan<byte> immediateBuffer)
        {
            //TODO: Implement fast path
            int shift = 0;
            ulong result = 0;
            while (shift < 64)
            {
                byte b = ReadRawByte(ref immediateBuffer);
                result |= (ulong)(b & 0x7F) << shift;
                if ((b & 0x80) == 0)
                {
                    return result;
                }
                shift += 7;
            }
            throw InvalidProtocolBufferException.MalformedVarint();
        }

        [SecurityCritical]
        private ulong ReadRawWrappedVarint64(ref ReadOnlySpan<byte> immediateBuffer)
        {
            int oldLimit = BeginReadNested(ref immediateBuffer);
            uint tag;
            ulong value = default(ulong);
            while ((tag = ReadTag(ref immediateBuffer)) != 0)
            {
                if (tag == WellKnownTypes.WrappersReflection.WrapperValueVarintTag)
                {
                    value = ReadRawVarint64(ref immediateBuffer);
                }
                else
                {
                    SkipLastField(ref immediateBuffer);
                }
            }
            EndReadNested(oldLimit);

            return value;
        }

        /// <summary>
        /// Reads a 32-bit little-endian integer from the stream.
        /// </summary>
        [SecuritySafeCritical]
        internal uint ReadRawLittleEndian32()
        {
            var immediateBuffer = ImmediateBuffer;
            return ReadRawLittleEndian32(ref immediateBuffer);
        }

        /// <summary>
        /// This supports the Protocol Buffers infrastructure and is not meant to be used directly from your code.
        /// </summary>
        [EditorBrowsable(EditorBrowsableState.Never)]
        [SecurityCritical]
        private uint ReadRawLittleEndian32(ref ReadOnlySpan<byte> immediateBuffer)
        {
            if (bufferPos + 4 > bufferSize)
            {
                return SlowReadRawLittleEndian32(ref immediateBuffer);
            }
            else
            {
                var ret = BinaryPrimitives.ReadUInt32LittleEndian(immediateBuffer.Slice(bufferPos, 4));
                bufferPos += 4;
                return ret;
            }
        }

        [SecurityCritical]
        private uint SlowReadRawLittleEndian32(ref ReadOnlySpan<byte> immediateBuffer)
        {
            uint b1 = ReadRawByte(ref immediateBuffer);
            uint b2 = ReadRawByte(ref immediateBuffer);
            uint b3 = ReadRawByte(ref immediateBuffer);
            uint b4 = ReadRawByte(ref immediateBuffer);
            return b1 | (b2 << 8) | (b3 << 16) | (b4 << 24);
        }

        /// <summary>
        /// Reads a 64-bit little-endian integer from the stream.
        /// </summary>
        [SecuritySafeCritical]
        public ulong ReadRawLittleEndian64()
        {
            var immediateBuffer = ImmediateBuffer;
            return ReadRawLittleEndian64(ref immediateBuffer);
        }

        /// <summary>
        /// This supports the Protocol Buffers infrastructure and is not meant to be used directly from your code.
        /// </summary>
        [EditorBrowsable(EditorBrowsableState.Never)]
        [SecurityCritical]
        internal ulong ReadRawLittleEndian64(ref ReadOnlySpan<byte> immediateBuffer)
        {
            if (bufferPos + 8 > bufferSize)
            {
                return SlowReadRawLittleEndian64(ref immediateBuffer);
            }
            else
            {
                var ret = BinaryPrimitives.ReadUInt64LittleEndian(immediateBuffer.Slice(bufferPos, 8));
                bufferPos += 8;
                return ret;
            }
        }

        [SecurityCritical]
        private ulong SlowReadRawLittleEndian64(ref ReadOnlySpan<byte> immediateBuffer)
        {
            ulong b1 = ReadRawByte(ref immediateBuffer);
            ulong b2 = ReadRawByte(ref immediateBuffer);
            ulong b3 = ReadRawByte(ref immediateBuffer);
            ulong b4 = ReadRawByte(ref immediateBuffer);
            ulong b5 = ReadRawByte(ref immediateBuffer);
            ulong b6 = ReadRawByte(ref immediateBuffer);
            ulong b7 = ReadRawByte(ref immediateBuffer);
            ulong b8 = ReadRawByte(ref immediateBuffer);
            return b1 | (b2 << 8) | (b3 << 16) | (b4 << 24)
                   | (b5 << 32) | (b6 << 40) | (b7 << 48) | (b8 << 56);
        }

        /// <summary>
        /// Decode a 32-bit value with ZigZag encoding.
        /// </summary>
        /// <remarks>
        /// ZigZag encodes signed integers into values that can be efficiently
        /// encoded with varint.  (Otherwise, negative values must be 
        /// sign-extended to 64 bits to be varint encoded, thus always taking
        /// 10 bytes on the wire.)
        /// </remarks>
        internal static int DecodeZigZag32(uint n)
        {
            return (int)(n >> 1) ^ -(int)(n & 1);
        }

        /// <summary>
        /// Decode a 32-bit value with ZigZag encoding.
        /// </summary>
        /// <remarks>
        /// ZigZag encodes signed integers into values that can be efficiently
        /// encoded with varint.  (Otherwise, negative values must be 
        /// sign-extended to 64 bits to be varint encoded, thus always taking
        /// 10 bytes on the wire.)
        /// </remarks>
        internal static long DecodeZigZag64(ulong n)
        {
            return (long)(n >> 1) ^ -(long)(n & 1);
        }
        #endregion

        #region Internal reading and buffer management

        /// <summary>
        /// Sets currentLimit to (current position) + byteLimit. This is called
        /// when descending into a length-delimited embedded message. The previous
        /// limit is returned.
        /// </summary>
        /// <returns>The old limit.</returns>
        [EditorBrowsable(EditorBrowsableState.Never)]
        public int PushLimit(int byteLimit)
        {
            if (byteLimit < 0)
            {
                throw InvalidProtocolBufferException.NegativeSize();
            }
            byteLimit += totalBytesRetired + bufferPos;
            int oldLimit = currentLimit;
            if (byteLimit > oldLimit)
            {
                throw InvalidProtocolBufferException.TruncatedMessage();
            }
            currentLimit = byteLimit;

            RecomputeBufferSizeAfterLimit();

            return oldLimit;
        }

        private void RecomputeBufferSizeAfterLimit()
        {
            bufferSize += bufferSizeAfterLimit;
            int bufferEnd = totalBytesRetired + bufferSize;
            if (bufferEnd > currentLimit)
            {
                // Limit is in current buffer.
                bufferSizeAfterLimit = bufferEnd - currentLimit;
                bufferSize -= bufferSizeAfterLimit;
            }
            else
            {
                bufferSizeAfterLimit = 0;
            }
        }

        /// <summary>
        /// Discards the current limit, returning the previous limit.
        /// </summary>
        [EditorBrowsable(EditorBrowsableState.Never)]
        public void PopLimit(int oldLimit)
        {
            currentLimit = oldLimit;
            RecomputeBufferSizeAfterLimit();
        }

        /// <summary>
        /// Returns whether or not all the data before the limit has been read.
        /// </summary>
        /// <returns></returns>
        [EditorBrowsable(EditorBrowsableState.Never)]
        public bool ReachedLimit
        {
            get
            {
                if (currentLimit == int.MaxValue)
                {
                    return false;
                }
                int currentAbsolutePosition = totalBytesRetired + bufferPos;
                return currentAbsolutePosition >= currentLimit;
            }
        }

        /// <summary>
        /// Returns true if the stream has reached the end of the input. This is the
        /// case if either the end of the underlying input source has been reached or
        /// the stream has reached a limit created using PushLimit.
        /// </summary>
        public bool IsAtEnd
        {
            [SecuritySafeCritical]
            get
            {
                var immediateBuffer = ImmediateBuffer;
                return IsAtEndCore(ref immediateBuffer);
            }
        }

        /// <summary>
        /// This supports the Protocol Buffers infrastructure and is not meant to be used directly from your code.
        /// </summary>
        [SecurityCritical]
        internal bool IsAtEndCore(ref ReadOnlySpan<byte> immediateBuffer) => bufferPos == bufferSize && (input == null && !hasNativeInput || !RefillBuffer(false, ref immediateBuffer));

        /// <summary>
        /// Called when buffer is empty to read more bytes from the
        /// input.  If mustSucceed is true, RefillBuffer() gurantees that
        /// either there will be at least one byte in the buffer when it returns
        /// or it will throw an exception.  If mustSucceed is false,
        /// RefillBuffer() returns false if no more bytes were available.
        /// </summary>
        [SecurityCritical]
        private bool RefillBuffer(bool mustSucceed, ref ReadOnlySpan<byte> immediateBuffer)
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
            if (input != null)
            {
                bufferSize = input.Read(buffer, 0, buffer.Length);
            }
            else if (hasNativeInput)
            {
                nativeInputPastBuffersLength += bufferSize;
                if (nativeInput.MoveNext())
                {
                    currentNativeBuffer = nativeInput.Current;
                    immediateBuffer = nativeInput.Current.Span;
                    bufferSize = immediateBuffer.Length;
                }
                else
                {
                    bufferSize = 0;
                }
            }
            else
            {
                bufferSize = 0;
            }

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
                if (totalBytesRead < 0 || totalBytesRead > sizeLimit)
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
        [SecuritySafeCritical]
        internal byte ReadRawByte()
        {
            var immediateBuffer = ImmediateBuffer;
            return ReadRawByte(ref immediateBuffer);
        }

        /// <summary>
        /// This supports the Protocol Buffers infrastructure and is not meant to be used directly from your code.
        /// </summary>
        [EditorBrowsable(EditorBrowsableState.Never)]
        [SecurityCritical]
        private byte ReadRawByte(ref ReadOnlySpan<byte> immediateBuffer)
        {
            if (bufferPos == bufferSize)
            {
                RefillBuffer(true, ref immediateBuffer);
            }
            return immediateBuffer[bufferPos++];
        }

        /// <summary>
        /// Reads a fixed size of bytes from the input.
        /// </summary>
        /// <exception cref="InvalidProtocolBufferException">
        /// the end of the stream or the current limit was reached
        /// </exception>
        [SecuritySafeCritical]
        internal byte[] ReadRawBytes(int size)
        {
            var immediateBuffer = ImmediateBuffer;
            return ReadRawBytes(size, ref immediateBuffer);
        }

        /// <summary>
        /// This supports the Protocol Buffers infrastructure and is not meant to be used directly from your code.
        /// </summary>
        [EditorBrowsable(EditorBrowsableState.Never)]
        [SecurityCritical]
        private byte[] ReadRawBytes(int size, ref ReadOnlySpan<byte> immediateBuffer)
        {
            if (size < 0)
            {
                throw InvalidProtocolBufferException.NegativeSize();
            }

            if (totalBytesRetired + bufferPos + size > currentLimit)
            {
                // Read to the end of the stream (up to the current limit) anyway.
                SkipRawBytes(currentLimit - totalBytesRetired - bufferPos, ref immediateBuffer);
                // Then fail.
                throw InvalidProtocolBufferException.TruncatedMessage();
            }

            if (size <= bufferSize - bufferPos)
            {
                // We have all the bytes we need already.
                byte[] bytes = new byte[size];
                immediateBuffer.Slice(bufferPos, size).CopyTo(bytes);
                bufferPos += size;
                return bytes;
            }
            else if (size < immediateBuffer.Length || hasNativeInput)
            {
                // Reading more bytes than are in the buffer, but not an excessive number
                // of bytes or all the bytes are already in a contiguous buffer.
                // We can safely allocate the resulting array ahead of time.

                // First copy what we have.
                byte[] bytes = new byte[size];
                int pos = bufferSize - bufferPos;
                immediateBuffer.Slice(bufferPos, pos).CopyTo(bytes);
                bufferPos = bufferSize;

                // We want to use RefillBuffer() and then copy from the buffer into our
                // byte array rather than reading directly into our byte array because
                // the input may be unbuffered.
                RefillBuffer(true, ref immediateBuffer);

                while (size - pos > bufferSize)
                {
                    immediateBuffer.Slice(0, bufferSize).CopyTo(bytes.AsSpan(pos, bufferSize));
                    pos += bufferSize;
                    bufferPos = bufferSize;
                    RefillBuffer(true, ref immediateBuffer);
                }

                immediateBuffer.Slice(0, size - pos).CopyTo(bytes.AsSpan(pos));
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
                    byte[] chunk = new byte[Math.Min(sizeLeft, immediateBuffer.Length)];
                    int pos = 0;
                    while (pos < chunk.Length)
                    {
                        int n = (input == null) ? -1 : input.Read(chunk, pos, chunk.Length - pos);
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
                immediateBuffer.Slice(originalBufferPos, newPos).CopyTo(bytes);

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
        [SecurityCritical]
        private void SkipRawBytes(int size, ref ReadOnlySpan<byte> immediateBuffer)
        {
            if (size < 0)
            {
                throw InvalidProtocolBufferException.NegativeSize();
            }

            if (totalBytesRetired + bufferPos + size > currentLimit)
            {
                // Read to the end of the stream anyway.
                SkipRawBytes(currentLimit - totalBytesRetired - bufferPos, ref immediateBuffer);
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
                    if (input == null && !hasNativeInput)
                    {
                        throw InvalidProtocolBufferException.TruncatedMessage();
                    }
                    SkipImpl(size - pos, ref immediateBuffer);
                    totalBytesRetired += size - pos;
                }
            }
        }

        /// <summary>
        /// Abstraction of skipping to cope with streams which can't really skip.
        /// </summary>
        [SecuritySafeCritical]
        private void SkipImpl(int amountToSkip, ref ReadOnlySpan<byte> immediateBuffer)
        {
            if (hasNativeInput)
            {
                while (amountToSkip > 0)
                {
                    if (!nativeInput.MoveNext())
                    {
                        throw InvalidProtocolBufferException.TruncatedMessage();
                    }
                    currentNativeBuffer = nativeInput.Current;
                    immediateBuffer = nativeInput.Current.Span;
                    bufferSize = immediateBuffer.Length;
                    bufferPos = 0;
                    if (bufferSize < amountToSkip)
                    {
                        bufferPos = bufferSize;
                        amountToSkip -= bufferSize;
                    }
                    else
                    {
                        bufferPos += amountToSkip;
                        amountToSkip = 0;
                    }
                }
            }
            else if (input.CanSeek)
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
                    int bytesRead = input.Read(skipBuffer, 0, Math.Min(skipBuffer.Length, amountToSkip));
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