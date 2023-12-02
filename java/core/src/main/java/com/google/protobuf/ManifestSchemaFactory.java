// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.protobuf.Internal.checkNotNull;

/**
 * Dynamically generates a manifest-based (i.e. table-based) schema for a given protobuf message.
 */
@CheckReturnValue
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
          SchemaUtil.unknownFieldSetFullSchema(),
          ExtensionSchemas.full(),
          messageInfo.getDefaultInstance());
    }

    return newSchema(messageType, messageInfo);
  }

  private static <T> Schema<T> newSchema(Class<T> messageType, MessageInfo messageInfo) {
    if (GeneratedMessageLite.class.isAssignableFrom(messageType)) {
      return allowExtensions(messageInfo)
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
    return allowExtensions(messageInfo)
        ? MessageSchema.newSchema(
            messageType,
            messageInfo,
            NewInstanceSchemas.full(),
            ListFieldSchema.full(),
            SchemaUtil.unknownFieldSetFullSchema(),
            ExtensionSchemas.full(),
            MapFieldSchemas.full())
        : MessageSchema.newSchema(
            messageType,
            messageInfo,
            NewInstanceSchemas.full(),
            ListFieldSchema.full(),
            SchemaUtil.unknownFieldSetFullSchema(),
            /* extensionSchema= */ null,
            MapFieldSchemas.full());
  }

  private static boolean allowExtensions(MessageInfo messageInfo) {
    switch (messageInfo.getSyntax()) {
      case PROTO3:
        return false;
      default:
        return true;
    }
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
