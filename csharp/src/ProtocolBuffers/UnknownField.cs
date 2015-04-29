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
using System.Collections.ObjectModel;
using Google.ProtocolBuffers.Collections;

namespace Google.ProtocolBuffers
{
    /// <summary>
    /// Represents a single field in an UnknownFieldSet.
    /// 
    /// An UnknownField consists of five lists of values. The lists correspond
    /// to the five "wire types" used in the protocol buffer binary format.
    /// The wire type of each field can be determined from the encoded form alone,
    /// without knowing the field's declared type. So, we are able to parse
    /// unknown values at least this far and separate them. Normally, only one
    /// of the five lists will contain any values, since it is impossible to
    /// define a valid message type that declares two different types for the
    /// same field number. However, the code is designed to allow for the case
    /// where the same unknown field number is encountered using multiple different
    /// wire types.
    /// 
    /// UnknownField is an immutable class. To construct one, you must use an
    /// UnknownField.Builder.
    /// </summary>
    public sealed class UnknownField
    {
        public const string UnknownFieldName = "unknown_field";
        private static readonly UnknownField defaultInstance = CreateBuilder().Build();
        private readonly ReadOnlyCollection<ulong> varintList;
        private readonly ReadOnlyCollection<uint> fixed32List;
        private readonly ReadOnlyCollection<ulong> fixed64List;
        private readonly ReadOnlyCollection<ByteString> lengthDelimitedList;
        private readonly ReadOnlyCollection<UnknownFieldSet> groupList;

        private UnknownField(ReadOnlyCollection<ulong> varintList,
                             ReadOnlyCollection<uint> fixed32List,
                             ReadOnlyCollection<ulong> fixed64List,
                             ReadOnlyCollection<ByteString> lengthDelimitedList,
                             ReadOnlyCollection<UnknownFieldSet> groupList)
        {
            this.varintList = varintList;
            this.fixed32List = fixed32List;
            this.fixed64List = fixed64List;
            this.lengthDelimitedList = lengthDelimitedList;
            this.groupList = groupList;
        }

        public static UnknownField DefaultInstance
        {
            get { return defaultInstance; }
        }

        /// <summary>
        /// The list of varint values for this field.
        /// </summary>
        public IList<ulong> VarintList
        {
            get { return varintList; }
        }

        /// <summary>
        /// The list of fixed32 values for this field.
        /// </summary>
        public IList<uint> Fixed32List
        {
            get { return fixed32List; }
        }

        /// <summary>
        /// The list of fixed64 values for this field.
        /// </summary>
        public IList<ulong> Fixed64List
        {
            get { return fixed64List; }
        }

        /// <summary>
        /// The list of length-delimited values for this field.
        /// </summary>
        public IList<ByteString> LengthDelimitedList
        {
            get { return lengthDelimitedList; }
        }

        /// <summary>
        /// The list of embedded group values for this field. These
        /// are represented using UnknownFieldSets rather than Messages
        /// since the group's type is presumably unknown.
        /// </summary>
        public IList<UnknownFieldSet> GroupList
        {
            get { return groupList; }
        }

        public override bool Equals(object other)
        {
            if (ReferenceEquals(this, other))
            {
                return true;
            }
            UnknownField otherField = other as UnknownField;
            return otherField != null
                   && Lists.Equals(varintList, otherField.varintList)
                   && Lists.Equals(fixed32List, otherField.fixed32List)
                   && Lists.Equals(fixed64List, otherField.fixed64List)
                   && Lists.Equals(lengthDelimitedList, otherField.lengthDelimitedList)
                   && Lists.Equals(groupList, otherField.groupList);
        }

        public override int GetHashCode()
        {
            int hash = 43;
            hash = hash*47 + Lists.GetHashCode(varintList);
            hash = hash*47 + Lists.GetHashCode(fixed32List);
            hash = hash*47 + Lists.GetHashCode(fixed64List);
            hash = hash*47 + Lists.GetHashCode(lengthDelimitedList);
            hash = hash*47 + Lists.GetHashCode(groupList);
            return hash;
        }

