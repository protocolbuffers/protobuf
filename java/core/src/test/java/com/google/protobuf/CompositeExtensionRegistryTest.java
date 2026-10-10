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
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import proto2_unittest.NonNestedExtensionLite;
import proto2_unittest.UnittestProto;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Test for {@link CompositeExtensionRegistry}. */
@RunWith(JUnit4.class)
public class CompositeExtensionRegistryTest {
  private ExtensionRegistry registry1;
  private ExtensionRegistry registry2;
  private ExtensionRegistry.ExtensionInfo extensionInfo;
  private Descriptors.Descriptor descriptor;
  private int fieldNumber;

  @Before
  public void setUp() throws Exception {
    registry1 = mock(ExtensionRegistry.class);
    registry2 = mock(ExtensionRegistry.class);
    extensionInfo = ExtensionRegistry.newExtensionInfo(UnittestProto.optionalInt32Extension);
    descriptor = UnittestProto.optionalInt32Extension.getDescriptor().getContainingType();
    fieldNumber = UnittestProto.optionalInt32Extension.getNumber();
  }

  @Test
  public void testZeroRegistries() throws Exception {
    ExtensionRegistry registry =
        ExtensionRegistry.combine(Collections.<ExtensionRegistry>emptyList());
    assertThat(registry.findExtensionByName("foo")).isNull();
    assertThat(registry.findImmutableExtensionByName("foo")).isNull();
    assertThat(registry.findMutableExtensionByName("foo")).isNull();
    assertThat(registry.findExtensionByNumber(descriptor, fieldNumber)).isNull();
    assertThat(registry.findImmutableExtensionByNumber(descriptor, fieldNumber)).isNull();
    assertThat(registry.findMutableExtensionByNumber(descriptor, fieldNumber)).isNull();
  }

  @Test
  public void testOneRegistryFound() throws Exception {
    ExtensionRegistry registry = ExtensionRegistry.combine(Collections.singleton(registry1));
    when(registry1.findImmutableExtensionByName("foo")).thenReturn(extensionInfo);

    assertThat(extensionInfo).isEqualTo(registry.findImmutableExtensionByName("foo"));
  }

  @Test
  public void testOneRegistryNotFound() throws Exception {
    ExtensionRegistry registry = ExtensionRegistry.combine(Collections.singleton(registry1));
    when(registry1.findImmutableExtensionByName("foo")).thenReturn(null);

    assertThat(registry.findImmutableExtensionByName("foo")).isNull();
  }

  @Test
  public void testFallthrough() throws Exception {
    List<ExtensionRegistry> extensions = new ArrayList<ExtensionRegistry>();
    extensions.add(registry1);
    extensions.add(registry2);

    ExtensionRegistry registry = ExtensionRegistry.combine(extensions);
    when(registry1.findImmutableExtensionByName("foo")).thenReturn(null);
    when(registry2.findImmutableExtensionByName("foo")).thenReturn(extensionInfo);

    assertThat(extensionInfo).isEqualTo(registry.findImmutableExtensionByName("foo"));
    verify(registry1).findImmutableExtensionByName("foo");
  }

  @Test
  public void testShortCircuit() throws Exception {
    List<ExtensionRegistry> extensions = new ArrayList<ExtensionRegistry>();
    extensions.add(registry1);
    extensions.add(registry2);

    ExtensionRegistry registry = ExtensionRegistry.combine(extensions);
    when(registry1.findImmutableExtensionByName("foo")).thenReturn(extensionInfo);

    assertThat(extensionInfo).isEqualTo(registry.findImmutableExtensionByName("foo"));
    verify(registry2, never()).findImmutableExtensionByName("foo");
  }

