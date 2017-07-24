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
using System.IO;
using System.Threading;
using System.Threading.Tasks;

namespace Google.Protobuf
{
    /// <summary>
    /// Extension methods on <see cref="IAsyncMessage"/> and <see cref="IAsyncMessage{T}"/>.
    /// </summary>
    public static class AsyncMessageExtensions
    {
        /// <summary>
        /// Merges data from the given byte array into an existing message.
        /// </summary>
        /// <param name="message">The message to merge the data into.</param>
        /// <param name="data">The data to merge, which must be protobuf-encoded binary data.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        public static async Task MergeFrom(this IAsyncMessage message, byte[] data, CancellationToken cancellationToken)
        {
            ProtoPreconditions.CheckNotNull(message, "message");
            ProtoPreconditions.CheckNotNull(data, "data");
            CodedInputStream input = new CodedInputStream(data);
            await message.MergeFromAsync(input, cancellationToken).ConfigureAwait(false);
            input.CheckReadEndOfStreamTag();
        }

        /// <summary>
        /// Merges data from the given stream into an existing message.
        /// </summary>
        /// <param name="message">The message to merge the data into.</param>
        /// <param name="input">Stream containing the data to merge, which must be protobuf-encoded binary data.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        public static async Task MergeFromAsync(this IAsyncMessage message, Stream input, CancellationToken cancellationToken)
        {
            ProtoPreconditions.CheckNotNull(message, "message");
            ProtoPreconditions.CheckNotNull(input, "input");
            CodedInputStream codedInput = new CodedInputStream(input);
            await message.MergeFromAsync(codedInput, cancellationToken).ConfigureAwait(false);
            codedInput.CheckReadEndOfStreamTag();
        }

        /// <summary>
        /// Merges length-delimited data from the given stream into an existing message.
        /// </summary>
        /// <remarks>
        /// The stream is expected to contain a length and then the data. Only the amount of data
        /// specified by the length will be consumed.
        /// </remarks>
        /// <param name="message">The message to merge the data into.</param>
        /// <param name="input">Stream containing the data to merge, which must be protobuf-encoded binary data.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        public static async Task MergeDelimitedFromAsync(this IAsyncMessage message, Stream input, CancellationToken cancellationToken)
        {
            ProtoPreconditions.CheckNotNull(message, "message");
            ProtoPreconditions.CheckNotNull(input, "input");
            int size = (int) await CodedInputStream.ReadRawVarint32Async(input, cancellationToken).ConfigureAwait(false);
            Stream limitedStream = new LimitedInputStream(input, size);
            await message.MergeFromAsync(limitedStream, cancellationToken).ConfigureAwait(false);
        }

        /// <summary>
        /// Writes the given message data to the given stream in protobuf encoding.
        /// </summary>
        /// <param name="message">The message to write to the stream.</param>
        /// <param name="output">The stream to write to.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        public static async Task WriteToAsync(this IAsyncMessage message, Stream output, CancellationToken cancellationToken)
        {
            ProtoPreconditions.CheckNotNull(message, "message");
            ProtoPreconditions.CheckNotNull(output, "output");
            CodedOutputStream codedOutput = new CodedOutputStream(output);
            await message.WriteToAsync(codedOutput, cancellationToken).ConfigureAwait(false);
            await codedOutput.FlushAsync(cancellationToken).ConfigureAwait(false);
        }

        /// <summary>
        /// Writes the length and then data of the given message to a stream.
        /// </summary>
        /// <param name="message">The message to write.</param>
        /// <param name="output">The output stream to write to.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        public static async Task WriteDelimitedToAsync(this IAsyncMessage message, Stream output, CancellationToken cancellationToken)
        {
            ProtoPreconditions.CheckNotNull(message, "message");
            ProtoPreconditions.CheckNotNull(output, "output");
            CodedOutputStream codedOutput = new CodedOutputStream(output);
            await codedOutput.WriteRawVarint32Async((uint)message.CalculateSize(), cancellationToken).ConfigureAwait(false);
            await message.WriteToAsync(codedOutput, cancellationToken).ConfigureAwait(false);
            await codedOutput.FlushAsync(cancellationToken).ConfigureAwait(false);
        }
    }
}
#endif