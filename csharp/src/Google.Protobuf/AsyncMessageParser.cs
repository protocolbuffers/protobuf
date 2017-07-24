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
using System;
using System.IO;
using System.Threading;
using System.Threading.Tasks;

namespace Google.Protobuf
{
    /// <summary>
    /// A general async message parser, typically used by reflection-based code as all the methods
    /// return simple <see cref="IAsyncMessage"/>.
    /// </summary>
    public interface IAsyncMessageParser
    {
        /// <summary>
        /// Parses a message from a byte array.
        /// </summary>
        /// <param name="data">The byte array containing the message. Must not be null.</param>
        /// <returns>The newly parsed message.</returns>
        IAsyncMessage ParseFrom(byte[] data);

        /// <summary>
        /// Parses a message from a byte array.
        /// </summary>
        /// <param name="data">The byte array containing the message. Must not be null.</param>
        /// <returns>The newly parsed message.</returns>
        IAsyncMessage ParseFrom(ByteString data);

        /// <summary>
        /// Parses a message from the given stream.
        /// </summary>
        /// <param name="input">The stream to parse.</param>
        /// <returns>The parsed message.</returns>
        IAsyncMessage ParseFrom(Stream input);

        /// <summary>
        /// Parses a message from the given stream.
        /// </summary>
        /// <param name="input">The stream to parse.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>The parsed message.</returns>
        Task<IAsyncMessage> ParseFromAsync(Stream input, CancellationToken cancellationToken);

        /// <summary>
        /// Parses a length-delimited message from the given stream.
        /// </summary>
        /// <remarks>
        /// The stream is expected to contain a length and then the data. Only the amount of data
        /// specified by the length will be consumed.
        /// </remarks>
        /// <param name="input">The stream to parse.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>The parsed message.</returns>
        Task<IAsyncMessage> ParseDelimitedFromAsync(Stream input, CancellationToken cancellationToken);

        /// <summary>
        /// Parses a length-delimited message from the given stream.
        /// </summary>
        /// <remarks>
        /// The stream is expected to contain a length and then the data. Only the amount of data
        /// specified by the length will be consumed.
        /// </remarks>
        /// <param name="input">The stream to parse.</param>
        /// <returns>The parsed message.</returns>
        IAsyncMessage ParseDelimitedFrom(Stream input);

        /// <summary>
        /// Parses a message from the given coded input stream.
        /// </summary>
        /// <param name="input">The stream to parse.</param>
        /// <returns>The parsed message.</returns>
        IAsyncMessage ParseFrom(CodedInputStream input);

        /// <summary>
        /// Parses a message from the given coded input stream.
        /// </summary>
        /// <param name="input">The stream to parse.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>The parsed message.</returns>
        Task<IAsyncMessage> ParseFromAsync(CodedInputStream input, CancellationToken cancellationToken);

        /// <summary>
        /// Parses a message from the given JSON.
        /// </summary>
        /// <param name="json">The JSON to parse.</param>
        /// <returns>The parsed message.</returns>
        /// <exception cref="InvalidJsonException">The JSON does not comply with RFC 7159</exception>
        /// <exception cref="InvalidProtocolBufferException">The JSON does not represent a Protocol Buffers message correctly</exception>
        IAsyncMessage ParseJson(string json);
    }

    /// <summary>
    /// A parser for a specific message type.
    /// </summary>
    /// <remarks>
    /// <p>
    /// This delegates most behavior to the
    /// <see cref="IMessage.MergeFrom"/> implementation within the original type, but
    /// provides convenient overloads to parse from a variety of sources.
    /// </p>
    /// <p>
    /// Most applications will never need to create their own instances of this type;
    /// instead, use the static <c>Parser</c> property of a generated message type to obtain a
    /// parser for that type.
    /// </p>
    /// </remarks>
    /// <typeparam name="T">The type of message to be parsed.</typeparam>
    public sealed class AsyncMessageParser<T> : MessageParser<T>, IAsyncMessageParser where T : IAsyncMessage<T>
    {
        /// <summary>
        /// Creates a new parser.
        /// </summary>
        /// <remarks>
        /// The factory method is effectively an optimization over using a generic constraint
        /// to require a parameterless constructor: delegates are significantly faster to execute.
        /// </remarks>
        /// <param name="factory">Function to invoke when a new, empty message is required.</param>
        public AsyncMessageParser(Func<T> factory) : base(factory) { }

        /// <summary>
        /// Parses a message from the given coded input stream.
        /// </summary>
        /// <param name="input">The stream to parse.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>The parsed message.</returns>
        public async Task<T> ParseFromAsync(CodedInputStream input, CancellationToken cancellationToken)
        {
            T message = CreateTemplate();
            await message.MergeFromAsync(input, cancellationToken).ConfigureAwait(false);
            return message;
        }

        /// <summary>
        /// Parses a message from the given stream.
        /// </summary>
        /// <param name="input">The stream to parse.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>The parsed message.</returns>
        public async Task<T> ParseFromAsync(Stream input, CancellationToken cancellationToken)
        {
            T message = CreateTemplate();
            await message.MergeFromAsync(input, cancellationToken).ConfigureAwait(false);
            return message;
        }

        /// <summary>
        /// Parses a length-delimited message from the given stream.
        /// </summary>
        /// <remarks>
        /// The stream is expected to contain a length and then the data. Only the amount of data
        /// specified by the length will be consumed.
        /// </remarks>
        /// <param name="input">The stream to parse.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>The parsed message.</returns>
        public async Task<T> ParseDelimitedFromAsync(Stream input, CancellationToken cancellationToken)
        {
            T message = CreateTemplate();
            await message.MergeFromAsync(input, cancellationToken).ConfigureAwait(false);
            return message;
        }

        /// <inheritdoc />
        IAsyncMessage IAsyncMessageParser.ParseFrom(byte[] data) => ParseFrom(data);
        /// <inheritdoc />
        IAsyncMessage IAsyncMessageParser.ParseFrom(ByteString data) => ParseFrom(data);
        /// <inheritdoc />
        IAsyncMessage IAsyncMessageParser.ParseFrom(Stream input) => ParseFrom(input);
        /// <inheritdoc />
        async Task<IAsyncMessage> IAsyncMessageParser.ParseFromAsync(Stream input, CancellationToken cancellationToken) => 
            await ParseFromAsync(input, cancellationToken).ConfigureAwait(false);
        /// <inheritdoc />
        IAsyncMessage IAsyncMessageParser.ParseDelimitedFrom(Stream input) => ParseDelimitedFrom(input);
        /// <inheritdoc />
        async Task<IAsyncMessage> IAsyncMessageParser.ParseDelimitedFromAsync(Stream input, CancellationToken cancellationToken) =>
            await ParseDelimitedFromAsync(input, cancellationToken).ConfigureAwait(false);
        /// <inheritdoc />
        IAsyncMessage IAsyncMessageParser.ParseFrom(CodedInputStream input) => ParseFrom(input);
        /// <inheritdoc />
        async Task<IAsyncMessage> IAsyncMessageParser.ParseFromAsync(CodedInputStream input, CancellationToken cancellationToken) => 
            await ParseFromAsync(input, cancellationToken).ConfigureAwait(false);
        /// <inheritdoc />
        IAsyncMessage IAsyncMessageParser.ParseJson(string json) => ParseJson(json);
    }
}

#endif