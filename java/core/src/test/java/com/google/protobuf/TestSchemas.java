// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import com.google.protobuf.testing.Proto2Testing;
import com.google.protobuf.testing.Proto2Testing.Proto2Empty;
import com.google.protobuf.testing.Proto2Testing.Proto2Message;
import com.google.protobuf.testing.Proto2Testing.Proto2MessageWithExtensions;
import com.google.protobuf.testing.Proto2Testing.Proto2MessageWithMaps;
import com.google.protobuf.testing.Proto3Testing.Proto3Empty;
import com.google.protobuf.testing.Proto3Testing.Proto3Message;
import com.google.protobuf.testing.Proto3Testing.Proto3MessageWithMaps;

/** Schemas to support testing. */
public class TestSchemas {

  private TestSchemas() {
  }

  public static final Schema<Proto2Message> genericProto2Schema =
      new ManifestSchemaFactory().createSchema(Proto2Message.class);
  public static final Schema<Proto3Message> genericProto3Schema =
      new ManifestSchemaFactory().createSchema(Proto3Message.class);

  public static void registerGenericProto2Schemas() {
    registerProto2Schemas();
  }

  public static void registerGenericProto3Schemas() {
    registerProto3Schemas();
  }

  private static void registerProto2Schemas() {
    Protobuf protobuf = Protobuf.getInstance();
    ManifestSchemaFactory factory = new ManifestSchemaFactory();
    protobuf.registerSchemaOverride(Proto2Message.class, factory.createSchema(Proto2Message.class));
    protobuf.registerSchemaOverride(
        Proto2Message.FieldGroup49.class, factory.createSchema(Proto2Message.FieldGroup49.class));
    protobuf.registerSchemaOverride(
        Proto2Message.FieldGroupList51.class,
        factory.createSchema(Proto2Message.FieldGroupList51.class));
    protobuf.registerSchemaOverride(
        Proto2Message.FieldGroup69.class, factory.createSchema(Proto2Message.FieldGroup69.class));
    protobuf.registerSchemaOverride(
        Proto2Message.RequiredNestedMessage.class,
        factory.createSchema(Proto2Message.RequiredNestedMessage.class));
    protobuf.registerSchemaOverride(
        Proto2Message.FieldRequiredGroup88.class,
        factory.createSchema(Proto2Message.FieldRequiredGroup88.class));
    protobuf.registerSchemaOverride(Proto2Empty.class, factory.createSchema(Proto2Empty.class));
    protobuf.registerSchemaOverride(
        Proto2MessageWithExtensions.class, factory.createSchema(Proto2MessageWithExtensions.class));
    protobuf.registerSchemaOverride(
        Proto2Testing.FieldGroup49.class, factory.createSchema(Proto2Testing.FieldGroup49.class));
    protobuf.registerSchemaOverride(
        Proto2Testing.FieldGroupList51.class,
        factory.createSchema(Proto2Testing.FieldGroupList51.class));
    protobuf.registerSchemaOverride(
        Proto2MessageWithMaps.class, factory.createSchema(Proto2MessageWithMaps.class));
  }

  private static void registerProto3Schemas() {
    Protobuf protobuf = Protobuf.getInstance();
    ManifestSchemaFactory factory = new ManifestSchemaFactory();
    protobuf.registerSchemaOverride(Proto3Message.class, factory.createSchema(Proto3Message.class));
    protobuf.registerSchemaOverride(Proto3Empty.class, factory.createSchema(Proto3Empty.class));
    protobuf.registerSchemaOverride(
        Proto3MessageWithMaps.class, factory.createSchema(Proto3MessageWithMaps.class));
  }
}
