#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
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
using Google.Protobuf.Compatibility;
using Google.Protobuf.WellKnownTypes;
using System;
using System.Collections.Generic;

namespace Google.Protobuf
{
    /// <summary>
    /// Factory methods for <see cref="FieldCodec{T}"/>.
    /// </summary>
    public static class FieldCodec
    {
        // TODO: Avoid the "dual hit" of lambda expressions: create open delegates instead. (At least test...)

        /// <summary>
        /// Retrieves a codec suitable for a string field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<string> ForString(uint tag)
        {
            return FieldCodec.ForString(tag, "");
        }

        /// <summary>
        /// Retrieves a codec suitable for a bytes field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<ByteString> ForBytes(uint tag)
        {
            return FieldCodec.ForBytes(tag, ByteString.Empty);
        }

        /// <summary>
        /// Retrieves a codec suitable for a bool field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<bool> ForBool(uint tag)
        {
            return FieldCodec.ForBool(tag, false);
        }

        /// <summary>
        /// Retrieves a codec suitable for an int32 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<int> ForInt32(uint tag)
        {
            return FieldCodec.ForInt32(tag, 0);
        }

        /// <summary>
        /// Retrieves a codec suitable for an sint32 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<int> ForSInt32(uint tag)
        {
            return FieldCodec.ForSInt32(tag, 0);
        }

        /// <summary>
        /// Retrieves a codec suitable for a fixed32 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<uint> ForFixed32(uint tag)
        {
            return FieldCodec.ForFixed32(tag, 0);
        }

        /// <summary>
        /// Retrieves a codec suitable for an sfixed32 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<int> ForSFixed32(uint tag)
        {
            return FieldCodec.ForSFixed32(tag, 0);
        }

        /// <summary>
        /// Retrieves a codec suitable for a uint32 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<uint> ForUInt32(uint tag)
        {
            return FieldCodec.ForUInt32(tag, 0);
        }

        /// <summary>
        /// Retrieves a codec suitable for an int64 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<long> ForInt64(uint tag)
        {
            return FieldCodec.ForInt64(tag, 0);
        }

        /// <summary>
        /// Retrieves a codec suitable for an sint64 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<long> ForSInt64(uint tag)
        {
            return FieldCodec.ForSInt64(tag, 0);
        }

        /// <summary>
        /// Retrieves a codec suitable for a fixed64 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<ulong> ForFixed64(uint tag)
        {
            return FieldCodec.ForFixed64(tag, 0);
        }

        /// <summary>
        /// Retrieves a codec suitable for an sfixed64 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<long> ForSFixed64(uint tag)
        {
            return FieldCodec.ForSFixed64(tag, 0);
        }

        /// <summary>
        /// Retrieves a codec suitable for a uint64 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<ulong> ForUInt64(uint tag)
        {
            return FieldCodec.ForUInt64(tag, 0);
        }

        /// <summary>
        /// Retrieves a codec suitable for a float field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<float> ForFloat(uint tag)
        {
            return FieldCodec.ForFloat(tag, 0);
        }

        /// <summary>
        /// Retrieves a codec suitable for a double field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<double> ForDouble(uint tag)
        {
            return FieldCodec.ForDouble(tag, 0);
        }

        // Enums are tricky. We can probably use expression trees to build these delegates automatically,
        // but it's easy to generate the code for it.

        /// <summary>
        /// Retrieves a codec suitable for an enum field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="toInt32">A conversion function from <see cref="Int32"/> to the enum type.</param>
        /// <param name="fromInt32">A conversion function from the enum type to <see cref="Int32"/>.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<T> ForEnum<T>(uint tag, Func<T, int> toInt32, Func<int, T> fromInt32)
        {
            return FieldCodec.ForEnum(tag, toInt32, fromInt32, default(T));
        }

