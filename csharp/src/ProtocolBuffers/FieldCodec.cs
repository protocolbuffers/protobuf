using System;
using System.Collections.Generic;

namespace Google.Protobuf
{
    /// <summary>
    /// Factory methods for <see cref="FieldCodec{T}"/>.
    /// </summary>
    public static class FieldCodec
    {
        public static FieldCodec<string> ForString(uint tag)
        {
            return new FieldCodec<string>(input => input.ReadString(), (output, value) => output.WriteString(value), CodedOutputStream.ComputeStringSize, tag); 
        }

        public static FieldCodec<ByteString> ForBytes(uint tag)
        {
            return new FieldCodec<ByteString>(input => input.ReadBytes(), (output, value) => output.WriteBytes(value), CodedOutputStream.ComputeBytesSize, tag);
        }

        public static FieldCodec<bool> ForBool(uint tag)
        {
            return new FieldCodec<bool>(input => input.ReadBool(), (output, value) => output.WriteBool(value), CodedOutputStream.ComputeBoolSize, tag);
        }

        public static FieldCodec<int> ForInt32(uint tag)
        {
            return new FieldCodec<int>(input => input.ReadInt32(), (output, value) => output.WriteInt32(value), CodedOutputStream.ComputeInt32Size, tag);
        }

        public static FieldCodec<int> ForSInt32(uint tag)
        {
            return new FieldCodec<int>(input => input.ReadSInt32(), (output, value) => output.WriteSInt32(value), CodedOutputStream.ComputeSInt32Size, tag);
        }

        public static FieldCodec<uint> ForFixedInt32(uint tag)
        {
            return new FieldCodec<uint>(input => input.ReadFixed32(), (output, value) => output.WriteFixed32(value), CodedOutputStream.ComputeFixed32Size, tag);
        }

        public static FieldCodec<uint> ForUInt32(uint tag)
        {
            return new FieldCodec<uint>(input => input.ReadUInt32(), (output, value) => output.WriteUInt32(value), CodedOutputStream.ComputeUInt32Size, tag);
        }

        public static FieldCodec<long> ForInt64(uint tag)
        {
            return new FieldCodec<long>(input => input.ReadInt64(), (output, value) => output.WriteInt64(value), CodedOutputStream.ComputeInt64Size, tag);
        }

        public static FieldCodec<long> ForSInt64(uint tag)
        {
            return new FieldCodec<long>(input => input.ReadSInt64(), (output, value) => output.WriteSInt64(value), CodedOutputStream.ComputeSInt64Size, tag);
        }

        public static FieldCodec<ulong> ForFixedInt64(uint tag)
        {
            return new FieldCodec<ulong>(input => input.ReadFixed64(), (output, value) => output.WriteFixed64(value), CodedOutputStream.ComputeFixed64Size, tag);
        }

        public static FieldCodec<ulong> ForUInt64(uint tag)
        {
            return new FieldCodec<ulong>(input => input.ReadUInt64(), (output, value) => output.WriteUInt64(value), CodedOutputStream.ComputeUInt64Size, tag);
        }

        public static FieldCodec<float> ForFloat(uint tag)
        {
            return new FieldCodec<float>(input => input.ReadFloat(), (output, value) => output.WriteFloat(value), CodedOutputStream.ComputeFloatSize, tag);
        }

        public static FieldCodec<double> ForDouble(uint tag)
        {
            return new FieldCodec<double>(input => input.ReadFloat(), (output, value) => output.WriteDouble(value), CodedOutputStream.ComputeDoubleSize, tag);
        }

        // Enums are tricky. We can probably use expression trees to build these delegates automatically,
        // but it's easy to generate the code fdor it.
        public static FieldCodec<T> ForEnum<T>(uint tag, Func<T, int> toInt32, Func<int, T> fromInt32)
        {
            return new FieldCodec<T>(input => fromInt32(
                input.ReadEnum()),
                (output, value) => output.WriteEnum(toInt32(value)),
                value => CodedOutputStream.ComputeEnumSize(toInt32(value)), tag);
        }

        public static FieldCodec<T> ForMessage<T>(uint tag, MessageParser<T> parser) where T : IMessage<T>
        {
            return new FieldCodec<T>(input => { T message = parser.CreateTemplate(); input.ReadMessage(message); return message; },
                (output, value) => output.WriteMessage(value), message => CodedOutputStream.ComputeMessageSize(message), tag);
        }
    }

    /// <summary>
    /// An encode/decode pair for a single field. This effectively encapsulates
    /// all the information needed to read or write the field value from/to a coded
    /// stream.
    /// </summary>
    /// <remarks>
    /// This never writes default values to the stream, and is not currently designed
    /// to play well with packed arrays.
    /// </remarks>
    public sealed class FieldCodec<T>
    {
        private static readonly Func<T, bool> IsDefault;
        private static readonly T Default;

        static FieldCodec()
        {
            if (typeof(T) == typeof(string))
            {
                Default = (T)(object)"";
                IsDefault = CreateDefaultValueCheck<string>(x => x.Length == 0);
            }
            else if (typeof(T) == typeof(ByteString))
            {
                Default = (T)(object)ByteString.Empty;
                IsDefault = CreateDefaultValueCheck<ByteString>(x => x.Length == 0);
            }
            else if (!typeof(T).IsValueType)
            {
                // Default default
                IsDefault = CreateDefaultValueCheck<T>(x => x == null);
            }
            else
            {
                // Default default
                IsDefault = CreateDefaultValueCheck<T>(x => EqualityComparer<T>.Default.Equals(x, default(T)));
            }
        }

        private static Func<T, bool> CreateDefaultValueCheck<TTmp>(Func<TTmp, bool> check)
        {
            return (Func<T, bool>)(object)check;
        }

        private readonly Func<CodedInputStream, T> reader;
        private readonly Action<CodedOutputStream, T> writer;
        private readonly Func<T, int> sizeComputer;
        private readonly uint tag;
        private readonly int tagSize;

        internal FieldCodec(
            Func<CodedInputStream, T> reader,
            Action<CodedOutputStream, T> writer,
            Func<T, int> sizeComputer,
            uint tag)
        {
            this.reader = reader;
            this.writer = writer;
            this.sizeComputer = sizeComputer;
            this.tag = tag;
            tagSize = CodedOutputStream.ComputeRawVarint32Size(tag);
        }

        public uint Tag { get { return tag; } }

        public T DefaultValue { get { return Default; } }

        public void Write(CodedOutputStream output, T value)
        {
            if (!IsDefault(value))
            {
                output.WriteTag(tag);
                writer(output, value);
            }
        }

        public T Read(CodedInputStream input)
        {
            return reader(input);
        }

        public int CalculateSize(T value)
        {
            return IsDefault(value) ? 0 : sizeComputer(value) + CodedOutputStream.ComputeRawVarint32Size(tag);
        }        
    }
}
