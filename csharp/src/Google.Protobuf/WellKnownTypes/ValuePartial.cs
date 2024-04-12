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

namespace Google.Protobuf.WellKnownTypes
{
    public partial class Value
    {
        /// <summary>
        /// Convenience method to create a Value message with a string value.
        /// </summary>
        /// <param name="value">Value to set for the StringValue property.</param>
        /// <returns>A newly-created Value message with the given value.</returns>
        public static Value ForString(string value)
        {
            ProtoPreconditions.CheckNotNull(value, nameof(value));
            return new Value { StringValue = value };
        }

        /// <summary>
        /// Convenience method to create a Value message with a number value.
        /// </summary>
        /// <param name="value">Value to set for the NumberValue property.</param>
        /// <returns>A newly-created Value message with the given value.</returns>
        public static Value ForNumber(double value)
        {
            return new Value { NumberValue = value };
        }

        /// <summary>
        /// Convenience method to create a Value message with a Boolean value.
        /// </summary>
        /// <param name="value">Value to set for the BoolValue property.</param>
        /// <returns>A newly-created Value message with the given value.</returns>
        public static Value ForBool(bool value)
        {
            return new Value { BoolValue = value };
        }

        /// <summary>
        /// Convenience method to create a Value message with a null initial value.
        /// </summary>
        /// <returns>A newly-created Value message a null initial value.</returns>
        public static Value ForNull()
        {
            return new Value { NullValue = 0 };
        }

        /// <summary>
        /// Convenience method to create a Value message with an initial list of values.
        /// </summary>
        /// <remarks>The values provided are not cloned; the references are copied directly.</remarks>
        /// <returns>A newly-created Value message an initial list value.</returns>
        public static Value ForList(params Value[] values)
        {
            ProtoPreconditions.CheckNotNull(values, nameof(values));
            return new Value { ListValue = new ListValue { Values = { values } } };
        }

        /// <summary>
        /// Convenience method to create a Value message with an initial struct value
        /// </summary>
        /// <remarks>The value provided is not cloned; the reference is copied directly.</remarks>
        /// <returns>A newly-created Value message an initial struct value.</returns>
        public static Value ForStruct(Struct value)
        {
            ProtoPreconditions.CheckNotNull(value, nameof(value));
            return new Value { StructValue = value };
        }
    }
}
