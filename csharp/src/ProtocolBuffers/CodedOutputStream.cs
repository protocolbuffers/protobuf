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
using System.IO;
using System.Linq;
using System.Text;
using Google.Protobuf.Collections;
using Google.Protobuf.Descriptors;

namespace Google.Protobuf
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
    public sealed partial class CodedOutputStream
    {
        private static readonly Encoding UTF8 = Encoding.UTF8;

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

        #region Writing of tags and fields
        /// <summary>
        /// Writes a double field value, including tag, to the stream.
        /// </summary>
        public void WriteDouble(int fieldNumber, double value)
        {
            WriteTag(fieldNumber, WireFormat.WireType.Fixed64);
            WriteDoubleNoTag(value);
        }

        /// <summary>
        /// Writes a float field value, including tag, to the stream.
        /// </summary>
        public void WriteFloat(int fieldNumber, float value)
        {
            WriteTag(fieldNumber, WireFormat.WireType.Fixed32);
            WriteFloatNoTag(value);
        }

        /// <summary>
        /// Writes a uint64 field value, including tag, to the stream.
        /// </summary>
        public void WriteUInt64(int fieldNumber, ulong value)
        {
            WriteTag(fieldNumber, WireFormat.WireType.Varint);
            WriteRawVarint64(value);
        }

        /// <summary>
        /// Writes an int64 field value, including tag, to the stream.
        /// </summary>
        public void WriteInt64(int fieldNumber, long value)
        {
            WriteTag(fieldNumber, WireFormat.WireType.Varint);
            WriteRawVarint64((ulong) value);
        }

        /// <summary>
        /// Writes an int32 field value, including tag, to the stream.
        /// </summary>
        public void WriteInt32(int fieldNumber, int value)
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
        public void WriteFixed64(int fieldNumber, ulong value)
        {
            WriteTag(fieldNumber, WireFormat.WireType.Fixed64);
            WriteRawLittleEndian64(value);
        }

        /// <summary>
        /// Writes a fixed32 field value, including tag, to the stream.
        /// </summary>
        public void WriteFixed32(int fieldNumber, uint value)
        {
            WriteTag(fieldNumber, WireFormat.WireType.Fixed32);
            WriteRawLittleEndian32(value);
        }

        /// <summary>
        /// Writes a bool field value, including tag, to the stream.
        /// </summary>
        public void WriteBool(int fieldNumber, bool value)
        {
            WriteTag(fieldNumber, WireFormat.WireType.Varint);
            WriteRawByte(value ? (byte) 1 : (byte) 0);
        }

        /// <summary>
        /// Writes a string field value, including tag, to the stream.
        /// </summary>
        public void WriteString(int fieldNumber, string value)
        {
            WriteTag(fieldNumber, WireFormat.WireType.LengthDelimited);
            // Optimise the case where we have enough space to write
            // the string directly to the buffer, which should be common.
            int length = UTF8.GetByteCount(value);
            WriteRawVarint32((uint) length);
            if (limit - position >= length)
            {
                if (length == value.Length) // Must be all ASCII...
                {
                                for (int i = 0; i < length; i++)
                                {
                        buffer[position + i] = (byte)value[i];
                    }
                }
                else
                {
                    UTF8.GetBytes(value, 0, value.Length, buffer, position);
                }
                position += length;
            }
            else
            {
                byte[] bytes = UTF8.GetBytes(value);
                WriteRawBytes(bytes);
            }
        }

        /// <summary>
        /// Writes a group field value, including tag, to the stream.
        /// </summary>
        public void WriteGroup(int fieldNumber, IMessage value)
        {
            WriteTag(fieldNumber, WireFormat.WireType.StartGroup);
            value.WriteTo(this);
            WriteTag(fieldNumber, WireFormat.WireType.EndGroup);
        }

        public void WriteMessage(int fieldNumber, IMessage value)
        {
            WriteTag(fieldNumber, WireFormat.WireType.LengthDelimited);
            WriteRawVarint32((uint) value.CalculateSize());
            value.WriteTo(this);
        }

        public void WriteBytes(int fieldNumber, ByteString value)
        {
            WriteTag(fieldNumber, WireFormat.WireType.LengthDelimited);
            WriteRawVarint32((uint) value.Length);
            value.WriteRawBytesTo(this);
        }

        public void WriteUInt32(int fieldNumber, uint value)
        {
            WriteTag(fieldNumber, WireFormat.WireType.Varint);
            WriteRawVarint32(value);
        }

        public void WriteEnum(int fieldNumber, int value)
        {
            // Currently just a pass-through, but it's nice to separate it logically from WriteInt32.
            WriteTag(fieldNumber, WireFormat.WireType.Varint);
            WriteInt32NoTag(value);
        }

        public void WriteSFixed32(int fieldNumber, int value)
        {
            WriteTag(fieldNumber, WireFormat.WireType.Fixed32);
            WriteRawLittleEndian32((uint) value);
        }

        public void WriteSFixed64(int fieldNumber, long value)
        {
            WriteTag(fieldNumber, WireFormat.WireType.Fixed64);
            WriteRawLittleEndian64((ulong) value);
        }

        public void WriteSInt32(int fieldNumber, int value)
        {
            WriteTag(fieldNumber, WireFormat.WireType.Varint);
            WriteRawVarint32(EncodeZigZag32(value));
        }

        public void WriteSInt64(int fieldNumber, long value)
        {
            WriteTag(fieldNumber, WireFormat.WireType.Varint);
            WriteRawVarint64(EncodeZigZag64(value));
        }
        #endregion

        #region Writing of values without tags
        // TODO(jonskeet): Remove this?
        public void WriteFieldNoTag(FieldType fieldType, object value)
        {
            switch (fieldType)
            {
                case FieldType.String:
                    WriteStringNoTag((string) value);
                    break;
                case FieldType.Message:
                    WriteMessageNoTag((IMessage) value);
                    break;
                case FieldType.Group:
                    WriteGroupNoTag((IMessage) value);
                    break;
                case FieldType.Bytes:
                    WriteBytesNoTag((ByteString) value);
                    break;
                case FieldType.Bool:
                    WriteBoolNoTag((bool) value);
                    break;
                case FieldType.Enum:
                    WriteEnumNoTag((int) value);
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
        public void WriteGroupNoTag(IMessage value)
        {
            value.WriteTo(this);
        }

        public void WriteMessageNoTag(IMessage value)
        {
            WriteRawVarint32((uint) value.CalculateSize());
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
        public void WriteMessageArray<T>(int fieldNumber, RepeatedField<T> list)
            where T : IMessage
        {
            if (list.Count == 0)
            {
                return;
            }
            foreach (T value in list)
            {
                WriteMessage(fieldNumber, value);
            }
        }

        public void WriteStringArray(int fieldNumber, RepeatedField<string> list)
        {
            if (list.Count == 0)
            {
                return;
            }
            foreach (var value in list)
            {
                WriteString(fieldNumber, value);
            }
        }

        public void WriteBytesArray(int fieldNumber, RepeatedField<ByteString> list)
        {
            if (list.Count == 0)
            {
                return;
            }
            foreach (var value in list)
            {
                WriteBytes(fieldNumber, value);
            }
        }

        public void WriteBoolArray(int fieldNumber, RepeatedField<bool> list)
        {
            if (list.Count == 0)
            {
                return;
            }
            foreach (var value in list)
            {
                WriteBool(fieldNumber, value);
            }
        }

        public void WriteInt32Array(int fieldNumber, RepeatedField<int> list)
        {
            if (list.Count == 0)
            {
                return;
            }
            foreach (var value in list)
            {
                WriteInt32(fieldNumber, value);
            }
        }

        public void WriteSInt32Array(int fieldNumber, RepeatedField<int> list)
        {
            if (list.Count == 0)
            {
                return;
            }
            foreach (var value in list)
            {
                WriteSInt32(fieldNumber, value);
            }
        }

        public void WriteUInt32Array(int fieldNumber, RepeatedField<uint> list)
        {
            if (list.Count == 0)
            {
                return;
            }
            foreach (var value in list)
            {
                WriteUInt32(fieldNumber, value);
            }
        }

        public void WriteFixed32Array(int fieldNumber, RepeatedField<uint> list)
        {
            if (list.Count == 0)
            {
                return;
            }
            foreach (var value in list)
            {
                WriteFixed32(fieldNumber, value);
            }
        }

        public void WriteSFixed32Array(int fieldNumber, RepeatedField<int> list)
        {
            if (list.Count == 0)
            {
                return;
            }
            foreach (var value in list)
            {
                WriteSFixed32(fieldNumber, value);
            }
        }

        public void WriteInt64Array(int fieldNumber, RepeatedField<long> list)
        {
            if (list.Count == 0)
            {
                return;
            }
            foreach (var value in list)
            {
                WriteInt64(fieldNumber, value);
            }
        }

        public void WriteSInt64Array(int fieldNumber, RepeatedField<long> list)
        {
            if (list.Count == 0)
            {
                return;
            }
            foreach (var value in list)
            {
                WriteSInt64(fieldNumber, value);
            }
        }

        public void WriteUInt64Array(int fieldNumber, RepeatedField<ulong> list)
        {
            if (list.Count == 0)
            {
                return;
            }
            foreach (var value in list)
            {
                WriteUInt64(fieldNumber, value);
            }
        }

        public void WriteFixed64Array(int fieldNumber, RepeatedField<ulong> list)
        {
            if (list.Count == 0)
            {
                return;
            }
            foreach (var value in list)
            {
                WriteFixed64(fieldNumber, value);
            }
        }

        public void WriteSFixed64Array(int fieldNumber, RepeatedField<long> list)
        {
            if (list.Count == 0)
            {
                return;
            }
            foreach (var value in list)
            {
                WriteSFixed64(fieldNumber, value);
            }
        }

        public void WriteDoubleArray(int fieldNumber, RepeatedField<double> list)
        {
            if (list.Count == 0)
            {
                return;
            }
            foreach (var value in list)
            {
                WriteDouble(fieldNumber, value);
            }
        }

        public void WriteFloatArray(int fieldNumber, RepeatedField<float> list)
        {
            if (list.Count == 0)
            {
                return;
            }
            foreach (var value in list)
            {
                WriteFloat(fieldNumber, value);
            }
        }

        public void WriteEnumArray<T>(int fieldNumber, RepeatedField<T> list)
            where T : struct, IComparable, IFormattable
        {
            if (list.Count == 0)
            {
                return;
            }
            // TODO(jonskeet): Avoid the Cast call here. Work out a better mass "T to int" conversion.
            foreach (int value in list.Cast<int>())
            {
                WriteEnum(fieldNumber, value);
            }
        }

        #endregion

        #region Write packed array members
        // TODO(jonskeet): A lot of these are really inefficient, due to method group conversions. Fix!
        public void WritePackedBoolArray(int fieldNumber, RepeatedField<bool> list)
        {
            if (list.Count == 0)
            {
                return;
            }
            uint size = (uint)list.Count;
            WriteTag(fieldNumber, WireFormat.WireType.LengthDelimited);
            WriteRawVarint32(size);
            foreach (var value in list)
            {
                WriteBoolNoTag(value);
            }
        }

        public void WritePackedInt32Array(int fieldNumber, RepeatedField<int> list)
        {
            if (list.Count == 0)
            {
                return;
            }
            uint size = list.CalculateSize(ComputeInt32SizeNoTag);
            WriteTag(fieldNumber, WireFormat.WireType.LengthDelimited);
            WriteRawVarint32(size);
            foreach (var value in list)
            {
                WriteInt32NoTag(value);
            }
        }

        public void WritePackedSInt32Array(int fieldNumber, RepeatedField<int> list)
        {
            if (list.Count == 0)
            {
                return;
            }
            uint size = list.CalculateSize(ComputeSInt32SizeNoTag);
            WriteTag(fieldNumber, WireFormat.WireType.LengthDelimited);
            WriteRawVarint32(size);
            foreach (var value in list)
            {
                WriteSInt32NoTag(value);
            }
        }

        public void WritePackedUInt32Array(int fieldNumber, RepeatedField<uint> list)
        {
            if (list.Count == 0)
            {
                return;
            }
            uint size = list.CalculateSize(ComputeUInt32SizeNoTag);
            WriteTag(fieldNumber, WireFormat.WireType.LengthDelimited);
            WriteRawVarint32(size);
            foreach (var value in list)
            {
                WriteUInt32NoTag(value);
            }
        }

        public void WritePackedFixed32Array(int fieldNumber, RepeatedField<uint> list)
        {
            if (list.Count == 0)
            {
                return;
            }
            uint size = (uint) list.Count * 4;
            WriteTag(fieldNumber, WireFormat.WireType.LengthDelimited);
            WriteRawVarint32(size);
            foreach (var value in list)
            {
                WriteFixed32NoTag(value);
            }
        }

        public void WritePackedSFixed32Array(int fieldNumber, RepeatedField<int> list)
        {
            if (list.Count == 0)
            {
                return;
            }
            uint size = (uint) list.Count * 4;
            WriteTag(fieldNumber, WireFormat.WireType.LengthDelimited);
            WriteRawVarint32(size);
            foreach (var value in list)
            {
                WriteSFixed32NoTag(value);
            }
        }

        public void WritePackedInt64Array(int fieldNumber, RepeatedField<long> list)
        {
            if (list.Count == 0)
            {
                return;
            }
            uint size = list.CalculateSize(ComputeInt64SizeNoTag);
            WriteTag(fieldNumber, WireFormat.WireType.LengthDelimited);
            WriteRawVarint32(size);
            foreach (var value in list)
            {
                WriteInt64NoTag(value);
            }
        }

        public void WritePackedSInt64Array(int fieldNumber, RepeatedField<long> list)
        {
            if (list.Count == 0)
            {
                return;
            }
            uint size = list.CalculateSize(ComputeSInt64SizeNoTag);
            WriteTag(fieldNumber, WireFormat.WireType.LengthDelimited);
            WriteRawVarint32(size);
            foreach (var value in list)
            {
                WriteSInt64NoTag(value);
            }
        }

        public void WritePackedUInt64Array(int fieldNumber, RepeatedField<ulong> list)
        {
            if (list.Count == 0)
            {
                return;
            }
            uint size = list.CalculateSize(ComputeUInt64SizeNoTag);
            WriteTag(fieldNumber, WireFormat.WireType.LengthDelimited);
            WriteRawVarint32(size);
            foreach (var value in list)
            {
                WriteUInt64NoTag(value);
            }
        }

        public void WritePackedFixed64Array(int fieldNumber, RepeatedField<ulong> list)
        {
            if (list.Count == 0)
            {
                return;
            }
            uint size = (uint) list.Count * 8;
            WriteTag(fieldNumber, WireFormat.WireType.LengthDelimited);
            WriteRawVarint32(size);
            foreach (var value in list)
            {
                WriteFixed64NoTag(value);
            }
        }

        public void WritePackedSFixed64Array(int fieldNumber, RepeatedField<long> list)
        {
            if (list.Count == 0)
            {
                return;
            }
            uint size = (uint) list.Count * 8;
            WriteTag(fieldNumber, WireFormat.WireType.LengthDelimited);
            WriteRawVarint32(size);
            foreach (var value in list)
            {
                WriteSFixed64NoTag(value);
            }
        }

        public void WritePackedDoubleArray(int fieldNumber, RepeatedField<double> list)
        {
            if (list.Count == 0)
            {
                return;
            }
            uint size = (uint) list.Count * 8;
            WriteTag(fieldNumber, WireFormat.WireType.LengthDelimited);
            WriteRawVarint32(size);
            foreach (var value in list)
            {
                WriteDoubleNoTag(value);
            }
        }

        public void WritePackedFloatArray(int fieldNumber, RepeatedField<float> list)
        {
            if (list.Count == 0)
            {
                return;
            }
            uint size = (uint) list.Count * 4;
            WriteTag(fieldNumber, WireFormat.WireType.LengthDelimited);
            WriteRawVarint32(size);
            foreach (var value in list)
            {
                WriteFloatNoTag(value);
            }
        }

        public void WritePackedEnumArray<T>(int fieldNumber, RepeatedField<T> list)
            where T : struct, IComparable, IFormattable
        {
            if (list.Count == 0)
            {
                return;
            }
            // Obviously, we'll want to get rid of this hack...
            var temporaryHack = new RepeatedField<int>();
            temporaryHack.Add(list.Cast<int>());
            uint size = temporaryHack.CalculateSize(ComputeEnumSizeNoTag);
            WriteTag(fieldNumber, WireFormat.WireType.LengthDelimited);
            WriteRawVarint32(size);
            foreach (int value in temporaryHack)
            {
                WriteEnumNoTag(value);
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
            // Optimize for the common case of a single byte value
            if (value < 128 && position < limit)
            {
                buffer[position++] = (byte)value;
                return;
            }

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