using System;
using System.Collections;
using System.Collections.Generic;
using System.Globalization;
using Google.ProtocolBuffers.Descriptors;

//Disable CS3011: only CLS-compliant members can be abstract
#pragma warning disable 3011

namespace Google.ProtocolBuffers.Serialization
{
    /// <summary>
    /// Provides a base class for writers that performs some basic type dispatching
    /// </summary>
    public abstract class AbstractWriter : ICodedOutputStream
    {
        /// <summary>
        /// Completes any pending write operations
        /// </summary>
        public virtual void Flush()
        {
        }

        /// <summary>
        /// Writes the message to the the formatted stream.
        /// </summary>
        public abstract void WriteMessage(IMessageLite message);

        /// <summary>
        /// Used to write any nessary root-message preamble. After this call you can call 
        /// IMessageLite.MergeTo(...) and complete the message with a call to WriteMessageEnd().
        /// These three calls are identical to just calling WriteMessage(message);
        /// </summary>
        /// <example>
        /// AbstractWriter writer;
        /// writer.WriteMessageStart();
        /// message.WriteTo(writer);
        /// writer.WriteMessageEnd();
        /// // ... or, but not both ...
        /// writer.WriteMessage(message);
        /// </example>
        public abstract void WriteMessageStart();

        /// <summary>
        /// Used to complete a root-message previously started with a call to WriteMessageStart()
        /// </summary>
        public abstract void WriteMessageEnd();

        /// <summary>
        /// Writes a Boolean value
        /// </summary>
        protected abstract void Write(string field, Boolean value);

        /// <summary>
        /// Writes a Int32 value
        /// </summary>
        protected abstract void Write(string field, Int32 value);

        /// <summary>
        /// Writes a UInt32 value
        /// </summary>
        protected abstract void Write(string field, UInt32 value);

        /// <summary>
        /// Writes a Int64 value
        /// </summary>
        protected abstract void Write(string field, Int64 value);

        /// <summary>
        /// Writes a UInt64 value
        /// </summary>
        protected abstract void Write(string field, UInt64 value);

        /// <summary>
        /// Writes a Single value
        /// </summary>
        protected abstract void Write(string field, Single value);

        /// <summary>
        /// Writes a Double value
        /// </summary>
        protected abstract void Write(string field, Double value);

        /// <summary>
        /// Writes a String value
        /// </summary>
        protected abstract void Write(string field, String value);

        /// <summary>
        /// Writes a set of bytes
        /// </summary>
        protected abstract void Write(string field, ByteString value);

        /// <summary>
        /// Writes a message or group as a field
        /// </summary>
        protected abstract void WriteMessageOrGroup(string field, IMessageLite message);

        /// <summary>
        /// Writes a System.Enum by the numeric and textual value
        /// </summary>
        protected abstract void WriteEnum(string field, int number, string name);

        /// <summary>
        /// Writes a field of the type determined by field.FieldType
        /// </summary>
        protected virtual void WriteField(FieldType fieldType, string field, object value)
        {
            switch (fieldType)
            {
                case FieldType.Bool:
                    Write(field, (bool) value);
                    break;
                case FieldType.Int64:
                case FieldType.SInt64:
                case FieldType.SFixed64:
                    Write(field, (long) value);
                    break;
                case FieldType.UInt64:
                case FieldType.Fixed64:
                    Write(field, (ulong) value);
                    break;
                case FieldType.Int32:
                case FieldType.SInt32:
                case FieldType.SFixed32:
                    Write(field, (int) value);
                    break;
                case FieldType.UInt32:
                case FieldType.Fixed32:
                    Write(field, (uint) value);
                    break;
                case FieldType.Float:
                    Write(field, (float) value);
                    break;
                case FieldType.Double:
                    Write(field, (double) value);
                    break;
                case FieldType.String:
                    Write(field, (string) value);
                    break;
                case FieldType.Bytes:
                    Write(field, (ByteString) value);
                    break;
                case FieldType.Group:
                    WriteMessageOrGroup(field, (IMessageLite) value);
                    break;
                case FieldType.Message:
                    WriteMessageOrGroup(field, (IMessageLite) value);
                    break;
                case FieldType.Enum:
                    {
                        if (value is IEnumLite)
                        {
                            WriteEnum(field, ((IEnumLite) value).Number, ((IEnumLite) value).Name);
                        }
                        else if (value is IConvertible)
                        {
                            WriteEnum(field, ((IConvertible)value).ToInt32(FrameworkPortability.InvariantCulture),
                                      ((IConvertible)value).ToString(FrameworkPortability.InvariantCulture));
                        }
                        else
                        {
                            throw new ArgumentException("Expected an Enum type for field " + field);
                        }
                        break;
                    }
                default:
                    throw InvalidProtocolBufferException.InvalidTag();
            }
        }

