#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System;
using System.Security;

namespace Google.Protobuf
{
    /// <summary>
    /// Fast parsing primitives for wrapper types
    /// </summary>
    [SecuritySafeCritical]
    internal static class ParsingPrimitivesWrappers
    {
        internal static float? ReadFloatWrapperLittleEndian(ref ReadOnlySpan<byte> buffer, ref ParserInternalState state)
        {
            // length:1 + tag:1 + value:4 = 6 bytes
            if (state.bufferPos + 6 <= state.bufferSize)
            {
                // The entire wrapper message is already contained in `buffer`.
                int length = buffer[state.bufferPos];
                if (length == 0)
                {
                    state.bufferPos++;
                    return 0F;
                }
                // tag:1 + value:4 = length of 5 bytes
                // field=1, type=32-bit = tag of 13
                if (length != 5 || buffer[state.bufferPos + 1] != 13)
                {
                    return ReadFloatWrapperSlow(ref buffer, ref state);
                }
                state.bufferPos += 2;
                return ParsingPrimitives.ParseFloat(ref buffer, ref state);
            }
            else
            {
                return ReadFloatWrapperSlow(ref buffer, ref state);
            }
        }

        internal static float? ReadFloatWrapperSlow(ref ReadOnlySpan<byte> buffer, ref ParserInternalState state)
        {
            int length = ParsingPrimitives.ParseLength(ref buffer, ref state);
            if (length == 0)
            {
                return 0F;
            }
            int finalBufferPos = state.totalBytesRetired + state.bufferPos + length;
            float result = 0F;
            do
            {
                // field=1, type=32-bit = tag of 13
                if (ParsingPrimitives.ParseTag(ref buffer, ref state) == 13)
                {
                    result = ParsingPrimitives.ParseFloat(ref buffer, ref state);
                }
                else
                {
                    ParsingPrimitivesMessages.SkipLastField(ref buffer, ref state);
                }
            }
            while (state.totalBytesRetired + state.bufferPos < finalBufferPos);
            return result;
        }

        internal static double? ReadDoubleWrapperLittleEndian(ref ReadOnlySpan<byte> buffer, ref ParserInternalState state)
        {
            // length:1 + tag:1 + value:8 = 10 bytes
            if (state.bufferPos + 10 <= state.bufferSize)
            {
                // The entire wrapper message is already contained in `buffer`.
                int length = buffer[state.bufferPos];
                if (length == 0)
                {
                    state.bufferPos++;
                    return 0D;
                }
                // tag:1 + value:8 = length of 9 bytes
                // field=1, type=64-bit = tag of 9
                if (length != 9 || buffer[state.bufferPos + 1] != 9)
                {
                    return ReadDoubleWrapperSlow(ref buffer, ref state);
                }
                state.bufferPos += 2;
                return ParsingPrimitives.ParseDouble(ref buffer, ref state);
            }
            else
            {
                return ReadDoubleWrapperSlow(ref buffer, ref state);
            }
        }

        internal static double? ReadDoubleWrapperSlow(ref ReadOnlySpan<byte> buffer, ref ParserInternalState state)
        {
            int length = ParsingPrimitives.ParseLength(ref buffer, ref state);
            if (length == 0)
            {
                return 0D;
            }
            int finalBufferPos = state.totalBytesRetired + state.bufferPos + length;
            double result = 0D;
            do
            {
                // field=1, type=64-bit = tag of 9
                if (ParsingPrimitives.ParseTag(ref buffer, ref state) == 9)
                {
                    result = ParsingPrimitives.ParseDouble(ref buffer, ref state);
                }
                else
                {
                    ParsingPrimitivesMessages.SkipLastField(ref buffer, ref state);
                }
            }
            while (state.totalBytesRetired + state.bufferPos < finalBufferPos);
            return result;
        }

        internal static bool? ReadBoolWrapper(ref ReadOnlySpan<byte> buffer, ref ParserInternalState state)
        {
            return ReadUInt64Wrapper(ref buffer, ref state) != 0;
        }

        internal static uint? ReadUInt32Wrapper(ref ReadOnlySpan<byte> buffer, ref ParserInternalState state)
        {
            // field=1, type=varint = tag of 8
            const int expectedTag = 8;
            // length:1 + tag:1 + value:10(varint64-max) = 12 bytes
            // Value can be 64 bits for negative integers
            if (state.bufferPos + 12 <= state.bufferSize)
            {
                // The entire wrapper message is already contained in `buffer`.
                int pos0 = state.bufferPos;
                int length = buffer[state.bufferPos++];
                if (length == 0)
                {
                    return 0;
                }
                // Length will always fit in a single byte.
                if (length >= 128)
                {
                    state.bufferPos = pos0;
                    return ReadUInt32WrapperSlow(ref buffer, ref state);
                }
                int finalBufferPos = state.bufferPos + length;
                if (buffer[state.bufferPos++] != expectedTag)
                {
                    state.bufferPos = pos0;
                    return ReadUInt32WrapperSlow(ref buffer, ref state);
                }
                var result = ParsingPrimitives.ParseRawVarint32(ref buffer, ref state);
                // Verify this message only contained a single field.
                if (state.bufferPos != finalBufferPos)
                {
                    state.bufferPos = pos0;
                    return ReadUInt32WrapperSlow(ref buffer, ref state);
                }
                return result;
            }
            else
            {
                return ReadUInt32WrapperSlow(ref buffer, ref state);
            }
        }

