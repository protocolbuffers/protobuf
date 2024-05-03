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
using System.IO;
using System.Runtime.CompilerServices;
using System.Security;

namespace Google.Protobuf
{
    /// <summary>
    /// Abstraction for reading from a stream / read only sequence.
    /// Parsing from the buffer is a loop of reading from current buffer / refreshing the buffer once done.
    /// </summary>
    [SecuritySafeCritical]
    internal struct SegmentedBufferHelper
    {
        private int? totalLength;
        private ReadOnlySequence<byte>.Enumerator readOnlySequenceEnumerator;
        private CodedInputStream codedInputStream;

        /// <summary>
        /// Initialize an instance with a coded input stream.
        /// This approach is faster than using a constructor because the instance to initialize is passed by reference
        /// and we can write directly into it without copying.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void Initialize(CodedInputStream codedInputStream, out SegmentedBufferHelper instance)
        {
            instance.totalLength = codedInputStream.InternalInputStream == null ? (int?)codedInputStream.InternalBuffer.Length : null;
            instance.readOnlySequenceEnumerator = default;
            instance.codedInputStream = codedInputStream;
        }

        /// <summary>
        /// Initialize an instance with a read only sequence.
        /// This approach is faster than using a constructor because the instance to initialize is passed by reference
        /// and we can write directly into it without copying.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void Initialize(ReadOnlySequence<byte> sequence, out SegmentedBufferHelper instance, out ReadOnlySpan<byte> firstSpan)
        {
            instance.codedInputStream = null;
            if (sequence.IsSingleSegment)
            {
                firstSpan = sequence.First.Span;
                instance.totalLength = firstSpan.Length;
                instance.readOnlySequenceEnumerator = default;
            }
            else
            {
                instance.readOnlySequenceEnumerator = sequence.GetEnumerator();
                instance.totalLength = (int) sequence.Length;

                // set firstSpan to the first segment
                instance.readOnlySequenceEnumerator.MoveNext();
                firstSpan = instance.readOnlySequenceEnumerator.Current.Span;
            }
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