        /// <summary>
        /// Constructs a new Builder.
        /// </summary>
        public static Builder CreateBuilder()
        {
            return new Builder();
        }

        /// <summary>
        /// Constructs a new Builder and initializes it to a copy of <paramref name="copyFrom"/>.
        /// </summary>
        public static Builder CreateBuilder(UnknownField copyFrom)
        {
            return new Builder().MergeFrom(copyFrom);
        }

        /// <summary>
        /// Serializes the field, including the field number, and writes it to
        /// <paramref name="output"/>.
        /// </summary>
        public void WriteTo(int fieldNumber, ICodedOutputStream output)
        {
            foreach (ulong value in varintList)
            {
                output.WriteUnknownField(fieldNumber, WireFormat.WireType.Varint, value);
            }
            foreach (uint value in fixed32List)
            {
                output.WriteUnknownField(fieldNumber, WireFormat.WireType.Fixed32, value);
            }
            foreach (ulong value in fixed64List)
            {
                output.WriteUnknownField(fieldNumber, WireFormat.WireType.Fixed64, value);
            }
            foreach (ByteString value in lengthDelimitedList)
            {
                output.WriteUnknownBytes(fieldNumber, value);
            }
            foreach (UnknownFieldSet value in groupList)
            {
#pragma warning disable 0612
                output.WriteUnknownGroup(fieldNumber, value);
#pragma warning restore 0612
            }
        }

        /// <summary>
        /// Computes the number of bytes required to encode this field, including field
        /// number.
        /// </summary>
        public int GetSerializedSize(int fieldNumber)
        {
            int result = 0;
            foreach (ulong value in varintList)
            {
                result += CodedOutputStream.ComputeUInt64Size(fieldNumber, value);
            }
            foreach (uint value in fixed32List)
            {
                result += CodedOutputStream.ComputeFixed32Size(fieldNumber, value);
            }
            foreach (ulong value in fixed64List)
            {
                result += CodedOutputStream.ComputeFixed64Size(fieldNumber, value);
            }
            foreach (ByteString value in lengthDelimitedList)
            {
                result += CodedOutputStream.ComputeBytesSize(fieldNumber, value);
            }
            foreach (UnknownFieldSet value in groupList)
            {
#pragma warning disable 0612
                result += CodedOutputStream.ComputeUnknownGroupSize(fieldNumber, value);
#pragma warning restore 0612
            }
            return result;
        }

        /// <summary>
        /// Serializes the length-delimited values of the field, including field
        /// number, and writes them to <paramref name="output"/> using the MessageSet wire format.
        /// </summary>
        /// <param name="fieldNumber"></param>
        /// <param name="output"></param>
        public void WriteAsMessageSetExtensionTo(int fieldNumber, ICodedOutputStream output)
        {
            foreach (ByteString value in lengthDelimitedList)
            {
                output.WriteMessageSetExtension(fieldNumber, UnknownFieldName, value);
            }
        }

        /// <summary>
        /// Get the number of bytes required to encode this field, incuding field number,
        /// using the MessageSet wire format.
        /// </summary>
        public int GetSerializedSizeAsMessageSetExtension(int fieldNumber)
        {
            int result = 0;
            foreach (ByteString value in lengthDelimitedList)
            {
                result += CodedOutputStream.ComputeRawMessageSetExtensionSize(fieldNumber, value);
            }
            return result;
        }

        /// <summary>
        /// Used to build instances of UnknownField.
        /// </summary>
        public sealed class Builder
        {
            private List<ulong> varintList;
            private List<uint> fixed32List;
            private List<ulong> fixed64List;
            private List<ByteString> lengthDelimitedList;
            private List<UnknownFieldSet> groupList;

