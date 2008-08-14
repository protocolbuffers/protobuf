// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.
// http://code.google.com/p/protobuf/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
using System;
using System.IO;
using System.Text;

namespace Google.ProtocolBuffers {

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
  public sealed class CodedOutputStream {
    /// <summary>
    /// The buffer size used by CreateInstance(Stream).
    /// </summary>
    public static readonly int DefaultBufferSize = 4096;

    private readonly byte[] buffer;
    private readonly int limit;
    private int position;
    private readonly Stream output;

    #region Construction
    private CodedOutputStream(byte[] buffer, int offset, int length) {
      this.output = null;
      this.buffer = buffer;
      this.position = offset;
      this.limit = offset + length;
    }

    private CodedOutputStream(Stream output, byte[] buffer) {
      this.output = output;
      this.buffer = buffer;
      this.position = 0;
      this.limit = buffer.Length;
    }

    /// <summary>
    /// Creates a new CodedOutputStream which write to the given stream.
    /// </summary>
    public static CodedOutputStream CreateInstance(Stream output) {
      return CreateInstance(output, DefaultBufferSize);
    }

    /// <summary>
    /// Creates a new CodedOutputStream which write to the given stream and uses
    /// the specified buffer size.
    /// </summary>
    public static CodedOutputStream CreateInstance(Stream output, int bufferSize) {
      return new CodedOutputStream(output, new byte[bufferSize]);
    }

    /// <summary>
    /// Creates a new CodedOutputStream that writes directly to the given
    /// byte array. If more bytes are written than fit in the array,
    /// OutOfSpaceException will be thrown.
    /// </summary>
    public static CodedOutputStream CreateInstance(byte[] flatArray) {
      return CreateInstance(flatArray, 0, flatArray.Length);
    }

    /// <summary>
    /// Creates a new CodedOutputStream that writes directly to the given
    /// byte array slice. If more bytes are written than fit in the array,
    /// OutOfSpaceException will be thrown.
    /// </summary>
    public static CodedOutputStream CreateInstance(byte[] flatArray, int offset, int length) {
      return new CodedOutputStream(flatArray, offset, length);
    }
    #endregion

    #region Writing of tags etc
    /// <summary>
    /// Writes a double field value, including tag, to the stream.
    /// </summary>
    public void WriteDouble(int fieldNumber, double value) {
      WriteTag(fieldNumber, WireFormat.WireType.Fixed64);
      WriteRawLittleEndian64(BitConverter.DoubleToInt64Bits(value));
    }

    /// <summary>
    /// Writes a float field value, including tag, to the stream.
    /// </summary>
    public void WriteFloat(int fieldNumber, float value) {
      WriteTag(fieldNumber, WireFormat.WireType.Fixed32);
      // FIXME: How do we convert a single to 32 bits? (Without unsafe code)
      //WriteRawLittleEndian32(BitConverter.SingleT(value));
    }

    /// <summary>
    /// Writes a uint64 field value, including tag, to the stream.
    /// </summary>
    public void WriteUInt64(int fieldNumber, ulong value) {
      WriteTag(fieldNumber, WireFormat.WireType.Varint);
      WriteRawVarint64(value);
    }

    /// <summary>
    /// Writes an int64 field value, including tag, to the stream.
    /// </summary>
    public void WriteInt64(int fieldNumber, long value) {
      WriteTag(fieldNumber, WireFormat.WireType.Varint);
      WriteRawVarint64((ulong)value);
    }

    /// <summary>
    /// Writes an int32 field value, including tag, to the stream.
    /// </summary>
    public void WriteInt32(int fieldNumber, int value) {
      WriteTag(fieldNumber, WireFormat.WireType.Varint);
      if (value >= 0) {
        WriteRawVarint32((uint)value);
      } else {
        // Must sign-extend.
        WriteRawVarint64((ulong)value);
      }
    }

    /// <summary>
    /// Writes a fixed64 field value, including tag, to the stream.
    /// </summary>
    public void WriteFixed64(int fieldNumber, long value) {
      WriteTag(fieldNumber, WireFormat.WireType.Fixed64);
      WriteRawLittleEndian64(value);
    }

    /// <summary>
    /// Writes a fixed32 field value, including tag, to the stream.
    /// </summary>
    public void WriteFixed32(int fieldNumber, int value) {
      WriteTag(fieldNumber, WireFormat.WireType.Fixed32);
      WriteRawLittleEndian32(value);
    }

