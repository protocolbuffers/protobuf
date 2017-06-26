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
using System.Threading;
using System.Threading.Tasks;

namespace Google.Protobuf
{
    /// <summary>
    /// Delegate type for FieldCodec asynchronous read handler
    /// </summary>
    public delegate Task<T> AsyncReadDelegate<T>(CodedInputStream inputStream, CancellationToken cancellationToken);

    /// <summary>
    /// Delegate type for FieldCodec asynchronous write handler
    /// </summary>
    public delegate Task AsyncWriteDelegate<T>(CodedOutputStream outputStream, T value, CancellationToken cancellationToken);

    public static partial class FieldCodec
    {
        private static class WrapperCodecsAsync
        {
            internal static async Task<T> ReadAsync<T>(CodedInputStream input, FieldCodec<T> codec, CancellationToken cancellationToken)
            {
                int length = await input.ReadLengthAsync(cancellationToken).ConfigureAwait(false);
                int oldLimit = input.PushLimit(length);

                uint tag;
                T value = codec.DefaultValue;
                while ((tag = await input.ReadTagAsync(cancellationToken).ConfigureAwait(false)) != 0)
                {
                    if (tag == codec.Tag)
                    {
                        value = await codec.ReadAsync(input, cancellationToken).ConfigureAwait(false);
                    }
                    else
                    {
                        await input.SkipLastFieldAsync(cancellationToken).ConfigureAwait(false);
                    }

                }
                input.CheckReadEndOfStreamTag();
                input.PopLimit(oldLimit);

                return value;
            }

            internal static async Task WriteAsync<T>(CodedOutputStream output, T value, FieldCodec<T> codec, CancellationToken cancellationToken)
            {
                await output.WriteLengthAsync(codec.CalculateSizeWithTag(value), cancellationToken).ConfigureAwait(false);
                await codec.WriteTagAndValueAsync(output, value, cancellationToken).ConfigureAwait(false);
            }
        }
    }

    public sealed partial class FieldCodec<T>
    {
        /// <summary>
        /// Returns a delegate to write a value (unconditionally) to a coded output stream.
        /// </summary>
        internal AsyncWriteDelegate<T> ValueAsyncWriter { get; }

        /// <summary>
        /// Returns a delegate to read a value from a coded input stream. It is assumed that
        /// the stream is already positioned on the appropriate tag.
        /// </summary>
        internal AsyncReadDelegate<T> ValueAsyncReader { get; }
        
        internal FieldCodec(
                Func<CodedInputStream, T> reader,
                Action<CodedOutputStream, T> writer,
                AsyncReadDelegate<T> asyncReader,
                AsyncWriteDelegate<T> asyncWriter,
                int fixedSize,
                uint tag) : this(reader, writer, asyncReader, asyncWriter, _ => fixedSize, tag)
        {
            FixedSize = fixedSize;
        }

        internal FieldCodec(
            Func<CodedInputStream, T> reader,
            Action<CodedOutputStream, T> writer,
            AsyncReadDelegate<T> asyncReader,
            AsyncWriteDelegate<T> asyncWriter,
            Func<T, int> sizeCalculator,
            uint tag) : this(reader, writer, asyncReader, asyncWriter, sizeCalculator, tag, DefaultDefault)
        {
        }

        internal FieldCodec(
            Func<CodedInputStream, T> reader,
            Action<CodedOutputStream, T> writer,
            AsyncReadDelegate<T> asyncReader,
            AsyncWriteDelegate<T> asyncWriter,
            Func<T, int> sizeCalculator,
            uint tag,
            T defaultValue)
        {
            ValueReader = reader;
            ValueAsyncReader = asyncReader;
            ValueWriter = writer;
            ValueAsyncWriter = asyncWriter;
            ValueSizeCalculator = sizeCalculator;
            FixedSize = 0;
            Tag = tag;
            DefaultValue = defaultValue;
            tagSize = CodedOutputStream.ComputeRawVarint32Size(tag);
            // Detect packed-ness once, so we can check for it within RepeatedField<T>.
            PackedRepeatedField = IsPackedRepeatedField(tag);
        }

        /// <summary>
        /// Write a tag and the given value, *if* the value is not the default.
        /// </summary>
        public async Task WriteTagAndValueAsync(CodedOutputStream output, T value, CancellationToken cancellationToken)
        {
            if (!IsDefault(value))
            {
                await output.WriteTagAsync(Tag, cancellationToken).ConfigureAwait(false);
                await ValueAsyncWriter(output, value, cancellationToken).ConfigureAwait(false);
            }
        }

        /// <summary>
        /// Reads a value of the codec type from the given <see cref="CodedInputStream"/>.
        /// </summary>
        /// <param name="input">The input stream to read from.</param>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>The value read from the stream.</returns>
        public Task<T> ReadAsync(CodedInputStream input, CancellationToken cancellationToken) => ValueAsyncReader(input, cancellationToken);
    }
}
#endif