        internal static uint? ReadUInt32WrapperSlow(ref ReadOnlySpan<byte> buffer, ref ParserInternalState state)
        {
            int length = ParsingPrimitives.ParseLength(ref buffer, ref state);
            if (length == 0)
            {
                return 0;
            }
            int finalBufferPos = state.totalBytesRetired + state.bufferPos + length;
            uint result = 0;
            do
            {
                // field=1, type=varint = tag of 8
                if (ParsingPrimitives.ParseTag(ref buffer, ref state) == 8)
                {
                    result = ParsingPrimitives.ParseRawVarint32(ref buffer, ref state);
                }
                else
                {
                    ParsingPrimitivesMessages.SkipLastField(ref buffer, ref state);
                }
            }
            while (state.totalBytesRetired + state.bufferPos < finalBufferPos);
            return result;
        }

        internal static int? ReadInt32Wrapper(ref ReadOnlySpan<byte> buffer, ref ParserInternalState state)
        {
            return (int?)ReadUInt32Wrapper(ref buffer, ref state);
        }

        internal static ulong? ReadUInt64Wrapper(ref ReadOnlySpan<byte> buffer, ref ParserInternalState state)
        {
            // field=1, type=varint = tag of 8
            const int expectedTag = 8;
            // length:1 + tag:1 + value:10(varint64-max) = 12 bytes
            if (state.bufferPos + 12 <= state.bufferSize)
            {
                // The entire wrapper message is already contained in `buffer`.
                int pos0 = state.bufferPos;
                int length = buffer[state.bufferPos++];
                if (length == 0)
                {
                    return 0L;
                }
                // Length will always fit in a single byte.
                if (length >= 128)
                {
                    state.bufferPos = pos0;
                    return ReadUInt64WrapperSlow(ref buffer, ref state);
                }
                int finalBufferPos = state.bufferPos + length;
                if (buffer[state.bufferPos++] != expectedTag)
                {
                    state.bufferPos = pos0;
                    return ReadUInt64WrapperSlow(ref buffer, ref state);
                }
                var result = ParsingPrimitives.ParseRawVarint64(ref buffer, ref state);
                // Verify this message only contained a single field.
                if (state.bufferPos != finalBufferPos)
                {
                    state.bufferPos = pos0;
                    return ReadUInt64WrapperSlow(ref buffer, ref state);
                }
                return result;
            }
            else
            {
                return ReadUInt64WrapperSlow(ref buffer, ref state);
            }
        }

        internal static ulong? ReadUInt64WrapperSlow(ref ReadOnlySpan<byte> buffer, ref ParserInternalState state)
        {
            // field=1, type=varint = tag of 8
            const int expectedTag = 8;
            int length = ParsingPrimitives.ParseLength(ref buffer, ref state);
            if (length == 0)
            {
                return 0L;
            }
            int finalBufferPos = state.totalBytesRetired + state.bufferPos + length;
            ulong result = 0L;
            do
            {
                if (ParsingPrimitives.ParseTag(ref buffer, ref state) == expectedTag)
                {
                    result = ParsingPrimitives.ParseRawVarint64(ref buffer, ref state);
                }
                else
                {
                    ParsingPrimitivesMessages.SkipLastField(ref buffer, ref state);
                }
            }
            while (state.totalBytesRetired + state.bufferPos < finalBufferPos);
            return result;
        }

        internal static long? ReadInt64Wrapper(ref ReadOnlySpan<byte> buffer, ref ParserInternalState state)
        {
            return (long?)ReadUInt64Wrapper(ref buffer, ref state);
        }

        internal static float? ReadFloatWrapperLittleEndian(ref ParseContext ctx)
        {
            return ParsingPrimitivesWrappers.ReadFloatWrapperLittleEndian(ref ctx.buffer, ref ctx.state);
        }

        internal static float? ReadFloatWrapperSlow(ref ParseContext ctx)
        {
            return ParsingPrimitivesWrappers.ReadFloatWrapperSlow(ref ctx.buffer, ref ctx.state);
        }

        internal static double? ReadDoubleWrapperLittleEndian(ref ParseContext ctx)
        {
            return ParsingPrimitivesWrappers.ReadDoubleWrapperLittleEndian(ref ctx.buffer, ref ctx.state);
        }

        internal static double? ReadDoubleWrapperSlow(ref ParseContext ctx)
        {
            return ParsingPrimitivesWrappers.ReadDoubleWrapperSlow(ref ctx.buffer, ref ctx.state);
        }

        internal static bool? ReadBoolWrapper(ref ParseContext ctx)
        {
            return ParsingPrimitivesWrappers.ReadBoolWrapper(ref ctx.buffer, ref ctx.state);
        }

        internal static uint? ReadUInt32Wrapper(ref ParseContext ctx)
        {
            return ParsingPrimitivesWrappers.ReadUInt32Wrapper(ref ctx.buffer, ref ctx.state);
        }

        internal static int? ReadInt32Wrapper(ref ParseContext ctx)
        {
            return ParsingPrimitivesWrappers.ReadInt32Wrapper(ref ctx.buffer, ref ctx.state);
        }

        internal static ulong? ReadUInt64Wrapper(ref ParseContext ctx)
        {
            return ParsingPrimitivesWrappers.ReadUInt64Wrapper(ref ctx.buffer, ref ctx.state);
        }

        internal static ulong? ReadUInt64WrapperSlow(ref ParseContext ctx)
        {
            return ParsingPrimitivesWrappers.ReadUInt64WrapperSlow(ref ctx.buffer, ref ctx.state);
        }

        internal static long? ReadInt64Wrapper(ref ParseContext ctx)
        {
            return ParsingPrimitivesWrappers.ReadInt64Wrapper(ref ctx.buffer, ref ctx.state);
        }
    }
}