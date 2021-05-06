#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
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

using Google.Protobuf.Reflection;
using System.Buffers;
using System.Collections;
using System;
using System.IO;
using System.Linq;
using System.Security;

namespace Google.Protobuf
{
    /// <summary>
    /// Extension methods on <see cref="IMessage"/> and <see cref="IMessage{T}"/>.
    /// </summary>
    public static class MessageExtensions
    {
        /// <summary>
        /// Merges data from the given byte array into an existing message.
        /// </summary>
        /// <param name="message">The message to merge the data into.</param>
        /// <param name="data">The data to merge, which must be protobuf-encoded binary data.</param>
        public static void MergeFrom(this IMessage message, byte[] data) =>
            MergeFrom(message, data, false, null);

        /// <summary>
        /// Merges data from the given byte array slice into an existing message.
        /// </summary>
        /// <param name="message">The message to merge the data into.</param>
        /// <param name="data">The data containing the slice to merge, which must be protobuf-encoded binary data.</param>
        /// <param name="offset">The offset of the slice to merge.</param>
        /// <param name="length">The length of the slice to merge.</param>
        public static void MergeFrom(this IMessage message, byte[] data, int offset, int length) =>
            MergeFrom(message, data, offset, length, false, null);

        /// <summary>
        /// Merges data from the given byte string into an existing message.
        /// </summary>
        /// <param name="message">The message to merge the data into.</param>
        /// <param name="data">The data to merge, which must be protobuf-encoded binary data.</param>
        public static void MergeFrom(this IMessage message, ByteString data) =>
            MergeFrom(message, data, false, null);

        /// <summary>
        /// Merges data from the given stream into an existing message.
        /// </summary>
        /// <param name="message">The message to merge the data into.</param>
        /// <param name="input">Stream containing the data to merge, which must be protobuf-encoded binary data.</param>
        public static void MergeFrom(this IMessage message, Stream input) =>
            MergeFrom(message, input, false, null);

        /// <summary>
        /// Merges data from the given span into an existing message.
        /// </summary>
        /// <param name="message">The message to merge the data into.</param>
        /// <param name="span">Span containing the data to merge, which must be protobuf-encoded binary data.</param>
        [SecuritySafeCritical]
        public static void MergeFrom(this IMessage message, ReadOnlySpan<byte> span) =>
            MergeFrom(message, span, false, null);

        /// <summary>
        /// Merges length-delimited data from the given stream into an existing message.
        /// </summary>
        /// <remarks>
        /// The stream is expected to contain a length and then the data. Only the amount of data
        /// specified by the length will be consumed.
        /// </remarks>
        /// <param name="message">The message to merge the data into.</param>
        /// <param name="input">Stream containing the data to merge, which must be protobuf-encoded binary data.</param>
        public static void MergeDelimitedFrom(this IMessage message, Stream input) =>
            MergeDelimitedFrom(message, input, false, null);

        /// <summary>
        /// Converts the given message into a byte array in protobuf encoding.
        /// </summary>
        /// <param name="message">The message to convert.</param>
        /// <returns>The message data as a byte array.</returns>
        public static byte[] ToByteArray(this IMessage message)
        {
            ProtoPreconditions.CheckNotNull(message, "message");
            byte[] result = new byte[message.CalculateSize()];
            CodedOutputStream output = new CodedOutputStream(result);
            message.WriteTo(output);
            output.CheckNoSpaceLeft();
            return result;
        }

        /// <summary>
        /// Writes the given message data to the given stream in protobuf encoding.
        /// </summary>
        /// <param name="message">The message to write to the stream.</param>
        /// <param name="output">The stream to write to.</param>
        public static void WriteTo(this IMessage message, Stream output)
        {
            ProtoPreconditions.CheckNotNull(message, "message");
            ProtoPreconditions.CheckNotNull(output, "output");
            CodedOutputStream codedOutput = new CodedOutputStream(output);
            message.WriteTo(codedOutput);
            codedOutput.Flush();
        }

