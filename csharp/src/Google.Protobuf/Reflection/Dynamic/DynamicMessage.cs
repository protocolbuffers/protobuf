using Google.Protobuf.Collections;
using System;
using System.Collections;

namespace Google.Protobuf.Reflection.Dynamic
{

    /// <summary>
    /// An implementation of IMessage that can represent arbitrary types, given a MessageaDescriptor.
    /// Unknown fields not supported.
    /// </summary>
    public sealed class DynamicMessage : IMessage
    {
        private readonly MessageDescriptor type;
        private readonly FieldSet fields;
        private int memoizedSize = -1;

        private DynamicMessage(MessageDescriptor type, FieldSet fields)
        {
            this.type = type;
            this.fields = fields;
        }

        /// <summary>
        /// Descriptor for this message.
        /// </summary>
        public MessageDescriptor Descriptor => type;

        /// <summary>
        /// Default implementation of Overridden method.
        /// This specific implementation is never used, mergeFrom is used from the builder class.
        /// MergeFrom functionality comes into picture when, we do desrialization from ByteString using static ParseFrom method.
        /// </summary>
        /// <param name="input"></param>
        /// <exception cref="NotImplementedException"></exception>
        void IMessage.MergeFrom(CodedInputStream input)
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// Writes the message to CodedOutputStream.
        /// </summary>
        /// <param name="output"></param>
        public void WriteTo(CodedOutputStream output)
        {
            fields.WriteTo(output);
        }

        /// <summary>
        /// Used to calculate the size of the serialized message.
        /// </summary>
        /// <returns></returns>
        public int CalculateSize()
        {
            int size = memoizedSize;
            if (size != -1)
            {
                return size;
            }
            size = fields.GetSerializedSize();
            memoizedSize = size;
            return size;
        }

        /// <summary>
        /// Parse data as a message of the given type and return it.
        /// </summary>
        /// <param name="type"></param>
        /// <param name="data"></param>
        /// <returns></returns>
        public static DynamicMessage ParseFrom(MessageDescriptor type, ByteString data)
        {
            Builder builder = NewBuilder(type);
            builder.MergeFrom(data);
            return builder.BuildParsed();
        }

        /// <summary>
        /// Construct a IMessage.Builder for the given type.
        /// </summary>
        /// <param name="type"></param>
        /// <returns></returns>
        public static Builder NewBuilder(MessageDescriptor type)
        {
            return new Builder(type);
        }

        /// <summary>
        /// Returns the value of the field, represented by field descriptor
        /// </summary>
        /// <param name="fd"></param>
        /// <returns></returns>
        public object GetField(FieldDescriptor fd)
        {
            return fields.GetField(fd);
        }

        /// <summary>
        /// Builder for dynamic messages. Instances are created with DynamicMessage.CreateBuilder..
        /// </summary>
        public sealed class Builder : IMessage
        {

            private readonly MessageDescriptor type;
            private FieldSet fields;

            internal Builder(MessageDescriptor type)
            {
                this.type = type;
                this.fields = FieldSet.CreateInstance();
            }

            private bool IsInitialized
            {
                get { return fields.IsInitializedWithRespectTo(type.Fields.ByJsonName()); }
            }

            MessageDescriptor IMessage.Descriptor => throw new NotImplementedException();

            /// <summary>
            /// Creates the DynamicMessage
            /// </summary>
            /// <returns></returns>
            /// <exception cref="Exception"></exception>
            public DynamicMessage Build()
            {
                if (fields != null && !IsInitialized)
                {
                    throw new Exception($"Message of type {typeof(DynamicMessage)} is missing required fields");
                }
                return BuildPartial();
            }

            /// <summary>
            /// Used to set a field
            /// </summary>
            /// <param name="fd"></param>
            /// <param name="value"></param>
            public void SetField(FieldDescriptor fd, object value)
            {
                fields.SetField(fd, value);
            }

            /// <summary>
            /// Used to add repeated field
            /// </summary>
            /// <param name="fd"></param>
            /// <param name="v"></param>
            public void AddRepeatedField(FieldDescriptor fd, object v)
            {
                fields.AddRepeatedField(fd, v);
            }

            private DynamicMessage BuildPartial()
            {
                if (fields == null)
                {
                    throw new InvalidOperationException("Build() has already been called on this Builder.");
                }
                DynamicMessage result = new DynamicMessage(type, fields);
                fields = null;
                return result;
            }