        /// <summary>
        /// Writes an array of field values
        /// </summary>
        protected virtual void WriteArray(FieldType fieldType, string field, IEnumerable items)
        {
            foreach (object obj in items)
            {
                WriteField(fieldType, field, obj);
            }
        }

        /// <summary>
        /// Writes a numeric unknown field of wire type: Fixed32, Fixed64, or Variant
        /// </summary>
        protected virtual void WriteUnknown(WireFormat.WireType wireType, int fieldNumber, ulong value)
        {
        }

        /// <summary>
        /// Writes an unknown field, Expect WireType of GroupStart or LengthPrefix
        /// </summary>
        protected virtual void WriteUnknown(WireFormat.WireType wireType, int fieldNumber, ByteString value)
        {
        }

        #region ICodedOutputStream Members

        void ICodedOutputStream.WriteUnknownGroup(int fieldNumber, IMessageLite value)
        {
        }

        void ICodedOutputStream.WriteUnknownBytes(int fieldNumber, ByteString value)
        {
        }

        void ICodedOutputStream.WriteUnknownField(int fieldNumber, WireFormat.WireType type, ulong value)
        {
        }

        void ICodedOutputStream.WriteMessageSetExtension(int fieldNumber, string fieldName, IMessageLite value)
        {
        }

        void ICodedOutputStream.WriteMessageSetExtension(int fieldNumber, string fieldName, ByteString value)
        {
        }

        void ICodedOutputStream.WriteField(FieldType fieldType, int fieldNumber, string fieldName, object value)
        {
            WriteField(fieldType, fieldName, value);
        }

        void ICodedOutputStream.WriteDouble(int fieldNumber, string fieldName, double value)
        {
            Write(fieldName, value);
        }

        void ICodedOutputStream.WriteFloat(int fieldNumber, string fieldName, float value)
        {
            Write(fieldName, value);
        }

        void ICodedOutputStream.WriteUInt64(int fieldNumber, string fieldName, ulong value)
        {
            Write(fieldName, value);
        }

        void ICodedOutputStream.WriteInt64(int fieldNumber, string fieldName, long value)
        {
            Write(fieldName, value);
        }

        void ICodedOutputStream.WriteInt32(int fieldNumber, string fieldName, int value)
        {
            Write(fieldName, value);
        }

        void ICodedOutputStream.WriteFixed64(int fieldNumber, string fieldName, ulong value)
        {
            Write(fieldName, value);
        }

        void ICodedOutputStream.WriteFixed32(int fieldNumber, string fieldName, uint value)
        {
            Write(fieldName, value);
        }

        void ICodedOutputStream.WriteBool(int fieldNumber, string fieldName, bool value)
        {
            Write(fieldName, value);
        }

        void ICodedOutputStream.WriteString(int fieldNumber, string fieldName, string value)
        {
            Write(fieldName, value);
        }

        void ICodedOutputStream.WriteGroup(int fieldNumber, string fieldName, IMessageLite value)
        {
            WriteMessageOrGroup(fieldName, value);
        }

        void ICodedOutputStream.WriteMessage(int fieldNumber, string fieldName, IMessageLite value)
        {
            WriteMessageOrGroup(fieldName, value);
        }

        void ICodedOutputStream.WriteBytes(int fieldNumber, string fieldName, ByteString value)
        {
            Write(fieldName, value);
        }

