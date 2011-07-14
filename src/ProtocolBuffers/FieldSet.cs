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
using System.Collections.Generic;
using Google.ProtocolBuffers.Collections;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers
{
    public interface IFieldDescriptorLite : IComparable<IFieldDescriptorLite>
    {
        bool IsRepeated { get; }
        bool IsRequired { get; }
        bool IsPacked { get; }
        bool IsExtension { get; }
        bool MessageSetWireFormat { get; } //field.ContainingType.Options.MessageSetWireFormat
        int FieldNumber { get; }
        string Name { get; }
        string FullName { get; }
        IEnumLiteMap EnumType { get; }
        FieldType FieldType { get; }
        MappedType MappedType { get; }
        object DefaultValue { get; }
    }

    /// <summary>
    /// A class which represents an arbitrary set of fields of some message type.
    /// This is used to implement DynamicMessage, and also to represent extensions
    /// in GeneratedMessage. This class is internal, since outside users should probably
    /// be using DynamicMessage.
    /// 
    /// As in the Java implementation, this class goes against the rest of the framework
    /// in terms of mutability. Instead of having a mutable Builder class and an immutable
    /// FieldSet class, FieldSet just has a MakeImmutable() method. This is safe so long as
    /// all callers are careful not to let a mutable FieldSet escape into the open. This would
    /// be impossible to guarantee if this were a public class, of course.
    /// 
    /// All repeated fields are stored as IList[object] even 
    /// TODO(jonskeet): Finish this comment!
    /// </summary>
    internal sealed class FieldSet
    {
        private static readonly FieldSet defaultInstance =
            new FieldSet(new Dictionary<IFieldDescriptorLite, object>()).MakeImmutable();

        private IDictionary<IFieldDescriptorLite, object> fields;

        private FieldSet(IDictionary<IFieldDescriptorLite, object> fields)
        {
            this.fields = fields;
        }

        public static FieldSet CreateInstance()
        {
            // Use SortedList to keep fields in the canonical order
            return new FieldSet(new SortedList<IFieldDescriptorLite, object>());
        }

        /// <summary>
        /// Makes this FieldSet immutable, and returns it for convenience. Any
        /// mutable repeated fields are made immutable, as well as the map itself.
        /// </summary>
        internal FieldSet MakeImmutable()
        {
            // First check if we have any repeated values
            bool hasRepeats = false;
            foreach (object value in fields.Values)
            {
                IList<object> list = value as IList<object>;
                if (list != null && !list.IsReadOnly)
                {
                    hasRepeats = true;
                    break;
                }
            }

            if (hasRepeats)
            {
                var tmp = new SortedList<IFieldDescriptorLite, object>();
                foreach (KeyValuePair<IFieldDescriptorLite, object> entry in fields)
                {
                    IList<object> list = entry.Value as IList<object>;
                    tmp[entry.Key] = list == null ? entry.Value : Lists.AsReadOnly(list);
                }
                fields = tmp;
            }

            fields = Dictionaries.AsReadOnly(fields);

            return this;
        }

        /// <summary>
        /// Returns the default, immutable instance with no fields defined.
        /// </summary>
        internal static FieldSet DefaultInstance
        {
            get { return defaultInstance; }
        }

        /// <summary>
        /// Returns an immutable mapping of fields. Note that although the mapping itself
        /// is immutable, the entries may not be (i.e. any repeated values are represented by
        /// mutable lists). The behaviour is not specified if the contents are mutated.
        /// </summary>
        internal IDictionary<IFieldDescriptorLite, object> AllFields
        {
            get { return Dictionaries.AsReadOnly(fields); }
        }

#if !LITE
        /// <summary>
        /// Force coercion to full descriptor dictionary.
        /// </summary>
        internal IDictionary<FieldDescriptor, object> AllFieldDescriptors
        {
            get
            {
                SortedList<FieldDescriptor, object> copy =
                    new SortedList<FieldDescriptor, object>();
                foreach (KeyValuePair<IFieldDescriptorLite, object> fd in fields)
                {
                    copy.Add((FieldDescriptor) fd.Key, fd.Value);
                }
                return Dictionaries.AsReadOnly(copy);
            }
        }
#endif

        /// <summary>
        /// See <see cref="IMessageLite.HasField"/>.
        /// </summary>
        public bool HasField(IFieldDescriptorLite field)
        {
            if (field.IsRepeated)
            {
                throw new ArgumentException("HasField() can only be called on non-repeated fields.");
            }

            return fields.ContainsKey(field);
        }

        /// <summary>
        /// Clears all fields.
        /// </summary>
        internal void Clear()
        {
            fields.Clear();
        }

        /// <summary>
        /// See <see cref="IMessageLite.Item(IFieldDescriptorLite)"/>
        /// </summary>
        /// <remarks>
        /// If the field is not set, the behaviour when fetching this property varies by field type:
        /// <list>
        /// <item>For singular message values, null is returned.</item>
        /// <item>For singular non-message values, the default value of the field is returned.</item>
        /// <item>For repeated values, an empty immutable list is returned. This will be compatible
        /// with IList[object], regardless of the type of the repeated item.</item>
        /// </list>
        /// This method returns null if the field is a singular message type
        /// and is not set; in this case it is up to the caller to fetch the 
        /// message's default instance. For repeated fields of message types, 
        /// an empty collection is returned. For repeated fields of non-message
        /// types, null is returned.
        /// <para />
        /// When setting this property, any list values are copied, and each element is checked
        /// to ensure it is of an appropriate type.
        /// </remarks>
        /// 
        internal object this[IFieldDescriptorLite field]
        {
            get
            {
                object result;
                if (fields.TryGetValue(field, out result))
                {
                    return result;
                }
                if (field.MappedType == MappedType.Message)
                {
                    if (field.IsRepeated)
                    {
                        return new List<object>();
                    }
                    else
                    {
                        return null;
                    }
                }
                return field.DefaultValue;
            }
            set
            {
                if (field.IsRepeated)
                {
                    List<object> list = value as List<object>;
                    if (list == null)
                    {
                        throw new ArgumentException("Wrong object type used with protocol message reflection.");
                    }

                    // Wrap the contents in a new list so that the caller cannot change
                    // the list's contents after setting it.
                    List<object> newList = new List<object>(list);
                    foreach (object element in newList)
                    {
                        VerifyType(field, element);
                    }
                    value = newList;
                }
                else
                {
                    VerifyType(field, value);
                }
                fields[field] = value;
            }
        }

        /// <summary>
        /// See <see cref="IMessageLite.Item(IFieldDescriptorLite,int)" />
        /// </summary>
        internal object this[IFieldDescriptorLite field, int index]
        {
            get
            {
                if (!field.IsRepeated)
                {
                    throw new ArgumentException(
                        "Indexer specifying field and index can only be called on repeated fields.");
                }

                return ((IList<object>) this[field])[index];
            }
            set
            {
                if (!field.IsRepeated)
                {
                    throw new ArgumentException(
                        "Indexer specifying field and index can only be called on repeated fields.");
                }
                VerifyType(field, value);
                object list;
                if (!fields.TryGetValue(field, out list))
                {
                    throw new ArgumentOutOfRangeException();
                }
                ((IList<object>) list)[index] = value;
            }
        }

        /// <summary>
        /// See <see cref="IBuilder{TMessage, TBuilder}.AddRepeatedField" />
        /// </summary>
        internal void AddRepeatedField(IFieldDescriptorLite field, object value)
        {
            if (!field.IsRepeated)
            {
                throw new ArgumentException("AddRepeatedField can only be called on repeated fields.");
            }
            VerifyType(field, value);
            object list;
            if (!fields.TryGetValue(field, out list))
            {
                list = new List<object>();
                fields[field] = list;
            }
            ((IList<object>) list).Add(value);
        }

        /// <summary>
        /// Returns an enumerator for the field map. Used to write the fields out.
        /// </summary>
        internal IEnumerator<KeyValuePair<IFieldDescriptorLite, object>> GetEnumerator()
        {
            return fields.GetEnumerator();
        }

        /// <summary>
        /// See <see cref="IMessageLite.IsInitialized" />
        /// </summary>
        /// <remarks>
        /// Since FieldSet itself does not have any way of knowing about
        /// required fields that aren't actually present in the set, it is up
        /// to the caller to check for genuinely required fields. This property
        /// merely checks that any messages present are themselves initialized.
        /// </remarks>
        internal bool IsInitialized
        {
            get
            {
                foreach (KeyValuePair<IFieldDescriptorLite, object> entry in fields)
                {
                    IFieldDescriptorLite field = entry.Key;
                    if (field.MappedType == MappedType.Message)
                    {
                        if (field.IsRepeated)
                        {
                            foreach (IMessageLite message in (IEnumerable) entry.Value)
                            {
                                if (!message.IsInitialized)
                                {
                                    return false;
                                }
                            }
                        }
                        else
                        {
                            if (!((IMessageLite) entry.Value).IsInitialized)
                            {
                                return false;
                            }
                        }
                    }
                }
                return true;
            }
        }

        /// <summary>
        /// Verifies whether all the required fields in the specified message
        /// descriptor are present in this field set, as well as whether
        /// all the embedded messages are themselves initialized.
        /// </summary>
        internal bool IsInitializedWithRespectTo(IEnumerable typeFields)
        {
            foreach (IFieldDescriptorLite field in typeFields)
            {
                if (field.IsRequired && !HasField(field))
                {
                    return false;
                }
            }
            return IsInitialized;
        }

        /// <summary>
        /// See <see cref="IBuilder{TMessage, TBuilder}.ClearField" />
        /// </summary>
        public void ClearField(IFieldDescriptorLite field)
        {
            fields.Remove(field);
        }

        /// <summary>
        /// See <see cref="IMessageLite.GetRepeatedFieldCount" />
        /// </summary>
        public int GetRepeatedFieldCount(IFieldDescriptorLite field)
        {
            if (!field.IsRepeated)
            {
                throw new ArgumentException("GetRepeatedFieldCount() can only be called on repeated fields.");
            }

            return ((IList<object>) this[field]).Count;
        }

#if !LITE
        /// <summary>
        /// See <see cref="IBuilder{TMessage, TBuilder}.MergeFrom(IMessageLite)" />
        /// </summary>
        public void MergeFrom(IMessage other)
        {
            foreach (KeyValuePair<FieldDescriptor, object> fd in other.AllFields)
            {
                MergeField(fd.Key, fd.Value);
            }
        }
#endif

        /// <summary>
        /// Implementation of both <c>MergeFrom</c> methods.
        /// </summary>
        /// <param name="otherFields"></param>
        public void MergeFrom(FieldSet other)
        {
            // Note:  We don't attempt to verify that other's fields have valid
            //   types.  Doing so would be a losing battle.  We'd have to verify
            //   all sub-messages as well, and we'd have to make copies of all of
            //   them to insure that they don't change after verification (since
            //   the IMessageLite interface itself cannot enforce immutability of
            //   implementations).
            // TODO(jonskeet):  Provide a function somewhere called MakeDeepCopy()
            //   which allows people to make secure deep copies of messages.

            foreach (KeyValuePair<IFieldDescriptorLite, object> entry in other.fields)
            {
                MergeField(entry.Key, entry.Value);
            }
        }

        private void MergeField(IFieldDescriptorLite field, object mergeValue)
        {
            object existingValue;
            fields.TryGetValue(field, out existingValue);
            if (field.IsRepeated)
            {
                if (existingValue == null)
                {
                    existingValue = new List<object>();
                    fields[field] = existingValue;
                }
                IList<object> list = (IList<object>) existingValue;
                foreach (object otherValue in (IEnumerable) mergeValue)
                {
                    list.Add(otherValue);
                }
            }
            else if (field.MappedType == MappedType.Message && existingValue != null)
            {
                IMessageLite existingMessage = (IMessageLite) existingValue;
                IMessageLite merged = existingMessage.WeakToBuilder()
                    .WeakMergeFrom((IMessageLite) mergeValue)
                    .WeakBuild();
                this[field] = merged;
            }
            else
            {
                this[field] = mergeValue;
            }
        }

        /// <summary>
        /// See <see cref="IMessageLite.WriteTo(CodedOutputStream)" />.
        /// </summary>
        public void WriteTo(ICodedOutputStream output)
        {
            foreach (KeyValuePair<IFieldDescriptorLite, object> entry in fields)
            {
                WriteField(entry.Key, entry.Value, output);
            }
        }

        /// <summary>
        /// Writes a single field to a CodedOutputStream.
        /// </summary>
        public void WriteField(IFieldDescriptorLite field, Object value, ICodedOutputStream output)
        {
            if (field.IsExtension && field.MessageSetWireFormat)
            {
                output.WriteMessageSetExtension(field.FieldNumber, field.Name, (IMessageLite) value);
            }
            else
            {
                if (field.IsRepeated)
                {
                    IEnumerable valueList = (IEnumerable) value;
                    if (field.IsPacked)
                    {
                        output.WritePackedArray(field.FieldType, field.FieldNumber, field.Name, valueList);
                    }
                    else
                    {
                        output.WriteArray(field.FieldType, field.FieldNumber, field.Name, valueList);
                    }
                }
                else
                {
                    output.WriteField(field.FieldType, field.FieldNumber, field.Name, value);
                }
            }
        }

        /// <summary>
        /// See <see cref="IMessageLite.SerializedSize" />. It's up to the caller to
        /// cache the resulting size if desired.
        /// </summary>
        public int SerializedSize
        {
            get
            {
                int size = 0;
                foreach (KeyValuePair<IFieldDescriptorLite, object> entry in fields)
                {
                    IFieldDescriptorLite field = entry.Key;
                    object value = entry.Value;

                    if (field.IsExtension && field.MessageSetWireFormat)
                    {
                        size += CodedOutputStream.ComputeMessageSetExtensionSize(field.FieldNumber, (IMessageLite) value);
                    }
                    else
                    {
                        if (field.IsRepeated)
                        {
                            IEnumerable valueList = (IEnumerable) value;
                            if (field.IsPacked)
                            {
                                int dataSize = 0;
                                foreach (object element in valueList)
                                {
                                    dataSize += CodedOutputStream.ComputeFieldSizeNoTag(field.FieldType, element);
                                }
                                size += dataSize + CodedOutputStream.ComputeTagSize(field.FieldNumber) +
                                        CodedOutputStream.ComputeRawVarint32Size((uint) dataSize);
                            }
                            else
                            {
                                foreach (object element in valueList)
                                {
                                    size += CodedOutputStream.ComputeFieldSize(field.FieldType, field.FieldNumber,
                                                                               element);
                                }
                            }
                        }
                        else
                        {
                            size += CodedOutputStream.ComputeFieldSize(field.FieldType, field.FieldNumber, value);
                        }
                    }
                }
                return size;
            }
        }

        /// <summary>
        /// Verifies that the given object is of the correct type to be a valid
        /// value for the given field.
        /// </summary>
        /// <remarks>
        /// For repeated fields, this checks if the object is of the right
        /// element type, not whether it's a list.
        /// </remarks>
        /// <exception cref="ArgumentException">The value is not of the right type.</exception>
        /// <exception cref="ArgumentNullException">The value is null.</exception>
        private static void VerifyType(IFieldDescriptorLite field, object value)
        {
            ThrowHelper.ThrowIfNull(value, "value");
            bool isValid = false;
            switch (field.MappedType)
            {
                case MappedType.Int32:
                    isValid = value is int;
                    break;
                case MappedType.Int64:
                    isValid = value is long;
                    break;
                case MappedType.UInt32:
                    isValid = value is uint;
                    break;
                case MappedType.UInt64:
                    isValid = value is ulong;
                    break;
                case MappedType.Single:
                    isValid = value is float;
                    break;
                case MappedType.Double:
                    isValid = value is double;
                    break;
                case MappedType.Boolean:
                    isValid = value is bool;
                    break;
                case MappedType.String:
                    isValid = value is string;
                    break;
                case MappedType.ByteString:
                    isValid = value is ByteString;
                    break;
                case MappedType.Enum:
                    IEnumLite enumValue = value as IEnumLite;
                    isValid = enumValue != null && field.EnumType.IsValidValue(enumValue);
                    break;
                case MappedType.Message:
                    IMessageLite messageValue = value as IMessageLite;
                    isValid = messageValue != null;
#if !LITE
                    if (isValid && messageValue is IMessage && field is FieldDescriptor)
                    {
                        isValid = ((IMessage) messageValue).DescriptorForType == ((FieldDescriptor) field).MessageType;
                    }
#endif
                    break;
            }

            if (!isValid)
            {
                // When chaining calls to SetField(), it can be hard to tell from
                // the stack trace which exact call failed, since the whole chain is
                // considered one line of code.  So, let's make sure to include the
                // field name and other useful info in the exception.
                string message = "Wrong object type used with protocol message reflection.";
#if !LITE
                FieldDescriptor fieldinfo =
                    field as FieldDescriptor;
                if (fieldinfo != null)
                {
                    message += "Message type \"" + fieldinfo.ContainingType.FullName;
                    message += "\", field \"" + (fieldinfo.IsExtension ? fieldinfo.FullName : fieldinfo.Name);
                    message += "\", value was type \"" + value.GetType().Name + "\".";
                }
#endif
                throw new ArgumentException(message);
            }
        }
    }
}