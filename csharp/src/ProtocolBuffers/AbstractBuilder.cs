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

namespace Google.ProtocolBuffers
{
    /// <summary>
    /// Implementation of the non-generic IMessage interface as far as possible.
    /// </summary>
    public abstract partial class AbstractBuilder<TMessage, TBuilder> : AbstractBuilderLite<TMessage, TBuilder>,
                                                                IBuilder<TMessage, TBuilder>
        where TMessage : AbstractMessage<TMessage, TBuilder>
        where TBuilder : AbstractBuilder<TMessage, TBuilder>
    {
        #region Unimplemented members of IBuilder

        public abstract UnknownFieldSet UnknownFields { get; set; }
        public abstract IDictionary<FieldDescriptor, object> AllFields { get; }
        public abstract object this[FieldDescriptor field] { get; set; }
        public abstract MessageDescriptor DescriptorForType { get; }
        public abstract int GetRepeatedFieldCount(FieldDescriptor field);
        public abstract object this[FieldDescriptor field, int index] { get; set; }
        public abstract bool HasField(FieldDescriptor field);
        public abstract IBuilder CreateBuilderForField(FieldDescriptor field);
        public abstract TBuilder ClearField(FieldDescriptor field);
        public abstract TBuilder AddRepeatedField(FieldDescriptor field, object value);

        #endregion

        public TBuilder SetUnknownFields(UnknownFieldSet fields)
        {
            UnknownFields = fields;
            return ThisBuilder;
        }

        public override TBuilder Clear()
        {
            foreach (FieldDescriptor field in AllFields.Keys)
            {
                ClearField(field);
            }
            return ThisBuilder;
        }

        public override sealed TBuilder MergeFrom(IMessageLite other)
        {
            if (other is IMessage)
            {
                return MergeFrom((IMessage) other);
            }
            throw new ArgumentException("MergeFrom(Message) can only merge messages of the same type.");
        }

        /// <summary>
        /// Merge the specified other message into the message being
        /// built. Merging occurs as follows. For each field:
        /// For singular primitive fields, if the field is set in <paramref name="other"/>,
        /// then <paramref name="other"/>'s value overwrites the value in this message.
        /// For singular message fields, if the field is set in <paramref name="other"/>,
        /// it is merged into the corresponding sub-message of this message using the same
        /// merging rules.
        /// For repeated fields, the elements in <paramref name="other"/> are concatenated
        /// with the elements in this message.
        /// </summary>
        /// <param name="other"></param>
        /// <returns></returns>
        public abstract TBuilder MergeFrom(TMessage other);

        public virtual TBuilder MergeFrom(IMessage other)
        {
            if (other.DescriptorForType != DescriptorForType)
            {
                throw new ArgumentException("MergeFrom(IMessage) can only merge messages of the same type.");
            }

            // Note:  We don't attempt to verify that other's fields have valid
            //   types.  Doing so would be a losing battle.  We'd have to verify
            //   all sub-messages as well, and we'd have to make copies of all of
            //   them to insure that they don't change after verification (since
            //   the Message interface itself cannot enforce immutability of
            //   implementations).
            // TODO(jonskeet):  Provide a function somewhere called MakeDeepCopy()
            //   which allows people to make secure deep copies of messages.
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
                else if (field.MappedType == MappedType.Message)
                {
                    // Merge singular messages
                    IMessageLite existingValue = (IMessageLite) this[field];
                    if (existingValue == existingValue.WeakDefaultInstanceForType)
                    {
                        this[field] = entry.Value;
                    }
                    else
                    {
                        this[field] = existingValue.WeakCreateBuilderForType()
                            .WeakMergeFrom(existingValue)
                            .WeakMergeFrom((IMessageLite) entry.Value)
                            .WeakBuild();
                    }
                }
                else
                {
                    // Overwrite simple values
                    this[field] = entry.Value;
                }
            }

            //Fix for unknown fields not merging, see java's AbstractMessage.Builder<T> line 236
            MergeUnknownFields(other.UnknownFields);

            return ThisBuilder;
        }

        public override TBuilder MergeFrom(ICodedInputStream input, ExtensionRegistry extensionRegistry)
        {
            UnknownFieldSet.Builder unknownFields = UnknownFieldSet.CreateBuilder(UnknownFields);
            unknownFields.MergeFrom(input, extensionRegistry, this);
            UnknownFields = unknownFields.Build();
            return ThisBuilder;
        }

        public virtual TBuilder MergeUnknownFields(UnknownFieldSet unknownFields)
        {
            UnknownFields = UnknownFieldSet.CreateBuilder(UnknownFields)
                .MergeFrom(unknownFields)
                .Build();
            return ThisBuilder;
        }

        public virtual IBuilder SetField(FieldDescriptor field, object value)
        {
            this[field] = value;
            return ThisBuilder;
        }

        public virtual IBuilder SetRepeatedField(FieldDescriptor field, int index, object value)
        {
            this[field, index] = value;
            return ThisBuilder;
        }

        #region Explicit Implementations

        IMessage IBuilder.WeakBuild()
        {
            return Build();
        }

        IBuilder IBuilder.WeakAddRepeatedField(FieldDescriptor field, object value)
        {
            return AddRepeatedField(field, value);
        }

        IBuilder IBuilder.WeakClear()
        {
            return Clear();
        }

        IBuilder IBuilder.WeakMergeFrom(IMessage message)
        {
            return MergeFrom(message);
        }

        IBuilder IBuilder.WeakMergeFrom(ICodedInputStream input)
        {
            return MergeFrom(input);
        }

        IBuilder IBuilder.WeakMergeFrom(ICodedInputStream input, ExtensionRegistry registry)
        {
            return MergeFrom(input, registry);
        }

        IBuilder IBuilder.WeakMergeFrom(ByteString data)
        {
            return MergeFrom(data);
        }

        IBuilder IBuilder.WeakMergeFrom(ByteString data, ExtensionRegistry registry)
        {
            return MergeFrom(data, registry);
        }

        IMessage IBuilder.WeakBuildPartial()
        {
            return BuildPartial();
        }

        IBuilder IBuilder.WeakClone()
        {
            return Clone();
        }

        IMessage IBuilder.WeakDefaultInstanceForType
        {
            get { return DefaultInstanceForType; }
        }

        IBuilder IBuilder.WeakClearField(FieldDescriptor field)
        {
            return ClearField(field);
        }

        #endregion

        /// <summary>
        /// Converts this builder to a string using <see cref="TextFormat" />.
        /// </summary>
        /// <remarks>
        /// This method is not sealed (in the way that it is in <see cref="AbstractMessage{TMessage, TBuilder}" />
        /// as it was added after earlier releases; some other implementations may already be overriding the
        /// method.
        /// </remarks>
        public override string ToString()
        {
            return TextFormat.PrintToString(this);
        }
    }
}