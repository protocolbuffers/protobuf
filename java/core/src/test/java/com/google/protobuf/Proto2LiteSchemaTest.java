// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import com.google.protobuf.testing.Proto2TestingLite.Proto2MessageLite;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class Proto2LiteSchemaTest extends AbstractProto2LiteSchemaTest {

  @Override
  protected Schema<Proto2MessageLite> schema() {
    return TestSchemasLite.genericProto2LiteSchema;
  }

  @Override
  protected void registerSchemas() {
    TestSchemasLite.registerGenericProto2LiteSchemas();
  }
}
