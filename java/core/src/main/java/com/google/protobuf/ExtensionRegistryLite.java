// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import javax.annotation.Nullable;

/**
 * Equivalent to {@link ExtensionRegistry} but supports only "lite" types.
 *
 * <p>If all of your types are lite types, then you only need to use {@code ExtensionRegistryLite}.
 * Similarly, if all your types are regular types, then you only need {@link ExtensionRegistry}.
 * Typically it does not make sense to mix the two, since if you have any regular types in your
 * program, you then require the full runtime and lose all the benefits of the lite runtime, so you
 * might as well make all your types be regular types. However, in some cases (e.g. when depending
 * on multiple third-party libraries where one uses lite types and one uses regular), you may find
 * yourself wanting to mix the two. In this case things get more complicated.
 *
 * <p>There are three factors to consider: Whether the type being extended is lite, whether the
 * embedded type (in the case of a message-typed extension) is lite, and whether the extension
 * itself is lite. Since all three are declared in different files, they could all be different.
 * Here are all the combinations and which type of registry to use:
 *
 * <pre>
 *   Extended type     Inner type    Extension         Use registry
 *   =======================================================================
 *   lite              lite          lite              ExtensionRegistryLite
 *   lite              regular       lite              ExtensionRegistry
 *   regular           regular       regular           ExtensionRegistry
 *   all other combinations                            not supported
 * </pre>
 *
 * <p>Note that just as regular types are not allowed to contain lite-type fields, they are also not
 * allowed to contain lite-type extensions. This is because regular types must be fully accessible
 * via reflection, which in turn means that all the inner messages must also support reflection. On
 * the other hand, since regular types implement the entire lite interface, there is no problem with
 * embedding regular types inside lite types.
 *
 * @author kenton@google.com Kenton Varda
 */
public class ExtensionRegistryLite {

  // Set true to enable lazy parsing feature for MessageSet.
  //
  // TODO: Now we use a global flag to control whether enable lazy
  // parsing feature for MessageSet, which may be too crude for some
  // applications. Need to support this feature on smaller granularity.
  private static volatile boolean eagerlyParseMessageSets = false;

  // Visible for testing.
  static final String EXTENSION_CLASS_NAME = "com.google.protobuf.Extension";

  private static class ExtensionClassHolder {
    static final Class<?> INSTANCE = resolveExtensionClass();

    static Class<?> resolveExtensionClass() {
      try {
        return Class.forName(EXTENSION_CLASS_NAME);
      } catch (ClassNotFoundException e) {
        // See comment in ExtensionRegistryFactory on the potential expense of this.
        return null;
      }
    }
  }

  public static boolean isEagerlyParseMessageSets() {
    return eagerlyParseMessageSets;
  }

  public static void setEagerlyParseMessageSets(boolean isEagerlyParse) {
    eagerlyParseMessageSets = isEagerlyParse;
  }

  /**
   * Construct a new, empty instance.
   *
   * <p>This may be an {@code ExtensionRegistry} if the full (non-Lite) proto libraries are
   * available.
   */
  public static ExtensionRegistryLite newInstance() {
    return Android.assumeLiteRuntime
        ? new ExtensionRegistryLite()
        : ExtensionRegistryFactory.create();
  }

  private static volatile ExtensionRegistryLite emptyRegistry;

  private static volatile ExtensionRegistryLite generatedRegistry;

  /**
   * Get the unmodifiable singleton empty instance of either ExtensionRegistryLite or {@code
   * ExtensionRegistry} (if the full (non-Lite) proto libraries are available).
   */
  public static ExtensionRegistryLite getEmptyRegistry() {
    if (Android.assumeLiteRuntime) {
      return EMPTY_REGISTRY_LITE;
    }
    ExtensionRegistryLite result = emptyRegistry;
    if (result == null) {
      synchronized (ExtensionRegistryLite.class) {
        result = emptyRegistry;
        if (result == null) {
          emptyRegistry = result = ExtensionRegistryFactory.createEmpty();
        }
      }
    }
    return result;
  }

  /**
   * Get the unmodifiable singleton instance for all the generated protobuf messages loadable in the
   * classpath.
   */
  public static ExtensionRegistryLite getGeneratedRegistry() {
    ExtensionRegistryLite result = generatedRegistry;
    if (result == null) {
      synchronized (ExtensionRegistryLite.class) {
        result = generatedRegistry;
        if (result == null) {
          generatedRegistry =
              result =
                  Android.assumeLiteRuntime
                      ? loadGeneratedRegistry()
                      : ExtensionRegistryFactory.loadGeneratedRegistry();
        }
      }
    }
    return result;
  }

  static ExtensionRegistryLite loadGeneratedRegistry() {
    return GeneratedExtensionRegistryLoader.load(ExtensionRegistryLite.class);
  }

  /** Returns an unmodifiable view of the registry. */
  public ExtensionRegistryLite getUnmodifiable() {
    return new ExtensionRegistryLite(this);
  }

  /**
   * Find an extension by containing type and field number.
   *
   * @return Information about the extension if found, or {@code null} otherwise.
   */
  @SuppressWarnings("unchecked")
  public <ContainingTypeT extends MessageLite>
      GeneratedMessageLite.GeneratedExtension<ContainingTypeT, ?> findLiteExtensionByNumber(
          final ContainingTypeT containingTypeDefaultInstance, final int fieldNumber) {
    return (GeneratedMessageLite.GeneratedExtension<ContainingTypeT, ?>)
        extensionsByNumber.get(containingTypeDefaultInstance, fieldNumber);
  }

