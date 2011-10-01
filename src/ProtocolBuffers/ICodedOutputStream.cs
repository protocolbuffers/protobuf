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
using System.Collections;
using System.Collections.Generic;
using Google.ProtocolBuffers.Descriptors;

//Disable warning CS3010: CLS-compliant interfaces must have only CLS-compliant members
#pragma warning disable 3010

namespace Google.ProtocolBuffers
{
    /// <summary>
    /// Provides an interface that is used write a message.  Most often proto buffers are written
    /// in their binary form by creating a instance via the CodedOutputStream.CreateInstance
    /// static factory.
    /// </summary>
    public interface ICodedOutputStream
    {
        /// <summary>
        /// Writes any message initialization data needed to the output stream
        /// </summary>
        /// <remarks>
        /// This is primarily used by text formats and unnecessary for protobuffers' own
        /// binary format.  The API for MessageStart/End was added for consistent handling
        /// of output streams regardless of the actual writer implementation.
        /// </remarks>
        void WriteMessageStart();
        /// <summary>
        /// Writes any message finalization data needed to the output stream
        /// </summary>
        /// <remarks>
        /// This is primarily used by text formats and unnecessary for protobuffers' own
        /// binary format.  The API for MessageStart/End was added for consistent handling
        /// of output streams regardless of the actual writer implementation.
        /// </remarks>
        void WriteMessageEnd();
        /// <summary>
        /// Indicates that all temporary buffers be written to the final output.
        /// </summary>
        void Flush();
        /// <summary>
        /// Writes an unknown message as a group
        /// </summary>
        [Obsolete]
        void WriteUnknownGroup(int fieldNumber, IMessageLite value);
        /// <summary>
        /// Writes an unknown field value of bytes
        /// </summary>
        void WriteUnknownBytes(int fieldNumber, ByteString value);
        /// <summary>
        /// Writes an unknown field of a primitive type
        /// </summary>
        [CLSCompliant(false)]
        void WriteUnknownField(int fieldNumber, WireFormat.WireType wireType, ulong value);
        /// <summary>
        /// Writes an extension as a message-set group
        /// </summary>
        void WriteMessageSetExtension(int fieldNumber, string fieldName, IMessageLite value);
        /// <summary>
        /// Writes an unknown extension as a message-set group
        /// </summary>
        void WriteMessageSetExtension(int fieldNumber, string fieldName, ByteString value);

        /// <summary>
        /// Writes a field value, including tag, to the stream.
        /// </summary>
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

        /// <summary>
        /// Writes a repeated field value, including tag(s), to the stream.
        /// </summary>
        void WriteArray(FieldType fieldType, int fieldNumber, string fieldName, IEnumerable list);

        /// <summary>
        /// Writes a repeated group value, including tag(s), to the stream.
        /// </summary>
        void WriteGroupArray<T>(int fieldNumber, string fieldName, IEnumerable<T> list)
            where T : IMessageLite;

        /// <summary>
        /// Writes a repeated message value, including tag(s), to the stream.
        /// </summary>
        void WriteMessageArray<T>(int fieldNumber, string fieldName, IEnumerable<T> list)
            where T : IMessageLite;

        /// <summary>
        /// Writes a repeated string value, including tag(s), to the stream.
        /// </summary>
        void WriteStringArray(int fieldNumber, string fieldName, IEnumerable<string> list);

        /// <summary>
        /// Writes a repeated ByteString value, including tag(s), to the stream.
        /// </summary>
        void WriteBytesArray(int fieldNumber, string fieldName, IEnumerable<ByteString> list);

        /// <summary>
        /// Writes a repeated boolean value, including tag(s), to the stream.
        /// </summary>
        void WriteBoolArray(int fieldNumber, string fieldName, IEnumerable<bool> list);

        /// <summary>
        /// Writes a repeated Int32 value, including tag(s), to the stream.
        /// </summary>
        void WriteInt32Array(int fieldNumber, string fieldName, IEnumerable<int> list);

        /// <summary>
        /// Writes a repeated SInt32 value, including tag(s), to the stream.
        /// </summary>
        void WriteSInt32Array(int fieldNumber, string fieldName, IEnumerable<int> list);

        /// <summary>
        /// Writes a repeated UInt32 value, including tag(s), to the stream.
        /// </summary>
        void WriteUInt32Array(int fieldNumber, string fieldName, IEnumerable<uint> list);

        /// <summary>
        /// Writes a repeated Fixed32 value, including tag(s), to the stream.
        /// </summary>
        void WriteFixed32Array(int fieldNumber, string fieldName, IEnumerable<uint> list);

        /// <summary>
        /// Writes a repeated SFixed32 value, including tag(s), to the stream.
        /// </summary>
        void WriteSFixed32Array(int fieldNumber, string fieldName, IEnumerable<int> list);

        /// <summary>
        /// Writes a repeated Int64 value, including tag(s), to the stream.
        /// </summary>
        void WriteInt64Array(int fieldNumber, string fieldName, IEnumerable<long> list);