    /// <summary>
    /// Writes a bool field value, including tag, to the stream.
    /// </summary>
    public void WriteBool(int fieldNumber, bool value) {
      WriteTag(fieldNumber, WireFormat.WireType.Varint);
      WriteRawByte(value ? (byte)1 : (byte)0);
    }

    /// <summary>
    /// Writes a string field value, including tag, to the stream.
    /// </summary>
    public void WriteString(int fieldNumber, string value) {
      WriteTag(fieldNumber, WireFormat.WireType.LengthDelimited);
      // TODO(jonskeet): Optimise this if possible
      // Unfortunately there does not appear to be any way to tell Java to encode
      // UTF-8 directly into our buffer, so we have to let it create its own byte
      // array and then copy. In .NET we can do the same thing very easily,
      // so we don't need to worry about only writing one buffer at a time.
      // We can optimise later.
      byte[] bytes = Encoding.UTF8.GetBytes(value);
      WriteRawVarint32((uint)bytes.Length);
      WriteRawBytes(bytes);
    }

    /// <summary>
    /// Writes a group field value, including tag, to the stream.
    /// </summary>
    public void WriteGroup(int fieldNumber, IMessage value) {
      WriteTag(fieldNumber, WireFormat.WireType.StartGroup);
      value.WriteTo(this);
      WriteTag(fieldNumber, WireFormat.WireType.EndGroup);
    }

    public void WriteUnknownGroup(int fieldNumber, UnknownFieldSet value) {
      WriteTag(fieldNumber, WireFormat.WireType.StartGroup);
      value.WriteTo(this);
      WriteTag(fieldNumber, WireFormat.WireType.EndGroup);
    }

    public void WriteMessage(int fieldNumber, IMessage value) {
      WriteTag(fieldNumber, WireFormat.WireType.LengthDelimited);
      WriteRawVarint32((uint)value.SerializedSize);
      value.WriteTo(this);
    }

    public void WriteBytes(int fieldNumber, ByteString value) {
      // TODO(jonskeet): Optimise this! (No need to copy the bytes twice.)
      byte[] bytes = value.ToByteArray();
      WriteRawVarint32((uint)bytes.Length);
      WriteRawBytes(bytes);
    }

    public void WriteUInt32(int fieldNumber, uint value) {
      WriteTag(fieldNumber, WireFormat.WireType.Varint);
      WriteRawVarint32(value);
    }

    public void WriteEnum(int fieldNumber, int value) {
      WriteTag(fieldNumber, WireFormat.WireType.Varint);
      WriteRawVarint32((uint)value);
    }

    public void WriteSFixed32(int fieldNumber, int value) {
      WriteTag(fieldNumber, WireFormat.WireType.Fixed32);
      WriteRawVarint32((uint)value);
    }

    public void WriteSFixed64(int fieldNumber, long value) {
      WriteTag(fieldNumber, WireFormat.WireType.Fixed64);
      WriteRawVarint64((ulong)value);
    }

    public void WriteSInt32(int fieldNumber, int value) {
      WriteTag(fieldNumber, WireFormat.WireType.Varint);
      WriteRawVarint32(EncodeZigZag32(value));
    }

    public void WriteSInt64(int fieldNumber, long value) {
      WriteTag(fieldNumber, WireFormat.WireType.Varint);
      WriteRawVarint64(EncodeZigZag64(value));
    }

    public void WriteMessageSetExtension(int fieldNumber, IMessage value) {
      WriteTag(WireFormat.MessageSetField.Item, WireFormat.WireType.StartGroup);
      WriteUInt32(WireFormat.MessageSetField.TypeID, (uint)fieldNumber);
      WriteMessage(WireFormat.MessageSetField.Message, value);
      WriteTag(WireFormat.MessageSetField.Item, WireFormat.WireType.EndGroup);
    }

    public void WriteRawMessageSetExtension(int fieldNumber, ByteString value) {
      WriteTag(WireFormat.MessageSetField.Item, WireFormat.WireType.StartGroup);
      WriteUInt32(WireFormat.MessageSetField.TypeID, (uint)fieldNumber);
      WriteBytes(WireFormat.MessageSetField.Message, value);
      WriteTag(WireFormat.MessageSetField.Item, WireFormat.WireType.EndGroup);
    }

