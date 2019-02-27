#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2017 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
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
using Google.Protobuf.Collections;

namespace Google.Protobuf
{
    /// <summary>
    /// Represents a single field in an UnknownFieldSet.
    ///
    /// An UnknownField consists of four lists of values. The lists correspond
    /// to the four "wire types" used in the protocol buffer binary format.
    /// Normally, only one of the four lists will contain any values, since it
    /// is impossible to define a valid message type that declares two different
    /// types for the same field number. However, the code is designed to allow
    /// for the case where the same unknown field number is encountered using
    /// multiple different wire types.
    ///
    /// </summary>
    internal sealed class UnknownField
    {
        private List<ulong> varintList;
        private List<uint> fixed32List;
        private List<ulong> fixed64List;
        private List<ByteString> lengthDelimitedList;
        private List<UnknownFieldSet> groupList;

        /// <summary>
        /// Creates a new UnknownField.
        /// </summary>
        public UnknownField()
        {
        }

        /// <summary>
        /// Checks if two unknown field are equal.
        /// </summary>
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

        /// <summary>
        /// Get the hash code of the unknown field.
        /// </summary>
        public override int GetHashCode()
        {
            int hash = 43;
            hash = hash * 47 + Lists.GetHashCode(varintList);
            hash = hash * 47 + Lists.GetHashCode(fixed32List);
            hash = hash * 47 + Lists.GetHashCode(fixed64List);
            hash = hash * 47 + Lists.GetHashCode(lengthDelimitedList);
            hash = hash * 47 + Lists.GetHashCode(groupList);
            return hash;
        }

        /// <summary>
        /// Serializes the field, including the field number, and writes it to
        /// <paramref name="output"/>
        /// </summary>
        /// <param name="fieldNumber">The unknown field number.</param>
        /// <param name="output">The CodedOutputStream to write to.</param>
        internal void WriteTo(int fieldNumber, CodedOutputStream output)
        {
            if (varintList != null)
            {
                foreach (ulong value in varintList)
                {
                    output.WriteTag(fieldNumber, WireFormat.WireType.Varint);
                    output.WriteUInt64(value);
                }
            }
            if (fixed32List != null)
            {
                foreach (uint value in fixed32List)
                {
                    output.WriteTag(fieldNumber, WireFormat.WireType.Fixed32);
                    output.WriteFixed32(value);
                }
            }
            if (fixed64List != null)
            {
                foreach (ulong value in fixed64List)
                {
                    output.WriteTag(fieldNumber, WireFormat.WireType.Fixed64);
                    output.WriteFixed64(value);
                }
            }
            if (lengthDelimitedList != null)
            {
                foreach (ByteString value in lengthDelimitedList)
                {
                    output.WriteTag(fieldNumber, WireFormat.WireType.LengthDelimited);
                    output.WriteBytes(value);
                }
            }
            if (groupList != null)
            {
                foreach (UnknownFieldSet value in groupList)
                {
                    output.WriteTag(fieldNumber, WireFormat.WireType.StartGroup);
                    value.WriteTo(output);
                    output.WriteTag(fieldNumber, WireFormat.WireType.EndGroup);
                }
            }
        }

        /// <summary>
        /// Computes the number of bytes required to encode this field, including field
        /// number.
        /// </summary>
        internal int GetSerializedSize(int fieldNumber)
        {
            int result = 0;
            if (varintList != null)
            {
                result += CodedOutputStream.ComputeTagSize(fieldNumber) * varintList.Count;
                foreach (ulong value in varintList)
                {
                    result += CodedOutputStream.ComputeUInt64Size(value);
                }
            }
            if (fixed32List != null)
            {
                result += CodedOutputStream.ComputeTagSize(fieldNumber) * fixed32List.Count;
                result += CodedOutputStream.ComputeFixed32Size(1) * fixed32List.Count;
            }
            if (fixed64List != null)
            {
                result += CodedOutputStream.ComputeTagSize(fieldNumber) * fixed64List.Count;
                result += CodedOutputStream.ComputeFixed64Size(1) * fixed64List.Count;
            }
            if (lengthDelimitedList != null)
            {
                result += CodedOutputStream.ComputeTagSize(fieldNumber) * lengthDelimitedList.Count;
                foreach (ByteString value in lengthDelimitedList)
                {
                    result += CodedOutputStream.ComputeBytesSize(value);
                }
            }
            if (groupList != null)
            {
                result += CodedOutputStream.ComputeTagSize(fieldNumber) * 2 * groupList.Count;
                foreach (UnknownFieldSet value in groupList)
                {
                    result += value.CalculateSize();
                }
            }
            return result;
        }

        /// <summary>
        /// Merge the values in <paramref name="other" /> into this field.  For each list
        /// of values, <paramref name="other"/>'s values are append to the ones in this
        /// field.
        /// </summary>
        internal UnknownField MergeFrom(UnknownField other)
        {
            varintList = AddAll(varintList, other.varintList);
            fixed32List = AddAll(fixed32List, other.fixed32List);
            fixed64List = AddAll(fixed64List, other.fixed64List);
            lengthDelimitedList = AddAll(lengthDelimitedList, other.lengthDelimitedList);
            groupList = AddAll(groupList, other.groupList);
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
        /// Adds a varint value.
        /// </summary>
        internal UnknownField AddVarint(ulong value)
        {
            varintList = Add(varintList, value);
            return this;
        }

        /// <summary>
        /// Adds a fixed32 value.
        /// </summary>
        internal UnknownField AddFixed32(uint value)
        {
            fixed32List = Add(fixed32List, value);
            return this;
        }

        /// <summary>
        /// Adds a fixed64 value.
        /// </summary>
        internal UnknownField AddFixed64(ulong value)
        {
            fixed64List = Add(fixed64List, value);
            return this;
        }

        /// <summary>
        /// Adds a length-delimited value.
        /// </summary>
        internal UnknownField AddLengthDelimited(ByteString value)
        {
            lengthDelimitedList = Add(lengthDelimitedList, value);
            return this;
        }

        internal UnknownField AddGroup(UnknownFieldSet value)
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
    }
}
