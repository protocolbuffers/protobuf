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

namespace Google.Protobuf.Reflection
{
    /// <summary>
    /// Container for a set of custom options specified within a message, field etc.
    /// </summary>
    /// <remarks>
    /// <para>
    /// This type is publicly immutable, but internally mutable. It is only populated
    /// by the descriptor parsing code - by the time any user code is able to see an instance,
    /// it will be fully initialized.
    /// </para>
    /// <para>
    /// If an option is requested using the incorrect method, an answer may still be returned: all
    /// of the numeric types are represented internally using 64-bit integers, for example. It is up to
    /// the caller to ensure that they make the appropriate method call for the option they're interested in.
    /// Note that enum options are simply stored as integers, so the value should be fetched using
    /// <see cref="TryGetInt32(int, out int)"/> and then cast appropriately.
    /// </para>
    /// <para>
    /// Repeated options are currently not supported. Asking for a single value of an option
    /// which was actually repeated will return the last value, except for message types where
    /// all the set values are merged together.
    /// </para>
    /// </remarks>
    public sealed class CustomOptions
    {
        /// <summary>
        /// Singleton for all descriptors with an empty set of options.
        /// </summary>
        internal static readonly CustomOptions Empty = new CustomOptions();
        
        /// <summary>
        /// A sequence of values per field. This needs to be per field rather than per tag to allow correct deserialization
        /// of repeated fields which could be "int, ByteString, int" - unlikely as that is. The fact that values are boxed
        /// is unfortunate; we might be able to use a struct instead, and we could combine uint and ulong values.
        /// </summary>
        private readonly Dictionary<int, List<FieldValue>> valuesByField = new Dictionary<int, List<FieldValue>>();

        private CustomOptions() { }

        /// <summary>
        /// Retrieves a Boolean value for the specified option field.
        /// </summary>
        /// <param name="field">The field to fetch the value for.</param>
        /// <param name="value">The output variable to populate.</param>
        /// <returns><c>true</c> if a suitable value for the field was found; <c>false</c> otherwise.</returns>
        public bool TryGetBool(int field, out bool value)
        {
            ulong? tmp = GetLastNumericValue(field);
            value = tmp == 1UL;
            return tmp != null;
        }

        /// <summary>
        /// Retrieves a signed 32-bit integer value for the specified option field.
        /// </summary>
        /// <param name="field">The field to fetch the value for.</param>
        /// <param name="value">The output variable to populate.</param>
        /// <returns><c>true</c> if a suitable value for the field was found; <c>false</c> otherwise.</returns>
        public bool TryGetInt32(int field, out int value)
        {
            ulong? tmp = GetLastNumericValue(field);
            value = (int) tmp.GetValueOrDefault();
            return tmp != null;
        }

        /// <summary>
        /// Retrieves a signed 64-bit integer value for the specified option field.
        /// </summary>
        /// <param name="field">The field to fetch the value for.</param>
        /// <param name="value">The output variable to populate.</param>
        /// <returns><c>true</c> if a suitable value for the field was found; <c>false</c> otherwise.</returns>
        public bool TryGetInt64(int field, out long value)
        {
            ulong? tmp = GetLastNumericValue(field);
            value = (long) tmp.GetValueOrDefault();
            return tmp != null;
        }

        /// <summary>
        /// Retrieves an unsigned 32-bit integer value for the specified option field,
        /// assuming a fixed-length representation.
        /// </summary>
        /// <param name="field">The field to fetch the value for.</param>
        /// <param name="value">The output variable to populate.</param>
        /// <returns><c>true</c> if a suitable value for the field was found; <c>false</c> otherwise.</returns>
        public bool TryGetFixed32(int field, out uint value) => TryGetUInt32(field, out value);

        /// <summary>
        /// Retrieves an unsigned 64-bit integer value for the specified option field,
        /// assuming a fixed-length representation.
        /// </summary>
        /// <param name="field">The field to fetch the value for.</param>
        /// <param name="value">The output variable to populate.</param>
        /// <returns><c>true</c> if a suitable value for the field was found; <c>false</c> otherwise.</returns>
        public bool TryGetFixed64(int field, out ulong value) => TryGetUInt64(field, out value);

        /// <summary>
        /// Retrieves a signed 32-bit integer value for the specified option field,
        /// assuming a fixed-length representation.
        /// </summary>
        /// <param name="field">The field to fetch the value for.</param>
        /// <param name="value">The output variable to populate.</param>
        /// <returns><c>true</c> if a suitable value for the field was found; <c>false</c> otherwise.</returns>
        public bool TryGetSFixed32(int field, out int value) => TryGetInt32(field, out value);

