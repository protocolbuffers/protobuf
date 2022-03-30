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

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import protobuf_unittest.LazyFieldsLite.LazyExtension;
import protobuf_unittest.LazyFieldsLite.LazyInnerMessageLite;
import protobuf_unittest.LazyFieldsLite.LazyMessageLite;
import protobuf_unittest.LazyFieldsLite.LazyNestedInnerMessageLite;
import java.util.ArrayList;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit test for messages with lazy fields. */
@RunWith(JUnit4.class)
public class LazyMessageLiteTest {

  @Test
  public void testSetValues() {
    LazyNestedInnerMessageLite nested = LazyNestedInnerMessageLite.newBuilder().setNum(3).build();
    LazyInnerMessageLite inner =
        LazyInnerMessageLite.newBuilder().setNum(2).setNested(nested).build();
    LazyMessageLite outer =
        LazyMessageLite.newBuilder()
            .setNum(1)
            .setInner(inner)
            .setOneofNum(123)
            .setOneofInner(inner)
            .build();

    assertThat(outer.getNum()).isEqualTo(1);
    assertThat(outer.getNumWithDefault()).isEqualTo(421);

    assertThat(outer.getInner().getNum()).isEqualTo(2);
    assertThat(outer.getInner().getNumWithDefault()).isEqualTo(42);

    assertThat(outer.getInner().getNum()).isEqualTo(2);
    assertThat(outer.getInner().getNumWithDefault()).isEqualTo(42);

    assertThat(outer.getInner().getNested().getNum()).isEqualTo(3);
    assertThat(outer.getInner().getNested().getNumWithDefault()).isEqualTo(4);

    assertThat(outer.hasOneofNum()).isFalse();
    assertThat(outer.hasOneofInner()).isTrue();

    assertThat(outer.getOneofInner().getNum()).isEqualTo(2);
    assertThat(outer.getOneofInner().getNumWithDefault()).isEqualTo(42);
    assertThat(outer.getOneofInner().getNested().getNum()).isEqualTo(3);
    assertThat(outer.getOneofInner().getNested().getNumWithDefault()).isEqualTo(4);
  }

  @Test
  public void testSetRepeatedValues() {
    LazyMessageLite outer =
        LazyMessageLite.newBuilder()
            .setNum(1)
            .addRepeatedInner(LazyInnerMessageLite.newBuilder().setNum(119))
            .addRepeatedInner(LazyInnerMessageLite.newBuilder().setNum(122))
            .build();

    assertThat(outer.getNum()).isEqualTo(1);
    assertThat(outer.getRepeatedInnerCount()).isEqualTo(2);
    assertThat(outer.getRepeatedInner(0).getNum()).isEqualTo(119);
    assertThat(outer.getRepeatedInner(1).getNum()).isEqualTo(122);
  }

  @Test
  public void testRepeatedMutability() throws Exception {
    LazyMessageLite outer =
        LazyMessageLite.newBuilder()
            .addRepeatedInner(LazyInnerMessageLite.newBuilder().setNum(119))
            .addRepeatedInner(LazyInnerMessageLite.newBuilder().setNum(122))
            .build();

    outer =
        LazyMessageLite.parseFrom(outer.toByteArray(),
            ExtensionRegistryLite.getEmptyRegistry());
    try {
      outer.getRepeatedInnerList().set(1, null);
      assertWithMessage("expected exception").fail();
    } catch (UnsupportedOperationException expected) {
    }
  }

  @Test
  public void testAddAll() {
    ArrayList<LazyInnerMessageLite> inners = new ArrayList<>();
    int count = 4;
    for (int i = 0; i < count; i++) {
      LazyInnerMessageLite inner = LazyInnerMessageLite.newBuilder().setNum(i).build();
      inners.add(inner);
    }

    LazyMessageLite outer = LazyMessageLite.newBuilder().addAllRepeatedInner(inners).build();
    assertThat(outer.getRepeatedInnerCount()).isEqualTo(count);
    for (int i = 0; i < count; i++) {
      assertThat(outer.getRepeatedInner(i).getNum()).isEqualTo(i);
    }
  }