  @Test
  public void testFindMutableExtensionByNameFallthroughAndShortCircuit() throws Exception {
    List<ExtensionRegistry> extensions = new ArrayList<ExtensionRegistry>();
    extensions.add(registry1);
    extensions.add(registry2);

    ExtensionRegistry registry = ExtensionRegistry.combine(extensions);
    when(registry1.findMutableExtensionByName("foo")).thenReturn(null);
    when(registry2.findMutableExtensionByName("foo")).thenReturn(extensionInfo);
    when(registry1.findMutableExtensionByName("bar")).thenReturn(extensionInfo);

    assertThat(registry.findMutableExtensionByName("foo")).isEqualTo(extensionInfo);
    verify(registry1).findMutableExtensionByName("foo");
    verify(registry2).findMutableExtensionByName("foo");

    assertThat(registry.findMutableExtensionByName("bar")).isEqualTo(extensionInfo);
    verify(registry1).findMutableExtensionByName("bar");
    verify(registry2, never()).findMutableExtensionByName("bar");
  }

  @Test
  public void testFindImmutableExtensionByNumberFallthroughAndShortCircuit() throws Exception {
    List<ExtensionRegistry> extensions = new ArrayList<ExtensionRegistry>();
    extensions.add(registry1);
    extensions.add(registry2);

    ExtensionRegistry registry = ExtensionRegistry.combine(extensions);
    when(registry1.findImmutableExtensionByNumber(descriptor, 1)).thenReturn(null);
    when(registry2.findImmutableExtensionByNumber(descriptor, 1)).thenReturn(extensionInfo);
    when(registry1.findImmutableExtensionByNumber(descriptor, 2)).thenReturn(extensionInfo);

    assertThat(registry.findImmutableExtensionByNumber(descriptor, 1)).isEqualTo(extensionInfo);
    verify(registry1).findImmutableExtensionByNumber(descriptor, 1);
    verify(registry2).findImmutableExtensionByNumber(descriptor, 1);

    assertThat(registry.findImmutableExtensionByNumber(descriptor, 2)).isEqualTo(extensionInfo);
    verify(registry1).findImmutableExtensionByNumber(descriptor, 2);
    verify(registry2, never()).findImmutableExtensionByNumber(descriptor, 2);
  }

  @Test
  public void testFindMutableExtensionByNumberFallthroughAndShortCircuit() throws Exception {
    List<ExtensionRegistry> extensions = new ArrayList<ExtensionRegistry>();
    extensions.add(registry1);
    extensions.add(registry2);

    ExtensionRegistry registry = ExtensionRegistry.combine(extensions);
    when(registry1.findMutableExtensionByNumber(descriptor, 1)).thenReturn(null);
    when(registry2.findMutableExtensionByNumber(descriptor, 1)).thenReturn(extensionInfo);
    when(registry1.findMutableExtensionByNumber(descriptor, 2)).thenReturn(extensionInfo);

    assertThat(registry.findMutableExtensionByNumber(descriptor, 1)).isEqualTo(extensionInfo);
    verify(registry1).findMutableExtensionByNumber(descriptor, 1);
    verify(registry2).findMutableExtensionByNumber(descriptor, 1);

    assertThat(registry.findMutableExtensionByNumber(descriptor, 2)).isEqualTo(extensionInfo);
    verify(registry1).findMutableExtensionByNumber(descriptor, 2);
    verify(registry2, never()).findMutableExtensionByNumber(descriptor, 2);
  }

  @Test
  public void testGetAllMutableExtensionsByExtendedType() throws Exception {
    List<ExtensionRegistry> extensions = new ArrayList<ExtensionRegistry>();
    extensions.add(registry1);
    extensions.add(registry2);
    ExtensionRegistry registry = ExtensionRegistry.combine(extensions);

    HashSet<ExtensionRegistry.ExtensionInfo> extensions1 =
        new HashSet<ExtensionRegistry.ExtensionInfo>();
    extensions1.add(extensionInfo);

    ExtensionRegistry.ExtensionInfo extensionInfo2 =
        ExtensionRegistry.newExtensionInfo(UnittestProto.optionalInt64Extension);
    HashSet<ExtensionRegistry.ExtensionInfo> extensions2 =
        new HashSet<ExtensionRegistry.ExtensionInfo>();
    extensions2.add(extensionInfo2);
    when(registry1.getAllMutableExtensionsByExtendedType("full_name")).thenReturn(extensions1);
    when(registry2.getAllMutableExtensionsByExtendedType("full_name")).thenReturn(extensions2);

    HashSet<ExtensionRegistry.ExtensionInfo> expectedSet =
        new HashSet<ExtensionRegistry.ExtensionInfo>();
    expectedSet.add(extensionInfo);
    expectedSet.add(extensionInfo2);
    assertThat(expectedSet).isEqualTo(registry.getAllMutableExtensionsByExtendedType("full_name"));
  }

