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
    /// An opaque struct that represents the current serialization state and is passed along
    /// as the serialization proceeds.
    /// All the public methods are intended to be invoked only by the generated code,
    /// users should never invoke them directly.
    /// </summary>
    [SecuritySafeCritical]
    public ref struct WriteContext
    {
        internal Span<byte> buffer;
        internal WriterInternalState state;

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Initialize(ref Span<byte> buffer, ref WriterInternalState state, out WriteContext ctx)
        {
            ctx.buffer = buffer;
            ctx.state = state;
        }

        /// <summary>
        /// Creates a WriteContext instance from CodedOutputStream.
        /// WARNING: internally this copies the CodedOutputStream's state, so after done with the WriteContext,
        /// the CodedOutputStream's state needs to be updated.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Initialize(CodedOutputStream output, out WriteContext ctx)
        {
            ctx.buffer = new Span<byte>(output.InternalBuffer);
            // ideally we would use a reference to the original state, but that doesn't seem possible
            // so we just copy the struct that holds the state. We will need to later store the state back
            // into CodedOutputStream if we want to keep it usable.
            ctx.state = output.InternalState;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Initialize(IBufferWriter<byte> output, out WriteContext ctx)
        {
            ctx.buffer = default;
            ctx.state = default;
            WriteBufferHelper.Initialize(output, out ctx.state.writeBufferHelper, out ctx.buffer);
            ctx.state.limit = ctx.buffer.Length;
            ctx.state.position = 0;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Initialize(ref Span<byte> buffer, out WriteContext ctx)
        {
            ctx.buffer = buffer;
            ctx.state = default;
            ctx.state.limit = ctx.buffer.Length;
            ctx.state.position = 0;
            WriteBufferHelper.InitializeNonRefreshable(out ctx.state.writeBufferHelper);
        }

        /// <summary>
        /// Writes a double field value, without a tag.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteDouble(double value)
        {
            WritingPrimitives.WriteDouble(ref buffer, ref state, value);
        }

        /// <summary>
        /// Writes a float field value, without a tag.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteFloat(float value)
        {
            WritingPrimitives.WriteFloat(ref buffer, ref state, value);
        }

        /// <summary>
        /// Writes a uint64 field value, without a tag.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteUInt64(ulong value)
        {
            WritingPrimitives.WriteUInt64(ref buffer, ref state, value);
        }

        /// <summary>
        /// Writes an int64 field value, without a tag.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteInt64(long value)
        {
            WritingPrimitives.WriteInt64(ref buffer, ref state, value);
        }

        /// <summary>
        /// Writes an int32 field value, without a tag.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteInt32(int value)
        {
            WritingPrimitives.WriteInt32(ref buffer, ref state, value);
        }

        /// <summary>
        /// Writes a fixed64 field value, without a tag.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteFixed64(ulong value)
        {
            WritingPrimitives.WriteFixed64(ref buffer, ref state, value);
        }

        /// <summary>
        /// Writes a fixed32 field value, without a tag.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteFixed32(uint value)
        {
            WritingPrimitives.WriteFixed32(ref buffer, ref state, value);
        }

        /// <summary>
        /// Writes a bool field value, without a tag.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteBool(bool value)
        {
            WritingPrimitives.WriteBool(ref buffer, ref state, value);
        }

        /// <summary>
        /// Writes a string field value, without a tag.
        /// The data is length-prefixed.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteString(string value)
        {
            WritingPrimitives.WriteString(ref buffer, ref state, value);
        }

        /// <summary>
        /// Writes a message, without a tag.
        /// The data is length-prefixed.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteMessage(IMessage value)
        {
            WritingPrimitivesMessages.WriteMessage(ref this, value);
        }

        /// <summary>
        /// Writes a group, without a tag, to the stream.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteGroup(IMessage value)
        {
            WritingPrimitivesMessages.WriteGroup(ref this, value);
        }

        /// <summary>
        /// Write a byte string, without a tag, to the stream.
        /// The data is length-prefixed.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteBytes(ByteString value)
        {
            WritingPrimitives.WriteBytes(ref buffer, ref state, value);
        }

        /// <summary>
        /// Writes a uint32 value, without a tag.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteUInt32(uint value)
        {
            WritingPrimitives.WriteUInt32(ref buffer, ref state, value);
        }

        /// <summary>
        /// Writes an enum value, without a tag.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteEnum(int value)
        {
            WritingPrimitives.WriteEnum(ref buffer, ref state, value);
        }

        /// <summary>
        /// Writes an sfixed32 value, without a tag.
        /// </summary>
        /// <param name="value">The value to write.</param>
        public void WriteSFixed32(int value)
        {
            WritingPrimitives.WriteSFixed32(ref buffer, ref state, value);
        }

        /// <summary>
        /// Writes an sfixed64 value, without a tag.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteSFixed64(long value)
        {
            WritingPrimitives.WriteSFixed64(ref buffer, ref state, value);
        }

        /// <summary>
        /// Writes an sint32 value, without a tag.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteSInt32(int value)
        {
            WritingPrimitives.WriteSInt32(ref buffer, ref state, value);
        }

        /// <summary>
        /// Writes an sint64 value, without a tag.
        /// </summary>
        /// <param name="value">The value to write</param>
        public void WriteSInt64(long value)
        {
            WritingPrimitives.WriteSInt64(ref buffer, ref state, value);
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
            WritingPrimitives.WriteLength(ref buffer, ref state, length);
        }

        /// <summary>
        /// Encodes and writes a tag.
        /// </summary>
        /// <param name="fieldNumber">The number of the field to write the tag for</param>
        /// <param name="type">The wire format type of the tag to write</param>
        public void WriteTag(int fieldNumber, WireFormat.WireType type)
        {
            WritingPrimitives.WriteTag(ref buffer, ref state, fieldNumber, type);
        }

        /// <summary>
        /// Writes an already-encoded tag.
        /// </summary>
        /// <param name="tag">The encoded tag</param>
        public void WriteTag(uint tag)
        {
            WritingPrimitives.WriteTag(ref buffer, ref state, tag);
        }

        /// <summary>
        /// Writes the given single-byte tag.
        /// </summary>
        /// <param name="b1">The encoded tag</param>
        public void WriteRawTag(byte b1)
        {
            WritingPrimitives.WriteRawTag(ref buffer, ref state, b1);
        }

        /// <summary>
        /// Writes the given two-byte tag.
        /// </summary>
        /// <param name="b1">The first byte of the encoded tag</param>
        /// <param name="b2">The second byte of the encoded tag</param>
        public void WriteRawTag(byte b1, byte b2)
        {
            WritingPrimitives.WriteRawTag(ref buffer, ref state, b1, b2);
        }

        /// <summary>
        /// Writes the given three-byte tag.
        /// </summary>
        /// <param name="b1">The first byte of the encoded tag</param>
        /// <param name="b2">The second byte of the encoded tag</param>
        /// <param name="b3">The third byte of the encoded tag</param>
        public void WriteRawTag(byte b1, byte b2, byte b3)
        {
            WritingPrimitives.WriteRawTag(ref buffer, ref state, b1, b2, b3);
        }

        /// <summary>
        /// Writes the given four-byte tag.
        /// </summary>
        /// <param name="b1">The first byte of the encoded tag</param>
        /// <param name="b2">The second byte of the encoded tag</param>
        /// <param name="b3">The third byte of the encoded tag</param>
        /// <param name="b4">The fourth byte of the encoded tag</param>
        public void WriteRawTag(byte b1, byte b2, byte b3, byte b4)
        {
            WritingPrimitives.WriteRawTag(ref buffer, ref state, b1, b2, b3, b4);
        }

        /// <summary>
        /// Writes the given five-byte tag.
        /// </summary>
        /// <param name="b1">The first byte of the encoded tag</param>
        /// <param name="b2">The second byte of the encoded tag</param>
        /// <param name="b3">The third byte of the encoded tag</param>
        /// <param name="b4">The fourth byte of the encoded tag</param>
        /// <param name="b5">The fifth byte of the encoded tag</param>
        public void WriteRawTag(byte b1, byte b2, byte b3, byte b4, byte b5)
        {
            WritingPrimitives.WriteRawTag(ref buffer, ref state, b1, b2, b3, b4, b5);
        }

        internal void Flush()
        {
            WriteBufferHelper.Flush(ref buffer, ref state);
        }

        internal void CheckNoSpaceLeft()
        {
            WriteBufferHelper.CheckNoSpaceLeft(ref state);
        }

        internal void CopyStateTo(CodedOutputStream output)
        {
            output.InternalState = state;
        }

        internal void LoadStateFrom(CodedOutputStream output)
        {
            state = output.InternalState;
        }
    }
}