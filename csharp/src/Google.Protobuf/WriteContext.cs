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
using System.Buffers.Binary;
using System.Collections.Generic;
using System.IO;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Security;
using System.Text;
using Google.Protobuf.Collections;

namespace Google.Protobuf
{
    /// <summary>
    /// An opaque struct that represents the current serialization state and is passed along
    /// as the serialization proceeds.
    /// All the public methods are intended to be invoked only by the generated code,
    /// users should never invoke them directly.
    /// </summary>
    [SecuritySafeCritical]
    public ref struct WriteContext
    {
        internal Span<byte> buffer;
        internal WriterInternalState state;

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Initialize(ref Span<byte> buffer, ref WriterInternalState state, out WriteContext ctx)
        {
            ctx.buffer = buffer;
            ctx.state = state;
        }

        /// <summary>
        /// Creates a WriteContext instance from CodedOutputStream.
        /// WARNING: internally this copies the CodedOutputStream's state, so after done with the WriteContext,
        /// the CodedOutputStream's state needs to be updated.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Initialize(CodedOutputStream output, out WriteContext ctx)
        {
            ctx.buffer = new Span<byte>(output.InternalBuffer);
            // ideally we would use a reference to the original state, but that doesn't seem possible
            // so we just copy the struct that holds the state. We will need to later store the state back
            // into CodedOutputStream if we want to keep it usable.
            ctx.state = output.InternalState;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        internal static void Initialize(IBufferWriter<byte> output, out WriteContext ctx)
        {
            ctx.buffer = default;
            ctx.state = default;
            WriteBufferHelper.Initialize(output, out ctx.state.writeBufferHelper, out ctx.buffer);
            ctx.state.limit = ctx.buffer.Length;
            ctx.state.position = 0;
        }  

        internal void CopyStateTo(CodedOutputStream output)
        {
            output.InternalState = state;
        }

        internal void LoadStateFrom(CodedOutputStream output)
        {
            state = output.InternalState;
        }
    }
}