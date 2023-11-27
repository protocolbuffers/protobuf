// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import com.google.common.collect.ImmutableSet;
import com.google.common.reflect.ClassPath;
import protobuf_unittest.NonNestedExtension;
import protobuf_unittest.NonNestedExtensionLite;
import java.io.IOException;
import java.lang.reflect.Method;
import java.net.URL;
import java.net.URLClassLoader;
import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;
import org.junit.Ignore;

/**
 * Tests for {@link ExtensionRegistryFactory} and the {@link ExtensionRegistry} instances it
 * creates.
 *
 * <p>This test simulates the runtime behaviour of the ExtensionRegistryFactory by delegating test
 * definitions to two inner classes {@link InnerTest} and {@link InnerLiteTest}, the latter of which
 * is executed using a custom ClassLoader, simulating the ProtoLite environment.
 *
 * <p>The test mechanism employed here is based on the pattern in {@code
 * com.google.common.util.concurrent.AbstractFutureFallbackAtomicHelperTest}
 *
 * <p>This test is temporarily disabled while we figure out how to fix the class loading used for
 * testing lite functionality.
 */
@SuppressWarnings("JUnit4ClassUsedInJUnit3")
@Ignore
public class ExtensionRegistryFactoryTest extends TestCase {

  // A classloader which blacklists some non-Lite classes.
  private static final ClassLoader LITE_CLASS_LOADER = getLiteOnlyClassLoader();

  /** Defines the set of test methods which will be run. */
  static interface RegistryTests {
    void testCreate();

    void testEmpty();

    void testIsFullRegistry();

    void testAdd();

    void testAdd_immutable();
  }

  /** Test implementations for the non-Lite usage of ExtensionRegistryFactory. */
  public static class InnerTest implements RegistryTests {

    @Override
    public void testCreate() {
      ExtensionRegistryLite registry = ExtensionRegistryFactory.create();

      assertThat(registry.getClass()).isEqualTo(ExtensionRegistry.class);
    }

    @Override
    public void testEmpty() {
      ExtensionRegistryLite emptyRegistry = ExtensionRegistryFactory.createEmpty();

      assertThat(emptyRegistry.getClass()).isEqualTo(ExtensionRegistry.class);
      assertThat(emptyRegistry).isEqualTo(ExtensionRegistry.EMPTY_REGISTRY);
    }

    @Override
    public void testIsFullRegistry() {
      ExtensionRegistryLite registry = ExtensionRegistryFactory.create();
      assertThat(ExtensionRegistryFactory.isFullRegistry(registry)).isTrue();
    }

    @Override
    public void testAdd() {
      ExtensionRegistryLite registry1 = ExtensionRegistryLite.newInstance();
      NonNestedExtensionLite.registerAllExtensions(registry1);
      registry1.add(NonNestedExtensionLite.nonNestedExtensionLite);

      ExtensionRegistryLite registry2 = ExtensionRegistryLite.newInstance();
      NonNestedExtension.registerAllExtensions((ExtensionRegistry) registry2);
      registry2.add(NonNestedExtension.nonNestedExtension);

      ExtensionRegistry fullRegistry1 = (ExtensionRegistry) registry1;
      ExtensionRegistry fullRegistry2 = (ExtensionRegistry) registry2;

      assertWithMessage("Test is using a non-lite extension")
          .that(NonNestedExtensionLite.nonNestedExtensionLite.getClass())
          .isInstanceOf(GeneratedMessageLite.GeneratedExtension.class);
      assertWithMessage("Extension is not registered in masqueraded full registry")
          .that(fullRegistry1.findImmutableExtensionByName("protobuf_unittest.nonNestedExtension"))
          .isNull();
      GeneratedMessageLite.GeneratedExtension<NonNestedExtensionLite.MessageLiteToBeExtended, ?>
          extension =
              registry1.findLiteExtensionByNumber(
                  NonNestedExtensionLite.MessageLiteToBeExtended.getDefaultInstance(), 1);
      assertWithMessage("Extension registered in lite registry").that(extension).isNotNull();

      assertWithMessage("Test is using a non-lite extension")
          .that(Extension.class.isAssignableFrom(NonNestedExtension.nonNestedExtension.getClass()))
          .isTrue();
      assertWithMessage("Extension is registered in masqueraded full registry")
          .that(fullRegistry2.findImmutableExtensionByName("protobuf_unittest.nonNestedExtension"))
          .isNotNull();
    }

    @Override
    public void testAdd_immutable() {
      ExtensionRegistryLite registry1 = ExtensionRegistryLite.newInstance().getUnmodifiable();
      try {
        NonNestedExtensionLite.registerAllExtensions(registry1);
        assertWithMessage("expected exception").fail();
      } catch (UnsupportedOperationException expected) {
      }
      try {
        registry1.add(NonNestedExtensionLite.nonNestedExtensionLite);
        assertWithMessage("expected exception").fail();
      } catch (UnsupportedOperationException expected) {
      }

      ExtensionRegistryLite registry2 = ExtensionRegistryLite.newInstance().getUnmodifiable();
      try {
        NonNestedExtension.registerAllExtensions((ExtensionRegistry) registry2);
        assertWithMessage("expected exception").fail();
      } catch (IllegalArgumentException expected) {
      }
      try {
        registry2.add(NonNestedExtension.nonNestedExtension);
        assertWithMessage("expected exception").fail();
      } catch (IllegalArgumentException expected) {
      }
    }
  }

