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
using Google.ProtocolBuffers.Descriptors;
using Google.ProtocolBuffers.FieldAccess;

namespace Google.ProtocolBuffers
{
    /// <summary>
    /// All generated protocol message builder classes extend this class. It implements
    /// most of the IBuilder interface using reflection. Users can ignore this class
    /// as an implementation detail.
    /// </summary>
    public abstract partial class GeneratedBuilder<TMessage, TBuilder> : AbstractBuilder<TMessage, TBuilder>
        where TMessage : GeneratedMessage<TMessage, TBuilder>
        where TBuilder : GeneratedBuilder<TMessage, TBuilder>, new()
    {
        /// <summary>
        /// Returns the message being built at the moment.
        /// </summary>
        protected abstract TMessage MessageBeingBuilt { get; }

        protected internal FieldAccessorTable<TMessage, TBuilder> InternalFieldAccessors
        {
            get { return DefaultInstanceForType.FieldAccessorsFromBuilder; }
        }

        public override IDictionary<FieldDescriptor, object> AllFields
        {
            get { return MessageBeingBuilt.AllFields; }
        }

        public override object this[FieldDescriptor field]
        {
            get
            {
                // For repeated fields, the underlying list object is still modifiable at this point.
                // Make sure not to expose the modifiable list to the caller.
                return field.IsRepeated
                           ? InternalFieldAccessors[field].GetRepeatedWrapper(ThisBuilder)
                           : MessageBeingBuilt[field];
            }
            set { InternalFieldAccessors[field].SetValue(ThisBuilder, value); }
        }

        /// <summary>
        /// Called by derived classes to parse an unknown field.
        /// </summary>
        /// <returns>true unless the tag is an end-group tag</returns>
        [CLSCompliant(false)]
        protected virtual bool ParseUnknownField(ICodedInputStream input, UnknownFieldSet.Builder unknownFields,
                                                 ExtensionRegistry extensionRegistry, uint tag, string fieldName)
        {
            return unknownFields.MergeFieldFrom(tag, input);
        }

        public override MessageDescriptor DescriptorForType
        {
            get { return DefaultInstanceForType.DescriptorForType; }
        }

        public override int GetRepeatedFieldCount(FieldDescriptor field)
        {
            return MessageBeingBuilt.GetRepeatedFieldCount(field);
        }

        public override object this[FieldDescriptor field, int index]
        {
            get { return MessageBeingBuilt[field, index]; }
            set { InternalFieldAccessors[field].SetRepeated(ThisBuilder, index, value); }
        }

        public override bool HasField(FieldDescriptor field)
        {
            return MessageBeingBuilt.HasField(field);
        }

        public override IBuilder CreateBuilderForField(FieldDescriptor field)
        {
            return InternalFieldAccessors[field].CreateBuilder();
        }

        public override TBuilder ClearField(FieldDescriptor field)
        {
            InternalFieldAccessors[field].Clear(ThisBuilder);
            return ThisBuilder;
        }

        public override TBuilder MergeFrom(TMessage other)
        {
            if (other.DescriptorForType != InternalFieldAccessors.Descriptor)
            {
                throw new ArgumentException("Message type mismatch");
            }

            foreach (KeyValuePair<FieldDescriptor, object> entry in other.AllFields)
            {
                FieldDescriptor field = entry.Key;
                if (field.IsRepeated)
                {
                    // Concatenate repeated fields
                    foreach (object element in (IEnumerable) entry.Value)
                    {
                        AddRepeatedField(field, element);
                    }
                }
                else if (field.MappedType == MappedType.Message && HasField(field))
                {
                    // Merge singular embedded messages
                    IMessageLite oldValue = (IMessageLite) this[field];
                    this[field] = oldValue.WeakCreateBuilderForType()
                        .WeakMergeFrom(oldValue)
                        .WeakMergeFrom((IMessageLite) entry.Value)
                        .WeakBuildPartial();
                }
                else
                {
                    // Just overwrite
                    this[field] = entry.Value;
                }
            }

            //Fix for unknown fields not merging, see java's AbstractMessage.Builder<T> line 236
            MergeUnknownFields(other.UnknownFields);

            return ThisBuilder;
        }

        public override TBuilder MergeUnknownFields(UnknownFieldSet unknownFields)
        {
            if (unknownFields != UnknownFieldSet.DefaultInstance)
            {
                TMessage result = MessageBeingBuilt;
                result.SetUnknownFields(UnknownFieldSet.CreateBuilder(result.UnknownFields)
                                            .MergeFrom(unknownFields)
                                            .Build());
            }
            return ThisBuilder;
        }

        public override TBuilder AddRepeatedField(FieldDescriptor field, object value)
        {
            InternalFieldAccessors[field].AddRepeated(ThisBuilder, value);
            return ThisBuilder;
        }

        /// <summary>
        /// Like Build(), but will wrap UninitializedMessageException in
        /// InvalidProtocolBufferException.
        /// </summary>
        public TMessage BuildParsed()
        {
            if (!IsInitialized)
            {
                throw new UninitializedMessageException(MessageBeingBuilt).AsInvalidProtocolBufferException();
            }
            return BuildPartial();
        }

        /// <summary>
        /// Implementation of <see cref="IBuilder{TMessage, TBuilder}.Build" />.
        /// </summary>
        public override TMessage Build()
        {
            // If the message is null, we'll throw a more appropriate exception in BuildPartial.
            if (!IsInitialized)
            {
                throw new UninitializedMessageException(MessageBeingBuilt);
            }
            return BuildPartial();
        }

        public override UnknownFieldSet UnknownFields
        {
            get { return MessageBeingBuilt.UnknownFields; }
            set { MessageBeingBuilt.SetUnknownFields(value); }
        }
    }
}