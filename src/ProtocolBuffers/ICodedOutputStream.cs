using System;
using Google.ProtocolBuffers.Descriptors;

//Disable warning CS3010: CLS-compliant interfaces must have only CLS-compliant members
#pragma warning disable 3010

namespace Google.ProtocolBuffers
{
    public interface ICodedOutputStream
    {
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

        [Obsolete]
        void WriteUnknownGroup(int fieldNumber, string fieldName, IMessageLite value);

        void WriteMessage(int fieldNumber, string fieldName, IMessageLite value);
        void WriteBytes(int fieldNumber, string fieldName, ByteString value);

        [CLSCompliant(false)]
        void WriteUInt32(int fieldNumber, string fieldName, uint value);

        void WriteEnum(int fieldNumber, string fieldName, int value, string textValue);
        void WriteSFixed32(int fieldNumber, string fieldName, int value);
        void WriteSFixed64(int fieldNumber, string fieldName, long value);
        void WriteSInt32(int fieldNumber, string fieldName, int value);
        void WriteSInt64(int fieldNumber, string fieldName, long value);
        void WriteMessageSetExtension(int fieldNumber, string fieldName, IMessageLite value);
        void WriteMessageSetExtension(int fieldNumber, string fieldName, ByteString value);
        void WriteArray(FieldType fieldType, int fieldNumber, string fieldName, System.Collections.IEnumerable list);
        void WritePackedArray(FieldType fieldType, int fieldNumber, string fieldName, System.Collections.IEnumerable list);
        void WriteArray<T>(FieldType fieldType, int fieldNumber, string fieldName, System.Collections.Generic.IEnumerable<T> list);
        void WritePackedArray<T>(FieldType fieldType, int fieldNumber, string fieldName, int calculatedSize, System.Collections.Generic.IEnumerable<T> list);
        void WriteField(FieldType fieldType, int fieldNumber, string fieldName, object value);
        void Flush();
    }
}