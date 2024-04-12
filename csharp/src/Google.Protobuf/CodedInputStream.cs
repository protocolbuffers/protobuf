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
using System.IO;
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
    [SecuritySafeCritical]
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
        /// The stream to read further input from, or null if the byte array buffer was provided
        /// directly on construction, with no further data available.
        /// </summary>
        private readonly Stream input;

        /// <summary>
        /// The parser state is kept separately so that other parse implementations can reuse the same
        /// parsing primitives.
        /// </summary>
        private ParserInternalState state;

        internal const int DefaultRecursionLimit = 100;
        internal const int DefaultSizeLimit = Int32.MaxValue;
        internal const int BufferSize = 4096;

        #region Construction
        // Note that the checks are performed such that we don't end up checking obviously-valid things
        // like non-null references for arrays we've just created.

        /// <summary>
        /// Creates a new CodedInputStream reading data from the given byte array.
        /// </summary>
        public CodedInputStream(byte[] buffer) : this(null, ProtoPreconditions.CheckNotNull(buffer, "buffer"), 0, buffer.Length, true)
        {            
        }

        /// <summary>
        /// Creates a new <see cref="CodedInputStream"/> that reads from the given byte array slice.
        /// </summary>
        public CodedInputStream(byte[] buffer, int offset, int length)
            : this(null, ProtoPreconditions.CheckNotNull(buffer, "buffer"), offset, offset + length, true)
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
        public CodedInputStream(Stream input, bool leaveOpen)
            : this(ProtoPreconditions.CheckNotNull(input, "input"), new byte[BufferSize], 0, 0, leaveOpen)
        {
        }
        
        /// <summary>
        /// Creates a new CodedInputStream reading data from the given
        /// stream and buffer, using the default limits.
        /// </summary>
        internal CodedInputStream(Stream input, byte[] buffer, int bufferPos, int bufferSize, bool leaveOpen)
        {
            this.input = input;
            this.buffer = buffer;
            this.state.bufferPos = bufferPos;
            this.state.bufferSize = bufferSize;
            this.state.sizeLimit = DefaultSizeLimit;
            this.state.recursionLimit = DefaultRecursionLimit;
            SegmentedBufferHelper.Initialize(this, out this.state.segmentedBufferHelper);
            this.leaveOpen = leaveOpen;

            this.state.currentLimit = int.MaxValue;
        }

        /// <summary>
        /// Creates a new CodedInputStream reading data from the given
        /// stream and buffer, using the specified limits.
        /// </summary>
        /// <remarks>
        /// This chains to the version with the default limits instead of vice versa to avoid
        /// having to check that the default values are valid every time.
        /// </remarks>
        internal CodedInputStream(Stream input, byte[] buffer, int bufferPos, int bufferSize, int sizeLimit, int recursionLimit, bool leaveOpen)
            : this(input, buffer, bufferPos, bufferSize, leaveOpen)
        {
            if (sizeLimit <= 0)
            {
                throw new ArgumentOutOfRangeException("sizeLimit", "Size limit must be positive");
            }
            if (recursionLimit <= 0)
            {
                throw new ArgumentOutOfRangeException("recursionLimit!", "Recursion limit must be positive");
            }
            this.state.sizeLimit = sizeLimit;
            this.state.recursionLimit = recursionLimit;
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
                    return input.Position - ((state.bufferSize + state.bufferSizeAfterLimit) - state.bufferPos);
                }
                return state.bufferPos;
            }
        }

        /// <summary>
        /// Returns the last tag read, or 0 if no tags have been read or we've read beyond
        /// the end of the stream.
        /// </summary>
        internal uint LastTag { get { return state.lastTag; } }

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
        public int SizeLimit { get { return state.sizeLimit; } }

        /// <summary>
        /// Returns the recursion limit for this stream. This limit is applied whilst reading messages,
        /// to avoid maliciously-recursive data.
        /// </summary>
        /// <remarks>
        /// The default limit is 100.
        /// </remarks>
        /// <value>
        /// The recursion limit for this stream.
        /// </value>
        public int RecursionLimit { get { return state.recursionLimit; } }

        /// <summary>
        /// Internal-only property; when set to true, unknown fields will be discarded while parsing.
        /// </summary>
        internal bool DiscardUnknownFields
        {
            get { return state.DiscardUnknownFields; }
            set { state.DiscardUnknownFields = value; }
        }

        /// <summary>
        /// Internal-only property; provides extension identifiers to compatible messages while parsing.
        /// </summary>
        internal ExtensionRegistry ExtensionRegistry
        {
            get { return state.ExtensionRegistry; }
            set { state.ExtensionRegistry = value; }
        }

        internal byte[] InternalBuffer => buffer;

        internal Stream InternalInputStream => input;

        internal ref ParserInternalState InternalState => ref state;

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
            ParsingPrimitivesMessages.CheckReadEndOfStreamTag(ref state);
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
            var span = new ReadOnlySpan<byte>(buffer);
            return ParsingPrimitives.PeekTag(ref span, ref state);
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
        public uint ReadTag()
        {
            var span = new ReadOnlySpan<byte>(buffer);
            return ParsingPrimitives.ParseTag(ref span, ref state);
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
        public void SkipLastField()
        {
            var span = new ReadOnlySpan<byte>(buffer);
            ParsingPrimitivesMessages.SkipLastField(ref span, ref state);
        }

        /// <summary>
        /// Skip a group.
        /// </summary>
        internal void SkipGroup(uint startGroupTag)
        {
            var span = new ReadOnlySpan<byte>(buffer);
            ParsingPrimitivesMessages.SkipGroup(ref span, ref state, startGroupTag);
        }

        /// <summary>
        /// Reads a double field from the stream.
        /// </summary>
        public double ReadDouble()
        {
            var span = new ReadOnlySpan<byte>(buffer);
            return ParsingPrimitives.ParseDouble(ref span, ref state);
        }

        /// <summary>
        /// Reads a float field from the stream.
        /// </summary>
        public float ReadFloat()
        {
            var span = new ReadOnlySpan<byte>(buffer);
            return ParsingPrimitives.ParseFloat(ref span, ref state);
        }

        /// <summary>
        /// Reads a uint64 field from the stream.
        /// </summary>
        public ulong ReadUInt64()
        {
            return ReadRawVarint64();
        }

        /// <summary>
        /// Reads an int64 field from the stream.
        /// </summary>
        public long ReadInt64()
        {
            return (long) ReadRawVarint64();
        }

        /// <summary>
        /// Reads an int32 field from the stream.
        /// </summary>
        public int ReadInt32()
        {
            return (int) ReadRawVarint32();
        }

        /// <summary>
        /// Reads a fixed64 field from the stream.
        /// </summary>
        public ulong ReadFixed64()
        {
            return ReadRawLittleEndian64();
        }

        /// <summary>
        /// Reads a fixed32 field from the stream.
        /// </summary>
        public uint ReadFixed32()
        {
            return ReadRawLittleEndian32();
        }

        /// <summary>
        /// Reads a bool field from the stream.
        /// </summary>
        public bool ReadBool()
        {
            return ReadRawVarint64() != 0;
        }

        /// <summary>
        /// Reads a string field from the stream.
        /// </summary>
        public string ReadString()
        {
            var span = new ReadOnlySpan<byte>(buffer);
            return ParsingPrimitives.ReadString(ref span, ref state);
        }

        /// <summary>
        /// Reads an embedded message field value from the stream.
        /// </summary>
        public void ReadMessage(IMessage builder)
        {
            // TODO(jtattermusch): if the message doesn't implement IBufferMessage (and thus does not provide the InternalMergeFrom method),
            // what we're doing here works fine, but could be more efficient.
            // What happens is that we first initialize a ParseContext from the current coded input stream only to parse the length of the message, at which point
            // we will need to switch back again to CodedInputStream-based parsing (which involves copying and storing the state) to be able to
            // invoke the legacy MergeFrom(CodedInputStream) method.
            // For now, this inefficiency is fine, considering this is only a backward-compatibility scenario (and regenerating the code fixes it).
            ParseContext.Initialize(buffer.AsSpan(), ref state, out ParseContext ctx);
            try
            {
                ParsingPrimitivesMessages.ReadMessage(ref ctx, builder);
            }
            finally
            {
                ctx.CopyStateTo(this);
            }
        }

        /// <summary>
        /// Reads an embedded group field from the stream.
        /// </summary>
        public void ReadGroup(IMessage builder)
        {
            ParseContext.Initialize(this, out ParseContext ctx);
            try
            {
                ParsingPrimitivesMessages.ReadGroup(ref ctx, builder);
            }
            finally
            {
                ctx.CopyStateTo(this);
            }
        }

        /// <summary>
        /// Reads a bytes field value from the stream.
        /// </summary>   
        public ByteString ReadBytes()
        {
            var span = new ReadOnlySpan<byte>(buffer);
            return ParsingPrimitives.ReadBytes(ref span, ref state);
        }

        /// <summary>
        /// Reads a uint32 field value from the stream.
        /// </summary>   
        public uint ReadUInt32()
        {
            return ReadRawVarint32();
        }

        /// <summary>
        /// Reads an enum field value from the stream.
        /// </summary>   
        public int ReadEnum()
        {
            // Currently just a pass-through, but it's nice to separate it logically from WriteInt32.
            return (int) ReadRawVarint32();
        }

        /// <summary>
        /// Reads an sfixed32 field value from the stream.
        /// </summary>   
        public int ReadSFixed32()
        {
            return (int) ReadRawLittleEndian32();
        }

        /// <summary>
        /// Reads an sfixed64 field value from the stream.
        /// </summary>   
        public long ReadSFixed64()
        {
            return (long) ReadRawLittleEndian64();
        }

        /// <summary>
        /// Reads an sint32 field value from the stream.
        /// </summary>   
        public int ReadSInt32()
        {
            return ParsingPrimitives.DecodeZigZag32(ReadRawVarint32());
        }

        /// <summary>
        /// Reads an sint64 field value from the stream.
        /// </summary>   
        public long ReadSInt64()
        {
            return ParsingPrimitives.DecodeZigZag64(ReadRawVarint64());
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
            var span = new ReadOnlySpan<byte>(buffer);
            return ParsingPrimitives.ParseLength(ref span, ref state);
        }

        /// <summary>
        /// Peeks at the next tag in the stream. If it matches <paramref name="tag"/>,
        /// the tag is consumed and the method returns <c>true</c>; otherwise, the
        /// stream is left in the original position and the method returns <c>false</c>.
        /// </summary>
        public bool MaybeConsumeTag(uint tag)
        {
            var span = new ReadOnlySpan<byte>(buffer);
            return ParsingPrimitives.MaybeConsumeTag(ref span, ref state, tag);
        }

#endregion

        #region Underlying reading primitives

        /// <summary>
        /// Reads a raw Varint from the stream.  If larger than 32 bits, discard the upper bits.
        /// This method is optimised for the case where we've got lots of data in the buffer.
        /// That means we can check the size just once, then just read directly from the buffer
        /// without constant rechecking of the buffer length.
        /// </summary>
        internal uint ReadRawVarint32()
        {
            var span = new ReadOnlySpan<byte>(buffer);
            return ParsingPrimitives.ParseRawVarint32(ref span, ref state);
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
            return ParsingPrimitives.ReadRawVarint32(input);
        }

        /// <summary>
        /// Reads a raw varint from the stream.
        /// </summary>
        internal ulong ReadRawVarint64()
        {
            var span = new ReadOnlySpan<byte>(buffer);
            return ParsingPrimitives.ParseRawVarint64(ref span, ref state);
        }

        /// <summary>
        /// Reads a 32-bit little-endian integer from the stream.
        /// </summary>
        internal uint ReadRawLittleEndian32()
        {
            var span = new ReadOnlySpan<byte>(buffer);
            return ParsingPrimitives.ParseRawLittleEndian32(ref span, ref state);
        }

        /// <summary>
        /// Reads a 64-bit little-endian integer from the stream.
        /// </summary>
        internal ulong ReadRawLittleEndian64()
        {
            var span = new ReadOnlySpan<byte>(buffer);
            return ParsingPrimitives.ParseRawLittleEndian64(ref span, ref state);
        }
        #endregion

        #region Internal reading and buffer management

        /// <summary>
        /// Sets currentLimit to (current position) + byteLimit. This is called
        /// when descending into a length-delimited embedded message. The previous
        /// limit is returned.
        /// </summary>
        /// <returns>The old limit.</returns>
        internal int PushLimit(int byteLimit)
        {
            return SegmentedBufferHelper.PushLimit(ref state, byteLimit);
        }

        /// <summary>
        /// Discards the current limit, returning the previous limit.
        /// </summary>
        internal void PopLimit(int oldLimit)
        {
            SegmentedBufferHelper.PopLimit(ref state, oldLimit);
        }

        /// <summary>
        /// Returns whether or not all the data before the limit has been read.
        /// </summary>
        /// <returns></returns>
        internal bool ReachedLimit
        {
            get
            {
                return SegmentedBufferHelper.IsReachedLimit(ref state);
            }
        }

        /// <summary>
        /// Returns true if the stream has reached the end of the input. This is the
        /// case if either the end of the underlying input source has been reached or
        /// the stream has reached a limit created using PushLimit.
        /// </summary>
        public bool IsAtEnd
        {
            get
            {
                var span = new ReadOnlySpan<byte>(buffer);
                return SegmentedBufferHelper.IsAtEnd(ref span, ref state);
            }
        }

        /// <summary>
        /// Reads a fixed size of bytes from the input.
        /// </summary>
        /// <exception cref="InvalidProtocolBufferException">
        /// the end of the stream or the current limit was reached
        /// </exception>
        internal byte[] ReadRawBytes(int size)
        {
            var span = new ReadOnlySpan<byte>(buffer);
            return ParsingPrimitives.ReadRawBytes(ref span, ref state, size);
        }

        /// <summary>
        /// Reads a top-level message or a nested message after the limits for this message have been pushed.
        /// (parser will proceed until the end of the current limit)
        /// NOTE: this method needs to be public because it's invoked by the generated code - e.g. msg.MergeFrom(CodedInputStream input) method
        /// </summary>
        public void ReadRawMessage(IMessage message)
        {
            ParseContext.Initialize(this, out ParseContext ctx);
            try
            {
                ParsingPrimitivesMessages.ReadRawMessage(ref ctx, message);
            }
            finally
            {
                ctx.CopyStateTo(this);
            }
        }
#endregion
    }
}
