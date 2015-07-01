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
using Google.Protobuf.DescriptorProtos;

namespace Google.Protobuf.Descriptors
{
    /// <summary>
    /// Descriptor for a field or extension within a message in a .proto file.
    /// </summary>
    public sealed class FieldDescriptor : IndexedDescriptorBase<FieldDescriptorProto, FieldOptions>,
                                          IComparable<FieldDescriptor>
    {
        private EnumDescriptor enumType;
        private MessageDescriptor messageType;
        private MessageDescriptor containingType;
        private OneofDescriptor containingOneof;
        private FieldType fieldType;

        private readonly object optionsLock = new object();

        internal FieldDescriptor(FieldDescriptorProto proto, FileDescriptor file,
                                 MessageDescriptor parent, int index)
            : base(proto, file, ComputeFullName(file, parent, proto.Name), index)
        {
            if (proto.Type != 0)
            {
                fieldType = GetFieldTypeFromProtoType(proto.Type);
            }

            if (FieldNumber <= 0)
            {
                throw new DescriptorValidationException(this,
                                                        "Field numbers must be positive integers.");
            }
            containingType = parent;
            if (proto.OneofIndex != 0)
            {
                if (proto.OneofIndex < 0 || proto.OneofIndex >= parent.Proto.OneofDecl.Count)
                {
                    throw new DescriptorValidationException(this,
                        "FieldDescriptorProto.oneof_index is out of range for type " + parent.Name);
                }
                containingOneof = parent.Oneofs[proto.OneofIndex];
                containingOneof.fieldCount ++;
            }

            file.DescriptorPool.AddSymbol(this);
        }

        /// <summary>
        /// Maps a field type as included in the .proto file to a FieldType.
        /// </summary>
        private static FieldType GetFieldTypeFromProtoType(FieldDescriptorProto.Types.Type type)
        {
            switch (type)
            {
                case FieldDescriptorProto.Types.Type.TYPE_DOUBLE:
                    return FieldType.Double;
                case FieldDescriptorProto.Types.Type.TYPE_FLOAT:
                    return FieldType.Float;
                case FieldDescriptorProto.Types.Type.TYPE_INT64:
                    return FieldType.Int64;
                case FieldDescriptorProto.Types.Type.TYPE_UINT64:
                    return FieldType.UInt64;
                case FieldDescriptorProto.Types.Type.TYPE_INT32:
                    return FieldType.Int32;
                case FieldDescriptorProto.Types.Type.TYPE_FIXED64:
                    return FieldType.Fixed64;
                case FieldDescriptorProto.Types.Type.TYPE_FIXED32:
                    return FieldType.Fixed32;
                case FieldDescriptorProto.Types.Type.TYPE_BOOL:
                    return FieldType.Bool;
                case FieldDescriptorProto.Types.Type.TYPE_STRING:
                    return FieldType.String;
                case FieldDescriptorProto.Types.Type.TYPE_GROUP:
                    return FieldType.Group;
                case FieldDescriptorProto.Types.Type.TYPE_MESSAGE:
                    return FieldType.Message;
                case FieldDescriptorProto.Types.Type.TYPE_BYTES:
                    return FieldType.Bytes;
                case FieldDescriptorProto.Types.Type.TYPE_UINT32:
                    return FieldType.UInt32;
                case FieldDescriptorProto.Types.Type.TYPE_ENUM:
                    return FieldType.Enum;
                case FieldDescriptorProto.Types.Type.TYPE_SFIXED32:
                    return FieldType.SFixed32;
                case FieldDescriptorProto.Types.Type.TYPE_SFIXED64:
                    return FieldType.SFixed64;
                case FieldDescriptorProto.Types.Type.TYPE_SINT32:
                    return FieldType.SInt32;
                case FieldDescriptorProto.Types.Type.TYPE_SINT64:
                    return FieldType.SInt64;
                default:
                    throw new ArgumentException("Invalid type specified");
            }
        }        

        public bool IsRepeated
        {
            get { return Proto.Label == FieldDescriptorProto.Types.Label.LABEL_REPEATED; }
        }

        public bool IsMap
        {
            get { return fieldType == FieldType.Message && messageType.Options != null && messageType.Options.MapEntry; }
        }

        public bool IsPacked
        {
            get { return Proto.Options.Packed; }
        }        

        /// <summary>
        /// Get the field's containing type. For extensions, this is the type being
        /// extended, not the location where the extension was defined. See
        /// <see cref="ExtensionScope" />.
        /// </summary>
        public MessageDescriptor ContainingType
        {
            get { return containingType; }
        }

        public OneofDescriptor ContainingOneof
        {
            get { return containingOneof; }
        }        

        public FieldType FieldType
        {
            get { return fieldType; }
        }

        public int FieldNumber
        {
            get { return Proto.Number; }
        }

        /// <summary>
        /// Compares this descriptor with another one, ordering in "canonical" order
        /// which simply means ascending order by field number. <paramref name="other"/>
        /// must be a field of the same type, i.e. the <see cref="ContainingType"/> of
        /// both fields must be the same.
        /// </summary>
        public int CompareTo(FieldDescriptor other)
        {
            if (other.containingType != containingType)
            {
                throw new ArgumentException("FieldDescriptors can only be compared to other FieldDescriptors " +
                                            "for fields of the same message type.");
            }
            return FieldNumber - other.FieldNumber;
        }

        /// <summary>
        /// For enum fields, returns the field's type.
        /// </summary>
        public EnumDescriptor EnumType
        {
            get
            {
                if (fieldType != FieldType.Enum)
                {
                    throw new InvalidOperationException("EnumType is only valid for enum fields.");
                }
                return enumType;
            }
        }

        /// <summary>
        /// For embedded message and group fields, returns the field's type.
        /// </summary>
        public MessageDescriptor MessageType
        {
            get
            {
                if (fieldType != FieldType.Message)
                {
                    throw new InvalidOperationException("MessageType is only valid for enum fields.");
                }
                return messageType;
            }
        }

        /// <summary>
        /// Look up and cross-link all field types etc.
        /// </summary>
        internal void CrossLink()
        {
            if (Proto.TypeName != "")
            {
                IDescriptor typeDescriptor =
                    File.DescriptorPool.LookupSymbol(Proto.TypeName, this);

                if (Proto.Type != 0)
                {
                    // Choose field type based on symbol.
                    if (typeDescriptor is MessageDescriptor)
                    {
                        fieldType = FieldType.Message;
                    }
                    else if (typeDescriptor is EnumDescriptor)
                    {
                        fieldType = FieldType.Enum;
                    }
                    else
                    {
                        throw new DescriptorValidationException(this, "\"" + Proto.TypeName + "\" is not a type.");
                    }
                }

                if (fieldType == FieldType.Message)
                {
                    if (!(typeDescriptor is MessageDescriptor))
                    {
                        throw new DescriptorValidationException(this,
                                                                "\"" + Proto.TypeName + "\" is not a message type.");
                    }
                    messageType = (MessageDescriptor) typeDescriptor;

                    if (Proto.DefaultValue != "")
                    {
                        throw new DescriptorValidationException(this, "Messages can't have default values.");
                    }
                }
                else if (fieldType == FieldType.Enum)
                {
                    if (!(typeDescriptor is EnumDescriptor))
                    {
                        throw new DescriptorValidationException(this, "\"" + Proto.TypeName + "\" is not an enum type.");
                    }
                    enumType = (EnumDescriptor) typeDescriptor;
                }
                else
                {
                    throw new DescriptorValidationException(this, "Field with primitive type has type_name.");
                }
            }
            else
            {
                if (fieldType == FieldType.Message || fieldType == FieldType.Enum)
                {
                    throw new DescriptorValidationException(this, "Field with message or enum type missing type_name.");
                }
            }

            // Note: no attempt to perform any default value parsing

            File.DescriptorPool.AddFieldByNumber(this);

            if (containingType != null && containingType.Options != null && containingType.Options.MessageSetWireFormat)
            {
                throw new DescriptorValidationException(this, "MessageSet format is not supported.");
            }
        }
    }
}