            /// <summary>
            /// Builds the field. After building, the builder is reset to an empty
            /// state. (This is actually easier than making it unusable.)
            /// </summary>
            public UnknownField Build()
            {
                return new UnknownField(MakeReadOnly(ref varintList),
                                        MakeReadOnly(ref fixed32List),
                                        MakeReadOnly(ref fixed64List),
                                        MakeReadOnly(ref lengthDelimitedList),
                                        MakeReadOnly(ref groupList));
            }

            /// <summary>
            /// Merge the values in <paramref name="other" /> into this field.  For each list
            /// of values, <paramref name="other"/>'s values are append to the ones in this
            /// field.
            /// </summary>
            public Builder MergeFrom(UnknownField other)
            {
                varintList = AddAll(varintList, other.VarintList);
                fixed32List = AddAll(fixed32List, other.Fixed32List);
                fixed64List = AddAll(fixed64List, other.Fixed64List);
                lengthDelimitedList = AddAll(lengthDelimitedList, other.LengthDelimitedList);
                groupList = AddAll(groupList, other.GroupList);
                return this;
            }

            /// <summary>
            /// Returns a new list containing all of the given specified values from
            /// both the <paramref name="current"/> and <paramref name="extras"/> lists.
            /// If <paramref name="current" /> is null and <paramref name="extras"/> is empty,
            /// null is returned. Otherwise, either a new list is created (if <paramref name="current" />
            /// is null) or the elements of <paramref name="extras"/> are added to <paramref name="current" />.
            /// </summary>
            private static List<T> AddAll<T>(List<T> current, IList<T> extras)
            {
                if (extras.Count == 0)
                {
                    return current;
                }
                if (current == null)
                {
                    current = new List<T>(extras);
                }
                else
                {
                    current.AddRange(extras);
                }
                return current;
            }

            /// <summary>
            /// Clears the contents of this builder.
            /// </summary>
            public Builder Clear()
            {
                varintList = null;
                fixed32List = null;
                fixed64List = null;
                lengthDelimitedList = null;
                groupList = null;
                return this;
            }

            /// <summary>
            /// Adds a varint value.
            /// </summary>
            public Builder AddVarint(ulong value)
            {
                varintList = Add(varintList, value);
                return this;
            }

            /// <summary>
            /// Adds a fixed32 value.
            /// </summary>
            public Builder AddFixed32(uint value)
            {
                fixed32List = Add(fixed32List, value);
                return this;
            }

            /// <summary>
            /// Adds a fixed64 value.
            /// </summary>
            public Builder AddFixed64(ulong value)
            {
                fixed64List = Add(fixed64List, value);
                return this;
            }

            /// <summary>
            /// Adds a length-delimited value.
            /// </summary>
            public Builder AddLengthDelimited(ByteString value)
            {
                lengthDelimitedList = Add(lengthDelimitedList, value);
                return this;
            }

            /// <summary>
            /// Adds an embedded group.
            /// </summary>
            /// <param name="value"></param>
            /// <returns></returns>
            public Builder AddGroup(UnknownFieldSet value)
            {
                groupList = Add(groupList, value);
                return this;
            }

            /// <summary>
            /// Adds <paramref name="value"/> to the <paramref name="list"/>, creating
            /// a new list if <paramref name="list"/> is null. The list is returned - either
            /// the original reference or the new list.
            /// </summary>
            private static List<T> Add<T>(List<T> list, T value)
            {
                if (list == null)
                {
                    list = new List<T>();
                }
                list.Add(value);
                return list;
            }

            /// <summary>
            /// Returns a read-only version of the given IList, and clears
            /// the field used for <paramref name="list"/>. If the value
            /// is null, an empty list is produced using Lists.Empty.
            /// </summary>
            /// <returns></returns>
            private static ReadOnlyCollection<T> MakeReadOnly<T>(ref List<T> list)
            {
                ReadOnlyCollection<T> ret = list == null ? Lists<T>.Empty : new ReadOnlyCollection<T>(list);
                list = null;
                return ret;
            }
        }
    }
}