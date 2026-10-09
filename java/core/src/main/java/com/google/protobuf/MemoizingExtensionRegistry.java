// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;

/**
 * Delegates calls to a wrapped {@link ExtensionRegistry} while memoizing {@link ExtensionInfo}
 * retrieval operations.
 */
final class MemoizingExtensionRegistry extends ExtensionRegistry {
  private final ExtensionRegistry delegate;
  private final Map<String, ExtensionInfoOrNull> immutableExtensionsByName =
      new ConcurrentHashMap<String, ExtensionInfoOrNull>();
  private final Map<String, ExtensionInfoOrNull> mutableExtensionsByName =
      new ConcurrentHashMap<String, ExtensionInfoOrNull>();
  private final Map<DescriptorIntPair, ExtensionInfoOrNull> immutableExtensionsByNumber =
      new ConcurrentHashMap<DescriptorIntPair, ExtensionInfoOrNull>();
  private final Map<DescriptorIntPair, ExtensionInfoOrNull> mutableExtensionsByNumber =
      new ConcurrentHashMap<DescriptorIntPair, ExtensionInfoOrNull>();

  MemoizingExtensionRegistry(ExtensionRegistry delegate) {
    super(true);
    this.delegate = delegate;
  }

  @Override
  public ExtensionRegistry getUnmodifiable() {
    return this;
  }

  @Override
  public ExtensionInfo findImmutableExtensionByName(String fullName) {
    ExtensionInfoOrNull cachedResult = immutableExtensionsByName.get(fullName);
    if (cachedResult != null) {
      return cachedResult.info;
    }
    ExtensionInfo result = delegate.findImmutableExtensionByName(fullName);
    immutableExtensionsByName.put(fullName, new ExtensionInfoOrNull(result));
    return result;
  }

  @Override
  public ExtensionInfo findMutableExtensionByName(String fullName) {
    ExtensionInfoOrNull cachedResult = mutableExtensionsByName.get(fullName);
    if (cachedResult != null) {
      return cachedResult.info;
    }
    ExtensionInfo result = delegate.findMutableExtensionByName(fullName);
    mutableExtensionsByName.put(fullName, new ExtensionInfoOrNull(result));
    return result;
  }

  @Override
  public ExtensionInfo findImmutableExtensionByNumber(
      Descriptors.Descriptor containingType, int fieldNumber) {
    DescriptorIntPair key = new DescriptorIntPair(containingType, fieldNumber);
    ExtensionInfoOrNull cachedResult = immutableExtensionsByNumber.get(key);
    if (cachedResult != null) {
      return cachedResult.info;
    }
    ExtensionInfo result = delegate.findImmutableExtensionByNumber(containingType, fieldNumber);
    immutableExtensionsByNumber.put(key, new ExtensionInfoOrNull(result));
    return result;
  }

  @Override
  public ExtensionInfo findMutableExtensionByNumber(
      Descriptors.Descriptor containingType, int fieldNumber) {
    DescriptorIntPair key = new DescriptorIntPair(containingType, fieldNumber);
    ExtensionInfoOrNull cachedResult = mutableExtensionsByNumber.get(key);
    if (cachedResult != null) {
      return cachedResult.info;
    }
    ExtensionInfo result = delegate.findMutableExtensionByNumber(containingType, fieldNumber);
    mutableExtensionsByNumber.put(key, new ExtensionInfoOrNull(result));
    return result;
  }

  // Note: getAll*ExtensionsByExtendedType are not memoized, because the implementation in
  // ExtensionRegistry is linear in the number of extensions anyways.

  @Override
  public Set<ExtensionInfo> getAllImmutableExtensionsByExtendedType(final String fullName) {
    return delegate.getAllImmutableExtensionsByExtendedType(fullName);
  }

  @Override
  public Set<ExtensionInfo> getAllMutableExtensionsByExtendedType(final String fullName) {
    return delegate.getAllMutableExtensionsByExtendedType(fullName);
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
        "MemoizingExtensionRegistry does not work for Lite extensions.");
  }

  private static UnsupportedOperationException newUnsupported() {
    return new UnsupportedOperationException("MemoizingExtensionRegistry cannot be modified");
  }

  /**
   * Wraps nullable {@link ExtensionInfo}, so that it can be used as a value in {@code
   * ConcurrentHashMap}.
   */
  private static final class ExtensionInfoOrNull {
    private final ExtensionInfo info;

    ExtensionInfoOrNull(ExtensionInfo info) {
      this.info = info;
    }
  }

  /** A (Descriptor, int) pair, used as a map key. */
  private static final class DescriptorIntPair {
    private final Descriptors.Descriptor descriptor;
    private final int number;

    DescriptorIntPair(final Descriptors.Descriptor descriptor, final int number) {
      this.descriptor = descriptor;
      this.number = number;
    }

    @Override
    public int hashCode() {
      return descriptor.hashCode() * ((1 << 16) - 1) + number;
    }

    @Override
    public boolean equals(final Object obj) {
      if (!(obj instanceof DescriptorIntPair)) {
        return false;
      }
      final DescriptorIntPair other = (DescriptorIntPair) obj;
      return descriptor == other.descriptor && number == other.number;
    }
  }
}
