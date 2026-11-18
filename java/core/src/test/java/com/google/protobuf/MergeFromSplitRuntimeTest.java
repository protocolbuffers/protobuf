// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;

import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.Descriptors.OneofDescriptor;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public final class MergeFromSplitRuntimeTest {

  private void testBoundaryMessage(Message defaultInstance, int fieldCount) throws Exception {
    Descriptor descriptor = defaultInstance.getDescriptorForType();
    Message.Builder sourceBuilder = defaultInstance.newBuilderForType();
    Message.Builder expectedBuilder = defaultInstance.newBuilderForType();

    // Strategy 1: Empty to Full
    for (int i = 1; i <= fieldCount; i++) {
      FieldDescriptor fd = descriptor.findFieldByNumber(i);
      assertThat(fd).isNotNull();
      sourceBuilder.setField(fd, i);
      expectedBuilder.setField(fd, i);
    }

    Message source = sourceBuilder.build();
    Message expected = expectedBuilder.build();

    Message.Builder destinationBuilder = defaultInstance.newBuilderForType();
    destinationBuilder.mergeFrom(source);

    Message actual = destinationBuilder.build();
    assertThat(actual).isEqualTo(expected);

    // Strategy 2: Overwrite Full
    Message.Builder destinationBuilder2 = defaultInstance.newBuilderForType();
    for (int i = 1; i <= fieldCount; i++) {
      FieldDescriptor fd = descriptor.findFieldByNumber(i);
      destinationBuilder2.setField(fd, i * 10);
    }
    destinationBuilder2.mergeFrom(source);

    Message.Builder expectedBuilder2 = defaultInstance.newBuilderForType();
    for (int i = 1; i <= fieldCount; i++) {
      FieldDescriptor fd = descriptor.findFieldByNumber(i);
      expectedBuilder2.setField(fd, i); // mergeFrom OVERWRITES non-repeated primitives
    }
    assertThat(destinationBuilder2.build()).isEqualTo(expectedBuilder2.build());

    // Strategy 3: Sparse merging (odd in source, even in dest)
    Message.Builder sourceBuilderSparse = defaultInstance.newBuilderForType();
    Message.Builder destBuilderSparse = defaultInstance.newBuilderForType();
    Message.Builder expectedBuilderSparse = defaultInstance.newBuilderForType();

    for (int i = 1; i <= fieldCount; i++) {
      FieldDescriptor fd = descriptor.findFieldByNumber(i);
      if (i % 2 != 0) {
        sourceBuilderSparse.setField(fd, i);
        expectedBuilderSparse.setField(fd, i);
      } else {
        destBuilderSparse.setField(fd, i * 10);
        expectedBuilderSparse.setField(fd, i * 10);
      }
    }

    Message sourceSparse = sourceBuilderSparse.build();
    destBuilderSparse.mergeFrom(sourceSparse);

    Message actualSparse = destBuilderSparse.build();
    assertThat(actualSparse).isEqualTo(expectedBuilderSparse.build());

    // Strategy 4: Overlapping Sparse merging
    Message.Builder sourceBuilderOverlap = defaultInstance.newBuilderForType();
    Message.Builder destBuilderOverlap = defaultInstance.newBuilderForType();
    Message.Builder expectedBuilderOverlap = defaultInstance.newBuilderForType();

    for (int i = 1; i <= fieldCount; i++) {
      FieldDescriptor fd = descriptor.findFieldByNumber(i);
      if (i % 2 == 0) {
        sourceBuilderOverlap.setField(fd, i * 100);
        expectedBuilderOverlap.setField(fd, i * 100);
      }
      if (i % 3 == 0) {
        destBuilderOverlap.setField(fd, i * 10);
        if (i % 2 != 0) {
          expectedBuilderOverlap.setField(fd, i * 10);
        }
      }
    }

    Message sourceOverlap = sourceBuilderOverlap.build();
    destBuilderOverlap.mergeFrom(sourceOverlap);

    Message actualOverlap = destBuilderOverlap.build();
    assertThat(actualOverlap).isEqualTo(expectedBuilderOverlap.build());
  }

  @Test
  public void testBoundary32() throws Exception {
    testBoundaryMessage(Boundary32.getDefaultInstance(), 32);
  }

  @Test
  public void testBoundary33() throws Exception {
    testBoundaryMessage(Boundary33.getDefaultInstance(), 33);
  }

  @Test
  public void testBoundary1024() throws Exception {
    testBoundaryMessage(Boundary1024.getDefaultInstance(), 1024);
  }

  @Test
  public void testBoundary1025() throws Exception {
    testBoundaryMessage(Boundary1025.getDefaultInstance(), 1025);
  }

  @Test
  public void testLargeButSafeOneof() throws Exception {
    Descriptor descriptor = LargeButSafeOneof.getDescriptor();
    OneofDescriptor oneof = descriptor.getOneofs().get(0);
    assertThat(oneof.getName()).isEqualTo("safe_oneof");
    assertThat(oneof.getFields()).hasSize(500);

    for (int i = 1; i <= 500; i++) {
      FieldDescriptor fd = descriptor.findFieldByNumber(i);
      assertThat(fd).isNotNull();

      LargeButSafeOneof source = LargeButSafeOneof.newBuilder().setField(fd, i).build();
      LargeButSafeOneof actual = LargeButSafeOneof.newBuilder().mergeFrom(source).build();

      assertThat(actual).isEqualTo(source);
      assertThat(actual.getSafeOneofCase().getNumber()).isEqualTo(i);
      assertThat(actual.getField(fd)).isEqualTo(i);

      // Verify merging with EXISTING different field in oneof
      int otherFieldNum = (i % 500) + 1;
      FieldDescriptor otherFd = descriptor.findFieldByNumber(otherFieldNum);
      assertThat(otherFd).isNotNull();
      LargeButSafeOneof destDifferent =
          LargeButSafeOneof.newBuilder().setField(otherFd, 999).build();
      LargeButSafeOneof mergedDifferent = destDifferent.toBuilder().mergeFrom(source).build();
      assertThat(mergedDifferent).isEqualTo(source);
      assertThat(mergedDifferent.getSafeOneofCase().getNumber()).isEqualTo(i);
      assertThat(mergedDifferent.getField(fd)).isEqualTo(i);
      assertThat(mergedDifferent.hasField(otherFd)).isFalse();

      // Verify merging with EXISTING SAME field in oneof
      LargeButSafeOneof destSame = LargeButSafeOneof.newBuilder().setField(fd, 999).build();
      LargeButSafeOneof mergedSame = destSame.toBuilder().mergeFrom(source).build();
      assertThat(mergedSame).isEqualTo(source); // Overwrites
      assertThat(mergedSame.getField(fd)).isEqualTo(i);
    }
  }

  private void testBoundaryWithOneof(
      Message defaultInstance, int fieldCount, int oneofField1Num, int oneofField2Num)
      throws Exception {
    Descriptor descriptor = defaultInstance.getDescriptorForType();

    // Test with oneofField1 set in source
    for (int i = 1; i <= fieldCount; i++) {
      FieldDescriptor fd = descriptor.findFieldByNumber(i);
      assertThat(fd).isNotNull();
    }
    FieldDescriptor fdOneof1 = descriptor.findFieldByNumber(oneofField1Num);
    FieldDescriptor fdOneof2 = descriptor.findFieldByNumber(oneofField2Num);
    assertThat(fdOneof1).isNotNull();
    assertThat(fdOneof2).isNotNull();

    Message.Builder sourceBuilder = defaultInstance.newBuilderForType();
    Message.Builder expectedBuilder = defaultInstance.newBuilderForType();

    for (int i = 1; i <= fieldCount; i++) {
      FieldDescriptor fd = descriptor.findFieldByNumber(i);
      sourceBuilder.setField(fd, i);
      expectedBuilder.setField(fd, i);
    }
    sourceBuilder.setField(fdOneof1, oneofField1Num);
    expectedBuilder.setField(fdOneof1, oneofField1Num);

    Message source = sourceBuilder.build();
    Message expected = expectedBuilder.build();

    // Merge to empty
    Message.Builder destBuilder = defaultInstance.newBuilderForType();
    destBuilder.mergeFrom(source);
    assertThat(destBuilder.build()).isEqualTo(expected);

    // Merge to dest with oneofField2 set
    Message.Builder destBuilder2 = defaultInstance.newBuilderForType();
    for (int i = 1; i <= fieldCount; i++) {
      FieldDescriptor fd = descriptor.findFieldByNumber(i);
      destBuilder2.setField(fd, i * 10);
    }
    destBuilder2.setField(fdOneof2, 999);
    destBuilder2.mergeFrom(source);

    Message.Builder expectedBuilder2 = defaultInstance.newBuilderForType();
    for (int i = 1; i <= fieldCount; i++) {
      FieldDescriptor fd = descriptor.findFieldByNumber(i);
      expectedBuilder2.setField(fd, i);
    }
    expectedBuilder2.setField(fdOneof1, oneofField1Num);
    assertThat(destBuilder2.build()).isEqualTo(expectedBuilder2.build());

    // Check that oneofField2 was CLEARED
    Message actual2 = destBuilder2.build();
    assertThat(actual2.hasField(fdOneof2)).isFalse();

    // Test with oneofField2 set in source
    Message.Builder sourceBuilder3 = defaultInstance.newBuilderForType();
    Message.Builder expectedBuilder3 = defaultInstance.newBuilderForType();

    for (int i = 1; i <= fieldCount; i++) {
      FieldDescriptor fd = descriptor.findFieldByNumber(i);
      sourceBuilder3.setField(fd, i * 100);
      expectedBuilder3.setField(fd, i * 100);
    }
    sourceBuilder3.setField(fdOneof2, oneofField2Num);
    expectedBuilder3.setField(fdOneof2, oneofField2Num);

    Message.Builder destBuilder3 =
        destBuilder2; // Reuse dest with oneofField1 set from previous merge actual2
    destBuilder3.mergeFrom(sourceBuilder3.build());
    assertThat(destBuilder3.build()).isEqualTo(expectedBuilder3.build());
    Message actual3 = destBuilder3.build();
    assertThat(actual3.hasField(fdOneof1)).isFalse();
  }

  @Test
  public void testBoundary32WithOneof() throws Exception {
    testBoundaryWithOneof(Boundary32WithOneof.getDefaultInstance(), 32, 33, 34);
  }

  @Test
  public void testBoundary1024WithOneof() throws Exception {
    testBoundaryWithOneof(Boundary1024WithOneof.getDefaultInstance(), 1024, 1025, 1026);
  }
}
