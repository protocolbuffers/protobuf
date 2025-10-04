// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import com.google.protobuf.testing.Proto2Testing.Proto2Message;
import com.google.protobuf.testing.Proto3Testing.Proto3Message;

/** Schemas to support testing. */
public class TestSchemas {

  private TestSchemas() {
  }

  public static final Schema<Proto2Message> genericProto2Schema =
      new ManifestSchemaFactory().createSchema(Proto2Message.class);
  public static final Schema<Proto3Message> genericProto3Schema =
      new ManifestSchemaFactory().createSchema(Proto3Message.class);

}
