// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;

import com.google.protobuf.UnittestLite.ForeignEnumLite;
import com.google.protobuf.UnittestLite.TestAllTypesLite;
import protobuf_unittest.UnittestProto.ForeignEnum;
import protobuf_unittest.UnittestProto.TestAllTypes;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class EnumTest {

  @Test
  public void testForNumber() {
    ForeignEnum e = ForeignEnum.forNumber(ForeignEnum.FOREIGN_BAR.getNumber());
    assertThat(e).isEqualTo(ForeignEnum.FOREIGN_BAR);

    e = ForeignEnum.forNumber(1000);
    assertThat(e).isNull();
  }

  @Test
  public void testForNumber_oneof() {
    TestAllTypes.OneofFieldCase e =
        TestAllTypes.OneofFieldCase.forNumber(
            TestAllTypes.OneofFieldCase.ONEOF_NESTED_MESSAGE.getNumber());
    assertThat(e).isEqualTo(TestAllTypes.OneofFieldCase.ONEOF_NESTED_MESSAGE);

    e = TestAllTypes.OneofFieldCase.forNumber(1000);
    assertThat(e).isNull();
  }

  @Test
  public void testForNumberLite() {
    ForeignEnumLite e = ForeignEnumLite.forNumber(ForeignEnumLite.FOREIGN_LITE_BAR.getNumber());
    assertThat(e).isEqualTo(ForeignEnumLite.FOREIGN_LITE_BAR);

    e = ForeignEnumLite.forNumber(1000);
    assertThat(e).isNull();
  }

  @Test
  public void testForNumberLite_oneof() {
    TestAllTypesLite.OneofFieldCase e =
        TestAllTypesLite.OneofFieldCase.forNumber(
            TestAllTypesLite.OneofFieldCase.ONEOF_NESTED_MESSAGE.getNumber());
    assertThat(e).isEqualTo(TestAllTypesLite.OneofFieldCase.ONEOF_NESTED_MESSAGE);

    e = TestAllTypesLite.OneofFieldCase.forNumber(1000);
    assertThat(e).isNull();
  }
}
