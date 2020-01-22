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
using System.Collections.Generic;
using System.IO;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Security;
using System.Text;
using Google.Protobuf.Collections;

namespace Google.Protobuf
{
    /// <summary>
    /// Reads and decodes protocol message fields.
    /// Note: experimental API that can change or be removed without any prior notice.
    /// </summary>
    /// <remarks>
    /// <para>
    /// This class is generally used by generated code to read appropriate
    /// primitives from the input. It effectively encapsulates the lowest
    /// levels of protocol buffer format.
    /// </para>
    /// <para>
    /// Repeated fields and map fields are not handled by this class; use <see cref="RepeatedField{T}"/>
    /// and <see cref="MapField{TKey, TValue}"/> to serialize such fields.
    /// </para>
    /// </remarks>
    [SecuritySafeCritical]
    public ref struct CodedInputReader
    {
        internal const int DefaultRecursionLimit = 100;

        private SequenceReader<byte> reader;
        private uint lastTag;
        private int recursionDepth;
        private long currentLimit;
        private Decoder decoder;

        private readonly int recursionLimit;


        /// <summary>
        /// Creates a new CodedInputReader reading data from the given <see cref="ReadOnlySequence{T}"/>.
        /// </summary>
        public CodedInputReader(ReadOnlySequence<byte> input) : this(input, DefaultRecursionLimit)
        {
        }

        internal CodedInputReader(ReadOnlySequence<byte> input, int recursionLimit)
        {
            this.reader = new SequenceReader<byte>(input);
            this.lastTag = 0;
            this.recursionDepth = 0;
            this.recursionLimit = recursionLimit;
            this.currentLimit = long.MaxValue;
            this.decoder = null;
            this.DiscardUnknownFields = false;
            this.ExtensionRegistry = null;
        }

        /// <summary>
        /// The total number of bytes processed by the reader.
        /// </summary>
        public long Position => reader.Consumed;

        /// <summary>
        /// Returns true if the reader has reached the end of the input. This is the
        /// case if either the end of the underlying input source has been reached or
        /// the reader has reached a limit created using PushLimit.
        /// </summary>
        public bool IsAtEnd => reader.End;

        /// <summary>
        /// Returns the last tag read, or 0 if no tags have been read or we've read beyond
        /// the end of the input.
        /// </summary>
        internal uint LastTag { get { return lastTag; } }

        /// <summary>
        /// Internal-only property; when set to true, unknown fields will be discarded while parsing.
        /// </summary>
        internal bool DiscardUnknownFields { get; set; }

        /// <summary>
        /// Internal-only property; provides extension identifiers to compatible messages while parsing.
        /// </summary>
        internal ExtensionRegistry ExtensionRegistry { get; set; }

        /// <summary>
        /// Creates a <see cref="CodedInputReader"/> with the specified recursion limits, reading
        /// from the input.
        /// </summary>
        /// <remarks>
        /// This method exists separately from the constructor to reduce the number of constructor overloads.
        /// It is likely to be used considerably less frequently than the constructors, as the default limits
        /// are suitable for most use cases.
        /// </remarks>
        /// <param name="input">The input to read from</param>
        /// <param name="recursionLimit">The maximum recursion depth to allow while reading.</param>
        /// <returns>A <c>CodedInputReader</c> reading from <paramref name="input"/> with the specified limits.</returns>
        public static CodedInputReader CreateWithLimits(ReadOnlySequence<byte> input, int recursionLimit)
        {
            return new CodedInputReader(input, recursionLimit);
        }

        #region Reading of tags etc

        /// <summary>
        /// Peeks at the next field tag. This is like calling <see cref="ReadTag"/>, but the
        /// tag is not consumed. (So a subsequent call to <see cref="ReadTag"/> will return the
        /// same value.)
        /// </summary>
        public uint PeekTag()
        {
            uint previousTag = lastTag;
            long consumed = reader.Consumed;

            uint tag = ReadTag();

            long rewindCount = reader.Consumed - consumed;
            if (rewindCount > 0)
            {
                reader.Rewind(rewindCount);
            }
            lastTag = previousTag;

            return tag;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private void ThrowEndOfInputIfFalse(bool condition)
        {
            if (!condition)
            {
                ThrowEndOfInput();
                return;
            }
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        private static void ThrowEndOfInput()
        {
            throw InvalidProtocolBufferException.TruncatedMessage();
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        private static void ThrowInvalidTagException()
        {
            throw InvalidProtocolBufferException.InvalidTag();
        }

        /// <summary>
        /// Reads a field tag, returning the tag of 0 for "end of input".
        /// </summary>
        /// <remarks>
        /// If this method returns 0, it doesn't necessarily mean the end of all
        /// the data in this CodedInputReader; it may be the end of the logical input
        /// for an embedded message, for example.
        /// </remarks>
        /// <returns>The next field tag, or 0 for end of input. (0 is never a valid tag.)</returns>
        public uint ReadTag()
        {
            if (ReachedLimit)
            {
                lastTag = 0;
                return 0;
            }

            // Optimize for common case of a 2 byte tag that is in the current span
            var current = reader.UnreadSpan;
            if (current.Length >= 2)
            {
                int tmp = current[0];
                if (tmp < 128)
                {
                    lastTag = (uint)tmp;
                    reader.Advance(1);
                }
                else
                {
                    int result = tmp & 0x7f;
                    if ((tmp = current[1]) < 128)
                    {
                        result |= tmp << 7;
                        lastTag = (uint)result;
                        reader.Advance(2);
                    }
                    else
                    {
                        // Nope, go the potentially slow route.
                        lastTag = ReadRawVarint32();
                    }
                }
            }
            else
            {
                if (IsAtEnd)
                {
                    lastTag = 0;
                    return 0;
                }

                lastTag = ReadRawVarint32();
            }

            if (WireFormat.GetTagFieldNumber(lastTag) == 0)
            {
                // If we actually read a tag with a field of 0, that's not a valid tag.
                ThrowInvalidTagException();
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
        /// <exception cref="InvalidOperationException">The last read operation read to the end of the logical input</exception>
        public void SkipLastField()
        {
            if (lastTag == 0)
            {
                throw new InvalidOperationException("SkipLastField cannot be called at the end of a input");
            }
            switch (WireFormat.GetTagWireType(lastTag))
            {
                case WireFormat.WireType.StartGroup:
                    SkipGroup(lastTag);
                    break;
                case WireFormat.WireType.EndGroup:
                    throw new InvalidProtocolBufferException(
                        "SkipLastField called on an end-group tag, indicating that the corresponding start-group was missing");
                case WireFormat.WireType.Fixed32:
                    ReadFixed32();
                    break;
                case WireFormat.WireType.Fixed64:
                    ReadFixed64();
                    break;
                case WireFormat.WireType.LengthDelimited:
                    var length = ReadLength();
                    SkipRawBytes(length);
                    break;
                case WireFormat.WireType.Varint:
                    ReadRawVarint32();
                    break;
            }
        }

        private void SkipRawBytes(int length)
        {
            if (length < 0)
            {
                throw InvalidProtocolBufferException.NegativeSize();
            }

            if (length + reader.Consumed > currentLimit)
            {
                // Read to the end of the limit.
                reader.Advance(currentLimit);
                // Then fail.
                throw InvalidProtocolBufferException.TruncatedMessage();
            }
            if (reader.Remaining < length)
            {
                reader.Advance(reader.Remaining);
                ThrowEndOfInput();
            }

            reader.Advance(length);
        }

        /// <summary>
        /// Skip a group.
        /// </summary>
        internal void SkipGroup(uint startGroupTag)
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
                tag = ReadTag();
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
        /// Reads a double field from the input.
        /// </summary>
        public double ReadDouble()
        {
            return BitConverter.Int64BitsToDouble((long)ReadRawLittleEndian64());
        }

        /// <summary>
        /// Reads a float field from the input.
        /// </summary>
        public float ReadFloat()
        {
            const int length = sizeof(float);

            ReadOnlySpan<byte> current = reader.UnreadSpan;
            if (BitConverter.IsLittleEndian && current.Length >= length)
            {
                // Fast path. All data is in the current span and we're little endian architecture.
                reader.Advance(length);

                // ReadUnaligned uses processor architecture for endianness. Content is little endian and
                // IsLittleEndian has been checked so this is safe to call.
                return Unsafe.ReadUnaligned<float>(ref MemoryMarshal.GetReference(current));
            }
            else
            {
                return ReadFloatSlow();
            }
        }

        private unsafe float ReadFloatSlow()
        {
            const int length = sizeof(float);

            byte* buffer = stackalloc byte[length];
            Span<byte> tempSpan = new Span<byte>(buffer, length);

            ThrowEndOfInputIfFalse(reader.TryCopyTo(tempSpan));
            reader.Advance(length);
            
            // Content is little endian. Reverse if needed to match endianness of architecture.
            if (!BitConverter.IsLittleEndian)
            {
                tempSpan.Reverse();
            }

            // ReadUnaligned uses processor architecture for endianness. Content is little endian and
            // IsLittleEndian has been checked so this is safe to call.
            return Unsafe.ReadUnaligned<float>(ref MemoryMarshal.GetReference(tempSpan));
        }

        /// <summary>
        /// Reads a uint64 field from the input.
        /// </summary>
        public ulong ReadUInt64()
        {
            return ReadRawVarint64();
        }

        /// <summary>
        /// Reads an int64 field from the input.
        /// </summary>
        public long ReadInt64()
        {
            return (long)ReadRawVarint64();
        }

        /// <summary>
        /// Reads an int32 field from the input.
        /// </summary>
        public int ReadInt32()
        {
            return (int)ReadRawVarint32();
        }

        /// <summary>
        /// Reads a fixed64 field from the input.
        /// </summary>
        public ulong ReadFixed64()
        {
            return ReadRawLittleEndian64();
        }

        /// <summary>
        /// Reads a fixed32 field from the input.
        /// </summary>
        public uint ReadFixed32()
        {
            return ReadRawLittleEndian32();
        }

        /// <summary>
        /// Reads a bool field from the input.
        /// </summary>
        public bool ReadBool()
        {
            return ReadRawVarint64() != 0;
        }

        /// <summary>
        /// Reads a string field from the input.
        /// </summary>
        public string ReadString()
        {
            int length = ReadLength();

            if (length == 0)
            {
                return string.Empty;
            }

            if (length < 0)
            {
                throw InvalidProtocolBufferException.NegativeSize();
            }

#if GOOGLE_PROTOBUF_SUPPORT_FAST_STRING
            ReadOnlySpan<byte> unreadSpan = reader.UnreadSpan;
            if (unreadSpan.Length >= length)
            {
                // Fast path: all bytes to decode appear in the same span.
                ReadOnlySpan<byte> data = unreadSpan.Slice(0, length);

                string value;
                unsafe
                {
                    fixed (byte* sourceBytes = &MemoryMarshal.GetReference(data))
                    {
                        value = CodedOutputStream.Utf8Encoding.GetString(sourceBytes, length);
                    }
                }

                reader.Advance(length);
                return value;
            }
#endif

            return ReadStringSlow(length);
        }

        /// <summary>
        /// Reads a string assuming that it is spread across multiple spans in the <see cref="ReadOnlySequence{T}"/>.
        /// </summary>
        /// <param name="byteLength">The length of the string to be decoded, in bytes.</param>
        /// <returns>The decoded string.</returns>
        private string ReadStringSlow(int byteLength)
        {
            ThrowEndOfInputIfFalse(reader.Remaining >= byteLength);

            if (decoder == null)
            {
                decoder = CodedOutputStream.Utf8Encoding.GetDecoder();
            }

            // We need to decode bytes incrementally across multiple spans.
            int maxCharLength = CodedOutputStream.Utf8Encoding.GetMaxCharCount(byteLength);
            char[] charArray = ArrayPool<char>.Shared.Rent(maxCharLength);

            try
            {
                int remainingByteLength = byteLength;
                int initializedChars = 0;
                while (remainingByteLength > 0)
                {
                    int bytesRead = Math.Min(remainingByteLength, reader.UnreadSpan.Length);
                    remainingByteLength -= bytesRead;
                    bool flush = remainingByteLength == 0;

                    unsafe
                    {
                        fixed (byte* pUnreadSpan = &MemoryMarshal.GetReference(reader.UnreadSpan))
                        fixed (char* pCharArray = &charArray[initializedChars])
                        {
                            initializedChars += decoder.GetChars(pUnreadSpan, bytesRead, pCharArray, charArray.Length - initializedChars, flush);
                        }

                        reader.Advance(bytesRead);
                    }
                }

                string value = new string(charArray, 0, initializedChars);
                return value;
            }
            finally
            {
                ArrayPool<char>.Shared.Return(charArray);
            }
        }

        /// <summary>
        /// Reads an embedded message field value from the input.
        /// </summary>   
        public void ReadMessage(IBufferMessage builder)
        {
            int length = ReadLength();
            if (recursionDepth >= recursionLimit)
            {
                throw InvalidProtocolBufferException.RecursionLimitExceeded();
            }
            long oldLimit = PushLimit(length);
            ++recursionDepth;
            builder.MergeFrom(ref this);
            CheckReadEndOfInputTag();
            // Check that we've read exactly as much data as expected.
            if (!ReachedLimit)
            {
                throw InvalidProtocolBufferException.TruncatedMessage();
            }
            --recursionDepth;
            PopLimit(oldLimit);
        }

        /// <summary>
        /// Reads an embedded group field from the input.
        /// </summary>
        public void ReadGroup(IBufferMessage builder)
        {
            if (recursionDepth >= recursionLimit)
            {
                throw InvalidProtocolBufferException.RecursionLimitExceeded();
            }
            ++recursionDepth;
            builder.MergeFrom(ref this);
            --recursionDepth;
        }

        /// <summary>
        /// Reads a bytes field value from the input.
        /// </summary>   
        public ByteString ReadBytes()
        {
            int length = ReadLength();

            if (length == 0)
            {
                return ByteString.Empty;
            }

            ThrowEndOfInputIfFalse(reader.Remaining >= length);

            if (length < 0)
            {
                throw InvalidProtocolBufferException.NegativeSize();
            }

            if (length + reader.Consumed > currentLimit)
            {
                // Read to the end of the limit.
                reader.Advance(currentLimit);
                // Then fail.
                throw InvalidProtocolBufferException.TruncatedMessage();
            }

            // Avoid creating a copy of Sequence if data is on current span
            var data = (reader.UnreadSpan.Length >= length)
                ? reader.UnreadSpan.Slice(0, length).ToArray()
                : reader.Sequence.Slice(reader.Position, length).ToArray();

            reader.Advance(length);

            return ByteString.AttachBytes(data);
        }

        /// <summary>
        /// Reads a uint32 field value from the input.
        /// </summary>   
        public uint ReadUInt32()
        {
            return ReadRawVarint32();
        }

        /// <summary>
        /// Reads an enum field value from the input.
        /// </summary>   
        public int ReadEnum()
        {
            // Currently just a pass-through, but it's nice to separate it logically from WriteInt32.
            return (int)ReadRawVarint32();
        }

        /// <summary>
        /// Reads an sfixed32 field value from the input.
        /// </summary>   
        public int ReadSFixed32()
        {
            return (int)ReadRawLittleEndian32();
        }

        /// <summary>
        /// Reads an sfixed64 field value from the input.
        /// </summary>   
        public long ReadSFixed64()
        {
            return (long)ReadRawLittleEndian64();
        }

        /// <summary>
        /// Reads an sint32 field value from the input.
        /// </summary>   
        public int ReadSInt32()
        {
            return CodedInputStream.DecodeZigZag32(ReadRawVarint32());
        }

        /// <summary>
        /// Reads an sint64 field value from the input.
        /// </summary>   
        public long ReadSInt64()
        {
            return CodedInputStream.DecodeZigZag64(ReadRawVarint64());
        }

        /// <summary>
        /// Reads a length for length-delimited data.
        /// </summary>
        /// <remarks>
        /// This is internally just reading a varint, but this method exists
        /// to make the calling code clearer.
        /// </remarks>
        public int ReadLength()
        {
            return (int)ReadRawVarint32();
        }

        /// <summary>
        /// Peeks at the next tag in the input. If it matches <paramref name="tag"/>,
        /// the tag is consumed and the method returns <c>true</c>; otherwise, the
        /// input is left in the original position and the method returns <c>false</c>.
        /// </summary>
        public bool MaybeConsumeTag(uint tag)
        {
            uint previousTag = lastTag;
            long consumed = reader.Consumed;

            uint newTag = ReadTag();
            if (newTag == tag)
            {
                // Match so consume tag
                return true;
            }

            // No match so rewind
            long rewindCount = reader.Consumed - consumed;
            if (rewindCount > 0)
            {
                reader.Rewind(rewindCount);
            }
            lastTag = previousTag;

            return false;
        }

        internal static float? ReadFloatWrapperLittleEndian(ref CodedInputReader input)
        {
            // length:1 + tag:1 + value:4 = 6 bytes
            const int wrapperLength = 6;

            var remaining = input.reader.UnreadSpan;
            if (remaining.Length >= wrapperLength)
            {
                // The entire wrapper message is already contained in `buffer`.
                int length = remaining[0];
                if (length == 0)
                {
                    input.reader.Advance(1);
                    return 0F;
                }
                // tag:1 + value:4 = length of 5 bytes
                // field=1, type=32-bit = tag of 13
                if (length != wrapperLength - 1 || remaining[1] != 13)
                {
                    return ReadFloatWrapperSlow(ref input);
                }
                // ReadUnaligned uses processor architecture for endianness. Content is little endian and
                // IsLittleEndian has been checked so this is safe to call.
                var result = Unsafe.ReadUnaligned<float>(ref MemoryMarshal.GetReference(remaining.Slice(2)));
                input.reader.Advance(wrapperLength);
                return result;
            }
            else
            {
                return ReadFloatWrapperSlow(ref input);
            }
        }

        internal static float? ReadFloatWrapperSlow(ref CodedInputReader input)
        {
            int length = input.ReadLength();
            if (length == 0)
            {
                return 0F;
            }
            long finalBufferPos = input.reader.Consumed + length;
            float result = 0F;
            do
            {
                // field=1, type=32-bit = tag of 13
                if (input.ReadTag() == 13)
                {
                    result = input.ReadFloat();
                }
                else
                {
                    input.SkipLastField();
                }
            }
            while (input.reader.Consumed < finalBufferPos);
            return result;
        }

        internal static double? ReadDoubleWrapperLittleEndian(ref CodedInputReader input)
        {
            // length:1 + tag:1 + value:8 = 10 bytes
            const int wrapperLength = 10;

            var remaining = input.reader.UnreadSpan;
            if (remaining.Length >= wrapperLength)
            {
                // The entire wrapper message is already contained in `buffer`.
                int length = remaining[0];
                if (length == 0)
                {
                    input.reader.Advance(1);
                    return 0D;
                }
                // tag:1 + value:8 = length of 9 bytes
                // field=1, type=64-bit = tag of 9
                if (length != wrapperLength - 1 || remaining[1] != 9)
                {
                    return ReadDoubleWrapperSlow(ref input);
                }
                // ReadUnaligned uses processor architecture for endianness. Content is little endian and
                // IsLittleEndian has been checked so this is safe to call.
                var result = Unsafe.ReadUnaligned<double>(ref MemoryMarshal.GetReference(remaining.Slice(2)));
                input.reader.Advance(wrapperLength);
                return result;
            }
            else
            {
                return ReadDoubleWrapperSlow(ref input);
            }
        }

        internal static double? ReadDoubleWrapperSlow(ref CodedInputReader input)
        {
            int length = input.ReadLength();
            if (length == 0)
            {
                return 0D;
            }
            long finalBufferPos = input.reader.Consumed + length;
            double result = 0D;
            do
            {
                // field=1, type=64-bit = tag of 9
                if (input.ReadTag() == 9)
                {
                    result = input.ReadDouble();
                }
                else
                {
                    input.SkipLastField();
                }
            }
            while (input.reader.Consumed < finalBufferPos);
            return result;
        }

        internal static bool? ReadBoolWrapper(ref CodedInputReader input)
        {
            return ReadUInt64Wrapper(ref input) != 0;
        }

        internal static uint? ReadUInt32Wrapper(ref CodedInputReader input)
        {
            // length:1 + tag:1 + value:5(varint32-max) = 7 bytes
            const int wrapperLength = 7;

            var remaining = input.reader.UnreadSpan;
            if (remaining.Length >= wrapperLength)
            {
                // The entire wrapper message is already contained in `buffer`.
                int length = remaining[0];
                if (length == 0)
                {
                    input.reader.Advance(1);
                    return 0;
                }
                // Length will always fit in a single byte.
                if (length >= 128)
                {
                    return ReadUInt32WrapperSlow(ref input);
                }
                long finalBufferPos = input.reader.Consumed + length + 1;
                // field=1, type=varint = tag of 8
                if (remaining[1] != 8)
                {
                    return ReadUInt32WrapperSlow(ref input);
                }
                long pos0 = input.reader.Consumed;
                input.reader.Advance(2);
                var result = input.ReadUInt32();
                // Verify this message only contained a single field.
                if (input.reader.Consumed != finalBufferPos)
                {
                    input.reader.Rewind(input.reader.Consumed - pos0);
                    return ReadUInt32WrapperSlow(ref input);
                }
                return result;
            }
            else
            {
                return ReadUInt32WrapperSlow(ref input);
            }
        }

        private static uint? ReadUInt32WrapperSlow(ref CodedInputReader input)
        {
            int length = input.ReadLength();
            if (length == 0)
            {
                return 0;
            }
            long finalBufferPos = input.reader.Consumed + length;
            uint result = 0;
            do
            {
                // field=1, type=varint = tag of 8
                if (input.ReadTag() == 8)
                {
                    result = input.ReadUInt32();
                }
                else
                {
                    input.SkipLastField();
                }
            }
            while (input.reader.Consumed < finalBufferPos);
            return result;
        }

        internal static int? ReadInt32Wrapper(ref CodedInputReader input)
        {
            return (int?)ReadUInt32Wrapper(ref input);
        }

        internal static ulong? ReadUInt64Wrapper(ref CodedInputReader input)
        {
            // field=1, type=varint = tag of 8
            const int expectedTag = 8;
            // length:1 + tag:1 + value:10(varint64-max) = 12 bytes
            const int wrapperLength = 12;

            var remaining = input.reader.UnreadSpan;
            if (remaining.Length >= wrapperLength)
            {
                // The entire wrapper message is already contained in `buffer`.
                int length = remaining[0];
                if (length == 0)
                {
                    input.reader.Advance(1);
                    return 0;
                }
                // Length will always fit in a single byte.
                if (length >= 128)
                {
                    return ReadUInt64WrapperSlow(ref input);
                }
                long finalBufferPos = input.reader.Consumed + length + 1;
                if (remaining[1] != expectedTag)
                {
                    return ReadUInt64WrapperSlow(ref input);
                }
                long pos0 = input.reader.Consumed;
                input.reader.Advance(2);
                var result = input.ReadUInt64();
                // Verify this message only contained a single field.
                if (input.reader.Consumed != finalBufferPos)
                {
                    input.reader.Rewind(input.reader.Consumed - pos0);
                    return ReadUInt64WrapperSlow(ref input);
                }
                return result;
            }
            else
            {
                return ReadUInt64WrapperSlow(ref input);
            }
        }

        internal static ulong? ReadUInt64WrapperSlow(ref CodedInputReader input)
        {
            // field=1, type=varint = tag of 8
            const int expectedTag = 8;

            int length = input.ReadLength();
            if (length == 0)
            {
                return 0L;
            }
            long finalBufferPos = input.reader.Consumed + length;
            ulong result = 0L;
            do
            {
                if (input.ReadTag() == expectedTag)
                {
                    result = input.ReadUInt64();
                }
                else
                {
                    input.SkipLastField();
                }
            }
            while (input.reader.Consumed < finalBufferPos);
            return result;
        }

        internal static long? ReadInt64Wrapper(ref CodedInputReader input)
        {
            return (long?)ReadUInt64Wrapper(ref input);
        }

        #endregion

        #region Underlying reading primitives

        /// <summary>
        /// Same code as ReadRawVarint32, but read each byte from reader individually
        /// </summary>
        private uint SlowReadRawVarint32()
        {
            byte value;

            ThrowEndOfInputIfFalse(reader.TryRead(out value));
            int tmp = value;
            if (tmp < 128)
            {
                return (uint)tmp;
            }
            int result = tmp & 0x7f;
            ThrowEndOfInputIfFalse(reader.TryRead(out value));
            tmp = value;
            if (tmp < 128)
            {
                result |= tmp << 7;
            }
            else
            {
                result |= (tmp & 0x7f) << 7;
                ThrowEndOfInputIfFalse(reader.TryRead(out value));
                tmp = value;
                if (tmp < 128)
                {
                    result |= tmp << 14;
                }
                else
                {
                    result |= (tmp & 0x7f) << 14;
                    ThrowEndOfInputIfFalse(reader.TryRead(out value));
                    tmp = value;
                    if (tmp < 128)
                    {
                        result |= tmp << 21;
                    }
                    else
                    {
                        result |= (tmp & 0x7f) << 21;
                        ThrowEndOfInputIfFalse(reader.TryRead(out value));
                        tmp = value;
                        result |= tmp << 28;
                        if (tmp >= 128)
                        {
                            // Discard upper 32 bits.
                            // Note that this has to use ReadRawByte() as we only ensure we've
                            // got at least 5 bytes at the start of the method. This lets us
                            // use the fast path in more cases, and we rarely hit this section of code.
                            for (int i = 0; i < 5; i++)
                            {
                                ThrowEndOfInputIfFalse(reader.TryRead(out value));
                                tmp = value;
                                if (tmp < 128)
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
        /// Reads a raw Varint from the input. If larger than 32 bits, discard the upper bits.
        /// This method is optimised for the case where we've got lots of data in the buffer.
        /// That means we can check the size just once, then just read directly from the buffer
        /// without constant rechecking of the buffer length.
        /// </summary>
        internal uint ReadRawVarint32()
        {
            var current = reader.UnreadSpan;

            if (current.Length < 5)
            {
                return SlowReadRawVarint32();
            }

            int bufferPos = 0;
            int tmp = current[bufferPos++];
            if (tmp < 128)
            {
                reader.Advance(bufferPos);
                return (uint)tmp;
            }
            int result = tmp & 0x7f;
            if ((tmp = current[bufferPos++]) < 128)
            {
                result |= tmp << 7;
            }
            else
            {
                result |= (tmp & 0x7f) << 7;
                if ((tmp = current[bufferPos++]) < 128)
                {
                    result |= tmp << 14;
                }
                else
                {
                    result |= (tmp & 0x7f) << 14;
                    if ((tmp = current[bufferPos++]) < 128)
                    {
                        result |= tmp << 21;
                    }
                    else
                    {
                        result |= (tmp & 0x7f) << 21;
                        result |= (tmp = current[bufferPos++]) << 28;
                        if (tmp >= 128)
                        {
                            reader.Advance(bufferPos);

                            // Discard upper 32 bits.
                            // Note that this has to use ReadRawByte() as we only ensure we've
                            // got at least 5 bytes at the start of the method. This lets us
                            // use the fast path in more cases, and we rarely hit this section of code.
                            for (int i = 0; i < 5; i++)
                            {
                                ThrowEndOfInputIfFalse(reader.TryRead(out var value));
                                tmp = value;
                                if (tmp < 128)
                                {
                                    return (uint)result;
                                }
                            }
                            throw InvalidProtocolBufferException.MalformedVarint();
                        }
                    }
                }
            }
            reader.Advance(bufferPos);
            return (uint)result;
        }

        /// <summary>
        /// Same code as ReadRawVarint64, but read each byte from reader individually
        /// </summary>
        private ulong SlowReadRawVarint64()
        {
            int shift = 0;
            ulong result = 0;
            while (shift < 64)
            {
                ThrowEndOfInputIfFalse(reader.TryRead(out byte b));
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
        /// Reads a raw varint from the input.
        /// </summary>
        internal ulong ReadRawVarint64()
        {
            var current = reader.UnreadSpan;

            if (current.Length < 10)
            {
                return SlowReadRawVarint64();
            }

            int bufferPos = 0;
            ulong result = current[bufferPos++];
            if (result < 128)
            {
                reader.Advance(bufferPos);
                return result;
            }
            result &= 0x7f;
            int shift = 7;
            do
            {
                byte b = current[bufferPos++];
                result |= (ulong)(b & 0x7F) << shift;
                if (b < 0x80)
                {
                    reader.Advance(bufferPos);
                    return result;
                }
                shift += 7;
            }
            while (shift < 64);

            throw InvalidProtocolBufferException.MalformedVarint();
        }

        /// <summary>
        /// Reads a 32-bit little-endian integer from the input.
        /// </summary>
        internal uint ReadRawLittleEndian32()
        {
            const int length = 4;

            ReadOnlySpan<byte> current = reader.UnreadSpan;
            if (current.Length >= length)
            {
                // Fast path. All data is in the current span.
                reader.Advance(length);

                return BinaryPrimitives.ReadUInt32LittleEndian(current);
            }
            else
            {
                return ReadRawLittleEndian32Slow();
            }
        }

        private unsafe uint ReadRawLittleEndian32Slow()
        {
            const int length = 4;

            byte* buffer = stackalloc byte[length];
            Span<byte> tempSpan = new Span<byte>(buffer, length);

            ThrowEndOfInputIfFalse(reader.TryCopyTo(tempSpan));
            reader.Advance(length);

            return BinaryPrimitives.ReadUInt32LittleEndian(tempSpan);
        }

        /// <summary>
        /// Reads a 64-bit little-endian integer from the input.
        /// </summary>
        internal unsafe ulong ReadRawLittleEndian64()
        {
            const int length = 8;

            ReadOnlySpan<byte> current = reader.UnreadSpan;
            if (current.Length >= length)
            {
                // Fast path. All data is in the current span.
                reader.Advance(length);

                return BinaryPrimitives.ReadUInt64LittleEndian(current);
            }
            else
            {
                return ReadRawLittleEndian64Slow();
            }
        }

        private unsafe ulong ReadRawLittleEndian64Slow()
        {
            const int length = 8;

            byte* buffer = stackalloc byte[length];
            Span<byte> tempSpan = new Span<byte>(buffer, length);

            ThrowEndOfInputIfFalse(reader.TryCopyTo(tempSpan));
            reader.Advance(length);

            return BinaryPrimitives.ReadUInt64LittleEndian(tempSpan);
        }
        #endregion

        /// <summary>
        /// Sets currentLimit to (current position) + byteLimit. This is called
        /// when descending into a length-delimited embedded message. The previous
        /// limit is returned.
        /// </summary>
        /// <returns>The old limit.</returns>
        internal long PushLimit(long byteLimit)
        {
            if (byteLimit < 0)
            {
                throw InvalidProtocolBufferException.NegativeSize();
            }
            
            byteLimit += reader.Consumed;
            long oldLimit = currentLimit;
            if (byteLimit > oldLimit)
            {
                throw InvalidProtocolBufferException.TruncatedMessage();
            }
            currentLimit = byteLimit;

            return oldLimit;
        }

        /// <summary>
        /// Discards the current limit, returning the previous limit.
        /// </summary>
        internal void PopLimit(long oldLimit)
        {
            currentLimit = oldLimit;
        }

        /// <summary>
        /// Returns whether or not all the data before the limit has been read.
        /// </summary>
        /// <returns></returns>
        internal bool ReachedLimit
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                if (currentLimit == long.MaxValue)
                {
                    return false;
                }

                return reader.Consumed >= currentLimit;
            }
        }

        /// <summary>
        /// Verifies that the last call to ReadTag() returned tag 0 - in other words,
        /// we've reached the end of the input when we expected to.
        /// </summary>
        /// <exception cref="InvalidProtocolBufferException">The 
        /// tag read was not the one specified</exception>
        internal void CheckReadEndOfInputTag()
        {
            if (lastTag != 0)
            {
                throw InvalidProtocolBufferException.MoreDataAvailable();
            }
        }
    }
}
#endif