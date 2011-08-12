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
using System.Reflection;
using Google.ProtocolBuffers.Collections;
using Google.ProtocolBuffers.DescriptorProtos;

namespace Google.ProtocolBuffers.Descriptors
{
    /// <summary>
    /// Descriptor for a field or extension within a message in a .proto file.
    /// </summary>
    public sealed class FieldDescriptor : IndexedDescriptorBase<FieldDescriptorProto, FieldOptions>,
                                          IComparable<FieldDescriptor>, IFieldDescriptorLite
    {
        private readonly MessageDescriptor extensionScope;
        private EnumDescriptor enumType;
        private MessageDescriptor messageType;
        private MessageDescriptor containingType;
        private object defaultValue;
        private FieldType fieldType;
        private MappedType mappedType;

        private CSharpFieldOptions csharpFieldOptions;
        private readonly object optionsLock = new object();

        internal FieldDescriptor(FieldDescriptorProto proto, FileDescriptor file,
                                 MessageDescriptor parent, int index, bool isExtension)
            : base(proto, file, ComputeFullName(file, parent, proto.Name), index)
        {
            if (proto.HasType)
            {
                fieldType = GetFieldTypeFromProtoType(proto.Type);
                mappedType = FieldTypeToMappedTypeMap[fieldType];
            }

            if (FieldNumber <= 0)
            {
                throw new DescriptorValidationException(this,
                                                        "Field numbers must be positive integers.");
            }

            if (isExtension)
            {
                if (!proto.HasExtendee)
                {
                    throw new DescriptorValidationException(this,
                                                            "FieldDescriptorProto.Extendee not set for extension field.");
                }
                containingType = null; // Will be filled in when cross-linking
                if (parent != null)
                {
                    extensionScope = parent;
                }
                else
                {
                    extensionScope = null;
                }
            }
            else
            {
                if (proto.HasExtendee)
                {
                    throw new DescriptorValidationException(this,
                                                            "FieldDescriptorProto.Extendee set for non-extension field.");
                }
                containingType = parent;
                extensionScope = null;
            }

            file.DescriptorPool.AddSymbol(this);
        }

        private CSharpFieldOptions BuildOrFakeCSharpOptions()
        {
            // TODO(jonskeet): Check if we could use FileDescriptorProto.Descriptor.Name - interesting bootstrap issues
            if (File.Proto.Name == "google/protobuf/csharp_options.proto")
            {
                if (Name == "csharp_field_options")
                {
                    return new CSharpFieldOptions.Builder {PropertyName = "CSharpFieldOptions"}.Build();
                }
                if (Name == "csharp_file_options")
                {
                    return new CSharpFieldOptions.Builder {PropertyName = "CSharpFileOptions"}.Build();
                }
            }
            CSharpFieldOptions.Builder builder = CSharpFieldOptions.CreateBuilder();
            if (Proto.Options.HasExtension(DescriptorProtos.CSharpOptions.CSharpFieldOptions))
            {
                builder.MergeFrom(Proto.Options.GetExtension(DescriptorProtos.CSharpOptions.CSharpFieldOptions));
            }
            if (!builder.HasPropertyName)
            {
                string fieldName = FieldType == FieldType.Group ? MessageType.Name : Name;
                string propertyName = NameHelpers.UnderscoresToPascalCase(fieldName);
                if (propertyName == ContainingType.Name)
                {
                    propertyName += "_";
                }
                builder.PropertyName = propertyName;
            }
            return builder.Build();
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

        /// <summary>
        /// Returns the default value for a mapped type.
        /// </summary>
        private static object GetDefaultValueForMappedType(MappedType type)
        {
            switch (type)
            {
                case MappedType.Int32:
                    return 0;
                case MappedType.Int64:
                    return (long) 0;
                case MappedType.UInt32:
                    return (uint) 0;
                case MappedType.UInt64:
                    return (ulong) 0;
                case MappedType.Single:
                    return (float) 0;
                case MappedType.Double:
                    return (double) 0;
                case MappedType.Boolean:
                    return false;
                case MappedType.String:
                    return "";
                case MappedType.ByteString:
                    return ByteString.Empty;
                case MappedType.Message:
                    return null;
                case MappedType.Enum:
                    return null;
                default:
                    throw new ArgumentException("Invalid type specified");
            }
        }

        public bool IsRequired
        {
            get { return Proto.Label == FieldDescriptorProto.Types.Label.LABEL_REQUIRED; }
        }

        public bool IsOptional
        {
            get { return Proto.Label == FieldDescriptorProto.Types.Label.LABEL_OPTIONAL; }
        }

        public bool IsRepeated
        {
            get { return Proto.Label == FieldDescriptorProto.Types.Label.LABEL_REPEATED; }
        }

        public bool IsPacked
        {
            get { return Proto.Options.Packed; }
        }

        /// <valule>
        /// Indicates whether or not the field had an explicitly-defined default value.
        /// </value>
        public bool HasDefaultValue
        {
            get { return Proto.HasDefaultValue; }
        }

        /// <value>
        /// The field's default value. Valid for all types except messages
        /// and groups. For all other types, the object returned is of the
        /// same class that would be returned by IMessage[this].
        /// For repeated fields this will always be an empty immutable list compatible with IList[object].
        /// For message fields it will always be null. For singular values, it will depend on the descriptor.
        /// </value>
        public object DefaultValue
        {
            get
            {
                if (MappedType == MappedType.Message)
                {
                    throw new InvalidOperationException(
                        "FieldDescriptor.DefaultValue called on an embedded message field.");
                }
                return defaultValue;
            }
        }

        /// <value>
        /// Indicates whether or not this field is an extension.
        /// </value>
        public bool IsExtension
        {
            get { return Proto.HasExtendee; }
        }

        /*
     * Get the field's containing type. For extensions, this is the type being
     * extended, not the location where the extension was defined.  See
     * {@link #getExtensionScope()}.
     */

        /// <summary>
        /// Get the field's containing type. For extensions, this is the type being
        /// extended, not the location where the extension was defined. See
        /// <see cref="ExtensionScope" />.
        /// </summary>
        public MessageDescriptor ContainingType
        {
            get { return containingType; }
        }

        /// <summary>
        /// Returns the C#-specific options for this field descriptor. This will always be
        /// completely filled in.
        /// </summary>
        public CSharpFieldOptions CSharpOptions
        {
            get
            {
                lock (optionsLock)
                {
                    if (csharpFieldOptions == null)
                    {
                        csharpFieldOptions = BuildOrFakeCSharpOptions();
                    }
                }
                return csharpFieldOptions;
            }
        }

        /// <summary>
        /// For extensions defined nested within message types, gets
        /// the outer type. Not valid for non-extension fields.
        /// </summary>
        /// <example>
        /// <code>
        /// message Foo {
        ///   extensions 1000 to max;
        /// }
        /// extend Foo {
        ///   optional int32 baz = 1234;
        /// }
        /// message Bar {
        ///   extend Foo {
        ///     optional int32 qux = 4321;
        ///   }
        /// }
        /// </code>
        /// The containing type for both <c>baz</c> and <c>qux</c> is <c>Foo</c>.
        /// However, the extension scope for <c>baz</c> is <c>null</c> while
        /// the extension scope for <c>qux</c> is <c>Bar</c>.
        /// </example>
        public MessageDescriptor ExtensionScope
        {
            get
            {
                if (!IsExtension)
                {
                    throw new InvalidOperationException("This field is not an extension.");
                }
                return extensionScope;
            }
        }

        public MappedType MappedType
        {
            get { return mappedType; }
        }

        public FieldType FieldType
        {
            get { return fieldType; }
        }

        public bool IsCLSCompliant
        {
            get 
            { 
                return mappedType != MappedType.UInt32 && 
                    mappedType != MappedType.UInt64 &&
                    !NameHelpers.UnderscoresToPascalCase(Name).StartsWith("_");
            }
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
        /// Compares this descriptor with another one, ordering in "canonical" order
        /// which simply means ascending order by field number. <paramref name="other"/>
        /// must be a field of the same type, i.e. the <see cref="ContainingType"/> of
        /// both fields must be the same.
        /// </summary>
        public int CompareTo(IFieldDescriptorLite other)
        {
            return FieldNumber - other.FieldNumber;
        }

        IEnumLiteMap IFieldDescriptorLite.EnumType
        {
            get { return EnumType; }
        }

        bool IFieldDescriptorLite.MessageSetWireFormat
        {
            get { return ContainingType.Options.MessageSetWireFormat; }
        }

        /// <summary>
        /// For enum fields, returns the field's type.
        /// </summary>
        public EnumDescriptor EnumType
        {
            get
            {
                if (MappedType != MappedType.Enum)
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
                if (MappedType != MappedType.Message)
                {
                    throw new InvalidOperationException("MessageType is only valid for enum fields.");
                }
                return messageType;
            }
        }

        /// <summary>
        /// Immutable mapping from field type to mapped type. Built using the attributes on
        /// FieldType values.
        /// </summary>
        public static readonly IDictionary<FieldType, MappedType> FieldTypeToMappedTypeMap = MapFieldTypes();

        private static IDictionary<FieldType, MappedType> MapFieldTypes()
        {
            var map = new Dictionary<FieldType, MappedType>();
            foreach (FieldInfo field in typeof (FieldType).GetFields(BindingFlags.Static | BindingFlags.Public))
            {
                FieldType fieldType = (FieldType) field.GetValue(null);
                FieldMappingAttribute mapping =
                    (FieldMappingAttribute) field.GetCustomAttributes(typeof (FieldMappingAttribute), false)[0];
                map[fieldType] = mapping.MappedType;
            }
            return Dictionaries.AsReadOnly(map);
        }

        /// <summary>
        /// Look up and cross-link all field types etc.
        /// </summary>
        internal void CrossLink()
        {
            if (Proto.HasExtendee)
            {
                IDescriptor extendee = File.DescriptorPool.LookupSymbol(Proto.Extendee, this);
                if (!(extendee is MessageDescriptor))
                {
                    throw new DescriptorValidationException(this, "\"" + Proto.Extendee + "\" is not a message type.");
                }
                containingType = (MessageDescriptor) extendee;

                if (!containingType.IsExtensionNumber(FieldNumber))
                {
                    throw new DescriptorValidationException(this,
                                                            "\"" + containingType.FullName + "\" does not declare " +
                                                            FieldNumber + " as an extension number.");
                }
            }

            if (Proto.HasTypeName)
            {
                IDescriptor typeDescriptor =
                    File.DescriptorPool.LookupSymbol(Proto.TypeName, this);

                if (!Proto.HasType)
                {
                    // Choose field type based on symbol.
                    if (typeDescriptor is MessageDescriptor)
                    {
                        fieldType = FieldType.Message;
                        mappedType = MappedType.Message;
                    }
                    else if (typeDescriptor is EnumDescriptor)
                    {
                        fieldType = FieldType.Enum;
                        mappedType = MappedType.Enum;
                    }
                    else
                    {
                        throw new DescriptorValidationException(this, "\"" + Proto.TypeName + "\" is not a type.");
                    }
                }

                if (MappedType == MappedType.Message)
                {
                    if (!(typeDescriptor is MessageDescriptor))
                    {
                        throw new DescriptorValidationException(this,
                                                                "\"" + Proto.TypeName + "\" is not a message type.");
                    }
                    messageType = (MessageDescriptor) typeDescriptor;

                    if (Proto.HasDefaultValue)
                    {
                        throw new DescriptorValidationException(this, "Messages can't have default values.");
                    }
                }
                else if (MappedType == Descriptors.MappedType.Enum)
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
                if (MappedType == MappedType.Message || MappedType == MappedType.Enum)
                {
                    throw new DescriptorValidationException(this, "Field with message or enum type missing type_name.");
                }
            }

            // We don't attempt to parse the default value until here because for
            // enums we need the enum type's descriptor.
            if (Proto.HasDefaultValue)
            {
                if (IsRepeated)
                {
                    throw new DescriptorValidationException(this, "Repeated fields cannot have default values.");
                }

                try
                {
                    switch (FieldType)
                    {
                        case FieldType.Int32:
                        case FieldType.SInt32:
                        case FieldType.SFixed32:
                            defaultValue = TextFormat.ParseInt32(Proto.DefaultValue);
                            break;
                        case FieldType.UInt32:
                        case FieldType.Fixed32:
                            defaultValue = TextFormat.ParseUInt32(Proto.DefaultValue);
                            break;
                        case FieldType.Int64:
                        case FieldType.SInt64:
                        case FieldType.SFixed64:
                            defaultValue = TextFormat.ParseInt64(Proto.DefaultValue);
                            break;
                        case FieldType.UInt64:
                        case FieldType.Fixed64:
                            defaultValue = TextFormat.ParseUInt64(Proto.DefaultValue);
                            break;
                        case FieldType.Float:
                            defaultValue = TextFormat.ParseFloat(Proto.DefaultValue);
                            break;
                        case FieldType.Double:
                            defaultValue = TextFormat.ParseDouble(Proto.DefaultValue);
                            break;
                        case FieldType.Bool:
                            if (Proto.DefaultValue == "true")
                            {
                                defaultValue = true;
                            }
                            else if (Proto.DefaultValue == "false")
                            {
                                defaultValue = false;
                            }
                            else
                            {
                                throw new FormatException("Boolean values must be \"true\" or \"false\"");
                            }
                            break;
                        case FieldType.String:
                            defaultValue = Proto.DefaultValue;
                            break;
                        case FieldType.Bytes:
                            try
                            {
                                defaultValue = TextFormat.UnescapeBytes(Proto.DefaultValue);
                            }
                            catch (FormatException e)
                            {
                                throw new DescriptorValidationException(this,
                                                                        "Couldn't parse default value: " + e.Message);
                            }
                            break;
                        case FieldType.Enum:
                            defaultValue = enumType.FindValueByName(Proto.DefaultValue);
                            if (defaultValue == null)
                            {
                                throw new DescriptorValidationException(this,
                                                                        "Unknown enum default value: \"" +
                                                                        Proto.DefaultValue + "\"");
                            }
                            break;
                        case FieldType.Message:
                        case FieldType.Group:
                            throw new DescriptorValidationException(this, "Message type had default value.");
                    }
                }
                catch (FormatException e)
                {
                    DescriptorValidationException validationException =
                        new DescriptorValidationException(this,
                                                          "Could not parse default value: \"" + Proto.DefaultValue +
                                                          "\"", e);
                    throw validationException;
                }
            }
            else
            {
                // Determine the default default for this field.
                if (IsRepeated)
                {
                    defaultValue = Lists<object>.Empty;
                }
                else
                {
                    switch (MappedType)
                    {
                        case MappedType.Enum:
                            // We guarantee elsewhere that an enum type always has at least
                            // one possible value.
                            defaultValue = enumType.Values[0];
                            break;
                        case MappedType.Message:
                            defaultValue = null;
                            break;
                        default:
                            defaultValue = GetDefaultValueForMappedType(MappedType);
                            break;
                    }
                }
            }

            if (!IsExtension)
            {
                File.DescriptorPool.AddFieldByNumber(this);
            }

            if (containingType != null && containingType.Options.MessageSetWireFormat)
            {
                if (IsExtension)
                {
                    if (!IsOptional || FieldType != FieldType.Message)
                    {
                        throw new DescriptorValidationException(this,
                                                                "Extensions of MessageSets must be optional messages.");
                    }
                }
                else
                {
                    throw new DescriptorValidationException(this, "MessageSets cannot have fields, only extensions.");
                }
            }
        }
    }
}