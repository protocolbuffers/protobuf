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
using System.Collections.Generic;
using Google.ProtocolBuffers.DescriptorProtos;

namespace Google.ProtocolBuffers.Descriptors
{
    /// <summary>
    /// Describes a message type.
    /// </summary>
    public sealed class MessageDescriptor : IndexedDescriptorBase<DescriptorProto, MessageOptions>
    {
        private readonly MessageDescriptor containingType;
        private readonly IList<MessageDescriptor> nestedTypes;
        private readonly IList<EnumDescriptor> enumTypes;
        private readonly IList<FieldDescriptor> fields;
        private readonly IList<FieldDescriptor> extensions;
        private bool hasRequiredFields;

        internal MessageDescriptor(DescriptorProto proto, FileDescriptor file, MessageDescriptor parent, int typeIndex)
            : base(proto, file, ComputeFullName(file, parent, proto.Name), typeIndex)
        {
            containingType = parent;

            nestedTypes = DescriptorUtil.ConvertAndMakeReadOnly(proto.NestedTypeList,
                                                                (type, index) =>
                                                                new MessageDescriptor(type, file, this, index));

            enumTypes = DescriptorUtil.ConvertAndMakeReadOnly(proto.EnumTypeList,
                                                              (type, index) =>
                                                              new EnumDescriptor(type, file, this, index));

            // TODO(jonskeet): Sort fields first?
            fields = DescriptorUtil.ConvertAndMakeReadOnly(proto.FieldList,
                                                           (field, index) =>
                                                           new FieldDescriptor(field, file, this, index, false));

            extensions = DescriptorUtil.ConvertAndMakeReadOnly(proto.ExtensionList,
                                                               (field, index) =>
                                                               new FieldDescriptor(field, file, this, index, true));

            file.DescriptorPool.AddSymbol(this);
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
        /// An unmodifiable list of this message type's extensions.
        /// </value>
        public IList<FieldDescriptor> Extensions
        {
            get { return extensions; }
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

        /// <summary>
        /// Returns a pre-computed result as to whether this message
        /// has required fields. This includes optional fields which are
        /// message types which in turn have required fields, and any 
        /// extension fields.
        /// </summary>
        internal bool HasRequiredFields
        {
            get { return hasRequiredFields; }
        }

        /// <summary>
        /// Determines if the given field number is an extension.
        /// </summary>
        public bool IsExtensionNumber(int number)
        {
            foreach (DescriptorProto.Types.ExtensionRange range in Proto.ExtensionRangeList)
            {
                if (range.Start <= number && number < range.End)
                {
                    return true;
                }
            }
            return false;
        }

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
        /// Finds a field by its property name, as it would be generated by protogen.
        /// </summary>
        /// <param name="propertyName">The property name within this message type.</param>
        /// <returns>The field's descriptor, or null if not found.</returns>
        public FieldDescriptor FindFieldByPropertyName(string propertyName)
        {
            // For reasonably short messages, this will be more efficient than a dictionary
            // lookup. It also means we don't need to do things lazily with locks etc.
            foreach (FieldDescriptor field in Fields)
            {
                if (field.CSharpOptions.PropertyName == propertyName)
                {
                    return field;
                }
            }
            return null;
        }

        /// <summary>
        /// Finds a nested descriptor by name. The is valid for fields, nested
        /// message types and enums.
        /// </summary>
        /// <param name="name">The unqualified name of the descriptor, e.g. "Foo"</param>
        /// <returns>The descriptor, or null if not found.</returns>
        public T FindDescriptor<T>(string name)
            where T : class, IDescriptor
        {
            return File.DescriptorPool.FindSymbol<T>(FullName + "." + name);
        }

        /// <summary>
        /// Looks up and cross-links all fields, nested types, and extensions.
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

            foreach (FieldDescriptor extension in extensions)
            {
                extension.CrossLink();
            }
        }

        internal void CheckRequiredFields()
        {
            IDictionary<MessageDescriptor, byte> alreadySeen = new Dictionary<MessageDescriptor, byte>();
            hasRequiredFields = CheckRequiredFields(alreadySeen);
        }

        private bool CheckRequiredFields(IDictionary<MessageDescriptor, byte> alreadySeen)
        {
            if (alreadySeen.ContainsKey(this))
            {
                // The type is already in the cache. This means that either:
                // a. The type has no required fields.
                // b. We are in the midst of checking if the type has required fields,
                //    somewhere up the stack.  In this case, we know that if the type
                //    has any required fields, they'll be found when we return to it,
                //    and the whole call to HasRequiredFields() will return true.
                //    Therefore, we don't have to check if this type has required fields
                //    here.
                return false;
            }
            alreadySeen[this] = 0; // Value is irrelevant; we want set semantics

            // If the type allows extensions, an extension with message type could contain
            // required fields, so we have to be conservative and assume such an
            // extension exists.
            if (Proto.ExtensionRangeCount != 0)
            {
                return true;
            }

            foreach (FieldDescriptor field in Fields)
            {
                if (field.IsRequired)
                {
                    return true;
                }
                if (field.MappedType == MappedType.Message)
                {
                    if (field.MessageType.CheckRequiredFields(alreadySeen))
                    {
                        return true;
                    }
                }
            }
            return false;
        }

        /// <summary>
        /// See FileDescriptor.ReplaceProto
        /// </summary>
        internal override void ReplaceProto(DescriptorProto newProto)
        {
            base.ReplaceProto(newProto);

            for (int i = 0; i < nestedTypes.Count; i++)
            {
                nestedTypes[i].ReplaceProto(newProto.GetNestedType(i));
            }

            for (int i = 0; i < enumTypes.Count; i++)
            {
                enumTypes[i].ReplaceProto(newProto.GetEnumType(i));
            }

            for (int i = 0; i < fields.Count; i++)
            {
                fields[i].ReplaceProto(newProto.GetField(i));
            }

            for (int i = 0; i < extensions.Count; i++)
            {
                extensions[i].ReplaceProto(newProto.GetExtension(i));
            }
        }
    }
}