        void ICodedOutputStream.WriteUInt32(int fieldNumber, string fieldName, uint value)
        {
            Write(fieldName, value);
        }

        void ICodedOutputStream.WriteEnum(int fieldNumber, string fieldName, int value, object rawValue)
        {
            WriteEnum(fieldName, value, rawValue.ToString());
        }

        void ICodedOutputStream.WriteSFixed32(int fieldNumber, string fieldName, int value)
        {
            Write(fieldName, value);
        }

        void ICodedOutputStream.WriteSFixed64(int fieldNumber, string fieldName, long value)
        {
            Write(fieldName, value);
        }

        void ICodedOutputStream.WriteSInt32(int fieldNumber, string fieldName, int value)
        {
            Write(fieldName, value);
        }

        void ICodedOutputStream.WriteSInt64(int fieldNumber, string fieldName, long value)
        {
            Write(fieldName, value);
        }


        void ICodedOutputStream.WriteArray(FieldType fieldType, int fieldNumber, string fieldName, IEnumerable list)
        {
            WriteArray(fieldType, fieldName, list);
        }

        void ICodedOutputStream.WriteGroupArray<T>(int fieldNumber, string fieldName, IEnumerable<T> list)
        {
            WriteArray(FieldType.Group, fieldName, list);
        }

        void ICodedOutputStream.WriteMessageArray<T>(int fieldNumber, string fieldName, IEnumerable<T> list)
        {
            WriteArray(FieldType.Message, fieldName, list);
        }

        void ICodedOutputStream.WriteStringArray(int fieldNumber, string fieldName, IEnumerable<string> list)
        {
            WriteArray(FieldType.String, fieldName, list);
        }

        void ICodedOutputStream.WriteBytesArray(int fieldNumber, string fieldName, IEnumerable<ByteString> list)
        {
            WriteArray(FieldType.Bytes, fieldName, list);
        }

        void ICodedOutputStream.WriteBoolArray(int fieldNumber, string fieldName, IEnumerable<bool> list)
        {
            WriteArray(FieldType.Bool, fieldName, list);
        }

        void ICodedOutputStream.WriteInt32Array(int fieldNumber, string fieldName, IEnumerable<int> list)
        {
            WriteArray(FieldType.Int32, fieldName, list);
        }

        void ICodedOutputStream.WriteSInt32Array(int fieldNumber, string fieldName, IEnumerable<int> list)
        {
            WriteArray(FieldType.SInt32, fieldName, list);
        }

        void ICodedOutputStream.WriteUInt32Array(int fieldNumber, string fieldName, IEnumerable<uint> list)
        {
            WriteArray(FieldType.UInt32, fieldName, list);
        }

        void ICodedOutputStream.WriteFixed32Array(int fieldNumber, string fieldName, IEnumerable<uint> list)
        {
            WriteArray(FieldType.Fixed32, fieldName, list);
        }

        void ICodedOutputStream.WriteSFixed32Array(int fieldNumber, string fieldName, IEnumerable<int> list)
        {
            WriteArray(FieldType.SFixed32, fieldName, list);
        }

        void ICodedOutputStream.WriteInt64Array(int fieldNumber, string fieldName, IEnumerable<long> list)
        {
            WriteArray(FieldType.Int64, fieldName, list);
        }

        void ICodedOutputStream.WriteSInt64Array(int fieldNumber, string fieldName, IEnumerable<long> list)
        {
            WriteArray(FieldType.SInt64, fieldName, list);
        }

        void ICodedOutputStream.WriteUInt64Array(int fieldNumber, string fieldName, IEnumerable<ulong> list)
        {
            WriteArray(FieldType.UInt64, fieldName, list);
        }

        void ICodedOutputStream.WriteFixed64Array(int fieldNumber, string fieldName, IEnumerable<ulong> list)
        {
            WriteArray(FieldType.Fixed64, fieldName, list);
        }

        void ICodedOutputStream.WriteSFixed64Array(int fieldNumber, string fieldName, IEnumerable<long> list)
        {
            WriteArray(FieldType.SFixed64, fieldName, list);
        }

