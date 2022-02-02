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

package com.google.protobuf;

import java.nio.ByteBuffer;

/**
 * An object responsible for allocation of buffers. This is an extension point to enable buffer
 * pooling within an application.
 */
@ExperimentalApi
abstract class BufferAllocator {
  private static final BufferAllocator UNPOOLED =
      new BufferAllocator() {
        @Override
        public AllocatedBuffer allocateHeapBuffer(int capacity) {
          return AllocatedBuffer.wrap(new byte[capacity]);
        }

        @Override
        public AllocatedBuffer allocateDirectBuffer(int capacity) {
          return AllocatedBuffer.wrap(ByteBuffer.allocateDirect(capacity));
        }
      };

  /** Returns an unpooled buffer allocator, which will create a new buffer for each request. */
  public static BufferAllocator unpooled() {
    return UNPOOLED;
  }

  /** Allocates a buffer with the given capacity that is backed by an array on the heap. */
  public abstract AllocatedBuffer allocateHeapBuffer(int capacity);

  /** Allocates a direct (i.e. non-heap) buffer with the given capacity. */
  public abstract AllocatedBuffer allocateDirectBuffer(int capacity);
}
