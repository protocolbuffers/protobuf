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

import com.google.protobuf.testing.Proto2TestingLite;
import com.google.protobuf.testing.Proto2TestingLite.Proto2EmptyLite;
import com.google.protobuf.testing.Proto2TestingLite.Proto2MessageLite;
import com.google.protobuf.testing.Proto2TestingLite.Proto2MessageLiteWithExtensions;
import com.google.protobuf.testing.Proto2TestingLite.Proto2MessageLiteWithMaps;
import com.google.protobuf.testing.Proto3TestingLite.Proto3EmptyLite;
import com.google.protobuf.testing.Proto3TestingLite.Proto3MessageLite;
import com.google.protobuf.testing.Proto3TestingLite.Proto3MessageLiteWithMaps;

/** Schemas to support testing. */
public final class TestSchemasLite {

  public static final Schema<Proto2MessageLite> genericProto2LiteSchema =
      new ManifestSchemaFactory().createSchema(Proto2MessageLite.class);
  public static final Schema<Proto3MessageLite> genericProto3LiteSchema =
      new ManifestSchemaFactory().createSchema(Proto3MessageLite.class);

  public static void registerGenericProto2LiteSchemas() {
    registerProto2LiteSchemas();
  }

  public static void registerGenericProto3LiteSchemas() {
    registerProto3LiteSchemas();
  }

  private static void registerProto2LiteSchemas() {
    Protobuf protobuf = Protobuf.getInstance();
    ManifestSchemaFactory factory = new ManifestSchemaFactory();
    protobuf.registerSchemaOverride(
        Proto2MessageLite.class, factory.createSchema(Proto2MessageLite.class));
    protobuf.registerSchemaOverride(
        Proto2MessageLite.FieldGroup49.class,
        factory.createSchema(Proto2MessageLite.FieldGroup49.class));
    protobuf.registerSchemaOverride(
        Proto2MessageLite.FieldGroupList51.class,
        factory.createSchema(Proto2MessageLite.FieldGroupList51.class));
    protobuf.registerSchemaOverride(
        Proto2MessageLite.FieldGroup69.class,
        factory.createSchema(Proto2MessageLite.FieldGroup69.class));
    protobuf.registerSchemaOverride(
        Proto2MessageLite.RequiredNestedMessage.class,
        factory.createSchema(Proto2MessageLite.RequiredNestedMessage.class));
    protobuf.registerSchemaOverride(
        Proto2MessageLite.FieldRequiredGroup88.class,
        factory.createSchema(Proto2MessageLite.FieldRequiredGroup88.class));
    protobuf.registerSchemaOverride(
        Proto2EmptyLite.class, factory.createSchema(Proto2EmptyLite.class));
    protobuf.registerSchemaOverride(
        Proto2MessageLiteWithExtensions.class,
        factory.createSchema(Proto2MessageLiteWithExtensions.class));
    protobuf.registerSchemaOverride(
        Proto2TestingLite.FieldGroup49.class,
        factory.createSchema(Proto2TestingLite.FieldGroup49.class));
    protobuf.registerSchemaOverride(
        Proto2TestingLite.FieldGroupList51.class,
        factory.createSchema(Proto2TestingLite.FieldGroupList51.class));
    protobuf.registerSchemaOverride(
        Proto2MessageLiteWithMaps.class, factory.createSchema(Proto2MessageLiteWithMaps.class));
  }

  private static void registerProto3LiteSchemas() {
    Protobuf protobuf = Protobuf.getInstance();
    ManifestSchemaFactory factory = new ManifestSchemaFactory();
    protobuf.registerSchemaOverride(
        Proto3MessageLite.class, factory.createSchema(Proto3MessageLite.class));
    protobuf.registerSchemaOverride(
        Proto3EmptyLite.class, factory.createSchema(Proto3EmptyLite.class));
    protobuf.registerSchemaOverride(
        Proto3MessageLiteWithMaps.class, factory.createSchema(Proto3MessageLiteWithMaps.class));
  }
}
