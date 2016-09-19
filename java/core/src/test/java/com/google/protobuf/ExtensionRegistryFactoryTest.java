// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

package com.google.protobuf;

import protobuf_unittest.NonNestedExtension;
import protobuf_unittest.NonNestedExtensionLite;
import java.lang.reflect.Method;
import java.net.URLClassLoader;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashSet;
import java.util.Set;
import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;

/**
 * Tests for {@link ExtensionRegistryFactory} and the {@link ExtensionRegistry} instances it
 * creates.
 *
 * <p>This test simulates the runtime behaviour of the ExtensionRegistryFactory by delegating test
 * definitions to two inner classes {@link InnerTest} and {@link InnerLiteTest}, the latter of
 * which is executed using a custom ClassLoader, simulating the ProtoLite environment.
 *
 * <p>The test mechanism employed here is based on the pattern in
 * {@code com.google.common.util.concurrent.AbstractFutureFallbackAtomicHelperTest}
 */
public class ExtensionRegistryFactoryTest extends TestCase {

  // A classloader which blacklists some non-Lite classes.
  private static final ClassLoader LITE_CLASS_LOADER = getLiteOnlyClassLoader();

  /**
   * Defines the set of test methods which will be run.
   */
  static interface RegistryTests {
    void testCreate();
    void testEmpty();
    void testIsFullRegistry();
    void testAdd();
    void testAdd_immutable();
  }

  /**
   * Test implementations for the non-Lite usage of ExtensionRegistryFactory.
   */
  public static class InnerTest implements RegistryTests {

    @Override
    public void testCreate() {
      ExtensionRegistryLite registry = ExtensionRegistryFactory.create();

      assertEquals(registry.getClass(), ExtensionRegistry.class);
    }

    @Override
    public void testEmpty() {
      ExtensionRegistryLite emptyRegistry = ExtensionRegistryFactory.createEmpty();

      assertEquals(emptyRegistry.getClass(), ExtensionRegistry.class);
      assertEquals(emptyRegistry, ExtensionRegistry.EMPTY_REGISTRY);
    }

    @Override
    public void testIsFullRegistry() {
      ExtensionRegistryLite registry = ExtensionRegistryFactory.create();
      assertTrue(ExtensionRegistryFactory.isFullRegistry(registry));
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

      assertTrue("Test is using a non-lite extension",
          GeneratedMessageLite.GeneratedExtension.class.isAssignableFrom(
              NonNestedExtensionLite.nonNestedExtensionLite.getClass()));
      assertNull("Extension is not registered in masqueraded full registry",
          fullRegistry1.findImmutableExtensionByName("protobuf_unittest.nonNestedExtension"));
      GeneratedMessageLite.GeneratedExtension<NonNestedExtensionLite.MessageLiteToBeExtended, ?>
      extension = registry1.findLiteExtensionByNumber(
          NonNestedExtensionLite.MessageLiteToBeExtended.getDefaultInstance(), 1);
      assertNotNull("Extension registered in lite registry", extension);

      assertTrue("Test is using a non-lite extension",
          GeneratedMessage.GeneratedExtension.class.isAssignableFrom(
          NonNestedExtension.nonNestedExtension.getClass()));
      assertNotNull("Extension is registered in masqueraded full registry",
          fullRegistry2.findImmutableExtensionByName("protobuf_unittest.nonNestedExtension"));
    }

    @Override
    public void testAdd_immutable() {
      ExtensionRegistryLite registry1 = ExtensionRegistryLite.newInstance().getUnmodifiable();
      try {
        NonNestedExtensionLite.registerAllExtensions(registry1);
        fail();
      } catch (UnsupportedOperationException expected) {}
      try {
        registry1.add(NonNestedExtensionLite.nonNestedExtensionLite);
        fail();
      } catch (UnsupportedOperationException expected) {}

      ExtensionRegistryLite registry2 = ExtensionRegistryLite.newInstance().getUnmodifiable();
      try {
        NonNestedExtension.registerAllExtensions((ExtensionRegistry) registry2);
        fail();
      } catch (IllegalArgumentException expected) {}
      try {
        registry2.add(NonNestedExtension.nonNestedExtension);
        fail();
      } catch (IllegalArgumentException expected) {}
    }
  }

  /**
   * Test implementations for the Lite usage of ExtensionRegistryFactory.
   */
  public static final class InnerLiteTest implements RegistryTests {

    @Override
    public void testCreate() {
      ExtensionRegistryLite registry = ExtensionRegistryFactory.create();

      assertEquals(registry.getClass(), ExtensionRegistryLite.class);
    }

    @Override
    public void testEmpty() {
      ExtensionRegistryLite emptyRegistry = ExtensionRegistryFactory.createEmpty();

      assertEquals(emptyRegistry.getClass(), ExtensionRegistryLite.class);
      assertEquals(emptyRegistry, ExtensionRegistryLite.EMPTY_REGISTRY_LITE);
    }

    @Override
    public void testIsFullRegistry() {
      ExtensionRegistryLite registry = ExtensionRegistryFactory.create();
      assertFalse(ExtensionRegistryFactory.isFullRegistry(registry));
    }

    @Override
    public void testAdd() {
      ExtensionRegistryLite registry = ExtensionRegistryLite.newInstance();
      NonNestedExtensionLite.registerAllExtensions(registry);
      GeneratedMessageLite.GeneratedExtension<NonNestedExtensionLite.MessageLiteToBeExtended, ?>
          extension = registry.findLiteExtensionByNumber(
              NonNestedExtensionLite.MessageLiteToBeExtended.getDefaultInstance(), 1);
      assertNotNull("Extension is registered in Lite registry", extension);
    }

    @Override
    public void testAdd_immutable() {
      ExtensionRegistryLite registry = ExtensionRegistryLite.newInstance().getUnmodifiable();
      try {
        NonNestedExtensionLite.registerAllExtensions(registry);
        fail();
      } catch (UnsupportedOperationException expected) {}
    }
  }

  /**
   * Defines a suite of tests which the JUnit3 runner retrieves by reflection.
   */
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
    test.getMethod(testName).invoke(test.newInstance());
  }

  /**
   * Constructs a custom ClassLoader blacklisting the classes which are inspected in the SUT
   * to determine the Lite/non-Lite runtime.
   */
  private static ClassLoader getLiteOnlyClassLoader() {
    ClassLoader testClassLoader = ExtensionRegistryFactoryTest.class.getClassLoader();
    final Set<String> classNamesNotInLite =
        Collections.unmodifiableSet(
            new HashSet<String>(
                Arrays.asList(
                    ExtensionRegistryFactory.FULL_REGISTRY_CLASS_NAME,
                    ExtensionRegistry.EXTENSION_CLASS_NAME)));

    // Construct a URLClassLoader delegating to the system ClassLoader, and looking up classes
    // in jar files based on the URLs already configured for this test's UrlClassLoader.
    // Certain classes throw a ClassNotFoundException by design.
    return new URLClassLoader(((URLClassLoader) testClassLoader).getURLs(),
        ClassLoader.getSystemClassLoader()) {
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
        } catch (ClassNotFoundException e) {
          loadedClass = super.loadClass(name, resolve);
        }
        return loadedClass;
      }
    };
  }
}
