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
using System.IO;
using System.Text;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers
{
    /// <summary>
    /// Readings and decodes protocol message fields.
    /// </summary>
    /// <remarks>
    /// This class contains two kinds of methods:  methods that read specific
    /// protocol message constructs and field types (e.g. ReadTag and
    /// ReadInt32) and methods that read low-level values (e.g.
    /// ReadRawVarint32 and ReadRawBytes).  If you are reading encoded protocol
    /// messages, you should use the former methods, but if you are reading some
    /// other format of your own design, use the latter. The names of the former
    /// methods are taken from the protocol buffer type names, not .NET types.
    /// (Hence ReadFloat instead of ReadSingle, and ReadBool instead of ReadBoolean.)
    /// 
    /// TODO(jonskeet): Consider whether recursion and size limits shouldn't be readonly,
    /// set at construction time.
    /// </remarks>
    public sealed class CodedInputStream : ICodedInputStream
    {
        private readonly byte[] buffer;
        private int bufferSize;
        private int bufferSizeAfterLimit = 0;
        private int bufferPos = 0;
        private readonly Stream input;
        private uint lastTag = 0;

        private uint nextTag = 0;
        private bool hasNextTag = false;

        internal const int DefaultRecursionLimit = 64;
        internal const int DefaultSizeLimit = 64 << 20; // 64MB
        public const int BufferSize = 4096;

        /// <summary>
        /// The total number of bytes read before the current buffer. The
        /// total bytes read up to the current position can be computed as
        /// totalBytesRetired + bufferPos.
        /// </summary>
        private int totalBytesRetired = 0;

        /// <summary>
        /// The absolute position of the end of the current message.
        /// </summary> 
        private int currentLimit = int.MaxValue;

        /// <summary>
        /// <see cref="SetRecursionLimit"/>
        /// </summary>
        private int recursionDepth = 0;

        private int recursionLimit = DefaultRecursionLimit;

        /// <summary>
        /// <see cref="SetSizeLimit"/>
        /// </summary>
        private int sizeLimit = DefaultSizeLimit;

        #region Construction

        /// <summary>
        /// Creates a new CodedInputStream reading data from the given
        /// stream.
        /// </summary>
        public static CodedInputStream CreateInstance(Stream input)
        {
            return new CodedInputStream(input);
        }

        /// <summary>
        /// Creates a new CodedInputStream reading data from the given
        /// byte array.
        /// </summary>
        public static CodedInputStream CreateInstance(byte[] buf)
        {
            return new CodedInputStream(buf, 0, buf.Length);
        }

        /// <summary>
        /// Creates a new CodedInputStream that reads from the given
        /// byte array slice.
        /// </summary>
        public static CodedInputStream CreateInstance(byte[] buf, int offset, int length)
        {
            return new CodedInputStream(buf, offset, length);
        }

        private CodedInputStream(byte[] buffer, int offset, int length)
        {
            this.buffer = buffer;
            this.bufferPos = offset;
            this.bufferSize = offset + length;
            this.input = null;
        }

        private CodedInputStream(Stream input)
        {
            this.buffer = new byte[BufferSize];
            this.bufferSize = 0;
            this.input = input;
        }

        #endregion

        void ICodedInputStream.ReadMessageStart() { }
        void ICodedInputStream.ReadMessageEnd() { }

        #region Validation

        /// <summary>
        /// Verifies that the last call to ReadTag() returned the given tag value.
        /// This is used to verify that a nested group ended with the correct
        /// end tag.
        /// </summary>
        /// <exception cref="InvalidProtocolBufferException">The last
        /// tag read was not the one specified</exception>
        [CLSCompliant(false)]
        public void CheckLastTagWas(uint value)
        {
            if (lastTag != value)
            {
                throw InvalidProtocolBufferException.InvalidEndTag();
            }
        }

        #endregion

        #region Reading of tags etc

        /// <summary>
        /// Attempt to peek at the next field tag.
        /// </summary>
        [CLSCompliant(false)]
        public bool PeekNextTag(out uint fieldTag, out string fieldName)
        {
            if (hasNextTag)
            {
                fieldName = null;
                fieldTag = nextTag;
                return true;
            }

            uint savedLast = lastTag;
            hasNextTag = ReadTag(out nextTag, out fieldName);
            lastTag = savedLast;
            fieldTag = nextTag;
            return hasNextTag;
        }

        /// <summary>
        /// Attempt to read a field tag, returning false if we have reached the end
        /// of the input data.
        /// </summary>
        /// <param name="fieldTag">The 'tag' of the field (id * 8 + wire-format)</param>
        /// <param name="fieldName">Not Supported - For protobuffer streams, this parameter is always null</param>
        /// <returns>true if the next fieldTag was read</returns>
        [CLSCompliant(false)]
        public bool ReadTag(out uint fieldTag, out string fieldName)
        {
            fieldName = null;

            if (hasNextTag)
            {
                fieldTag = nextTag;
                lastTag = fieldTag;
                hasNextTag = false;
                return true;
            }

            if (IsAtEnd)
            {
                fieldTag = 0;
                lastTag = fieldTag;
                return false;
            }

            fieldTag = ReadRawVarint32();
            lastTag = fieldTag;
            if (lastTag == 0)
            {
                // If we actually read zero, that's not a valid tag.
                throw InvalidProtocolBufferException.InvalidTag();
            }
            return true;
        }

        /// <summary>
        /// Read a double field from the stream.
        /// </summary>
        public bool ReadDouble(ref double value)
        {
#if SILVERLIGHT || COMPACT_FRAMEWORK_35
            if (BitConverter.IsLittleEndian && 8 <= bufferSize - bufferPos)
            {
                value = BitConverter.ToDouble(buffer, bufferPos);
                bufferPos += 8;
            }
            else
            {
                byte[] rawBytes = ReadRawBytes(8);
                if (!BitConverter.IsLittleEndian) 
                    ByteArray.Reverse(rawBytes);
                value = BitConverter.ToDouble(rawBytes, 0);
            }
#else
            value = BitConverter.Int64BitsToDouble((long) ReadRawLittleEndian64());
#endif
            return true;
        }

        /// <summary>
        /// Read a float field from the stream.
        /// </summary>
        public bool ReadFloat(ref float value)
        {
            if (BitConverter.IsLittleEndian && 4 <= bufferSize - bufferPos)
            {
                value = BitConverter.ToSingle(buffer, bufferPos);
                bufferPos += 4;
            }
            else
            {
                byte[] rawBytes = ReadRawBytes(4);
                if (!BitConverter.IsLittleEndian)
                {
                    ByteArray.Reverse(rawBytes);
                }
                value = BitConverter.ToSingle(rawBytes, 0);
            }
            return true;
        }

        /// <summary>
        /// Read a uint64 field from the stream.
        /// </summary>
        [CLSCompliant(false)]
        public bool ReadUInt64(ref ulong value)
        {
            value = ReadRawVarint64();
            return true;
        }

        /// <summary>
        /// Read an int64 field from the stream.
        /// </summary>
        public bool ReadInt64(ref long value)
        {
            value = (long) ReadRawVarint64();
            return true;
        }

        /// <summary>
        /// Read an int32 field from the stream.
        /// </summary>
        public bool ReadInt32(ref int value)
        {
            value = (int) ReadRawVarint32();
            return true;
        }

        /// <summary>
        /// Read a fixed64 field from the stream.
        /// </summary>
        [CLSCompliant(false)]
        public bool ReadFixed64(ref ulong value)
        {
            value = ReadRawLittleEndian64();
            return true;
        }

        /// <summary>
        /// Read a fixed32 field from the stream.
        /// </summary>
        [CLSCompliant(false)]
        public bool ReadFixed32(ref uint value)
        {
            value = ReadRawLittleEndian32();
            return true;
        }

        /// <summary>
        /// Read a bool field from the stream.
        /// </summary>
        public bool ReadBool(ref bool value)
        {
            value = ReadRawVarint32() != 0;
            return true;
        }

        /// <summary>
        /// Reads a string field from the stream.
        /// </summary>
        public bool ReadString(ref string value)
        {
            int size = (int) ReadRawVarint32();
            // No need to read any data for an empty string.
            if (size == 0)
            {
                value = "";
                return true;
            }
            if (size <= bufferSize - bufferPos)
            {
                // Fast path:  We already have the bytes in a contiguous buffer, so
                //   just copy directly from it.
                String result = Encoding.UTF8.GetString(buffer, bufferPos, size);
                bufferPos += size;
                value = result;
                return true;
            }
            // Slow path: Build a byte array first then copy it.
            value = Encoding.UTF8.GetString(ReadRawBytes(size), 0, size);
            return true;
        }

        /// <summary>
        /// Reads a group field value from the stream.
        /// </summary>    
        public void ReadGroup(int fieldNumber, IBuilderLite builder,
                              ExtensionRegistry extensionRegistry)
        {
            if (recursionDepth >= recursionLimit)
            {
                throw InvalidProtocolBufferException.RecursionLimitExceeded();
            }
            ++recursionDepth;
            builder.WeakMergeFrom(this, extensionRegistry);
            CheckLastTagWas(WireFormat.MakeTag(fieldNumber, WireFormat.WireType.EndGroup));
            --recursionDepth;
        }

        /// <summary>
        /// Reads a group field value from the stream and merges it into the given
        /// UnknownFieldSet.
        /// </summary>   
        [Obsolete]
        public void ReadUnknownGroup(int fieldNumber, IBuilderLite builder)
        {
            if (recursionDepth >= recursionLimit)
            {
                throw InvalidProtocolBufferException.RecursionLimitExceeded();
            }
            ++recursionDepth;
            builder.WeakMergeFrom(this);
            CheckLastTagWas(WireFormat.MakeTag(fieldNumber, WireFormat.WireType.EndGroup));
            --recursionDepth;
        }

        /// <summary>
        /// Reads an embedded message field value from the stream.
        /// </summary>   
        public void ReadMessage(IBuilderLite builder, ExtensionRegistry extensionRegistry)
        {
            int length = (int) ReadRawVarint32();
            if (recursionDepth >= recursionLimit)
            {
                throw InvalidProtocolBufferException.RecursionLimitExceeded();
            }
            int oldLimit = PushLimit(length);
            ++recursionDepth;
            builder.WeakMergeFrom(this, extensionRegistry);
            CheckLastTagWas(0);
            --recursionDepth;
            PopLimit(oldLimit);
        }

        /// <summary>
        /// Reads a bytes field value from the stream.
        /// </summary>   
        public bool ReadBytes(ref ByteString value)
        {
            int size = (int) ReadRawVarint32();
            if (size < bufferSize - bufferPos && size > 0)
            {
                // Fast path:  We already have the bytes in a contiguous buffer, so
                //   just copy directly from it.
                ByteString result = ByteString.CopyFrom(buffer, bufferPos, size);
                bufferPos += size;
                value = result;
                return true;
            }
            else
            {
                // Slow path:  Build a byte array and attach it to a new ByteString.
                value = ByteString.AttachBytes(ReadRawBytes(size));
                return true;
            }
        }

        /// <summary>
        /// Reads a uint32 field value from the stream.
        /// </summary>   
        [CLSCompliant(false)]
        public bool ReadUInt32(ref uint value)
        {
            value = ReadRawVarint32();
            return true;
        }

        /// <summary>
        /// Reads an enum field value from the stream. The caller is responsible
        /// for converting the numeric value to an actual enum.
        /// </summary>   
        public bool ReadEnum(ref IEnumLite value, out object unknown, IEnumLiteMap mapping)
        {
            int rawValue = (int) ReadRawVarint32();

            value = mapping.FindValueByNumber(rawValue);
            if (value != null)
            {
                unknown = null;
                return true;
            }
            unknown = rawValue;
            return false;
        }

        /// <summary>
        /// Reads an enum field value from the stream. If the enum is valid for type T,
        /// then the ref value is set and it returns true.  Otherwise the unkown output
        /// value is set and this method returns false.
        /// </summary>   
        [CLSCompliant(false)]
        public bool ReadEnum<T>(ref T value, out object unknown)
            where T : struct, IComparable, IFormattable, IConvertible
        {
            int number = (int) ReadRawVarint32();
            if (Enum.IsDefined(typeof (T), number))
            {
                unknown = null;
                value = (T) (object) number;
                return true;
            }
            unknown = number;
            return false;
        }

        /// <summary>
        /// Reads an sfixed32 field value from the stream.
        /// </summary>   
        public bool ReadSFixed32(ref int value)
        {
            value = (int) ReadRawLittleEndian32();
            return true;
        }

        /// <summary>
        /// Reads an sfixed64 field value from the stream.
        /// </summary>   
        public bool ReadSFixed64(ref long value)
        {
            value = (long) ReadRawLittleEndian64();
            return true;
        }

        /// <summary>
        /// Reads an sint32 field value from the stream.
        /// </summary>   
        public bool ReadSInt32(ref int value)
        {
            value = DecodeZigZag32(ReadRawVarint32());
            return true;
        }

        /// <summary>
        /// Reads an sint64 field value from the stream.
        /// </summary>   
        public bool ReadSInt64(ref long value)
        {
            value = DecodeZigZag64(ReadRawVarint64());
            return true;
        }

        private bool BeginArray(uint fieldTag, out bool isPacked, out int oldLimit)
        {
            isPacked = WireFormat.GetTagWireType(fieldTag) == WireFormat.WireType.LengthDelimited;

            if (isPacked)
            {
                int length = (int) (ReadRawVarint32() & int.MaxValue);
                if (length > 0)
                {
                    oldLimit = PushLimit(length);
                    return true;
                }
                oldLimit = -1;
                return false; //packed but empty
            }

            oldLimit = -1;
            return true;
        }

        /// <summary>
        /// Returns true if the next tag is also part of the same unpacked array.
        /// </summary>
        private bool ContinueArray(uint currentTag)
        {
            string ignore;
            uint next;
            if (PeekNextTag(out next, out ignore))
            {
                if (next == currentTag)
                {
                    hasNextTag = false;
                    return true;
                }
            }
            return false;
        }

        /// <summary>
        /// Returns true if the next tag is also part of the same array, which may or may not be packed.
        /// </summary>
        private bool ContinueArray(uint currentTag, bool packed, int oldLimit)
        {
            if (packed)
            {
                if (ReachedLimit)
                {
                    PopLimit(oldLimit);
                    return false;
                }
                return true;
            }

            string ignore;
            uint next;
            if (PeekNextTag(out next, out ignore))
            {
                if (next == currentTag)
                {
                    hasNextTag = false;
                    return true;
                }
            }
            return false;
        }

        [CLSCompliant(false)]
        public void ReadPrimitiveArray(FieldType fieldType, uint fieldTag, string fieldName, ICollection<object> list)
        {
            WireFormat.WireType normal = WireFormat.GetWireType(fieldType);
            WireFormat.WireType wformat = WireFormat.GetTagWireType(fieldTag);

            // 2.3 allows packed form even if the field is not declared packed.
            if (normal != wformat && wformat == WireFormat.WireType.LengthDelimited)
            {
                int length = (int) (ReadRawVarint32() & int.MaxValue);
                int limit = PushLimit(length);
                while (!ReachedLimit)
                {
                    Object value = null;
                    if (ReadPrimitiveField(fieldType, ref value))
                    {
                        list.Add(value);
                    }
                }
                PopLimit(limit);
            }
            else
            {
                Object value = null;
                do
                {
                    if (ReadPrimitiveField(fieldType, ref value))
                    {
                        list.Add(value);
                    }
                } while (ContinueArray(fieldTag));
            }
        }

        [CLSCompliant(false)]
        public void ReadStringArray(uint fieldTag, string fieldName, ICollection<string> list)
        {
            string tmp = null;
            do
            {
                ReadString(ref tmp);
                list.Add(tmp);
            } while (ContinueArray(fieldTag));
        }

        [CLSCompliant(false)]
        public void ReadBytesArray(uint fieldTag, string fieldName, ICollection<ByteString> list)
        {
            ByteString tmp = null;
            do
            {
                ReadBytes(ref tmp);
                list.Add(tmp);
            } while (ContinueArray(fieldTag));
        }

        [CLSCompliant(false)]
        public void ReadBoolArray(uint fieldTag, string fieldName, ICollection<bool> list)
        {
            bool isPacked;
            int holdLimit;
            if (BeginArray(fieldTag, out isPacked, out holdLimit))
            {
                bool tmp = false;
                do
                {
                    ReadBool(ref tmp);
                    list.Add(tmp);
                } while (ContinueArray(fieldTag, isPacked, holdLimit));
            }
        }

        [CLSCompliant(false)]
        public void ReadInt32Array(uint fieldTag, string fieldName, ICollection<int> list)
        {
            bool isPacked;
            int holdLimit;
            if (BeginArray(fieldTag, out isPacked, out holdLimit))
            {
                int tmp = 0;
                do
                {
                    ReadInt32(ref tmp);
                    list.Add(tmp);
                } while (ContinueArray(fieldTag, isPacked, holdLimit));
            }
        }

        [CLSCompliant(false)]
        public void ReadSInt32Array(uint fieldTag, string fieldName, ICollection<int> list)
        {
            bool isPacked;
            int holdLimit;
            if (BeginArray(fieldTag, out isPacked, out holdLimit))
            {
                int tmp = 0;
                do
                {
                    ReadSInt32(ref tmp);
                    list.Add(tmp);
                } while (ContinueArray(fieldTag, isPacked, holdLimit));
            }
        }

        [CLSCompliant(false)]
        public void ReadUInt32Array(uint fieldTag, string fieldName, ICollection<uint> list)
        {
            bool isPacked;
            int holdLimit;
            if (BeginArray(fieldTag, out isPacked, out holdLimit))
            {
                uint tmp = 0;
                do
                {
                    ReadUInt32(ref tmp);
                    list.Add(tmp);
                } while (ContinueArray(fieldTag, isPacked, holdLimit));
            }
        }

        [CLSCompliant(false)]
        public void ReadFixed32Array(uint fieldTag, string fieldName, ICollection<uint> list)
        {
            bool isPacked;
            int holdLimit;
            if (BeginArray(fieldTag, out isPacked, out holdLimit))
            {
                uint tmp = 0;
                do
                {
                    ReadFixed32(ref tmp);
                    list.Add(tmp);
                } while (ContinueArray(fieldTag, isPacked, holdLimit));
            }
        }

        [CLSCompliant(false)]
        public void ReadSFixed32Array(uint fieldTag, string fieldName, ICollection<int> list)
        {
            bool isPacked;
            int holdLimit;
            if (BeginArray(fieldTag, out isPacked, out holdLimit))
            {
                int tmp = 0;
                do
                {
                    ReadSFixed32(ref tmp);
                    list.Add(tmp);
                } while (ContinueArray(fieldTag, isPacked, holdLimit));
            }
        }

        [CLSCompliant(false)]
        public void ReadInt64Array(uint fieldTag, string fieldName, ICollection<long> list)
        {
            bool isPacked;
            int holdLimit;
            if (BeginArray(fieldTag, out isPacked, out holdLimit))
            {
                long tmp = 0;
                do
                {
                    ReadInt64(ref tmp);
                    list.Add(tmp);
                } while (ContinueArray(fieldTag, isPacked, holdLimit));
            }
        }

        [CLSCompliant(false)]
        public void ReadSInt64Array(uint fieldTag, string fieldName, ICollection<long> list)
        {
            bool isPacked;
            int holdLimit;
            if (BeginArray(fieldTag, out isPacked, out holdLimit))
            {
                long tmp = 0;
                do
                {
                    ReadSInt64(ref tmp);
                    list.Add(tmp);
                } while (ContinueArray(fieldTag, isPacked, holdLimit));
            }
        }

        [CLSCompliant(false)]
        public void ReadUInt64Array(uint fieldTag, string fieldName, ICollection<ulong> list)
        {
            bool isPacked;
            int holdLimit;
            if (BeginArray(fieldTag, out isPacked, out holdLimit))
            {
                ulong tmp = 0;
                do
                {
                    ReadUInt64(ref tmp);
                    list.Add(tmp);
                } while (ContinueArray(fieldTag, isPacked, holdLimit));
            }
        }

        [CLSCompliant(false)]
        public void ReadFixed64Array(uint fieldTag, string fieldName, ICollection<ulong> list)
        {
            bool isPacked;
            int holdLimit;
            if (BeginArray(fieldTag, out isPacked, out holdLimit))
            {
                ulong tmp = 0;
                do
                {
                    ReadFixed64(ref tmp);
                    list.Add(tmp);
                } while (ContinueArray(fieldTag, isPacked, holdLimit));
            }
        }

        [CLSCompliant(false)]
        public void ReadSFixed64Array(uint fieldTag, string fieldName, ICollection<long> list)
        {
            bool isPacked;
            int holdLimit;
            if (BeginArray(fieldTag, out isPacked, out holdLimit))
            {
                long tmp = 0;
                do
                {
                    ReadSFixed64(ref tmp);
                    list.Add(tmp);
                } while (ContinueArray(fieldTag, isPacked, holdLimit));
            }
        }

        [CLSCompliant(false)]
        public void ReadDoubleArray(uint fieldTag, string fieldName, ICollection<double> list)
        {
            bool isPacked;
            int holdLimit;
            if (BeginArray(fieldTag, out isPacked, out holdLimit))
            {
                double tmp = 0;
                do
                {
                    ReadDouble(ref tmp);
                    list.Add(tmp);
                } while (ContinueArray(fieldTag, isPacked, holdLimit));
            }
        }

        [CLSCompliant(false)]
        public void ReadFloatArray(uint fieldTag, string fieldName, ICollection<float> list)
        {
            bool isPacked;
            int holdLimit;
            if (BeginArray(fieldTag, out isPacked, out holdLimit))
            {
                float tmp = 0;
                do
                {
                    ReadFloat(ref tmp);
                    list.Add(tmp);
                } while (ContinueArray(fieldTag, isPacked, holdLimit));
            }
        }

        [CLSCompliant(false)]
        public void ReadEnumArray(uint fieldTag, string fieldName, ICollection<IEnumLite> list,
                                  out ICollection<object> unknown, IEnumLiteMap mapping)
        {
            unknown = null;
            object unkval;
            IEnumLite value = null;
            WireFormat.WireType wformat = WireFormat.GetTagWireType(fieldTag);

            // 2.3 allows packed form even if the field is not declared packed.
            if (wformat == WireFormat.WireType.LengthDelimited)
            {
                int length = (int) (ReadRawVarint32() & int.MaxValue);
                int limit = PushLimit(length);
                while (!ReachedLimit)
                {
                    if (ReadEnum(ref value, out unkval, mapping))
                    {
                        list.Add(value);
                    }
                    else
                    {
                        if (unknown == null)
                        {
                            unknown = new List<object>();
                        }
                        unknown.Add(unkval);
                    }
                }
                PopLimit(limit);
            }
            else
            {
                do
                {
                    if (ReadEnum(ref value, out unkval, mapping))
                    {
                        list.Add(value);
                    }
                    else
                    {
                        if (unknown == null)
                        {
                            unknown = new List<object>();
                        }
                        unknown.Add(unkval);
                    }
                } while (ContinueArray(fieldTag));
            }
        }

        [CLSCompliant(false)]
        public void ReadEnumArray<T>(uint fieldTag, string fieldName, ICollection<T> list,
                                     out ICollection<object> unknown)
            where T : struct, IComparable, IFormattable, IConvertible
        {
            unknown = null;
            object unkval;
            T value = default(T);
            WireFormat.WireType wformat = WireFormat.GetTagWireType(fieldTag);

            // 2.3 allows packed form even if the field is not declared packed.
            if (wformat == WireFormat.WireType.LengthDelimited)
            {
                int length = (int) (ReadRawVarint32() & int.MaxValue);
                int limit = PushLimit(length);
                while (!ReachedLimit)
                {
                    if (ReadEnum<T>(ref value, out unkval))
                    {
                        list.Add(value);
                    }
                    else
                    {
                        if (unknown == null)
                        {
                            unknown = new List<object>();
                        }
                        unknown.Add(unkval);
                    }
                }
                PopLimit(limit);
            }
            else
            {
                do
                {
                    if (ReadEnum(ref value, out unkval))
                    {
                        list.Add(value);
                    }
                    else
                    {
                        if (unknown == null)
                        {
                            unknown = new List<object>();
                        }
                        unknown.Add(unkval);
                    }
                } while (ContinueArray(fieldTag));
            }
        }

        [CLSCompliant(false)]
        public void ReadMessageArray<T>(uint fieldTag, string fieldName, ICollection<T> list, T messageType,
                                        ExtensionRegistry registry) where T : IMessageLite
        {
            do
            {
                IBuilderLite builder = messageType.WeakCreateBuilderForType();
                ReadMessage(builder, registry);
                list.Add((T) builder.WeakBuildPartial());
            } while (ContinueArray(fieldTag));
        }

        [CLSCompliant(false)]
        public void ReadGroupArray<T>(uint fieldTag, string fieldName, ICollection<T> list, T messageType,
                                      ExtensionRegistry registry) where T : IMessageLite
        {
            do
            {
                IBuilderLite builder = messageType.WeakCreateBuilderForType();
                ReadGroup(WireFormat.GetTagFieldNumber(fieldTag), builder, registry);
                list.Add((T) builder.WeakBuildPartial());
            } while (ContinueArray(fieldTag));
        }

        /// <summary>
        /// Reads a field of any primitive type. Enums, groups and embedded
        /// messages are not handled by this method.
        /// </summary>
        public bool ReadPrimitiveField(FieldType fieldType, ref object value)
        {
            switch (fieldType)
            {
                case FieldType.Double:
                    {
                        double tmp = 0;
                        if (ReadDouble(ref tmp))
                        {
                            value = tmp;
                            return true;
                        }
                        return false;
                    }
                case FieldType.Float:
                    {
                        float tmp = 0;
                        if (ReadFloat(ref tmp))
                        {
                            value = tmp;
                            return true;
                        }
                        return false;
                    }
                case FieldType.Int64:
                    {
                        long tmp = 0;
                        if (ReadInt64(ref tmp))
                        {
                            value = tmp;
                            return true;
                        }
                        return false;
                    }
                case FieldType.UInt64:
                    {
                        ulong tmp = 0;
                        if (ReadUInt64(ref tmp))
                        {
                            value = tmp;
                            return true;
                        }
                        return false;
                    }
                case FieldType.Int32:
                    {
                        int tmp = 0;
                        if (ReadInt32(ref tmp))
                        {
                            value = tmp;
                            return true;
                        }
                        return false;
                    }
                case FieldType.Fixed64:
                    {
                        ulong tmp = 0;
                        if (ReadFixed64(ref tmp))
                        {
                            value = tmp;
                            return true;
                        }
                        return false;
                    }
                case FieldType.Fixed32:
                    {
                        uint tmp = 0;
                        if (ReadFixed32(ref tmp))
                        {
                            value = tmp;
                            return true;
                        }
                        return false;
                    }
                case FieldType.Bool:
                    {
                        bool tmp = false;
                        if (ReadBool(ref tmp))
                        {
                            value = tmp;
                            return true;
                        }
                        return false;
                    }
                case FieldType.String:
                    {
                        string tmp = null;
                        if (ReadString(ref tmp))
                        {
                            value = tmp;
                            return true;
                        }
                        return false;
                    }
                case FieldType.Bytes:
                    {
                        ByteString tmp = null;
                        if (ReadBytes(ref tmp))
                        {
                            value = tmp;
                            return true;
                        }
                        return false;
                    }
                case FieldType.UInt32:
                    {
                        uint tmp = 0;
                        if (ReadUInt32(ref tmp))
                        {
                            value = tmp;
                            return true;
                        }
                        return false;
                    }
                case FieldType.SFixed32:
                    {
                        int tmp = 0;
                        if (ReadSFixed32(ref tmp))
                        {
                            value = tmp;
                            return true;
                        }
                        return false;
                    }
                case FieldType.SFixed64:
                    {
                        long tmp = 0;
                        if (ReadSFixed64(ref tmp))
                        {
                            value = tmp;
                            return true;
                        }
                        return false;
                    }
                case FieldType.SInt32:
                    {
                        int tmp = 0;
                        if (ReadSInt32(ref tmp))
                        {
                            value = tmp;
                            return true;
                        }
                        return false;
                    }
                case FieldType.SInt64:
                    {
                        long tmp = 0;
                        if (ReadSInt64(ref tmp))
                        {
                            value = tmp;
                            return true;
                        }
                        return false;
                    }
                case FieldType.Group:
                    throw new ArgumentException("ReadPrimitiveField() cannot handle nested groups.");
                case FieldType.Message:
                    throw new ArgumentException("ReadPrimitiveField() cannot handle embedded messages.");
                    // We don't handle enums because we don't know what to do if the
                    // value is not recognized.
                case FieldType.Enum:
                    throw new ArgumentException("ReadPrimitiveField() cannot handle enums.");
                default:
                    throw new ArgumentOutOfRangeException("Invalid field type " + fieldType);
            }
        }

        #endregion

        #region Underlying reading primitives

        /// <summary>
        /// Same code as ReadRawVarint32, but read each byte individually, checking for
        /// buffer overflow.
        /// </summary>
        private uint SlowReadRawVarint32()
        {
            int tmp = ReadRawByte();
            if (tmp < 128)
            {
                return (uint) tmp;
            }
            int result = tmp & 0x7f;
            if ((tmp = ReadRawByte()) < 128)
            {
                result |= tmp << 7;
            }
            else
            {
                result |= (tmp & 0x7f) << 7;
                if ((tmp = ReadRawByte()) < 128)
                {
                    result |= tmp << 14;
                }
                else
                {
                    result |= (tmp & 0x7f) << 14;
                    if ((tmp = ReadRawByte()) < 128)
                    {
                        result |= tmp << 21;
                    }
                    else
                    {
                        result |= (tmp & 0x7f) << 21;
                        result |= (tmp = ReadRawByte()) << 28;
                        if (tmp >= 128)
                        {
                            // Discard upper 32 bits.
                            for (int i = 0; i < 5; i++)
                            {
                                if (ReadRawByte() < 128)
                                {
                                    return (uint) result;
                                }
                            }
                            throw InvalidProtocolBufferException.MalformedVarint();
                        }
                    }
                }
            }
            return (uint) result;
        }

        /// <summary>
        /// Read a raw Varint from the stream.  If larger than 32 bits, discard the upper bits.
        /// This method is optimised for the case where we've got lots of data in the buffer.
        /// That means we can check the size just once, then just read directly from the buffer
        /// without constant rechecking of the buffer length.
        /// </summary>
        [CLSCompliant(false)]
        public uint ReadRawVarint32()
        {
            if (bufferPos + 5 > bufferSize)
            {
                return SlowReadRawVarint32();
            }

            int tmp = buffer[bufferPos++];
            if (tmp < 128)
            {
                return (uint) tmp;
            }
            int result = tmp & 0x7f;
            if ((tmp = buffer[bufferPos++]) < 128)
            {
                result |= tmp << 7;
            }
            else
            {
                result |= (tmp & 0x7f) << 7;
                if ((tmp = buffer[bufferPos++]) < 128)
                {
                    result |= tmp << 14;
                }
                else
                {
                    result |= (tmp & 0x7f) << 14;
                    if ((tmp = buffer[bufferPos++]) < 128)
                    {
                        result |= tmp << 21;
                    }
                    else
                    {
                        result |= (tmp & 0x7f) << 21;
                        result |= (tmp = buffer[bufferPos++]) << 28;
                        if (tmp >= 128)
                        {
                            // Discard upper 32 bits.
                            // Note that this has to use ReadRawByte() as we only ensure we've
                            // got at least 5 bytes at the start of the method. This lets us
                            // use the fast path in more cases, and we rarely hit this section of code.
                            for (int i = 0; i < 5; i++)
                            {
                                if (ReadRawByte() < 128)
                                {
                                    return (uint) result;
                                }
                            }
                            throw InvalidProtocolBufferException.MalformedVarint();
                        }
                    }
                }
            }
            return (uint) result;
        }

        /// <summary>
        /// Reads a varint from the input one byte at a time, so that it does not
        /// read any bytes after the end of the varint. If you simply wrapped the
        /// stream in a CodedInputStream and used ReadRawVarint32(Stream)}
        /// then you would probably end up reading past the end of the varint since
        /// CodedInputStream buffers its input.
        /// </summary>
        /// <param name="input"></param>
        /// <returns></returns>
        [CLSCompliant(false)]
        public static uint ReadRawVarint32(Stream input)
        {
            int result = 0;
            int offset = 0;
            for (; offset < 32; offset += 7)
            {
                int b = input.ReadByte();
                if (b == -1)
                {
                    throw InvalidProtocolBufferException.TruncatedMessage();
                }
                result |= (b & 0x7f) << offset;
                if ((b & 0x80) == 0)
                {
                    return (uint) result;
                }
            }
            // Keep reading up to 64 bits.
            for (; offset < 64; offset += 7)
            {
                int b = input.ReadByte();
                if (b == -1)
                {
                    throw InvalidProtocolBufferException.TruncatedMessage();
                }
                if ((b & 0x80) == 0)
                {
                    return (uint) result;
                }
            }
            throw InvalidProtocolBufferException.MalformedVarint();
        }

        /// <summary>
        /// Read a raw varint from the stream.
        /// </summary>
        [CLSCompliant(false)]
        public ulong ReadRawVarint64()
        {
            int shift = 0;
            ulong result = 0;
            while (shift < 64)
            {
                byte b = ReadRawByte();
                result |= (ulong) (b & 0x7F) << shift;
                if ((b & 0x80) == 0)
                {
                    return result;
                }
                shift += 7;
            }
            throw InvalidProtocolBufferException.MalformedVarint();
        }

        /// <summary>
        /// Read a 32-bit little-endian integer from the stream.
        /// </summary>
        [CLSCompliant(false)]
        public uint ReadRawLittleEndian32()
        {
            uint b1 = ReadRawByte();
            uint b2 = ReadRawByte();
            uint b3 = ReadRawByte();
            uint b4 = ReadRawByte();
            return b1 | (b2 << 8) | (b3 << 16) | (b4 << 24);
        }

        /// <summary>
        /// Read a 64-bit little-endian integer from the stream.
        /// </summary>
        [CLSCompliant(false)]
        public ulong ReadRawLittleEndian64()
        {
            ulong b1 = ReadRawByte();
            ulong b2 = ReadRawByte();
            ulong b3 = ReadRawByte();
            ulong b4 = ReadRawByte();
            ulong b5 = ReadRawByte();
            ulong b6 = ReadRawByte();
            ulong b7 = ReadRawByte();
            ulong b8 = ReadRawByte();
            return b1 | (b2 << 8) | (b3 << 16) | (b4 << 24)
                   | (b5 << 32) | (b6 << 40) | (b7 << 48) | (b8 << 56);
        }

        #endregion

        /// <summary>
        /// Decode a 32-bit value with ZigZag encoding.
        /// </summary>
        /// <remarks>
        /// ZigZag encodes signed integers into values that can be efficiently
        /// encoded with varint.  (Otherwise, negative values must be 
        /// sign-extended to 64 bits to be varint encoded, thus always taking
        /// 10 bytes on the wire.)
        /// </remarks>
        [CLSCompliant(false)]
        public static int DecodeZigZag32(uint n)
        {
            return (int) (n >> 1) ^ -(int) (n & 1);
        }

        /// <summary>
        /// Decode a 32-bit value with ZigZag encoding.
        /// </summary>
        /// <remarks>
        /// ZigZag encodes signed integers into values that can be efficiently
        /// encoded with varint.  (Otherwise, negative values must be 
        /// sign-extended to 64 bits to be varint encoded, thus always taking
        /// 10 bytes on the wire.)
        /// </remarks>
        [CLSCompliant(false)]
        public static long DecodeZigZag64(ulong n)
        {
            return (long) (n >> 1) ^ -(long) (n & 1);
        }

        /// <summary>
        /// Set the maximum message recursion depth.
        /// </summary>
        /// <remarks>
        /// In order to prevent malicious
        /// messages from causing stack overflows, CodedInputStream limits
        /// how deeply messages may be nested.  The default limit is 64.
        /// </remarks>
        public int SetRecursionLimit(int limit)
        {
            if (limit < 0)
            {
                throw new ArgumentOutOfRangeException("Recursion limit cannot be negative: " + limit);
            }
            int oldLimit = recursionLimit;
            recursionLimit = limit;
            return oldLimit;
        }

        /// <summary>
        /// Set the maximum message size.
        /// </summary>
        /// <remarks>
        /// In order to prevent malicious messages from exhausting memory or
        /// causing integer overflows, CodedInputStream limits how large a message may be.
        /// The default limit is 64MB.  You should set this limit as small
        /// as you can without harming your app's functionality.  Note that
        /// size limits only apply when reading from an InputStream, not
        /// when constructed around a raw byte array (nor with ByteString.NewCodedInput).
        /// If you want to read several messages from a single CodedInputStream, you
        /// can call ResetSizeCounter() after each message to avoid hitting the
        /// size limit.
        /// </remarks>
        public int SetSizeLimit(int limit)
        {
            if (limit < 0)
            {
                throw new ArgumentOutOfRangeException("Size limit cannot be negative: " + limit);
            }
            int oldLimit = sizeLimit;
            sizeLimit = limit;
            return oldLimit;
        }

        #region Internal reading and buffer management

        /// <summary>
        /// Resets the current size counter to zero (see SetSizeLimit).
        /// </summary>
        public void ResetSizeCounter()
        {
            totalBytesRetired = 0;
        }

        /// <summary>
        /// Sets currentLimit to (current position) + byteLimit. This is called
        /// when descending into a length-delimited embedded message. The previous
        /// limit is returned.
        /// </summary>
        /// <returns>The old limit.</returns>
        public int PushLimit(int byteLimit)
        {
            if (byteLimit < 0)
            {
                throw InvalidProtocolBufferException.NegativeSize();
            }
            byteLimit += totalBytesRetired + bufferPos;
            int oldLimit = currentLimit;
            if (byteLimit > oldLimit)
            {
                throw InvalidProtocolBufferException.TruncatedMessage();
            }
            currentLimit = byteLimit;

            RecomputeBufferSizeAfterLimit();

            return oldLimit;
        }

        private void RecomputeBufferSizeAfterLimit()
        {
            bufferSize += bufferSizeAfterLimit;
            int bufferEnd = totalBytesRetired + bufferSize;
            if (bufferEnd > currentLimit)
            {
                // Limit is in current buffer.
                bufferSizeAfterLimit = bufferEnd - currentLimit;
                bufferSize -= bufferSizeAfterLimit;
            }
            else
            {
                bufferSizeAfterLimit = 0;
            }
        }

        /// <summary>
        /// Discards the current limit, returning the previous limit.
        /// </summary>
        public void PopLimit(int oldLimit)
        {
            currentLimit = oldLimit;
            RecomputeBufferSizeAfterLimit();
        }

        /// <summary>
        /// Returns whether or not all the data before the limit has been read.
        /// </summary>
        /// <returns></returns>
        public bool ReachedLimit
        {
            get
            {
                if (currentLimit == int.MaxValue)
                {
                    return false;
                }
                int currentAbsolutePosition = totalBytesRetired + bufferPos;
                return currentAbsolutePosition >= currentLimit;
            }
        }

        /// <summary>
        /// Returns true if the stream has reached the end of the input. This is the
        /// case if either the end of the underlying input source has been reached or
        /// the stream has reached a limit created using PushLimit.
        /// </summary>
        public bool IsAtEnd
        {
            get { return bufferPos == bufferSize && !RefillBuffer(false); }
        }

        /// <summary>
        /// Called when buffer is empty to read more bytes from the
        /// input.  If <paramref name="mustSucceed"/> is true, RefillBuffer() gurantees that
        /// either there will be at least one byte in the buffer when it returns
        /// or it will throw an exception.  If <paramref name="mustSucceed"/> is false,
        /// RefillBuffer() returns false if no more bytes were available.
        /// </summary>
        /// <param name="mustSucceed"></param>
        /// <returns></returns>
        private bool RefillBuffer(bool mustSucceed)
        {
            if (bufferPos < bufferSize)
            {
                throw new InvalidOperationException("RefillBuffer() called when buffer wasn't empty.");
            }

            if (totalBytesRetired + bufferSize == currentLimit)
            {
                // Oops, we hit a limit.
                if (mustSucceed)
                {
                    throw InvalidProtocolBufferException.TruncatedMessage();
                }
                else
                {
                    return false;
                }
            }

            totalBytesRetired += bufferSize;

            bufferPos = 0;
            bufferSize = (input == null) ? 0 : input.Read(buffer, 0, buffer.Length);
            if (bufferSize < 0)
            {
                throw new InvalidOperationException("Stream.Read returned a negative count");
            }
            if (bufferSize == 0)
            {
                if (mustSucceed)
                {
                    throw InvalidProtocolBufferException.TruncatedMessage();
                }
                else
                {
                    return false;
                }
            }
            else
            {
                RecomputeBufferSizeAfterLimit();
                int totalBytesRead =
                    totalBytesRetired + bufferSize + bufferSizeAfterLimit;
                if (totalBytesRead > sizeLimit || totalBytesRead < 0)
                {
                    throw InvalidProtocolBufferException.SizeLimitExceeded();
                }
                return true;
            }
        }

        /// <summary>
        /// Read one byte from the input.
        /// </summary>
        /// <exception cref="InvalidProtocolBufferException">
        /// the end of the stream or the current limit was reached
        /// </exception>
        public byte ReadRawByte()
        {
            if (bufferPos == bufferSize)
            {
                RefillBuffer(true);
            }
            return buffer[bufferPos++];
        }

        /// <summary>
        /// Read a fixed size of bytes from the input.
        /// </summary>
        /// <exception cref="InvalidProtocolBufferException">
        /// the end of the stream or the current limit was reached
        /// </exception>
        public byte[] ReadRawBytes(int size)
        {
            if (size < 0)
            {
                throw InvalidProtocolBufferException.NegativeSize();
            }

            if (totalBytesRetired + bufferPos + size > currentLimit)
            {
                // Read to the end of the stream anyway.
                SkipRawBytes(currentLimit - totalBytesRetired - bufferPos);
                // Then fail.
                throw InvalidProtocolBufferException.TruncatedMessage();
            }

            if (size <= bufferSize - bufferPos)
            {
                // We have all the bytes we need already.
                byte[] bytes = new byte[size];
                ByteArray.Copy(buffer, bufferPos, bytes, 0, size);
                bufferPos += size;
                return bytes;
            }
            else if (size < BufferSize)
            {
                // Reading more bytes than are in the buffer, but not an excessive number
                // of bytes.  We can safely allocate the resulting array ahead of time.

                // First copy what we have.
                byte[] bytes = new byte[size];
                int pos = bufferSize - bufferPos;
                ByteArray.Copy(buffer, bufferPos, bytes, 0, pos);
                bufferPos = bufferSize;

                // We want to use RefillBuffer() and then copy from the buffer into our
                // byte array rather than reading directly into our byte array because
                // the input may be unbuffered.
                RefillBuffer(true);

                while (size - pos > bufferSize)
                {
                    Buffer.BlockCopy(buffer, 0, bytes, pos, bufferSize);
                    pos += bufferSize;
                    bufferPos = bufferSize;
                    RefillBuffer(true);
                }

                ByteArray.Copy(buffer, 0, bytes, pos, size - pos);
                bufferPos = size - pos;

                return bytes;
            }
            else
            {
                // The size is very large.  For security reasons, we can't allocate the
                // entire byte array yet.  The size comes directly from the input, so a
                // maliciously-crafted message could provide a bogus very large size in
                // order to trick the app into allocating a lot of memory.  We avoid this
                // by allocating and reading only a small chunk at a time, so that the
                // malicious message must actually *be* extremely large to cause
                // problems.  Meanwhile, we limit the allowed size of a message elsewhere.

                // Remember the buffer markers since we'll have to copy the bytes out of
                // it later.
                int originalBufferPos = bufferPos;
                int originalBufferSize = bufferSize;

                // Mark the current buffer consumed.
                totalBytesRetired += bufferSize;
                bufferPos = 0;
                bufferSize = 0;

                // Read all the rest of the bytes we need.
                int sizeLeft = size - (originalBufferSize - originalBufferPos);
                List<byte[]> chunks = new List<byte[]>();

                while (sizeLeft > 0)
                {
                    byte[] chunk = new byte[Math.Min(sizeLeft, BufferSize)];
                    int pos = 0;
                    while (pos < chunk.Length)
                    {
                        int n = (input == null) ? -1 : input.Read(chunk, pos, chunk.Length - pos);
                        if (n <= 0)
                        {
                            throw InvalidProtocolBufferException.TruncatedMessage();
                        }
                        totalBytesRetired += n;
                        pos += n;
                    }
                    sizeLeft -= chunk.Length;
                    chunks.Add(chunk);
                }

                // OK, got everything.  Now concatenate it all into one buffer.
                byte[] bytes = new byte[size];

                // Start by copying the leftover bytes from this.buffer.
                int newPos = originalBufferSize - originalBufferPos;
                ByteArray.Copy(buffer, originalBufferPos, bytes, 0, newPos);

                // And now all the chunks.
                foreach (byte[] chunk in chunks)
                {
                    Buffer.BlockCopy(chunk, 0, bytes, newPos, chunk.Length);
                    newPos += chunk.Length;
                }

                // Done.
                return bytes;
            }
        }

        /// <summary>
        /// Reads and discards a single field, given its tag value.
        /// </summary>
        /// <returns>false if the tag is an end-group tag, in which case
        /// nothing is skipped. Otherwise, returns true.</returns>
        [CLSCompliant(false)]
        public bool SkipField()
        {
            uint tag = lastTag;
            switch (WireFormat.GetTagWireType(tag))
            {
                case WireFormat.WireType.Varint:
                    ReadRawVarint64();
                    return true;
                case WireFormat.WireType.Fixed64:
                    ReadRawLittleEndian64();
                    return true;
                case WireFormat.WireType.LengthDelimited:
                    SkipRawBytes((int) ReadRawVarint32());
                    return true;
                case WireFormat.WireType.StartGroup:
                    SkipMessage();
                    CheckLastTagWas(
                        WireFormat.MakeTag(WireFormat.GetTagFieldNumber(tag),
                                           WireFormat.WireType.EndGroup));
                    return true;
                case WireFormat.WireType.EndGroup:
                    return false;
                case WireFormat.WireType.Fixed32:
                    ReadRawLittleEndian32();
                    return true;
                default:
                    throw InvalidProtocolBufferException.InvalidWireType();
            }
        }

        /// <summary>
        /// Reads and discards an entire message.  This will read either until EOF
        /// or until an endgroup tag, whichever comes first.
        /// </summary>
        public void SkipMessage()
        {
            uint tag;
            string name;
            while (ReadTag(out tag, out name))
            {
                if (!SkipField())
                {
                    return;
                }
            }
        }

        /// <summary>
        /// Reads and discards <paramref name="size"/> bytes.
        /// </summary>
        /// <exception cref="InvalidProtocolBufferException">the end of the stream
        /// or the current limit was reached</exception>
        public void SkipRawBytes(int size)
        {
            if (size < 0)
            {
                throw InvalidProtocolBufferException.NegativeSize();
            }

            if (totalBytesRetired + bufferPos + size > currentLimit)
            {
                // Read to the end of the stream anyway.
                SkipRawBytes(currentLimit - totalBytesRetired - bufferPos);
                // Then fail.
                throw InvalidProtocolBufferException.TruncatedMessage();
            }

            if (size <= bufferSize - bufferPos)
            {
                // We have all the bytes we need already.
                bufferPos += size;
            }
            else
            {
                // Skipping more bytes than are in the buffer.  First skip what we have.
                int pos = bufferSize - bufferPos;
                totalBytesRetired += pos;
                bufferPos = 0;
                bufferSize = 0;

                // Then skip directly from the InputStream for the rest.
                if (pos < size)
                {
                    if (input == null)
                    {
                        throw InvalidProtocolBufferException.TruncatedMessage();
                    }
                    SkipImpl(size - pos);
                    totalBytesRetired += size - pos;
                }
            }
        }

        /// <summary>
        /// Abstraction of skipping to cope with streams which can't really skip.
        /// </summary>
        private void SkipImpl(int amountToSkip)
        {
            if (input.CanSeek)
            {
                long previousPosition = input.Position;
                input.Position += amountToSkip;
                if (input.Position != previousPosition + amountToSkip)
                {
                    throw InvalidProtocolBufferException.TruncatedMessage();
                }
            }
            else
            {
                byte[] skipBuffer = new byte[1024];
                while (amountToSkip > 0)
                {
                    int bytesRead = input.Read(skipBuffer, 0, skipBuffer.Length);
                    if (bytesRead <= 0)
                    {
                        throw InvalidProtocolBufferException.TruncatedMessage();
                    }
                    amountToSkip -= bytesRead;
                }
            }
        }

        #endregion
    }
}