  /** Add an extension from a lite generated file to the registry. */
  public final void add(final GeneratedMessageLite.GeneratedExtension<?, ?> extension) {
    extensionsByNumber.put(
        extension.getContainingTypeDefaultInstance(), extension.getNumber(), extension);
  }

  /**
   * Add an extension from a lite generated file to the registry only if it is a non-lite extension
   * i.e. {@link GeneratedMessageLite.GeneratedExtension}.
   */
  public final void add(ExtensionLite<?, ?> extension) {
    if (extension instanceof GeneratedMessageLite.GeneratedExtension) {
      add((GeneratedMessageLite.GeneratedExtension<?, ?>) extension);
    }
    if (!Android.assumeLiteRuntime && ExtensionRegistryFactory.isFullRegistry(this)) {
      try {
        this.getClass().getMethod("add", ExtensionClassHolder.INSTANCE).invoke(this, extension);
      } catch (Exception e) {
        throw new IllegalArgumentException(
            String.format("Could not invoke ExtensionRegistry#add for %s", extension), e);
      }
    }
  }

  // =================================================================
  // Private stuff.

  private static final ExtensionMap EMPTY_MAP = new ExtensionMap(0, true);

  static final ExtensionRegistryLite EMPTY_REGISTRY_LITE = new ExtensionRegistryLite(true);

  private final ExtensionMap extensionsByNumber;

  ExtensionRegistryLite() {
    this.extensionsByNumber = new ExtensionMap();
  }

  public ExtensionRegistryLite(int initialCapacity) {
    this.extensionsByNumber = new ExtensionMap(initialCapacity);
  }

  ExtensionRegistryLite(ExtensionRegistryLite other) {
    if (other == EMPTY_REGISTRY_LITE) {
      this.extensionsByNumber = EMPTY_MAP;
    } else {
      this.extensionsByNumber = other.extensionsByNumber.asUnmodifiable();
    }
  }

  ExtensionRegistryLite(boolean empty) {
    this.extensionsByNumber = EMPTY_MAP;
  }

  private static final class ExtensionMap {
    private Object[] objects;
    private int[] numbers;
    private GeneratedMessageLite.GeneratedExtension<?, ?>[] values;
    private int capacity;
    private int size;
    private final boolean unmodifiable;

    ExtensionMap() {
      this(16);
    }

    ExtensionMap(int capacity) {
      this(capacity, false);
    }

    private ExtensionMap(int capacity, boolean unmodifiable) {
      this.capacity = capacity > 0 ? nextPowerOfTwo(capacity) : 0;
      if (this.capacity > 0) {
        this.objects = new Object[this.capacity];
        this.numbers = new int[this.capacity];
        this.values = new GeneratedMessageLite.GeneratedExtension<?, ?>[this.capacity];
      } else {
        this.objects = null;
        this.numbers = null;
        this.values = null;
      }
      this.size = 0;
      this.unmodifiable = unmodifiable;
    }

    private ExtensionMap(ExtensionMap other, boolean unmodifiable) {
      this.capacity = other.capacity;
      this.size = other.size;
      this.objects = other.objects != null ? other.objects.clone() : null;
      this.numbers = other.numbers != null ? other.numbers.clone() : null;
      this.values = other.values != null ? other.values.clone() : null;
      this.unmodifiable = unmodifiable;
    }

    private static int nextPowerOfTwo(int n) {
      int v = n - 1;
      v |= v >> 1;
      v |= v >> 2;
      v |= v >> 4;
      v |= v >> 8;
      v |= v >> 16;
      return v + 1;
    }

    @Nullable
    GeneratedMessageLite.GeneratedExtension<?, ?> get(Object object, int number) {
      if (capacity == 0) {
        return null;
      }
      int hash = (System.identityHashCode(object) * 31 + number) & (capacity - 1);
      for (int i = 0; i < capacity; i++) {
        int index = (hash + i) & (capacity - 1);
        if (objects[index] == null) {
          return null;
        }
        if (objects[index] == object && numbers[index] == number) {
          return values[index];
        }
      }
      return null;
    }

    void put(Object object, int number, GeneratedMessageLite.GeneratedExtension<?, ?> value) {
      if (unmodifiable) {
        throw new UnsupportedOperationException();
      }
      if (capacity == 0) {
        // This case should not happen if used correctly via constructors, but for safety:
        throw new IllegalStateException("ExtensionMap not initialized");
      }
      if (size >= capacity * 0.75) {
        resize();
      }
      putInternal(object, number, value);
    }

    private void putInternal(
        Object object, int number, GeneratedMessageLite.GeneratedExtension<?, ?> value) {
      int hash = (System.identityHashCode(object) * 31 + number) & (capacity - 1);
      for (int i = 0; i < capacity; i++) {
        int index = (hash + i) & (capacity - 1);
        if (objects[index] == null) {
          objects[index] = object;
          numbers[index] = number;
          values[index] = value;
          size++;
          return;
        }
        if (objects[index] == object && numbers[index] == number) {
          values[index] = value;
          return;
        }
      }
    }

    private void resize() {
      int newCapacity = capacity * 2;
      ExtensionMap newMap = new ExtensionMap(newCapacity);
      for (int i = 0; i < capacity; i++) {
        if (objects[i] != null) {
          newMap.putInternal(objects[i], numbers[i], values[i]);
        }
      }
      this.capacity = newMap.capacity;
      this.objects = newMap.objects;
      this.numbers = newMap.numbers;
      this.values = newMap.values;
    }

    ExtensionMap asUnmodifiable() {
      return new ExtensionMap(this, true);
    }
  }
}