        /// <summary>
        /// Writes the length and then data of the given message to a stream.
        /// </summary>
        /// <param name="message">The message to write.</param>
        /// <param name="output">The output stream to write to.</param>
        public static void WriteDelimitedTo(this IMessage message, Stream output)
        {
            ProtoPreconditions.CheckNotNull(message, "message");
            ProtoPreconditions.CheckNotNull(output, "output");
            CodedOutputStream codedOutput = new CodedOutputStream(output);
            codedOutput.WriteLength(message.CalculateSize());
            message.WriteTo(codedOutput);
            codedOutput.Flush();
        }

        /// <summary>
        /// Converts the given message into a byte string in protobuf encoding.
        /// </summary>
        /// <param name="message">The message to convert.</param>
        /// <returns>The message data as a byte string.</returns>
        public static ByteString ToByteString(this IMessage message)
        {
            ProtoPreconditions.CheckNotNull(message, "message");
            return ByteString.AttachBytes(message.ToByteArray());
        }

        /// <summary>
        /// Writes the given message data to the given buffer writer in protobuf encoding.
        /// </summary>
        /// <param name="message">The message to write to the stream.</param>
        /// <param name="output">The stream to write to.</param>
        [SecuritySafeCritical]
        public static void WriteTo(this IMessage message, IBufferWriter<byte> output)
        {
            ProtoPreconditions.CheckNotNull(message, nameof(message));
            ProtoPreconditions.CheckNotNull(output, nameof(output));

            WriteContext.Initialize(output, out WriteContext ctx);
            WritingPrimitivesMessages.WriteRawMessage(ref ctx, message);
            ctx.Flush();
        }

        /// <summary>
        /// Writes the given message data to the given span in protobuf encoding.
        /// The size of the destination span needs to fit the serialized size
        /// of the message exactly, otherwise an exception is thrown.
        /// </summary>
        /// <param name="message">The message to write to the stream.</param>
        /// <param name="output">The span to write to. Size must match size of the message exactly.</param>
        [SecuritySafeCritical]
        public static void WriteTo(this IMessage message, Span<byte> output)
        {
            ProtoPreconditions.CheckNotNull(message, nameof(message));

            WriteContext.Initialize(ref output, out WriteContext ctx);
            WritingPrimitivesMessages.WriteRawMessage(ref ctx, message);
            ctx.CheckNoSpaceLeft();
        }

        /// <summary>
        /// Checks if all required fields in a message have values set. For proto3 messages, this returns true
        /// </summary>
        public static bool IsInitialized(this IMessage message)
        {
            if (message.Descriptor.File.Syntax == Syntax.Proto3)
            {
                return true;
            }

            if (!message.Descriptor.IsExtensionsInitialized(message))
            {
                return false;
            }

            return message.Descriptor
                .Fields
                .InDeclarationOrder()
                .All(f =>
                {
                    if (f.IsMap)
                    {
                        var valueField = f.MessageType.Fields[2];
                        if (valueField.FieldType == FieldType.Message)
                        {
                            var map = (IDictionary)f.Accessor.GetValue(message);
                            return map.Values.Cast<IMessage>().All(IsInitialized);
                        }
                        else
                        {
                            return true;
                        }
                    }
                    else if (f.IsRepeated && f.FieldType == FieldType.Message || f.FieldType == FieldType.Group)
                    {
                        var enumerable = (IEnumerable)f.Accessor.GetValue(message);
                        return enumerable.Cast<IMessage>().All(IsInitialized);
                    }
                    else if (f.FieldType == FieldType.Message || f.FieldType == FieldType.Group)
                    {
                        if (f.Accessor.HasValue(message))
                        {
                            return ((IMessage)f.Accessor.GetValue(message)).IsInitialized();
                        }
                        else
                        {
                            return !f.IsRequired;
                        }
                    }
                    else if (f.IsRequired)
                    {
                        return f.Accessor.HasValue(message);
                    }
                    else
                    {
                        return true;
                    }
                });
        }