        /// <summary>
        /// Retrieves a codec suitable for a string field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<string> ForString(uint tag, string defaultValue)
        {
            return new FieldCodec<string>(
                input => input.ReadString(),
                (output, value) => output.WriteString(value),
#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
                (ref CodedInputReader input) => input.ReadString(),
                (ref CodedOutputWriter output, string value) => output.WriteString(value),
#endif
                CodedOutputStream.ComputeStringSize,
                tag,
                defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for a bytes field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<ByteString> ForBytes(uint tag, ByteString defaultValue)
        {
            return new FieldCodec<ByteString>(
                input => input.ReadBytes(),
                (output, value) => output.WriteBytes(value),
#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
                (ref CodedInputReader input) => input.ReadBytes(),
                (ref CodedOutputWriter output, ByteString value) => output.WriteBytes(value),
#endif
                CodedOutputStream.ComputeBytesSize,
                tag,
                defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for a bool field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<bool> ForBool(uint tag, bool defaultValue)
        {
            return new FieldCodec<bool>(
                input => input.ReadBool(),
                (output, value) => output.WriteBool(value),
#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
                (ref CodedInputReader input) => input.ReadBool(),
                (ref CodedOutputWriter output, bool value) => output.WriteBool(value),
#endif
                CodedOutputStream.BoolSize,
                tag,
                defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for an int32 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<int> ForInt32(uint tag, int defaultValue)
        {
            return new FieldCodec<int>(
                input => input.ReadInt32(),
                (output, value) => output.WriteInt32(value),
#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
                (ref CodedInputReader input) => input.ReadInt32(),
                (ref CodedOutputWriter output, int value) => output.WriteInt32(value),
#endif
                CodedOutputStream.ComputeInt32Size,
                tag,
                defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for an sint32 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<int> ForSInt32(uint tag, int defaultValue)
        {
            return new FieldCodec<int>(
                input => input.ReadSInt32(),
                (output, value) => output.WriteSInt32(value),
#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
                (ref CodedInputReader input) => input.ReadSInt32(),
                (ref CodedOutputWriter output, int value) => output.WriteSInt32(value),
#endif
                CodedOutputStream.ComputeSInt32Size,
                tag,
                defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for a fixed32 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<uint> ForFixed32(uint tag, uint defaultValue)
        {
            return new FieldCodec<uint>(
                input => input.ReadFixed32(),
                (output, value) => output.WriteFixed32(value),
#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
                (ref CodedInputReader input) => input.ReadFixed32(),
                (ref CodedOutputWriter output, uint value) => output.WriteFixed32(value),
#endif
                4,
                tag,
                defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for an sfixed32 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<int> ForSFixed32(uint tag, int defaultValue)
        {
            return new FieldCodec<int>(
                input => input.ReadSFixed32(),
                (output, value) => output.WriteSFixed32(value),
#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
                (ref CodedInputReader input) => input.ReadSFixed32(),
                (ref CodedOutputWriter output, int value) => output.WriteSFixed32(value),
#endif
                4,
                tag,
                defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for a uint32 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<uint> ForUInt32(uint tag, uint defaultValue)
        {
            return new FieldCodec<uint>(
                input => input.ReadUInt32(),
                (output, value) => output.WriteUInt32(value),
#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
                (ref CodedInputReader input) => input.ReadUInt32(),
                (ref CodedOutputWriter output, uint value) => output.WriteUInt32(value),
#endif
                CodedOutputStream.ComputeUInt32Size,
                tag,
                defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for an int64 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<long> ForInt64(uint tag, long defaultValue)
        {
            return new FieldCodec<long>(
                input => input.ReadInt64(),
                (output, value) => output.WriteInt64(value),
#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
                (ref CodedInputReader input) => input.ReadInt64(),
                (ref CodedOutputWriter output, long value) => output.WriteInt64(value),
#endif
                CodedOutputStream.ComputeInt64Size,
                tag,
                defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for an sint64 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<long> ForSInt64(uint tag, long defaultValue)
        {
            return new FieldCodec<long>(
                input => input.ReadSInt64(),
                (output, value) => output.WriteSInt64(value),
#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
                (ref CodedInputReader input) => input.ReadSInt64(),
                (ref CodedOutputWriter output, long value) => output.WriteSInt64(value),
#endif
                CodedOutputStream.ComputeSInt64Size,
                tag,
                defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for a fixed64 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<ulong> ForFixed64(uint tag, ulong defaultValue)
        {
            return new FieldCodec<ulong>(
                input => input.ReadFixed64(),
                (output, value) => output.WriteFixed64(value),
#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
                (ref CodedInputReader input) => input.ReadFixed64(),
                (ref CodedOutputWriter output, ulong value) => output.WriteFixed64(value),
#endif
                8,
                tag,
                defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for an sfixed64 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<long> ForSFixed64(uint tag, long defaultValue)
        {
            return new FieldCodec<long>(
                input => input.ReadSFixed64(),
                (output, value) => output.WriteSFixed64(value),
#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
                (ref CodedInputReader input) => input.ReadSFixed64(),
                (ref CodedOutputWriter output, long value) => output.WriteSFixed64(value),
#endif
                8,
                tag,
                defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for a uint64 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<ulong> ForUInt64(uint tag, ulong defaultValue)
        {
            return new FieldCodec<ulong>(
                input => input.ReadUInt64(),
                (output, value) => output.WriteUInt64(value),
#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
                (ref CodedInputReader input) => input.ReadUInt64(),
                (ref CodedOutputWriter output, ulong value) => output.WriteUInt64(value),
#endif
                CodedOutputStream.ComputeUInt64Size,
                tag,
                defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for a float field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<float> ForFloat(uint tag, float defaultValue)
        {
            return new FieldCodec<float>(
                input => input.ReadFloat(),
                (output, value) => output.WriteFloat(value),
#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
                (ref CodedInputReader input) => input.ReadFloat(),
                (ref CodedOutputWriter output, float value) => output.WriteFloat(value),
#endif
                CodedOutputStream.FloatSize,
                tag,
                defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for a double field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<double> ForDouble(uint tag, double defaultValue)
        {
            return new FieldCodec<double>(
                input => input.ReadDouble(),
                (output, value) => output.WriteDouble(value),
#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
                (ref CodedInputReader input) => input.ReadDouble(),
                (ref CodedOutputWriter output, double value) => output.WriteDouble(value),
#endif
                CodedOutputStream.DoubleSize,
                tag,
                defaultValue);
        }

        // Enums are tricky. We can probably use expression trees to build these delegates automatically,
        // but it's easy to generate the code for it.

        /// <summary>
        /// Retrieves a codec suitable for an enum field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="toInt32">A conversion function from <see cref="Int32"/> to the enum type.</param>
        /// <param name="fromInt32">A conversion function from the enum type to <see cref="Int32"/>.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<T> ForEnum<T>(uint tag, Func<T, int> toInt32, Func<int, T> fromInt32, T defaultValue)
        {
            return new FieldCodec<T>(
                input => fromInt32(input.ReadEnum()),
                (output, value) => output.WriteEnum(toInt32(value)),
#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
                (ref CodedInputReader input) => fromInt32(input.ReadEnum()),
                (ref CodedOutputWriter output, T value) => output.WriteEnum(toInt32(value)),
#endif
                value => CodedOutputStream.ComputeEnumSize(toInt32(value)), tag, defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for a message field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="parser">A parser to use for the message type.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<T> ForMessage<T>(uint tag, MessageParser<T> parser) where T : class, IMessage<T>
        {
            return new FieldCodec<T>(
                input =>
                {
                    T message = parser.CreateTemplate();
                    input.ReadMessage(message);
                    return message;
                },
                (output, value) => output.WriteMessage(value),
                (CodedInputStream i, ref T v) =>
                {
                    if (v == null)
                    {
                        v = parser.CreateTemplate();
                    }

                    i.ReadMessage(v);
                },
                (ref T v, T v2) =>
                {
                    if (v2 == null)
                    {
                        return false;
                    }
                    else if (v == null)
                    {
                        v = v2.Clone();
                    }
                    else
                    {
                        v.MergeFrom(v2);
                    }
                    return true;
                },
#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
                (ref CodedInputReader input) =>
                {
                    T message = parser.CreateTemplate();

                    if (message is IBufferMessage m)
                    {
                        input.ReadMessage(m);
                        return message;
                    }

                    throw new NotSupportedException(nameof(T) + " doesn't implement " + nameof(IBufferMessage));
                },
                (ref CodedOutputWriter output, T value) =>
                {
                    if (value is IBufferMessage m)
                    {
                        output.WriteMessage(m);
                        return;
                    }

                    throw new NotSupportedException(nameof(T) + " doesn't implement " + nameof(IBufferMessage));
                },
                (ref CodedInputReader i, ref T v) =>
                {
                    if (v == null)
                    {
                        v = parser.CreateTemplate();
                    }

                    if (v is IBufferMessage m)
                    {
                        i.ReadMessage(m);
                        return;
                    }

                    throw new NotSupportedException(nameof(T) + " doesn't implement " + nameof(IBufferMessage));
                },
#endif
                message => CodedOutputStream.ComputeMessageSize(message), tag);
        }

        /// <summary>
        /// Retrieves a codec suitable for a group field with the given tag.
        /// </summary>
        /// <param name="startTag">The start group tag.</param>
        /// <param name="endTag">The end group tag.</param>
        /// <param name="parser">A parser to use for the group message type.</param>
        /// <returns>A codec for given tag</returns>
        public static FieldCodec<T> ForGroup<T>(uint startTag, uint endTag, MessageParser<T> parser) where T : class, IMessage<T>
        {
            return new FieldCodec<T>(
                input => 
                { 
                    T message = parser.CreateTemplate();
                    input.ReadGroup(message);
                    return message;
                },
                (output, value) => output.WriteGroup(value),
                (CodedInputStream i, ref T v) =>
                {
                    if (v == null)
                    {
                        v = parser.CreateTemplate();
                    }

                    i.ReadGroup(v);
                },
                (ref T v, T v2) =>
                {
                    if (v2 == null)
                    {
                        return v == null;
                    }
                    else if (v == null)
                    {
                        v = v2.Clone();
                    }
                    else
                    {
                        v.MergeFrom(v2);
                    }
                    return true;
                }, 
#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
                (ref CodedInputReader input) =>
                {
                    T message = parser.CreateTemplate();

                    if (message is IBufferMessage m)
                    {
                        input.ReadGroup(m);
                        return message;
                    }

                    throw new NotSupportedException(nameof(T) + " doesn't implement " + nameof(IBufferMessage));
                },
                (ref CodedOutputWriter output, T value) =>
                {
                    if (value is IBufferMessage m)
                    {
                        output.WriteGroup(m);
                        return;
                    }

                    throw new NotSupportedException(nameof(T) + " doesn't implement " + nameof(IBufferMessage));
                },
                (ref CodedInputReader i, ref T v) =>
                {
                    if (v == null)
                    {
                        v = parser.CreateTemplate();
                    }

                    if (v is IBufferMessage m)
                    {
                        i.ReadGroup(m);
                        return;
                    }

                    throw new NotSupportedException(nameof(T) + " doesn't implement " + nameof(IBufferMessage));
                },
#endif
                message => CodedOutputStream.ComputeGroupSize(message), startTag, endTag);
        }

        /// <summary>
        /// Creates a codec for a wrapper type of a class - which must be string or ByteString.
        /// </summary>
        public static FieldCodec<T> ForClassWrapper<T>(uint tag) where T : class
        {
            var nestedCodec = WrapperCodecs.GetCodec<T>();
            return new FieldCodec<T>(
                input => WrapperCodecs.Read<T>(input, nestedCodec),
                (output, value) => WrapperCodecs.Write<T>(output, value, nestedCodec),
                (CodedInputStream i, ref T v) => v = WrapperCodecs.Read<T>(i, nestedCodec),
                (ref T v, T v2) => { v = v2; return v == null; },
#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
                (ref CodedInputReader input) => WrapperCodecs.Read<T>(ref input, nestedCodec),
                (ref CodedOutputWriter output, T value) => WrapperCodecs.Write<T>(ref output, value, nestedCodec),
                (ref CodedInputReader i, ref T v) => v = WrapperCodecs.Read<T>(ref i, nestedCodec),
#endif
                value => WrapperCodecs.CalculateSize<T>(value, nestedCodec),
                tag, 0,
                null); // Default value for the wrapper
        }

        /// <summary>
        /// Creates a codec for a wrapper type of a struct - which must be Int32, Int64, UInt32, UInt64,
        /// Bool, Single or Double.
        /// </summary>
        public static FieldCodec<T?> ForStructWrapper<T>(uint tag) where T : struct
        {
            var nestedCodec = WrapperCodecs.GetCodec<T>();
            return new FieldCodec<T?>(
                WrapperCodecs.GetStreamReader<T>(),
                (output, value) => WrapperCodecs.Write<T>(output, value.Value, nestedCodec),
                (CodedInputStream i, ref T? v) => v = WrapperCodecs.Read<T>(i, nestedCodec),
                (ref T? v, T? v2) => { if (v2.HasValue) { v = v2; } return v.HasValue; },
#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
                WrapperCodecs.GetBufferReader<T>(),
                (ref CodedOutputWriter output, T? value) => WrapperCodecs.Write<T>(ref output, value.Value, nestedCodec),
                (ref CodedInputReader i, ref T? v) => v = WrapperCodecs.Read<T>(ref i, nestedCodec),
#endif
                value => value == null ? 0 : WrapperCodecs.CalculateSize<T>(value.Value, nestedCodec),
                tag, 0,
                null); // Default value for the wrapper
        }

        /// <summary>
        /// Helper code to create codecs for wrapper types.
        /// </summary>
        /// <remarks>
        /// Somewhat ugly with all the static methods, but the conversions involved to/from nullable types make it
        /// slightly tricky to improve. So long as we keep the public API (ForClassWrapper, ForStructWrapper) in place,
        /// we can refactor later if we come up with something cleaner.
        /// </remarks>
        private static class WrapperCodecs
        {
            private static readonly Dictionary<System.Type, object> Codecs = new Dictionary<System.Type, object>
            {
                { typeof(bool), ForBool(WireFormat.MakeTag(WrappersReflection.WrapperValueFieldNumber, WireFormat.WireType.Varint)) },
                { typeof(int), ForInt32(WireFormat.MakeTag(WrappersReflection.WrapperValueFieldNumber, WireFormat.WireType.Varint)) },
                { typeof(long), ForInt64(WireFormat.MakeTag(WrappersReflection.WrapperValueFieldNumber, WireFormat.WireType.Varint)) },
                { typeof(uint), ForUInt32(WireFormat.MakeTag(WrappersReflection.WrapperValueFieldNumber, WireFormat.WireType.Varint)) },
                { typeof(ulong), ForUInt64(WireFormat.MakeTag(WrappersReflection.WrapperValueFieldNumber, WireFormat.WireType.Varint)) },
                { typeof(float), ForFloat(WireFormat.MakeTag(WrappersReflection.WrapperValueFieldNumber, WireFormat.WireType.Fixed32)) },
                { typeof(double), ForDouble(WireFormat.MakeTag(WrappersReflection.WrapperValueFieldNumber, WireFormat.WireType.Fixed64)) },
                { typeof(string), ForString(WireFormat.MakeTag(WrappersReflection.WrapperValueFieldNumber, WireFormat.WireType.LengthDelimited)) },
                { typeof(ByteString), ForBytes(WireFormat.MakeTag(WrappersReflection.WrapperValueFieldNumber, WireFormat.WireType.LengthDelimited)) }
            };

            private static readonly Dictionary<System.Type, object> StreamReaders = new Dictionary<System.Type, object>
            {
                // TODO: Provide more optimized readers.
                { typeof(bool), (Func<CodedInputStream, bool?>)CodedInputStream.ReadBoolWrapper },
                { typeof(int), (Func<CodedInputStream, int?>)CodedInputStream.ReadInt32Wrapper },
                { typeof(long), (Func<CodedInputStream, long?>)CodedInputStream.ReadInt64Wrapper },
                { typeof(uint), (Func<CodedInputStream, uint?>)CodedInputStream.ReadUInt32Wrapper },
                { typeof(ulong), (Func<CodedInputStream, ulong?>)CodedInputStream.ReadUInt64Wrapper },
                { typeof(float), BitConverter.IsLittleEndian ?
                    (Func<CodedInputStream, float?>)CodedInputStream.ReadFloatWrapperLittleEndian :
                    (Func<CodedInputStream, float?>)CodedInputStream.ReadFloatWrapperSlow },
                { typeof(double), BitConverter.IsLittleEndian ?
                    (Func<CodedInputStream, double?>)CodedInputStream.ReadDoubleWrapperLittleEndian :
                    (Func<CodedInputStream, double?>)CodedInputStream.ReadDoubleWrapperSlow },
                // `string` and `ByteString` less performance-sensitive. Do not implement for now.
                { typeof(string), null },
                { typeof(ByteString), null },
            };

#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
            private static readonly Dictionary<System.Type, object> BufferReaders = new Dictionary<System.Type, object>
            {
                // TODO: Provide more optimized readers.
                { typeof(bool), (BufferReader<bool?>)CodedInputReader.ReadBoolWrapper },
                { typeof(int), (BufferReader<int?>)CodedInputReader.ReadInt32Wrapper },
                { typeof(long), (BufferReader<long?>)CodedInputReader.ReadInt64Wrapper },
                { typeof(uint), (BufferReader<uint?>)CodedInputReader.ReadUInt32Wrapper },
                { typeof(ulong), (BufferReader<ulong?>)CodedInputReader.ReadUInt64Wrapper },
                { typeof(float), BitConverter.IsLittleEndian ?
                    (BufferReader<float?>)CodedInputReader.ReadFloatWrapperLittleEndian :
                    (BufferReader<float?>)CodedInputReader.ReadFloatWrapperSlow },
                { typeof(double), BitConverter.IsLittleEndian ?
                    (BufferReader<double?>)CodedInputReader.ReadDoubleWrapperLittleEndian :
                    (BufferReader<double?>)CodedInputReader.ReadDoubleWrapperSlow },
                // `string` and `ByteString` less performance-sensitive. Do not implement for now.
                { typeof(string), null },
                { typeof(ByteString), null },
            };
#endif

            /// <summary>
            /// Returns a field codec which effectively wraps a value of type T in a message.
            ///
            /// </summary>
            internal static FieldCodec<T> GetCodec<T>()
            {
                object value;
                if (!Codecs.TryGetValue(typeof(T), out value))
                {
                    throw new InvalidOperationException("Invalid type argument requested for wrapper codec: " + typeof(T));
                }
                return (FieldCodec<T>) value;
            }

            internal static Func<CodedInputStream, T?> GetStreamReader<T>() where T : struct
            {
                object value;
                if (!StreamReaders.TryGetValue(typeof(T), out value))
                {
                    throw new InvalidOperationException("Invalid type argument requested for wrapper reader: " + typeof(T));
                }
                if (value == null)
                {
                    // Return default unoptimized reader for the wrapper type.
                    var nestedCoded = GetCodec<T>();
                    return input => Read<T>(input, nestedCoded);
                }
                // Return optimized read for the wrapper type.
                return (Func<CodedInputStream, T?>)value;
            }

#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
            internal static BufferReader<T?> GetBufferReader<T>() where T : struct
            {
                object value;
                if (!BufferReaders.TryGetValue(typeof(T), out value))
                {
                    throw new InvalidOperationException("Invalid type argument requested for wrapper reader: " + typeof(T));
                }
                if (value == null)
                {
                    // Return default unoptimized reader for the wrapper type.
                    var nestedCoded = GetCodec<T>();
                    return (ref CodedInputReader input) => Read<T>(ref input, nestedCoded);
                }
                // Return optimized read for the wrapper type.
                return (BufferReader<T?>)value;
            }
#endif

            internal static T Read<T>(CodedInputStream input, FieldCodec<T> codec)
            {
                int length = input.ReadLength();
                int oldLimit = input.PushLimit(length);

                uint tag;
                T value = codec.DefaultValue;
                while ((tag = input.ReadTag()) != 0)
                {
                    if (tag == codec.Tag)
                    {
                        value = codec.Read(input);
                    }
                    else
                    {
                        input.SkipLastField();
                    }

                }
                input.CheckReadEndOfStreamTag();
                input.PopLimit(oldLimit);

                return value;
            }

            internal static void Write<T>(CodedOutputStream output, T value, FieldCodec<T> codec)
            {
                output.WriteLength(codec.CalculateSizeWithTag(value));
                codec.WriteTagAndValue(output, value);
            }

#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
            internal static T Read<T>(ref CodedInputReader input, FieldCodec<T> codec)
            {
                int length = input.ReadLength();
                long oldLimit = input.PushLimit(length);

                uint tag;
                T value = codec.DefaultValue;
                while ((tag = input.ReadTag()) != 0)
                {
                    if (tag == codec.Tag)
                    {
                        value = codec.Read(ref input);
                    }
                    else
                    {
                        input.SkipLastField();
                    }

                }
                input.CheckReadEndOfInputTag();
                input.PopLimit(oldLimit);

                return value;
            }

            internal static void Write<T>(ref CodedOutputWriter output, T value, FieldCodec<T> codec)
            {
                output.WriteLength(codec.CalculateSizeWithTag(value));
                codec.WriteTagAndValue(ref output, value);
            }
#endif

            internal  static int CalculateSize<T>(T value, FieldCodec<T> codec)
            {
                int fieldLength = codec.CalculateSizeWithTag(value);
                return CodedOutputStream.ComputeLengthSize(fieldLength) + fieldLength;
            }
        }
    }

#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
    internal delegate void BufferWriter<in TValue>(ref CodedOutputWriter writer, TValue value);
    internal delegate TValue BufferReader<out TValue>(ref CodedInputReader reader);
    internal delegate void BufferInputMerger<TValue>(ref CodedInputReader input, ref TValue value);
#endif

    /// <summary>
    /// <para>
    /// An encode/decode pair for a single field. This effectively encapsulates
    /// all the information needed to read or write the field value from/to a coded
    /// stream.
    /// </para>
    /// <para>
    /// This class is public and has to be as it is used by generated code, but its public
    /// API is very limited - just what the generated code needs to call directly.
    /// </para>
    /// </summary>
    /// <remarks>
    /// This never writes default values to the stream, and does not address "packedness"
    /// in repeated fields itself, other than to know whether or not the field *should* be packed.
    /// </remarks>
    public sealed class FieldCodec<T>
    {
        private static readonly EqualityComparer<T> EqualityComparer = ProtobufEqualityComparers.GetEqualityComparer<T>();
        private static readonly T DefaultDefault;
        // Only non-nullable value types support packing. This is the simplest way of detecting that.
        private static readonly bool TypeSupportsPacking = default(T) != null;

        /// <summary>
        /// Merges an input stream into a value
        /// </summary>
        internal delegate void InputMerger(CodedInputStream input, ref T value);

        /// <summary>
        /// Merges a value into a reference to another value, returning a boolean if the value was set
        /// </summary>
        internal delegate bool ValuesMerger(ref T value, T other);

        static FieldCodec()
        {
            if (typeof(T) == typeof(string))
            {
                DefaultDefault = (T)(object)"";
            }
            else if (typeof(T) == typeof(ByteString))
            {
                DefaultDefault = (T)(object)ByteString.Empty;
            }
            // Otherwise it's the default value of the CLR type
        }

        internal static bool IsPackedRepeatedField(uint tag) =>
            TypeSupportsPacking && WireFormat.GetTagWireType(tag) == WireFormat.WireType.LengthDelimited;

        internal bool PackedRepeatedField { get; }

        /// <summary>
        /// Returns a delegate to write a value (unconditionally) to a coded output stream.
        /// </summary>
        internal Action<CodedOutputStream, T> ValueWriter { get; }

#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
        /// <summary>
        /// Returns a delegate to write a value (unconditionally) to a coded output writer.
        /// </summary>
        internal BufferWriter<T> BufferValueWriter { get; }
#endif

        /// <summary>
        /// Returns the size calculator for just a value.
        /// </summary>
        internal Func<T, int> ValueSizeCalculator { get; }

        /// <summary>
        /// Returns a delegate to read a value from a coded input stream. It is assumed that
        /// the stream is already positioned on the appropriate tag.
        /// </summary>
        internal Func<CodedInputStream, T> ValueReader { get; }

#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
        /// <summary>
        /// Returns a delegate to read a value from a coded input reader. It is assumed that
        /// the reader is already positioned on the appropriate tag.
        /// </summary>
        internal BufferReader<T> BufferValueReader { get; }
#endif

        /// <summary>
        /// Returns a delegate to merge a value from a coded input stream.
        /// It is assumed that the stream is already positioned on the appropriate tag
        /// </summary>
        internal InputMerger ValueMerger { get; }

#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
        /// <summary>
        /// Returns a delegate to merge a value from a coded input stream.
        /// It is assumed that the stream is already positioned on the appropriate tag
        /// </summary>
        internal BufferInputMerger<T> BufferValueMerger { get; }
#endif

        /// <summary>
        /// Returns a delegate to merge two values together.
        /// </summary>
        internal ValuesMerger FieldMerger { get; }

        /// <summary>
        /// Returns the fixed size for an entry, or 0 if sizes vary.
        /// </summary>
        internal int FixedSize { get; }

        /// <summary>
        /// Gets the tag of the codec.
        /// </summary>
        /// <value>
        /// The tag of the codec.
        /// </value>
        internal uint Tag { get; }

        /// <summary>
        /// Gets the end tag of the codec or 0 if there is no end tag
        /// </summary>
        /// <value>
        /// The end tag of the codec.
        /// </value>
        internal uint EndTag { get; }

        /// <summary>
        /// Default value for this codec. Usually the same for every instance of the same type, but
        /// for string/ByteString wrapper fields the codec's default value is null, whereas for
        /// other string/ByteString fields it's "" or ByteString.Empty.
        /// </summary>
        /// <value>
        /// The default value of the codec's type.
        /// </value>
        internal T DefaultValue { get; }

        private readonly int tagSize;

        internal FieldCodec(
                Func<CodedInputStream, T> reader,
                Action<CodedOutputStream, T> writer,
#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
                BufferReader<T> bufferReader,
                BufferWriter<T> bufferWriter,
#endif
                int fixedSize,
                uint tag,
                T defaultValue) : this(
                    reader,
                    writer,
#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
                    bufferReader,
                    bufferWriter,
#endif
                    _ => fixedSize,
                    tag,
                    defaultValue)
        {
            FixedSize = fixedSize;
        }

        internal FieldCodec(
            Func<CodedInputStream, T> reader,
            Action<CodedOutputStream, T> writer,
#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
            BufferReader<T> bufferReader,
            BufferWriter<T> bufferWriter,
#endif
            Func<T, int> sizeCalculator,
            uint tag,
            T defaultValue) : this(
                reader,
                writer,
                (CodedInputStream i, ref T v) => v = reader(i),
                (ref T v, T v2) => { v = v2; return true; },
#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
                bufferReader,
                bufferWriter,
                (ref CodedInputReader i, ref T v) => v = bufferReader(ref i),
#endif
                sizeCalculator,
                tag,
                0,
                defaultValue)
        {
        }

        internal FieldCodec(
            Func<CodedInputStream, T> reader,
            Action<CodedOutputStream, T> writer,
            InputMerger inputMerger,
            ValuesMerger valuesMerger,
#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
            BufferReader<T> bufferReader,
            BufferWriter<T> bufferWriter,
            BufferInputMerger<T> bufferInputMerger,
#endif
            Func<T, int> sizeCalculator,
            uint tag,
            uint endTag = 0) : this(
                reader,
                writer,
                inputMerger,
                valuesMerger,
#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
                bufferReader,
                bufferWriter,
                bufferInputMerger,
#endif
                sizeCalculator,
                tag,
                endTag,
                DefaultDefault)
        {
        }

        internal FieldCodec(
            Func<CodedInputStream, T> reader,
            Action<CodedOutputStream, T> writer,
            InputMerger inputMerger,
            ValuesMerger valuesMerger,
#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
            BufferReader<T> bufferReader,
            BufferWriter<T> bufferWriter,
            BufferInputMerger<T> bufferInputMerger,
#endif
            Func<T, int> sizeCalculator,
            uint tag,
            uint endTag,
            T defaultValue)
        {
            ValueReader = reader;
            ValueWriter = writer;
            ValueMerger = inputMerger;
            FieldMerger = valuesMerger;
#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
            BufferValueReader = bufferReader;
            BufferValueWriter = bufferWriter;
            BufferValueMerger = bufferInputMerger;
#endif
            ValueSizeCalculator = sizeCalculator;
            FixedSize = 0;
            Tag = tag;
            EndTag = endTag;
            DefaultValue = defaultValue;
            tagSize = CodedOutputStream.ComputeRawVarint32Size(tag);
            if (endTag != 0)
                tagSize += CodedOutputStream.ComputeRawVarint32Size(endTag);
            // Detect packed-ness once, so we can check for it within RepeatedField<T>.
            PackedRepeatedField = IsPackedRepeatedField(tag);
        }

        /// <summary>
        /// Write a tag and the given value, *if* the value is not the default.
        /// </summary>
        public void WriteTagAndValue(CodedOutputStream output, T value)
        {
            if (!IsDefault(value))
            {
                output.WriteTag(Tag);
                ValueWriter(output, value);
                if (EndTag != 0)
                {
                    output.WriteTag(EndTag);
                }
            }
        }

#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
        /// <summary>
        /// Write a tag and the given value, *if* the value is not the default.
        /// </summary>
        public void WriteTagAndValue(ref CodedOutputWriter output, T value)
        {
            if (!IsDefault(value))
            {
                output.WriteTag(Tag);
                BufferValueWriter(ref output, value);
                if (EndTag != 0)
                {
                    output.WriteTag(EndTag);
                }
            }
        }
#endif

        /// <summary>
        /// Reads a value of the codec type from the given <see cref="CodedInputStream"/>.
        /// </summary>
        /// <param name="input">The input stream to read from.</param>
        /// <returns>The value read from the stream.</returns>
        public T Read(CodedInputStream input) => ValueReader(input);

#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
        /// <summary>
        /// Reads a value of the codec type from the given <see cref="CodedInputReader"/>.
        /// </summary>
        /// <param name="input">The input stream to read from.</param>
        /// <returns>The value read from the stream.</returns>
        public T Read(ref CodedInputReader input) => BufferValueReader(ref input);
#endif

        /// <summary>
        /// Calculates the size required to write the given value, with a tag,
        /// if the value is not the default.
        /// </summary>
        public int CalculateSizeWithTag(T value) => IsDefault(value) ? 0 : ValueSizeCalculator(value) + tagSize;

        private bool IsDefault(T value) => EqualityComparer.Equals(value, DefaultValue);
    }
}
