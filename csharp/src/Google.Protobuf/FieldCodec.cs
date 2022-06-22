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
using Google.Protobuf.WellKnownTypes;
using System;
using System.Collections.Generic;
using System.Security;

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
        public static FieldCodec<string> ForString(uint tag) => ForString(tag, "");

        /// <summary>
        /// Retrieves a codec suitable for a bytes field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<ByteString> ForBytes(uint tag) => ForBytes(tag, ByteString.Empty);

        /// <summary>
        /// Retrieves a codec suitable for a bool field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<bool> ForBool(uint tag) => ForBool(tag, false);

        /// <summary>
        /// Retrieves a codec suitable for an int32 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<int> ForInt32(uint tag) => ForInt32(tag, 0);

        /// <summary>
        /// Retrieves a codec suitable for an sint32 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<int> ForSInt32(uint tag) => ForSInt32(tag, 0);

        /// <summary>
        /// Retrieves a codec suitable for a fixed32 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<uint> ForFixed32(uint tag) => ForFixed32(tag, 0);

        /// <summary>
        /// Retrieves a codec suitable for an sfixed32 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<int> ForSFixed32(uint tag) => ForSFixed32(tag, 0);

        /// <summary>
        /// Retrieves a codec suitable for a uint32 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<uint> ForUInt32(uint tag) => ForUInt32(tag, 0);

        /// <summary>
        /// Retrieves a codec suitable for an int64 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<long> ForInt64(uint tag) => ForInt64(tag, 0);

        /// <summary>
        /// Retrieves a codec suitable for an sint64 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<long> ForSInt64(uint tag) => ForSInt64(tag, 0);

        /// <summary>
        /// Retrieves a codec suitable for a fixed64 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<ulong> ForFixed64(uint tag) => ForFixed64(tag, 0);

        /// <summary>
        /// Retrieves a codec suitable for an sfixed64 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<long> ForSFixed64(uint tag) => ForSFixed64(tag, 0);

        /// <summary>
        /// Retrieves a codec suitable for a uint64 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<ulong> ForUInt64(uint tag) => ForUInt64(tag, 0);

        /// <summary>
        /// Retrieves a codec suitable for a float field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<float> ForFloat(uint tag) => ForFloat(tag, 0);

        /// <summary>
        /// Retrieves a codec suitable for a double field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<double> ForDouble(uint tag) => ForDouble(tag, 0);

        // Enums are tricky. We can probably use expression trees to build these delegates automatically,
        // but it's easy to generate the code for it.

        /// <summary>
        /// Retrieves a codec suitable for an enum field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="toInt32">A conversion function from <see cref="Int32"/> to the enum type.</param>
        /// <param name="fromInt32">A conversion function from the enum type to <see cref="Int32"/>.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<T> ForEnum<T>(uint tag, Func<T, int> toInt32, Func<int, T> fromInt32) =>
            ForEnum(tag, toInt32, fromInt32, default);

        /// <summary>
        /// Retrieves a codec suitable for a string field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<string> ForString(uint tag, string defaultValue)
        {
            return new FieldCodec<string>((ref ParseContext ctx) => ctx.ReadString(), (ref WriteContext ctx, string value) => ctx.WriteString(value), CodedOutputStream.ComputeStringSize, tag, defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for a bytes field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<ByteString> ForBytes(uint tag, ByteString defaultValue)
        {
            return new FieldCodec<ByteString>((ref ParseContext ctx) => ctx.ReadBytes(), (ref WriteContext ctx, ByteString value) => ctx.WriteBytes(value), CodedOutputStream.ComputeBytesSize, tag, defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for a bool field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<bool> ForBool(uint tag, bool defaultValue)
        {
            return new FieldCodec<bool>((ref ParseContext ctx) => ctx.ReadBool(), (ref WriteContext ctx, bool value) => ctx.WriteBool(value), CodedOutputStream.BoolSize, tag, defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for an int32 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<int> ForInt32(uint tag, int defaultValue)
        {
            return new FieldCodec<int>((ref ParseContext ctx) => ctx.ReadInt32(), (ref WriteContext output, int value) => output.WriteInt32(value), CodedOutputStream.ComputeInt32Size, tag, defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for an sint32 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<int> ForSInt32(uint tag, int defaultValue)
        {
            return new FieldCodec<int>((ref ParseContext ctx) => ctx.ReadSInt32(), (ref WriteContext output, int value) => output.WriteSInt32(value), CodedOutputStream.ComputeSInt32Size, tag, defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for a fixed32 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<uint> ForFixed32(uint tag, uint defaultValue)
        {
            return new FieldCodec<uint>((ref ParseContext ctx) => ctx.ReadFixed32(), (ref WriteContext output, uint value) => output.WriteFixed32(value), 4, tag, defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for an sfixed32 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<int> ForSFixed32(uint tag, int defaultValue)
        {
            return new FieldCodec<int>((ref ParseContext ctx) => ctx.ReadSFixed32(), (ref WriteContext output, int value) => output.WriteSFixed32(value), 4, tag, defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for a uint32 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<uint> ForUInt32(uint tag, uint defaultValue)
        {
            return new FieldCodec<uint>((ref ParseContext ctx) => ctx.ReadUInt32(), (ref WriteContext output, uint value) => output.WriteUInt32(value), CodedOutputStream.ComputeUInt32Size, tag, defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for an int64 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<long> ForInt64(uint tag, long defaultValue)
        {
            return new FieldCodec<long>((ref ParseContext ctx) => ctx.ReadInt64(), (ref WriteContext output, long value) => output.WriteInt64(value), CodedOutputStream.ComputeInt64Size, tag, defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for an sint64 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<long> ForSInt64(uint tag, long defaultValue)
        {
            return new FieldCodec<long>((ref ParseContext ctx) => ctx.ReadSInt64(), (ref WriteContext output, long value) => output.WriteSInt64(value), CodedOutputStream.ComputeSInt64Size, tag, defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for a fixed64 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<ulong> ForFixed64(uint tag, ulong defaultValue)
        {
            return new FieldCodec<ulong>((ref ParseContext ctx) => ctx.ReadFixed64(), (ref WriteContext output, ulong value) => output.WriteFixed64(value), 8, tag, defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for an sfixed64 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<long> ForSFixed64(uint tag, long defaultValue)
        {
            return new FieldCodec<long>((ref ParseContext ctx) => ctx.ReadSFixed64(), (ref WriteContext output, long value) => output.WriteSFixed64(value), 8, tag, defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for a uint64 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<ulong> ForUInt64(uint tag, ulong defaultValue)
        {
            return new FieldCodec<ulong>((ref ParseContext ctx) => ctx.ReadUInt64(), (ref WriteContext output, ulong value) => output.WriteUInt64(value), CodedOutputStream.ComputeUInt64Size, tag, defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for a float field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<float> ForFloat(uint tag, float defaultValue)
        {
            return new FieldCodec<float>((ref ParseContext ctx) => ctx.ReadFloat(), (ref WriteContext output, float value) => output.WriteFloat(value), CodedOutputStream.FloatSize, tag, defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for a double field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<double> ForDouble(uint tag, double defaultValue)
        {
            return new FieldCodec<double>((ref ParseContext ctx) => ctx.ReadDouble(), (ref WriteContext output, double value) => output.WriteDouble(value), CodedOutputStream.DoubleSize, tag, defaultValue);
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
            return new FieldCodec<T>((ref ParseContext ctx) => fromInt32(
                ctx.ReadEnum()),
                (ref WriteContext output, T value) => output.WriteEnum(toInt32(value)),
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
                (ref ParseContext ctx) => 
                { 
                    T message = parser.CreateTemplate(); 
                    ctx.ReadMessage(message); 
                    return message; 
                },
                (ref WriteContext output, T value) => output.WriteMessage(value),
                (ref ParseContext ctx, ref T v) => 
                {
                    if (v == null)
                    {
                        v = parser.CreateTemplate();
                    }

                    ctx.ReadMessage(v);
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
                (ref ParseContext ctx) => 
                { 
                    T message = parser.CreateTemplate();
                    ctx.ReadGroup(message);
                    return message;
                },
                (ref WriteContext output, T value) => output.WriteGroup(value), 
                (ref ParseContext ctx, ref T v) => 
                {
                    if (v == null)
                    {
                        v = parser.CreateTemplate();
                    }

                    ctx.ReadGroup(v);
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
                message => CodedOutputStream.ComputeGroupSize(message), startTag, endTag);
        }

        /// <summary>
        /// Creates a codec for a wrapper type of a class - which must be string or ByteString.
        /// </summary>
        public static FieldCodec<T> ForClassWrapper<T>(uint tag) where T : class
        {
            var nestedCodec = WrapperCodecs.GetCodec<T>();
            return new FieldCodec<T>(
                (ref ParseContext ctx) => WrapperCodecs.Read<T>(ref ctx, nestedCodec),
                (ref WriteContext output, T value) => WrapperCodecs.Write<T>(ref output, value, nestedCodec),
                (ref ParseContext ctx, ref T v) => v = WrapperCodecs.Read<T>(ref ctx, nestedCodec),
                (ref T v, T v2) => { v = v2; return v == null; },
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
                WrapperCodecs.GetReader<T>(),
                (ref WriteContext output, T? value) => WrapperCodecs.Write<T>(ref output, value.Value, nestedCodec),
                (ref ParseContext ctx, ref T? v) => v = WrapperCodecs.Read<T>(ref ctx, nestedCodec),
                (ref T? v, T? v2) => { if (v2.HasValue) { v = v2; } return v.HasValue; },
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

            private static readonly Dictionary<System.Type, object> Readers = new Dictionary<System.Type, object>
            {
                // TODO: Provide more optimized readers.
                { typeof(bool), (ValueReader<bool?>)ParsingPrimitivesWrappers.ReadBoolWrapper },
                { typeof(int), (ValueReader<int?>)ParsingPrimitivesWrappers.ReadInt32Wrapper },
                { typeof(long), (ValueReader<long?>)ParsingPrimitivesWrappers.ReadInt64Wrapper },
                { typeof(uint), (ValueReader<uint?>)ParsingPrimitivesWrappers.ReadUInt32Wrapper },
                { typeof(ulong), (ValueReader<ulong?>)ParsingPrimitivesWrappers.ReadUInt64Wrapper },
                { typeof(float), BitConverter.IsLittleEndian ?
                    (ValueReader<float?>)ParsingPrimitivesWrappers.ReadFloatWrapperLittleEndian :
                    (ValueReader<float?>)ParsingPrimitivesWrappers.ReadFloatWrapperSlow },
                { typeof(double), BitConverter.IsLittleEndian ?
                    (ValueReader<double?>)ParsingPrimitivesWrappers.ReadDoubleWrapperLittleEndian :
                    (ValueReader<double?>)ParsingPrimitivesWrappers.ReadDoubleWrapperSlow },
                // `string` and `ByteString` less performance-sensitive. Do not implement for now.
                { typeof(string), null },
                { typeof(ByteString), null },
            };

            /// <summary>
            /// Returns a field codec which effectively wraps a value of type T in a message.
            ///
            /// </summary>
            internal static FieldCodec<T> GetCodec<T>()
            {
                if (!Codecs.TryGetValue(typeof(T), out object value))
                {
                    throw new InvalidOperationException("Invalid type argument requested for wrapper codec: " + typeof(T));
                }
                return (FieldCodec<T>) value;
            }

            internal static ValueReader<T?> GetReader<T>() where T : struct
            {
                if (!Readers.TryGetValue(typeof(T), out object value))
                {
                    throw new InvalidOperationException("Invalid type argument requested for wrapper reader: " + typeof(T));
                }
                if (value == null)
                {
                    // Return default unoptimized reader for the wrapper type.
                    var nestedCoded = GetCodec<T>();
                    return (ref ParseContext ctx) => Read<T>(ref ctx, nestedCoded);
                }
                // Return optimized read for the wrapper type.
                return (ValueReader<T?>)value;
            }

            [SecuritySafeCritical]
            internal static T Read<T>(ref ParseContext ctx, FieldCodec<T> codec)
            {
                int length = ctx.ReadLength();
                int oldLimit = SegmentedBufferHelper.PushLimit(ref ctx.state, length);

                uint tag;
                T value = codec.DefaultValue;
                while ((tag = ctx.ReadTag()) != 0)
                {
                    if (tag == codec.Tag)
                    {
                        value = codec.Read(ref ctx);
                    }
                    else
                    {
                        ParsingPrimitivesMessages.SkipLastField(ref ctx.buffer, ref ctx.state);
                    }

                }
                ParsingPrimitivesMessages.CheckReadEndOfStreamTag(ref ctx.state);
                SegmentedBufferHelper.PopLimit(ref ctx.state, oldLimit);

                return value;
            }

            internal static void Write<T>(ref WriteContext ctx, T value, FieldCodec<T> codec)
            {
                ctx.WriteLength(codec.CalculateSizeWithTag(value));
                codec.WriteTagAndValue(ref ctx, value);
            }

            internal  static int CalculateSize<T>(T value, FieldCodec<T> codec)
            {
                int fieldLength = codec.CalculateSizeWithTag(value);
                return CodedOutputStream.ComputeLengthSize(fieldLength) + fieldLength;
            }
        }
    }

    internal delegate TValue ValueReader<out TValue>(ref ParseContext ctx);
    internal delegate void ValueWriter<T>(ref WriteContext ctx, T value);

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
        internal delegate void InputMerger(ref ParseContext ctx, ref T value);

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
        internal ValueWriter<T> ValueWriter { get; }

        /// <summary>
        /// Returns the size calculator for just a value.
        /// </summary>
        internal Func<T, int> ValueSizeCalculator { get; }

        /// <summary>
        /// Returns a delegate to read a value from a coded input stream. It is assumed that
        /// the stream is already positioned on the appropriate tag.
        /// </summary>
        internal ValueReader<T> ValueReader { get; }

        /// <summary>
        /// Returns a delegate to merge a value from a coded input stream.
        /// It is assumed that the stream is already positioned on the appropriate tag
        /// </summary>
        internal InputMerger ValueMerger { get; }

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
                ValueReader<T> reader,
                ValueWriter<T> writer,
                int fixedSize,
                uint tag,
                T defaultValue) : this(reader, writer, _ => fixedSize, tag, defaultValue)
        {
            FixedSize = fixedSize;
        }

        internal FieldCodec(
            ValueReader<T> reader,
            ValueWriter<T> writer,
            Func<T, int> sizeCalculator,
            uint tag,
            T defaultValue) : this(reader, writer, (ref ParseContext ctx, ref T v) => v = reader(ref ctx), (ref T v, T v2) => { v = v2; return true; }, sizeCalculator, tag, 0, defaultValue)
        {
        }

        internal FieldCodec(
            ValueReader<T> reader,
            ValueWriter<T> writer,
            InputMerger inputMerger,
            ValuesMerger valuesMerger,
            Func<T, int> sizeCalculator,
            uint tag,
            uint endTag = 0) : this(reader, writer, inputMerger, valuesMerger, sizeCalculator, tag, endTag, DefaultDefault)
        {
        }

        internal FieldCodec(
            ValueReader<T> reader,
            ValueWriter<T> writer,
            InputMerger inputMerger,
            ValuesMerger valuesMerger,
            Func<T, int> sizeCalculator,
            uint tag,
            uint endTag,
            T defaultValue)
        {
            ValueReader = reader;
            ValueWriter = writer;
            ValueMerger = inputMerger;
            FieldMerger = valuesMerger;
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
            WriteContext.Initialize(output, out WriteContext ctx);
            try
            {
                WriteTagAndValue(ref ctx, value);
            }
            finally
            {
                ctx.CopyStateTo(output);
            }


            //if (!IsDefault(value))
            //{
            //    output.WriteTag(Tag);
            //    ValueWriter(output, value);
            //    if (EndTag != 0)
            //    {
            //        output.WriteTag(EndTag);
            //    }
            //}
        }

        /// <summary>
        /// Write a tag and the given value, *if* the value is not the default.
        /// </summary>
        public void WriteTagAndValue(ref WriteContext ctx, T value)
        {
            if (!IsDefault(value))
            {
                ctx.WriteTag(Tag);
                ValueWriter(ref ctx, value);
                if (EndTag != 0)
                {
                    ctx.WriteTag(EndTag);
                }
            }
        }

        /// <summary>
        /// Reads a value of the codec type from the given <see cref="CodedInputStream"/>.
        /// </summary>
        /// <param name="input">The input stream to read from.</param>
        /// <returns>The value read from the stream.</returns>
        public T Read(CodedInputStream input)
        {
            ParseContext.Initialize(input, out ParseContext ctx);
            try
            {
                return ValueReader(ref ctx);
            }
            finally
            {
                ctx.CopyStateTo(input);
            }
        }

        /// <summary>
        /// Reads a value of the codec type from the given <see cref="ParseContext"/>.
        /// </summary>
        /// <param name="ctx">The parse context to read from.</param>
        /// <returns>The value read.</returns>
        public T Read(ref ParseContext ctx)
        {
            return ValueReader(ref ctx);
        }

        /// <summary>
        /// Calculates the size required to write the given value, with a tag,
        /// if the value is not the default.
        /// </summary>
        public int CalculateSizeWithTag(T value) => IsDefault(value) ? 0 : ValueSizeCalculator(value) + tagSize;

        /// <summary>
        /// Calculates the size required to write the given value, with a tag, even
        /// if the value is the default.
        /// </summary>
        internal int CalculateUnconditionalSizeWithTag(T value) => ValueSizeCalculator(value) + tagSize;

        private bool IsDefault(T value) => EqualityComparer.Equals(value, DefaultValue);
    }
}
