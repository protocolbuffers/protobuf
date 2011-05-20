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
using System;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers.FieldAccess
{
    /// <summary>
    /// Provides access to fields in generated messages via reflection.
    /// This type is public to allow it to be used by generated messages, which
    /// create appropriate instances in the .proto file description class.
    /// TODO(jonskeet): See if we can hide it somewhere...
    /// </summary>
    public sealed class FieldAccessorTable<TMessage, TBuilder>
        where TMessage : IMessage<TMessage, TBuilder>
        where TBuilder : IBuilder<TMessage, TBuilder>
    {
        private readonly IFieldAccessor<TMessage, TBuilder>[] accessors;

        private readonly MessageDescriptor descriptor;

        public MessageDescriptor Descriptor
        {
            get { return descriptor; }
        }

        /// <summary>
        /// Constructs a FieldAccessorTable for a particular message class.
        /// Only one FieldAccessorTable should be constructed per class.
        /// The property names should all actually correspond with the field descriptor's
        /// CSharpOptions.PropertyName property, but bootstrapping issues currently
        /// prevent us from using that. This may be addressed at a future time, in which case
        /// we can keep this constructor for backwards compatibility, just ignoring the parameter.
        /// TODO(jonskeet): Make it so.
        /// </summary>
        /// <param name="descriptor">The type's descriptor</param>
        /// <param name="propertyNames">The Pascal-case names of all the field-based properties in the message.</param>
        public FieldAccessorTable(MessageDescriptor descriptor, String[] propertyNames)
        {
            this.descriptor = descriptor;
            accessors = new IFieldAccessor<TMessage, TBuilder>[descriptor.Fields.Count];
            for (int i = 0; i < accessors.Length; i++)
            {
                accessors[i] = CreateAccessor(descriptor.Fields[i], propertyNames[i]);
            }
        }

        /// <summary>
        /// Creates an accessor for a single field
        /// </summary>   
        private static IFieldAccessor<TMessage, TBuilder> CreateAccessor(FieldDescriptor field, string name)
        {
            if (field.IsRepeated)
            {
                switch (field.MappedType)
                {
                    case MappedType.Message:
                        return new RepeatedMessageAccessor<TMessage, TBuilder>(name);
                    case MappedType.Enum:
                        return new RepeatedEnumAccessor<TMessage, TBuilder>(field, name);
                    default:
                        return new RepeatedPrimitiveAccessor<TMessage, TBuilder>(name);
                }
            }
            else
            {
                switch (field.MappedType)
                {
                    case MappedType.Message:
                        return new SingleMessageAccessor<TMessage, TBuilder>(name);
                    case MappedType.Enum:
                        return new SingleEnumAccessor<TMessage, TBuilder>(field, name);
                    default:
                        return new SinglePrimitiveAccessor<TMessage, TBuilder>(name);
                }
            }
        }

        internal IFieldAccessor<TMessage, TBuilder> this[FieldDescriptor field]
        {
            get
            {
                if (field.ContainingType != descriptor)
                {
                    throw new ArgumentException("FieldDescriptor does not match message type.");
                }
                else if (field.IsExtension)
                {
                    // If this type had extensions, it would subclass ExtendableMessage,
                    // which overrides the reflection interface to handle extensions.
                    throw new ArgumentException("This type does not have extensions.");
                }
                return accessors[field.Index];
            }
        }
    }
}