  @Test
  public void testGetDefaultValues() {
    LazyMessageLite outer = LazyMessageLite.getDefaultInstance();

    assertThat(outer.getNum()).isEqualTo(0);
    assertThat(outer.getNumWithDefault()).isEqualTo(421);

    assertThat(outer.getInner().getNum()).isEqualTo(0);
    assertThat(outer.getInner().getNumWithDefault()).isEqualTo(42);

    assertThat(outer.getInner().getNested().getNum()).isEqualTo(0);
    assertThat(outer.getInner().getNested().getNumWithDefault()).isEqualTo(4);

    assertThat(outer.getOneofNum()).isEqualTo(0);

    assertThat(outer.getOneofInner().getNum()).isEqualTo(0);
    assertThat(outer.getOneofInner().getNumWithDefault()).isEqualTo(42);
    assertThat(outer.getOneofInner().getNested().getNum()).isEqualTo(0);
    assertThat(outer.getOneofInner().getNested().getNumWithDefault()).isEqualTo(4);
  }

  @Test
  public void testClearValues() {
    LazyInnerMessageLite inner = LazyInnerMessageLite.newBuilder().setNum(115).build();

    LazyMessageLite.Builder outerBuilder = LazyMessageLite.newBuilder();

    assertThat(outerBuilder.build().getNum()).isEqualTo(0);

    // Set/Clear num
    outerBuilder.setNum(100);

    assertThat(outerBuilder.build().getNum()).isEqualTo(100);
    assertThat(outerBuilder.build().getNumWithDefault()).isEqualTo(421);
    assertThat(outerBuilder.build().hasInner()).isFalse();

    outerBuilder.clearNum();

    assertThat(outerBuilder.build().getNum()).isEqualTo(0);
    assertThat(outerBuilder.build().getNumWithDefault()).isEqualTo(421);
    assertThat(outerBuilder.build().hasInner()).isFalse();

    // Set/Clear all
    outerBuilder
        .setNum(100)
        .setInner(inner)
        .addRepeatedInner(LazyInnerMessageLite.newBuilder().setNum(119))
        .addRepeatedInner(LazyInnerMessageLite.newBuilder().setNum(122))
        .setOneofInner(LazyInnerMessageLite.newBuilder().setNum(123));

    LazyMessageLite outer = outerBuilder.build();
    assertThat(outer.getNum()).isEqualTo(100);
    assertThat(outer.getNumWithDefault()).isEqualTo(421);
    assertThat(outer.hasInner()).isTrue();
    assertThat(outer.getInner().getNum()).isEqualTo(115);
    assertThat(outer.getRepeatedInnerCount()).isEqualTo(2);
    assertThat(outer.getRepeatedInner(0).getNum()).isEqualTo(119);
    assertThat(outer.getRepeatedInner(1).getNum()).isEqualTo(122);
    assertThat(outer.hasOneofInner()).isTrue();
    assertThat(outer.getOneofInner().getNum()).isEqualTo(123);

    outerBuilder.clear();

    outer = outerBuilder.build();

    assertThat(outer.getNum()).isEqualTo(0);
    assertThat(outer.getNumWithDefault()).isEqualTo(421);
    assertThat(outer.hasInner()).isFalse();
    assertThat(outer.getRepeatedInnerCount()).isEqualTo(0);
    assertThat(outer.hasOneofInner()).isFalse();
    assertThat(outer.getOneofInner().getNum()).isEqualTo(0);
  }

  @Test
  public void testMergeValues() {
    LazyMessageLite outerBase = LazyMessageLite.newBuilder().setNumWithDefault(122).build();

    LazyInnerMessageLite innerMerging = LazyInnerMessageLite.newBuilder().setNum(115).build();
    LazyMessageLite outerMerging =
        LazyMessageLite.newBuilder()
            .setNum(119)
            .setInner(innerMerging)
            .setOneofInner(innerMerging)
            .build();

    LazyMessageLite merged = LazyMessageLite.newBuilder(outerBase).mergeFrom(outerMerging).build();
    assertThat(merged.getNum()).isEqualTo(119);
    assertThat(merged.getNumWithDefault()).isEqualTo(122);
    assertThat(merged.getInner().getNum()).isEqualTo(115);
    assertThat(merged.getInner().getNumWithDefault()).isEqualTo(42);
    assertThat(merged.getOneofInner().getNum()).isEqualTo(115);
    assertThat(merged.getOneofInner().getNumWithDefault()).isEqualTo(42);
  }

