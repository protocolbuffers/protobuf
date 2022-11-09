using System;
using System.Collections;
using System.Collections.Generic;

namespace Google.Protobuf.Reflection.Dynamic
{

    internal sealed class FieldSet
    {

        private IDictionary<FieldDescriptor, object> fields;

        /// <summary>
        /// Creates a new instance.
        /// </summary>
        /// <returns></returns>
        internal static FieldSet CreateInstance()
        {
            return new FieldSet(new Dictionary<FieldDescriptor, object>());
        }

        private FieldSet(IDictionary<FieldDescriptor, object> fields)
        {
            this.fields = fields;
        }

        /// <summary>
        /// Checks if all the required fields have value.
        /// </summary>
        /// <param name="typeFields"></param>
        /// <returns></returns>
        internal bool IsInitializedWithRespectTo(IEnumerable typeFields)
        {
            foreach (KeyValuePair<string, FieldDescriptor> field in typeFields)
            {
                // according to proto3, field can't be required.
                // So result to this method will always be true.
                if (field.Value.IsRequired && !HasField(field.Value))
                {
                    return false;
                }
            }
            return true;
        }

        /// <summary>
        /// Checks if the fields dictionary contains the field.
        /// </summary>
        private bool HasField(FieldDescriptor field)
        {
            if (field.IsRepeated)
            {
                throw new ArgumentException("HasField() can only be called on non-repeated fields.");
            }

            return fields.ContainsKey(field);
        }

        /// <summary>
        /// Returns the serialized size.
        /// </summary>
        /// <returns></returns>
        internal int GetSerializedSize()
        {
            int size = 0;
            foreach (KeyValuePair<FieldDescriptor, object> kvPair in fields)
            {
                size += ComputeSize(kvPair.Key, kvPair.Value);
            }
            return size;
        }

        /// <summary>
        /// Computes the size of the field. The computation varies if the field is repeated.
        /// </summary>
        /// <param name="desc"></param>
        /// <param name="value"></param>
        /// <returns></returns>
        private int ComputeSize(FieldDescriptor desc, object value)
        {
            if (desc.IsRepeated)
            {
                int dataSize = 0;
                int tagSize = CodedOutputStream.ComputeTagSize(desc.FieldNumber);
                foreach (object val in (IEnumerable) value)
                {
                    dataSize += ComputeElementSizeNoTag(desc.FieldType, val) + tagSize;
                }
                return dataSize;
            }
            else
            {
                return ComputeElementSize(desc.FieldType, desc.FieldNumber, value);
            }

        }

        /// <summary>
        /// Computes the total size of the filed, along with its tag.
        /// </summary>
        /// <param name="fieldType"></param>
        /// <param name="fieldNumber"></param>
        /// <param name="value"></param>
        /// <returns></returns>
        private int ComputeElementSize(FieldType fieldType, int fieldNumber, object value)
        {
            int tagSize = CodedOutputStream.ComputeTagSize(fieldNumber);
            return tagSize + ComputeElementSizeNoTag(fieldType, value);
        }

