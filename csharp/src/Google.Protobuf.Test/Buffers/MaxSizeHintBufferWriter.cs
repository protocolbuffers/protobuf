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

#if GOOGLE_PROTOBUF_SUPPORT_SYSTEM_MEMORY
using System;
using System.Buffers;

namespace Google.Protobuf.Buffers
{
    internal class MaxSizeHintBufferWriter<T> : IBufferWriter<T>
    {
        private readonly IBufferWriter<T> bufferWriter;
        private readonly int maxSizeHint;

        public MaxSizeHintBufferWriter(IBufferWriter<T> bufferWriter, int maxSizeHint)
        {
            this.bufferWriter = bufferWriter;
            this.maxSizeHint = maxSizeHint;
        }

        public void Advance(int count)
        {
            bufferWriter.Advance(count);
        }

        public Memory<T> GetMemory(int sizeHint = 0)
        {
            if (sizeHint == 0)
            {
                sizeHint = maxSizeHint;
            }
            // IBufferWriter contract defines that when a size is specified then that is the minimum size returned
            return bufferWriter.GetMemory(sizeHint);
        }

        public Span<T> GetSpan(int sizeHint = 0)
        {
            if (sizeHint == 0)
            {
                sizeHint = maxSizeHint;
            }
            // IBufferWriter contract defines that when a size is specified then that is the minimum size returned
            return bufferWriter.GetSpan(sizeHint);
        }
    }
}
#endif