    public void WriteField(Descriptors.FieldDescriptor.Type fieldType, int fieldNumber, object value) {
      switch (fieldType) {
        case Descriptors.FieldDescriptor.Type.Double: WriteDouble(fieldNumber, (double)value); break;
        case Descriptors.FieldDescriptor.Type.Float: WriteFloat(fieldNumber, (float)value); break;
        case Descriptors.FieldDescriptor.Type.Int64: WriteInt64(fieldNumber, (long)value); break;
        case Descriptors.FieldDescriptor.Type.UInt64: WriteUInt64(fieldNumber, (ulong)value); break;
        case Descriptors.FieldDescriptor.Type.Int32: WriteInt32(fieldNumber, (int)value); break;
        case Descriptors.FieldDescriptor.Type.Fixed64: WriteFixed64(fieldNumber, (long)value); break;
        case Descriptors.FieldDescriptor.Type.Fixed32: WriteFixed32(fieldNumber, (int)value); break;
        case Descriptors.FieldDescriptor.Type.Bool: WriteBool(fieldNumber, (bool)value); break;
        case Descriptors.FieldDescriptor.Type.String: WriteString(fieldNumber, (string)value); break;
        case Descriptors.FieldDescriptor.Type.Group: WriteGroup(fieldNumber, (IMessage)value); break;
        case Descriptors.FieldDescriptor.Type.Message: WriteMessage(fieldNumber, (IMessage)value); break;
        case Descriptors.FieldDescriptor.Type.Bytes: WriteBytes(fieldNumber, (ByteString)value); break;
        case Descriptors.FieldDescriptor.Type.UInt32: WriteUInt32(fieldNumber, (uint)value); break;
        case Descriptors.FieldDescriptor.Type.SFixed32: WriteSFixed32(fieldNumber, (int)value); break;
        case Descriptors.FieldDescriptor.Type.SFixed64: WriteSFixed64(fieldNumber, (long)value); break;
        case Descriptors.FieldDescriptor.Type.SInt32: WriteSInt32(fieldNumber, (int)value); break;
        case Descriptors.FieldDescriptor.Type.SInt64: WriteSInt64(fieldNumber, (long)value); break;
        case Descriptors.FieldDescriptor.Type.Enum: WriteEnum(fieldNumber, ((Descriptors.EnumValueDescriptor)value).Number);
          break;
      }
    }

    #endregion

    #region Underlying writing primitives
    /// <summary>
    /// Encodes and writes a tag.
    /// </summary>
    public void WriteTag(int fieldNumber, WireFormat.WireType type) {
      WriteRawVarint32(WireFormat.MakeTag(fieldNumber, type));
    }

    public void WriteRawVarint32(uint value) {
      while (true) {
        if ((value & ~0x7F) == 0) {
          WriteRawByte(value);
          return;
        } else {
          WriteRawByte((value & 0x7F) | 0x80);
          value >>= 7;
        }
      }
    }

    public void WriteRawVarint64(ulong value) {
      while (true) {
        if ((value & ~0x7FUL) == 0) {
          WriteRawByte((uint)value);
          return;
        } else {
          WriteRawByte(((uint)value & 0x7F) | 0x80);
          value >>= 7;
        }
      }
    }

    public void WriteRawLittleEndian32(int value) {
      WriteRawByte((byte)value);
      WriteRawByte((byte)(value >> 8));
      WriteRawByte((byte)(value >> 16));
      WriteRawByte((byte)(value >> 24));
    }

    public void WriteRawLittleEndian64(long value) {
      WriteRawByte((byte)value);
      WriteRawByte((byte)(value >> 8));
      WriteRawByte((byte)(value >> 16));
      WriteRawByte((byte)(value >> 24));
      WriteRawByte((byte)(value >> 32));
      WriteRawByte((byte)(value >> 40));
      WriteRawByte((byte)(value >> 48));
      WriteRawByte((byte)(value >> 56));
    }

    public void WriteRawByte(byte value) {
      if (position == limit) {
        RefreshBuffer();
      }

      buffer[position++] = value;
    }

    public void WriteRawByte(uint value) {
      WriteRawByte((byte)value);
    }

    /// <summary>
    /// Writes out an array of bytes.
    /// </summary>
    public void WriteRawBytes(byte[] value) {
      WriteRawBytes(value, 0, value.Length);
    }

