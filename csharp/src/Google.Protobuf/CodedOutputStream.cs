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
using System.Text;

namespace Google.Protobuf
{
    /// <summary>
    /// Encodes and writes protocol message fields.
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
    public sealed partial class CodedOutputStream : IDisposable
    {
        /// <summary>
        /// The buffer size used by CreateInstance(Stream).
        /// </summary>
        public static readonly int DefaultBufferSize = 4096;

        private readonly bool leaveOpen;
        private readonly byte[] buffer;
        private WriterInternalState state;

        private readonly Stream output;

        #region Construction
        /// <summary>
        /// Creates a new CodedOutputStream that writes directly to the given
        /// byte array. If more bytes are written than fit in the array,
        /// OutOfSpaceException will be thrown.
        /// </summary>
        public CodedOutputStream(byte[] flatArray) : this(flatArray, 0, flatArray.Length)
        {
        }

        /// <summary>
        /// Creates a new CodedOutputStream that writes directly to the given
        /// byte array slice. If more bytes are written than fit in the array,
        /// OutOfSpaceException will be thrown.
        /// </summary>
        private CodedOutputStream(byte[] buffer, int offset, int length)
        {
            this.output = null;
            this.buffer = ProtoPreconditions.CheckNotNull(buffer, nameof(buffer));
            this.state.position = offset;
            this.state.limit = offset + length;
            WriteBufferHelper.Initialize(this, out this.state.writeBufferHelper);
            leaveOpen = true; // Simple way of avoiding trying to dispose of a null reference
        }

        private CodedOutputStream(Stream output, byte[] buffer, bool leaveOpen)
        {
            this.output = ProtoPreconditions.CheckNotNull(output, nameof(output));
            this.buffer = buffer;
            this.state.position = 0;
            this.state.limit = buffer.Length;
            WriteBufferHelper.Initialize(this, out this.state.writeBufferHelper);
            this.leaveOpen = leaveOpen;
        }

        /// <summary>
        /// Creates a new <see cref="CodedOutputStream" /> which write to the given stream, and disposes of that
        /// stream when the returned <c>CodedOutputStream</c> is disposed.
        /// </summary>
        /// <param name="output">The stream to write to. It will be disposed when the returned <c>CodedOutputStream is disposed.</c></param>
        public CodedOutputStream(Stream output) : this(output, DefaultBufferSize, false)
        {
        }

        /// <summary>
        /// Creates a new CodedOutputStream which write to the given stream and uses
        /// the specified buffer size.
        /// </summary>
        /// <param name="output">The stream to write to. It will be disposed when the returned <c>CodedOutputStream is disposed.</c></param>
        /// <param name="bufferSize">The size of buffer to use internally.</param>
        public CodedOutputStream(Stream output, int bufferSize) : this(output, new byte[bufferSize], false)
        {
        }

        /// <summary>
        /// Creates a new CodedOutputStream which write to the given stream.
        /// </summary>
        /// <param name="output">The stream to write to.</param>
        /// <param name="leaveOpen">If <c>true</c>, <paramref name="output"/> is left open when the returned <c>CodedOutputStream</c> is disposed;
        /// if <c>false</c>, the provided stream is disposed as well.</param>
        public CodedOutputStream(Stream output, bool leaveOpen) : this(output, DefaultBufferSize, leaveOpen)
        {
        }

        /// <summary>
        /// Creates a new CodedOutputStream which write to the given stream and uses
        /// the specified buffer size.
        /// </summary>
        /// <param name="output">The stream to write to.</param>
        /// <param name="bufferSize">The size of buffer to use internally.</param>
        /// <param name="leaveOpen">If <c>true</c>, <paramref name="output"/> is left open when the returned <c>CodedOutputStream</c> is disposed;
        /// if <c>false</c>, the provided stream is disposed as well.</param>
        public CodedOutputStream(Stream output, int bufferSize, bool leaveOpen) : this(output, new byte[bufferSize], leaveOpen)
        {
        }
        #endregion

        /// <summary>
        /// Returns the current position in the stream, or the position in the output buffer
        /// </summary>
        public long Position
        {
            get
            {
                if (output != null)
                {
                    return output.Position + state.position;
                }
                return state.position;
            }
        }

        #region Writing of values (not including tags)

        /// <summary>
        /// Writes a double field value, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteDouble(double value)
        {
            var span = new Span<byte>(buffer);
            WritingPrimitives.WriteDouble(ref span, ref state, value);
        }

        /// <summary>
        /// Writes a float field value, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteFloat(float value)
        {
            var span = new Span<byte>(buffer);
            WritingPrimitives.WriteFloat(ref span, ref state, value);
        }

        /// <summary>
        /// Writes a uint64 field value, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteUInt64(ulong value)
        {
            var span = new Span<byte>(buffer);
            WritingPrimitives.WriteUInt64(ref span, ref state, value);
        }

        /// <summary>
        /// Writes an int64 field value, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteInt64(long value)
        {
            var span = new Span<byte>(buffer);
            WritingPrimitives.WriteInt64(ref span, ref state, value);
        }

