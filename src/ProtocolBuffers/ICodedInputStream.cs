#region Copyright notice and license

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://github.com/jskeet/dotnet-protobufs/
// Original C++/Java/Python code:
// http://code.google.com/p/protobuf/
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
        /// Reads any message initialization data expected from the input stream
        /// </summary>
        /// <remarks>
        /// This is primarily used by text formats and unnecessary for protobuffers' own
        /// binary format.  The API for MessageStart/End was added for consistent handling
        /// of output streams regardless of the actual writer implementation.
        /// </remarks>
        void ReadMessageStart();
        /// <summary>
        /// Reads any message finalization data expected from the input stream
        /// </summary>
        /// <remarks>
        /// This is primarily used by text formats and unnecessary for protobuffers' own
        /// binary format.  The API for MessageStart/End was added for consistent handling
        /// of output streams regardless of the actual writer implementation.
        /// </remarks>
        void ReadMessageEnd();
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
        /// type is numeric, it will read a packed array.
        /// </summary>
        [CLSCompliant(false)]
        void ReadPrimitiveArray(FieldType fieldType, uint fieldTag, string fieldName, ICollection<object> list);

        /// <summary>
        /// Reads an array of primitive values into the list, if the wire-type of fieldTag is length-prefixed, it will
        /// read a packed array.
        /// </summary>
        [CLSCompliant(false)]
        void ReadEnumArray(uint fieldTag, string fieldName, ICollection<IEnumLite> list, out ICollection<object> unknown,
                           IEnumLiteMap mapping);

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
        void ReadMessageArray<T>(uint fieldTag, string fieldName, ICollection<T> list, T messageType,
                                 ExtensionRegistry registry) where T : IMessageLite;

        /// <summary>
        /// Reads a set of messages using the <paramref name="messageType"/> as a template.
        /// </summary>
        [CLSCompliant(false)]
        void ReadGroupArray<T>(uint fieldTag, string fieldName, ICollection<T> list, T messageType,
                               ExtensionRegistry registry) where T : IMessageLite;

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

        /// <summary>
        /// Reads one or more repeated string field values from the stream.
        /// </summary>   
        [CLSCompliant(false)]
        void ReadStringArray(uint fieldTag, string fieldName, ICollection<string> list);

        /// <summary>
        /// Reads one or more repeated ByteString field values from the stream.
        /// </summary>   
        [CLSCompliant(false)]
        void ReadBytesArray(uint fieldTag, string fieldName, ICollection<ByteString> list);

        /// <summary>
        /// Reads one or more repeated boolean field values from the stream.
        /// </summary>
        [CLSCompliant(false)]
        void ReadBoolArray(uint fieldTag, string fieldName, ICollection<bool> list);

        /// <summary>
        /// Reads one or more repeated Int32 field values from the stream.
        /// </summary>
        [CLSCompliant(false)]
        void ReadInt32Array(uint fieldTag, string fieldName, ICollection<int> list);

        /// <summary>
        /// Reads one or more repeated SInt32 field values from the stream.
        /// </summary>
        [CLSCompliant(false)]
        void ReadSInt32Array(uint fieldTag, string fieldName, ICollection<int> list);

        /// <summary>
        /// Reads one or more repeated UInt32 field values from the stream.
        /// </summary>
        [CLSCompliant(false)]
        void ReadUInt32Array(uint fieldTag, string fieldName, ICollection<uint> list);

        /// <summary>
        /// Reads one or more repeated Fixed32 field values from the stream.
        /// </summary>
        [CLSCompliant(false)]
        void ReadFixed32Array(uint fieldTag, string fieldName, ICollection<uint> list);

        /// <summary>
        /// Reads one or more repeated SFixed32 field values from the stream.
        /// </summary>
        [CLSCompliant(false)]
        void ReadSFixed32Array(uint fieldTag, string fieldName, ICollection<int> list);

        /// <summary>
        /// Reads one or more repeated Int64 field values from the stream.
        /// </summary>
        [CLSCompliant(false)]
        void ReadInt64Array(uint fieldTag, string fieldName, ICollection<long> list);

        /// <summary>
        /// Reads one or more repeated SInt64 field values from the stream.
        /// </summary>
        [CLSCompliant(false)]
        void ReadSInt64Array(uint fieldTag, string fieldName, ICollection<long> list);

        /// <summary>
        /// Reads one or more repeated UInt64 field values from the stream.
        /// </summary>
        [CLSCompliant(false)]
        void ReadUInt64Array(uint fieldTag, string fieldName, ICollection<ulong> list);

        /// <summary>
        /// Reads one or more repeated Fixed64 field values from the stream.
        /// </summary>
        [CLSCompliant(false)]
        void ReadFixed64Array(uint fieldTag, string fieldName, ICollection<ulong> list);

        /// <summary>
        /// Reads one or more repeated SFixed64 field values from the stream.
        /// </summary>
        [CLSCompliant(false)]
        void ReadSFixed64Array(uint fieldTag, string fieldName, ICollection<long> list);

        /// <summary>
        /// Reads one or more repeated Double field values from the stream.
        /// </summary>
        [CLSCompliant(false)]
        void ReadDoubleArray(uint fieldTag, string fieldName, ICollection<double> list);

        /// <summary>
        /// Reads one or more repeated Float field values from the stream.
        /// </summary>
        [CLSCompliant(false)]
        void ReadFloatArray(uint fieldTag, string fieldName, ICollection<float> list);
    }
}