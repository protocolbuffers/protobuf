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

using System.Runtime.CompilerServices;
using System.Security;

namespace Google.Protobuf
{
    /// <summary>
    /// Writing messages / groups.
    /// </summary>
    [SecuritySafeCritical]
    internal static class WritingPrimitivesMessages
    {
        /// <summary>
        /// Writes a message, without a tag.
        /// The data is length-prefixed.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void WriteMessage(ref WriteContext ctx, IMessage value)
        {
            WritingPrimitives.WriteLength(ref ctx.buffer, ref ctx.state, value.CalculateSize());
            WriteRawMessage(ref ctx, value);
        }

        /// <summary>
        /// Writes a group, without a tag.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void WriteGroup(ref WriteContext ctx, IMessage value)
        {
            WriteRawMessage(ref ctx, value);
        }

        /// <summary>
        /// Writes a message, without a tag.
        /// Message will be written without a length prefix.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void WriteRawMessage(ref WriteContext ctx, IMessage message)
        {
            if (message is IBufferMessage bufferMessage)
            {
                bufferMessage.InternalWriteTo(ref ctx);
            }
            else
            {
                // If we reached here, it means we've ran into a nested message with older generated code
                // which doesn't provide the InternalWriteTo method that takes a WriteContext.
                // With a slight performance overhead, we can still serialize this message just fine,
                // but we need to find the original CodedOutputStream instance that initiated this
                // serialization process and make sure its internal state is up to date.
                // Note that this performance overhead is not very high (basically copying contents of a struct)
                // and it will only be incurred in case the application mixes older and newer generated code.
                // Regenerating the code from .proto files will remove this overhead because it will
                // generate the InternalWriteTo method we need.

                if (ctx.state.CodedOutputStream == null)
                {
                    // This can only happen when the serialization started without providing a CodedOutputStream instance
                    // (e.g. WriteContext was created directly from a IBufferWriter).
                    // That also means that one of the new parsing APIs was used at the top level
                    // and in such case it is reasonable to require that all the nested message provide
                    // up-to-date generated code with WriteContext support (and fail otherwise).
                    throw new InvalidProtocolBufferException($"Message {message.GetType().Name} doesn't provide the generated method that enables WriteContext-based serialization. You might need to regenerate the generated protobuf code.");
                }

                ctx.CopyStateTo(ctx.state.CodedOutputStream);
                try
                {
                    // fallback parse using the CodedOutputStream that started current serialization tree
                    message.WriteTo(ctx.state.CodedOutputStream);
                }
                finally
                {
                    ctx.LoadStateFrom(ctx.state.CodedOutputStream);
                }
            }
        }
    }
}