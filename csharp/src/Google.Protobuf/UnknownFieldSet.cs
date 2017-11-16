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
	public sealed partial class UnknownFieldSet : IMessage
	{
		private static readonly UnknownFieldSet defaultInstance =
				new UnknownFieldSet();
		private readonly IDictionary<int, UnknownField> fields;

		/// <summary>
		/// Creates a new UnknownFieldSet.
		/// </summary>
		public UnknownFieldSet()
		{
			this.fields = new Dictionary<int, UnknownField>();
		}

		/// <summary>
		/// Gets the default UnknownFieldSet instance.
		/// </summary>
		public static UnknownFieldSet DefaultInstance
		{
			get { return defaultInstance; }
		}

		/// <summary>
		/// Checks whether or not the given field number is present in the set.
		/// </summary>
		public bool HasField(int field)
		{
			return fields.ContainsKey(field);
		}

		/// <summary>
		/// Fetches a field by number, returning an empty field if not present.
		/// Never returns null.
		/// </summary>
		public UnknownField this[int number]
		{
			get
			{
				UnknownField ret;
				if (!fields.TryGetValue(number, out ret))
				{
					ret = UnknownField.DefaultInstance;
				}
				return ret;
			}
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
				int hash = field.Key.GetHashCode() ^ field.Value.GetHashCode();
				ret ^= hash;
			}
			return ret;
		}

		#region IMessage Members
		/// <summary>
		/// Gets the descriptor. It is never used. Returns null to implement
		/// the interface member 'IMessage.Descriptor'.
		/// </summary>
		public MessageDescriptor Descriptor
		{
			get { return null; }
		}

		/// <summary>
		/// Checks if the unknown field set is initialized. It is never used.
		/// Returns true to implement the interface.
		/// </summary>
		public bool IsInitialized
		{
			get { return true; }
		}
		#endregion

		// Optimization:  We keep around the last field that was
		// modified so that we can efficiently add to it multiple times in a
		// row (important when parsing an unknown repeated field).
		private int lastFieldNumber;
		private UnknownField lastField;

		private UnknownField GetField(int number)
		{
			if (lastField != null)
			{
				if (number == lastFieldNumber)
				{
					return lastField;
				}
			}
			if (number == 0)
			{
				return null;
			}

			lastField = new UnknownField();
			UnknownField existing;
			if (fields.TryGetValue(number, out existing))
			{
				lastField.MergeFrom(existing);
			} else
			{
				AddField(number, lastField);
			}
			lastFieldNumber = number;
			return lastField;
		}

		/// <summary>
		/// Adds a field to the set. If a field with the same number already exists, it
		/// is replaced.
		/// </summary>
		public UnknownFieldSet AddField(int number, UnknownField field)
		{
			if (number == 0)
			{
				throw new ArgumentOutOfRangeException("number", "Zero is not a valid field number.");
			}
			fields[number] = field;
			return this;
		}

		/// <summary>
		/// Resets to an empty set.
		/// </summary>
		public UnknownFieldSet Clear()
		{
			fields.Clear();
			lastFieldNumber = 0;
			lastField = null;
			return this;
		}

		/// <summary>
		/// Parse an entire message from <paramref name="input"/> and merge
		/// its fields into this set.
		/// </summary>
		public void MergeFrom(CodedInputStream input)
		{
			uint tag;
			tag = input.ReadTag();
			while (tag != 0)
			{
				if (!MergeFieldFrom(input))
				{
					break;
				}
				tag = input.ReadTag();
			}
		}

		/// <summary>
		/// Parse a single field from <paramref name="input"/> and merge it
		/// into this set.
		/// </summary>
		/// <param name="input">The coded input stream containing the field</param>
		/// <returns>false if the tag is an "end group" tag, true otherwise</returns>
		public bool MergeFieldFrom(CodedInputStream input)
		{
			uint tag = input.LastTag;
			int number = WireFormat.GetTagFieldNumber(tag);
			switch (WireFormat.GetTagWireType(tag))
			{
				case WireFormat.WireType.Varint:
					{
						ulong uint64 = input.ReadUInt64();
						GetField(number).AddVarint(uint64);
						return true;
					}
				case WireFormat.WireType.Fixed32:
					{
						uint uint32 = input.ReadFixed32();
						GetField(number).AddFixed32(uint32);
						return true;
					}
				case WireFormat.WireType.Fixed64:
					{
						ulong uint64 = input.ReadFixed64();
						GetField(number).AddFixed64(uint64);
						return true;
					}
				case WireFormat.WireType.LengthDelimited:
					{
						ByteString bytes = input.ReadBytes();
						GetField(number).AddLengthDelimited(bytes);
						return true;
					}
				default:
					throw new InvalidOperationException("Wire Type is invalid.");
			}
		}

		/// <summary>
		/// Merges the fields from <paramref name="other"/> into this set.
		/// If a field number exists in both sets, the values in <paramref name="other"/>
		/// will be appended to the values in this set.
		/// </summary>
		public UnknownFieldSet MergeFrom(UnknownFieldSet other)
		{
			if (other != DefaultInstance)
			{
				foreach (KeyValuePair<int, UnknownField> entry in other.fields)
				{
					MergeField(entry.Key, entry.Value);
				}
			}
			return this;
		}

		/// <summary>
		/// Adds a field to the unknown field set. If a field with the same
		/// number already exists, the two are merged.
		/// </summary>
		public UnknownFieldSet MergeField(int number, UnknownField field)
		{
			if (number == 0)
			{
				throw new ArgumentOutOfRangeException("number", "Zero is not a valid field number.");
			}
			if (HasField(number))
			{
				GetField(number).MergeFrom(field);
			}
			else
			{
				AddField(number, field);
			}
			return this;
		}
	}
}

