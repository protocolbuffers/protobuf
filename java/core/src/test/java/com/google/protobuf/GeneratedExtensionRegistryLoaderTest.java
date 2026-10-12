// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import com.google.protobuf.UnittestLite.TestAllExtensionsLite;
import java.lang.reflect.Field;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/**
 * Unit tests for {@link GeneratedExtensionRegistryLoader}, {@link
 * ExtensionRegistryLite#getGeneratedRegistry()}, and {@link
 * ExtensionRegistryFactory#loadGeneratedRegistry()}.
 */
@RunWith(JUnit4.class)
public final class GeneratedExtensionRegistryLoaderTest {

  private static final GeneratedMessageLite.GeneratedExtension<TestAllExtensionsLite, Integer>
      TEST_LITE_EXTENSION =
          GeneratedMessageLite.newSingularGeneratedExtension(
              TestAllExtensionsLite.getDefaultInstance(),
              0,
              null,
              null,
              1001,
              WireFormat.FieldType.INT32,
              Integer.class);

  public static final class TestLiteRegistryLoader
      extends GeneratedExtensionRegistryLoader<ExtensionRegistryLite> {
    private static final ExtensionRegistryLite INSTANCE = createRegistry();

    private static ExtensionRegistryLite createRegistry() {
      ExtensionRegistryLite registry = ExtensionRegistryLite.newInstance();
      registry.add(TEST_LITE_EXTENSION);
      registry.add(UnittestLite.optionalInt32ExtensionLite);
      return registry.getUnmodifiable();
    }

    public TestLiteRegistryLoader() {}

    @Override
    protected ExtensionRegistryLite getInstance() {
      return INSTANCE;
    }
  }

  public static final class PrivateConstructorRegistryLoader
      extends GeneratedExtensionRegistryLoader<ExtensionRegistryLite> {
    private PrivateConstructorRegistryLoader() {}

    @Override
    protected ExtensionRegistryLite getInstance() {
      return ExtensionRegistryLite.getEmptyRegistry();
    }
  }

  private static String getLiteClassName() throws Exception {
    Field field = GeneratedExtensionRegistryLoader.class.getDeclaredField("LITE_CLASS_NAME");
    field.setAccessible(true);
    return (String) field.get(null);
  }

  private static void setLiteClassName(String className) throws Exception {
    Field field = GeneratedExtensionRegistryLoader.class.getDeclaredField("LITE_CLASS_NAME");
    field.setAccessible(true);
    field.set(null, className);
  }

  @Test
  public void getGeneratedRegistry_returnsSingletonInstance() {
    ExtensionRegistryLite registry = ExtensionRegistryLite.getGeneratedRegistry();
    assertThat(registry).isNotNull();
    assertThat(ExtensionRegistryLite.getGeneratedRegistry()).isSameInstanceAs(registry);
  }

  @Test
  public void extensionRegistryFactory_loadGeneratedRegistry_returnsNonNullRegistry() {
    ExtensionRegistryLite registry = ExtensionRegistryFactory.loadGeneratedRegistry();
    assertThat(registry).isNotNull();
    if (!Android.assumeLiteRuntime && ExtensionRegistryFactory.EXTENSION_REGISTRY_CLASS != null) {
      assertThat(ExtensionRegistryFactory.isFullRegistry(registry)).isTrue();
    }
  }

  @Test
  public void load_customLiteLoader_loadsRegistryWithExtension() throws Exception {
    String previous = getLiteClassName();
    setLiteClassName(TestLiteRegistryLoader.class.getName());
    try {
      ExtensionRegistryLite loaded = ExtensionRegistryLite.loadGeneratedRegistry();
      assertThat(loaded).isSameInstanceAs(TestLiteRegistryLoader.INSTANCE);
      assertThat(
              loaded.findLiteExtensionByNumber(
                  TestAllExtensionsLite.getDefaultInstance(), TEST_LITE_EXTENSION.getNumber()))
          .isSameInstanceAs(TEST_LITE_EXTENSION);

      TestAllExtensionsLite message =
          TestAllExtensionsLite.newBuilder()
              .setExtension(UnittestLite.optionalInt32ExtensionLite, 123)
              .build();
      TestAllExtensionsLite parsed =
          TestAllExtensionsLite.parseFrom(message.toByteString(), loaded);
      assertThat(parsed.hasExtension(UnittestLite.optionalInt32ExtensionLite)).isTrue();
      assertThat((int) parsed.getExtension(UnittestLite.optionalInt32ExtensionLite)).isEqualTo(123);
    } finally {
      setLiteClassName(previous);
    }
  }

  @Test
  public void load_inaccessibleLoaderConstructor_throwsIllegalStateException() throws Exception {
    String previous = getLiteClassName();
    setLiteClassName(PrivateConstructorRegistryLoader.class.getName());
    try {
      IllegalStateException exception =
          assertThrows(IllegalStateException.class, ExtensionRegistryLite::loadGeneratedRegistry);
      assertThat(exception).hasCauseThat().isInstanceOf(NoSuchMethodException.class);
    } finally {
      setLiteClassName(previous);
    }
  }

  @Test
  public void load_nonExistentLiteClassName_fallsBackWithoutThrowingClassNotFoundException()
      throws Exception {
    String previous = getLiteClassName();
    setLiteClassName("com.google.protobuf.NonExistentLiteRegistryLoader");
    try {
      assertThat(ExtensionRegistryFactory.loadGeneratedRegistry()).isNotNull();
    } finally {
      setLiteClassName(previous);
    }
  }
}
