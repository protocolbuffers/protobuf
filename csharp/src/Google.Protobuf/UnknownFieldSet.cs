#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
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
using System.IO;
using Google.Protobuf.Reflection;

namespace Google.Protobuf
{
    /// <summary>
    /// Used to keep track of fields which were seen when parsing a protocol message
    /// but whose field numbers or types are unrecognized. This most frequently
    /// occurs when new fields are added to a message type and then messages containing
    /// those fields are read by old software that was built before the new types were
    /// added.
    ///
    /// Most users will never need to use this class directly.
    /// </summary>
    public sealed partial class UnknownFieldSet
    {
        private readonly IDictionary<int, UnknownField> fields;

        /// <summary>
        /// Creates a new UnknownFieldSet.
        /// </summary>
        internal UnknownFieldSet()
        {
            this.fields = new Dictionary<int, UnknownField>();
        }

        /// <summary>
        /// Checks whether or not the given field number is present in the set.
        /// </summary>
        internal bool HasField(int field)
        {
            return fields.ContainsKey(field);
        }

        /// <summary>
        /// Serializes the set and writes it to <paramref name="output"/>.
        /// </summary>
        public void WriteTo(CodedOutputStream output)
        {
            foreach (KeyValuePair<int, UnknownField> entry in fields)
            {
                entry.Value.WriteTo(entry.Key, output);
            }
        }

        /// <summary>
        /// Gets the number of bytes required to encode this set.
        /// </summary>
        public int CalculateSize()
        {
            int result = 0;
            foreach (KeyValuePair<int, UnknownField> entry in fields)
            {
                result += entry.Value.GetSerializedSize(entry.Key);
            }
            return result;
        }

        /// <summary>
        /// Checks if two unknown field sets are equal.
        /// </summary>
        public override bool Equals(object other)
        {
            if (ReferenceEquals(this, other))
            {
                return true;
            }
            UnknownFieldSet otherSet = other as UnknownFieldSet;
            IDictionary<int, UnknownField> otherFields = otherSet.fields;
            if (fields.Count  != otherFields.Count)
            {
                return false;
            }
            foreach (KeyValuePair<int, UnknownField> leftEntry in fields)
            {
                UnknownField rightValue;
                if (!otherFields.TryGetValue(leftEntry.Key, out rightValue))
                {
                    return false;
                }
                if (!leftEntry.Value.Equals(rightValue))
                {
                    return false;
                }
            }
            return true;
        }

        /// <summary>
        /// Gets the unknown field set's hash code.
        /// </summary>
        public override int GetHashCode()
        {
            int ret = 1;
            foreach (KeyValuePair<int, UnknownField> field in fields)
            {
                // Use ^ here to make the field order irrelevant.
                int hash = field.Key.GetHashCode() ^ field.Value.GetHashCode();
                ret ^= hash;
            }
            return ret;
        }

        // Optimization:  We keep around the last field that was
        // modified so that we can efficiently add to it multiple times in a
        // row (important when parsing an unknown repeated field).
        private int lastFieldNumber;
        private UnknownField lastField;

        private UnknownField GetOrAddField(int number)
        {
            if (lastField != null && number == lastFieldNumber)
            {
                return lastField;
            }
            if (number == 0)
            {
                return null;
            }

            UnknownField existing;
            if (fields.TryGetValue(number, out existing))
            {
                return existing;
            }
            lastField = new UnknownField();
            AddOrReplaceField(number, lastField);
            lastFieldNumber = number;
            return lastField;
        }

        /// <summary>
        /// Adds a field to the set. If a field with the same number already exists, it
        /// is replaced.
        /// </summary>
        internal UnknownFieldSet AddOrReplaceField(int number, UnknownField field)
        {
            if (number == 0)
            {
                throw new ArgumentOutOfRangeException("number", "Zero is not a valid field number.");
            }
            fields[number] = field;
            return this;
        }

