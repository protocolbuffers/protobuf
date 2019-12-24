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
using System.Collections;

namespace Google.Protobuf.Reflection
{
    /// <summary>
    /// Allows fields to be reflectively accessed.
    /// </summary>
    public interface IFieldAccessor
    {
        /// <summary>
        /// Returns the descriptor associated with this field.
        /// </summary>
        FieldDescriptor Descriptor { get; }

        /// <summary>
        /// Clears the field in the specified message. (For repeated fields,
        /// this clears the list.)
        /// </summary>
        void Clear(IMessage message);

        /// <summary>
        /// Fetches the field value. For repeated values, this will be an
        /// <see cref="IList"/> implementation. For map values, this will be an
        /// <see cref="IDictionary"/> implementation.
        /// </summary>
        object GetValue(IMessage message);

        /// <summary>
        /// Indicates whether the field in the specified message is set. For proto3 fields, this throws an <see cref="InvalidOperationException"/>
        /// </summary>
        bool HasValue(IMessage message);

        /// <summary>
        /// Mutator for single "simple" fields only.
        /// </summary>
        /// <remarks>
        /// Repeated fields are mutated by fetching the value and manipulating it as a list.
        /// Map fields are mutated by fetching the value and manipulating it as a dictionary.
        /// </remarks>
        /// <exception cref="InvalidOperationException">The field is not a "simple" field.</exception>
        void SetValue(IMessage message, object value);
    }
}