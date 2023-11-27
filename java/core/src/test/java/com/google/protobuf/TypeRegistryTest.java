// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import com.google.protobuf.Descriptors.Descriptor;
import protobuf_unittest.UnittestProto;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public final class TypeRegistryTest {

  @Test
  public void getDescriptorForTypeUrl_throwsExceptionForUnknownTypes() throws Exception {
    assertThrows(
        InvalidProtocolBufferException.class,
        () -> TypeRegistry.getEmptyTypeRegistry().getDescriptorForTypeUrl("UnknownType"));
    assertThrows(
        InvalidProtocolBufferException.class,
        () -> TypeRegistry.getEmptyTypeRegistry().getDescriptorForTypeUrl("///"));
  }

  @Test
  public void findDescriptorByFullName() throws Exception {
    Descriptor descriptor = UnittestProto.TestAllTypes.getDescriptor();
    assertThat(TypeRegistry.getEmptyTypeRegistry().find(descriptor.getFullName())).isNull();

    assertThat(TypeRegistry.newBuilder().add(descriptor).build().find(descriptor.getFullName()))
        .isSameInstanceAs(descriptor);
  }

  @Test
  public void findDescriptorByTypeUrl() throws Exception {
    Descriptor descriptor = UnittestProto.TestAllTypes.getDescriptor();
    assertThat(
            TypeRegistry.getEmptyTypeRegistry()
                .getDescriptorForTypeUrl("type.googleapis.com/" + descriptor.getFullName()))
        .isNull();

    assertThat(
            TypeRegistry.newBuilder()
                .add(descriptor)
                .build()
                .getDescriptorForTypeUrl("type.googleapis.com/" + descriptor.getFullName()))
        .isSameInstanceAs(descriptor);
  }
}
