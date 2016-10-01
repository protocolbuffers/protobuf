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

import static com.google.protobuf.Internal.checkNotNull;

/** Manufactures instances of {@link Proto3TableSchema}. */
@ExperimentalApi
public final class Proto2SchemaFactory implements SchemaFactory {
  /**
   * The mode with which to generate schemas.
   *
   * <p>For testing purposes only.
   */
  public enum Mode {
    /** Always use a table-based indexing of fields. */
    TABLE,

    /** Always used lookup-based (i.e. binary search) indexing of fields. */
    LOOKUP,

    /**
     * Default. Determine the appropriate field indexing mode based on how sparse the field numbers
     * are for the message.
     */
    DYNAMIC
  }

  private final MessageInfoFactory messageDescriptorFactory;
  private final Mode mode;

  public Proto2SchemaFactory() {
    this(DescriptorMessageInfoFactory.getInstance());
  }

  public Proto2SchemaFactory(MessageInfoFactory messageDescriptorFactory) {
    this(messageDescriptorFactory, Mode.DYNAMIC);
  }

  /** For testing purposes only. Allows specification of {@link Mode}. */
  public Proto2SchemaFactory(MessageInfoFactory messageDescriptorFactory, Mode mode) {
    if (!isSupported()) {
      throw new IllegalStateException("Schema factory is unsupported on this platform");
    }
    this.messageDescriptorFactory =
        checkNotNull(messageDescriptorFactory, "messageDescriptorFactory");
    this.mode = checkNotNull(mode, "mode");
  }

  public static boolean isSupported() {
    return UnsafeUtil.hasUnsafeArrayOperations() && UnsafeUtil.hasUnsafeByteBufferOperations();
  }

  @Override
  public <T> Schema<T> createSchema(Class<T> messageType) {
    SchemaUtil.requireGeneratedMessage(messageType);

    MessageInfo descriptor = messageDescriptorFactory.messageInfoFor(messageType);
    switch (mode) {
      case TABLE:
        return newTableSchema(messageType, descriptor);
      case LOOKUP:
        return newLookupSchema(messageType, descriptor);
      default:
        return SchemaUtil.shouldUseTableSwitch(descriptor.getFields())
            ? newTableSchema(messageType, descriptor)
            : newLookupSchema(messageType, descriptor);
    }
  }

  private <T> Schema<T> newTableSchema(Class<T> messageType, MessageInfo descriptor) {
    if (GeneratedMessageLite.class.isAssignableFrom(messageType)) {
      return new Proto2LiteTableSchema<T>(messageType, descriptor);
    }
    return new Proto2TableSchema<T>(messageType, descriptor);
  }

  private <T> Schema<T> newLookupSchema(Class<T> messageType, MessageInfo descriptor) {
    if (GeneratedMessageLite.class.isAssignableFrom(messageType)) {
      return new Proto2LiteLookupSchema<T>(messageType, descriptor);
    }
    return new Proto2LookupSchema<T>(messageType, descriptor);
  }
}
