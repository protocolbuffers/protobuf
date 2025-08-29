// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoInteractions;
import static org.mockito.Mockito.when;

import com.google.protobuf.ExtensionRegistry.ExtensionInfo;
import proto2_unittest.NonNestedExtensionLite;
import proto2_unittest.UnittestProto;
import java.util.HashSet;
import java.util.Set;
import java.util.function.Function;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

@RunWith(JUnit4.class)
public class DelegatingExtensionRegistryTest {
  @Mock private ExtensionRegistry mock1;
  private ExtensionInfo extensionInfo1;
  private Descriptors.Descriptor descriptor1;
  @Mock private ExtensionRegistry mock2;
  private ExtensionInfo extensionInfo2;
  private Descriptors.Descriptor descriptor2;

  private ExtensionRegistry delegate;
  private ExtensionRegistry registry;

  @Before
  public void setup() {
    MockitoAnnotations.initMocks(this);
    registry = DelegatingExtensionRegistry.createDelegating(() -> delegate);
    extensionInfo1 = ExtensionRegistry.newExtensionInfo(UnittestProto.optionalInt32Extension);
    descriptor1 = UnittestProto.optionalInt32Extension.getDescriptor().getContainingType();
    extensionInfo2 = ExtensionRegistry.newExtensionInfo(UnittestProto.optionalInt64Extension);
    descriptor2 = UnittestProto.optionalInt64Extension.getDescriptor().getContainingType();
  }

  private <T> void assertUsesCurrentDelegate(
      Function<ExtensionRegistry, T> fn, T expected1, T expected2) {
    verifyNoInteractions(mock1);
    verifyNoInteractions(mock2);
    delegate = mock1;
    assertThat(fn.apply(registry)).isEqualTo(expected1);
    Object unused = fn.apply(verify(mock1, times(1)));
    verifyNoInteractions(mock2);

    reset(mock1);
    delegate = mock2;
    assertThat(fn.apply(registry)).isEqualTo(expected2);
    unused = fn.apply(verify(mock2, times(1)));
    verifyNoInteractions(mock1);
  }

  @Test
  public void testFindImmutableExtensionByName() {
    when(mock1.findImmutableExtensionByName("foo")).thenReturn(extensionInfo1);
    when(mock2.findImmutableExtensionByName("foo")).thenReturn(extensionInfo2);

    assertUsesCurrentDelegate(
        r -> r.findImmutableExtensionByName("foo"), extensionInfo1, extensionInfo2);
  }

  @Test
  public void testFindImmutableExtensionByNameReturningNull() {
    when(mock1.findImmutableExtensionByName("foo")).thenReturn(null);
    when(mock2.findImmutableExtensionByName("foo")).thenReturn(null);

    assertUsesCurrentDelegate(r -> r.findImmutableExtensionByName("foo"), null, null);
  }

  @Test
  public void testFindMutableExtensionByName() {
    when(mock1.findMutableExtensionByName("foo")).thenReturn(extensionInfo1);
    when(mock2.findMutableExtensionByName("foo")).thenReturn(extensionInfo2);

    assertUsesCurrentDelegate(
        r -> r.findMutableExtensionByName("foo"), extensionInfo1, extensionInfo2);
  }

  @Test
  public void testFindMutableExtensionByNameReturningNull() {
    when(mock1.findMutableExtensionByName("foo")).thenReturn(null);
    when(mock2.findMutableExtensionByName("foo")).thenReturn(null);

    assertUsesCurrentDelegate(r -> r.findMutableExtensionByName("foo"), null, null);
  }

  @Test
  public void testFindImmutableExtensionByNumber() {
    when(mock1.findImmutableExtensionByNumber(descriptor1, 1)).thenReturn(extensionInfo1);
    when(mock2.findImmutableExtensionByNumber(descriptor1, 1)).thenReturn(extensionInfo1);

    assertUsesCurrentDelegate(
        r -> r.findImmutableExtensionByNumber(descriptor1, 1), extensionInfo1, extensionInfo1);
  }

  @Test
  public void testFindImmutableExtensionByNumberReturningNull() {
    when(mock1.findImmutableExtensionByNumber(descriptor1, 1)).thenReturn(null);
    when(mock2.findImmutableExtensionByNumber(descriptor1, 1)).thenReturn(null);

    assertUsesCurrentDelegate(r -> r.findImmutableExtensionByNumber(descriptor1, 1), null, null);
  }

  @Test
  public void testFindMutableExtensionByNumber() {
    when(mock1.findMutableExtensionByNumber(descriptor1, 1)).thenReturn(extensionInfo1);
    when(mock2.findMutableExtensionByNumber(descriptor1, 1)).thenReturn(extensionInfo1);

    assertUsesCurrentDelegate(
        r -> r.findMutableExtensionByNumber(descriptor1, 1), extensionInfo1, extensionInfo1);
  }

  @Test
  public void testFindMutableExtensionByNumberReturningNull() {
    when(mock1.findMutableExtensionByNumber(descriptor1, 1)).thenReturn(null);
    when(mock2.findMutableExtensionByNumber(descriptor1, 1)).thenReturn(null);

    assertUsesCurrentDelegate(r -> r.findMutableExtensionByNumber(descriptor1, 1), null, null);
  }

  @SuppressWarnings({"unchecked", "rawtypes"}) // its too hard to get a proper lite extension
  @Test
  public void testFindLiteExtensionByNumber() {
    NonNestedExtensionLite.MessageLiteToBeExtended msg =
        NonNestedExtensionLite.MessageLiteToBeExtended.getDefaultInstance();

    GeneratedMessageLite.GeneratedExtension rawExt =
        mock(GeneratedMessageLite.GeneratedExtension.class);

    when(mock1.findLiteExtensionByNumber(msg, 1)).thenReturn(rawExt);
    when(mock2.findLiteExtensionByNumber(msg, 1)).thenReturn(rawExt);

    assertUsesCurrentDelegate(r -> r.findLiteExtensionByNumber(msg, 1), rawExt, rawExt);
  }

  @Test
  public void testGetAllImmutableExtensionsByExtendedType() {
    Set<ExtensionRegistry.ExtensionInfo> extensions1 = new HashSet<>();
    extensions1.add(extensionInfo1);

    Set<ExtensionRegistry.ExtensionInfo> extensions2 = new HashSet<>();
    extensions2.add(extensionInfo2);

    when(mock1.getAllImmutableExtensionsByExtendedType("foo")).thenReturn(extensions1);
    when(mock2.getAllImmutableExtensionsByExtendedType("foo")).thenReturn(extensions2);

    assertUsesCurrentDelegate(
        r -> r.getAllImmutableExtensionsByExtendedType("foo"), extensions1, extensions2);
  }

  @Test
  public void testGetAllMutableExtensionsByExtendedType() {
    Set<ExtensionRegistry.ExtensionInfo> extensions1 = new HashSet<>();
    extensions1.add(extensionInfo1);

    Set<ExtensionRegistry.ExtensionInfo> extensions2 = new HashSet<>();
    extensions2.add(extensionInfo2);

    when(mock1.getAllMutableExtensionsByExtendedType("foo")).thenReturn(extensions1);
    when(mock2.getAllMutableExtensionsByExtendedType("foo")).thenReturn(extensions2);

    assertUsesCurrentDelegate(
        r -> r.getAllMutableExtensionsByExtendedType("foo"), extensions1, extensions2);
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
}