    /// <summary>
    /// Writes out part of an array of bytes.
    /// </summary>
    public void WriteRawBytes(byte[] value, int offset, int length) {
      if (limit - position >= length) {
        Array.Copy(value, offset, buffer, position, length);
        // We have room in the current buffer.
        position += length;
      } else {
        // Write extends past current buffer.  Fill the rest of this buffer and
        // flush.
        int bytesWritten = limit - position;
        Array.Copy(value, offset, buffer, position, bytesWritten);
        offset += bytesWritten;
        length -= bytesWritten;
        position = limit;
        RefreshBuffer();

        // Now deal with the rest.
        // Since we have an output stream, this is our buffer
        // and buffer offset == 0
        if (length <= limit) {
          // Fits in new buffer.
          Array.Copy(value, offset, buffer, 0, length);
          position = length;
        } else {
          // Write is very big.  Let's do it all at once.
          output.Write(value, offset, length);
        }
      }
    }
    #endregion

    #region Size computations

    const int LittleEndian64Size = 8;
    const int LittleEndian32Size = 4;

    /// <summary>
    /// Compute the number of bytes that would be needed to encode a
    /// double field, including the tag.
    /// </summary>
    public static int ComputeDoubleSize(int fieldNumber, double value) {
      return ComputeTagSize(fieldNumber) + LittleEndian64Size;
    }

    /// <summary>
    /// Compute the number of bytes that would be needed to encode a
    /// float field, including the tag.
    /// </summary>
    public static int ComputeFloatSize(int fieldNumber, float value) {
      return ComputeTagSize(fieldNumber) + LittleEndian32Size;
    }

    /// <summary>
    /// Compute the number of bytes that would be needed to encode a
    /// uint64 field, including the tag.
    /// </summary>
    public static int ComputeUInt64Size(int fieldNumber, ulong value) {
      return ComputeTagSize(fieldNumber) + ComputeRawVarint64Size(value);
    }

    /// <summary>
    /// Compute the number of bytes that would be needed to encode an
    /// int64 field, including the tag.
    /// </summary>
    public static int ComputeInt64Size(int fieldNumber, long value) {
      return ComputeTagSize(fieldNumber) + ComputeRawVarint64Size((ulong)value);
    }

    /// <summary>
    /// Compute the number of bytes that would be needed to encode an
    /// int32 field, including the tag.
    /// </summary>
    public static int ComputeInt32Size(int fieldNumber, int value) {
      if (value >= 0) {
        return ComputeTagSize(fieldNumber) + ComputeRawVarint32Size((uint)value);
      } else {
        // Must sign-extend.
        return ComputeTagSize(fieldNumber) + 10;
      }
    }

    /// <summary>
    /// Compute the number of bytes that would be needed to encode a
    /// fixed64 field, including the tag.
    /// </summary>
    public static int ComputeFixed64Size(int fieldNumber, long value) {
      return ComputeTagSize(fieldNumber) + LittleEndian64Size;
    }

    /// <summary>
    /// Compute the number of bytes that would be needed to encode a
    /// fixed32 field, including the tag.
    /// </summary>
    public static int ComputeFixed32Size(int fieldNumber, int value) {
      return ComputeTagSize(fieldNumber) + LittleEndian32Size;
    }

    /// <summary>
    /// Compute the number of bytes that would be needed to encode a
    /// bool field, including the tag.
    /// </summary>
    public static int ComputeBoolSize(int fieldNumber, bool value) {
      return ComputeTagSize(fieldNumber) + 1;
    }

    /// <summary>
    /// Compute the number of bytes that would be needed to encode a
    /// string field, including the tag.
    /// </summary>
    public static int ComputeStringSize(int fieldNumber, String value) {
      int byteArraySize = Encoding.UTF8.GetByteCount(value);
      return ComputeTagSize(fieldNumber) +
             ComputeRawVarint32Size((uint)byteArraySize) +
             byteArraySize;
    }

    /// <summary>
    /// Compute the number of bytes that would be needed to encode a
    /// group field, including the tag.
    /// </summary>
    public static int ComputeGroupSize(int fieldNumber, IMessage value) {
      return ComputeTagSize(fieldNumber) * 2 + value.SerializedSize;
    }

    /// <summary>
    /// Compute the number of bytes that would be needed to encode a
    /// group field represented by an UnknownFieldSet, including the tag.
    /// </summary>
    public static int ComputeUnknownGroupSize(int fieldNumber,
                                              UnknownFieldSet value) {
      return ComputeTagSize(fieldNumber) * 2 + value.SerializedSize;
    }

    /// <summary>
    /// Compute the number of bytes that would be needed to encode an
    /// embedded message field, including the tag.
    /// </summary>
    public static int ComputeMessageSize(int fieldNumber, IMessage value) {
      int size = value.SerializedSize;
      return ComputeTagSize(fieldNumber) + ComputeRawVarint32Size((uint)size) + size;
    }

