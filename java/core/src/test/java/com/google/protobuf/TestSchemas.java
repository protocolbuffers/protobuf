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