        /// <summary>
        /// Retrieves a signed 64-bit integer value for the specified option field,
        /// assuming a fixed-length representation.
        /// </summary>
        /// <param name="field">The field to fetch the value for.</param>
        /// <param name="value">The output variable to populate.</param>
        /// <returns><c>true</c> if a suitable value for the field was found; <c>false</c> otherwise.</returns>
        public bool TryGetSFixed64(int field, out long value) => TryGetInt64(field, out value);
        
        /// <summary>
        /// Retrieves a signed 32-bit integer value for the specified option field,
        /// assuming a zigzag encoding.
        /// </summary>
        /// <param name="field">The field to fetch the value for.</param>
        /// <param name="value">The output variable to populate.</param>
        /// <returns><c>true</c> if a suitable value for the field was found; <c>false</c> otherwise.</returns>
        public bool TryGetSInt32(int field, out int value)
        {
            ulong? tmp = GetLastNumericValue(field);
            value = CodedInputStream.DecodeZigZag32((uint) tmp.GetValueOrDefault());
            return tmp != null;
        }

        /// <summary>
        /// Retrieves a signed 64-bit integer value for the specified option field,
        /// assuming a zigzag encoding.
        /// </summary>
        /// <param name="field">The field to fetch the value for.</param>
        /// <param name="value">The output variable to populate.</param>
        /// <returns><c>true</c> if a suitable value for the field was found; <c>false</c> otherwise.</returns>
        public bool TryGetSInt64(int field, out long value)
        {
            ulong? tmp = GetLastNumericValue(field);
            value = CodedInputStream.DecodeZigZag64(tmp.GetValueOrDefault());
            return tmp != null;
        }

        /// <summary>
        /// Retrieves an unsigned 32-bit integer value for the specified option field.
        /// </summary>
        /// <param name="field">The field to fetch the value for.</param>
        /// <param name="value">The output variable to populate.</param>
        /// <returns><c>true</c> if a suitable value for the field was found; <c>false</c> otherwise.</returns>
        public bool TryGetUInt32(int field, out uint value)
        {
            ulong? tmp = GetLastNumericValue(field);
            value = (uint) tmp.GetValueOrDefault();
            return tmp != null;
        }

        /// <summary>
        /// Retrieves an unsigned 64-bit integer value for the specified option field.
        /// </summary>
        /// <param name="field">The field to fetch the value for.</param>
        /// <param name="value">The output variable to populate.</param>
        /// <returns><c>true</c> if a suitable value for the field was found; <c>false</c> otherwise.</returns>
        public bool TryGetUInt64(int field, out ulong value)
        {
            ulong? tmp = GetLastNumericValue(field);
            value = tmp.GetValueOrDefault();
            return tmp != null;
        }

        /// <summary>
        /// Retrieves a 32-bit floating point value for the specified option field.
        /// </summary>
        /// <param name="field">The field to fetch the value for.</param>
        /// <param name="value">The output variable to populate.</param>
        /// <returns><c>true</c> if a suitable value for the field was found; <c>false</c> otherwise.</returns>
        public bool TryGetFloat(int field, out float value)
        {
            ulong? tmp = GetLastNumericValue(field);
            int int32 = (int) tmp.GetValueOrDefault();
            byte[] bytes = BitConverter.GetBytes(int32);
            value = BitConverter.ToSingle(bytes, 0);
            return tmp != null;
        }

        /// <summary>
        /// Retrieves a 64-bit floating point value for the specified option field.
        /// </summary>
        /// <param name="field">The field to fetch the value for.</param>
        /// <param name="value">The output variable to populate.</param>
        /// <returns><c>true</c> if a suitable value for the field was found; <c>false</c> otherwise.</returns>
        public bool TryGetDouble(int field, out double value)
        {
            ulong? tmp = GetLastNumericValue(field);
            value = BitConverter.Int64BitsToDouble((long) tmp.GetValueOrDefault());
            return tmp != null;
        }

        /// <summary>
        /// Retrieves a string value for the specified option field.
        /// </summary>
        /// <param name="field">The field to fetch the value for.</param>
        /// <param name="value">The output variable to populate.</param>
        /// <returns><c>true</c> if a suitable value for the field was found; <c>false</c> otherwise.</returns>
        public bool TryGetString(int field, out string value)
        {
            ByteString bytes = GetLastByteStringValue(field);
            value = bytes?.ToStringUtf8();
            return bytes != null;
        }

