// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import java.nio.ByteBuffer;

/**
 * An object responsible for allocation of buffers. This is an extension point to enable buffer
 * pooling within an application.
 */
@CheckReturnValue
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
