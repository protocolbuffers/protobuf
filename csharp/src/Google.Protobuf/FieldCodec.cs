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

        /// <summary>
        /// Retrieves a codec suitable for an enum field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<T> ForEnum<T>(uint tag) where T : System.Enum
        {
            return ForEnum<T>(tag, default);
        }

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
            return new StringFieldCodec(tag, defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for a bytes field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<ByteString> ForBytes(uint tag, ByteString defaultValue)
        {
            return new BytesFieldCodec(tag, defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for a bool field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<bool> ForBool(uint tag, bool defaultValue)
        {
            return new BoolFieldCodec(tag, defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for an int32 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<int> ForInt32(uint tag, int defaultValue)
        {
            return new Int32FieldCodec(tag, defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for an sint32 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<int> ForSInt32(uint tag, int defaultValue)
        {
            return new SInt32FieldCodec(tag, defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for a fixed32 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<uint> ForFixed32(uint tag, uint defaultValue)
        {
            return new Fixed32FieldCodec(tag, defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for an sfixed32 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<int> ForSFixed32(uint tag, int defaultValue)
        {
            return new SFixed32FieldCodec(tag, defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for a uint32 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<uint> ForUInt32(uint tag, uint defaultValue)
        {
            return new UInt32FieldCodec(tag, defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for an int64 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<long> ForInt64(uint tag, long defaultValue)
        {
            return new Int64FieldCodec(tag, defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for an sint64 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<long> ForSInt64(uint tag, long defaultValue)
        {
            return new SInt64FieldCodec(tag, defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for a fixed64 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<ulong> ForFixed64(uint tag, ulong defaultValue)
        {
            return new Fixed64FieldCodec(tag, defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for an sfixed64 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<long> ForSFixed64(uint tag, long defaultValue)
        {
            return new SFixed64FieldCodec(tag, defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for a uint64 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<ulong> ForUInt64(uint tag, ulong defaultValue)
        {
            return new UInt64FieldCodec(tag, defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for a float field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<float> ForFloat(uint tag, float defaultValue)
        {
            return new FloatFieldCodec(tag, defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for a double field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<double> ForDouble(uint tag, double defaultValue)
        {
            return new DoubleFieldCodec(tag, defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for an enum field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<T> ForEnum<T>(uint tag, T defaultValue) where T : System.Enum
        {
            return new EnumFieldCodec<T>(tag, defaultValue);
        }

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
            return new LegacyEnumFieldCodec<T>(tag, toInt32, fromInt32, defaultValue);
        }

        /// <summary>
        /// Retrieves a codec suitable for a message field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="parser">A parser to use for the message type.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<T> ForMessage<T>(uint tag, MessageParser<T> parser) where T : class, IMessage<T>
        {
            return new MessageFieldCodec<T>(tag, parser);
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
            return new GroupFieldCodec<T>(startTag, endTag, parser);
        }

        /// <summary>
        /// Creates a codec for a wrapper type of a class - which must be string or ByteString.
        /// </summary>
        public static FieldCodec<T> ForClassWrapper<T>(uint tag) where T : class
        {
            return new ClassWrapperFieldCodec<T>(tag);
        }

        /// <summary>
        /// Creates a codec for a wrapper type of a struct - which must be Int32, Int64, UInt32, UInt64,
        /// Bool, Single or Double.
        /// </summary>
        public static FieldCodec<T?> ForStructWrapper<T>(uint tag) where T : struct
        {
            return new StructWrapperFieldCodec<T>(tag);
        }

        internal abstract class FixedSizeFieldCodec<T> : FieldCodec<T>
        {
            internal FixedSizeFieldCodec(int fixedSize, uint tag, T defaultValue) : base(fixedSize, tag, defaultValue)
            {
            }

            internal sealed override int ValueSizeCalculator(T value)
            {
                return FixedSize;
            }
        }

        internal sealed class StringFieldCodec : FieldCodec<string>
        {
            internal StringFieldCodec(uint tag, string defaultValue) : base(tag, defaultValue)
            {
            }

            internal override void ValueWriter(ref WriteContext ctx, string value)
            {
                ctx.WriteString(value);
            }

            internal override int ValueSizeCalculator(string value)
            {
                return CodedOutputStream.ComputeStringSize(value);
            }

            internal override string ValueReader(ref ParseContext ctx)
            {
                return ctx.ReadString();
            }
        }

        internal sealed class BytesFieldCodec : FieldCodec<ByteString>
        {
            internal BytesFieldCodec(uint tag, ByteString defaultValue) : base(tag, defaultValue)
            {
            }

            internal override void ValueWriter(ref WriteContext ctx, ByteString value)
            {
                ctx.WriteBytes(value);
            }

            internal override int ValueSizeCalculator(ByteString value)
            {
                return CodedOutputStream.ComputeBytesSize(value);
            }

            internal override ByteString ValueReader(ref ParseContext ctx)
            {
                return ctx.ReadBytes();
            }
        }

        internal sealed class BoolFieldCodec : FixedSizeFieldCodec<bool>
        {
            internal BoolFieldCodec(uint tag, bool defaultValue) : base(CodedOutputStream.BoolSize, tag, defaultValue)
            {
            }

            internal override void ValueWriter(ref WriteContext ctx, bool value)
            {
                ctx.WriteBool(value);
            }

            internal override bool ValueReader(ref ParseContext ctx)
            {
                return ctx.ReadBool();
            }
        }

        internal sealed class Int32FieldCodec : FieldCodec<int>
        {
            internal Int32FieldCodec(uint tag, int defaultValue) : base(tag, defaultValue)
            {
            }

            internal override void ValueWriter(ref WriteContext ctx, int value)
            {
                ctx.WriteInt32(value);
            }

            internal override int ValueSizeCalculator(int value)
            {
                return CodedOutputStream.ComputeInt32Size(value);
            }

            internal override int ValueReader(ref ParseContext ctx)
            {
                return ctx.ReadInt32();
            }
        }

        internal sealed class SInt32FieldCodec : FieldCodec<int>
        {
            internal SInt32FieldCodec(uint tag, int defaultValue) : base(tag, defaultValue)
            {
            }

            internal override void ValueWriter(ref WriteContext ctx, int value)
            {
                ctx.WriteSInt32(value);
            }

            internal override int ValueSizeCalculator(int value)
            {
                return CodedOutputStream.ComputeSInt32Size(value);
            }

            internal override int ValueReader(ref ParseContext ctx)
            {
                return ctx.ReadSInt32();
            }
        }

        internal sealed class Fixed32FieldCodec : FixedSizeFieldCodec<uint>
        {
            internal Fixed32FieldCodec(uint tag, uint defaultValue) : base(4, tag, defaultValue)
            {
            }

            internal override void ValueWriter(ref WriteContext ctx, uint value)
            {
                ctx.WriteFixed32(value);
            }

            internal override uint ValueReader(ref ParseContext ctx)
            {
                return ctx.ReadFixed32();
            }
        }

        internal sealed class SFixed32FieldCodec : FixedSizeFieldCodec<int>
        {
            internal SFixed32FieldCodec(uint tag, int defaultValue) : base(4, tag, defaultValue)
            {
            }

            internal override void ValueWriter(ref WriteContext ctx, int value)
            {
                ctx.WriteSFixed32(value);
            }

            internal override int ValueReader(ref ParseContext ctx)
            {
                return ctx.ReadSFixed32();
            }
        }

        internal sealed class UInt32FieldCodec : FieldCodec<uint>
        {
            internal UInt32FieldCodec(uint tag, uint defaultValue) : base(tag, defaultValue)
            {
            }

            internal override void ValueWriter(ref WriteContext ctx, uint value)
            {
                ctx.WriteUInt32(value);
            }

            internal override int ValueSizeCalculator(uint value)
            {
                return CodedOutputStream.ComputeUInt32Size(value);
            }

            internal override uint ValueReader(ref ParseContext ctx)
            {
                return ctx.ReadUInt32();
            }
        }

        internal sealed class Int64FieldCodec : FieldCodec<long>
        {
            internal Int64FieldCodec(uint tag, long defaultValue) : base(tag, defaultValue)
            {
            }

            internal override void ValueWriter(ref WriteContext ctx, long value)
            {
                ctx.WriteInt64(value);
            }

            internal override int ValueSizeCalculator(long value)
            {
                return CodedOutputStream.ComputeInt64Size(value);
            }

            internal override long ValueReader(ref ParseContext ctx)
            {
                return ctx.ReadInt64();
            }
        }

        internal sealed class SInt64FieldCodec : FieldCodec<long>
        {
            internal SInt64FieldCodec(uint tag, long defaultValue) : base(tag, defaultValue)
            {
            }

            internal override void ValueWriter(ref WriteContext ctx, long value)
            {
                ctx.WriteSInt64(value);
            }

            internal override int ValueSizeCalculator(long value)
            {
                return CodedOutputStream.ComputeSInt64Size(value);
            }

            internal override long ValueReader(ref ParseContext ctx)
            {
                return ctx.ReadSInt64();
            }
        }

        internal sealed class Fixed64FieldCodec : FixedSizeFieldCodec<ulong>
        {
            internal Fixed64FieldCodec(uint tag, ulong defaultValue) : base(8, tag, defaultValue)
            {
            }

            internal override void ValueWriter(ref WriteContext ctx, ulong value)
            {
                ctx.WriteFixed64(value);
            }

            internal override ulong ValueReader(ref ParseContext ctx)
            {
                return ctx.ReadFixed64();
            }
        }

        internal sealed class SFixed64FieldCodec : FixedSizeFieldCodec<long>
        {
            internal SFixed64FieldCodec(uint tag, long defaultValue) : base(8, tag, defaultValue)
            {
            }

            internal override void ValueWriter(ref WriteContext ctx, long value)
            {
                ctx.WriteSFixed64(value);
            }

            internal override long ValueReader(ref ParseContext ctx)
            {
                return ctx.ReadSFixed64();
            }
        }

        internal sealed class UInt64FieldCodec : FieldCodec<ulong>
        {
            internal UInt64FieldCodec(uint tag, ulong defaultValue) : base(tag, defaultValue)
            {
            }

            internal override void ValueWriter(ref WriteContext ctx, ulong value)
            {
                ctx.WriteUInt64(value);
            }

            internal override int ValueSizeCalculator(ulong value)
            {
                return CodedOutputStream.ComputeUInt64Size(value);
            }

            internal override ulong ValueReader(ref ParseContext ctx)
            {
                return ctx.ReadUInt64();
            }
        }

        internal sealed class FloatFieldCodec : FixedSizeFieldCodec<float>
        {
            internal FloatFieldCodec(uint tag, float defaultValue) : base(CodedOutputStream.FloatSize, tag, defaultValue)
            {
            }

            internal override void ValueWriter(ref WriteContext ctx, float value)
            {
                ctx.WriteFloat(value);
            }

            internal override float ValueReader(ref ParseContext ctx)
            {
                return ctx.ReadFloat();
            }
        }

        internal sealed class DoubleFieldCodec : FixedSizeFieldCodec<double>
        {
            internal DoubleFieldCodec(uint tag, double defaultValue) : base(CodedOutputStream.DoubleSize, tag, defaultValue)
            {
            }

            internal override void ValueWriter(ref WriteContext ctx, double value)
            {
                ctx.WriteDouble(value);
            }

            internal override double ValueReader(ref ParseContext ctx)
            {
                return ctx.ReadDouble();
            }
        }

        internal sealed class EnumFieldCodec<T> : FieldCodec<T> where T : System.Enum
        {
            internal EnumFieldCodec(uint tag, T defaultValue) : base(tag, defaultValue)
            {
            }

            internal override void ValueWriter(ref WriteContext ctx, T value)
            {
                ctx.WriteEnum((int)(object)value);
            }

            internal override int ValueSizeCalculator(T value)
            {
                return CodedOutputStream.ComputeEnumSize((int)(object)value);
            }

            internal override T ValueReader(ref ParseContext ctx)
            {
                return (T)(object)ctx.ReadEnum();
            }
        }

        internal sealed class LegacyEnumFieldCodec<T> : FieldCodec<T>
        {
            private readonly Func<T, int> toInt32;
            private readonly Func<int, T> fromInt32;

            internal LegacyEnumFieldCodec(uint tag, Func<T, int> toInt32, Func<int, T> fromInt32, T defaultValue) : base(tag, defaultValue)
            {
                this.toInt32 = toInt32;
                this.fromInt32 = fromInt32;
            }

            internal override void ValueWriter(ref WriteContext ctx, T value)
            {
                ctx.WriteEnum(toInt32(value));
            }

            internal override int ValueSizeCalculator(T value)
            {
                return CodedOutputStream.ComputeEnumSize(toInt32(value));
            }

            internal override T ValueReader(ref ParseContext ctx)
            {
                return fromInt32(ctx.ReadEnum());
            }
        }

        internal sealed class MessageFieldCodec<T> : FieldCodec<T> where T : class, IMessage<T>
        {
            private readonly MessageParser<T> parser;

            internal MessageFieldCodec(uint tag, MessageParser<T> parser) : base(tag, null)
            {
                this.parser = parser;
            }

            internal override void ValueWriter(ref WriteContext ctx, T value)
            {
                ctx.WriteMessage(value);
            }

            internal override int ValueSizeCalculator(T value)
            {
                return CodedOutputStream.ComputeMessageSize(value);
            }

            internal override T ValueReader(ref ParseContext ctx)
            {
                T value = parser.CreateTemplate();
                ctx.ReadMessage(value);
                return value;
            }

            internal override void ValueMerger(ref ParseContext ctx, ref T value)
            {
                value ??= parser.CreateTemplate();
                ctx.ReadMessage(value);
            }

            internal override bool FieldMerger(ref T value, T other)
            {
                if (other == null)
                {
                    return false;
                }
                else if (value == null)
                {
                    value = other.Clone();
                }
                else
                {
                    value.MergeFrom(other);
                }

                return true;
            }
        }

        internal sealed class GroupFieldCodec<T> : FieldCodec<T> where T : class, IMessage<T>
        {
            private readonly MessageParser<T> parser;

            internal GroupFieldCodec(uint startTag, uint endTag, MessageParser<T> parser) : base(startTag, endTag, null)
            {
                this.parser = parser;
            }

            internal override void ValueWriter(ref WriteContext ctx, T value)
            {
                ctx.WriteGroup(value);
            }

            internal override int ValueSizeCalculator(T value)
            {
                return CodedOutputStream.ComputeGroupSize(value);
            }

            internal override T ValueReader(ref ParseContext ctx)
            {
                T value = parser.CreateTemplate();
                ctx.ReadGroup(value);
                return value;
            }

            internal override void ValueMerger(ref ParseContext ctx, ref T value)
            {
                value ??= parser.CreateTemplate();
                ctx.ReadGroup(value);
            }

            internal override bool FieldMerger(ref T value, T other)
            {
                if (other == null)
                {
                    return value == null;
                }
                else if (value == null)
                {
                    value = other.Clone();
                }
                else
                {
                    value.MergeFrom(other);
                }

                return true;
            }
        }

        internal sealed class ClassWrapperFieldCodec<T> : FieldCodec<T> where T : class
        {
            private readonly FieldCodec<T> nestedCodec;

            internal ClassWrapperFieldCodec(uint tag) : base(tag, null)
            {
                nestedCodec = WrapperCodecs.GetCodec<T>();
            }

            internal override void ValueWriter(ref WriteContext ctx, T value)
            {
                WrapperCodecs.Write(ref ctx, value, nestedCodec);
            }

            internal override int ValueSizeCalculator(T value)
            {
                return WrapperCodecs.CalculateSize(value, nestedCodec);
            }

            internal override T ValueReader(ref ParseContext ctx)
            {
                return WrapperCodecs.Read(ref ctx, nestedCodec);
            }

            internal override void ValueMerger(ref ParseContext ctx, ref T value)
            {
                value = WrapperCodecs.Read(ref ctx, nestedCodec);
            }

            internal override bool FieldMerger(ref T value, T other)
            {
                value = other;
                return value == null;
            }
        }

        internal sealed class StructWrapperFieldCodec<T> : FieldCodec<T?> where T : struct
        {
            private readonly FieldCodec<T> nestedCodec;
            private readonly ValueReader<T?> reader;

            internal StructWrapperFieldCodec(uint tag) : base(tag, null)
            {
                nestedCodec = WrapperCodecs.GetCodec<T>();
                reader = WrapperCodecs.GetReader<T>();
            }

            internal override void ValueWriter(ref WriteContext ctx, T? value)
            {
                WrapperCodecs.Write(ref ctx, value.Value, nestedCodec);
            }

            internal override int ValueSizeCalculator(T? value)
            {
                return value == null ? 0 : WrapperCodecs.CalculateSize(value.Value, nestedCodec);
            }

            internal override T? ValueReader(ref ParseContext ctx)
            {
                return reader(ref ctx);
            }

            internal override void ValueMerger(ref ParseContext ctx, ref T? value)
            {
                value = WrapperCodecs.Read(ref ctx, nestedCodec);
            }

            internal override bool FieldMerger(ref T? value, T? other)
            {
                if (other.HasValue)
                {
                    value = other;
                }

                return value.HasValue;
            }
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
                    return (ref ParseContext ctx) => Read(ref ctx, nestedCoded);
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

            internal static int CalculateSize<T>(T value, FieldCodec<T> codec)
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
    public abstract class FieldCodec<T>
    {
        private static readonly EqualityComparer<T> EqualityComparer = ProtobufEqualityComparers.GetEqualityComparer<T>();

        // Only non-nullable value types support packing. This is the simplest way of detecting that.
        private static readonly bool TypeSupportsPacking = default(T) != null;

        internal static bool IsPackedRepeatedField(uint tag) =>
            TypeSupportsPacking && WireFormat.GetTagWireType(tag) == WireFormat.WireType.LengthDelimited;

        internal bool PackedRepeatedField { get; }

        /// <summary>
        /// Writes a value (unconditionally) to a coded output stream.
        /// </summary>
        internal abstract void ValueWriter(ref WriteContext ctx, T value);

        /// <summary>
        /// Calculates the size for just a value.
        /// </summary>
        internal abstract int ValueSizeCalculator(T value);

        /// <summary>
        /// Reads a value from a coded input stream. It is assumed that
        /// the stream is already positioned on the appropriate tag.
        /// </summary>
        internal abstract T ValueReader(ref ParseContext ctx);

        /// <summary>
        /// Merges a value from a coded input stream.
        /// It is assumed that the stream is already positioned on the appropriate tag
        /// </summary>
        internal virtual void ValueMerger(ref ParseContext ctx, ref T value)
        {
            value = ValueReader(ref ctx);
        }

        /// <summary>
        /// Merges two values together.
        /// </summary>
        internal virtual bool FieldMerger(ref T value, T other)
        {
            value = other;
            return true;
        }

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

        internal FieldCodec(int fixedSize, uint tag, T defaultValue) : this(tag, defaultValue)
        {
            FixedSize = fixedSize;
        }

        internal FieldCodec(uint tag, T defaultValue) : this(tag, 0, defaultValue)
        {
        }

        internal FieldCodec(uint tag, uint endTag, T defaultValue)
        {
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
