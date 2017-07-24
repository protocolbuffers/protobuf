#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2017 Google Inc.  All rights reserved.
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

#if !PROTOBUF_NO_ASYNC
using System.Threading;
using System.Threading.Tasks;

namespace Google.Protobuf
{
    /// <summary>
    /// Extension interface for a Protocol Buffers message, supporting
    /// basic asynchronous operations required for serialization.
    /// </summary>
    public interface IAsyncMessage : IMessage
    {
        /// <summary>
        /// Merges the data from the specified coded input stream with the current message.
        /// </summary>
        /// <remarks>See the user guide for precise merge semantics.</remarks>
        /// <param name="input">Coded input stream to read the data from. Must not be null.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        Task MergeFromAsync(CodedInputStream input, CancellationToken cancellationToken);

        /// <summary>
        /// Writes the data to the given coded output stream.
        /// </summary>
        /// <param name="output">Coded output stream to write the data to. Must not be null.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        Task WriteToAsync(CodedOutputStream output, CancellationToken cancellationToken);
    }

    /// <summary>
    /// Generic interface for a Protocol Buffers message,
    /// where the type parameter is expected to be the same type as
    /// the implementation class.
    /// </summary>
    /// <typeparam name="T">The message type.</typeparam>
    public interface IAsyncMessage<T> : IMessage<T>, IAsyncMessage where T : IAsyncMessage<T> { }
}

#endif