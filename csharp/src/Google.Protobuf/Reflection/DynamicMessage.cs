using Google.Protobuf.Collections;
using System;
using System.Collections;

namespace Google.Protobuf.Reflection
{

    /// <summary>
    /// An implementation of IMessage that can represent arbitrary types, given a MessageaDescriptor.
    /// </summary>
    public sealed partial class DynamicMessage : IMessage<DynamicMessage>
    {

        private readonly MessageParser<DynamicMessage> _parser = null;
        private FieldSet _fieldSet = FieldSet.CreateInstance();
        private UnknownFieldSet _unknownFields = new UnknownFieldSet();

        /// <summary>
        /// Properties for parsing
        /// </summary> 
        public MessageParser<DynamicMessage> Parser { get { return _parser; } }

        /// <summary>
        /// Holds the descriptor information. This can't be static, as every DynamicMessage can have a separate descriptor.
        /// </summary>
        public MessageDescriptor Descriptor { get; }

        /// <summary>
        /// The default const being replaced with this one.
        /// The constructor takes in message descriptor.
        /// </summary>
        /// <param name="descriptor"></param>
        public DynamicMessage(MessageDescriptor descriptor)
        {
            this.Descriptor = descriptor;
            _parser = new MessageParser<DynamicMessage>(() => new DynamicMessage(descriptor));
            OnConstruction();
        }

        partial void OnConstruction();

        /// <summary>
        /// The FieldSet reference
        /// </summary>
        public FieldSet FieldSet => _fieldSet;

        /// <summary>
        /// Descriptor for this message.
        /// </summary>
        MessageDescriptor IMessage.Descriptor { get; }

        /// <summary>
        /// 
        /// </summary>
        //public UnknownFieldSet UnknownFields => _unknownFields;

        /// <summary>
        /// Used to calculate the size of the serialized message.
        /// </summary>
        /// <returns></returns>
        public int CalculateSize()
        {
            int size = _fieldSet.GetSerializedSize();
            if (_unknownFields != null)
            {
                size += _unknownFields.CalculateSize();
            }
            return size;
        }

        /// <summary>
        /// Merges content from input into current DynamicMessage
        /// </summary>
        /// <param name="input"></param>
        public void MergeFrom(CodedInputStream input)
        {
            uint tag;
            while ((tag = input.ReadTag()) != 0)
            {
                int fieldNumber = WireFormat.GetTagFieldNumber(tag);
                FieldDescriptor fd = Descriptor.FindFieldByNumber(fieldNumber);

                if (fd == null)
                {
                    _unknownFields = UnknownFieldSet.MergeFieldFrom(_unknownFields, input);
                }
                else if (fd.FieldType == FieldType.Message)
                {
                    MergeFromMessageTypeField(input, fd);
                }
                else
                {
                    MergeFromPrimitiveTypeField(input, tag, fd);
                }
            }
        }

        /// <summary>
        /// Writes the message to CodedOutputStream.
        /// </summary>
        /// <param name="output"></param>
        public void WriteTo(CodedOutputStream output)
        {
            _fieldSet.WriteTo(output);
            _unknownFields?.WriteTo(output);
        }

        /// <summary>
        /// Merges other DynamicMessage into current one.
        /// </summary>
        /// <param name="message"></param>
        public void MergeFrom(DynamicMessage message)
        {
            if (message == null)
                return;
            _fieldSet.Merge(message._fieldSet);
            _unknownFields = UnknownFieldSet.MergeFrom(_unknownFields, message._unknownFields);
        }

        /// <summary>
        /// Compares for equality
        /// </summary>
        /// <param name="other"></param>
        /// <returns></returns>
        public bool Equals(DynamicMessage other)
        {
            if (ReferenceEquals(other, null))
            {
                return false;
            }
            if (ReferenceEquals(other, this))
            {
                return true;
            }
            if (!Equals(_fieldSet, other._fieldSet)) return false;
            return Equals(_unknownFields, other._unknownFields);
        }

        /// <summary>
        /// Clone a DynamicMessage
        /// </summary>
        /// <returns></returns>
        public DynamicMessage Clone()
        {
            return new DynamicMessage(Descriptor)
            {
                _fieldSet = this._fieldSet.Clone(),
                _unknownFields = UnknownFieldSet.Clone(this._unknownFields)
            };
        }

        /// <summary>
        /// To add a field to DynamicMessage
        /// </summary>
        /// <param name="fieldName"></param>
        /// <param name="v"></param>
        /// <exception cref="Exception"></exception>
        public void Add(String fieldName, object v)
        {
            FieldDescriptor fieldDescriptor = Descriptor.FindFieldByName(fieldName);
            if (fieldDescriptor == null)
            {
                throw new Exception("unknown field not supported by this version of the method");
            }
            else if (fieldDescriptor.IsRepeated)
            {
                _fieldSet.AddRepeatedField(fieldDescriptor, v);
            }
            else
            {
                _fieldSet.SetField(fieldDescriptor, v);
            }
        }

        private void MergeFromPrimitiveTypeField(CodedInputStream input, uint tag, FieldDescriptor fd)
        {
            if (fd.ToProto().Label != FieldDescriptorProto.Types.Label.Repeated)
            {
                object value = ReadField(fd.FieldType, input);
                _fieldSet.SetField(fd, value);
            }
            else
            {
                IEnumerator enumerator = GetFieldCodec(tag, fd.FieldType, input);
                while (enumerator.MoveNext())
                {
                    _fieldSet.AddRepeatedField(fd, enumerator.Current);
                }
            }

        }

        private void MergeFromMessageTypeField(CodedInputStream input, FieldDescriptor fd)
        {
            DynamicMessage value = new DynamicMessage(fd.MessageType);
            input.ReadMessage(value);
            if (fd.ToProto().Label != FieldDescriptorProto.Types.Label.Repeated)
            {
                _fieldSet.SetField(fd, value);
            }
            else
            {
                _fieldSet.AddRepeatedField(fd, value);
            }
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
        /// Used for repeated fields to get enumerator.
        /// Based on the fieldType, creates Repeated field, adds entries from CodedInputStrem to repeatedField and returns enumerator on the repeated field.
        /// </summary>
        /// <param name="tag"></param>
        /// <param name="fieldType"></param>
        /// <param name="input"></param>
        /// <returns></returns>
        private IEnumerator GetFieldCodec(uint tag, FieldType fieldType, CodedInputStream input)
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
                    RepeatedField<string> rfString = new RepeatedField<string>();
                    rfString.AddEntriesFrom(input, FieldCodec.ForString(tag));
                    return rfString.GetEnumerator();
                case FieldType.Bytes:
                    RepeatedField<ByteString> rfBytes = new RepeatedField<ByteString>();
                    rfBytes.AddEntriesFrom(input, FieldCodec.ForBytes(tag));
                    return rfBytes.GetEnumerator();
            }
            throw new Exception("FieldType:" + fieldType + ", not handled in GetFieldCodec method.");
        }


    }
}