        void ICodedOutputStream.WriteDoubleArray(int fieldNumber, string fieldName, IEnumerable<double> list)
        {
            WriteArray(FieldType.Double, fieldName, list);
        }

        void ICodedOutputStream.WriteFloatArray(int fieldNumber, string fieldName, IEnumerable<float> list)
        {
            WriteArray(FieldType.Float, fieldName, list);
        }

        void ICodedOutputStream.WriteEnumArray<T>(int fieldNumber, string fieldName, IEnumerable<T> list)
        {
            WriteArray(FieldType.Enum, fieldName, list);
        }

        void ICodedOutputStream.WritePackedArray(FieldType fieldType, int fieldNumber, string fieldName,
                                                 IEnumerable list)
        {
            WriteArray(fieldType, fieldName, list);
        }


        void ICodedOutputStream.WritePackedBoolArray(int fieldNumber, string fieldName, int computedSize,
                                                     IEnumerable<bool> list)
        {
            WriteArray(FieldType.Bool, fieldName, list);
        }

        void ICodedOutputStream.WritePackedInt32Array(int fieldNumber, string fieldName, int computedSize,
                                                      IEnumerable<int> list)
        {
            WriteArray(FieldType.Int32, fieldName, list);
        }

        void ICodedOutputStream.WritePackedSInt32Array(int fieldNumber, string fieldName, int computedSize,
                                                       IEnumerable<int> list)
        {
            WriteArray(FieldType.SInt32, fieldName, list);
        }

        void ICodedOutputStream.WritePackedUInt32Array(int fieldNumber, string fieldName, int computedSize,
                                                       IEnumerable<uint> list)
        {
            WriteArray(FieldType.UInt32, fieldName, list);
        }

        void ICodedOutputStream.WritePackedFixed32Array(int fieldNumber, string fieldName, int computedSize,
                                                        IEnumerable<uint> list)
        {
            WriteArray(FieldType.Fixed32, fieldName, list);
        }

        void ICodedOutputStream.WritePackedSFixed32Array(int fieldNumber, string fieldName, int computedSize,
                                                         IEnumerable<int> list)
        {
            WriteArray(FieldType.SFixed32, fieldName, list);
        }

        void ICodedOutputStream.WritePackedInt64Array(int fieldNumber, string fieldName, int computedSize,
                                                      IEnumerable<long> list)
        {
            WriteArray(FieldType.Int64, fieldName, list);
        }

        void ICodedOutputStream.WritePackedSInt64Array(int fieldNumber, string fieldName, int computedSize,
                                                       IEnumerable<long> list)
        {
            WriteArray(FieldType.SInt64, fieldName, list);
        }

        void ICodedOutputStream.WritePackedUInt64Array(int fieldNumber, string fieldName, int computedSize,
                                                       IEnumerable<ulong> list)
        {
            WriteArray(FieldType.UInt64, fieldName, list);
        }

        void ICodedOutputStream.WritePackedFixed64Array(int fieldNumber, string fieldName, int computedSize,
                                                        IEnumerable<ulong> list)
        {
            WriteArray(FieldType.Fixed64, fieldName, list);
        }

        void ICodedOutputStream.WritePackedSFixed64Array(int fieldNumber, string fieldName, int computedSize,
                                                         IEnumerable<long> list)
        {
            WriteArray(FieldType.SFixed64, fieldName, list);
        }

        void ICodedOutputStream.WritePackedDoubleArray(int fieldNumber, string fieldName, int computedSize,
                                                       IEnumerable<double> list)
        {
            WriteArray(FieldType.Double, fieldName, list);
        }

        void ICodedOutputStream.WritePackedFloatArray(int fieldNumber, string fieldName, int computedSize,
                                                      IEnumerable<float> list)
        {
            WriteArray(FieldType.Float, fieldName, list);
        }

        void ICodedOutputStream.WritePackedEnumArray<T>(int fieldNumber, string fieldName, int computedSize,
                                                        IEnumerable<T> list)
        {
            WriteArray(FieldType.Enum, fieldName, list);
        }

        #endregion
    }
}