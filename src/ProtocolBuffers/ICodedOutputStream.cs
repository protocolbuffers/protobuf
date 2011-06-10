using System;
using System.Collections.Generic;
using Google.ProtocolBuffers.Descriptors;

//Disable warning CS3010: CLS-compliant interfaces must have only CLS-compliant members
#pragma warning disable 3010

namespace Google.ProtocolBuffers
{
    public interface ICodedOutputStream
    {
        void Flush();

        [Obsolete]
        void WriteUnknownGroup(int fieldNumber, IMessageLite value);
        void WriteUnknownBytes(int fieldNumber, ByteString value);
        [CLSCompliant(false)]
        void WriteUnknownField(int fieldNumber, WireFormat.WireType wireType, ulong value);

        void WriteMessageSetExtension(int fieldNumber, string fieldName, IMessageLite value);
        void WriteMessageSetExtension(int fieldNumber, string fieldName, ByteString value);

        void WriteField(FieldType fieldType, int fieldNumber, string fieldName, object value);

        /// <summary>
        /// Writes a double field value, including tag, to the stream.
        /// </summary>
        void WriteDouble(int fieldNumber, string fieldName, double value);

        /// <summary>
        /// Writes a float field value, including tag, to the stream.
        /// </summary>
        void WriteFloat(int fieldNumber, string fieldName, float value);

        /// <summary>
        /// Writes a uint64 field value, including tag, to the stream.
        /// </summary>
        [CLSCompliant(false)]
        void WriteUInt64(int fieldNumber, string fieldName, ulong value);

        /// <summary>
        /// Writes an int64 field value, including tag, to the stream.
        /// </summary>
        void WriteInt64(int fieldNumber, string fieldName, long value);

        /// <summary>
        /// Writes an int32 field value, including tag, to the stream.
        /// </summary>
        void WriteInt32(int fieldNumber, string fieldName, int value);

        /// <summary>
        /// Writes a fixed64 field value, including tag, to the stream.
        /// </summary>
        [CLSCompliant(false)]
        void WriteFixed64(int fieldNumber, string fieldName, ulong value);

        /// <summary>
        /// Writes a fixed32 field value, including tag, to the stream.
        /// </summary>
        [CLSCompliant(false)]
        void WriteFixed32(int fieldNumber, string fieldName, uint value);

        /// <summary>
        /// Writes a bool field value, including tag, to the stream.
        /// </summary>
        void WriteBool(int fieldNumber, string fieldName, bool value);

        /// <summary>
        /// Writes a string field value, including tag, to the stream.
        /// </summary>
        void WriteString(int fieldNumber, string fieldName, string value);

        /// <summary>
        /// Writes a group field value, including tag, to the stream.
        /// </summary>
        void WriteGroup(int fieldNumber, string fieldName, IMessageLite value);

        /// <summary>
        /// Writes a message field value, including tag, to the stream.
        /// </summary>
        void WriteMessage(int fieldNumber, string fieldName, IMessageLite value);
        /// <summary>
        /// Writes a byte array field value, including tag, to the stream.
        /// </summary>
        void WriteBytes(int fieldNumber, string fieldName, ByteString value);

        /// <summary>
        /// Writes a UInt32 field value, including tag, to the stream.
        /// </summary>
        [CLSCompliant(false)]
        void WriteUInt32(int fieldNumber, string fieldName, uint value);

        /// <summary>
        /// Writes an enum field value, including tag, to the stream.
        /// </summary>
        void WriteEnum(int fieldNumber, string fieldName, int value, object rawValue);
        /// <summary>
        /// Writes a fixed 32-bit field value, including tag, to the stream.
        /// </summary>
        void WriteSFixed32(int fieldNumber, string fieldName, int value);
        /// <summary>
        /// Writes a signed fixed 64-bit field value, including tag, to the stream.
        /// </summary>
        void WriteSFixed64(int fieldNumber, string fieldName, long value);
        /// <summary>
        /// Writes a signed 32-bit field value, including tag, to the stream.
        /// </summary>
        void WriteSInt32(int fieldNumber, string fieldName, int value);
        /// <summary>
        /// Writes a signed 64-bit field value, including tag, to the stream.
        /// </summary>
        void WriteSInt64(int fieldNumber, string fieldName, long value);

