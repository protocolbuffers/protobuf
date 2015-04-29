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
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers.FieldAccess
{
    /// <summary>
    /// Accessor for fields representing a non-repeated enum value.
    /// </summary>
    internal sealed class SingleEnumAccessor<TMessage, TBuilder> : SinglePrimitiveAccessor<TMessage, TBuilder>
        where TMessage : IMessage<TMessage, TBuilder>
        where TBuilder : IBuilder<TMessage, TBuilder>
    {
        private readonly EnumDescriptor enumDescriptor;

        internal SingleEnumAccessor(FieldDescriptor field, string name, bool supportFieldPresence) : base(field, name, supportFieldPresence)
        {
            enumDescriptor = field.EnumType;
        }

        /// <summary>
        /// Returns an EnumValueDescriptor representing the value in the builder.
        /// Note that if an enum has multiple values for the same number, the descriptor
        /// for the first value with that number will be returned.
        /// </summary>
        public override object GetValue(TMessage message)
        {
            // Note: This relies on the fact that the CLR allows unboxing from an enum to
            // its underlying value
            int rawValue = (int) base.GetValue(message);
            return enumDescriptor.FindValueByNumber(rawValue);
        }

        /// <summary>
        /// Sets the value as an enum (via an int) in the builder,
        /// from an EnumValueDescriptor parameter.
        /// </summary>
        public override void SetValue(TBuilder builder, object value)
        {
            ThrowHelper.ThrowIfNull(value, "value");
            EnumValueDescriptor valueDescriptor = (EnumValueDescriptor) value;
            base.SetValue(builder, valueDescriptor.Number);
        }
    }
}