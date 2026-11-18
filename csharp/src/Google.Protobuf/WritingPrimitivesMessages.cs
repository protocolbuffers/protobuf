#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
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