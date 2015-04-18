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

namespace Google.ProtocolBuffers.FieldAccess
{
    /// <summary>
    /// Allows fields to be reflectively accessed in a smart manner.
    /// The property descriptors for each field are created once and then cached.
    /// In addition, this interface holds knowledge of repeated fields, builders etc.
    /// </summary>
    internal interface IFieldAccessor<TMessage, TBuilder>
        where TMessage : IMessage<TMessage, TBuilder>
        where TBuilder : IBuilder<TMessage, TBuilder>
    {
        /// <summary>
        /// Indicates whether the specified message contains the field.
        /// </summary>
        bool Has(TMessage message);

        /// <summary>
        /// Gets the count of the repeated field in the specified message.
        /// </summary>
        int GetRepeatedCount(TMessage message);

        /// <summary>
        /// Clears the field in the specified builder.
        /// </summary>
        /// <param name="builder"></param>
        void Clear(TBuilder builder);

        /// <summary>
        /// Creates a builder for the type of this field (which must be a message field).
        /// </summary>
        IBuilder CreateBuilder();

        /// <summary>
        /// Accessor for single fields
        /// </summary>
        object GetValue(TMessage message);

        /// <summary>
        /// Mutator for single fields
        /// </summary>
        void SetValue(TBuilder builder, object value);

        /// <summary>
        /// Accessor for repeated fields
        /// </summary>
        object GetRepeatedValue(TMessage message, int index);

        /// <summary>
        /// Mutator for repeated fields
        /// </summary>
        void SetRepeated(TBuilder builder, int index, object value);

        /// <summary>
        /// Adds the specified value to the field in the given builder.
        /// </summary>
        void AddRepeated(TBuilder builder, object value);

        /// <summary>
        /// Returns a read-only wrapper around the value of a repeated field.
        /// </summary>
        object GetRepeatedWrapper(TBuilder builder);
    }
}