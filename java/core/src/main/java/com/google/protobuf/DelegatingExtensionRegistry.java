// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import java.util.Set;
import java.util.function.Supplier;

/**
 * Allows creating {@link ExtensionRegistry} instances that delegate to a runtime-provided delegate.
 * This is useful for allowing singletons to use an ExtensionRegistry that might change over time.
 */
public final class DelegatingExtensionRegistry {
  private DelegatingExtensionRegistry() {}

  /**
   * Returns a new {@link ExtensionRegistry} that delegates to the given delegate, which is accessed
   * on-demand when needed. This is useful for allowing singletons to use an ExtensionRegistry that
   * might change over time.
   */
  public static ExtensionRegistry createDelegating(Supplier<ExtensionRegistry> delegate) {
    return new DelegatingExtensionRegistryImpl(delegate);
  }

  /**
   * Delegates calls to a wrapped {@link ExtensionRegistry}, which is retrieved on-demand. This is
   * useful for allowing singletons to use an ExtensionRegistry that might change over time.
   */
  private static final class DelegatingExtensionRegistryImpl extends ExtensionRegistry {
    private final Supplier<ExtensionRegistry> delegate;

    DelegatingExtensionRegistryImpl(Supplier<ExtensionRegistry> delegate) {
      super(true);
      this.delegate = delegate;
    }

    @Override
    public ExtensionRegistry getUnmodifiable() {
      return this;
    }

    @Override
    public ExtensionInfo findImmutableExtensionByName(String fullName) {
      return delegate.get().findImmutableExtensionByName(fullName);
    }

    @Override
    public ExtensionInfo findMutableExtensionByName(String fullName) {
      return delegate.get().findMutableExtensionByName(fullName);
    }

    @Override
    public ExtensionInfo findImmutableExtensionByNumber(
        Descriptors.Descriptor containingType, int fieldNumber) {
      return delegate.get().findImmutableExtensionByNumber(containingType, fieldNumber);
    }

    @Override
    public ExtensionInfo findMutableExtensionByNumber(
        Descriptors.Descriptor containingType, int fieldNumber) {
      return delegate.get().findMutableExtensionByNumber(containingType, fieldNumber);
    }

    @Override
    public Set<ExtensionInfo> getAllImmutableExtensionsByExtendedType(final String fullName) {
      return delegate.get().getAllImmutableExtensionsByExtendedType(fullName);
    }

    @Override
    public Set<ExtensionInfo> getAllMutableExtensionsByExtendedType(final String fullName) {
      return delegate.get().getAllMutableExtensionsByExtendedType(fullName);
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
    public <T extends MessageLite>
        GeneratedMessageLite.GeneratedExtension<T, ?> findLiteExtensionByNumber(
            T containingTypeDefaultInstance, int fieldNumber) {
      return delegate.get().findLiteExtensionByNumber(containingTypeDefaultInstance, fieldNumber);
    }

    private static UnsupportedOperationException newUnsupported() {
      return new UnsupportedOperationException("DelegatingExtensionRegistry cannot be modified");
    }
  }
}
