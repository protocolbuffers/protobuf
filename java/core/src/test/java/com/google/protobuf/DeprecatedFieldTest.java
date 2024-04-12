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

import static com.google.common.truth.Truth.assertWithMessage;

import protobuf_unittest.UnittestProto.TestDeprecatedFields;
import java.lang.reflect.AnnotatedElement;
import java.lang.reflect.Method;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Test field deprecation. */
@RunWith(JUnit4.class)
public class DeprecatedFieldTest {
  private final String[] deprecatedGetterNames = {"hasDeprecatedInt32", "getDeprecatedInt32"};

  private final String[] deprecatedBuilderGetterNames = {
    "hasDeprecatedInt32", "getDeprecatedInt32", "clearDeprecatedInt32"
  };

  private final String[] deprecatedBuilderSetterNames = {"setDeprecatedInt32"};

  @Test
  public void testDeprecatedField() throws Exception {
    Class<?> deprecatedFields = TestDeprecatedFields.class;
    Class<?> deprecatedFieldsBuilder = TestDeprecatedFields.Builder.class;
    for (String name : deprecatedGetterNames) {
      Method method = deprecatedFields.getMethod(name);
      assertWithMessage("Method %s should be deprecated", name).that(isDeprecated(method)).isTrue();
    }
    for (String name : deprecatedBuilderGetterNames) {
      Method method = deprecatedFieldsBuilder.getMethod(name);
      assertWithMessage("Method %s should be deprecated", name).that(isDeprecated(method)).isTrue();
    }
    for (String name : deprecatedBuilderSetterNames) {
      Method method = deprecatedFieldsBuilder.getMethod(name, int.class);
      assertWithMessage("Method %s should be deprecated", name).that(isDeprecated(method)).isTrue();
    }
  }

  @Test
  public void testDeprecatedFieldInOneof() throws Exception {
    Class<?> oneofCase = TestDeprecatedFields.OneofFieldsCase.class;
    String name = "DEPRECATED_INT32_IN_ONEOF";
    java.lang.reflect.Field enumValue = oneofCase.getField(name);
    assertWithMessage("Enum value %s should be deprecated.", name)
        .that(isDeprecated(enumValue))
        .isTrue();
  }

  private boolean isDeprecated(AnnotatedElement annotated) {
    return annotated.isAnnotationPresent(Deprecated.class);
  }
}
