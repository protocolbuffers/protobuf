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

namespace Google.Protobuf
{
#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
    /// <summary>
    /// Interface for a Protocol Buffers message, supporting
    /// <see cref="CodedInputReader"/> and <see cref="CodedOutputWriter"/>
    /// serialization operations.
    /// </summary>
    public interface IBufferMessage : IMessage
    {
        /// <summary>
        /// Merges the data from the specified <see cref="CodedInputReader"/> with the current message.
        /// </summary>
        /// <remarks>See the user guide for precise merge semantics.</remarks>
        /// <param name="input"><see cref="CodedInputReader"/> to read data from. Must not be null.</param>
        void MergeFrom(ref CodedInputReader input);

        /// <summary>
        /// Internal implementation of merging data from given parse context into this message.
        /// Users should never invoke this method directly.
        /// </summary>        
        void MergeFrom_Internal(ref ParseContext ctx);

        /// <summary>
        /// Writes the data to the given <see cref="CodedOutputWriter"/>.
        /// </summary>
        /// <param name="output"><see cref="CodedOutputWriter"/> to write the data to. Must not be null.</param>
        void WriteTo(ref CodedOutputWriter output);
    }
#endif
}
