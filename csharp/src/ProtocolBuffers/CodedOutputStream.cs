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
using System.IO;
using System.Text;
using Google.ProtocolBuffers.Collections;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers
{
    /// <summary>
    /// Encodes and writes protocol message fields.
    /// </summary>
    /// <remarks>
    /// This class contains two kinds of methods:  methods that write specific
    /// protocol message constructs and field types (e.g. WriteTag and
    /// WriteInt32) and methods that write low-level values (e.g.
    /// WriteRawVarint32 and WriteRawBytes).  If you are writing encoded protocol
    /// messages, you should use the former methods, but if you are writing some
    /// other format of your own design, use the latter. The names of the former
    /// methods are taken from the protocol buffer type names, not .NET types.
    /// (Hence WriteFloat instead of WriteSingle, and WriteBool instead of WriteBoolean.)
    /// </remarks>
    public sealed partial class CodedOutputStream : ICodedOutputStream
    {
        /// <summary>
        /// The buffer size used by CreateInstance(Stream).
        /// </summary>
        public static readonly int DefaultBufferSize = 4096;

        private readonly byte[] buffer;
        private readonly int limit;
        private int position;
        private readonly Stream output;

        #region Construction

        private CodedOutputStream(byte[] buffer, int offset, int length)
        {
            this.output = null;
            this.buffer = buffer;
            this.position = offset;
            this.limit = offset + length;
        }

        private CodedOutputStream(Stream output, byte[] buffer)
        {
            this.output = output;
            this.buffer = buffer;
            this.position = 0;
            this.limit = buffer.Length;
        }

        /// <summary>
        /// Creates a new CodedOutputStream which write to the given stream.
        /// </summary>
        public static CodedOutputStream CreateInstance(Stream output)
        {
            return CreateInstance(output, DefaultBufferSize);
        }

        /// <summary>
        /// Creates a new CodedOutputStream which write to the given stream and uses
        /// the specified buffer size.
        /// </summary>
        public static CodedOutputStream CreateInstance(Stream output, int bufferSize)
        {
            return new CodedOutputStream(output, new byte[bufferSize]);
        }

        /// <summary>
        /// Creates a new CodedOutputStream that writes directly to the given
        /// byte array. If more bytes are written than fit in the array,
        /// OutOfSpaceException will be thrown.
        /// </summary>
        public static CodedOutputStream CreateInstance(byte[] flatArray)
        {
            return CreateInstance(flatArray, 0, flatArray.Length);
        }

        /// <summary>
        /// Creates a new CodedOutputStream that writes directly to the given
        /// byte array slice. If more bytes are written than fit in the array,
        /// OutOfSpaceException will be thrown.
        /// </summary>
        public static CodedOutputStream CreateInstance(byte[] flatArray, int offset, int length)
        {
            return new CodedOutputStream(flatArray, offset, length);
        }

        #endregion

        /// <summary>
        /// Returns the current position in the stream, or the position in the output buffer
        /// </summary>
        public long Position
        {
            get
            {
                if (output != null)
                {
                    return output.Position + position;
                }
                return position;
            }
        }

        void ICodedOutputStream.WriteMessageStart() { }
        void ICodedOutputStream.WriteMessageEnd() { Flush(); }

        #region Writing of unknown fields

        [Obsolete]
        public void WriteUnknownGroup(int fieldNumber, IMessageLite value)
        {
            WriteTag(fieldNumber, WireFormat.WireType.StartGroup);
            value.WriteTo(this);
            WriteTag(fieldNumber, WireFormat.WireType.EndGroup);
        }

        public void WriteUnknownBytes(int fieldNumber, ByteString value)
        {
            WriteBytes(fieldNumber, null /*not used*/, value);
        }

        public void WriteUnknownField(int fieldNumber, WireFormat.WireType wireType, ulong value)
        {
            if (wireType == WireFormat.WireType.Varint)
            {
                WriteUInt64(fieldNumber, null /*not used*/, value);
            }
            else if (wireType == WireFormat.WireType.Fixed32)
            {
                WriteFixed32(fieldNumber, null /*not used*/, (uint) value);
            }
            else if (wireType == WireFormat.WireType.Fixed64)
            {
                WriteFixed64(fieldNumber, null /*not used*/, value);
            }
            else
            {
                throw InvalidProtocolBufferException.InvalidWireType();
            }
        }

        #endregion

        #region Writing of tags and fields

        public void WriteField(FieldType fieldType, int fieldNumber, string fieldName, object value)
        {
            switch (fieldType)
            {
                case FieldType.String:
                    WriteString(fieldNumber, fieldName, (string) value);
                    break;
                case FieldType.Message:
                    WriteMessage(fieldNumber, fieldName, (IMessageLite) value);
                    break;
                case FieldType.Group:
                    WriteGroup(fieldNumber, fieldName, (IMessageLite) value);
                    break;
                case FieldType.Bytes:
                    WriteBytes(fieldNumber, fieldName, (ByteString) value);
                    break;
                case FieldType.Bool:
                    WriteBool(fieldNumber, fieldName, (bool) value);
                    break;
                case FieldType.Enum:
                    if (value is Enum)
                    {
                        WriteEnum(fieldNumber, fieldName, (int) value, null /*not used*/);
                    }
                    else
                    {
                        WriteEnum(fieldNumber, fieldName, ((IEnumLite) value).Number, null /*not used*/);
                    }
                    break;
                case FieldType.Int32:
                    WriteInt32(fieldNumber, fieldName, (int) value);
                    break;
                case FieldType.Int64:
                    WriteInt64(fieldNumber, fieldName, (long) value);
                    break;
                case FieldType.UInt32:
                    WriteUInt32(fieldNumber, fieldName, (uint) value);
                    break;
                case FieldType.UInt64:
                    WriteUInt64(fieldNumber, fieldName, (ulong) value);
                    break;
                case FieldType.SInt32:
                    WriteSInt32(fieldNumber, fieldName, (int) value);
                    break;
                case FieldType.SInt64:
                    WriteSInt64(fieldNumber, fieldName, (long) value);
                    break;
                case FieldType.Fixed32:
                    WriteFixed32(fieldNumber, fieldName, (uint) value);
                    break;
                case FieldType.Fixed64:
                    WriteFixed64(fieldNumber, fieldName, (ulong) value);
                    break;
                case FieldType.SFixed32:
                    WriteSFixed32(fieldNumber, fieldName, (int) value);
                    break;
                case FieldType.SFixed64:
                    WriteSFixed64(fieldNumber, fieldName, (long) value);
                    break;
                case FieldType.Double:
                    WriteDouble(fieldNumber, fieldName, (double) value);
                    break;
                case FieldType.Float:
                    WriteFloat(fieldNumber, fieldName, (float) value);
                    break;
            }
        }

        /// <summary>
        /// Writes a double field value, including tag, to the stream.
        /// </summary>
        public void WriteDouble(int fieldNumber, string fieldName, double value)
        {
            WriteTag(fieldNumber, WireFormat.WireType.Fixed64);
            WriteDoubleNoTag(value);
        }

        /// <summary>
        /// Writes a float field value, including tag, to the stream.
        /// </summary>
        public void WriteFloat(int fieldNumber, string fieldName, float value)
        {
            WriteTag(fieldNumber, WireFormat.WireType.Fixed32);
            WriteFloatNoTag(value);
        }

        /// <summary>
        /// Writes a uint64 field value, including tag, to the stream.
        /// </summary>
        public void WriteUInt64(int fieldNumber, string fieldName, ulong value)
        {
            WriteTag(fieldNumber, WireFormat.WireType.Varint);
            WriteRawVarint64(value);
        }

        /// <summary>
        /// Writes an int64 field value, including tag, to the stream.
        /// </summary>
        public void WriteInt64(int fieldNumber, string fieldName, long value)
        {
            WriteTag(fieldNumber, WireFormat.WireType.Varint);
            WriteRawVarint64((ulong) value);
        }

        /// <summary>
        /// Writes an int32 field value, including tag, to the stream.
        /// </summary>
        public void WriteInt32(int fieldNumber, string fieldName, int value)
        {
            WriteTag(fieldNumber, WireFormat.WireType.Varint);
            if (value >= 0)
            {
                WriteRawVarint32((uint) value);
            }
            else
            {
                // Must sign-extend.
                WriteRawVarint64((ulong) value);
            }
        }

        /// <summary>
        /// Writes a fixed64 field value, including tag, to the stream.
        /// </summary>
        public void WriteFixed64(int fieldNumber, string fieldName, ulong value)
        {
            WriteTag(fieldNumber, WireFormat.WireType.Fixed64);
            WriteRawLittleEndian64(value);
        }

        /// <summary>
        /// Writes a fixed32 field value, including tag, to the stream.
        /// </summary>
        public void WriteFixed32(int fieldNumber, string fieldName, uint value)
        {
            WriteTag(fieldNumber, WireFormat.WireType.Fixed32);
            WriteRawLittleEndian32(value);
        }

        /// <summary>
        /// Writes a bool field value, including tag, to the stream.
        /// </summary>
        public void WriteBool(int fieldNumber, string fieldName, bool value)
        {
            WriteTag(fieldNumber, WireFormat.WireType.Varint);
            WriteRawByte(value ? (byte) 1 : (byte) 0);
        }

        /// <summary>
        /// Writes a string field value, including tag, to the stream.
        /// </summary>
        public void WriteString(int fieldNumber, string fieldName, string value)
        {
            WriteTag(fieldNumber, WireFormat.WireType.LengthDelimited);
            // Optimise the case where we have enough space to write
            // the string directly to the buffer, which should be common.
            int length = Encoding.UTF8.GetByteCount(value);
            WriteRawVarint32((uint) length);
            if (limit - position >= length)
            {
                Encoding.UTF8.GetBytes(value, 0, value.Length, buffer, position);
                position += length;
            }
            else
            {
                byte[] bytes = Encoding.UTF8.GetBytes(value);
                WriteRawBytes(bytes);
            }
        }

        /// <summary>
        /// Writes a group field value, including tag, to the stream.
        /// </summary>
        public void WriteGroup(int fieldNumber, string fieldName, IMessageLite value)
        {
            WriteTag(fieldNumber, WireFormat.WireType.StartGroup);
            value.WriteTo(this);
            WriteTag(fieldNumber, WireFormat.WireType.EndGroup);
        }

        public void WriteMessage(int fieldNumber, string fieldName, IMessageLite value)
        {
            WriteTag(fieldNumber, WireFormat.WireType.LengthDelimited);
            WriteRawVarint32((uint) value.SerializedSize);
            value.WriteTo(this);
        }

        public void WriteBytes(int fieldNumber, string fieldName, ByteString value)
        {
            WriteTag(fieldNumber, WireFormat.WireType.LengthDelimited);
            WriteRawVarint32((uint) value.Length);
            value.WriteRawBytesTo(this);
        }

        public void WriteUInt32(int fieldNumber, string fieldName, uint value)
        {
            WriteTag(fieldNumber, WireFormat.WireType.Varint);
            WriteRawVarint32(value);
        }

        public void WriteEnum(int fieldNumber, string fieldName, int value, object rawValue)
        {
            WriteTag(fieldNumber, WireFormat.WireType.Varint);
            WriteInt32NoTag(value);
        }

        public void WriteSFixed32(int fieldNumber, string fieldName, int value)
        {
            WriteTag(fieldNumber, WireFormat.WireType.Fixed32);
            WriteRawLittleEndian32((uint) value);
        }

        public void WriteSFixed64(int fieldNumber, string fieldName, long value)
        {
            WriteTag(fieldNumber, WireFormat.WireType.Fixed64);
            WriteRawLittleEndian64((ulong) value);
        }

        public void WriteSInt32(int fieldNumber, string fieldName, int value)
        {
            WriteTag(fieldNumber, WireFormat.WireType.Varint);
            WriteRawVarint32(EncodeZigZag32(value));
        }

        public void WriteSInt64(int fieldNumber, string fieldName, long value)
        {
            WriteTag(fieldNumber, WireFormat.WireType.Varint);
            WriteRawVarint64(EncodeZigZag64(value));
        }

        public void WriteMessageSetExtension(int fieldNumber, string fieldName, IMessageLite value)
        {
            WriteTag(WireFormat.MessageSetField.Item, WireFormat.WireType.StartGroup);
            WriteUInt32(WireFormat.MessageSetField.TypeID, "type_id", (uint) fieldNumber);
            WriteMessage(WireFormat.MessageSetField.Message, "message", value);
            WriteTag(WireFormat.MessageSetField.Item, WireFormat.WireType.EndGroup);
        }

        public void WriteMessageSetExtension(int fieldNumber, string fieldName, ByteString value)
        {
            WriteTag(WireFormat.MessageSetField.Item, WireFormat.WireType.StartGroup);
            WriteUInt32(WireFormat.MessageSetField.TypeID, "type_id", (uint) fieldNumber);
            WriteBytes(WireFormat.MessageSetField.Message, "message", value);
            WriteTag(WireFormat.MessageSetField.Item, WireFormat.WireType.EndGroup);
        }

        #endregion

        #region Writing of values without tags

        public void WriteFieldNoTag(FieldType fieldType, object value)
        {
            switch (fieldType)
            {
                case FieldType.String:
                    WriteStringNoTag((string) value);
                    break;
                case FieldType.Message:
                    WriteMessageNoTag((IMessageLite) value);
                    break;
                case FieldType.Group:
                    WriteGroupNoTag((IMessageLite) value);
                    break;
                case FieldType.Bytes:
                    WriteBytesNoTag((ByteString) value);
                    break;
                case FieldType.Bool:
                    WriteBoolNoTag((bool) value);
                    break;
                case FieldType.Enum:
                    if (value is Enum)
                    {
                        WriteEnumNoTag((int) value);
                    }
                    else
                    {
                        WriteEnumNoTag(((IEnumLite) value).Number);
                    }
                    break;
                case FieldType.Int32:
                    WriteInt32NoTag((int) value);
                    break;
                case FieldType.Int64:
                    WriteInt64NoTag((long) value);
                    break;
                case FieldType.UInt32:
                    WriteUInt32NoTag((uint) value);
                    break;
                case FieldType.UInt64:
                    WriteUInt64NoTag((ulong) value);
                    break;
                case FieldType.SInt32:
                    WriteSInt32NoTag((int) value);
                    break;
                case FieldType.SInt64:
                    WriteSInt64NoTag((long) value);
                    break;
                case FieldType.Fixed32:
                    WriteFixed32NoTag((uint) value);
                    break;
                case FieldType.Fixed64:
                    WriteFixed64NoTag((ulong) value);
                    break;
                case FieldType.SFixed32:
                    WriteSFixed32NoTag((int) value);
                    break;
                case FieldType.SFixed64:
                    WriteSFixed64NoTag((long) value);
                    break;
                case FieldType.Double:
                    WriteDoubleNoTag((double) value);
                    break;
                case FieldType.Float:
                    WriteFloatNoTag((float) value);
                    break;
            }
        }

        /// <summary>
        /// Writes a double field value, including tag, to the stream.
        /// </summary>
        public void WriteDoubleNoTag(double value)
        {
            WriteRawLittleEndian64((ulong)FrameworkPortability.DoubleToInt64(value));
        }

        /// <summary>
        /// Writes a float field value, without a tag, to the stream.
        /// </summary>
        public void WriteFloatNoTag(float value)
        {
            byte[] rawBytes = BitConverter.GetBytes(value);
            if (!BitConverter.IsLittleEndian)
            {
                ByteArray.Reverse(rawBytes);
            }

            if (limit - position >= 4)
            {
                buffer[position++] = rawBytes[0];
                buffer[position++] = rawBytes[1];
                buffer[position++] = rawBytes[2];
                buffer[position++] = rawBytes[3];
            }
            else
            {
                WriteRawBytes(rawBytes, 0, 4);
            }
        }

        /// <summary>
        /// Writes a uint64 field value, without a tag, to the stream.
        /// </summary>
        public void WriteUInt64NoTag(ulong value)
        {
            WriteRawVarint64(value);
        }

        /// <summary>
        /// Writes an int64 field value, without a tag, to the stream.
        /// </summary>
        public void WriteInt64NoTag(long value)
        {
            WriteRawVarint64((ulong) value);
        }

        /// <summary>
        /// Writes an int32 field value, without a tag, to the stream.
        /// </summary>
        public void WriteInt32NoTag(int value)
        {
            if (value >= 0)
            {
                WriteRawVarint32((uint) value);
            }
            else
            {
                // Must sign-extend.
                WriteRawVarint64((ulong) value);
            }
        }

        /// <summary>
        /// Writes a fixed64 field value, without a tag, to the stream.
        /// </summary>
        public void WriteFixed64NoTag(ulong value)
        {
            WriteRawLittleEndian64(value);
        }

        /// <summary>
        /// Writes a fixed32 field value, without a tag, to the stream.
        /// </summary>
        public void WriteFixed32NoTag(uint value)
        {
            WriteRawLittleEndian32(value);
        }

        /// <summary>
        /// Writes a bool field value, without a tag, to the stream.
        /// </summary>
        public void WriteBoolNoTag(bool value)
        {
            WriteRawByte(value ? (byte) 1 : (byte) 0);
        }

        /// <summary>
        /// Writes a string field value, without a tag, to the stream.
        /// </summary>
        public void WriteStringNoTag(string value)
        {
            // Optimise the case where we have enough space to write
            // the string directly to the buffer, which should be common.
            int length = Encoding.UTF8.GetByteCount(value);
            WriteRawVarint32((uint) length);
            if (limit - position >= length)
            {
                Encoding.UTF8.GetBytes(value, 0, value.Length, buffer, position);
                position += length;
            }
            else
            {
                byte[] bytes = Encoding.UTF8.GetBytes(value);
                WriteRawBytes(bytes);
            }
        }

        /// <summary>
        /// Writes a group field value, without a tag, to the stream.
        /// </summary>
        public void WriteGroupNoTag(IMessageLite value)
        {
            value.WriteTo(this);
        }

        public void WriteMessageNoTag(IMessageLite value)
        {
            WriteRawVarint32((uint) value.SerializedSize);
            value.WriteTo(this);
        }

        public void WriteBytesNoTag(ByteString value)
        {
            WriteRawVarint32((uint) value.Length);
            value.WriteRawBytesTo(this);
        }

        public void WriteUInt32NoTag(uint value)
        {
            WriteRawVarint32(value);
        }

        public void WriteEnumNoTag(int value)
        {
            WriteInt32NoTag(value);
        }

        public void WriteSFixed32NoTag(int value)
        {
            WriteRawLittleEndian32((uint) value);
        }

        public void WriteSFixed64NoTag(long value)
        {
            WriteRawLittleEndian64((ulong) value);
        }

        public void WriteSInt32NoTag(int value)
        {
            WriteRawVarint32(EncodeZigZag32(value));
        }

        public void WriteSInt64NoTag(long value)
        {
            WriteRawVarint64(EncodeZigZag64(value));
        }

        #endregion

        #region Write array members

        public void WriteArray(FieldType fieldType, int fieldNumber, string fieldName, IEnumerable list)
        {
            foreach (object element in list)
            {
                WriteField(fieldType, fieldNumber, fieldName, element);
            }
        }

        public void WriteGroupArray<T>(int fieldNumber, string fieldName, IEnumerable<T> list)
            where T : IMessageLite
        {
            foreach (IMessageLite value in list)
            {
                WriteGroup(fieldNumber, fieldName, value);
            }
        }

        public void WriteMessageArray<T>(int fieldNumber, string fieldName, IEnumerable<T> list)
            where T : IMessageLite
        {
            foreach (IMessageLite value in list)
            {
                WriteMessage(fieldNumber, fieldName, value);
            }
        }

        public void WriteStringArray(int fieldNumber, string fieldName, IEnumerable<string> list)
        {
            foreach (var value in list)
            {
                WriteString(fieldNumber, fieldName, value);
            }
        }

        public void WriteBytesArray(int fieldNumber, string fieldName, IEnumerable<ByteString> list)
        {
            foreach (var value in list)
            {
                WriteBytes(fieldNumber, fieldName, value);
            }
        }

        public void WriteBoolArray(int fieldNumber, string fieldName, IEnumerable<bool> list)
        {
            foreach (var value in list)
            {
                WriteBool(fieldNumber, fieldName, value);
            }
        }

        public void WriteInt32Array(int fieldNumber, string fieldName, IEnumerable<int> list)
        {
            foreach (var value in list)
            {
                WriteInt32(fieldNumber, fieldName, value);
            }
        }

        public void WriteSInt32Array(int fieldNumber, string fieldName, IEnumerable<int> list)
        {
            foreach (var value in list)
            {
                WriteSInt32(fieldNumber, fieldName, value);
            }
        }

        public void WriteUInt32Array(int fieldNumber, string fieldName, IEnumerable<uint> list)
        {
            foreach (var value in list)
            {
                WriteUInt32(fieldNumber, fieldName, value);
            }
        }

        public void WriteFixed32Array(int fieldNumber, string fieldName, IEnumerable<uint> list)
        {
            foreach (var value in list)
            {
                WriteFixed32(fieldNumber, fieldName, value);
            }
        }

        public void WriteSFixed32Array(int fieldNumber, string fieldName, IEnumerable<int> list)
        {
            foreach (var value in list)
            {
                WriteSFixed32(fieldNumber, fieldName, value);
            }
        }

        public void WriteInt64Array(int fieldNumber, string fieldName, IEnumerable<long> list)
        {
            foreach (var value in list)
            {
                WriteInt64(fieldNumber, fieldName, value);
            }
        }

        public void WriteSInt64Array(int fieldNumber, string fieldName, IEnumerable<long> list)
        {
            foreach (var value in list)
            {
                WriteSInt64(fieldNumber, fieldName, value);
            }
        }

        public void WriteUInt64Array(int fieldNumber, string fieldName, IEnumerable<ulong> list)
        {
            foreach (var value in list)
            {
                WriteUInt64(fieldNumber, fieldName, value);
            }
        }

        public void WriteFixed64Array(int fieldNumber, string fieldName, IEnumerable<ulong> list)
        {
            foreach (var value in list)
            {
                WriteFixed64(fieldNumber, fieldName, value);
            }
        }

        public void WriteSFixed64Array(int fieldNumber, string fieldName, IEnumerable<long> list)
        {
            foreach (var value in list)
            {
                WriteSFixed64(fieldNumber, fieldName, value);
            }
        }

        public void WriteDoubleArray(int fieldNumber, string fieldName, IEnumerable<double> list)
        {
            foreach (var value in list)
            {
                WriteDouble(fieldNumber, fieldName, value);
            }
        }

        public void WriteFloatArray(int fieldNumber, string fieldName, IEnumerable<float> list)
        {
            foreach (var value in list)
            {
                WriteFloat(fieldNumber, fieldName, value);
            }
        }

        public void WriteEnumArray<T>(int fieldNumber, string fieldName, IEnumerable<T> list)
            where T : struct, IComparable, IFormattable
        {
            if (list is ICastArray)
            {
                foreach (int value in ((ICastArray) list).CastArray<int>())
                {
                    WriteEnum(fieldNumber, fieldName, value, null /*unused*/);
                }
            }
            else
            {
                foreach (object value in list)
                {
                    WriteEnum(fieldNumber, fieldName, (int) value, null /*unused*/);
                }
            }
        }

        #endregion

        #region Write packed array members

        public void WritePackedArray(FieldType fieldType, int fieldNumber, string fieldName, IEnumerable list)
        {
            int calculatedSize = 0;
            foreach (object element in list)
            {
                calculatedSize += ComputeFieldSizeNoTag(fieldType, element);
            }

            WriteTag(fieldNumber, WireFormat.WireType.LengthDelimited);
            WriteRawVarint32((uint) calculatedSize);

            foreach (object element in list)
            {
                WriteFieldNoTag(fieldType, element);
            }
        }

        public void WritePackedGroupArray<T>(int fieldNumber, string fieldName, int calculatedSize, IEnumerable<T> list)
            where T : IMessageLite
        {
            WriteTag(fieldNumber, WireFormat.WireType.LengthDelimited);
            WriteRawVarint32((uint) calculatedSize);
            foreach (IMessageLite value in list)
            {
                WriteGroupNoTag(value);
            }
        }

        public void WritePackedMessageArray<T>(int fieldNumber, string fieldName, int calculatedSize,
                                               IEnumerable<T> list)
            where T : IMessageLite
        {
            WriteTag(fieldNumber, WireFormat.WireType.LengthDelimited);
            WriteRawVarint32((uint) calculatedSize);
            foreach (IMessageLite value in list)
            {
                WriteMessageNoTag(value);
            }
        }

        public void WritePackedStringArray(int fieldNumber, string fieldName, int calculatedSize,
                                           IEnumerable<string> list)
        {
            WriteTag(fieldNumber, WireFormat.WireType.LengthDelimited);
            WriteRawVarint32((uint) calculatedSize);
            foreach (var value in list)
            {
                WriteStringNoTag(value);
            }
        }

        public void WritePackedBytesArray(int fieldNumber, string fieldName, int calculatedSize,
                                          IEnumerable<ByteString> list)
        {
            WriteTag(fieldNumber, WireFormat.WireType.LengthDelimited);
            WriteRawVarint32((uint) calculatedSize);
            foreach (var value in list)
            {
                WriteBytesNoTag(value);
            }
        }

        public void WritePackedBoolArray(int fieldNumber, string fieldName, int calculatedSize, IEnumerable<bool> list)
        {
            WriteTag(fieldNumber, WireFormat.WireType.LengthDelimited);
            WriteRawVarint32((uint) calculatedSize);
            foreach (var value in list)
            {
                WriteBoolNoTag(value);
            }
        }

        public void WritePackedInt32Array(int fieldNumber, string fieldName, int calculatedSize, IEnumerable<int> list)
        {
            WriteTag(fieldNumber, WireFormat.WireType.LengthDelimited);
            WriteRawVarint32((uint) calculatedSize);
            foreach (var value in list)
            {
                WriteInt32NoTag(value);
            }
        }

        public void WritePackedSInt32Array(int fieldNumber, string fieldName, int calculatedSize, IEnumerable<int> list)
        {
            WriteTag(fieldNumber, WireFormat.WireType.LengthDelimited);
            WriteRawVarint32((uint) calculatedSize);
            foreach (var value in list)
            {
                WriteSInt32NoTag(value);
            }
        }

        public void WritePackedUInt32Array(int fieldNumber, string fieldName, int calculatedSize, IEnumerable<uint> list)
        {
            WriteTag(fieldNumber, WireFormat.WireType.LengthDelimited);
            WriteRawVarint32((uint) calculatedSize);
            foreach (var value in list)
            {
                WriteUInt32NoTag(value);
            }
        }

        public void WritePackedFixed32Array(int fieldNumber, string fieldName, int calculatedSize,
                                            IEnumerable<uint> list)
        {
            WriteTag(fieldNumber, WireFormat.WireType.LengthDelimited);
            WriteRawVarint32((uint) calculatedSize);
            foreach (var value in list)
            {
                WriteFixed32NoTag(value);
            }
        }

        public void WritePackedSFixed32Array(int fieldNumber, string fieldName, int calculatedSize,
                                             IEnumerable<int> list)
        {
            WriteTag(fieldNumber, WireFormat.WireType.LengthDelimited);
            WriteRawVarint32((uint) calculatedSize);
            foreach (var value in list)
            {
                WriteSFixed32NoTag(value);
            }
        }

        public void WritePackedInt64Array(int fieldNumber, string fieldName, int calculatedSize, IEnumerable<long> list)
        {
            WriteTag(fieldNumber, WireFormat.WireType.LengthDelimited);
            WriteRawVarint32((uint) calculatedSize);
            foreach (var value in list)
            {
                WriteInt64NoTag(value);
            }
        }

        public void WritePackedSInt64Array(int fieldNumber, string fieldName, int calculatedSize, IEnumerable<long> list)
        {
            WriteTag(fieldNumber, WireFormat.WireType.LengthDelimited);
            WriteRawVarint32((uint) calculatedSize);
            foreach (var value in list)
            {
                WriteSInt64NoTag(value);
            }
        }

        public void WritePackedUInt64Array(int fieldNumber, string fieldName, int calculatedSize,
                                           IEnumerable<ulong> list)
        {
            WriteTag(fieldNumber, WireFormat.WireType.LengthDelimited);
            WriteRawVarint32((uint) calculatedSize);
            foreach (var value in list)
            {
                WriteUInt64NoTag(value);
            }
        }

        public void WritePackedFixed64Array(int fieldNumber, string fieldName, int calculatedSize,
                                            IEnumerable<ulong> list)
        {
            WriteTag(fieldNumber, WireFormat.WireType.LengthDelimited);
            WriteRawVarint32((uint) calculatedSize);
            foreach (var value in list)
            {
                WriteFixed64NoTag(value);
            }
        }

        public void WritePackedSFixed64Array(int fieldNumber, string fieldName, int calculatedSize,
                                             IEnumerable<long> list)
        {
            WriteTag(fieldNumber, WireFormat.WireType.LengthDelimited);
            WriteRawVarint32((uint) calculatedSize);
            foreach (var value in list)
            {
                WriteSFixed64NoTag(value);
            }
        }

        public void WritePackedDoubleArray(int fieldNumber, string fieldName, int calculatedSize,
                                           IEnumerable<double> list)
        {
            WriteTag(fieldNumber, WireFormat.WireType.LengthDelimited);
            WriteRawVarint32((uint) calculatedSize);
            foreach (var value in list)
            {
                WriteDoubleNoTag(value);
            }
        }

        public void WritePackedFloatArray(int fieldNumber, string fieldName, int calculatedSize, IEnumerable<float> list)
        {
            WriteTag(fieldNumber, WireFormat.WireType.LengthDelimited);
            WriteRawVarint32((uint) calculatedSize);
            foreach (var value in list)
            {
                WriteFloatNoTag(value);
            }
        }

        public void WritePackedEnumArray<T>(int fieldNumber, string fieldName, int calculatedSize, IEnumerable<T> list)
            where T : struct, IComparable, IFormattable
        {
            WriteTag(fieldNumber, WireFormat.WireType.LengthDelimited);
            WriteRawVarint32((uint) calculatedSize);
            if (list is ICastArray)
            {
                foreach (int value in ((ICastArray) list).CastArray<int>())
                {
                    WriteEnumNoTag(value);
                }
            }
            else
            {
                foreach (object value in list)
                {
                    WriteEnumNoTag((int) value);
                }
            }
        }

        #endregion

        #region Underlying writing primitives

        /// <summary>
        /// Encodes and writes a tag.
        /// </summary>
        public void WriteTag(int fieldNumber, WireFormat.WireType type)
        {
            WriteRawVarint32(WireFormat.MakeTag(fieldNumber, type));
        }

        /// <summary>
        /// Writes a 32 bit value as a varint. The fast route is taken when
        /// there's enough buffer space left to whizz through without checking
        /// for each byte; otherwise, we resort to calling WriteRawByte each time.
        /// </summary>
        public void WriteRawVarint32(uint value)
        {
            while (value > 127 && position < limit)
            {
                buffer[position++] = (byte) ((value & 0x7F) | 0x80);
                value >>= 7;
            }
            while (value > 127)
            {
                WriteRawByte((byte) ((value & 0x7F) | 0x80));
                value >>= 7;
            }
            if (position < limit)
            {
                buffer[position++] = (byte) value;
            }
            else
            {
                WriteRawByte((byte) value);
            }
        }

        public void WriteRawVarint64(ulong value)
        {
            while (value > 127 && position < limit)
            {
                buffer[position++] = (byte) ((value & 0x7F) | 0x80);
                value >>= 7;
            }
            while (value > 127)
            {
                WriteRawByte((byte) ((value & 0x7F) | 0x80));
                value >>= 7;
            }
            if (position < limit)
            {
                buffer[position++] = (byte) value;
            }
            else
            {
                WriteRawByte((byte) value);
            }
        }

        public void WriteRawLittleEndian32(uint value)
        {
            if (position + 4 > limit)
            {
                WriteRawByte((byte) value);
                WriteRawByte((byte) (value >> 8));
                WriteRawByte((byte) (value >> 16));
                WriteRawByte((byte) (value >> 24));
            }
            else
            {
                buffer[position++] = ((byte) value);
                buffer[position++] = ((byte) (value >> 8));
                buffer[position++] = ((byte) (value >> 16));
                buffer[position++] = ((byte) (value >> 24));
            }
        }

        public void WriteRawLittleEndian64(ulong value)
        {
            if (position + 8 > limit)
            {
                WriteRawByte((byte) value);
                WriteRawByte((byte) (value >> 8));
                WriteRawByte((byte) (value >> 16));
                WriteRawByte((byte) (value >> 24));
                WriteRawByte((byte) (value >> 32));
                WriteRawByte((byte) (value >> 40));
                WriteRawByte((byte) (value >> 48));
                WriteRawByte((byte) (value >> 56));
            }
            else
            {
                buffer[position++] = ((byte) value);
                buffer[position++] = ((byte) (value >> 8));
                buffer[position++] = ((byte) (value >> 16));
                buffer[position++] = ((byte) (value >> 24));
                buffer[position++] = ((byte) (value >> 32));
                buffer[position++] = ((byte) (value >> 40));
                buffer[position++] = ((byte) (value >> 48));
                buffer[position++] = ((byte) (value >> 56));
            }
        }

        public void WriteRawByte(byte value)
        {
            if (position == limit)
            {
                RefreshBuffer();
            }

            buffer[position++] = value;
        }

        public void WriteRawByte(uint value)
        {
            WriteRawByte((byte) value);
        }

        /// <summary>
        /// Writes out an array of bytes.
        /// </summary>
        public void WriteRawBytes(byte[] value)
        {
            WriteRawBytes(value, 0, value.Length);
        }

        /// <summary>
        /// Writes out part of an array of bytes.
        /// </summary>
        public void WriteRawBytes(byte[] value, int offset, int length)
        {
            if (limit - position >= length)
            {
                ByteArray.Copy(value, offset, buffer, position, length);
                // We have room in the current buffer.
                position += length;
            }
            else
            {
                // Write extends past current buffer.  Fill the rest of this buffer and
                // flush.
                int bytesWritten = limit - position;
                ByteArray.Copy(value, offset, buffer, position, bytesWritten);
                offset += bytesWritten;
                length -= bytesWritten;
                position = limit;
                RefreshBuffer();

                // Now deal with the rest.
                // Since we have an output stream, this is our buffer
                // and buffer offset == 0
                if (length <= limit)
                {
                    // Fits in new buffer.
                    ByteArray.Copy(value, offset, buffer, 0, length);
                    position = length;
                }
                else
                {
                    // Write is very big.  Let's do it all at once.
                    output.Write(value, offset, length);
                }
            }
        }

        #endregion

        /// <summary>
        /// Encode a 32-bit value with ZigZag encoding.
        /// </summary>
        /// <remarks>
        /// ZigZag encodes signed integers into values that can be efficiently
        /// encoded with varint.  (Otherwise, negative values must be 
        /// sign-extended to 64 bits to be varint encoded, thus always taking
        /// 10 bytes on the wire.)
        /// </remarks>
        public static uint EncodeZigZag32(int n)
        {
            // Note:  the right-shift must be arithmetic
            return (uint) ((n << 1) ^ (n >> 31));
        }

        /// <summary>
        /// Encode a 64-bit value with ZigZag encoding.
        /// </summary>
        /// <remarks>
        /// ZigZag encodes signed integers into values that can be efficiently
        /// encoded with varint.  (Otherwise, negative values must be 
        /// sign-extended to 64 bits to be varint encoded, thus always taking
        /// 10 bytes on the wire.)
        /// </remarks>
        public static ulong EncodeZigZag64(long n)
        {
            return (ulong) ((n << 1) ^ (n >> 63));
        }

        private void RefreshBuffer()
        {
            if (output == null)
            {
                // We're writing to a single buffer.
                throw new OutOfSpaceException();
            }

            // Since we have an output stream, this is our buffer
            // and buffer offset == 0
            output.Write(buffer, 0, position);
            position = 0;
        }

        /// <summary>
        /// Indicates that a CodedOutputStream wrapping a flat byte array
        /// ran out of space.
        /// </summary>
        public sealed class OutOfSpaceException : IOException
        {
            internal OutOfSpaceException()
                : base("CodedOutputStream was writing to a flat byte array and ran out of space.")
            {
            }
        }

        public void Flush()
        {
            if (output != null)
            {
                RefreshBuffer();
            }
        }

        /// <summary>
        /// Verifies that SpaceLeft returns zero. It's common to create a byte array
        /// that is exactly big enough to hold a message, then write to it with
        /// a CodedOutputStream. Calling CheckNoSpaceLeft after writing verifies that
        /// the message was actually as big as expected, which can help bugs.
        /// </summary>
        public void CheckNoSpaceLeft()
        {
            if (SpaceLeft != 0)
            {
                throw new InvalidOperationException("Did not write as much data as expected.");
            }
        }

        /// <summary>
        /// If writing to a flat array, returns the space left in the array. Otherwise,
        /// throws an InvalidOperationException.
        /// </summary>
        public int SpaceLeft
        {
            get
            {
                if (output == null)
                {
                    return limit - position;
                }
                else
                {
                    throw new InvalidOperationException(
                        "SpaceLeft can only be called on CodedOutputStreams that are " +
                        "writing to a flat array.");
                }
            }
        }
    }
}