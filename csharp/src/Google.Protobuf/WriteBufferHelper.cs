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
using System.Runtime.CompilerServices;
using System.Security;

namespace Google.Protobuf
{
    /// <summary>
    /// Abstraction for writing to a steam / IBufferWriter
    /// </summary>
    [SecuritySafeCritical]
    internal struct WriteBufferHelper
    {
        private IBufferWriter<byte> bufferWriter;
        private CodedOutputStream codedOutputStream;

        public CodedOutputStream CodedOutputStream => codedOutputStream;

        /// <summary>
        /// Initialize an instance with a coded output stream.
        /// This approach is faster than using a constructor because the instance to initialize is passed by reference
        /// and we can write directly into it without copying.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void Initialize(CodedOutputStream codedOutputStream, out WriteBufferHelper instance)
        {
            instance.bufferWriter = null;
            instance.codedOutputStream = codedOutputStream;
        }

        /// <summary>
        /// Initialize an instance with a buffer writer.
        /// This approach is faster than using a constructor because the instance to initialize is passed by reference
        /// and we can write directly into it without copying.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void Initialize(IBufferWriter<byte> bufferWriter, out WriteBufferHelper instance, out Span<byte> buffer)
        {
            instance.bufferWriter = bufferWriter;
            instance.codedOutputStream = null;
            buffer = default;  // TODO: initialize the initial buffer so that the first write is not via slowpath.
        }

        /// <summary>
        /// Initialize an instance with a buffer represented by a single span (i.e. buffer cannot be refreshed)
        /// This approach is faster than using a constructor because the instance to initialize is passed by reference
        /// and we can write directly into it without copying.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void InitializeNonRefreshable(out WriteBufferHelper instance)
        {
            instance.bufferWriter = null;
            instance.codedOutputStream = null;
        }

        /// <summary>
        /// Verifies that SpaceLeft returns zero.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void CheckNoSpaceLeft(ref WriterInternalState state)
        {
            if (GetSpaceLeft(ref state) != 0)
            {
                throw new InvalidOperationException("Did not write as much data as expected.");
            }
        }

        /// <summary>
        /// If writing to a flat array, returns the space left in the array. Otherwise,
        /// throws an InvalidOperationException.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static int GetSpaceLeft(ref WriterInternalState state)
        {
            if (state.writeBufferHelper.codedOutputStream?.InternalOutputStream == null && state.writeBufferHelper.bufferWriter == null)
            {
                return state.limit - state.position;
            }
            else
            {
                throw new InvalidOperationException(
                    "SpaceLeft can only be called on CodedOutputStreams that are " +
                        "writing to a flat array or when writing to a single span.");
            }
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        public static void RefreshBuffer(ref Span<byte> buffer, ref WriterInternalState state)
        {
            if (state.writeBufferHelper.codedOutputStream?.InternalOutputStream != null)
            {
                // because we're using coded output stream, we know that "buffer" and codedOutputStream.InternalBuffer are identical.
                state.writeBufferHelper.codedOutputStream.InternalOutputStream.Write(state.writeBufferHelper.codedOutputStream.InternalBuffer, 0, state.position);
                // reset position, limit stays the same because we are reusing the codedOutputStream's internal buffer.
                state.position = 0;
            }
            else if (state.writeBufferHelper.bufferWriter != null)
            {
                // commit the bytes and get a new buffer to write to.
                state.writeBufferHelper.bufferWriter.Advance(state.position);
                state.position = 0;
                buffer = state.writeBufferHelper.bufferWriter.GetSpan();
                state.limit = buffer.Length;
            }
            else
            {
                // We're writing to a single buffer.
                throw new CodedOutputStream.OutOfSpaceException();
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void Flush(ref Span<byte> buffer, ref WriterInternalState state)
        {
            if (state.writeBufferHelper.codedOutputStream?.InternalOutputStream != null)
            {
                // because we're using coded output stream, we know that "buffer" and codedOutputStream.InternalBuffer are identical.
                state.writeBufferHelper.codedOutputStream.InternalOutputStream.Write(state.writeBufferHelper.codedOutputStream.InternalBuffer, 0, state.position);
                state.position = 0;
            }
            else if (state.writeBufferHelper.bufferWriter != null)
            {
                // calling Advance invalidates the current buffer and we must not continue writing to it,
                // so we set the current buffer to point to an empty Span. If any subsequent writes happen,
                // the first subsequent write will trigger refresing of the buffer.
                state.writeBufferHelper.bufferWriter.Advance(state.position);
                state.position = 0;
                state.limit = 0;
                buffer = default;  // invalidate the current buffer
            }
        }
    }
}