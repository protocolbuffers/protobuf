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
    /// Abstraction for reading from a stream / read only sequence.
    /// Parsing from the buffer is a loop of reading from current buffer / refreshing the buffer once done.
    /// </summary>
    internal struct SegmentedBufferHelper
    {
        private readonly int? totalLength;
        private ReadOnlySequence<byte>.Enumerator readOnlySequenceEnumerator;
        private readonly CodedInputStream codedInputStream;

        public SegmentedBufferHelper(ReadOnlySequence<byte> sequence, out ReadOnlySpan<byte> firstSpan)
        {
            this.codedInputStream = null;
            if (sequence.IsSingleSegment)
            {
                firstSpan = sequence.First.Span;
                this.totalLength = firstSpan.Length;
                this.readOnlySequenceEnumerator = default;
            }
            else
            {
                // TODO(jtattermusch): try to initialize the first segment, otherwise the
                // very first read will result in slowpath (because the first thing to do is to
                // refill to get the first buffer segment)
                firstSpan = default;
                this.totalLength = (int) sequence.Length;
                this.readOnlySequenceEnumerator = sequence.GetEnumerator();
            }
        }

        public SegmentedBufferHelper(CodedInputStream codedInputStream)
        {
            this.totalLength = codedInputStream.InternalInputStream == null ? (int?)codedInputStream.InternalBuffer.Length : null;
            this.readOnlySequenceEnumerator = default;
            this.codedInputStream = codedInputStream;
        }
        
        public bool RefillBuffer(ref ReadOnlySpan<byte> buffer, ref ParserInternalState state, bool mustSucceed)
        {
            if (codedInputStream != null)
            {
                return RefillFromCodedInputStream(ref buffer, ref state, mustSucceed);
            }
            else
            {
                return RefillFromReadOnlySequence(ref buffer, ref state, mustSucceed);
            }
        }

        public int? TotalLength => totalLength;

        public CodedInputStream CodedInputStream => codedInputStream;

        /// <summary>
        /// Sets currentLimit to (current position) + byteLimit. This is called
        /// when descending into a length-delimited embedded message. The previous
        /// limit is returned.
        /// </summary>
        /// <returns>The old limit.</returns>
        public static int PushLimit(ref ParserInternalState state, int byteLimit)
        {
            if (byteLimit < 0)
            {
                throw InvalidProtocolBufferException.NegativeSize();
            }
            byteLimit += state.totalBytesRetired + state.bufferPos;
            int oldLimit = state.currentLimit;
            if (byteLimit > oldLimit)
            {
                throw InvalidProtocolBufferException.TruncatedMessage();
            }
            state.currentLimit = byteLimit;

            RecomputeBufferSizeAfterLimit(ref state);

            return oldLimit;
        }

        /// <summary>
        /// Discards the current limit, returning the previous limit.
        /// </summary>
        public static void PopLimit(ref ParserInternalState state, int oldLimit)
        {
            state.currentLimit = oldLimit;
            RecomputeBufferSizeAfterLimit(ref state);
        }

        /// <summary>
        /// Returns whether or not all the data before the limit has been read.
        /// </summary>
        /// <returns></returns>
        public static bool IsReachedLimit(ref ParserInternalState state)
        {
            if (state.currentLimit == int.MaxValue)
            {
                return false;
            }
            int currentAbsolutePosition = state.totalBytesRetired + state.bufferPos;
            return currentAbsolutePosition >= state.currentLimit;
        }

        /// <summary>
        /// Returns true if the stream has reached the end of the input. This is the
        /// case if either the end of the underlying input source has been reached or
        /// the stream has reached a limit created using PushLimit.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool IsAtEnd(ref ReadOnlySpan<byte> buffer, ref ParserInternalState state)
        {
            return state.bufferPos == state.bufferSize && !state.segmentedBufferHelper.RefillBuffer(ref buffer, ref state, false);
        }

        private bool RefillFromReadOnlySequence(ref ReadOnlySpan<byte> buffer, ref ParserInternalState state, bool mustSucceed)
        {
            CheckCurrentBufferIsEmpty(ref state);

            if (state.totalBytesRetired + state.bufferSize == state.currentLimit)
            {
                // Oops, we hit a limit.
                if (mustSucceed)
                {
                    throw InvalidProtocolBufferException.TruncatedMessage();
                }
                else
                {
                    return false;
                }
            }

            state.totalBytesRetired += state.bufferSize;

            state.bufferPos = 0;
            state.bufferSize = 0;
            while (readOnlySequenceEnumerator.MoveNext())
            {
                
                buffer = readOnlySequenceEnumerator.Current.Span;
                state.bufferSize = buffer.Length;
                if (buffer.Length != 0)
                {
                    break;
                }
            }

            if (state.bufferSize == 0)
            {
                if (mustSucceed)
                {
                    throw InvalidProtocolBufferException.TruncatedMessage();
                }
                else
                {
                    return false;
                }
            }
            else
            {
                RecomputeBufferSizeAfterLimit(ref state);
                int totalBytesRead =
                    state.totalBytesRetired + state.bufferSize + state.bufferSizeAfterLimit;
                if (totalBytesRead < 0 || totalBytesRead > state.sizeLimit)
                {
                    throw InvalidProtocolBufferException.SizeLimitExceeded();
                }
                return true;
            }
        }

        private bool RefillFromCodedInputStream(ref ReadOnlySpan<byte> buffer, ref ParserInternalState state, bool mustSucceed)
        {
            CheckCurrentBufferIsEmpty(ref state);

            if (state.totalBytesRetired + state.bufferSize == state.currentLimit)
            {
                // Oops, we hit a limit.
                if (mustSucceed)
                {
                    throw InvalidProtocolBufferException.TruncatedMessage();
                }
                else
                {
                    return false;
                }
            }

            Stream input = codedInputStream.InternalInputStream;

            state.totalBytesRetired += state.bufferSize;

            state.bufferPos = 0;
            state.bufferSize = (input == null) ? 0 : input.Read(codedInputStream.InternalBuffer, 0, buffer.Length);
            if (state.bufferSize < 0)
            {
                throw new InvalidOperationException("Stream.Read returned a negative count");
            }
            if (state.bufferSize == 0)
            {
                if (mustSucceed)
                {
                    throw InvalidProtocolBufferException.TruncatedMessage();
                }
                else
                {
                    return false;
                }
            }
            else
            {
                RecomputeBufferSizeAfterLimit(ref state);
                int totalBytesRead =
                    state.totalBytesRetired + state.bufferSize + state.bufferSizeAfterLimit;
                if (totalBytesRead < 0 || totalBytesRead > state.sizeLimit)
                {
                    throw InvalidProtocolBufferException.SizeLimitExceeded();
                }
                return true;
            }
        }

        private static void RecomputeBufferSizeAfterLimit(ref ParserInternalState state)
        {
            state.bufferSize += state.bufferSizeAfterLimit;
            int bufferEnd = state.totalBytesRetired + state.bufferSize;
            if (bufferEnd > state.currentLimit)
            {
                // Limit is in current buffer.
                state.bufferSizeAfterLimit = bufferEnd - state.currentLimit;
                state.bufferSize -= state.bufferSizeAfterLimit;
            }
            else
            {
                state.bufferSizeAfterLimit = 0;
            }
        }

        private static void CheckCurrentBufferIsEmpty(ref ParserInternalState state)
        {
            if (state.bufferPos < state.bufferSize)
            {
                throw new InvalidOperationException("RefillBuffer() called when buffer wasn't empty.");
            }
        }
    }
}