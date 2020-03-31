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
using System.Buffers;
using System.IO;
using System.Runtime.CompilerServices;

namespace Google.Protobuf
{
    /// <summary>
    /// Reading and skipping messages / groups
    /// </summary>
    internal static class ParsingPrimitivesMessages
    {
        public static void SkipLastField(ref ReadOnlySpan<byte> buffer, ref ParserInternalState state)
        {
            if (state.lastTag == 0)
            {
                throw new InvalidOperationException("SkipLastField cannot be called at the end of a stream");
            }
            switch (WireFormat.GetTagWireType(state.lastTag))
            {
                case WireFormat.WireType.StartGroup:
                    SkipGroup(ref buffer, ref state, state.lastTag);
                    break;
                case WireFormat.WireType.EndGroup:
                    throw new InvalidProtocolBufferException(
                        "SkipLastField called on an end-group tag, indicating that the corresponding start-group was missing");
                case WireFormat.WireType.Fixed32:
                    ParsingPrimitives.ParseRawLittleEndian32(ref buffer, ref state);
                    break;
                case WireFormat.WireType.Fixed64:
                    ParsingPrimitives.ParseRawLittleEndian64(ref buffer, ref state);
                    break;
                case WireFormat.WireType.LengthDelimited:
                    var length = ParsingPrimitives.ParseLength(ref buffer, ref state);
                    ParsingPrimitives.SkipRawBytes(ref buffer, ref state, length);
                    break;
                case WireFormat.WireType.Varint:
                    ParsingPrimitives.ParseRawVarint32(ref buffer, ref state);
                    break;
            }
        }

        /// <summary>
        /// Skip a group.
        /// </summary>
        public static void SkipGroup(ref ReadOnlySpan<byte> buffer, ref ParserInternalState state, uint startGroupTag)
        {
            // Note: Currently we expect this to be the way that groups are read. We could put the recursion
            // depth changes into the ReadTag method instead, potentially...
            state.recursionDepth++;
            if (state.recursionDepth >= state.recursionLimit)
            {
                throw InvalidProtocolBufferException.RecursionLimitExceeded();
            }
            uint tag;
            while (true)
            {
                tag = ParsingPrimitives.ParseTag(ref buffer, ref state);
                if (tag == 0)
                {
                    throw InvalidProtocolBufferException.TruncatedMessage();
                }
                // Can't call SkipLastField for this case- that would throw.
                if (WireFormat.GetTagWireType(tag) == WireFormat.WireType.EndGroup)
                {
                    break;
                }
                // This recursion will allow us to handle nested groups.
                SkipLastField(ref buffer, ref state);
            }
            int startField = WireFormat.GetTagFieldNumber(startGroupTag);
            int endField = WireFormat.GetTagFieldNumber(tag);
            if (startField != endField)
            {
                throw new InvalidProtocolBufferException(
                    $"Mismatched end-group tag. Started with field {startField}; ended with field {endField}");
            }
            state.recursionDepth--;
        }

        public static void ReadMessage(ref CodedInputReader ctx, IMessage message)
        {
            int length = ParsingPrimitives.ParseLength(ref ctx.buffer, ref ctx.state);
            if (ctx.state.recursionDepth >= ctx.state.recursionLimit)
            {
                throw InvalidProtocolBufferException.RecursionLimitExceeded();
            }
            int oldLimit = SegmentedBufferHelper.PushLimit(ref ctx.state, length);
            ++ctx.state.recursionDepth;

            ReadRawMessage(ref ctx, message);

            CheckReadEndOfStreamTag(ref ctx.state);
            // Check that we've read exactly as much data as expected.
            if (!SegmentedBufferHelper.IsReachedLimit(ref ctx.state))
            {
                throw InvalidProtocolBufferException.TruncatedMessage();
            }
            --ctx.state.recursionDepth;
            SegmentedBufferHelper.PopLimit(ref ctx.state, oldLimit);
        }

        public static void ReadMessage(ref ParseContext ctx, IMessage message)
        {
            int length = ParsingPrimitives.ParseLength(ref ctx.buffer, ref ctx.state);
            if (ctx.state.recursionDepth >= ctx.state.recursionLimit)
            {
                throw InvalidProtocolBufferException.RecursionLimitExceeded();
            }
            int oldLimit = SegmentedBufferHelper.PushLimit(ref ctx.state, length);
            ++ctx.state.recursionDepth;

            ReadRawMessage(ref ctx, message);

            CheckReadEndOfStreamTag(ref ctx.state);
            // Check that we've read exactly as much data as expected.
            if (!SegmentedBufferHelper.IsReachedLimit(ref ctx.state))
            {
                throw InvalidProtocolBufferException.TruncatedMessage();
            }
            --ctx.state.recursionDepth;
            SegmentedBufferHelper.PopLimit(ref ctx.state, oldLimit);
        }

        public static void ReadGroup(ref CodedInputReader ctx, IMessage message)
        {
            if (ctx.state.recursionDepth >= ctx.state.recursionLimit)
            {
                throw InvalidProtocolBufferException.RecursionLimitExceeded();
            }
            ++ctx.state.recursionDepth;
            
            ReadRawMessage(ref ctx, message);

            --ctx.state.recursionDepth;
        }

        public static void ReadGroup(ref ParseContext ctx, IMessage message)
        {
            if (ctx.state.recursionDepth >= ctx.state.recursionLimit)
            {
                throw InvalidProtocolBufferException.RecursionLimitExceeded();
            }
            ++ctx.state.recursionDepth;
            
            ReadRawMessage(ref ctx, message);

            --ctx.state.recursionDepth;
        }

        public static void ReadRawMessage(ref CodedInputReader ctx, IMessage message)
        {
            if (message is IBufferMessage bufferMessage)
            {
                bufferMessage.MergeFrom(ref ctx);
            }
            else
            {
                if (ctx.state.codedInputStream == null)
                {
                    // TODO: improve the msg
                    throw new InvalidProtocolBufferException("Cannot parse message with current parse context. Do you need to regenerate the code?");
                }
                message.MergeFrom(ctx.state.codedInputStream);
            }
        }

        public static void ReadRawMessage(ref ParseContext ctx, IMessage message)
        {
            if (message is IBufferMessage bufferMessage)
            {
                bufferMessage.MergeFrom_Internal(ref ctx);   
            }
            else
            {
                if (ctx.state.codedInputStream == null)
                {
                    // TODO: improve the msg
                    throw new InvalidProtocolBufferException("Cannot parse message with current parse context. Do you need to regenerate the code?");
                }
                message.MergeFrom(ctx.state.codedInputStream);
            }
        }

        /// <summary>
        /// Verifies that the last call to ReadTag() returned tag 0 - in other words,
        /// we've reached the end of the stream when we expected to.
        /// </summary>
        /// <exception cref="InvalidProtocolBufferException">The 
        /// tag read was not the one specified</exception>
        public static void CheckReadEndOfStreamTag(ref ParserInternalState state)
        {
            if (state.lastTag != 0)
            {
                throw InvalidProtocolBufferException.MoreDataAvailable();
            }
        }
    }
}