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
using System.Collections.Generic;

namespace Google.Protobuf.Reflection
{
    /// <summary>
    /// Descriptor for an enum type in a .proto file.
    /// </summary>
    public sealed class EnumDescriptor : DescriptorBase
    {
        private readonly EnumDescriptorProto proto;
        private readonly MessageDescriptor containingType;
        private readonly IList<EnumValueDescriptor> values;
        private readonly Type clrType;

        internal EnumDescriptor(EnumDescriptorProto proto, FileDescriptor file, MessageDescriptor parent, int index, Type clrType)
            : base(file, file.ComputeFullName(parent, proto.Name), index)
        {
            this.proto = proto;
            this.clrType = clrType;
            containingType = parent;

            if (proto.Value.Count == 0)
            {
                // We cannot allow enums with no values because this would mean there
                // would be no valid default value for fields of this type.
                throw new DescriptorValidationException(this, "Enums must contain at least one value.");
            }

            values = DescriptorUtil.ConvertAndMakeReadOnly(proto.Value,
                                                           (value, i) => new EnumValueDescriptor(value, file, this, i));

            File.DescriptorPool.AddSymbol(this);
        }

        internal EnumDescriptorProto Proto { get { return proto; } }

        /// <summary>
        /// The brief name of the descriptor's target.
        /// </summary>
        public override string Name { get { return proto.Name; } }

        /// <summary>
        /// The CLR type for this enum. For generated code, this will be a CLR enum type.
        /// </summary>
        public Type ClrType { get { return clrType; } }

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
        /// Finds an enum value by number. If multiple enum values have the
        /// same number, this returns the first defined value with that number.
        /// If there is no value for the given number, this returns <c>null</c>.
        /// </summary>
        public EnumValueDescriptor FindValueByNumber(int number)
        {
            return File.DescriptorPool.FindEnumValueByNumber(this, number);
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
    }
}