        /// <summary>
        /// Computes the size of the field without tag.
        /// </summary>
        /// <param name="fieldType"></param>
        /// <param name="value"></param>
        /// <returns></returns>
        /// <exception cref="ArgumentException"></exception>
        private int ComputeElementSizeNoTag(FieldType fieldType, object value)
        {
            switch (fieldType)
            {
                case FieldType.String:
                    return CodedOutputStream.ComputeStringSize(value.ToString());
                case FieldType.Bool:
                    return CodedOutputStream.ComputeBoolSize((bool) value);
                case FieldType.Int32:
                    return CodedOutputStream.ComputeInt32Size((int) value);
                case FieldType.Int64:
                    return CodedOutputStream.ComputeInt64Size((long) value);
                case FieldType.Double:
                    return CodedOutputStream.ComputeDoubleSize((double) value);
                case FieldType.Float:
                    return CodedOutputStream.ComputeFloatSize((float) value);
                case FieldType.Bytes:
                    return CodedOutputStream.ComputeBytesSize((ByteString) value);
                case FieldType.UInt32:
                    return CodedOutputStream.ComputeUInt32Size((uint) value);
                case FieldType.UInt64:
                    return CodedOutputStream.ComputeUInt64Size((ulong) value);
                case FieldType.SInt32:
                    return CodedOutputStream.ComputeSInt32Size((int) value);
                case FieldType.SInt64:
                    return CodedOutputStream.ComputeSInt64Size((long) value);
                case FieldType.Fixed32:
                    return CodedOutputStream.ComputeFixed32Size((uint) value);
                case FieldType.Fixed64:
                    return CodedOutputStream.ComputeFixed64Size((ulong) value);
                case FieldType.SFixed32:
                    return CodedOutputStream.ComputeSFixed32Size((int) value);
                case FieldType.SFixed64:
                    return CodedOutputStream.ComputeSFixed64Size((long) value);
                case FieldType.Message:
                    return CodedOutputStream.ComputeMessageSize((IMessage) value);
                case FieldType.Enum:
                    return CodedOutputStream.ComputeEnumSize((int) value);

            }
            throw new ArgumentException("unidentified type :" + fieldType.ToString());
        }

        /// <summary>
        /// Iterates all the fields and writes them to CodedOutputStream.
        /// </summary>
        /// <param name="output"></param>
        internal void WriteTo(CodedOutputStream output)
        {
            foreach (KeyValuePair<FieldDescriptor, object> kvPair in fields)
            {
                WriteField(kvPair.Key, kvPair.Value, output);
            }
        }

        /// <summary>
        /// Writes the field into output stream, based on isRepeated.
        /// </summary>
        /// <param name="descriptor"></param>
        /// <param name="value"></param>
        /// <param name="output"></param>
        private void WriteField(FieldDescriptor descriptor, object value, CodedOutputStream output)
        {
            FieldType fieldType = descriptor.FieldType;
            if (descriptor.IsRepeated)
            {
                foreach (Object val in (IEnumerable) value)
                {
                    WriteElement(output, fieldType, descriptor.FieldNumber, val);
                }
            }
            else
            {
                WriteElement(output, fieldType, descriptor.FieldNumber, value);
            }
        }

        /// <summary>
        /// Writes a single field, first tag and then data.
        /// </summary>
        /// <param name="output"></param>
        /// <param name="type"></param>
        /// <param name="number"></param>
        /// <param name="value"></param>
        private void WriteElement(CodedOutputStream output, FieldType type, int number, object value)
        {
            output.WriteTag(number, GetWireFormatForFieldType(type));
            WriteElementNoTag(output, type, value);
        }

        /// <summary>
        /// Returns the wire format for the field type.
        /// </summary>
        /// <param name="type"></param>
        /// <returns></returns>
        /// <exception cref="ArgumentException"></exception>
        private WireFormat.WireType GetWireFormatForFieldType(FieldType type)
        {
            switch (type)
            {
                case FieldType.Float:
                    return WireFormat.WireType.Fixed32;
                case FieldType.Fixed32:
                    return WireFormat.WireType.Fixed32;
                case FieldType.SFixed32:
                    return WireFormat.WireType.Fixed32;
                case FieldType.Fixed64:
                    return WireFormat.WireType.Fixed64;
                case FieldType.SFixed64:
                    return WireFormat.WireType.Fixed64;
                case FieldType.Double:
                    return WireFormat.WireType.Fixed64;
                case FieldType.Bool:
                    return WireFormat.WireType.Varint;
                case FieldType.Int32:
                    return WireFormat.WireType.Varint;
                case FieldType.Int64:
                    return WireFormat.WireType.Varint;
                case FieldType.UInt32:
                    return WireFormat.WireType.Varint;
                case FieldType.UInt64:
                    return WireFormat.WireType.Varint;
                case FieldType.Enum:
                    return WireFormat.WireType.Varint;
                case FieldType.SInt32:
                    return WireFormat.WireType.Varint;
                case FieldType.SInt64:
                    return WireFormat.WireType.Varint;
                case FieldType.String:
                    return WireFormat.WireType.LengthDelimited;
                case FieldType.Bytes:
                    return WireFormat.WireType.LengthDelimited;
                case FieldType.Message:
                    return WireFormat.WireType.LengthDelimited;
                case FieldType.Group:
                    return WireFormat.WireType.StartGroup;
            }
            throw new ArgumentException("unidentified type :" + type.ToString());
        }

