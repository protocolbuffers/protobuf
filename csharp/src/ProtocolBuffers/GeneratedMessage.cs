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
using System.IO;
using System.Xml;
using Google.ProtocolBuffers.Collections;
using Google.ProtocolBuffers.Descriptors;
using Google.ProtocolBuffers.FieldAccess;

namespace Google.ProtocolBuffers
{
    /// <summary>
    /// All generated protocol message classes extend this class. It implements
    /// most of the IMessage interface using reflection. Users
    /// can ignore this class as an implementation detail.
    /// </summary>
    public abstract partial class GeneratedMessage<TMessage, TBuilder> : AbstractMessage<TMessage, TBuilder>
        where TMessage : GeneratedMessage<TMessage, TBuilder>
        where TBuilder : GeneratedBuilder<TMessage, TBuilder>, new()
    {
        private UnknownFieldSet unknownFields = UnknownFieldSet.DefaultInstance;

        /// <summary>
        /// Returns the message as a TMessage.
        /// </summary>
        protected abstract TMessage ThisMessage { get; }

        internal FieldAccessorTable<TMessage, TBuilder> FieldAccessorsFromBuilder
        {
            get { return InternalFieldAccessors; }
        }

        protected abstract FieldAccessorTable<TMessage, TBuilder> InternalFieldAccessors { get; }

        public override MessageDescriptor DescriptorForType
        {
            get { return InternalFieldAccessors.Descriptor; }
        }

        internal IDictionary<FieldDescriptor, Object> GetMutableFieldMap()
        {
            // Use a SortedDictionary so we'll end up serializing fields in order
            var ret = new SortedDictionary<FieldDescriptor, object>();
            MessageDescriptor descriptor = DescriptorForType;
            foreach (FieldDescriptor field in descriptor.Fields)
            {
                IFieldAccessor<TMessage, TBuilder> accessor = InternalFieldAccessors[field];
                if (field.IsRepeated)
                {
                    if (accessor.GetRepeatedCount(ThisMessage) != 0)
                    {
                        ret[field] = accessor.GetValue(ThisMessage);
                    }
                }
                else if (HasField(field))
                {
                    ret[field] = accessor.GetValue(ThisMessage);
                }
            }
            return ret;
        }

        public override bool IsInitialized
        {
            get
            {
                foreach (FieldDescriptor field in DescriptorForType.Fields)
                {
                    // Check that all required fields are present.
                    if (field.IsRequired && !HasField(field))
                    {
                        return false;
                    }
                    // Check that embedded messages are initialized.
                    // This code is similar to that in AbstractMessage, but we don't
                    // fetch all the field values - just the ones we need to.
                    if (field.MappedType == MappedType.Message)
                    {
                        if (field.IsRepeated)
                        {
                            // We know it's an IList<T>, but not the exact type - so
                            // IEnumerable is the best we can do. (C# generics aren't covariant yet.)
                            foreach (IMessageLite element in (IEnumerable) this[field])
                            {
                                if (!element.IsInitialized)
                                {
                                    return false;
                                }
                            }
                        }
                        else
                        {
                            if (HasField(field) && !((IMessageLite) this[field]).IsInitialized)
                            {
                                return false;
                            }
                        }
                    }
                }
                return true;
            }
        }

        public override IDictionary<FieldDescriptor, object> AllFields
        {
            get { return Dictionaries.AsReadOnly(GetMutableFieldMap()); }
        }

        public override bool HasField(FieldDescriptor field)
        {
            return InternalFieldAccessors[field].Has(ThisMessage);
        }

        public override int GetRepeatedFieldCount(FieldDescriptor field)
        {
            return InternalFieldAccessors[field].GetRepeatedCount(ThisMessage);
        }

        public override object this[FieldDescriptor field, int index]
        {
            get { return InternalFieldAccessors[field].GetRepeatedValue(ThisMessage, index); }
        }

        public override object this[FieldDescriptor field]
        {
            get { return InternalFieldAccessors[field].GetValue(ThisMessage); }
        }

        public override UnknownFieldSet UnknownFields
        {
            get { return unknownFields; }
        }

        /// <summary>
        /// Replaces the set of unknown fields for this message. This should
        /// only be used before a message is built, by the builder. (In the
        /// Java code it is private, but the builder is nested so has access
        /// to it.)
        /// </summary>
        internal void SetUnknownFields(UnknownFieldSet fieldSet)
        {
            unknownFields = fieldSet;
        }
    }
}