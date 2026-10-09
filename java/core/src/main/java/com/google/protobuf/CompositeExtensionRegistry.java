// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashSet;
import java.util.Set;

/**
 * Wraps zero or more {@link ExtensionRegistry} instances and delegates extension lookup calls to
 * each of them in order. This class is useful for wrapping a {@link GeneratedExtensionRegistry} or
 * other immutable ExtensionRegistry to add more extensions.
 *
 * <p>This ExtensionRegistry is immutable and all {@code add} methods will throw an {@link
 * UnsupportedOperationException}. {@link #findLiteExtensionByNumber} is also unsupported.
 *
 * @author schwardo@google.com (Don Schwarz)
 */
class CompositeExtensionRegistry extends ExtensionRegistry {
  @Override
  public ExtensionRegistry getUnmodifiable() {
    return this;
  }

  @Override
  public ExtensionInfo findImmutableExtensionByName(String fullName) {
    for (ExtensionRegistry registry : registries) {
      ExtensionInfo info = registry.findImmutableExtensionByName(fullName);
      if (info != null) {
        return info;
      }
    }
    return null;
  }

  @Override
  public ExtensionInfo findMutableExtensionByName(String fullName) {
    for (ExtensionRegistry registry : registries) {
      ExtensionInfo info = registry.findMutableExtensionByName(fullName);
      if (info != null) {
        return info;
      }
    }
    return null;
  }

  @Override
  public ExtensionInfo findImmutableExtensionByNumber(
      Descriptors.Descriptor containingType, int fieldNumber) {
    for (ExtensionRegistry registry : registries) {
      ExtensionInfo info = registry.findImmutableExtensionByNumber(containingType, fieldNumber);
      if (info != null) {
        return info;
      }
    }
    return null;
  }

  @Override
  public ExtensionInfo findMutableExtensionByNumber(
      Descriptors.Descriptor containingType, int fieldNumber) {
    for (ExtensionRegistry registry : registries) {
      ExtensionInfo info = registry.findMutableExtensionByNumber(containingType, fieldNumber);
      if (info != null) {
        return info;
      }
    }
    return null;
  }

  @Override
  public Set<ExtensionInfo> getAllMutableExtensionsByExtendedType(final String fullName) {
    Set<ExtensionInfo> result = new HashSet<ExtensionInfo>();
    for (ExtensionRegistry registry : registries) {
      result.addAll(registry.getAllMutableExtensionsByExtendedType(fullName));
    }
    return result;
  }

  @Override
  public Set<ExtensionInfo> getAllImmutableExtensionsByExtendedType(final String fullName) {
    Set<ExtensionInfo> result = new HashSet<ExtensionInfo>();
    for (ExtensionRegistry registry : registries) {
      result.addAll(registry.getAllImmutableExtensionsByExtendedType(fullName));
    }
    return result;
  }

  @Override
  public void add(Extension<?, ?> extension) {
    throw newUnsupported();
  }

  @Override
  public void add(Descriptors.FieldDescriptor type) {
    throw newUnsupported();
  }

  @Override
  public void add(Descriptors.FieldDescriptor type, Message defaultInstance) {
    throw newUnsupported();
  }

  @Override
  public <ContainingType extends MessageLite>
      GeneratedMessageLite.GeneratedExtension<ContainingType, ?> findLiteExtensionByNumber(
          ContainingType containingTypeDefaultInstance, int fieldNumber) {
    throw new UnsupportedOperationException(
        "GeneratedExtensionRegistry does not work for Lite extensions.");
  }

  private static UnsupportedOperationException newUnsupported() {
    return new UnsupportedOperationException("GeneratedExtensionRegistry cannot be modified");
  }

  private final Collection<ExtensionRegistry> registries;

  CompositeExtensionRegistry(Collection<ExtensionRegistry> registries) {
    super(true);
    this.registries = new ArrayList<ExtensionRegistry>(registries);
  }
}
