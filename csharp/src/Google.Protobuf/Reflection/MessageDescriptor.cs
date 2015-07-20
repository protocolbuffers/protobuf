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

using Google.Protobuf.Collections;
using System;
using System.Collections.Generic;
using System.Linq;

namespace Google.Protobuf.Reflection
{
    /// <summary>
    /// Describes a message type.
    /// </summary>
    public sealed class MessageDescriptor : DescriptorBase
    {
        private static readonly HashSet<string> WellKnownTypeNames = new HashSet<string>
        {
            "google/protobuf/any.proto",
            "google/protobuf/api.proto",
            "google/protobuf/duration.proto",
            "google/protobuf/empty.proto",
            "google/protobuf/wrappers.proto",
            "google/protobuf/timestamp.proto",
            "google/protobuf/field_mask.proto",
            "google/protobuf/source_context.proto",
            "google/protobuf/struct.proto",
            "google/protobuf/type.proto",
        };

        private readonly DescriptorProto proto;
        private readonly MessageDescriptor containingType;
        private readonly IList<MessageDescriptor> nestedTypes;
        private readonly IList<EnumDescriptor> enumTypes;
        private readonly IList<FieldDescriptor> fields;
        private readonly IList<OneofDescriptor> oneofs;
        // CLR representation of the type described by this descriptor, if any.
        private readonly Type generatedType;
        private IDictionary<int, IFieldAccessor> fieldAccessorsByFieldNumber;
        
        internal MessageDescriptor(DescriptorProto proto, FileDescriptor file, MessageDescriptor parent, int typeIndex, IEnumerator<Type> generatedTypeIterator)
            : base(file, file.ComputeFullName(parent, proto.Name), typeIndex)
        {
            this.proto = proto;
            generatedType = ReflectionUtil.GetNextType(generatedTypeIterator);
            containingType = parent;

            oneofs = DescriptorUtil.ConvertAndMakeReadOnly(proto.OneofDecl,
                                                               (oneof, index) =>
                                                               new OneofDescriptor(oneof, file, this, index));

            nestedTypes = DescriptorUtil.ConvertAndMakeReadOnly(proto.NestedType,
                                                                (type, index) =>
                                                                new MessageDescriptor(type, file, this, index, generatedTypeIterator));

            enumTypes = DescriptorUtil.ConvertAndMakeReadOnly(proto.EnumType,
                                                              (type, index) =>
                                                              new EnumDescriptor(type, file, this, index, ReflectionUtil.GetNextType(generatedTypeIterator)));

            // TODO(jonskeet): Sort fields first?
            fields = DescriptorUtil.ConvertAndMakeReadOnly(proto.Field,
                                                           (field, index) =>
                                                           new FieldDescriptor(field, file, this, index));
            file.DescriptorPool.AddSymbol(this);
        }
                
        /// <summary>
        /// Returns the total number of nested types and enums, recursively.
        /// </summary>
        private int CountTotalGeneratedTypes()
        {
            return nestedTypes.Sum(nested => nested.CountTotalGeneratedTypes()) + enumTypes.Count;
        }

        /// <summary>
        /// The brief name of the descriptor's target.
        /// </summary>
        public override string Name { get { return proto.Name; } }

        internal DescriptorProto Proto { get { return proto; } }

        /// <summary>
        /// The generated type for this message, or <c>null</c> if the descriptor does not represent a generated type.
        /// </summary>
        public Type GeneratedType { get { return generatedType; } }

        /// <summary>
        /// Returns whether this message is one of the "well known types" which may have runtime/protoc support.
        /// </summary>
        internal bool IsWellKnownType
        {
            get
            {
                return File.Package == "google.protobuf" && WellKnownTypeNames.Contains(File.Name);
            }
        }

        /// <value>
        /// If this is a nested type, get the outer descriptor, otherwise null.
        /// </value>
        public MessageDescriptor ContainingType
        {
            get { return containingType; }
        }

        /// <value>
        /// An unmodifiable list of this message type's fields.
        /// </value>
        public IList<FieldDescriptor> Fields
        {
            get { return fields; }
        }

        /// <value>
        /// An unmodifiable list of this message type's nested types.
        /// </value>
        public IList<MessageDescriptor> NestedTypes
        {
            get { return nestedTypes; }
        }

        /// <value>
        /// An unmodifiable list of this message type's enum types.
        /// </value>
        public IList<EnumDescriptor> EnumTypes
        {
            get { return enumTypes; }
        }

        public IList<OneofDescriptor> Oneofs
        {
            get { return oneofs; }
        }

        /// <summary>
        /// Returns a map from field number to accessor.
        /// TODO: Revisit this. It's mostly in place to make the transition from FieldAccessorTable
        /// to descriptor-based reflection simple in terms of tests. Work out what we really want.
        /// </summary>
        public IDictionary<int, IFieldAccessor> FieldAccessorsByFieldNumber { get { return fieldAccessorsByFieldNumber; } }

        /// <summary>
        /// Finds a field by field name.
        /// </summary>
        /// <param name="name">The unqualified name of the field (e.g. "foo").</param>
        /// <returns>The field's descriptor, or null if not found.</returns>
        public FieldDescriptor FindFieldByName(String name)
        {
            return File.DescriptorPool.FindSymbol<FieldDescriptor>(FullName + "." + name);
        }

        /// <summary>
        /// Finds a field by field number.
        /// </summary>
        /// <param name="number">The field number within this message type.</param>
        /// <returns>The field's descriptor, or null if not found.</returns>
        public FieldDescriptor FindFieldByNumber(int number)
        {
            return File.DescriptorPool.FindFieldByNumber(this, number);
        }

        /// <summary>
        /// Finds a nested descriptor by name. The is valid for fields, nested
        /// message types, oneofs and enums.
        /// </summary>
        /// <param name="name">The unqualified name of the descriptor, e.g. "Foo"</param>
        /// <returns>The descriptor, or null if not found.</returns>
        public T FindDescriptor<T>(string name)
            where T : class, IDescriptor
        {
            return File.DescriptorPool.FindSymbol<T>(FullName + "." + name);
        }

        /// <summary>
        /// Looks up and cross-links all fields and nested types.
        /// </summary>
        internal void CrossLink()
        {
            foreach (MessageDescriptor message in nestedTypes)
            {
                message.CrossLink();
            }

            foreach (FieldDescriptor field in fields)
            {
                field.CrossLink();
            }

            foreach (OneofDescriptor oneof in oneofs)
            {
                oneof.CrossLink();
            }

            fieldAccessorsByFieldNumber = new ReadOnlyDictionary<int, IFieldAccessor>(fields.ToDictionary(field => field.FieldNumber, field => field.Accessor));
        }        
    }
}