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

/**
 * Dynamically generates a manifest-based (i.e. table-based) schema for a given protobuf message.
 */
@ExperimentalApi
final class ManifestSchemaFactory implements SchemaFactory {

  private final MessageInfoFactory messageInfoFactory;

  public ManifestSchemaFactory() {
    this(getDefaultMessageInfoFactory());
  }

  private ManifestSchemaFactory(MessageInfoFactory messageInfoFactory) {
    this.messageInfoFactory = checkNotNull(messageInfoFactory, "messageInfoFactory");
  }

  @Override
  public <T> Schema<T> createSchema(Class<T> messageType) {
    SchemaUtil.requireGeneratedMessage(messageType);

    MessageInfo messageInfo = messageInfoFactory.messageInfoFor(messageType);

    // MessageSet has a special schema.
    if (messageInfo.isMessageSetWireFormat()) {
      if (GeneratedMessageLite.class.isAssignableFrom(messageType)) {
        return MessageSetSchema.newSchema(
            SchemaUtil.unknownFieldSetLiteSchema(),
            ExtensionSchemas.lite(),
            messageInfo.getDefaultInstance());
      }
      return MessageSetSchema.newSchema(
          SchemaUtil.proto2UnknownFieldSetSchema(),
          ExtensionSchemas.full(),
          messageInfo.getDefaultInstance());
    }

    return newSchema(messageType, messageInfo);
  }

  private static <T> Schema<T> newSchema(Class<T> messageType, MessageInfo messageInfo) {
    if (GeneratedMessageLite.class.isAssignableFrom(messageType)) {
      return isProto2(messageInfo)
          ? MessageSchema.newSchema(
              messageType,
              messageInfo,
              NewInstanceSchemas.lite(),
              ListFieldSchema.lite(),
              SchemaUtil.unknownFieldSetLiteSchema(),
              ExtensionSchemas.lite(),
              MapFieldSchemas.lite())
          : MessageSchema.newSchema(
              messageType,
              messageInfo,
              NewInstanceSchemas.lite(),
              ListFieldSchema.lite(),
              SchemaUtil.unknownFieldSetLiteSchema(),
              /* extensionSchema= */ null,
              MapFieldSchemas.lite());
    }
    return isProto2(messageInfo)
        ? MessageSchema.newSchema(
            messageType,
            messageInfo,
            NewInstanceSchemas.full(),
            ListFieldSchema.full(),
            SchemaUtil.proto2UnknownFieldSetSchema(),
            ExtensionSchemas.full(),
            MapFieldSchemas.full())
        : MessageSchema.newSchema(
            messageType,
            messageInfo,
            NewInstanceSchemas.full(),
            ListFieldSchema.full(),
            SchemaUtil.proto3UnknownFieldSetSchema(),
            /* extensionSchema= */ null,
            MapFieldSchemas.full());
  }

  private static boolean isProto2(MessageInfo messageInfo) {
    return messageInfo.getSyntax() == ProtoSyntax.PROTO2;
  }

  private static MessageInfoFactory getDefaultMessageInfoFactory() {
    return new CompositeMessageInfoFactory(
        GeneratedMessageInfoFactory.getInstance(), getDescriptorMessageInfoFactory());
  }

  private static class CompositeMessageInfoFactory implements MessageInfoFactory {
    private MessageInfoFactory[] factories;

    CompositeMessageInfoFactory(MessageInfoFactory... factories) {
      this.factories = factories;
    }

    @Override
    public boolean isSupported(Class<?> clazz) {
      for (MessageInfoFactory factory : factories) {
        if (factory.isSupported(clazz)) {
          return true;
        }
      }
      return false;
    }

    @Override
    public MessageInfo messageInfoFor(Class<?> clazz) {
      for (MessageInfoFactory factory : factories) {
        if (factory.isSupported(clazz)) {
          return factory.messageInfoFor(clazz);
        }
      }
      throw new UnsupportedOperationException(
          "No factory is available for message type: " + clazz.getName());
    }
  }

  private static final MessageInfoFactory EMPTY_FACTORY =
      new MessageInfoFactory() {
        @Override
        public boolean isSupported(Class<?> clazz) {
          return false;
        }

        @Override
        public MessageInfo messageInfoFor(Class<?> clazz) {
          throw new IllegalStateException("This should never be called.");
        }
      };

  private static MessageInfoFactory getDescriptorMessageInfoFactory() {
    try {
      Class<?> clazz = Class.forName("com.google.protobuf.DescriptorMessageInfoFactory");
      return (MessageInfoFactory) clazz.getDeclaredMethod("getInstance").invoke(null);
    } catch (Exception e) {
      return EMPTY_FACTORY;
    }
  }
}
