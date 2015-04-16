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
using System.Globalization;
using System.Text;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers.ProtoGen
{
    internal abstract class FieldGeneratorBase : SourceGeneratorBase<FieldDescriptor>
    {
        private readonly int _fieldOrdinal;

        protected FieldGeneratorBase(FieldDescriptor descriptor, int fieldOrdinal)
            : base(descriptor)
        {
            _fieldOrdinal = fieldOrdinal;
        }

        public abstract void WriteHash(TextGenerator writer);
        public abstract void WriteEquals(TextGenerator writer);
        public abstract void WriteToString(TextGenerator writer);

        public int FieldOrdinal
        {
            get { return _fieldOrdinal; }
        }

        private static bool AllPrintableAscii(string text)
        {
            foreach (char c in text)
            {
                if (c < 0x20 || c > 0x7e)
                {
                    return false;
                }
            }
            return true;
        }

        /// <summary>
        /// This returns true if the field has a non-default default value.  For instance this returns 
        /// false for numerics with a default of zero '0', or booleans with a default of false.
        /// </summary>
        protected bool HasDefaultValue
        {
            get
            {
                switch (Descriptor.FieldType)
                {
                    case FieldType.Float:
                    case FieldType.Double:
                    case FieldType.Int32:
                    case FieldType.Int64:
                    case FieldType.SInt32:
                    case FieldType.SInt64:
                    case FieldType.SFixed32:
                    case FieldType.SFixed64:
                    case FieldType.UInt32:
                    case FieldType.UInt64:
                    case FieldType.Fixed32:
                    case FieldType.Fixed64:
                        {
                            IConvertible value = (IConvertible) Descriptor.DefaultValue;
                            return value.ToString(CultureInfo.InvariantCulture) != "0";
                        }
                    case FieldType.Bool:
                        return ((bool) Descriptor.DefaultValue) == true;
                    default:
                        return true;
                }
            }
        }

        /// <remarks>Copy exists in ExtensionGenerator.cs</remarks>
        protected string DefaultValue
        {
            get
            {
                string suffix = "";
                switch (Descriptor.FieldType)
                {
                    case FieldType.Float:
                        suffix = "F";
                        break;
                    case FieldType.Double:
                        suffix = "D";
                        break;
                    case FieldType.Int64:
                        suffix = "L";
                        break;
                    case FieldType.UInt64:
                        suffix = "UL";
                        break;
                }
                switch (Descriptor.FieldType)
                {
                    case FieldType.Float:
                    case FieldType.Double:
                    case FieldType.Int32:
                    case FieldType.Int64:
                    case FieldType.SInt32:
                    case FieldType.SInt64:
                    case FieldType.SFixed32:
                    case FieldType.SFixed64:
                    case FieldType.UInt32:
                    case FieldType.UInt64:
                    case FieldType.Fixed32:
                    case FieldType.Fixed64:
                        {
                            // The simple Object.ToString converts using the current culture.
                            // We want to always use the invariant culture so it's predictable.
                            IConvertible value = (IConvertible) Descriptor.DefaultValue;
                            //a few things that must be handled explicitly
                            if (Descriptor.FieldType == FieldType.Double && value is double)
                            {
                                if (double.IsNaN((double) value))
                                {
                                    return "double.NaN";
                                }
                                if (double.IsPositiveInfinity((double) value))
                                {
                                    return "double.PositiveInfinity";
                                }
                                if (double.IsNegativeInfinity((double) value))
                                {
                                    return "double.NegativeInfinity";
                                }
                            }
                            else if (Descriptor.FieldType == FieldType.Float && value is float)
                            {
                                if (float.IsNaN((float) value))
                                {
                                    return "float.NaN";
                                }
                                if (float.IsPositiveInfinity((float) value))
                                {
                                    return "float.PositiveInfinity";
                                }
                                if (float.IsNegativeInfinity((float) value))
                                {
                                    return "float.NegativeInfinity";
                                }
                            }
                            return value.ToString(CultureInfo.InvariantCulture) + suffix;
                        }
                    case FieldType.Bool:
                        return (bool) Descriptor.DefaultValue ? "true" : "false";

                    case FieldType.Bytes:
                        if (!Descriptor.HasDefaultValue)
                        {
                            return "pb::ByteString.Empty";
                        }
                        if (UseLiteRuntime && Descriptor.DefaultValue is ByteString)
                        {
                            string temp = (((ByteString) Descriptor.DefaultValue).ToBase64());
                            return String.Format("pb::ByteString.FromBase64(\"{0}\")", temp);
                        }
                        return string.Format("(pb::ByteString) {0}.Descriptor.Fields[{1}].DefaultValue",
                                             GetClassName(Descriptor.ContainingType), Descriptor.Index);
                    case FieldType.String:
                        if (AllPrintableAscii(Descriptor.Proto.DefaultValue))
                        {
                            // All chars are ASCII and printable.  In this case we only
                            // need to escape quotes and backslashes.
                            return "\"" + Descriptor.Proto.DefaultValue
                                              .Replace("\\", "\\\\")
                                              .Replace("'", "\\'")
                                              .Replace("\"", "\\\"")
                                   + "\"";
                        }
                        if (UseLiteRuntime && Descriptor.DefaultValue is String)
                        {
                            string temp = Convert.ToBase64String(
                                    Encoding.UTF8.GetBytes((String) Descriptor.DefaultValue));
                            return String.Format("pb::ByteString.FromBase64(\"{0}\").ToStringUtf8()", temp);
                        }
                        return string.Format("(string) {0}.Descriptor.Fields[{1}].DefaultValue",
                                             GetClassName(Descriptor.ContainingType), Descriptor.Index);
                    case FieldType.Enum:
                        return TypeName + "." + ((EnumValueDescriptor) Descriptor.DefaultValue).Name;
                    case FieldType.Message:
                    case FieldType.Group:
                        return TypeName + ".DefaultInstance";
                    default:
                        throw new InvalidOperationException("Invalid field descriptor type");
                }
            }
        }

        protected string PropertyName
        {
            get { return Descriptor.CSharpOptions.PropertyName; }
        }

        protected string Name
        {
            get { return NameHelpers.UnderscoresToCamelCase(GetFieldName(Descriptor)); }
        }

        protected int Number
        {
            get { return Descriptor.FieldNumber; }
        }

        protected void AddNullCheck(TextGenerator writer)
        {
            AddNullCheck(writer, "value");
        }

        protected void AddNullCheck(TextGenerator writer, string name)
        {
            if (IsNullableType)
            {
                writer.WriteLine("  pb::ThrowHelper.ThrowIfNull({0}, \"{0}\");", name);
            }
        }

        protected void AddPublicMemberAttributes(TextGenerator writer)
        {
            AddDeprecatedFlag(writer);
            AddClsComplianceCheck(writer);
        }

        protected void AddClsComplianceCheck(TextGenerator writer)
        {
            if (!Descriptor.IsCLSCompliant && Descriptor.File.CSharpOptions.ClsCompliance)
            {
                writer.WriteLine("[global::System.CLSCompliant(false)]");
            }
        }

        protected bool IsObsolete { get { return Descriptor.Options.Deprecated; } }

        /// <summary>
        /// Writes [global::System.ObsoleteAttribute()] if the member is obsolete
        /// </summary>
        protected void AddDeprecatedFlag(TextGenerator writer)
        {
            if (IsObsolete)
            {
                writer.WriteLine("[global::System.ObsoleteAttribute()]");
            }
        }

        /// <summary>
        /// For encodings with fixed sizes, returns that size in bytes.  Otherwise
        /// returns -1. TODO(jonskeet): Make this less ugly.
        /// </summary>
        protected int FixedSize
        {
            get
            {
                switch (Descriptor.FieldType)
                {
                    case FieldType.UInt32:
                    case FieldType.UInt64:
                    case FieldType.Int32:
                    case FieldType.Int64:
                    case FieldType.SInt32:
                    case FieldType.SInt64:
                    case FieldType.Enum:
                    case FieldType.Bytes:
                    case FieldType.String:
                    case FieldType.Message:
                    case FieldType.Group:
                        return -1;
                    case FieldType.Float:
                        return WireFormat.FloatSize;
                    case FieldType.SFixed32:
                        return WireFormat.SFixed32Size;
                    case FieldType.Fixed32:
                        return WireFormat.Fixed32Size;
                    case FieldType.Double:
                        return WireFormat.DoubleSize;
                    case FieldType.SFixed64:
                        return WireFormat.SFixed64Size;
                    case FieldType.Fixed64:
                        return WireFormat.Fixed64Size;
                    case FieldType.Bool:
                        return WireFormat.BoolSize;
                    default:
                        throw new InvalidOperationException("Invalid field descriptor type");
                }
            }
        }

        protected bool IsNullableType
        {
            get
            {
                switch (Descriptor.FieldType)
                {
                    case FieldType.Float:
                    case FieldType.Double:
                    case FieldType.Int32:
                    case FieldType.Int64:
                    case FieldType.SInt32:
                    case FieldType.SInt64:
                    case FieldType.SFixed32:
                    case FieldType.SFixed64:
                    case FieldType.UInt32:
                    case FieldType.UInt64:
                    case FieldType.Fixed32:
                    case FieldType.Fixed64:
                    case FieldType.Bool:
                    case FieldType.Enum:
                        return false;
                    case FieldType.Bytes:
                    case FieldType.String:
                    case FieldType.Message:
                    case FieldType.Group:
                        return true;
                    default:
                        throw new InvalidOperationException("Invalid field descriptor type");
                }
            }
        }

        protected string TypeName
        {
            get
            {
                switch (Descriptor.FieldType)
                {
                    case FieldType.Enum:
                        return GetClassName(Descriptor.EnumType);
                    case FieldType.Message:
                    case FieldType.Group:
                        return GetClassName(Descriptor.MessageType);
                    default:
                        return DescriptorUtil.GetMappedTypeName(Descriptor.MappedType);
                }
            }
        }

        protected string MessageOrGroup
        {
            get { return Descriptor.FieldType == FieldType.Group ? "Group" : "Message"; }
        }

        /// <summary>
        /// Returns the type name as used in CodedInputStream method names: SFixed32, UInt32 etc.
        /// </summary>
        protected string CapitalizedTypeName
        {
            get
            {
                // Our enum names match perfectly. How serendipitous.
                return Descriptor.FieldType.ToString();
            }
        }
    }
}