        /// <summary>
        /// Write the element value into the output stream.
        /// </summary>
        /// <param name="output"></param>
        /// <param name="type"></param>
        /// <param name="value"></param>
        /// <exception cref="ArgumentException"></exception>
        private void WriteElementNoTag(CodedOutputStream output, FieldType type, object value)
        {
            switch (type)
            {
                case FieldType.String:
                    output.WriteString(value.ToString());
                    return;
                case FieldType.Bool:
                    output.WriteBool((bool) value);
                    return;
                case FieldType.Int32:
                    output.WriteInt32((int) value);
                    return;
                case FieldType.Int64:
                    output.WriteInt64((long) value);
                    return;
                case FieldType.Double:
                    output.WriteDouble((double) value);
                    return;
                case FieldType.UInt32:
                    output.WriteUInt32((uint) value);
                    return;
                case FieldType.UInt64:
                    output.WriteUInt64((ulong) value);
                    return;
                case FieldType.SInt32:
                    output.WriteSInt32((int) value);
                    return;
                case FieldType.SInt64:
                    output.WriteSInt64((long) value);
                    return;
                case FieldType.Fixed32:
                    output.WriteFixed32((uint) value);
                    return;
                case FieldType.Fixed64:
                    output.WriteFixed64((ulong) value);
                    return;
                case FieldType.SFixed32:
                    output.WriteSFixed32((int) value);
                    return;
                case FieldType.SFixed64:
                    output.WriteSFixed64((long) value);
                    return;
                case FieldType.Float:
                    output.WriteFloat((float) value);
                    return;
                case FieldType.Bytes:
                    output.WriteBytes((ByteString) value);
                    return;
                case FieldType.Enum:
                    output.WriteEnum((int) value);
                    return;
                case FieldType.Message:
                    output.WriteMessage((IMessage) value);
                    return;
            }
            throw new ArgumentException("unidentified type :" + type.ToString());
        }

        /// <summary>
        /// Sets the field, if the value is null throw exception.
        /// </summary>
        /// <param name="field"></param>
        /// <param name="value"></param>
        /// <exception cref="ArgumentNullException"></exception>
        internal void SetField(FieldDescriptor field, object value)
        {
            if (value == null)
            {
                throw new ArgumentNullException(field.Name);
            }
            fields.Add(field, value);
        }

        /// <summary>
        /// Creates a list if field not present in the dictionary, and adds the value to the list.
        /// If field is not repeated throw exception.
        /// </summary>
        /// <param name="field"></param>
        /// <param name="value"></param>
        /// <exception cref="ArgumentException"></exception>
        internal void AddRepeatedField(FieldDescriptor field, object value)
        {
            if (!field.IsRepeated)
            {
                throw new ArgumentException("AddRepeatedField can only be called on repeated fields.");
            }
            if (!fields.TryGetValue(field, out object list))
            {
                list = new List<object>();
                fields[field] = list;
            }

            ((IList<object>) list).Add(value);
        }

        /// <summary>
        /// Returns the value of the field represented by FieldDescriptor.
        /// </summary>
        /// <param name="fd"></param>
        /// <returns></returns>
        internal object GetField(FieldDescriptor fd)
        {
            fields.TryGetValue(fd, out object value);
            return value;
        }

    }

}
