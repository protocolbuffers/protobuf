#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System;
using System.Buffers;
using System.Collections.Generic;
using System.Security;
using Google.Protobuf.Collections;

namespace Google.Protobuf
{
    /// <summary>
    /// Reading and skipping messages / groups
    /// </summary>
    [SecuritySafeCritical]
    internal static class ParsingPrimitivesMessages
    {
        private static readonly byte[] ZeroLengthMessageStreamData = new byte[] { 0 };

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

        public static KeyValuePair<TKey, TValue> ReadMapEntry<TKey, TValue>(ref ParseContext ctx, MapField<TKey, TValue>.Codec codec)
        {
            int length = ParsingPrimitives.ParseLength(ref ctx.buffer, ref ctx.state);
            if (ctx.state.recursionDepth >= ctx.state.recursionLimit)
            {
                throw InvalidProtocolBufferException.RecursionLimitExceeded();
            }
            int oldLimit = SegmentedBufferHelper.PushLimit(ref ctx.state, length);
            ++ctx.state.recursionDepth;

            TKey key = codec.KeyCodec.DefaultValue;
            TValue value = codec.ValueCodec.DefaultValue;

            uint tag;
            while ((tag = ctx.ReadTag()) != 0)
            {
                if (tag == codec.KeyCodec.Tag)
                {
                    key = codec.KeyCodec.Read(ref ctx);
                }
                else if (tag == codec.ValueCodec.Tag)
                {
                    value = codec.ValueCodec.Read(ref ctx);
                }
                else
                {
                    SkipLastField(ref ctx.buffer, ref ctx.state);
                }
            }

            // Corner case: a map entry with a key but no value, where the value type is a message.
            // Read it as if we'd seen input with no data (i.e. create a "default" message).
            if (value == null)
            {
                if (ctx.state.CodedInputStream != null)
                {
                    // the decoded message might not support parsing from ParseContext, so
                    // we need to allow fallback to the legacy MergeFrom(CodedInputStream) parsing.
                    value = codec.ValueCodec.Read(new CodedInputStream(ZeroLengthMessageStreamData));
                }
                else
                {
                    ParseContext.Initialize(new ReadOnlySequence<byte>(ZeroLengthMessageStreamData), out ParseContext zeroLengthCtx);
                    value = codec.ValueCodec.Read(ref zeroLengthCtx);
                }
            }

            CheckReadEndOfStreamTag(ref ctx.state);
            // Check that we've read exactly as much data as expected.
            if (!SegmentedBufferHelper.IsReachedLimit(ref ctx.state))
            {
                throw InvalidProtocolBufferException.TruncatedMessage();
            }
            --ctx.state.recursionDepth;
            SegmentedBufferHelper.PopLimit(ref ctx.state, oldLimit);

            return new KeyValuePair<TKey, TValue>(key, value);
        }

        public static void ReadGroup(ref ParseContext ctx, IMessage message)
        {
            if (ctx.state.recursionDepth >= ctx.state.recursionLimit)
            {
                throw InvalidProtocolBufferException.RecursionLimitExceeded();
            }
            ++ctx.state.recursionDepth;
            
            uint tag = ctx.state.lastTag;
            int fieldNumber = WireFormat.GetTagFieldNumber(tag);
            ReadRawMessage(ref ctx, message);
            CheckLastTagWas(ref ctx.state, WireFormat.MakeTag(fieldNumber, WireFormat.WireType.EndGroup));

            --ctx.state.recursionDepth;
        }

        public static void ReadGroup(ref ParseContext ctx, int fieldNumber, UnknownFieldSet set)
        {
            if (ctx.state.recursionDepth >= ctx.state.recursionLimit)
            {
                throw InvalidProtocolBufferException.RecursionLimitExceeded();
            }
            ++ctx.state.recursionDepth;

            set.MergeGroupFrom(ref ctx);
            CheckLastTagWas(ref ctx.state, WireFormat.MakeTag(fieldNumber, WireFormat.WireType.EndGroup));

            --ctx.state.recursionDepth;
        }

        public static void ReadRawMessage(ref ParseContext ctx, IMessage message)
        {
            if (message is IBufferMessage bufferMessage)
            {
                bufferMessage.InternalMergeFrom(ref ctx);   
            }
            else
            {
                // If we reached here, it means we've ran into a nested message with older generated code
                // which doesn't provide the InternalMergeFrom method that takes a ParseContext.
                // With a slight performance overhead, we can still parse this message just fine,
                // but we need to find the original CodedInputStream instance that initiated this
                // parsing process and make sure its internal state is up to date.
                // Note that this performance overhead is not very high (basically copying contents of a struct)
                // and it will only be incurred in case the application mixes older and newer generated code.
                // Regenerating the code from .proto files will remove this overhead because it will
                // generate the InternalMergeFrom method we need.

                if (ctx.state.CodedInputStream == null)
                {
                    // This can only happen when the parsing started without providing a CodedInputStream instance
                    // (e.g. ParseContext was created directly from a ReadOnlySequence).
                    // That also means that one of the new parsing APIs was used at the top level
                    // and in such case it is reasonable to require that all the nested message provide
                    // up-to-date generated code with ParseContext support (and fail otherwise).
                    throw new InvalidProtocolBufferException($"Message {message.GetType().Name} doesn't provide the generated method that enables ParseContext-based parsing. You might need to regenerate the generated protobuf code.");
                }

                ctx.CopyStateTo(ctx.state.CodedInputStream);
                try
                {
                    // fallback parse using the CodedInputStream that started current parsing tree
                    message.MergeFrom(ctx.state.CodedInputStream);
                }
                finally
                {
                    ctx.LoadStateFrom(ctx.state.CodedInputStream);
                }
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

        private static void CheckLastTagWas(ref ParserInternalState state, uint expectedTag)
        {
            if (state.lastTag != expectedTag) {
               throw InvalidProtocolBufferException.InvalidEndTag();
            }
        }
    }
}