  /** Test implementations for the Lite usage of ExtensionRegistryFactory. */
  public static final class InnerLiteTest implements RegistryTests {

    @Override
    public void testCreate() {
      ExtensionRegistryLite registry = ExtensionRegistryFactory.create();

      assertThat(registry.getClass()).isEqualTo(ExtensionRegistryLite.class);
    }

    @Override
    public void testEmpty() {
      ExtensionRegistryLite emptyRegistry = ExtensionRegistryFactory.createEmpty();

      assertThat(emptyRegistry.getClass()).isEqualTo(ExtensionRegistryLite.class);
      assertThat(emptyRegistry).isEqualTo(ExtensionRegistryLite.EMPTY_REGISTRY_LITE);
    }

    @Override
    public void testIsFullRegistry() {
      ExtensionRegistryLite registry = ExtensionRegistryFactory.create();
      assertThat(ExtensionRegistryFactory.isFullRegistry(registry)).isFalse();
    }

    @Override
    public void testAdd() {
      ExtensionRegistryLite registry = ExtensionRegistryLite.newInstance();
      NonNestedExtensionLite.registerAllExtensions(registry);
      GeneratedMessageLite.GeneratedExtension<NonNestedExtensionLite.MessageLiteToBeExtended, ?>
          extension =
              registry.findLiteExtensionByNumber(
                  NonNestedExtensionLite.MessageLiteToBeExtended.getDefaultInstance(), 1);
      assertWithMessage("Extension is registered in Lite registry").that(extension).isNotNull();
    }

    @Override
    public void testAdd_immutable() {
      ExtensionRegistryLite registry = ExtensionRegistryLite.newInstance().getUnmodifiable();
      try {
        NonNestedExtensionLite.registerAllExtensions(registry);
        assertWithMessage("expected exception").fail();
      } catch (UnsupportedOperationException expected) {
      }
    }
  }

  /** Defines a suite of tests which the JUnit3 runner retrieves by reflection. */
  public static Test suite() {
    TestSuite suite = new TestSuite();
    for (Method method : RegistryTests.class.getMethods()) {
      suite.addTest(TestSuite.createTest(ExtensionRegistryFactoryTest.class, method.getName()));
    }
    return suite;
  }

  /**
   * Sequentially runs first the Lite and then the non-Lite test variant via classloader
   * manipulation.
   */
  @Override
  public void runTest() throws Exception {
    ClassLoader storedClassLoader = Thread.currentThread().getContextClassLoader();
    Thread.currentThread().setContextClassLoader(LITE_CLASS_LOADER);
    try {
      runTestMethod(LITE_CLASS_LOADER, InnerLiteTest.class);
    } finally {
      Thread.currentThread().setContextClassLoader(storedClassLoader);
    }
    try {
      runTestMethod(storedClassLoader, InnerTest.class);
    } finally {
      Thread.currentThread().setContextClassLoader(storedClassLoader);
    }
  }

  private void runTestMethod(ClassLoader classLoader, Class<? extends RegistryTests> testClass)
      throws Exception {
    classLoader.loadClass(ExtensionRegistryFactory.class.getName());
    Class<?> test = classLoader.loadClass(testClass.getName());
    String testName = getName();
    test.getMethod(testName).invoke(test.getDeclaredConstructor().newInstance());
  }

  /**
   * Constructs a custom ClassLoader blacklisting the classes which are inspected in the SUT to
   * determine the Lite/non-Lite runtime.
   */
  private static ClassLoader getLiteOnlyClassLoader() {

    ImmutableSet<ClassPath.ClassInfo> classes = ImmutableSet.of();
    try {
      classes = ClassPath.from(ExtensionRegistryFactoryTest.class.getClassLoader()).getAllClasses();
    } catch (IOException ex) {
      throw new RuntimeException(ex);
    }
    URL[] urls = new URL[classes.size()];
    int i = 0;
    for (ClassPath.ClassInfo classInfo : classes) {
      urls[i++] = classInfo.url();
    }
    final ImmutableSet<String> classNamesNotInLite =
        ImmutableSet.of(
            ExtensionRegistryFactory.FULL_REGISTRY_CLASS_NAME,
            ExtensionRegistry.EXTENSION_CLASS_NAME);

    // Construct a URLClassLoader delegating to the system ClassLoader, and looking up classes
    // in jar files based on the URLs already configured for this test's UrlClassLoader.
    // Certain classes throw a ClassNotFoundException by design.
    return new URLClassLoader(urls, ClassLoader.getSystemClassLoader()) {
      @Override
      public Class<?> loadClass(String name, boolean resolve) throws ClassNotFoundException {
        if (classNamesNotInLite.contains(name)) {
          throw new ClassNotFoundException("Class deliberately blacklisted by test.");
        }
        Class<?> loadedClass = null;
        try {
          loadedClass = findLoadedClass(name);
          if (loadedClass == null) {
            loadedClass = findClass(name);
            if (resolve) {
              resolveClass(loadedClass);
            }
          }
        } catch (ClassNotFoundException | SecurityException e) {
          // Java 8+ would throw a SecurityException if we attempt to find a loaded class from
          // java.lang.* package. We don't really care about those anyway, so just delegate to the
          // parent class loader.
          loadedClass = super.loadClass(name, resolve);
        }
        return loadedClass;
      }
    };
  }
}
