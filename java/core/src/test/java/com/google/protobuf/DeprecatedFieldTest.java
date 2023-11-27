// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

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
