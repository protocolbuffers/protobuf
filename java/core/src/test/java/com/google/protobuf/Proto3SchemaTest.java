// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import com.google.protobuf.testing.Proto3Testing.Proto3Message;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class Proto3SchemaTest extends AbstractProto3SchemaTest {
  @Override
  protected void registerSchemas() {
    TestSchemas.registerGenericProto3Schemas();
  }

  @Override
  protected Schema<Proto3Message> schema() {
    return TestSchemas.genericProto3Schema;
  }
}