        void WriteArray(FieldType fieldType, int fieldNumber, string fieldName, System.Collections.IEnumerable list);

        void WriteGroupArray<T>(int fieldNumber, string fieldName, IEnumerable<T> list)
            where T : IMessageLite;
            
        void WriteMessageArray<T>(int fieldNumber, string fieldName, IEnumerable<T> list)
            where T : IMessageLite;
            
        void WriteStringArray(int fieldNumber, string fieldName, IEnumerable<string> list);
            
        void WriteBytesArray(int fieldNumber, string fieldName, IEnumerable<ByteString> list);
                    
        void WriteBoolArray(int fieldNumber, string fieldName, IEnumerable<bool> list);
                    
        void WriteInt32Array(int fieldNumber, string fieldName, IEnumerable<int> list);
            
        void WriteSInt32Array(int fieldNumber, string fieldName, IEnumerable<int> list);
            
        void WriteUInt32Array(int fieldNumber, string fieldName, IEnumerable<uint> list);
            
        void WriteFixed32Array(int fieldNumber, string fieldName, IEnumerable<uint> list);
            
        void WriteSFixed32Array(int fieldNumber, string fieldName, IEnumerable<int> list);
                    
        void WriteInt64Array(int fieldNumber, string fieldName, IEnumerable<long> list);
            
        void WriteSInt64Array(int fieldNumber, string fieldName, IEnumerable<long> list);
            
        void WriteUInt64Array(int fieldNumber, string fieldName, IEnumerable<ulong> list);
            
        void WriteFixed64Array(int fieldNumber, string fieldName, IEnumerable<ulong> list);
            
        void WriteSFixed64Array(int fieldNumber, string fieldName, IEnumerable<long> list);

        void WriteDoubleArray(int fieldNumber, string fieldName, IEnumerable<double> list);
            
        void WriteFloatArray(int fieldNumber, string fieldName, IEnumerable<float> list);

        [CLSCompliant(false)]
        void WriteEnumArray<T>(int fieldNumber, string fieldName, IEnumerable<T> list) 
            where T : struct, IComparable, IFormattable, IConvertible;

        void WritePackedArray(FieldType fieldType, int fieldNumber, string fieldName, System.Collections.IEnumerable list);
        
        void WritePackedBoolArray(int fieldNumber, string fieldName, int calculatedSize, IEnumerable<bool> list);

        void WritePackedInt32Array(int fieldNumber, string fieldName, int calculatedSize, IEnumerable<int> list);

        void WritePackedSInt32Array(int fieldNumber, string fieldName, int calculatedSize, IEnumerable<int> list);

        void WritePackedUInt32Array(int fieldNumber, string fieldName, int calculatedSize, IEnumerable<uint> list);

        void WritePackedFixed32Array(int fieldNumber, string fieldName, int calculatedSize, IEnumerable<uint> list);

        void WritePackedSFixed32Array(int fieldNumber, string fieldName, int calculatedSize, IEnumerable<int> list);

        void WritePackedInt64Array(int fieldNumber, string fieldName, int calculatedSize, IEnumerable<long> list);

        void WritePackedSInt64Array(int fieldNumber, string fieldName, int calculatedSize, IEnumerable<long> list);

        void WritePackedUInt64Array(int fieldNumber, string fieldName, int calculatedSize, IEnumerable<ulong> list);

        void WritePackedFixed64Array(int fieldNumber, string fieldName, int calculatedSize, IEnumerable<ulong> list);

        void WritePackedSFixed64Array(int fieldNumber, string fieldName, int calculatedSize, IEnumerable<long> list);

        void WritePackedDoubleArray(int fieldNumber, string fieldName, int calculatedSize, IEnumerable<double> list);

        void WritePackedFloatArray(int fieldNumber, string fieldName, int calculatedSize, IEnumerable<float> list);

        [CLSCompliant(false)]
        void WritePackedEnumArray<T>(int fieldNumber, string fieldName, int calculatedSize, IEnumerable<T> list)
            where T : struct, IComparable, IFormattable, IConvertible;
    }
}