    /// <summary>
    /// Compute the number of bytes that would be needed to encode a
    /// bytes field, including the tag.
    /// </summary>
    public static int ComputeBytesSize(int fieldNumber, ByteString value) {
      return ComputeTagSize(fieldNumber) +
             ComputeRawVarint32Size((uint)value.Length) +
             value.Length;
    }

    /// <summary>
    /// Compute the number of bytes that would be needed to encode a
    /// uint32 field, including the tag.
    /// </summary>
    public static int ComputeUInt32Size(int fieldNumber, uint value) {
      return ComputeTagSize(fieldNumber) + ComputeRawVarint32Size(value);
    }

    /// <summary>
    /// Compute the number of bytes that would be needed to encode a
    /// enum field, including the tag. The caller is responsible for
    /// converting the enum value to its numeric value.
    /// </summary>
    public static int ComputeEnumSize(int fieldNumber, int value) {
      return ComputeTagSize(fieldNumber) + ComputeRawVarint32Size((uint)value);
    }

    /// <summary>
    /// Compute the number of bytes that would be needed to encode an
    /// sfixed32 field, including the tag.
    /// </summary>
    public static int ComputeSFixed32Size(int fieldNumber, int value) {
      return ComputeTagSize(fieldNumber) + LittleEndian32Size;
    }

    /// <summary>
    /// Compute the number of bytes that would be needed to encode an
    /// sfixed64 field, including the tag.
    /// </summary>
    public static int ComputeSFixed64Size(int fieldNumber, long value) {
      return ComputeTagSize(fieldNumber) + LittleEndian64Size;
    }

    /// <summary>
    /// Compute the number of bytes that would be needed to encode an
    /// sint32 field, including the tag.
    /// </summary>
    public static int ComputeSInt32Size(int fieldNumber, int value) {
      return ComputeTagSize(fieldNumber) +
             ComputeRawVarint32Size(EncodeZigZag32(value));
    }

    /// <summary>
    /// Compute the number of bytes that would be needed to encode an
    /// sint64 field, including the tag.
    /// </summary>
    public static int ComputeSInt64Size(int fieldNumber, long value) {
      return ComputeTagSize(fieldNumber) +
             ComputeRawVarint64Size(EncodeZigZag64(value));
    }

    /*
     * Compute the number of bytes that would be needed to encode a
     * MessageSet extension to the stream.  For historical reasons,
     * the wire format differs from normal fields.
     */
    /// <summary>
    /// Compute the number of bytes that would be needed to encode a
    /// MessageSet extension to the stream. For historical reasons,
    /// the wire format differs from normal fields.
    /// </summary>
    public static int ComputeMessageSetExtensionSize(int fieldNumber, IMessage value) {
      return ComputeTagSize(WireFormat.MessageSetField.Item) * 2 +
             ComputeUInt32Size(WireFormat.MessageSetField.TypeID, (uint) fieldNumber) +
             ComputeMessageSize(WireFormat.MessageSetField.Message, value);
    }

    /// <summary>
    /// Compute the number of bytes that would be needed to encode an
    /// unparsed MessageSet extension field to the stream. For
    /// historical reasons, the wire format differs from normal fields.
    /// </summary>
    public static int ComputeRawMessageSetExtensionSize(int fieldNumber, ByteString value) {
      return ComputeTagSize(WireFormat.MessageSetField.Item) * 2 +
             ComputeUInt32Size(WireFormat.MessageSetField.TypeID, (uint) fieldNumber) +
             ComputeBytesSize(WireFormat.MessageSetField.Message, value);
    }

    /// <summary>
    /// Compute the number of bytes that would be needed to encode a varint.
    /// </summary>
    public static int ComputeRawVarint32Size(uint value) {
      if ((value & (0xffffffff << 7)) == 0) return 1;
      if ((value & (0xffffffff << 14)) == 0) return 2;
      if ((value & (0xffffffff << 21)) == 0) return 3;
      if ((value & (0xffffffff << 28)) == 0) return 4;
      return 5;
    }

