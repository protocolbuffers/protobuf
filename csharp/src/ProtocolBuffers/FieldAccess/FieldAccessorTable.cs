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
using Google.Protobuf.Descriptors;

namespace Google.Protobuf.FieldAccess
{
    /// <summary>
    /// Provides access to fields in generated messages via reflection.
    /// </summary>
    public sealed class FieldAccessorTable<T> where T : IMessage<T>
    {
        private readonly IFieldAccessor<T>[] accessors;
        private readonly MessageDescriptor descriptor;

        /// <summary>
        /// Constructs a FieldAccessorTable for a particular message class.
        /// Only one FieldAccessorTable should be constructed per class.
        /// </summary>
        /// <param name="descriptor">The type's descriptor</param>
        /// <param name="propertyNames">The Pascal-case names of all the field-based properties in the message.</param>
        public FieldAccessorTable(MessageDescriptor descriptor, string[] propertyNames)
        {
            this.descriptor = descriptor;
            accessors = new IFieldAccessor<T>[descriptor.Fields.Count];
            bool supportFieldPresence = descriptor.File.Syntax == FileDescriptor.ProtoSyntax.Proto2;
            for (int i = 0; i < accessors.Length; i++)
            {
                var field = descriptor.Fields[i];
                var name = propertyNames[i];
                accessors[i] = field.IsRepeated
                    ? (IFieldAccessor<T>) new RepeatedFieldAccessor<T>(propertyNames[i])
                    : new SingleFieldAccessor<T>(field, name, supportFieldPresence);
            }
            // TODO(jonskeet): Oneof support
        }

        internal IFieldAccessor<T> this[int fieldNumber]
        {
            get
            {
                FieldDescriptor field = descriptor.FindFieldByNumber(fieldNumber);
                // TODO: Handle extensions.
                return accessors[field.Index];
            }
        }

        internal IFieldAccessor<T> this[FieldDescriptor field]
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