        /// <summary>
        /// Parse a single field from <paramref name="input"/> and merge it
        /// into this set.
        /// </summary>
        /// <param name="input">The coded input stream containing the field</param>
        /// <returns>false if the tag is an "end group" tag, true otherwise</returns>
        private bool MergeFieldFrom(CodedInputStream input)
        {
            uint tag = input.LastTag;
            int number = WireFormat.GetTagFieldNumber(tag);
            switch (WireFormat.GetTagWireType(tag))
            {
                case WireFormat.WireType.Varint:
                    {
                        ulong uint64 = input.ReadUInt64();
                        GetOrAddField(number).AddVarint(uint64);
                        return true;
                    }
                case WireFormat.WireType.Fixed32:
                    {
                        uint uint32 = input.ReadFixed32();
                        GetOrAddField(number).AddFixed32(uint32);
                        return true;
                    }
                case WireFormat.WireType.Fixed64:
                    {
                        ulong uint64 = input.ReadFixed64();
                        GetOrAddField(number).AddFixed64(uint64);
                        return true;
                    }
                case WireFormat.WireType.LengthDelimited:
                    {
                        ByteString bytes = input.ReadBytes();
                        GetOrAddField(number).AddLengthDelimited(bytes);
                        return true;
                    }
                case WireFormat.WireType.StartGroup:
                    {
                        UnknownFieldSet set = new UnknownFieldSet();
                        input.ReadGroup(number, set);
                        GetOrAddField(number).AddGroup(set);
                        return true;
                    }
                case WireFormat.WireType.EndGroup:
                    {
                        return false;
                    }
                default:
                    throw InvalidProtocolBufferException.InvalidWireType();
            }
        }

        internal void MergeGroupFrom(CodedInputStream input)
        {
            while (true)
            {
                uint tag = input.ReadTag();
                if (tag == 0)
                {
                    break;
                }
                if (!MergeFieldFrom(input))
                {
                    break;
                }
            }
        }

        /// <summary>
        /// Create a new UnknownFieldSet if unknownFields is null.
        /// Parse a single field from <paramref name="input"/> and merge it
        /// into unknownFields. If <paramref name="input"/> is configured to discard unknown fields,
        /// <paramref name="unknownFields"/> will be returned as-is and the field will be skipped.
        /// </summary>
        /// <param name="unknownFields">The UnknownFieldSet which need to be merged</param>
        /// <param name="input">The coded input stream containing the field</param>
        /// <returns>The merged UnknownFieldSet</returns>
        public static UnknownFieldSet MergeFieldFrom(UnknownFieldSet unknownFields,
                                                     CodedInputStream input)
        {
            if (input.DiscardUnknownFields)
            {
                input.SkipLastField();
                return unknownFields;
            }
            if (unknownFields == null)
            {
                unknownFields = new UnknownFieldSet();
            }
            if (!unknownFields.MergeFieldFrom(input))
            {
                throw new InvalidProtocolBufferException("Merge an unknown field of end-group tag, indicating that the corresponding start-group was missing."); // match the old code-gen
            }
            return unknownFields;
        }

        /// <summary>
        /// Merges the fields from <paramref name="other"/> into this set.
        /// If a field number exists in both sets, the values in <paramref name="other"/>
        /// will be appended to the values in this set.
        /// </summary>
        private UnknownFieldSet MergeFrom(UnknownFieldSet other)
        {
            if (other != null)
            {
                foreach (KeyValuePair<int, UnknownField> entry in other.fields)
                {
                    MergeField(entry.Key, entry.Value);
                }
            }
            return this;
        }

        /// <summary>
        /// Created a new UnknownFieldSet to <paramref name="unknownFields"/> if
        /// needed and merges the fields from <paramref name="other"/> into the first set.
        /// If a field number exists in both sets, the values in <paramref name="other"/>
        /// will be appended to the values in this set.
        /// </summary>
        public static UnknownFieldSet MergeFrom(UnknownFieldSet unknownFields,
                                                UnknownFieldSet other)
        {
            if (other == null)
            {
                return unknownFields;
            }
            if (unknownFields == null)
            {
                unknownFields = new UnknownFieldSet();
            }
            unknownFields.MergeFrom(other);
            return unknownFields;
        }


        /// <summary>
        /// Adds a field to the unknown field set. If a field with the same
        /// number already exists, the two are merged.
        /// </summary>
        private UnknownFieldSet MergeField(int number, UnknownField field)
        {
            if (number == 0)
            {
                throw new ArgumentOutOfRangeException("number", "Zero is not a valid field number.");
            }
            if (HasField(number))
            {
                GetOrAddField(number).MergeFrom(field);
            }
            else
            {
                AddOrReplaceField(number, field);
            }
            return this;
        }

        /// <summary>
        /// Clone an unknown field set from <paramref name="other"/>.
        /// </summary>
        public static UnknownFieldSet Clone(UnknownFieldSet other)
        {
            if (other == null)
            {
                return null;
            }
            UnknownFieldSet unknownFields = new UnknownFieldSet();
            unknownFields.MergeFrom(other);
            return unknownFields;
        }
    }
}