  @Test
  public void testGetAllImmutableExtensionsByExtendedType() throws Exception {
    List<ExtensionRegistry> extensions = new ArrayList<ExtensionRegistry>();
    extensions.add(registry1);
    extensions.add(registry2);
    ExtensionRegistry registry = ExtensionRegistry.combine(extensions);

    HashSet<ExtensionRegistry.ExtensionInfo> extensions1 =
        new HashSet<ExtensionRegistry.ExtensionInfo>();
    extensions1.add(extensionInfo);

    ExtensionRegistry.ExtensionInfo extensionInfo2 =
        ExtensionRegistry.newExtensionInfo(UnittestProto.optionalInt64Extension);
    HashSet<ExtensionRegistry.ExtensionInfo> extensions2 =
        new HashSet<ExtensionRegistry.ExtensionInfo>();
    extensions2.add(extensionInfo2);
    when(registry1.getAllImmutableExtensionsByExtendedType("full_name")).thenReturn(extensions1);
    when(registry2.getAllImmutableExtensionsByExtendedType("full_name")).thenReturn(extensions2);

    HashSet<ExtensionRegistry.ExtensionInfo> expectedSet =
        new HashSet<ExtensionRegistry.ExtensionInfo>();
    expectedSet.add(extensionInfo);
    expectedSet.add(extensionInfo2);
    assertThat(expectedSet)
        .isEqualTo(registry.getAllImmutableExtensionsByExtendedType("full_name"));
  }

  @Test
  public void testUnmodifiableIsSelf() {
    CompositeExtensionRegistry composite =
        new CompositeExtensionRegistry(Collections.<ExtensionRegistry>emptyList());
    assertThat(composite.getUnmodifiable()).isSameInstanceAs(composite);
  }

  @Test
  public void testAddExtensionNotSupported() {
    CompositeExtensionRegistry composite =
        new CompositeExtensionRegistry(Collections.<ExtensionRegistry>emptyList());
    assertThrows(
        UnsupportedOperationException.class,
        () -> composite.add(UnittestProto.optionalInt32Extension));
  }

  @Test
  public void testAddDescriptorNotSupported() {
    CompositeExtensionRegistry composite =
        new CompositeExtensionRegistry(Collections.<ExtensionRegistry>emptyList());
    assertThrows(
        UnsupportedOperationException.class,
        () -> composite.add(UnittestProto.optionalInt32Extension.getDescriptor()));
  }

  @Test
  public void testAddDescriptorAndMessageNotSupported() {
    CompositeExtensionRegistry composite =
        new CompositeExtensionRegistry(Collections.<ExtensionRegistry>emptyList());
    assertThrows(
        UnsupportedOperationException.class,
        () ->
            composite.add(
                UnittestProto.optionalForeignMessageExtension.getDescriptor(),
                UnittestProto.ForeignMessage.getDefaultInstance()));
  }

  @Test
  public void testFindLiteExtensionByNumberNotSupported() {
    CompositeExtensionRegistry composite =
        new CompositeExtensionRegistry(Collections.<ExtensionRegistry>emptyList());
    assertThrows(
        UnsupportedOperationException.class,
        () ->
            composite.findLiteExtensionByNumber(
                NonNestedExtensionLite.MessageLiteToBeExtended.getDefaultInstance(), 1));
  }
}
