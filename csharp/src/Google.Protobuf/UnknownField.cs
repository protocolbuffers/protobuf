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
        internal List<ulong> VarintList { get; private set; }
        internal List<uint> Fixed32List { get; private set; }
        internal List<ulong> Fixed64List { get; private set; }
        internal List<ByteString> LengthDelimitedList { get; private set; }
        internal List<UnknownFieldSet> GroupList { get; private set; }

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
                   && Lists.Equals(VarintList, otherField.VarintList)
                   && Lists.Equals(Fixed32List, otherField.Fixed32List)
                   && Lists.Equals(Fixed64List, otherField.Fixed64List)
                   && Lists.Equals(LengthDelimitedList, otherField.LengthDelimitedList)
                   && Lists.Equals(GroupList, otherField.GroupList);
        }

        /// <summary>
        /// Get the hash code of the unknown field.
        /// </summary>
        public override int GetHashCode()
        {
            int hash = 43;
            hash = hash * 47 + Lists.GetHashCode(VarintList);
            hash = hash * 47 + Lists.GetHashCode(Fixed32List);
            hash = hash * 47 + Lists.GetHashCode(Fixed64List);
            hash = hash * 47 + Lists.GetHashCode(LengthDelimitedList);
            hash = hash * 47 + Lists.GetHashCode(GroupList);
            return hash;
        }

        /// <summary>
        /// Serializes the field, including the field number, and writes it to
        /// <paramref name="output"/>
        /// </summary>
        /// <param name="fieldNumber">The unknown field number.</param>
        /// <param name="output">The write context to write to.</param>
        internal void WriteTo(int fieldNumber, ref WriteContext output)
        {
            if (VarintList != null)
            {
                foreach (ulong value in VarintList)
                {
                    output.WriteTag(fieldNumber, WireFormat.WireType.Varint);
                    output.WriteUInt64(value);
                }
            }
            if (Fixed32List != null)
            {
                foreach (uint value in Fixed32List)
                {
                    output.WriteTag(fieldNumber, WireFormat.WireType.Fixed32);
                    output.WriteFixed32(value);
                }
            }
            if (Fixed64List != null)
            {
                foreach (ulong value in Fixed64List)
                {
                    output.WriteTag(fieldNumber, WireFormat.WireType.Fixed64);
                    output.WriteFixed64(value);
                }
            }
            if (LengthDelimitedList != null)
            {
                foreach (ByteString value in LengthDelimitedList)
                {
                    output.WriteTag(fieldNumber, WireFormat.WireType.LengthDelimited);
                    output.WriteBytes(value);
                }
            }
            if (GroupList != null)
            {
                foreach (UnknownFieldSet value in GroupList)
                {
                    output.WriteTag(fieldNumber, WireFormat.WireType.StartGroup);
                    value.WriteTo(ref output);
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
            if (VarintList != null)
            {
                result += CodedOutputStream.ComputeTagSize(fieldNumber) * VarintList.Count;
                foreach (ulong value in VarintList)
                {
                    result += CodedOutputStream.ComputeUInt64Size(value);
                }
            }
            if (Fixed32List != null)
            {
                result += CodedOutputStream.ComputeTagSize(fieldNumber) * Fixed32List.Count;
                result += CodedOutputStream.ComputeFixed32Size(1) * Fixed32List.Count;
            }
            if (Fixed64List != null)
            {
                result += CodedOutputStream.ComputeTagSize(fieldNumber) * Fixed64List.Count;
                result += CodedOutputStream.ComputeFixed64Size(1) * Fixed64List.Count;
            }
            if (LengthDelimitedList != null)
            {
                result += CodedOutputStream.ComputeTagSize(fieldNumber) * LengthDelimitedList.Count;
                foreach (ByteString value in LengthDelimitedList)
                {
                    result += CodedOutputStream.ComputeBytesSize(value);
                }
            }
            if (GroupList != null)
            {
                result += CodedOutputStream.ComputeTagSize(fieldNumber) * 2 * GroupList.Count;
                foreach (UnknownFieldSet value in GroupList)
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
            VarintList = AddAll(VarintList, other.VarintList);
            Fixed32List = AddAll(Fixed32List, other.Fixed32List);
            Fixed64List = AddAll(Fixed64List, other.Fixed64List);
            LengthDelimitedList = AddAll(LengthDelimitedList, other.LengthDelimitedList);
            GroupList = AddAll(GroupList, other.GroupList);
            return this;
        }

        /// <summary>
        /// Returns a new list containing all of the given specified values from
        /// both the <paramref name="current"/> and <paramref name="extras"/> lists.
        /// If <paramref name="current" /> is null and <paramref name="extras"/> is null or empty,
        /// null is returned. Otherwise, either a new list is created (if <paramref name="current" />
        /// is null) or the elements of <paramref name="extras"/> are added to <paramref name="current" />.
        /// </summary>
        private static List<T> AddAll<T>(List<T> current, IList<T> extras)
        {
            if (extras == null || extras.Count == 0)
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
            VarintList = Add(VarintList, value);
            return this;
        }

        /// <summary>
        /// Adds a fixed32 value.
        /// </summary>
        internal UnknownField AddFixed32(uint value)
        {
            Fixed32List = Add(Fixed32List, value);
            return this;
        }

        /// <summary>
        /// Adds a fixed64 value.
        /// </summary>
        internal UnknownField AddFixed64(ulong value)
        {
            Fixed64List = Add(Fixed64List, value);
            return this;
        }

        /// <summary>
        /// Adds a length-delimited value.
        /// </summary>
        internal UnknownField AddLengthDelimited(ByteString value)
        {
            LengthDelimitedList = Add(LengthDelimitedList, value);
            return this;
        }

        internal UnknownField AddGroup(UnknownFieldSet value)
        {
            GroupList = Add(GroupList, value);
            return this;
        }

        internal static bool TryGetLastVarint(UnknownField f, out ulong value) => TryGetLastOf(f.VarintList, out value);

        internal static bool TryGetLastFixed32(UnknownField f, out uint value) => TryGetLastOf(f.Fixed32List, out value);

        internal static bool TryGetLastFixed64(UnknownField f, out ulong value) => TryGetLastOf(f.Fixed64List, out value);

        internal static bool TryGetLastLengthDelimited(UnknownField f, out ByteString value) => TryGetLastOf(f.LengthDelimitedList, out value);

        internal static bool TryGetLastOf<T>(List<T> list, out T value)
        {
            if (list != null && list.Count != 0)
            {
                value = list[list.Count - 1];
                return true;
            }
            else
            {
                value = default(T);
                return false;
            }
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