    /// <summary>
    /// Compute the number of bytes that would be needed to encode a varint.
    /// </summary>
    public static int ComputeRawVarint64Size(ulong value) {
      if ((value & (0xffffffffffffffffL << 7)) == 0) return 1;
      if ((value & (0xffffffffffffffffL << 14)) == 0) return 2;
      if ((value & (0xffffffffffffffffL << 21)) == 0) return 3;
      if ((value & (0xffffffffffffffffL << 28)) == 0) return 4;
      if ((value & (0xffffffffffffffffL << 35)) == 0) return 5;
      if ((value & (0xffffffffffffffffL << 42)) == 0) return 6;
      if ((value & (0xffffffffffffffffL << 49)) == 0) return 7;
      if ((value & (0xffffffffffffffffL << 56)) == 0) return 8;
      if ((value & (0xffffffffffffffffL << 63)) == 0) return 9;
      return 10;
    }


    /*
     * Compute the number of bytes that would be needed to encode a
     * field of arbitrary type, including tag, to the stream.
     *
     * @param type   The field's type.
     * @param number The field's number.
     * @param value  Object representing the field's value.  Must be of the exact
     *               type which would be returned by
     *               {@link Message#getField(Descriptors.FieldDescriptor)} for
     *               this field.
     */
    public static int ComputeFieldSize(Descriptors.FieldDescriptor.Type fieldType, int fieldNumber, Object value) {
      switch (fieldType) {
        case Descriptors.FieldDescriptor.Type.Double: return ComputeDoubleSize(fieldNumber, (double)value);
        case Descriptors.FieldDescriptor.Type.Float: return ComputeFloatSize(fieldNumber, (float)value);
        case Descriptors.FieldDescriptor.Type.Int64: return ComputeInt64Size(fieldNumber, (long)value);
        case Descriptors.FieldDescriptor.Type.UInt64: return ComputeUInt64Size(fieldNumber, (ulong)value);
        case Descriptors.FieldDescriptor.Type.Int32: return ComputeInt32Size(fieldNumber, (int)value);
        case Descriptors.FieldDescriptor.Type.Fixed64: return ComputeFixed64Size(fieldNumber, (long)value);
        case Descriptors.FieldDescriptor.Type.Fixed32: return ComputeFixed32Size(fieldNumber, (int)value);
        case Descriptors.FieldDescriptor.Type.Bool: return ComputeBoolSize(fieldNumber, (bool)value);
        case Descriptors.FieldDescriptor.Type.String: return ComputeStringSize(fieldNumber, (string)value);
        case Descriptors.FieldDescriptor.Type.Group: return ComputeGroupSize(fieldNumber, (IMessage)value);
        case Descriptors.FieldDescriptor.Type.Message: return ComputeMessageSize(fieldNumber, (IMessage)value);
        case Descriptors.FieldDescriptor.Type.Bytes: return ComputeBytesSize(fieldNumber, (ByteString)value);
        case Descriptors.FieldDescriptor.Type.UInt32: return ComputeUInt32Size(fieldNumber, (uint)value);
        case Descriptors.FieldDescriptor.Type.SFixed32: return ComputeSFixed32Size(fieldNumber, (int)value);
        case Descriptors.FieldDescriptor.Type.SFixed64: return ComputeSFixed64Size(fieldNumber, (long)value);
        case Descriptors.FieldDescriptor.Type.SInt32: return ComputeSInt32Size(fieldNumber, (int)value);
        case Descriptors.FieldDescriptor.Type.SInt64: return ComputeSInt64Size(fieldNumber, (long)value);
        case Descriptors.FieldDescriptor.Type.Enum: return ComputeEnumSize(fieldNumber, ((Descriptors.EnumValueDescriptor)value).Number);
        default:
          throw new ArgumentOutOfRangeException("Invalid field type " + fieldType);
      }
    }

    /// <summary>
    /// Compute the number of bytes that would be needed to encode a tag.
    /// </summary>
    public static int ComputeTagSize(int fieldNumber) {
      return ComputeRawVarint32Size(WireFormat.MakeTag(fieldNumber, 0));
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
    public static uint EncodeZigZag32(int n) {
      // Note:  the right-shift must be arithmetic
      return (uint)((n << 1) ^ (n >> 31));
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
    public static ulong EncodeZigZag64(long n) {
      return (ulong)((n << 1) ^ (n >> 63));
    }

    private void RefreshBuffer() {
      if (output == null) {
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
    public class OutOfSpaceException : IOException {
      internal OutOfSpaceException()
        : base("CodedOutputStream was writing to a flat byte array and ran out of space.") {
      }
    }

    public void Flush() {
      if (output != null) {
        RefreshBuffer();
      }
    }
  }
}