            /// <summary>
            /// Reads every field and sets it in the FieldSet instance
            /// </summary>
            /// <param name="input"></param>
            /// <exception cref="Exception"></exception>
            public void MergeFrom(CodedInputStream input)
            {
                uint tag;
                while ((tag = input.ReadTag()) != 0)
                {
                    int fieldNumber = WireFormat.GetTagFieldNumber(tag);
                    FieldDescriptor fd = type.FindFieldByNumber(fieldNumber);

                    if (fd == null)
                    {
                        throw new Exception($"Field descriptor not found for fieldNumber: {fieldNumber}");
                    }
                    if (fd.FieldType == FieldType.Message)
                    {
                        MergeFromMessageTypeField(input, fd);
                    }
                    else
                    {
                        MergeFromPrimitiveTypeField(input, tag, fd);
                    }
                }
            }

            private void MergeFromPrimitiveTypeField(CodedInputStream input, uint tag, FieldDescriptor fd)
            {
                if (fd.ToProto().Label != FieldDescriptorProto.Types.Label.Repeated)
                {
                    object value = ReadField(fd.FieldType, input);
                    fields.SetField(fd, value);
                }
                else
                {
                    IEnumerator enumerator = GetFieldCodec(tag, fd.FieldType, input);
                    while (enumerator.MoveNext())
                    {
                        fields.AddRepeatedField(fd, enumerator.Current);
                    }
                }

            }

            private void MergeFromMessageTypeField(CodedInputStream input, FieldDescriptor fd)
            {
                Builder value = NewBuilder(fd.MessageType);
                input.ReadMessage(value);
                DynamicMessage val = value.Build();
                if (fd.ToProto().Label != FieldDescriptorProto.Types.Label.Repeated)
                {
                    fields.SetField(fd, val);
                }
                else
                {
                    fields.AddRepeatedField(fd, val);
                }
            }

            /// <summary>
            /// Used for repeated fields to get enumerator.
            /// Based on the fieldType, creates Repeated field, adds entries from CodedInputStrem to repeatedField and returns enumerator on the repeated field.
            /// </summary>
            /// <param name="tag"></param>
            /// <param name="fieldType"></param>
            /// <param name="input"></param>
            /// <returns></returns>
            private static IEnumerator GetFieldCodec(uint tag, FieldType fieldType, CodedInputStream input)
            {
                switch (fieldType)
                {
                    case FieldType.Int32:
                        RepeatedField<int> rf = new RepeatedField<int>();
                        rf.AddEntriesFrom(input, FieldCodec.ForInt32(tag));
                        return rf.GetEnumerator();
                    case FieldType.Int64:
                        RepeatedField<long> rf1 = new RepeatedField<long>();
                        rf1.AddEntriesFrom(input, FieldCodec.ForInt64(tag));
                        return rf1.GetEnumerator();
                    case FieldType.Bool:
                        RepeatedField<bool> rfBool = new RepeatedField<bool>();
                        rfBool.AddEntriesFrom(input, FieldCodec.ForBool(tag));
                        return rfBool.GetEnumerator();
                    case FieldType.UInt32:
                        RepeatedField<uint> rfUint = new RepeatedField<uint>();
                        rfUint.AddEntriesFrom(input, FieldCodec.ForUInt32(tag));
                        return rfUint.GetEnumerator();
                    case FieldType.UInt64:
                        RepeatedField<ulong> rfUint64 = new RepeatedField<ulong>();
                        rfUint64.AddEntriesFrom(input, FieldCodec.ForUInt64(tag));
                        return rfUint64.GetEnumerator();
                    case FieldType.SInt32:
                        RepeatedField<int> rfSint32 = new RepeatedField<int>();
                        rfSint32.AddEntriesFrom(input, FieldCodec.ForSInt32(tag));
                        return rfSint32.GetEnumerator();
                    case FieldType.SInt64:
                        RepeatedField<long> rfSint64 = new RepeatedField<long>();
                        rfSint64.AddEntriesFrom(input, FieldCodec.ForSInt64(tag));
                        return rfSint64.GetEnumerator();
                    case FieldType.Fixed32:
                        RepeatedField<uint> rfFixed32 = new RepeatedField<uint>();
                        rfFixed32.AddEntriesFrom(input, FieldCodec.ForFixed32(tag));
                        return rfFixed32.GetEnumerator();
                    case FieldType.Fixed64:
                        RepeatedField<ulong> rfFixed64 = new RepeatedField<ulong>();
                        rfFixed64.AddEntriesFrom(input, FieldCodec.ForFixed64(tag));
                        return rfFixed64.GetEnumerator();
                    case FieldType.SFixed32:
                        RepeatedField<int> rfSFixed32 = new RepeatedField<int>();
                        rfSFixed32.AddEntriesFrom(input, FieldCodec.ForSFixed32(tag));
                        return rfSFixed32.GetEnumerator();
                    case FieldType.SFixed64:
                        RepeatedField<long> rfSFixed64 = new RepeatedField<long>();
                        rfSFixed64.AddEntriesFrom(input, FieldCodec.ForSFixed64(tag));
                        return rfSFixed64.GetEnumerator();
                    case FieldType.Float:
                        RepeatedField<float> rfFloat = new RepeatedField<float>();
                        rfFloat.AddEntriesFrom(input, FieldCodec.ForFloat(tag));
                        return rfFloat.GetEnumerator();
                    case FieldType.Double:
                        RepeatedField<double> rfDouble = new RepeatedField<double>();
                        rfDouble.AddEntriesFrom(input, FieldCodec.ForDouble(tag));
                        return rfDouble.GetEnumerator();
                    case FieldType.String:
                        RepeatedField<String> rfString = new RepeatedField<String>();
                        rfString.AddEntriesFrom(input, FieldCodec.ForString(tag));
                        return rfString.GetEnumerator();
                    case FieldType.Bytes:
                        RepeatedField<ByteString> rfBytes = new RepeatedField<ByteString>();
                        rfBytes.AddEntriesFrom(input, FieldCodec.ForBytes(tag));
                        return rfBytes.GetEnumerator();
                }
                throw new Exception("FieldType:" + fieldType + ", not handled in GetFieldCodec method.");
            }

