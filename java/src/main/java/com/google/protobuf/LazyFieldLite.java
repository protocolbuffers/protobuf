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

import java.io.IOException;

/**
 * LazyFieldLite encapsulates the logic of lazily parsing message fields. It stores
 * the message in a ByteString initially and then parse it on-demand.
 *
 * LazyField is thread-compatible e.g. concurrent read are safe, however,
 * synchronizations are needed under read/write situations.
 *
 * This class is internal implementation detail, so you don't need to use it directly.
 *
 * @author xiangl@google.com (Xiang Li)
 */
public class LazyFieldLite {
  private ByteString bytes;
  private ExtensionRegistryLite extensionRegistry;
  private volatile boolean isDirty = false;

  protected volatile MessageLite value;

  public LazyFieldLite(ExtensionRegistryLite extensionRegistry, ByteString bytes) {
    this.extensionRegistry = extensionRegistry;
    this.bytes = bytes;
  }

  public LazyFieldLite() {
  }

  public static LazyFieldLite fromValue(MessageLite value) {
    LazyFieldLite lf = new LazyFieldLite();
    lf.setValue(value);
    return lf;
  }

  public boolean containsDefaultInstance() {
    return value == null && bytes == null;
  }

  public void clear() {
    bytes = null;
    value = null;
    extensionRegistry = null;
    isDirty = true;
  }

  /**
   * Returns message instance. At first time, serialized data is parsed by
   * {@code defaultInstance.getParserForType()}.
   *
   * @param defaultInstance its message's default instance. It's also used to get parser for the
   * message type.
   */
  public MessageLite getValue(MessageLite defaultInstance) {
    ensureInitialized(defaultInstance);
    return value;
  }

  /**
   * LazyField is not thread-safe for write access. Synchronizations are needed
   * under read/write situations.
   */
  public MessageLite setValue(MessageLite value) {
    MessageLite originalValue = this.value;
    this.value = value;
    bytes = null;
    isDirty = true;
    return originalValue;
  }

  public void merge(LazyFieldLite value) {
    if (value.containsDefaultInstance()) {
      return;
    }

    if (bytes == null) {
      this.bytes = value.bytes;
    } else {
      this.bytes.concat(value.toByteString());
    }
    isDirty = false;
  }

  public void setByteString(ByteString bytes, ExtensionRegistryLite extensionRegistry) {
    this.bytes = bytes;
    this.extensionRegistry = extensionRegistry;
    isDirty = false;
  }

  public ExtensionRegistryLite getExtensionRegistry() {
    return extensionRegistry;
  }

  /**
   * Due to the optional field can be duplicated at the end of serialized
   * bytes, which will make the serialized size changed after LazyField
   * parsed. Be careful when using this method.
   */
  public int getSerializedSize() {
    if (isDirty) {
      return value.getSerializedSize();
    }
    return bytes.size();
  }

  public ByteString toByteString() {
    if (!isDirty) {
      return bytes;
    }
    synchronized (this) {
      if (!isDirty) {
        return bytes;
      }
      if (value == null) {
        bytes = ByteString.EMPTY;
      } else {
        bytes = value.toByteString();
      }
      isDirty = false;
      return bytes;
    }
  }

  protected void ensureInitialized(MessageLite defaultInstance) {
    if (value != null) {
      return;
    }
    synchronized (this) {
      if (value != null) {
        return;
      }
      try {
        if (bytes != null) {
          value = defaultInstance.getParserForType()
              .parseFrom(bytes, extensionRegistry);
        } else {
          value = defaultInstance;
        }
      } catch (IOException e) {
        // TODO(xiangl): Refactory the API to support the exception thrown from
        // lazily load messages.
      }
    }
  }
}
