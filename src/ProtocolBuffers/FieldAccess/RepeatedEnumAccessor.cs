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
using System.Collections;
using System.Collections.Generic;
using Google.ProtocolBuffers.Collections;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers.FieldAccess
{
    /// <summary>
    /// Accessor for a repeated enum field.
    /// </summary>
    internal sealed class RepeatedEnumAccessor<TMessage, TBuilder> : RepeatedPrimitiveAccessor<TMessage, TBuilder>
        where TMessage : IMessage<TMessage, TBuilder>
        where TBuilder : IBuilder<TMessage, TBuilder>
    {
        private readonly EnumDescriptor enumDescriptor;

        internal RepeatedEnumAccessor(FieldDescriptor field, string name) : base(name)
        {
            enumDescriptor = field.EnumType;
        }

        public override object GetValue(TMessage message)
        {
            List<EnumValueDescriptor> ret = new List<EnumValueDescriptor>();
            foreach (int rawValue in (IEnumerable) base.GetValue(message))
            {
                ret.Add(enumDescriptor.FindValueByNumber(rawValue));
            }
            return Lists.AsReadOnly(ret);
        }

        public override object GetRepeatedValue(TMessage message, int index)
        {
            // Note: This relies on the fact that the CLR allows unboxing from an enum to
            // its underlying value
            int rawValue = (int) base.GetRepeatedValue(message, index);
            return enumDescriptor.FindValueByNumber(rawValue);
        }

        public override void AddRepeated(TBuilder builder, object value)
        {
            ThrowHelper.ThrowIfNull(value, "value");
            base.AddRepeated(builder, ((EnumValueDescriptor) value).Number);
        }

        public override void SetRepeated(TBuilder builder, int index, object value)
        {
            ThrowHelper.ThrowIfNull(value, "value");
            base.SetRepeated(builder, index, ((EnumValueDescriptor) value).Number);
        }
    }
}