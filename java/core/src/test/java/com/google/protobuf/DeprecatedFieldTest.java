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

import protobuf_unittest.UnittestProto.TestDeprecatedFields;
import java.lang.reflect.AnnotatedElement;
import java.lang.reflect.Method;
import junit.framework.TestCase;

/**
 * Test field deprecation
 *
 * @author birdo@google.com (Roberto Scaramuzzi)
 */
public class DeprecatedFieldTest extends TestCase {
  private String[] deprecatedGetterNames = {
      "hasDeprecatedInt32",
      "getDeprecatedInt32"};

  private String[] deprecatedBuilderGetterNames = {
      "hasDeprecatedInt32",
      "getDeprecatedInt32",
      "clearDeprecatedInt32"};

  private String[] deprecatedBuilderSetterNames = {
      "setDeprecatedInt32"};

  public void testDeprecatedField() throws Exception {
    Class<?> deprecatedFields = TestDeprecatedFields.class;
    Class<?> deprecatedFieldsBuilder = TestDeprecatedFields.Builder.class;
    for (String name : deprecatedGetterNames) {
      Method method = deprecatedFields.getMethod(name);
      assertTrue("Method " + name + " should be deprecated",
          isDeprecated(method));
    }
    for (String name : deprecatedBuilderGetterNames) {
      Method method = deprecatedFieldsBuilder.getMethod(name);
      assertTrue("Method " + name + " should be deprecated",
          isDeprecated(method));
    }
    for (String name : deprecatedBuilderSetterNames) {
      Method method = deprecatedFieldsBuilder.getMethod(name, int.class);
      assertTrue("Method " + name + " should be deprecated",
          isDeprecated(method));
    }
  }

  public void testDeprecatedFieldInOneof() throws Exception {
    Class<?> oneofCase = TestDeprecatedFields.OneofFieldsCase.class;
    String name = "DEPRECATED_INT32_IN_ONEOF";
    java.lang.reflect.Field enumValue = oneofCase.getField(name);
    assertTrue("Enum value " + name + " should be deprecated.",
       isDeprecated(enumValue));
  }

  private boolean isDeprecated(AnnotatedElement annotated) {
    return annotated.isAnnotationPresent(Deprecated.class);
  }
}
