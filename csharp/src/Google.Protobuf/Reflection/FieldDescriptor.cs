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
using Google.Protobuf.Compatibility;
using System;

namespace Google.Protobuf.Reflection
{
    /// <summary>
    /// Descriptor for a field or extension within a message in a .proto file.
    /// </summary>
    public sealed class FieldDescriptor : DescriptorBase, IComparable<FieldDescriptor>
    {
        private EnumDescriptor enumType;
        private MessageDescriptor extendeeType;
        private MessageDescriptor messageType;
        private FieldType fieldType;
        private readonly string propertyName; // Annoyingly, needed in Crosslink.
        private IFieldAccessor accessor;

        /// <summary>
        /// Get the field's containing message type, or <c>null</c> if it is a field defined at the top level of a file as an extension.
        /// </summary>
        public MessageDescriptor ContainingType { get; }

        /// <summary>
        /// Returns the oneof containing this field, or <c>null</c> if it is not part of a oneof.
        /// </summary>
        public OneofDescriptor ContainingOneof { get; }

        /// <summary>
        /// The effective JSON name for this field. This is usually the lower-camel-cased form of the field name,
        /// but can be overridden using the <c>json_name</c> option in the .proto file.
        /// </summary>
        public string JsonName { get; }

        internal FieldDescriptorProto Proto { get; }

        internal Extension Extension { get; }

        internal FieldDescriptor(FieldDescriptorProto proto, FileDescriptor file,
                                 MessageDescriptor parent, int index, string propertyName, Extension extension)
            : base(file, file.ComputeFullName(parent, proto.Name), index)
        {
            Proto = proto;
            if (proto.Type != 0)
            {
                fieldType = GetFieldTypeFromProtoType(proto.Type);
            }

            if (FieldNumber <= 0)
            {
                throw new DescriptorValidationException(this, "Field numbers must be positive integers.");
            }
            ContainingType = parent;
            if (proto.HasOneofIndex)
            {
                if (proto.OneofIndex < 0 || proto.OneofIndex >= parent.Proto.OneofDecl.Count)
                {
                    throw new DescriptorValidationException(this,
                        $"FieldDescriptorProto.oneof_index is out of range for type {parent.Name}");
                }
                ContainingOneof = parent.Oneofs[proto.OneofIndex];
            }

            file.DescriptorPool.AddSymbol(this);
            // We can't create the accessor until we've cross-linked, unfortunately, as we
            // may not know whether the type of the field is a map or not. Remember the property name
            // for later.
            // We could trust the generated code and check whether the type of the property is
            // a MapField, but that feels a tad nasty.
            this.propertyName = propertyName;
            Extension = extension;
            JsonName =  Proto.JsonName == "" ? JsonFormatter.ToJsonName(Proto.Name) : Proto.JsonName;
        }


        /// <summary>
        /// The brief name of the descriptor's target.
        /// </summary>
        public override string Name => Proto.Name;

        /// <summary>
        /// Returns the accessor for this field.
        /// </summary>
        /// <remarks>
        /// <para>
        /// While a <see cref="FieldDescriptor"/> describes the field, it does not provide
        /// any way of obtaining or changing the value of the field within a specific message;
        /// that is the responsibility of the accessor.
        /// </para>
        /// <para>
        /// In descriptors for generated code, the value returned by this property will be non-null for all
        /// regular fields. However, if a message containing a map field is introspected, the list of nested messages will include
        /// an auto-generated nested key/value pair message for the field. This is not represented in any
        /// generated type, and the value of the map field itself is represented by a dictionary in the
        /// reflection API. There are never instances of those "hidden" messages, so no accessor is provided
        /// and this property will return null.
        /// </para>
        /// <para>
        /// In dynamically loaded descriptors, the value returned by this property will current be null;
        /// if and when dynamic messages are supported, it will return a suitable accessor to work with
        /// them.
        /// </para>
        /// </remarks>
        public IFieldAccessor Accessor => accessor;

        /// <summary>
        /// Maps a field type as included in the .proto file to a FieldType.
        /// </summary>
        private static FieldType GetFieldTypeFromProtoType(FieldDescriptorProto.Types.Type type)
        {
            switch (type)
            {
                case FieldDescriptorProto.Types.Type.Double:
                    return FieldType.Double;
                case FieldDescriptorProto.Types.Type.Float:
                    return FieldType.Float;
                case FieldDescriptorProto.Types.Type.Int64:
                    return FieldType.Int64;
                case FieldDescriptorProto.Types.Type.Uint64:
                    return FieldType.UInt64;
                case FieldDescriptorProto.Types.Type.Int32:
                    return FieldType.Int32;
                case FieldDescriptorProto.Types.Type.Fixed64:
                    return FieldType.Fixed64;
                case FieldDescriptorProto.Types.Type.Fixed32:
                    return FieldType.Fixed32;
                case FieldDescriptorProto.Types.Type.Bool:
                    return FieldType.Bool;
                case FieldDescriptorProto.Types.Type.String:
                    return FieldType.String;
                case FieldDescriptorProto.Types.Type.Group:
                    return FieldType.Group;
                case FieldDescriptorProto.Types.Type.Message:
                    return FieldType.Message;
                case FieldDescriptorProto.Types.Type.Bytes:
                    return FieldType.Bytes;
                case FieldDescriptorProto.Types.Type.Uint32:
                    return FieldType.UInt32;
                case FieldDescriptorProto.Types.Type.Enum:
                    return FieldType.Enum;
                case FieldDescriptorProto.Types.Type.Sfixed32:
                    return FieldType.SFixed32;
                case FieldDescriptorProto.Types.Type.Sfixed64:
                    return FieldType.SFixed64;
                case FieldDescriptorProto.Types.Type.Sint32:
                    return FieldType.SInt32;
                case FieldDescriptorProto.Types.Type.Sint64:
                    return FieldType.SInt64;
                default:
                    throw new ArgumentException("Invalid type specified");
            }
        }