  @Test
  public void testMergeDefaultValues() {
    LazyInnerMessageLite innerBase = LazyInnerMessageLite.newBuilder().setNum(115).build();
    LazyMessageLite outerBase =
        LazyMessageLite.newBuilder()
            .setNum(119)
            .setNumWithDefault(122)
            .setInner(innerBase)
            .setOneofInner(innerBase)
            .build();

    LazyMessageLite outerMerging = LazyMessageLite.getDefaultInstance();

    LazyMessageLite merged = LazyMessageLite.newBuilder(outerBase).mergeFrom(outerMerging).build();
    // Merging default-instance shouldn't overwrite values in the base message.
    assertThat(merged.getNum()).isEqualTo(119);
    assertThat(merged.getNumWithDefault()).isEqualTo(122);
    assertThat(merged.getInner().getNum()).isEqualTo(115);
    assertThat(merged.getInner().getNumWithDefault()).isEqualTo(42);
    assertThat(merged.getOneofInner().getNum()).isEqualTo(115);
    assertThat(merged.getOneofInner().getNumWithDefault()).isEqualTo(42);
  }

  // Regression test for b/28198805.
  @Test
  public void testMergeOneofMessages() throws Exception {
    LazyInnerMessageLite inner = LazyInnerMessageLite.getDefaultInstance();
    LazyMessageLite outer = LazyMessageLite.newBuilder().setOneofInner(inner).build();
    ByteString data1 = outer.toByteString();

    // The following should not alter the content of the 'outer' message.
    LazyMessageLite.Builder merged = outer.toBuilder();
    LazyInnerMessageLite anotherInner = LazyInnerMessageLite.newBuilder().setNum(12345).build();
    merged.setOneofInner(anotherInner);

    // Check that the 'outer' stays the same.
    ByteString data2 = outer.toByteString();
    assertThat(data1).isEqualTo(data2);
    assertThat(outer.getOneofInner().getNum()).isEqualTo(0);
  }

  @Test
  public void testSerialize() throws InvalidProtocolBufferException {
    LazyNestedInnerMessageLite nested = LazyNestedInnerMessageLite.newBuilder().setNum(3).build();
    LazyInnerMessageLite inner =
        LazyInnerMessageLite.newBuilder().setNum(2).setNested(nested).build();
    LazyMessageLite outer =
        LazyMessageLite.newBuilder().setNum(1).setInner(inner).setOneofInner(inner).build();

    ByteString bytes = outer.toByteString();
    assertThat(bytes.size()).isEqualTo(outer.getSerializedSize());

    LazyMessageLite deserialized =
        LazyMessageLite.parseFrom(bytes, ExtensionRegistryLite.getEmptyRegistry());

    assertThat(deserialized.getNum()).isEqualTo(1);
    assertThat(deserialized.getNumWithDefault()).isEqualTo(421);

    assertThat(deserialized.getInner().getNum()).isEqualTo(2);
    assertThat(deserialized.getInner().getNumWithDefault()).isEqualTo(42);

    assertThat(deserialized.getInner().getNested().getNum()).isEqualTo(3);
    assertThat(deserialized.getInner().getNested().getNumWithDefault()).isEqualTo(4);

    assertThat(deserialized.getOneofInner().getNum()).isEqualTo(2);
    assertThat(deserialized.getOneofInner().getNumWithDefault()).isEqualTo(42);
    assertThat(deserialized.getOneofInner().getNested().getNum()).isEqualTo(3);
    assertThat(deserialized.getOneofInner().getNested().getNumWithDefault()).isEqualTo(4);

    assertThat(deserialized.toByteString()).isEqualTo(bytes);
  }

  @Test
  public void testExtensions() throws Exception {
    LazyInnerMessageLite.Builder innerBuilder = LazyInnerMessageLite.newBuilder();
    innerBuilder.setExtension(
        LazyExtension.extension, LazyExtension.newBuilder().setName("name").build());
    assertThat(innerBuilder.hasExtension(LazyExtension.extension)).isTrue();
    assertThat(innerBuilder.getExtension(LazyExtension.extension).getName()).isEqualTo("name");

    LazyInnerMessageLite innerMessage = innerBuilder.build();
    assertThat(innerMessage.hasExtension(LazyExtension.extension)).isTrue();
    assertThat(innerMessage.getExtension(LazyExtension.extension).getName()).isEqualTo("name");

    LazyMessageLite lite = LazyMessageLite.newBuilder().setInner(innerMessage).build();
    assertThat(lite.getInner().hasExtension(LazyExtension.extension)).isTrue();
    assertThat(lite.getInner().getExtension(LazyExtension.extension).getName()).isEqualTo("name");
  }
}
