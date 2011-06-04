using System;
using System.Collections.Generic;
using Google.ProtocolBuffers.Descriptors;

//Disable warning CS3010: CLS-compliant interfaces must have only CLS-compliant members
#pragma warning disable 3010

namespace Google.ProtocolBuffers
{
    public interface ICodedInputStream
    {
        /// <summary>
        /// Attempt to read a field tag, returning false if we have reached the end
        /// of the input data.
        /// </summary>
        /// <remarks>
        /// <para>
        /// If fieldTag is non-zero and ReadTag returns true then the value in fieldName
        /// may or may not be populated.  However, if fieldTag is zero and ReadTag returns
        /// true, then fieldName should be populated with a non-null field name.
        /// </para><para>
        /// In other words if ReadTag returns true then either fieldTag will be non-zero OR
        /// fieldName will be non-zero.  In some cases both may be populated, however the
        /// builders will always prefer the fieldTag over fieldName.
        /// </para>
        /// </remarks>
        [CLSCompliant(false)]
        bool ReadTag(out uint fieldTag, out string fieldName);

        /// <summary>
        /// Read a double field from the stream.
        /// </summary>
        bool ReadDouble(ref double value);

        /// <summary>
        /// Read a float field from the stream.
        /// </summary>
        bool ReadFloat(ref float value);

        /// <summary>
        /// Read a uint64 field from the stream.
        /// </summary>
        [CLSCompliant(false)]
        bool ReadUInt64(ref ulong value);

        /// <summary>
        /// Read an int64 field from the stream.
        /// </summary>
        bool ReadInt64(ref long value);

        /// <summary>
        /// Read an int32 field from the stream.
        /// </summary>
        bool ReadInt32(ref int value);

        /// <summary>
        /// Read a fixed64 field from the stream.
        /// </summary>
        [CLSCompliant(false)]
        bool ReadFixed64(ref ulong value);

        /// <summary>
        /// Read a fixed32 field from the stream.
        /// </summary>
        [CLSCompliant(false)]
        bool ReadFixed32(ref uint value);

        /// <summary>
        /// Read a bool field from the stream.
        /// </summary>
        bool ReadBool(ref bool value);

        /// <summary>
        /// Reads a string field from the stream.
        /// </summary>
        bool ReadString(ref string value);

        /// <summary>
        /// Reads a group field value from the stream.
        /// </summary>    
        void ReadGroup(int fieldNumber, IBuilderLite builder,
                                       ExtensionRegistry extensionRegistry);

        /// <summary>
        /// Reads a group field value from the stream and merges it into the given
        /// UnknownFieldSet.
        /// </summary>   
        [Obsolete]
        void ReadUnknownGroup(int fieldNumber, IBuilderLite builder);

        /// <summary>
        /// Reads an embedded message field value from the stream.
        /// </summary>   
        void ReadMessage(IBuilderLite builder, ExtensionRegistry extensionRegistry);

        /// <summary>
        /// Reads a bytes field value from the stream.
        /// </summary>   
        bool ReadBytes(ref ByteString value);

        /// <summary>
        /// Reads a uint32 field value from the stream.
        /// </summary>   
        [CLSCompliant(false)]
        bool ReadUInt32(ref uint value);

        /// <summary>
        /// Reads an enum field value from the stream. The caller is responsible
        /// for converting the numeric value to an actual enum.
        /// </summary>   
        bool ReadEnum(ref IEnumLite value, out object unknown, IEnumLiteMap mapping);

        /// <summary>
        /// Reads an enum field value from the stream. If the enum is valid for type T,
        /// then the ref value is set and it returns true.  Otherwise the unkown output
        /// value is set and this method returns false.
        /// </summary>   
        [CLSCompliant(false)]
        bool ReadEnum<T>(ref T value, out object unknown)
            where T : struct, IComparable, IFormattable, IConvertible;

        /// <summary>
        /// Reads an sfixed32 field value from the stream.
        /// </summary>   
        bool ReadSFixed32(ref int value);

        /// <summary>
        /// Reads an sfixed64 field value from the stream.
        /// </summary>   
        bool ReadSFixed64(ref long value);

        /// <summary>
        /// Reads an sint32 field value from the stream.
        /// </summary>   
        bool ReadSInt32(ref int value);

        /// <summary>
        /// Reads an sint64 field value from the stream.
        /// </summary>   
        bool ReadSInt64(ref long value);

        /// <summary>
        /// Reads an array of primitive values into the list, if the wire-type of fieldTag is length-prefixed and the 
        /// type is numberic, it will read a packed array.
        /// </summary>
        [CLSCompliant(false)]
        void ReadPrimitiveArray<T>(FieldType fieldType, uint fieldTag, string fieldName, ICollection<T> list);

        /// <summary>
        /// Reads an array of primitive values into the list, if the wire-type of fieldTag is length-prefixed, it will
        /// read a packed array.
        /// </summary>
        [CLSCompliant(false)]
        void ReadEnumArray(uint fieldTag, string fieldName, ICollection<IEnumLite> list, out ICollection<object> unknown, IEnumLiteMap mapping);

        /// <summary>
        /// Reads an array of primitive values into the list, if the wire-type of fieldTag is length-prefixed, it will
        /// read a packed array.
        /// </summary>
        [CLSCompliant(false)]
        void ReadEnumArray<T>(uint fieldTag, string fieldName, ICollection<T> list, out ICollection<object> unknown)
            where T : struct, IComparable, IFormattable, IConvertible;

        /// <summary>
        /// Reads a set of messages using the <paramref name="messageType"/> as a template.  T is not guaranteed to be 
        /// the most derived type, it is only the type specifier for the collection.
        /// </summary>
        [CLSCompliant(false)]
        void ReadMessageArray<T>(uint fieldTag, string fieldName, ICollection<T> list, T messageType, ExtensionRegistry registry) where T : IMessageLite;

        /// <summary>
        /// Reads a set of messages using the <paramref name="messageType"/> as a template.
        /// </summary>
        [CLSCompliant(false)]
        void ReadGroupArray<T>(uint fieldTag, string fieldName, ICollection<T> list, T messageType, ExtensionRegistry registry) where T : IMessageLite;

        /// <summary>
        /// Reads a field of any primitive type. Enums, groups and embedded
        /// messages are not handled by this method.
        /// </summary>
        bool ReadPrimitiveField(FieldType fieldType, ref object value);

        /// <summary>
        /// Returns true if the stream has reached the end of the input. This is the
        /// case if either the end of the underlying input source has been reached or
        /// the stream has reached a limit created using PushLimit.
        /// </summary>
        bool IsAtEnd { get; }

        /// <summary>
        /// Reads and discards a single field, given its tag value.
        /// </summary>
        /// <returns>false if the tag is an end-group tag, in which case
        /// nothing is skipped. Otherwise, returns true.</returns>
        [CLSCompliant(false)]
        bool SkipField();
    }
}