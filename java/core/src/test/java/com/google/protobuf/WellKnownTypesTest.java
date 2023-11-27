// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.EnumDescriptor;
import com.google.protobuf.Descriptors.EnumValueDescriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.test.TestWellKnownTypes;

import junit.framework.TestCase;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * This test ensures that well-known types are included in protobuf Java
 * runtime library.
 */
public class WellKnownTypesTest extends TestCase {
  public void testWellKnownTypes() {
    // The test passes if it compiles.
    TestWellKnownTypes message = TestWellKnownTypes.newBuilder().build();
    assertEquals(0, message.getAnyField().getSerializedSize());
    assertEquals(0, message.getApiField().getSerializedSize());
    assertEquals(0, message.getDurationField().getSerializedSize());
    assertEquals(0, message.getEmptyField().getSerializedSize());
    assertEquals(0, message.getFieldMaskField().getSerializedSize());
    assertEquals(0, message.getSourceContextField().getSerializedSize());
    assertEquals(0, message.getStructField().getSerializedSize());
    assertEquals(0, message.getTimestampField().getSerializedSize());
    assertEquals(0, message.getTypeField().getSerializedSize());
    assertEquals(0, message.getInt32Field().getSerializedSize());
  }
}
