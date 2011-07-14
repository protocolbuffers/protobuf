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
using System.Collections.Generic;
using Google.ProtocolBuffers.DescriptorProtos;

namespace Google.ProtocolBuffers.Descriptors
{
    /// <summary>
    /// Descriptor for an enum type in a .proto file.
    /// </summary>
    public sealed class EnumDescriptor : IndexedDescriptorBase<EnumDescriptorProto, EnumOptions>,
                                         IEnumLiteMap<EnumValueDescriptor>
    {
        private readonly MessageDescriptor containingType;
        private readonly IList<EnumValueDescriptor> values;

        internal EnumDescriptor(EnumDescriptorProto proto, FileDescriptor file, MessageDescriptor parent, int index)
            : base(proto, file, ComputeFullName(file, parent, proto.Name), index)
        {
            containingType = parent;

            if (proto.ValueCount == 0)
            {
                // We cannot allow enums with no values because this would mean there
                // would be no valid default value for fields of this type.
                throw new DescriptorValidationException(this, "Enums must contain at least one value.");
            }

            values = DescriptorUtil.ConvertAndMakeReadOnly(proto.ValueList,
                                                           (value, i) => new EnumValueDescriptor(value, file, this, i));

            File.DescriptorPool.AddSymbol(this);
        }

        /// <value>
        /// If this is a nested type, get the outer descriptor, otherwise null.
        /// </value>
        public MessageDescriptor ContainingType
        {
            get { return containingType; }
        }

        /// <value>
        /// An unmodifiable list of defined value descriptors for this enum.
        /// </value>
        public IList<EnumValueDescriptor> Values
        {
            get { return values; }
        }

        /// <summary>
        /// Logic moved from FieldSet to continue current behavior
        /// </summary>
        public bool IsValidValue(IEnumLite value)
        {
            return value is EnumValueDescriptor && ((EnumValueDescriptor) value).EnumDescriptor == this;
        }

        /// <summary>
        /// Finds an enum value by number. If multiple enum values have the
        /// same number, this returns the first defined value with that number.
        /// </summary>
        public EnumValueDescriptor FindValueByNumber(int number)
        {
            return File.DescriptorPool.FindEnumValueByNumber(this, number);
        }

        IEnumLite IEnumLiteMap.FindValueByNumber(int number)
        {
            return FindValueByNumber(number);
        }

        IEnumLite IEnumLiteMap.FindValueByName(string name)
        {
            return FindValueByName(name);
        }

        /// <summary>
        /// Finds an enum value by name.
        /// </summary>
        /// <param name="name">The unqualified name of the value (e.g. "FOO").</param>
        /// <returns>The value's descriptor, or null if not found.</returns>
        public EnumValueDescriptor FindValueByName(string name)
        {
            return File.DescriptorPool.FindSymbol<EnumValueDescriptor>(FullName + "." + name);
        }

        internal override void ReplaceProto(EnumDescriptorProto newProto)
        {
            base.ReplaceProto(newProto);
            for (int i = 0; i < values.Count; i++)
            {
                values[i].ReplaceProto(newProto.GetValue(i));
            }
        }
    }
}