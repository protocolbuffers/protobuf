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
final class ManifestSchemaFactory {

  private final MessageInfoFactory messageInfoFactory;

  public ManifestSchemaFactory() {
    this(getDefaultMessageInfoFactory());
  }

  private ManifestSchemaFactory(MessageInfoFactory messageInfoFactory) {
    this.messageInfoFactory = checkNotNull(messageInfoFactory, "messageInfoFactory");
  }

  public <T> Schema<T> createSchema(Class<T> messageType) {
    if (!useLiteRuntime(messageType)) {
      throw new IllegalArgumentException(
          "Full runtime messages are not supported by this schema factory: "
              + messageType.getName());
    }

    MessageInfo messageInfo = messageInfoFactory.messageInfoFor(messageType);

    // MessageSet has a special schema.
    if (messageInfo.isMessageSetWireFormat()) {
      return MessageSetSchema.newSchema(
          SchemaUtil.unknownFieldSetLiteSchema(),
          ExtensionSchemas.lite(),
          messageInfo.getDefaultInstance());
    }

    return newSchema(messageType, messageInfo);
  }

  private static <T> Schema<T> newSchema(Class<T> messageType, MessageInfo messageInfo) {
    return MessageSchema.newSchema(
        messageType,
        messageInfo,
        NewInstanceSchemas.lite(),
        ListFieldSchemas.lite(),
        SchemaUtil.unknownFieldSetLiteSchema(),
        allowExtensions(messageInfo) ? ExtensionSchemas.lite() : null,
        MapFieldSchemas.lite());
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
    return GeneratedMessageInfoFactory.getInstance();
  }

  private static boolean useLiteRuntime(Class<?> messageType) {
    return Android.assumeLiteRuntime || GeneratedMessageLite.class.isAssignableFrom(messageType);
  }
}
