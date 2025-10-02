// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import com.google.protobuf.testing.Proto2TestingLite.Proto2MessageLite;
import com.google.protobuf.testing.Proto3TestingLite.Proto3MessageLite;

/** Schemas to support testing. */
public final class TestSchemasLite {

  private TestSchemasLite() {}

  public static final Schema<Proto2MessageLite> genericProto2LiteSchema =
      new ManifestSchemaFactory().createSchema(Proto2MessageLite.class);
  public static final Schema<Proto3MessageLite> genericProto3LiteSchema =
      new ManifestSchemaFactory().createSchema(Proto3MessageLite.class);
}
