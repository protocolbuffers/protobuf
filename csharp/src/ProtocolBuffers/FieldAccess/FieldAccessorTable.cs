#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
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
using System.Collections.ObjectModel;
using Google.Protobuf.Descriptors;

namespace Google.Protobuf.FieldAccess
{
    /// <summary>
    /// Provides access to fields in generated messages via reflection.
    /// </summary>
    public sealed class FieldAccessorTable
    {
        private readonly ReadOnlyCollection<IFieldAccessor> accessors;
        private readonly ReadOnlyCollection<OneofAccessor> oneofs;
        private readonly MessageDescriptor descriptor;

        /// <summary>
        /// Constructs a FieldAccessorTable for a particular message class.
        /// Only one FieldAccessorTable should be constructed per class.
        /// </summary>
        /// <param name="type">The CLR type for the message.</param>
        /// <param name="descriptor">The type's descriptor</param>
        /// <param name="propertyNames">The Pascal-case names of all the field-based properties in the message.</param>
        public FieldAccessorTable(Type type, MessageDescriptor descriptor, string[] propertyNames, string[] oneofPropertyNames)
        {
            this.descriptor = descriptor;
            var accessorsArray = new IFieldAccessor[descriptor.Fields.Count];
            for (int i = 0; i < accessorsArray.Length; i++)
            {
                var field = descriptor.Fields[i];
                var name = propertyNames[i];
                accessorsArray[i] =
                    field.IsMap ? new MapFieldAccessor(type, name, field)
                    : field.IsRepeated ? new RepeatedFieldAccessor(type, name, field)
                    : (IFieldAccessor) new SingleFieldAccessor(type, name, field);
            }
            accessors = new ReadOnlyCollection<IFieldAccessor>(accessorsArray);
            var oneofsArray = new OneofAccessor[descriptor.Oneofs.Count];
            for (int i = 0; i < oneofsArray.Length; i++)
            {
                var oneof = descriptor.Oneofs[i];
                oneofsArray[i] = new OneofAccessor(type, oneofPropertyNames[i], oneof);
            }
            oneofs = new ReadOnlyCollection<OneofAccessor>(oneofsArray);
        }

        // TODO: Validate the name here... should possibly make this type a more "general reflection access" type,
        // bearing in mind the oneof parts to come as well.
        /// <summary>
        /// Returns all of the field accessors for the message type.
        /// </summary>
        public ReadOnlyCollection<IFieldAccessor> Accessors { get { return accessors; } }

        public ReadOnlyCollection<OneofAccessor> Oneofs { get { return oneofs; } }

        // TODO: Review this, as it's easy to get confused between FieldNumber and Index.
        // Currently only used to get an accessor related to a oneof... maybe just make that simpler?
        public IFieldAccessor this[int fieldNumber]
        {
            get
            {
                FieldDescriptor field = descriptor.FindFieldByNumber(fieldNumber);
                return accessors[field.Index];
            }
        }
    }
}