        // Implementations allowing unknown fields to be discarded.
        internal static void MergeFrom(this IMessage message, byte[] data, bool discardUnknownFields, ExtensionRegistry registry)
        {
            ProtoPreconditions.CheckNotNull(message, "message");
            ProtoPreconditions.CheckNotNull(data, "data");
            CodedInputStream input = new CodedInputStream(data);
            input.DiscardUnknownFields = discardUnknownFields;
            input.ExtensionRegistry = registry;
            message.MergeFrom(input);
            input.CheckReadEndOfStreamTag();
        }

        internal static void MergeFrom(this IMessage message, byte[] data, int offset, int length, bool discardUnknownFields, ExtensionRegistry registry)
        {
            ProtoPreconditions.CheckNotNull(message, "message");
            ProtoPreconditions.CheckNotNull(data, "data");
            CodedInputStream input = new CodedInputStream(data, offset, length);
            input.DiscardUnknownFields = discardUnknownFields;
            input.ExtensionRegistry = registry;
            message.MergeFrom(input);
            input.CheckReadEndOfStreamTag();
        }

        internal static void MergeFrom(this IMessage message, ByteString data, bool discardUnknownFields, ExtensionRegistry registry)
        {
            ProtoPreconditions.CheckNotNull(message, "message");
            ProtoPreconditions.CheckNotNull(data, "data");
            CodedInputStream input = data.CreateCodedInput();
            input.DiscardUnknownFields = discardUnknownFields;
            input.ExtensionRegistry = registry;
            message.MergeFrom(input);
            input.CheckReadEndOfStreamTag();
        }

        internal static void MergeFrom(this IMessage message, Stream input, bool discardUnknownFields, ExtensionRegistry registry)
        {
            ProtoPreconditions.CheckNotNull(message, "message");
            ProtoPreconditions.CheckNotNull(input, "input");
            CodedInputStream codedInput = new CodedInputStream(input);
            codedInput.DiscardUnknownFields = discardUnknownFields;
            codedInput.ExtensionRegistry = registry;
            message.MergeFrom(codedInput);
            codedInput.CheckReadEndOfStreamTag();
        }

        [SecuritySafeCritical]
        internal static void MergeFrom(this IMessage message, ReadOnlySequence<byte> data, bool discardUnknownFields, ExtensionRegistry registry)
        {
            ParseContext.Initialize(data, out ParseContext ctx);
            ctx.DiscardUnknownFields = discardUnknownFields;
            ctx.ExtensionRegistry = registry;
            ParsingPrimitivesMessages.ReadRawMessage(ref ctx, message);
            ParsingPrimitivesMessages.CheckReadEndOfStreamTag(ref ctx.state);
        }

        [SecuritySafeCritical]
        internal static void MergeFrom(this IMessage message, ReadOnlySpan<byte> data, bool discardUnknownFields, ExtensionRegistry registry)
        {
            ParseContext.Initialize(data, out ParseContext ctx);
            ctx.DiscardUnknownFields = discardUnknownFields;
            ctx.ExtensionRegistry = registry;
            ParsingPrimitivesMessages.ReadRawMessage(ref ctx, message);
            ParsingPrimitivesMessages.CheckReadEndOfStreamTag(ref ctx.state);
        }

        internal static void MergeDelimitedFrom(this IMessage message, Stream input, bool discardUnknownFields, ExtensionRegistry registry)
        {
            ProtoPreconditions.CheckNotNull(message, "message");
            ProtoPreconditions.CheckNotNull(input, "input");
            int size = (int) CodedInputStream.ReadRawVarint32(input);
            Stream limitedStream = new LimitedInputStream(input, size);
            MergeFrom(message, limitedStream, discardUnknownFields, registry);
        }
    }
}
