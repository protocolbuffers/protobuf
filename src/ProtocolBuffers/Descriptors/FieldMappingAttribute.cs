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

namespace Google.ProtocolBuffers.Descriptors
{
    /// <summary>
    /// Defined specifically for the <see cref="FieldType" /> enumeration,
    /// this allows each field type to specify the mapped type and wire type.
    /// </summary>
    [CLSCompliant(false)]
    [AttributeUsage(AttributeTargets.Field)]
    public sealed class FieldMappingAttribute : Attribute
    {
        public FieldMappingAttribute(MappedType mappedType, WireFormat.WireType wireType)
        {
            MappedType = mappedType;
            WireType = wireType;
        }

        public MappedType MappedType { get; private set; }
        public WireFormat.WireType WireType { get; private set; }


        /// <summary>
        /// Immutable mapping from field type to mapped type. Built using the attributes on
        /// FieldType values.
        /// </summary>
        private static readonly IDictionary<FieldType, FieldMappingAttribute> FieldTypeToMappedTypeMap = MapFieldTypes();

        private static IDictionary<FieldType, FieldMappingAttribute> MapFieldTypes()
        {
            var map = new Dictionary<FieldType, FieldMappingAttribute>();
            foreach (FieldInfo field in typeof (FieldType).GetFields(BindingFlags.Static | BindingFlags.Public))
            {
                FieldType fieldType = (FieldType) field.GetValue(null);
                FieldMappingAttribute mapping =
                    (FieldMappingAttribute) field.GetCustomAttributes(typeof (FieldMappingAttribute), false)[0];
                map[fieldType] = mapping;
            }
            return Dictionaries.AsReadOnly(map);
        }

        internal static MappedType MappedTypeFromFieldType(FieldType type)
        {
            return FieldTypeToMappedTypeMap[type].MappedType;
        }

        internal static WireFormat.WireType WireTypeFromFieldType(FieldType type, bool packed)
        {
            return packed ? WireFormat.WireType.LengthDelimited : FieldTypeToMappedTypeMap[type].WireType;
        }
    }
}