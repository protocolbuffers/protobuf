// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import com.google.protobuf.ExtensionRegistry.ExtensionInfo;
import proto2_unittest.NonNestedExtensionLite;
import proto2_unittest.UnittestProto;
import java.util.HashSet;
import java.util.Set;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

@RunWith(JUnit4.class)
public class MemoizingExtensionRegistryTest {
  @Mock private ExtensionRegistry delegate;
  private ExtensionInfo extensionInfo;
  private ExtensionRegistry registry;
  private Descriptors.Descriptor descriptor;

  @Before
  public void setup() {
    MockitoAnnotations.initMocks(this);
    registry = new MemoizingExtensionRegistry(delegate);
    extensionInfo = ExtensionRegistry.newExtensionInfo(UnittestProto.optionalInt32Extension);
    descriptor = UnittestProto.optionalInt32Extension.getDescriptor().getContainingType();
  }

  @Test
  public void testFindImmutableExtensionByName() {
    when(delegate.findImmutableExtensionByName("foo")).thenReturn(extensionInfo);
    assertThat(registry.findImmutableExtensionByName("foo")).isEqualTo(extensionInfo);
    assertThat(registry.findImmutableExtensionByName("foo")).isEqualTo(extensionInfo);
    verify(delegate, times(1)).findImmutableExtensionByName("foo");
  }

  @Test
  public void testFindImmutableExtensionByNameReturningNull() {
    when(delegate.findImmutableExtensionByName("foo")).thenReturn(null);
    assertThat(registry.findImmutableExtensionByName("foo")).isNull();
    assertThat(registry.findImmutableExtensionByName("foo")).isNull();
    verify(delegate, times(1)).findImmutableExtensionByName("foo");
  }

  @Test
  public void testFindMutableExtensionByName() {
    when(delegate.findMutableExtensionByName("foo")).thenReturn(extensionInfo);
    assertThat(registry.findMutableExtensionByName("foo")).isEqualTo(extensionInfo);
    assertThat(registry.findMutableExtensionByName("foo")).isEqualTo(extensionInfo);
    verify(delegate, times(1)).findMutableExtensionByName("foo");
  }

  @Test
  public void testFindMutableExtensionByNameReturningNull() {
    when(delegate.findMutableExtensionByName("foo")).thenReturn(null);
    assertThat(registry.findMutableExtensionByName("foo")).isNull();
    assertThat(registry.findMutableExtensionByName("foo")).isNull();
    verify(delegate, times(1)).findMutableExtensionByName("foo");
  }

  @Test
  public void testFindImmutableExtensionByNumber() {
    when(delegate.findImmutableExtensionByNumber(descriptor, 1)).thenReturn(extensionInfo);
    assertThat(registry.findImmutableExtensionByNumber(descriptor, 1)).isEqualTo(extensionInfo);
    assertThat(registry.findImmutableExtensionByNumber(descriptor, 1)).isEqualTo(extensionInfo);
    verify(delegate, times(1)).findImmutableExtensionByNumber(descriptor, 1);
  }

  @Test
  public void testFindImmutableExtensionByNumberReturningNull() {
    when(delegate.findImmutableExtensionByNumber(descriptor, 1)).thenReturn(null);
    assertThat(registry.findImmutableExtensionByNumber(descriptor, 1)).isEqualTo(null);
    assertThat(registry.findImmutableExtensionByNumber(descriptor, 1)).isEqualTo(null);
    verify(delegate, times(1)).findImmutableExtensionByNumber(descriptor, 1);
  }

  @Test
  public void testFindMutableExtensionByNumber() {
    when(delegate.findMutableExtensionByNumber(descriptor, 1)).thenReturn(extensionInfo);
    assertThat(registry.findMutableExtensionByNumber(descriptor, 1)).isEqualTo(extensionInfo);
    assertThat(registry.findMutableExtensionByNumber(descriptor, 1)).isEqualTo(extensionInfo);
    verify(delegate, times(1)).findMutableExtensionByNumber(descriptor, 1);
  }

  @Test
  public void testFindMutableExtensionByNumberReturningNull() {
    when(delegate.findMutableExtensionByNumber(descriptor, 1)).thenReturn(null);
    assertThat(registry.findMutableExtensionByNumber(descriptor, 1)).isEqualTo(null);
    assertThat(registry.findMutableExtensionByNumber(descriptor, 1)).isEqualTo(null);
    verify(delegate, times(1)).findMutableExtensionByNumber(descriptor, 1);
  }

  @Test
  public void testGetAllImmutableExtensionsByExtendedType() {
    Set<ExtensionRegistry.ExtensionInfo> extensions = new HashSet<>();
    extensions.add(extensionInfo);

    when(delegate.getAllImmutableExtensionsByExtendedType("foo")).thenReturn(extensions);
    assertThat(registry.getAllImmutableExtensionsByExtendedType("foo")).isEqualTo(extensions);
    verify(delegate).getAllImmutableExtensionsByExtendedType("foo");
  }

  @Test
  public void testGetAllMutableExtensionsByExtendedType() {
    Set<ExtensionRegistry.ExtensionInfo> extensions = new HashSet<>();
    extensions.add(extensionInfo);

    when(delegate.getAllMutableExtensionsByExtendedType("foo")).thenReturn(extensions);
    assertThat(registry.getAllMutableExtensionsByExtendedType("foo")).isEqualTo(extensions);
    verify(delegate).getAllMutableExtensionsByExtendedType("foo");
  }

  @Test
  public void testUnmodifiableIsSelf() {
    assertThat(registry.getUnmodifiable()).isSameInstanceAs(registry);
  }

  @Test
  public void testAddExtensionNotSupported() {
    assertThrows(
        UnsupportedOperationException.class,
        () -> registry.add(UnittestProto.optionalInt32Extension));
  }

  @Test
  public void testAddDescriptorNotSupported() {
    assertThrows(
        UnsupportedOperationException.class,
        () -> registry.add(UnittestProto.optionalInt32Extension.getDescriptor()));
  }

  @Test
  public void testAddDescriptorAndMessageNotSupported() {
    assertThrows(
        UnsupportedOperationException.class,
        () ->
            registry.add(
                UnittestProto.optionalForeignMessageExtension.getDescriptor(),
                UnittestProto.ForeignMessage.getDefaultInstance()));
  }

  @Test
  public void testFindLiteExtensionByNumberNotSupported() {
    assertThrows(
        UnsupportedOperationException.class,
        () ->
            registry.findLiteExtensionByNumber(
                NonNestedExtensionLite.MessageLiteToBeExtended.getDefaultInstance(), 1));
  }
}