        /// <summary>
        /// Retrieves a bytes value for the specified option field.
        /// </summary>
        /// <param name="field">The field to fetch the value for.</param>
        /// <param name="value">The output variable to populate.</param>
        /// <returns><c>true</c> if a suitable value for the field was found; <c>false</c> otherwise.</returns>
        public bool TryGetBytes(int field, out ByteString value)
        {
            ByteString bytes = GetLastByteStringValue(field);
            value = bytes;
            return bytes != null;
        }

        /// <summary>
        /// Retrieves a message value for the specified option field.
        /// </summary>
        /// <param name="field">The field to fetch the value for.</param>
        /// <param name="value">The output variable to populate.</param>
        /// <returns><c>true</c> if a suitable value for the field was found; <c>false</c> otherwise.</returns>
        public bool TryGetMessage<T>(int field, out T value) where T : class, IMessage, new()
        {
            value = null;
            List<FieldValue> values;
            if (!valuesByField.TryGetValue(field, out values))
            {
                return false;
            }
            foreach (FieldValue fieldValue in values)
            {
                if (fieldValue.ByteString != null)
                {
                    if (value == null)
                    {
                        value = new T();
                    }
                    value.MergeFrom(fieldValue.ByteString);
                }
            }
            return value != null;
        }

        private ulong? GetLastNumericValue(int field)
        {
            List<FieldValue> values;
            if (!valuesByField.TryGetValue(field, out values))
            {
                return null;
            }
            for (int i = values.Count - 1; i >= 0; i--)
            {
                // A non-bytestring value is a numeric value
                if (values[i].ByteString == null)
                {
                    return values[i].Number;
                }
            }
            return null;
        }

        private ByteString GetLastByteStringValue(int field)
        {
            List<FieldValue> values;
            if (!valuesByField.TryGetValue(field, out values))
            {
                return null;
            }
            for (int i = values.Count - 1; i >= 0; i--)
            {
                if (values[i].ByteString != null)
                {
                    return values[i].ByteString;
                }
            }
            return null;
        }

        /// <summary>
        /// Reads an unknown field, either parsing it and storing it or skipping it.
        /// </summary>
        /// <remarks>
        /// If the current set of options is empty and we manage to read a field, a new set of options
        /// will be created and returned. Otherwise, the return value is <c>this</c>. This allows
        /// us to start with a singleton empty set of options and just create new ones where necessary.
        /// </remarks>
        /// <param name="input">Input stream to read from. </param>
        /// <returns>The resulting set of custom options, either <c>this</c> or a new set.</returns>
        internal CustomOptions ReadOrSkipUnknownField(CodedInputStream input)
        {
            var tag = input.LastTag;
            var field = WireFormat.GetTagFieldNumber(tag);
            switch (WireFormat.GetTagWireType(tag))
            {
                case WireFormat.WireType.LengthDelimited:
                    return AddValue(field, new FieldValue(input.ReadBytes()));
                case WireFormat.WireType.Fixed32:
                    return AddValue(field, new FieldValue(input.ReadFixed32()));
                case WireFormat.WireType.Fixed64:
                    return AddValue(field, new FieldValue(input.ReadFixed64()));
                case WireFormat.WireType.Varint:
                    return AddValue(field, new FieldValue(input.ReadRawVarint64()));
                // For StartGroup, EndGroup or any wire format we don't understand,
                // just use the normal behavior (call SkipLastField).
                default:
                    input.SkipLastField();
                    return this;
            }
        }

        private CustomOptions AddValue(int field, FieldValue value)
        {
            var ret = valuesByField.Count == 0 ? new CustomOptions() : this;
            List<FieldValue> valuesForField;
            if (!ret.valuesByField.TryGetValue(field, out valuesForField))
            {
                // Expect almost all 
                valuesForField = new List<FieldValue>(1);
                ret.valuesByField[field] = valuesForField;
            }
            valuesForField.Add(value);
            return ret;
        }

        /// <summary>
        /// All field values can be stored as a byte string or a 64-bit integer.
        /// This struct avoids unnecessary boxing.
        /// </summary>
        private struct FieldValue
        {
            internal ulong Number { get; }
            internal ByteString ByteString { get; }

            internal FieldValue(ulong number)
            {
                Number = number;
                ByteString = null;
            }

            internal FieldValue(ByteString byteString)
            {
                Number = 0;
                ByteString = byteString;
            }
        }
    }
}