        /// <summary>
        /// Returns <c>true</c> if this field is a repeated field; <c>false</c> otherwise.
        /// </summary>
        public bool IsRepeated => Proto.Label == FieldDescriptorProto.Types.Label.Repeated;

        /// <summary>
        /// Returns <c>true</c> if this field is a required field; <c>false</c> otherwise.
        /// </summary>
        public bool IsRequired => Proto.Label == FieldDescriptorProto.Types.Label.Required;

        /// <summary>
        /// Returns <c>true</c> if this field is a map field; <c>false</c> otherwise.
        /// </summary>
        public bool IsMap => fieldType == FieldType.Message && messageType.Proto.Options != null && messageType.Proto.Options.MapEntry;

        /// <summary>
        /// Returns <c>true</c> if this field is a packed, repeated field; <c>false</c> otherwise.
        /// </summary>
        public bool IsPacked => File.Proto.Syntax == "proto2" ? Proto.Options?.Packed ?? false : !Proto.Options.HasPacked || Proto.Options.Packed;

        /// <summary>
        /// Returns the type of the field.
        /// </summary>
        public FieldType FieldType => fieldType;

        /// <summary>
        /// Returns the field number declared in the proto file.
        /// </summary>
        public int FieldNumber => Proto.Number;

        /// <summary>
        /// Compares this descriptor with another one, ordering in "canonical" order
        /// which simply means ascending order by field number. <paramref name="other"/>
        /// must be a field of the same type, i.e. the <see cref="ContainingType"/> of
        /// both fields must be the same.
        /// </summary>
        public int CompareTo(FieldDescriptor other)
        {
            if (other.ContainingType != ContainingType)
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
                if (fieldType != FieldType.Message && fieldType != FieldType.Group)
                {
                    throw new InvalidOperationException("MessageType is only valid for message or group fields.");
                }
                return messageType;
            }
        }

        /// <summary>
        /// For extension fields, returns the extended type
        /// </summary>
        public MessageDescriptor ExtendeeType
        {
            get
            {
                if (!Proto.HasExtendee)
                {
                    throw new InvalidOperationException("ExtendeeType is only valid for extension fields.");
                }
                return extendeeType;
            }
        }

        /// <summary>
        /// The (possibly empty) set of custom options for this field.
        /// </summary>
        //[Obsolete("CustomOptions are obsolete. Use GetOption")]
        public CustomOptions CustomOptions => Proto.Options.CustomOptions;

        /* // uncomment this in the full proto2 support PR
        /// <summary>
        /// Gets a single value enum option for this descriptor
        /// </summary>
        public T GetOption<T>(Extension<FieldOptions, T> extension)
        {
            var value = Proto.Options.GetExtension(extension);
            return value is IDeepCloneable<T> clonable ? clonable.Clone() : value;
        }

        /// <summary>
        /// Gets a repeated value enum option for this descriptor
        /// </summary>
        public RepeatedField<T> GetOption<T>(RepeatedExtension<FieldOptions, T> extension)
        {
            return Proto.Options.GetExtension(extension).Clone();
        }
        */

        /// <summary>
        /// Look up and cross-link all field types etc.
        /// </summary>
        internal void CrossLink()
        {
            if (Proto.HasTypeName)
            {
                IDescriptor typeDescriptor =
                    File.DescriptorPool.LookupSymbol(Proto.TypeName, this);

                if (Proto.HasType)
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
                        throw new DescriptorValidationException(this, $"\"{Proto.TypeName}\" is not a type.");
                    }
                }

                if (fieldType == FieldType.Message || fieldType == FieldType.Group)
                {
                    if (!(typeDescriptor is MessageDescriptor))
                    {
                        throw new DescriptorValidationException(this, $"\"{Proto.TypeName}\" is not a message type.");
                    }
                    messageType = (MessageDescriptor) typeDescriptor;

                    if (Proto.HasDefaultValue)
                    {
                        throw new DescriptorValidationException(this, "Messages can't have default values.");
                    }
                }
                else if (fieldType == FieldType.Enum)
                {
                    if (!(typeDescriptor is EnumDescriptor))
                    {
                        throw new DescriptorValidationException(this, $"\"{Proto.TypeName}\" is not an enum type.");
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

            if (Proto.HasExtendee)
            {
                extendeeType = File.DescriptorPool.LookupSymbol(Proto.Extendee, this) as MessageDescriptor;
            }

            // Note: no attempt to perform any default value parsing

            File.DescriptorPool.AddFieldByNumber(this);

            if (ContainingType != null && ContainingType.Proto.HasOptions && ContainingType.Proto.Options.MessageSetWireFormat)
            {
                throw new DescriptorValidationException(this, "MessageSet format is not supported.");
            }
            accessor = CreateAccessor();
        }

        private IFieldAccessor CreateAccessor()
        {
            // If we're given no property name, that's because we really don't want an accessor.
            // This could be because it's a map message, or it could be that we're loading a FileDescriptor dynamically.
            // TODO: Support dynamic messages.
            if (propertyName == null)
            {
                return null;
            }

            if (Extension != null)
            {
                return new ExtensionAccessor(this);
            }
            var property = ContainingType.ClrType.GetProperty(propertyName);
            if (property == null)
            {
                throw new DescriptorValidationException(this, $"Property {propertyName} not found in {ContainingType.ClrType}");
            }
            return IsMap ? new MapFieldAccessor(property, this)
                : IsRepeated ? new RepeatedFieldAccessor(property, this)
                : (IFieldAccessor) new SingleFieldAccessor(property, this);
        }
    }
}
 