        /// <summary>
        /// Writes a repeated SInt64 value, including tag(s), to the stream.
        /// </summary>
        void WriteSInt64Array(int fieldNumber, string fieldName, IEnumerable<long> list);

        /// <summary>
        /// Writes a repeated UInt64 value, including tag(s), to the stream.
        /// </summary>
        void WriteUInt64Array(int fieldNumber, string fieldName, IEnumerable<ulong> list);

        /// <summary>
        /// Writes a repeated Fixed64 value, including tag(s), to the stream.
        /// </summary>
        void WriteFixed64Array(int fieldNumber, string fieldName, IEnumerable<ulong> list);

        /// <summary>
        /// Writes a repeated SFixed64 value, including tag(s), to the stream.
        /// </summary>
        void WriteSFixed64Array(int fieldNumber, string fieldName, IEnumerable<long> list);

        /// <summary>
        /// Writes a repeated Double value, including tag(s), to the stream.
        /// </summary>
        void WriteDoubleArray(int fieldNumber, string fieldName, IEnumerable<double> list);

        /// <summary>
        /// Writes a repeated Float value, including tag(s), to the stream.
        /// </summary>
        void WriteFloatArray(int fieldNumber, string fieldName, IEnumerable<float> list);

        /// <summary>
        /// Writes a repeated enumeration value of type T, including tag(s), to the stream.
        /// </summary>
        [CLSCompliant(false)]
        void WriteEnumArray<T>(int fieldNumber, string fieldName, IEnumerable<T> list)
            where T : struct, IComparable, IFormattable, IConvertible;

        /// <summary>
        /// Writes a packed repeated primitive, including tag and length, to the stream.
        /// </summary>
        void WritePackedArray(FieldType fieldType, int fieldNumber, string fieldName, IEnumerable list);

        /// <summary>
        /// Writes a packed repeated boolean, including tag and length, to the stream.
        /// </summary>
        void WritePackedBoolArray(int fieldNumber, string fieldName, int calculatedSize, IEnumerable<bool> list);

        /// <summary>
        /// Writes a packed repeated Int32, including tag and length, to the stream.
        /// </summary>
        void WritePackedInt32Array(int fieldNumber, string fieldName, int calculatedSize, IEnumerable<int> list);

        /// <summary>
        /// Writes a packed repeated SInt32, including tag and length, to the stream.
        /// </summary>
        void WritePackedSInt32Array(int fieldNumber, string fieldName, int calculatedSize, IEnumerable<int> list);

        /// <summary>
        /// Writes a packed repeated UInt32, including tag and length, to the stream.
        /// </summary>
        void WritePackedUInt32Array(int fieldNumber, string fieldName, int calculatedSize, IEnumerable<uint> list);

        /// <summary>
        /// Writes a packed repeated Fixed32, including tag and length, to the stream.
        /// </summary>
        void WritePackedFixed32Array(int fieldNumber, string fieldName, int calculatedSize, IEnumerable<uint> list);

        /// <summary>
        /// Writes a packed repeated SFixed32, including tag and length, to the stream.
        /// </summary>
        void WritePackedSFixed32Array(int fieldNumber, string fieldName, int calculatedSize, IEnumerable<int> list);

        /// <summary>
        /// Writes a packed repeated Int64, including tag and length, to the stream.
        /// </summary>
        void WritePackedInt64Array(int fieldNumber, string fieldName, int calculatedSize, IEnumerable<long> list);

        /// <summary>
        /// Writes a packed repeated SInt64, including tag and length, to the stream.
        /// </summary>
        void WritePackedSInt64Array(int fieldNumber, string fieldName, int calculatedSize, IEnumerable<long> list);

        /// <summary>
        /// Writes a packed repeated UInt64, including tag and length, to the stream.
        /// </summary>
        void WritePackedUInt64Array(int fieldNumber, string fieldName, int calculatedSize, IEnumerable<ulong> list);

        /// <summary>
        /// Writes a packed repeated Fixed64, including tag and length, to the stream.
        /// </summary>
        void WritePackedFixed64Array(int fieldNumber, string fieldName, int calculatedSize, IEnumerable<ulong> list);

        /// <summary>
        /// Writes a packed repeated SFixed64, including tag and length, to the stream.
        /// </summary>
        void WritePackedSFixed64Array(int fieldNumber, string fieldName, int calculatedSize, IEnumerable<long> list);

        /// <summary>
        /// Writes a packed repeated Double, including tag and length, to the stream.
        /// </summary>
        void WritePackedDoubleArray(int fieldNumber, string fieldName, int calculatedSize, IEnumerable<double> list);

        /// <summary>
        /// Writes a packed repeated Float, including tag and length, to the stream.
        /// </summary>
        void WritePackedFloatArray(int fieldNumber, string fieldName, int calculatedSize, IEnumerable<float> list);

        /// <summary>
        /// Writes a packed repeated enumeration of type T, including tag and length, to the stream.
        /// </summary>
        [CLSCompliant(false)]
        void WritePackedEnumArray<T>(int fieldNumber, string fieldName, int calculatedSize, IEnumerable<T> list)
            where T : struct, IComparable, IFormattable, IConvertible;
    }
}