        /// <summary>
        /// Writes an int32 field value, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteInt32(int value)
        {
            var span = new Span<byte>(buffer);
            WritingPrimitives.WriteInt32(ref span, ref state, value);
        }

        /// <summary>
        /// Writes a fixed64 field value, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteFixed64(ulong value)
        {
            var span = new Span<byte>(buffer);
            WritingPrimitives.WriteFixed64(ref span, ref state, value);
        }

        /// <summary>
        /// Writes a fixed32 field value, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteFixed32(uint value)
        {
            var span = new Span<byte>(buffer);
            WritingPrimitives.WriteFixed32(ref span, ref state, value);
        }

        /// <summary>
        /// Writes a bool field value, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteBool(bool value)
        {
            var span = new Span<byte>(buffer);
            WritingPrimitives.WriteBool(ref span, ref state, value);
        }

        /// <summary>
        /// Writes a string field value, without a tag, to the stream.
        /// The data is length-prefixed.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteString(string value)
        {
            var span = new Span<byte>(buffer);
            WritingPrimitives.WriteString(ref span, ref state, value);
        }

        /// <summary>
        /// Writes a message, without a tag, to the stream.
        /// The data is length-prefixed.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteMessage(IMessage value)
        {
            // TODO(jtattermusch): if the message doesn't implement IBufferMessage (and thus does not provide the InternalWriteTo method),
            // what we're doing here works fine, but could be more efficient.
            // For now, this inefficiency is fine, considering this is only a backward-compatibility scenario (and regenerating the code fixes it).
            var span = new Span<byte>(buffer);
            WriteContext.Initialize(ref span, ref state, out WriteContext ctx);
            try
            {
                WritingPrimitivesMessages.WriteMessage(ref ctx, value);
            }
            finally
            {
                ctx.CopyStateTo(this);
            }
        }

        /// <summary>
        /// Writes a message, without a tag, to the stream.
        /// Only the message data is written, without a length-delimiter.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteRawMessage(IMessage value)
        {
            // TODO(jtattermusch): if the message doesn't implement IBufferMessage (and thus does not provide the InternalWriteTo method),
            // what we're doing here works fine, but could be more efficient.
            // For now, this inefficiency is fine, considering this is only a backward-compatibility scenario (and regenerating the code fixes it).
            var span = new Span<byte>(buffer);
            WriteContext.Initialize(ref span, ref state, out WriteContext ctx);
            try
            {
                WritingPrimitivesMessages.WriteRawMessage(ref ctx, value);
            }
            finally
            {
                ctx.CopyStateTo(this);
            }
        }

        /// <summary>
        /// Writes a group, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteGroup(IMessage value)
        {
            var span = new Span<byte>(buffer);
            WriteContext.Initialize(ref span, ref state, out WriteContext ctx);
            try
            {
                WritingPrimitivesMessages.WriteGroup(ref ctx, value);
            }
            finally
            {
                ctx.CopyStateTo(this);
            }
        }

        /// <summary>
        /// Write a byte string, without a tag, to the stream.
        /// The data is length-prefixed.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteBytes(ByteString value)
        {
            var span = new Span<byte>(buffer);
            WritingPrimitives.WriteBytes(ref span, ref state, value);
        }

        /// <summary>
        /// Writes a uint32 value, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteUInt32(uint value)
        {
            var span = new Span<byte>(buffer);
            WritingPrimitives.WriteUInt32(ref span, ref state, value);
        }

        /// <summary>
        /// Writes an enum value, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteEnum(int value)
        {
            var span = new Span<byte>(buffer);
            WritingPrimitives.WriteEnum(ref span, ref state, value);
        }

        /// <summary>
        /// Writes an sfixed32 value, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write.</param>
        public void WriteSFixed32(int value)
        {
            var span = new Span<byte>(buffer);
            WritingPrimitives.WriteSFixed32(ref span, ref state, value);
        }

        /// <summary>
        /// Writes an sfixed64 value, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteSFixed64(long value)
        {
            var span = new Span<byte>(buffer);
            WritingPrimitives.WriteSFixed64(ref span, ref state, value);
        }

        /// <summary>
        /// Writes an sint32 value, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteSInt32(int value)
        {
            var span = new Span<byte>(buffer);
            WritingPrimitives.WriteSInt32(ref span, ref state, value);
        }

        /// <summary>
        /// Writes an sint64 value, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteSInt64(long value)
        {
            var span = new Span<byte>(buffer);
            WritingPrimitives.WriteSInt64(ref span, ref state, value);
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
            var span = new Span<byte>(buffer);
            WritingPrimitives.WriteLength(ref span, ref state, length);
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
            var span = new Span<byte>(buffer);
            WritingPrimitives.WriteTag(ref span, ref state, fieldNumber, type);
        }

        /// <summary>
        /// Writes an already-encoded tag.
        /// </summary>
        /// <param name="tag">The encoded tag</param>
        public void WriteTag(uint tag)
        {
            var span = new Span<byte>(buffer);
            WritingPrimitives.WriteTag(ref span, ref state, tag);
        }

