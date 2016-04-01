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

import com.google.protobuf.UnittestLite.ForeignEnumLite;
import com.google.protobuf.UnittestLite.TestAllTypesLite;
import protobuf_unittest.UnittestProto.ForeignEnum;
import protobuf_unittest.UnittestProto.TestAllTypes;

import junit.framework.TestCase;

public class EnumTest extends TestCase {
  
  public void testForNumber() {
    ForeignEnum e = ForeignEnum.forNumber(ForeignEnum.FOREIGN_BAR.getNumber());
    assertEquals(ForeignEnum.FOREIGN_BAR, e);

    e = ForeignEnum.forNumber(1000);
    assertEquals(null, e);
  }
  
  public void testForNumber_oneof() {
    TestAllTypes.OneofFieldCase e = TestAllTypes.OneofFieldCase.forNumber(
        TestAllTypes.OneofFieldCase.ONEOF_NESTED_MESSAGE.getNumber());
    assertEquals(TestAllTypes.OneofFieldCase.ONEOF_NESTED_MESSAGE, e);

    e = TestAllTypes.OneofFieldCase.forNumber(1000);
    assertEquals(null, e);
  }
  
  public void testForNumberLite() {
    ForeignEnumLite e = ForeignEnumLite.forNumber(ForeignEnumLite.FOREIGN_LITE_BAR.getNumber());
    assertEquals(ForeignEnumLite.FOREIGN_LITE_BAR, e);

    e = ForeignEnumLite.forNumber(1000);
    assertEquals(null, e);
  }
  
  public void testForNumberLite_oneof() {
    TestAllTypesLite.OneofFieldCase e = TestAllTypesLite.OneofFieldCase.forNumber(
        TestAllTypesLite.OneofFieldCase.ONEOF_NESTED_MESSAGE.getNumber());
    assertEquals(TestAllTypesLite.OneofFieldCase.ONEOF_NESTED_MESSAGE, e);

    e = TestAllTypesLite.OneofFieldCase.forNumber(1000);
    assertEquals(null, e);
  }
}

