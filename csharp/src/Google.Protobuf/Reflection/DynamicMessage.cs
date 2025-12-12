#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Security;
using Google.Protobuf.Compatibility;

namespace Google.Protobuf.Reflection
{
    /// <summary>
    /// An implementation of <see cref="IMessage"/> that can represent arbitrary types,
    /// given a <see cref="MessageDescriptor"/>.
    /// </summary>
    /// <remarks>
    /// <para>
    /// This class is used to implement dynamic message support, allowing messages to be
    /// created and manipulated at runtime without generating code. This is useful for
    /// scenarios such as dynamic gRPC clients, protoc plugins written in C#, and
    /// processing user-provided schemas at runtime.
    /// </para>
    /// <para>
    /// Unlike generated message types, DynamicMessage is mutable. Use <see cref="SetField"/>
    /// and <see cref="ClearField"/> to modify field values.
    /// </para>
    /// </remarks>
    public sealed class DynamicMessage
        : IMessage<DynamicMessage>,
            IBufferMessage,
            IEquatable<DynamicMessage>,
            IDeepCloneable<DynamicMessage>
    {
        private readonly MessageDescriptor descriptor;
        private readonly SortedDictionary<int, object> fields;
        private readonly FieldDescriptor[] oneofCases;
        // Extensions are stored in the same 'fields' dictionary, but we need to track their
        // FieldDescriptors separately since descriptor.FindFieldByNumber() won't find them.
        // This maps field number to the extension FieldDescriptor.
        private readonly Dictionary<int, FieldDescriptor> extensionFieldDescriptors;
        private UnknownFieldSet unknownFields;

        /// <summary>
        /// Creates a new empty DynamicMessage for the given descriptor.
        /// </summary>
        /// <param name="descriptor">The message descriptor. Must be a dynamically loaded descriptor
        /// (i.e., one without a generated CLR type).</param>
        /// <exception cref="ArgumentException">The descriptor has a CLR type associated with it,
        /// indicating it was loaded from generated code rather than dynamically.</exception>
        public DynamicMessage(MessageDescriptor descriptor)
        {
            this.descriptor = ProtoPreconditions.CheckNotNull(descriptor, nameof(descriptor));
            if (descriptor.ClrType != null)
            {
                throw new ArgumentException(
                    $"DynamicMessage cannot be used with generated code descriptors. " +
                    $"The descriptor for '{descriptor.FullName}' has ClrType '{descriptor.ClrType.FullName}'. " +
                    $"Use FileDescriptor.BuildFromByteStrings() to create a dynamic descriptor, " +
                    $"or use the generated message type directly.",
                    nameof(descriptor));
            }
            this.fields = new SortedDictionary<int, object>();
            this.oneofCases = new FieldDescriptor[descriptor.Oneofs.Count];
            this.extensionFieldDescriptors = new Dictionary<int, FieldDescriptor>();
        }

        /// <summary>
        /// Creates a DynamicMessage as a copy of another message.
        /// </summary>
        private DynamicMessage(DynamicMessage other)
        {
            this.descriptor = other.descriptor;
            this.fields = new SortedDictionary<int, object>();
            this.oneofCases = new FieldDescriptor[other.oneofCases.Length];
            this.extensionFieldDescriptors = new Dictionary<int, FieldDescriptor>(other.extensionFieldDescriptors);

            // Deep copy fields
            foreach (var entry in other.fields)
            {
                fields[entry.Key] = CloneValue(entry.Value);
            }
            Array.Copy(other.oneofCases, oneofCases, oneofCases.Length);
            unknownFields = UnknownFieldSet.Clone(other.unknownFields);
        }

        /// <summary>
        /// Creates a parser for DynamicMessage of the given descriptor.
        /// </summary>
        /// <param name="descriptor">The message descriptor.</param>
        /// <returns>A message parser.</returns>
        public static MessageParser<DynamicMessage> CreateParser(MessageDescriptor descriptor)
        {
            return new MessageParser<DynamicMessage>(() => new DynamicMessage(descriptor));
        }

        private static object CloneValue(object value)
        {
            if (value is DynamicMessage dm)
            {
                return dm.Clone();
            }
            if (value is IList list)
            {
                var newList = new List<object>(list.Count);
                foreach (var item in list)
                {
                    newList.Add(CloneValue(item));
                }
                return newList;
            }
            if (value is IDictionary dict)
            {
                var newDict = new Dictionary<object, object>(dict.Count);
                foreach (DictionaryEntry entry in dict)
                {
                    // Map keys are scalar types; values may include nested messages.
                    newDict[entry.Key] = CloneValue(entry.Value);
                }
                return newDict;
            }
            // Primitive types and ByteString are immutable
            return value;
        }

        private static bool CanBePacked(FieldDescriptor field)
        {
            return field.FieldType switch
            {
                FieldType.Double
                or FieldType.Float
                or FieldType.Int64
                or FieldType.UInt64
                or FieldType.Int32
                or FieldType.Fixed64
                or FieldType.Fixed32
                or FieldType.Bool
                or FieldType.UInt32
                or FieldType.SFixed32
                or FieldType.SFixed64
                or FieldType.SInt32
                or FieldType.SInt64
                or FieldType.Enum
                    => true,
                _ => false,
            };
        }

        #region Static factory methods

        /// <summary>
        /// Parses a message from the given byte array.
        /// </summary>
        /// <param name="descriptor">The message descriptor.</param>
        /// <param name="data">The byte array containing the serialized message.</param>
        /// <returns>The parsed message.</returns>
        public static DynamicMessage ParseFrom(MessageDescriptor descriptor, byte[] data)
        {
            ProtoPreconditions.CheckNotNull(descriptor, nameof(descriptor));
            ProtoPreconditions.CheckNotNull(data, nameof(data));
            var message = new DynamicMessage(descriptor);
            message.MergeFrom(data);
            return message;
        }

        /// <summary>
        /// Parses a message from the given <see cref="ByteString"/>.
        /// </summary>
        /// <param name="descriptor">The message descriptor.</param>
        /// <param name="data">The ByteString containing the serialized message.</param>
        /// <returns>The parsed message.</returns>
        public static DynamicMessage ParseFrom(MessageDescriptor descriptor, ByteString data)
        {
            ProtoPreconditions.CheckNotNull(descriptor, nameof(descriptor));
            ProtoPreconditions.CheckNotNull(data, nameof(data));
            return ParseFrom(descriptor, data.ToByteArray());
        }

        /// <summary>
        /// Parses a message from the given <see cref="CodedInputStream"/>.
        /// </summary>
        /// <param name="descriptor">The message descriptor.</param>
        /// <param name="input">The coded input stream to read from.</param>
        /// <returns>The parsed message.</returns>
        public static DynamicMessage ParseFrom(MessageDescriptor descriptor, CodedInputStream input)
        {
            ProtoPreconditions.CheckNotNull(descriptor, nameof(descriptor));
            ProtoPreconditions.CheckNotNull(input, nameof(input));
            var message = new DynamicMessage(descriptor);
            message.MergeFrom(input);
            return message;
        }

        #endregion

        #region IMessage implementation

        /// <inheritdoc/>
        public MessageDescriptor Descriptor => descriptor;

        /// <inheritdoc/>
        public void MergeFrom(CodedInputStream input)
        {
            ProtoPreconditions.CheckNotNull(input, nameof(input));
            ParseContext.Initialize(input, out ParseContext ctx);
            try
            {
                MergeFromInternal(ref ctx);
            }
            finally
            {
                ctx.CopyStateTo(input);
            }
        }

        /// <summary>
        /// Merges fields from a byte array.
        /// </summary>
        /// <param name="data">The byte array to parse.</param>
        public void MergeFrom(byte[] data)
        {
            ProtoPreconditions.CheckNotNull(data, nameof(data));
            var input = new CodedInputStream(data);
            MergeFrom(input);
        }

        /// <inheritdoc/>
        public void WriteTo(CodedOutputStream output)
        {
            WriteContext.Initialize(output, out WriteContext ctx);
            try
            {
                ((IBufferMessage)this).InternalWriteTo(ref ctx);
            }
            finally
            {
                ctx.CopyStateTo(output);
            }
        }

        /// <inheritdoc/>
        public int CalculateSize()
        {
            int size = 0;
            foreach (var entry in fields)
            {
                var field = FindFieldByNumberIncludingExtensions(entry.Key);
                var value = entry.Value;

                if (field.IsMap)
                {
                    size += ComputeMapFieldSize(field, (IDictionary)value);
                }
                else if (field.IsRepeated)
                {
                    size += ComputeRepeatedFieldSize(field, (IList)value);
                }
                else
                {
                    size += ComputeSingularFieldSize(field, value);
                }
            }

            if (unknownFields != null)
            {
                size += unknownFields.CalculateSize();
            }

            return size;
        }

        #endregion

        #region IBufferMessage implementation

        /// <inheritdoc/>
        [SecuritySafeCritical]
        void IBufferMessage.InternalMergeFrom(ref ParseContext ctx)
        {
            MergeFromInternal(ref ctx);
        }

        /// <inheritdoc/>
        [SecuritySafeCritical]
        void IBufferMessage.InternalWriteTo(ref WriteContext ctx)
        {
            foreach (var entry in fields)
            {
                var field = FindFieldByNumberIncludingExtensions(entry.Key);
                var value = entry.Value;

                if (field.IsMap)
                {
                    WriteMapField(ref ctx, field, (IDictionary)value);
                }
                else if (field.IsRepeated)
                {
                    WriteRepeatedField(ref ctx, field, (IList)value);
                }
                else
                {
                    WriteSingularField(ref ctx, field, value);
                }
            }

            unknownFields?.WriteTo(ref ctx);
        }

        #endregion

        #region IMessage<DynamicMessage> implementation

        /// <inheritdoc/>
        public void MergeFrom(DynamicMessage message)
        {
            if (message == null)
                return;
            if (message.descriptor != descriptor)
            {
                throw new ArgumentException("Cannot merge messages of different types.");
            }

            foreach (var entry in message.fields)
            {
                var fieldNumber = entry.Key;
                var field = message.FindFieldByNumberIncludingExtensions(fieldNumber);
                var value = entry.Value;

                if (field.IsMap)
                {
                    var existingDict = GetOrCreateMap(field);
                    foreach (DictionaryEntry mapEntry in (IDictionary)value)
                    {
                        existingDict[mapEntry.Key] = mapEntry.Value;
                    }
                    // Track extension field descriptors
                    if (field.IsExtension)
                    {
                        extensionFieldDescriptors[fieldNumber] = field;
                    }
                }
                else if (field.IsRepeated)
                {
                    var existingList = GetOrCreateList(field);
                    foreach (var item in (IList)value)
                    {
                        existingList.Add(item);
                    }
                    // Track extension field descriptors
                    if (field.IsExtension)
                    {
                        extensionFieldDescriptors[fieldNumber] = field;
                    }
                }
                else if (field.FieldType == FieldType.Message)
                {
                    if (
                        fields.TryGetValue(fieldNumber, out var existing)
                        && existing is DynamicMessage existingMsg
                    )
                    {
                        existingMsg.MergeFrom((DynamicMessage)value);
                    }
                    else
                    {
                        SetField(field, ((DynamicMessage)value).Clone());
                    }
                }
                else
                {
                    SetField(field, value);
                }
            }

            unknownFields = UnknownFieldSet.MergeFrom(unknownFields, message.unknownFields);
        }

        #endregion

        #region IEquatable<DynamicMessage> implementation

        /// <inheritdoc/>
        public bool Equals(DynamicMessage other)
        {
            if (other is null)
                return false;
            if (ReferenceEquals(this, other))
                return true;
            if (descriptor != other.descriptor)
                return false;

            // Note: GetField for repeated/map fields may store an empty list/dictionary so that
            // caller mutations persist. Empty repeated/map fields are still logically "not set",
            // so equality should ignore such entries.
            foreach (var entry in fields)
            {
                var field = FindFieldByNumberIncludingExtensions(entry.Key);
                if (field.IsMap)
                {
                    if (((IDictionary)entry.Value).Count == 0)
                    {
                        // Empty map fields are ignored for equality
                        continue;
                    }
                }
                else if (field.IsRepeated)
                {
                    if (((IList)entry.Value).Count == 0)
                    {
                        // Empty repeated fields are ignored for equality
                        continue;
                    }
                }

                if (!other.fields.TryGetValue(entry.Key, out var otherValue))
                {
                    return false;
                }
                if (!FieldValuesEqual(entry.Value, otherValue))
                {
                    return false;
                }
            }

            foreach (var entry in other.fields)
            {
                var field = other.FindFieldByNumberIncludingExtensions(entry.Key);
                if (field.IsMap)
                {
                    if (((IDictionary)entry.Value).Count == 0)
                    {
                        // Empty map fields are ignored for equality
                        continue;
                    }
                }
                else if (field.IsRepeated)
                {
                    if (((IList)entry.Value).Count == 0)
                    {
                        // Empty repeated fields are ignored for equality
                        continue;
                    }
                }

                if (!fields.TryGetValue(entry.Key, out var thisValue))
                {
                    return false;
                }
                if (!FieldValuesEqual(thisValue, entry.Value))
                {
                    return false;
                }
            }

            if (unknownFields == null)
            {
                return other.unknownFields == null;
            }
            return unknownFields.Equals(other.unknownFields);
        }

        /// <inheritdoc/>
        public override bool Equals(object obj) => Equals(obj as DynamicMessage);

        /// <inheritdoc/>
        public override int GetHashCode()
        {
            int hash = descriptor.GetHashCode();
            foreach (var entry in fields)
            {
                var field = FindFieldByNumberIncludingExtensions(entry.Key);
                if (field.IsMap)
                {
                    if (((IDictionary)entry.Value).Count == 0)
                    {
                        // Empty map fields are ignored for equality
                        continue;
                    }
                }
                else if (field.IsRepeated)
                {
                    if (((IList)entry.Value).Count == 0)
                    {
                        // Empty repeated fields are ignored for equality
                        continue;
                    }
                }
                hash = hash * 31 + entry.Key;
                hash = hash * 31 + GetFieldValueHashCode(entry.Value);
            }
            if (unknownFields != null)
            {
                hash = hash * 31 + unknownFields.GetHashCode();
            }
            return hash;
        }

        #endregion

        #region IDeepCloneable<DynamicMessage> implementation

        /// <inheritdoc/>
        public DynamicMessage Clone() => new DynamicMessage(this);

        #endregion

        #region Field access methods

        /// <summary>
        /// Checks whether a field is set.
        /// </summary>
        public bool HasField(FieldDescriptor field)
        {
            VerifyContainingType(field);
            if (field.IsRepeated)
            {
                throw new ArgumentException("HasField cannot be called on repeated fields.");
            }
            return fields.ContainsKey(field.FieldNumber);
        }

        /// <summary>
        /// Gets the value of a field.
        /// </summary>
        public object GetField(FieldDescriptor field)
        {
            VerifyContainingType(field);
            if (fields.TryGetValue(field.FieldNumber, out var value))
            {
                return value;
            }
            var defaultValue = GetDefaultValue(field);
            // For repeated and map fields, store the default collection so that
            // callers can add to it and have the changes reflected in this message.
            // This is important for JsonParser which gets the list and adds to it.
            if (field.IsRepeated || field.IsMap)
            {
                fields[field.FieldNumber] = defaultValue;
            }
            return defaultValue;
        }

        /// <summary>
        /// Sets a field value.
        /// </summary>
        public void SetField(FieldDescriptor field, object value)
        {
            VerifyContainingType(field);
            VerifyType(field, value);

            var oneof = field.RealContainingOneof;
            if (oneof != null)
            {
                var currentCase = oneofCases[oneof.Index];
                if (currentCase != null && currentCase != field)
                {
                    fields.Remove(currentCase.FieldNumber);
                }
                oneofCases[oneof.Index] = field;
            }
            else if (!field.HasPresence && !field.IsRepeated)
            {
                if (IsDefaultValue(field, value))
                {
                    fields.Remove(field.FieldNumber);
                    // Also remove from extension tracking if it's an extension
                    if (field.IsExtension)
                    {
                        extensionFieldDescriptors.Remove(field.FieldNumber);
                    }
                    return;
                }
            }

            fields[field.FieldNumber] = value;
            // Track extension field descriptors so we can find them later
            if (field.IsExtension)
            {
                extensionFieldDescriptors[field.FieldNumber] = field;
            }
        }

        /// <summary>
        /// Clears a field.
        /// </summary>
        public void ClearField(FieldDescriptor field)
        {
            VerifyContainingType(field);
            fields.Remove(field.FieldNumber);

            // Remove from extension tracking if it's an extension
            if (field.IsExtension)
            {
                extensionFieldDescriptors.Remove(field.FieldNumber);
            }

            var oneof = field.RealContainingOneof;
            if (oneof != null && oneofCases[oneof.Index] == field)
            {
                oneofCases[oneof.Index] = null;
            }
        }

        /// <summary>
        /// Gets the number of elements in a repeated field.
        /// </summary>
        public int GetRepeatedFieldCount(FieldDescriptor field)
        {
            VerifyContainingType(field);
            if (!field.IsRepeated)
            {
                throw new ArgumentException(
                    "GetRepeatedFieldCount can only be called on repeated fields."
                );
            }
            if (fields.TryGetValue(field.FieldNumber, out var value))
            {
                return ((IList)value).Count;
            }
            return 0;
        }

        /// <summary>
        /// Gets an element from a repeated field.
        /// </summary>
        public object GetRepeatedField(FieldDescriptor field, int index)
        {
            VerifyContainingType(field);
            if (!field.IsRepeated)
            {
                throw new ArgumentException(
                    "GetRepeatedField can only be called on repeated fields."
                );
            }
            if (!fields.TryGetValue(field.FieldNumber, out var value))
            {
                throw new ArgumentOutOfRangeException(nameof(index));
            }
            return ((IList)value)[index];
        }

        /// <summary>
        /// Sets an element in a repeated field.
        /// </summary>
        public void SetRepeatedField(FieldDescriptor field, int index, object value)
        {
            VerifyContainingType(field);
            if (!field.IsRepeated)
            {
                throw new ArgumentException(
                    "SetRepeatedField can only be called on repeated fields."
                );
            }
            VerifyElementType(field, value);

            if (!fields.TryGetValue(field.FieldNumber, out var listObj))
            {
                throw new ArgumentOutOfRangeException(nameof(index));
            }
            ((List<object>)listObj)[index] = value;
        }

        /// <summary>
        /// Adds an element to a repeated field.
        /// </summary>
        public void AddRepeatedField(FieldDescriptor field, object value)
        {
            VerifyContainingType(field);
            if (!field.IsRepeated)
            {
                throw new ArgumentException(
                    "AddRepeatedField can only be called on repeated fields."
                );
            }
            VerifyElementType(field, value);

            GetOrCreateList(field).Add(value);
            // Track extension field descriptors so we can find them later
            if (field.IsExtension)
            {
                extensionFieldDescriptors[field.FieldNumber] = field;
            }
        }

        /// <summary>
        /// Checks whether a oneof has a field set.
        /// </summary>
        public bool HasOneof(OneofDescriptor oneof)
        {
            VerifyOneofContainingType(oneof);
            return oneofCases[oneof.Index] != null;
        }

        /// <summary>
        /// Gets the field descriptor for the currently set field in a oneof.
        /// </summary>
        public FieldDescriptor GetOneofFieldDescriptor(OneofDescriptor oneof)
        {
            VerifyOneofContainingType(oneof);
            return oneofCases[oneof.Index];
        }

        /// <summary>
        /// Clears a oneof.
        /// </summary>
        public void ClearOneof(OneofDescriptor oneof)
        {
            VerifyOneofContainingType(oneof);
            var currentCase = oneofCases[oneof.Index];
            if (currentCase != null)
            {
                fields.Remove(currentCase.FieldNumber);
                oneofCases[oneof.Index] = null;
            }
        }

        /// <summary>
        /// Gets the unknown fields for this message.
        /// </summary>
        public UnknownFieldSet UnknownFields => unknownFields;

        /// <summary>
        /// Gets all fields that are set in this message.
        /// </summary>
        public IDictionary<FieldDescriptor, object> GetAllFields()
        {
            var result = new Dictionary<FieldDescriptor, object>();
            foreach (var entry in fields)
            {
                var field = FindFieldByNumberIncludingExtensions(entry.Key);
                if (field.IsMap)
                {
                    if (((IDictionary)entry.Value).Count == 0)
                    {
                        // Empty map fields are ignored for equality
                        continue;
                    }
                }
                else if (field.IsRepeated)
                {
                    if (((IList)entry.Value).Count == 0)
                    {
                        // Empty repeated fields are ignored for equality
                        continue;
                    }
                }

                result[field] = entry.Value;
            }
            return result;
        }

        #endregion

        #region Private helpers

        /// <summary>
        /// Finds a field by number, checking both regular fields and extensions.
        /// </summary>
        private FieldDescriptor FindFieldByNumberIncludingExtensions(int fieldNumber)
        {
            // First check regular fields
            var field = descriptor.FindFieldByNumber(fieldNumber);
            if (field != null)
            {
                return field;
            }
            // Then check extension fields we know about
            extensionFieldDescriptors.TryGetValue(fieldNumber, out field);
            return field;
        }

        private void VerifyContainingType(FieldDescriptor field)
        {
            // ContainingType returns:
            // - For regular fields: the message type that contains the field
            // - For extension fields: the type being extended (extendee type)
            if (field.ContainingType != null && field.ContainingType.Equals(descriptor))
            {
                return;
            }

            throw new ArgumentException(
                $"Field '{field.FullName}' does not belong to message type '{descriptor.FullName}'."
            );
        }

        private void VerifyOneofContainingType(OneofDescriptor oneof)
        {
            if (!oneof.ContainingType.Equals(descriptor))
            {
                throw new ArgumentException(
                    $"Oneof '{oneof.Name}' does not belong to message type '{descriptor.FullName}'."
                );
            }
        }

        private static object GetDefaultValue(FieldDescriptor field)
        {
            if (field.IsMap)
            {
                return new Dictionary<object, object>();
            }
            if (field.IsRepeated)
            {
                return new List<object>();
            }
            return field.FieldType switch
            {
                FieldType.Bool => false,
                FieldType.Bytes => ByteString.Empty,
                FieldType.String => "",
                FieldType.Double => 0.0,
                FieldType.Float => 0f,
                FieldType.Int32 or FieldType.SInt32 or FieldType.SFixed32 => 0,
                FieldType.Int64 or FieldType.SInt64 or FieldType.SFixed64 => 0L,
                FieldType.UInt32 or FieldType.Fixed32 => 0u,
                FieldType.UInt64 or FieldType.Fixed64 => 0UL,
                FieldType.Enum => 0,
                FieldType.Message or FieldType.Group => new DynamicMessage(field.MessageType),
                _ => throw new InvalidOperationException($"Unknown field type: {field.FieldType}"),
            };
        }

        private static bool FieldValuesEqual(object a, object b)
        {
            if (a == null && b == null)
                return true;
            if (a == null || b == null)
                return false;

            if (a is IList listA && b is IList listB)
            {
                if (listA.Count != listB.Count)
                    return false;
                for (int i = 0; i < listA.Count; i++)
                {
                    if (!FieldValuesEqual(listA[i], listB[i]))
                        return false;
                }
                return true;
            }

            if (a is IDictionary dictA && b is IDictionary dictB)
            {
                if (dictA.Count != dictB.Count)
                    return false;
                foreach (DictionaryEntry entry in dictA)
                {
                    if (!dictB.Contains(entry.Key))
                        return false;
                    if (!FieldValuesEqual(entry.Value, dictB[entry.Key]))
                        return false;
                }
                return true;
            }

            if (a is ByteString bsA && b is ByteString bsB)
            {
                return bsA.Equals(bsB);
            }

            // Use bitwise comparison for float/double to properly handle NaN
            if (a is double da && b is double db)
            {
                return Collections.ProtobufEqualityComparers.BitwiseDoubleEqualityComparer.Equals(da, db);
            }

            if (a is float fa && b is float fb)
            {
                return Collections.ProtobufEqualityComparers.BitwiseSingleEqualityComparer.Equals(fa, fb);
            }

            return a.Equals(b);
        }

        private static int GetFieldValueHashCode(object value)
        {
            if (value == null)
                return 0;

            if (value is IList list)
            {
                int hash = 0;
                foreach (var item in list)
                {
                    hash = hash * 31 + GetFieldValueHashCode(item);
                }
                return hash;
            }

            if (value is IDictionary dict)
            {
                int hash = 0;
                foreach (DictionaryEntry entry in dict)
                {
                    hash ^= GetFieldValueHashCode(entry.Key) ^ GetFieldValueHashCode(entry.Value);
                }
                return hash;
            }

            // Use bitwise hash codes for float/double to be consistent with equality
            if (value is double d)
            {
                return Collections.ProtobufEqualityComparers.BitwiseDoubleEqualityComparer.GetHashCode(d);
            }

            if (value is float f)
            {
                return Collections.ProtobufEqualityComparers.BitwiseSingleEqualityComparer.GetHashCode(f);
            }

            return value.GetHashCode();
        }

        private static void VerifyType(FieldDescriptor field, object value)
        {
            if (field.IsMap)
            {
                if (value is not IDictionary dict)
                {
                    throw new ArgumentException($"Value for map field must be a dictionary.");
                }
                // For map fields, we verify the key/value types
                var keyField = field.MessageType.FindFieldByNumber(1);
                var valueField = field.MessageType.FindFieldByNumber(2);
                foreach (DictionaryEntry entry in dict)
                {
                    VerifyElementType(keyField, entry.Key);
                    VerifyElementType(valueField, entry.Value);
                }
            }
            else if (field.IsRepeated)
            {
                if (value is not IList list)
                {
                    throw new ArgumentException($"Value for repeated field must be a list.");
                }
                foreach (var item in list)
                {
                    VerifyElementType(field, item);
                }
            }
            else
            {
                VerifyElementType(field, value);
            }
        }

        private static void VerifyElementType(FieldDescriptor field, object value)
        {
            if (value == null)
            {
                throw new ArgumentNullException(nameof(value), $"Value for field {field.FullName} cannot be null.");
            }
            var expectedType = GetExpectedType(field);
            if (!expectedType.IsAssignableFrom(value.GetType()))
            {
                throw new ArgumentException(
                    $"Value of type {value.GetType()} is not valid for field {field.FullName} which expects {expectedType}."
                );
            }
        }

        private static Type GetExpectedType(FieldDescriptor field)
        {
            return field.FieldType switch
            {
                FieldType.Double => typeof(double),
                FieldType.Float => typeof(float),
                FieldType.Int64 or FieldType.SInt64 or FieldType.SFixed64 => typeof(long),
                FieldType.UInt64 or FieldType.Fixed64 => typeof(ulong),
                FieldType.Int32
                or FieldType.SInt32
                or FieldType.SFixed32
                or FieldType.Enum
                    => typeof(int),
                FieldType.UInt32 or FieldType.Fixed32 => typeof(uint),
                FieldType.Bool => typeof(bool),
                FieldType.String => typeof(string),
                FieldType.Bytes => typeof(ByteString),
                FieldType.Message or FieldType.Group => typeof(IMessage),
                _ => typeof(object),
            };
        }

        private static bool IsDefaultValue(FieldDescriptor field, object value)
        {
            if (value == null)
                return true;

            return field.FieldType switch
            {
                FieldType.Bool => (bool)value == false,
                FieldType.String => string.IsNullOrEmpty((string)value),
                FieldType.Bytes => ((ByteString)value).IsEmpty,
                FieldType.Double => (double)value == 0.0,
                FieldType.Float => (float)value == 0f,
                FieldType.Int32
                or FieldType.SInt32
                or FieldType.SFixed32
                or FieldType.Enum
                    => (int)value == 0,
                FieldType.Int64 or FieldType.SInt64 or FieldType.SFixed64 => (long)value == 0L,
                FieldType.UInt32 or FieldType.Fixed32 => (uint)value == 0u,
                FieldType.UInt64 or FieldType.Fixed64 => (ulong)value == 0UL,
                _ => false,
            };
        }

        private List<object> GetOrCreateList(FieldDescriptor field)
        {
            if (!fields.TryGetValue(field.FieldNumber, out var listObj))
            {
                listObj = new List<object>();
                fields[field.FieldNumber] = listObj;
            }
            return (List<object>)listObj;
        }

        private Dictionary<object, object> GetOrCreateMap(FieldDescriptor field)
        {
            if (!fields.TryGetValue(field.FieldNumber, out var dictObj))
            {
                dictObj = new Dictionary<object, object>();
                fields[field.FieldNumber] = dictObj;
            }
            return (Dictionary<object, object>)dictObj;
        }

        #endregion

        #region Parsing

        [SecuritySafeCritical]
        private void MergeFromInternal(ref ParseContext ctx)
        {
            uint tag;
            while ((tag = ctx.ReadTag()) != 0)
            {
                var fieldNumber = WireFormat.GetTagFieldNumber(tag);
                var field = descriptor.FindFieldByNumber(fieldNumber);

                if (field == null)
                {
                    unknownFields = UnknownFieldSet.MergeFieldFrom(unknownFields, ref ctx);
                    continue;
                }

                var wireType = WireFormat.GetTagWireType(tag);

                if (field.IsMap)
                {
                    ReadMapEntry(ref ctx, field);
                    // Track extension field descriptors
                    if (field.IsExtension)
                    {
                        extensionFieldDescriptors[fieldNumber] = field;
                    }
                }
                else if (field.IsRepeated)
                {
                    if (wireType == WireFormat.WireType.LengthDelimited && CanBePacked(field))
                    {
                        ReadPackedRepeatedField(ref ctx, field);
                    }
                    else
                    {
                        var value = ReadFieldValue(ref ctx, field, wireType);
                        GetOrCreateList(field).Add(value);
                    }
                    // Track extension field descriptors
                    if (field.IsExtension)
                    {
                        extensionFieldDescriptors[fieldNumber] = field;
                    }
                }
                else
                {
                    var value = ReadFieldValue(ref ctx, field, wireType);
                    SetFieldInternal(field, value);
                    // Track extension field descriptors
                    if (field.IsExtension)
                    {
                        extensionFieldDescriptors[fieldNumber] = field;
                    }
                }
            }
        }

        private void SetFieldInternal(FieldDescriptor field, object value)
        {
            var oneof = field.RealContainingOneof;
            if (oneof != null)
            {
                var currentCase = oneofCases[oneof.Index];
                if (currentCase != null && currentCase != field)
                {
                    fields.Remove(currentCase.FieldNumber);
                }
                oneofCases[oneof.Index] = field;
            }
            fields[field.FieldNumber] = value;
        }

        private void ReadMapEntry(ref ParseContext ctx, FieldDescriptor field)
        {
            var mapFields = field.MessageType.Fields;
            var keyField = mapFields.InDeclarationOrder()[0];
            var valueField = mapFields.InDeclarationOrder()[1];

            object key = GetDefaultValue(keyField);
            object value = GetDefaultValue(valueField);

            var length = ctx.ReadLength();
            var oldLimit = SegmentedBufferHelper.PushLimit(ref ctx.state, length);

            uint tag;
            while ((tag = ctx.ReadTag()) != 0)
            {
                var wireType = WireFormat.GetTagWireType(tag);
                var tagFieldNumber = WireFormat.GetTagFieldNumber(tag);

                if (tagFieldNumber == keyField.FieldNumber)
                {
                    key = ReadFieldValue(ref ctx, keyField, wireType);
                }
                else if (tagFieldNumber == valueField.FieldNumber)
                {
                    value = ReadFieldValue(ref ctx, valueField, wireType);
                }
                else
                {
                    // Skip unknown fields in map entry using existing utility
                    ParsingPrimitivesMessages.SkipLastField(ref ctx.buffer, ref ctx.state);
                }
            }

            SegmentedBufferHelper.PopLimit(ref ctx.state, oldLimit);

            GetOrCreateMap(field)[key] = value;
        }

        private void ReadPackedRepeatedField(ref ParseContext ctx, FieldDescriptor field)
        {
            var length = ctx.ReadLength();
            var oldLimit = SegmentedBufferHelper.PushLimit(ref ctx.state, length);

            var list = GetOrCreateList(field);
            while (!SegmentedBufferHelper.IsReachedLimit(ref ctx.state))
            {
                var value = ReadFieldValue(ref ctx, field, GetWireType(field));
                list.Add(value);
            }

            SegmentedBufferHelper.PopLimit(ref ctx.state, oldLimit);
        }

        private object ReadFieldValue(
            ref ParseContext ctx,
            FieldDescriptor field,
            WireFormat.WireType wireType
        )
        {
            return field.FieldType switch
            {
                FieldType.Double => ctx.ReadDouble(),
                FieldType.Float => ctx.ReadFloat(),
                FieldType.Int64 => ctx.ReadInt64(),
                FieldType.UInt64 => ctx.ReadUInt64(),
                FieldType.Int32 => ctx.ReadInt32(),
                FieldType.Fixed64 => ctx.ReadFixed64(),
                FieldType.Fixed32 => ctx.ReadFixed32(),
                FieldType.Bool => ctx.ReadBool(),
                FieldType.String => ctx.ReadString(),
                FieldType.Group => ReadGroup(ref ctx, field),
                FieldType.Message => ReadMessage(ref ctx, field),
                FieldType.Bytes => ctx.ReadBytes(),
                FieldType.UInt32 => ctx.ReadUInt32(),
                FieldType.SFixed32 => ctx.ReadSFixed32(),
                FieldType.SFixed64 => ctx.ReadSFixed64(),
                FieldType.SInt32 => ctx.ReadSInt32(),
                FieldType.SInt64 => ctx.ReadSInt64(),
                FieldType.Enum => ctx.ReadEnum(),
                _ => throw new InvalidOperationException($"Unknown field type: {field.FieldType}"),
            };
        }

        private DynamicMessage ReadMessage(ref ParseContext ctx, FieldDescriptor field)
        {
            var message = new DynamicMessage(field.MessageType);
            ctx.ReadMessage(message);
            return message;
        }

        private DynamicMessage ReadGroup(ref ParseContext ctx, FieldDescriptor field)
        {
            var message = new DynamicMessage(field.MessageType);
            ctx.ReadGroup(message);
            return message;
        }

        #endregion

        #region Writing helpers

        private static void WriteSingularField(
            ref WriteContext ctx,
            FieldDescriptor field,
            object value
        )
        {
            ctx.WriteTag(field.FieldNumber, GetWireType(field));
            WriteFieldValue(ref ctx, field, value);
        }

        private static void WriteRepeatedField(
            ref WriteContext ctx,
            FieldDescriptor field,
            IList values
        )
        {
            if (values.Count == 0)
                return;

            // Only primitive types can be packed; messages, bytes, and strings cannot
            if (field.IsPacked && CanBePacked(field))
            {
                ctx.WriteTag(field.FieldNumber, WireFormat.WireType.LengthDelimited);
                int dataSize = 0;
                foreach (var value in values)
                {
                    dataSize += ComputeFieldValueSize(field, value);
                }
                ctx.WriteLength(dataSize);
                foreach (var value in values)
                {
                    WriteFieldValue(ref ctx, field, value);
                }
            }
            else
            {
                var wireType = GetWireType(field);
                foreach (var value in values)
                {
                    ctx.WriteTag(field.FieldNumber, wireType);
                    WriteFieldValue(ref ctx, field, value);
                    if (field.FieldType == FieldType.Group)
                    {
                        ctx.WriteTag(field.FieldNumber, WireFormat.WireType.EndGroup);
                    }
                }
            }
        }

        private static void WriteMapField(
            ref WriteContext ctx,
            FieldDescriptor field,
            IDictionary map
        )
        {
            if (map.Count == 0)
                return;

            var mapFields = field.MessageType.Fields;
            var keyField = mapFields.InDeclarationOrder()[0];
            var valueField = mapFields.InDeclarationOrder()[1];

            foreach (DictionaryEntry entry in map)
            {
                ctx.WriteTag(field.FieldNumber, WireFormat.WireType.LengthDelimited);

                int entrySize = 0;
                entrySize +=
                    CodedOutputStream.ComputeTagSize(keyField.FieldNumber)
                    + ComputeFieldValueSize(keyField, entry.Key);
                entrySize +=
                    CodedOutputStream.ComputeTagSize(valueField.FieldNumber)
                    + ComputeFieldValueSize(valueField, entry.Value);

                ctx.WriteLength(entrySize);

                ctx.WriteTag(keyField.FieldNumber, GetWireType(keyField));
                WriteFieldValue(ref ctx, keyField, entry.Key);

                ctx.WriteTag(valueField.FieldNumber, GetWireType(valueField));
                WriteFieldValue(ref ctx, valueField, entry.Value);
            }
        }

        private static void WriteFieldValue(
            ref WriteContext ctx,
            FieldDescriptor field,
            object value
        )
        {
            switch (field.FieldType)
            {
                case FieldType.Double:
                    ctx.WriteDouble((double)value);
                    break;
                case FieldType.Float:
                    ctx.WriteFloat((float)value);
                    break;
                case FieldType.Int64:
                    ctx.WriteInt64((long)value);
                    break;
                case FieldType.UInt64:
                    ctx.WriteUInt64((ulong)value);
                    break;
                case FieldType.Int32:
                    ctx.WriteInt32((int)value);
                    break;
                case FieldType.Fixed64:
                    ctx.WriteFixed64((ulong)value);
                    break;
                case FieldType.Fixed32:
                    ctx.WriteFixed32((uint)value);
                    break;
                case FieldType.Bool:
                    ctx.WriteBool((bool)value);
                    break;
                case FieldType.String:
                    ctx.WriteString((string)value);
                    break;
                case FieldType.Group:
                    ctx.WriteGroup((IMessage)value);
                    break;
                case FieldType.Message:
                    ctx.WriteMessage((IMessage)value);
                    break;
                case FieldType.Bytes:
                    ctx.WriteBytes((ByteString)value);
                    break;
                case FieldType.UInt32:
                    ctx.WriteUInt32((uint)value);
                    break;
                case FieldType.SFixed32:
                    ctx.WriteSFixed32((int)value);
                    break;
                case FieldType.SFixed64:
                    ctx.WriteSFixed64((long)value);
                    break;
                case FieldType.SInt32:
                    ctx.WriteSInt32((int)value);
                    break;
                case FieldType.SInt64:
                    ctx.WriteSInt64((long)value);
                    break;
                case FieldType.Enum:
                    ctx.WriteEnum((int)value);
                    break;
                default:
                    throw new InvalidOperationException($"Unknown field type: {field.FieldType}");
            }
        }

        private static WireFormat.WireType GetWireType(FieldDescriptor field)
        {
            return field.FieldType switch
            {
                FieldType.Double => WireFormat.WireType.Fixed64,
                FieldType.Float => WireFormat.WireType.Fixed32,
                FieldType.Int64
                or FieldType.UInt64
                or FieldType.Int32
                or FieldType.UInt32
                or FieldType.Bool
                or FieldType.Enum
                or FieldType.SInt32
                or FieldType.SInt64
                    => WireFormat.WireType.Varint,
                FieldType.Fixed64 or FieldType.SFixed64 => WireFormat.WireType.Fixed64,
                FieldType.Fixed32 or FieldType.SFixed32 => WireFormat.WireType.Fixed32,
                FieldType.String
                or FieldType.Bytes
                or FieldType.Message
                    => WireFormat.WireType.LengthDelimited,
                FieldType.Group => WireFormat.WireType.StartGroup,
                _ => throw new InvalidOperationException($"Unknown field type: {field.FieldType}"),
            };
        }

        #endregion

        #region Size computation helpers

        private static int ComputeSingularFieldSize(FieldDescriptor field, object value)
        {
            int size = CodedOutputStream.ComputeTagSize(field.FieldNumber);
            size += ComputeFieldValueSize(field, value);
            if (field.FieldType == FieldType.Group)
            {
                size += CodedOutputStream.ComputeTagSize(field.FieldNumber);
            }
            return size;
        }

        private static int ComputeRepeatedFieldSize(FieldDescriptor field, IList values)
        {
            if (values.Count == 0)
                return 0;

            int size = 0;
            // Only primitive types can be packed; messages, bytes, and strings cannot
            if (field.IsPacked && CanBePacked(field))
            {
                int dataSize = 0;
                foreach (var value in values)
                {
                    dataSize += ComputeFieldValueSize(field, value);
                }
                size += CodedOutputStream.ComputeTagSize(field.FieldNumber);
                size += CodedOutputStream.ComputeLengthSize(dataSize);
                size += dataSize;
            }
            else
            {
                foreach (var value in values)
                {
                    size += ComputeSingularFieldSize(field, value);
                }
            }
            return size;
        }

        private static int ComputeMapFieldSize(FieldDescriptor field, IDictionary map)
        {
            if (map.Count == 0)
                return 0;

            var mapFields = field.MessageType.Fields;
            var keyField = mapFields.InDeclarationOrder()[0];
            var valueField = mapFields.InDeclarationOrder()[1];

            int size = 0;
            foreach (DictionaryEntry entry in map)
            {
                int entrySize = 0;
                entrySize +=
                    CodedOutputStream.ComputeTagSize(keyField.FieldNumber)
                    + ComputeFieldValueSize(keyField, entry.Key);
                entrySize +=
                    CodedOutputStream.ComputeTagSize(valueField.FieldNumber)
                    + ComputeFieldValueSize(valueField, entry.Value);

                size += CodedOutputStream.ComputeTagSize(field.FieldNumber);
                size += CodedOutputStream.ComputeLengthSize(entrySize);
                size += entrySize;
            }
            return size;
        }

        private static int ComputeFieldValueSize(FieldDescriptor field, object value)
        {
            return field.FieldType switch
            {
                FieldType.Double => CodedOutputStream.ComputeDoubleSize((double)value),
                FieldType.Float => CodedOutputStream.ComputeFloatSize((float)value),
                FieldType.Int64 => CodedOutputStream.ComputeInt64Size((long)value),
                FieldType.UInt64 => CodedOutputStream.ComputeUInt64Size((ulong)value),
                FieldType.Int32 => CodedOutputStream.ComputeInt32Size((int)value),
                FieldType.Fixed64 => CodedOutputStream.ComputeFixed64Size((ulong)value),
                FieldType.Fixed32 => CodedOutputStream.ComputeFixed32Size((uint)value),
                FieldType.Bool => CodedOutputStream.ComputeBoolSize((bool)value),
                FieldType.String => CodedOutputStream.ComputeStringSize((string)value),
                FieldType.Group => CodedOutputStream.ComputeGroupSize((IMessage)value),
                FieldType.Message => CodedOutputStream.ComputeMessageSize((IMessage)value),
                FieldType.Bytes => CodedOutputStream.ComputeBytesSize((ByteString)value),
                FieldType.UInt32 => CodedOutputStream.ComputeUInt32Size((uint)value),
                FieldType.SFixed32 => CodedOutputStream.ComputeSFixed32Size((int)value),
                FieldType.SFixed64 => CodedOutputStream.ComputeSFixed64Size((long)value),
                FieldType.SInt32 => CodedOutputStream.ComputeSInt32Size((int)value),
                FieldType.SInt64 => CodedOutputStream.ComputeSInt64Size((long)value),
                FieldType.Enum => CodedOutputStream.ComputeEnumSize((int)value),
                _ => throw new InvalidOperationException($"Unknown field type: {field.FieldType}"),
            };
        }

        #endregion
    }
}