            /// <summary>
            /// Reads the field from the CodedInputStream, based on the fieldType and returns it.
            /// </summary>
            /// <param name="fieldType"></param>
            /// <param name="input"></param>
            /// <returns></returns>
            private object ReadField(FieldType fieldType, CodedInputStream input)
            {
                switch (fieldType)
                {
                    case FieldType.Int32:
                        return input.ReadInt32();
                    case FieldType.Int64:
                        return input.ReadInt64();
                    case FieldType.Bytes:
                        return input.ReadBytes();
                    case FieldType.String:
                        return input.ReadString();
                    case FieldType.Double:
                        return input.ReadDouble();
                    case FieldType.Float:
                        return input.ReadFloat();
                    case FieldType.Bool:
                        return input.ReadBool();
                    case FieldType.UInt32:
                        return input.ReadUInt32();
                    case FieldType.UInt64:
                        return input.ReadUInt64();
                    case FieldType.Fixed32:
                        return input.ReadFixed32();
                    case FieldType.Fixed64:
                        return input.ReadFixed64();
                    case FieldType.SFixed64:
                        return input.ReadSFixed64();
                    case FieldType.SFixed32:
                        return input.ReadSFixed32();
                    case FieldType.SInt32:
                        return input.ReadSInt32();
                    case FieldType.SInt64:
                        return input.ReadSInt64();
                    case FieldType.Enum:
                        return input.ReadEnum();
                }
                throw new Exception("FieldType:" + fieldType + ", not handled while reading field.");
            }

            /// <summary>
            /// Marks the fields as immutable and returns a newly created DynsmicMessage.
            /// </summary>
            /// <returns></returns>
            internal DynamicMessage BuildParsed()
            {
                DynamicMessage result = new DynamicMessage(type, fields);
                return result;
            }

            /// <summary>
            /// Default implementation of Overridden method.
            /// This implementation is never used, writeTo is used from DynamicMessage class.
            /// WriteTo functionality is used when we do serialization of DynamicMessage to generate ByteString.
            /// </summary>
            /// <param name="output"></param>
            /// <exception cref="NotImplementedException"></exception>
            void IMessage.WriteTo(CodedOutputStream output)
            {
                throw new NotImplementedException();
            }

            /// <summary>
            /// Default implementation of Overridden method.
            /// This implementation is never used, CalculateSize is used from DynamicMessage class.
            /// CalculateSize functionality is used when we do serialization of DynamicMessage to generate ByteString.
            /// </summary>
            /// <returns></returns>
            /// <exception cref="NotImplementedException"></exception>
            int IMessage.CalculateSize()
            {
                throw new NotImplementedException();
            }

        }

    }

}
