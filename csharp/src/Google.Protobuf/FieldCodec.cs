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
            return new FieldCodec<string>(input => input.ReadString(), (output, value) => output.WriteString(value), CodedOutputStream.ComputeStringSize, tag);
        }

        /// <summary>
        /// Retrieves a codec suitable for a bytes field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<ByteString> ForBytes(uint tag)
        {
            return new FieldCodec<ByteString>(input => input.ReadBytes(), (output, value) => output.WriteBytes(value), CodedOutputStream.ComputeBytesSize, tag);
        }

        /// <summary>
        /// Retrieves a codec suitable for a bool field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<bool> ForBool(uint tag)
        {
            return new FieldCodec<bool>(input => input.ReadBool(), (output, value) => output.WriteBool(value), CodedOutputStream.ComputeBoolSize, tag);
        }

        /// <summary>
        /// Retrieves a codec suitable for an int32 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<int> ForInt32(uint tag)
        {
            return new FieldCodec<int>(input => input.ReadInt32(), (output, value) => output.WriteInt32(value), CodedOutputStream.ComputeInt32Size, tag);
        }

        /// <summary>
        /// Retrieves a codec suitable for an sint32 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<int> ForSInt32(uint tag)
        {
            return new FieldCodec<int>(input => input.ReadSInt32(), (output, value) => output.WriteSInt32(value), CodedOutputStream.ComputeSInt32Size, tag);
        }

        /// <summary>
        /// Retrieves a codec suitable for a fixed32 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<uint> ForFixed32(uint tag)
        {
            return new FieldCodec<uint>(input => input.ReadFixed32(), (output, value) => output.WriteFixed32(value), 4, tag);
        }

        /// <summary>
        /// Retrieves a codec suitable for an sfixed32 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<int> ForSFixed32(uint tag)
        {
            return new FieldCodec<int>(input => input.ReadSFixed32(), (output, value) => output.WriteSFixed32(value), 4, tag);
        }

        /// <summary>
        /// Retrieves a codec suitable for a uint32 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<uint> ForUInt32(uint tag)
        {
            return new FieldCodec<uint>(input => input.ReadUInt32(), (output, value) => output.WriteUInt32(value), CodedOutputStream.ComputeUInt32Size, tag);
        }

        /// <summary>
        /// Retrieves a codec suitable for an int64 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<long> ForInt64(uint tag)
        {
            return new FieldCodec<long>(input => input.ReadInt64(), (output, value) => output.WriteInt64(value), CodedOutputStream.ComputeInt64Size, tag);
        }

        /// <summary>
        /// Retrieves a codec suitable for an sint64 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<long> ForSInt64(uint tag)
        {
            return new FieldCodec<long>(input => input.ReadSInt64(), (output, value) => output.WriteSInt64(value), CodedOutputStream.ComputeSInt64Size, tag);
        }

        /// <summary>
        /// Retrieves a codec suitable for a fixed64 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<ulong> ForFixed64(uint tag)
        {
            return new FieldCodec<ulong>(input => input.ReadFixed64(), (output, value) => output.WriteFixed64(value), 8, tag);
        }

        /// <summary>
        /// Retrieves a codec suitable for an sfixed64 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<long> ForSFixed64(uint tag)
        {
            return new FieldCodec<long>(input => input.ReadSFixed64(), (output, value) => output.WriteSFixed64(value), 8, tag);
        }

        /// <summary>
        /// Retrieves a codec suitable for a uint64 field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<ulong> ForUInt64(uint tag)
        {
            return new FieldCodec<ulong>(input => input.ReadUInt64(), (output, value) => output.WriteUInt64(value), CodedOutputStream.ComputeUInt64Size, tag);
        }

        /// <summary>
        /// Retrieves a codec suitable for a float field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<float> ForFloat(uint tag)
        {
            return new FieldCodec<float>(input => input.ReadFloat(), (output, value) => output.WriteFloat(value), CodedOutputStream.ComputeFloatSize, tag);
        }

        /// <summary>
        /// Retrieves a codec suitable for a double field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<double> ForDouble(uint tag)
        {
            return new FieldCodec<double>(input => input.ReadDouble(), (output, value) => output.WriteDouble(value), CodedOutputStream.ComputeDoubleSize, tag);
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
            return new FieldCodec<T>(input => fromInt32(
                input.ReadEnum()),
                (output, value) => output.WriteEnum(toInt32(value)),
                value => CodedOutputStream.ComputeEnumSize(toInt32(value)), tag);
        }

        /// <summary>
        /// Retrieves a codec suitable for a message field with the given tag.
        /// </summary>
        /// <param name="tag">The tag.</param>
        /// <param name="parser">A parser to use for the message type.</param>
        /// <returns>A codec for the given tag.</returns>
        public static FieldCodec<T> ForMessage<T>(uint tag, MessageParser<T> parser) where T : IMessage<T>
        {
            return new FieldCodec<T>(input => { T message = parser.CreateTemplate(); input.ReadMessage(message); return message; },
                (output, value) => output.WriteMessage(value), message => CodedOutputStream.ComputeMessageSize(message), tag);
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
                value => WrapperCodecs.CalculateSize<T>(value, nestedCodec),
                tag,
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
                input => WrapperCodecs.Read<T>(input, nestedCodec),
                (output, value) => WrapperCodecs.Write<T>(output, value.Value, nestedCodec),
                value => value == null ? 0 : WrapperCodecs.CalculateSize<T>(value.Value, nestedCodec),
                tag,
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

            internal  static int CalculateSize<T>(T value, FieldCodec<T> codec)
            {
                int fieldLength = codec.CalculateSizeWithTag(value);
                return CodedOutputStream.ComputeLengthSize(fieldLength) + fieldLength;
            }
        }
    }

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
        private static readonly T DefaultDefault;
        private static readonly bool TypeSupportsPacking = typeof(T).IsValueType() && Nullable.GetUnderlyingType(typeof(T)) == null;

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

        /// <summary>
        /// Returns the size calculator for just a value.
        /// </summary>
        internal Func<T, int> ValueSizeCalculator { get; }

        /// <summary>
        /// Returns a delegate to read a value from a coded input stream. It is assumed that
        /// the stream is already positioned on the appropriate tag.
        /// </summary>
        internal Func<CodedInputStream, T> ValueReader { get; }

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
                int fixedSize,
                uint tag) : this(reader, writer, _ => fixedSize, tag)
        {
            FixedSize = fixedSize;
        }

        internal FieldCodec(
            Func<CodedInputStream, T> reader,
            Action<CodedOutputStream, T> writer,
            Func<T, int> sizeCalculator,
            uint tag) : this(reader, writer, sizeCalculator, tag, DefaultDefault)
        {
        }

        internal FieldCodec(
            Func<CodedInputStream, T> reader,
            Action<CodedOutputStream, T> writer,
            Func<T, int> sizeCalculator,
            uint tag,
            T defaultValue)
        {
            ValueReader = reader;
            ValueWriter = writer;
            ValueSizeCalculator = sizeCalculator;
            FixedSize = 0;
            Tag = tag;
            DefaultValue = defaultValue;
            tagSize = CodedOutputStream.ComputeRawVarint32Size(tag);
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
            }
        }

        /// <summary>
        /// Reads a value of the codec type from the given <see cref="CodedInputStream"/>.
        /// </summary>
        /// <param name="input">The input stream to read from.</param>
        /// <returns>The value read from the stream.</returns>
        public T Read(CodedInputStream input) => ValueReader(input);

        /// <summary>
        /// Calculates the size required to write the given value, with a tag,
        /// if the value is not the default.
        /// </summary>
        public int CalculateSizeWithTag(T value) => IsDefault(value) ? 0 : ValueSizeCalculator(value) + tagSize;

        private bool IsDefault(T value) => EqualityComparer<T>.Default.Equals(value, DefaultValue);
    }
}