        /// <summary>
        /// Writes the given single-byte tag directly to the stream.
        /// </summary>
        /// <param name="b1">The encoded tag</param>
        public void WriteRawTag(byte b1)
        {
            var span = new Span<byte>(buffer);
            WritingPrimitives.WriteRawTag(ref span, ref state, b1);
        }

        /// <summary>
        /// Writes the given two-byte tag directly to the stream.
        /// </summary>
        /// <param name="b1">The first byte of the encoded tag</param>
        /// <param name="b2">The second byte of the encoded tag</param>
        public void WriteRawTag(byte b1, byte b2)
        {
            var span = new Span<byte>(buffer);
            WritingPrimitives.WriteRawTag(ref span, ref state, b1, b2);
        }

        /// <summary>
        /// Writes the given three-byte tag directly to the stream.
        /// </summary>
        /// <param name="b1">The first byte of the encoded tag</param>
        /// <param name="b2">The second byte of the encoded tag</param>
        /// <param name="b3">The third byte of the encoded tag</param>
        public void WriteRawTag(byte b1, byte b2, byte b3)
        {
            var span = new Span<byte>(buffer);
            WritingPrimitives.WriteRawTag(ref span, ref state, b1, b2, b3);
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
            var span = new Span<byte>(buffer);
            WritingPrimitives.WriteRawTag(ref span, ref state, b1, b2, b3, b4);
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
            var span = new Span<byte>(buffer);
            WritingPrimitives.WriteRawTag(ref span, ref state, b1, b2, b3, b4, b5);
        }
        #endregion

        #region Underlying writing primitives
        
        /// <summary>
        /// Writes a 32 bit value as a varint. The fast route is taken when
        /// there's enough buffer space left to whizz through without checking
        /// for each byte; otherwise, we resort to calling WriteRawByte each time.
        /// </summary>
        internal void WriteRawVarint32(uint value)
        {
            var span = new Span<byte>(buffer);
            WritingPrimitives.WriteRawVarint32(ref span, ref state, value);
        }

        internal void WriteRawVarint64(ulong value)
        {
            var span = new Span<byte>(buffer);
            WritingPrimitives.WriteRawVarint64(ref span, ref state, value);
        }

        internal void WriteRawLittleEndian32(uint value)
        {
            var span = new Span<byte>(buffer);
            WritingPrimitives.WriteRawLittleEndian32(ref span, ref state, value);
        }

        internal void WriteRawLittleEndian64(ulong value)
        {
            var span = new Span<byte>(buffer);
            WritingPrimitives.WriteRawLittleEndian64(ref span, ref state, value);
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
            var span = new Span<byte>(buffer);
            WritingPrimitives.WriteRawBytes(ref span, ref state, value, offset, length);
        }

        #endregion

        /// <summary>
        /// Indicates that a CodedOutputStream wrapping a flat byte array
        /// ran out of space.
        /// </summary>
        public sealed class OutOfSpaceException : IOException
        {
            internal OutOfSpaceException()
                : base("CodedOutputStream was writing to a flat byte array and ran out of space.")
            {
            }
        }

        /// <summary>
        /// Flushes any buffered data and optionally closes the underlying stream, if any.
        /// </summary>
        /// <remarks>
        /// <para>
        /// By default, any underlying stream is closed by this method. To configure this behaviour,
        /// use a constructor overload with a <c>leaveOpen</c> parameter. If this instance does not
        /// have an underlying stream, this method does nothing.
        /// </para>
        /// <para>
        /// For the sake of efficiency, calling this method does not prevent future write calls - but
        /// if a later write ends up writing to a stream which has been disposed, that is likely to
        /// fail. It is recommend that you not call any other methods after this.
        /// </para>
        /// </remarks>
        public void Dispose()
        {
            Flush();
            if (!leaveOpen)
            {
                output.Dispose();
            }
        }

        /// <summary>
        /// Flushes any buffered data to the underlying stream (if there is one).
        /// </summary>
        public void Flush()
        {
            var span = new Span<byte>(buffer);
            WriteBufferHelper.Flush(ref span, ref state);
        }

        /// <summary>
        /// Verifies that SpaceLeft returns zero. It's common to create a byte array
        /// that is exactly big enough to hold a message, then write to it with
        /// a CodedOutputStream. Calling CheckNoSpaceLeft after writing verifies that
        /// the message was actually as big as expected, which can help finding bugs.
        /// </summary>
        public void CheckNoSpaceLeft()
        {
            WriteBufferHelper.CheckNoSpaceLeft(ref state);
        }

        /// <summary>
        /// If writing to a flat array, returns the space left in the array. Otherwise,
        /// throws an InvalidOperationException.
        /// </summary>
        public int SpaceLeft => WriteBufferHelper.GetSpaceLeft(ref state);

        internal byte[] InternalBuffer => buffer;

        internal Stream InternalOutputStream => output;

        internal ref WriterInternalState InternalState => ref state;
    }
}
