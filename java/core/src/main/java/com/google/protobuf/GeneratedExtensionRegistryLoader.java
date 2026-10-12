// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Iterator;
import java.util.ServiceConfigurationError;
import java.util.ServiceLoader;
import java.util.logging.Level;
import java.util.logging.Logger;

/** Loads the blaze generated extension registry using a {@link ServiceLoader}. */
// public for b/312040363#comment2; no one should use this directly
public abstract class GeneratedExtensionRegistryLoader<T extends ExtensionRegistryLite> {

  // LINT.IfChange
  // Non-final because proguard might not obfuscate the string if it were inlined by the compiler.
  @SuppressWarnings("ConstantField")
  private static String LITE_CLASS_NAME =
      "com.google.protobuf.BlazeGeneratedExtensionRegistryLiteLoader";
  // LINT.ThenChange(//depot/google3/third_party/java_src/protobuf/current/java/com/google/protobuf/lite_proguard.pgcfg)

  protected abstract T getInstance();

  @SuppressWarnings("unchecked")
  static <T extends ExtensionRegistryLite> T load(Class<T> extensionRegistryClass) {
    ClassLoader classLoader = GeneratedExtensionRegistryLoader.class.getClassLoader();
    if (extensionRegistryClass.equals(ExtensionRegistryLite.class)) {
      // When lite protos are present during the invocation of java_binary/java_plugin/... a special
      // well known class is created using com.google.protobuf.tools.ExtensionRegistryGenerator.
      // This is not done for full protos. So we only bother looking for the well known class when
      // trying to load the lite extension registry.
      //
      // Note that Blaze forbids mixing lite and full protos in Java within google3. This is not
      // true in OSS, but thankfully this code is not exercised in OSS at all yet.
      try {
        Class<GeneratedExtensionRegistryLoader<T>> loaderClass =
            (Class<GeneratedExtensionRegistryLoader<T>>)
                // We use an identifiernamestring'd constant to support proguard obfuscation
                Class.forName(LITE_CLASS_NAME, true, classLoader);
        GeneratedExtensionRegistryLoader<T> loader;
        try {
          loader = loaderClass.getConstructor().newInstance();
        } catch (ReflectiveOperationException e) {
          throw new IllegalStateException(e);
        }

        return extensionRegistryClass.cast(loader.getInstance());
      } catch (ClassNotFoundException cnfe) {
        // fall through to ServiceLoader
      }
    }

    // There is not much evidence that this path is actually used for lite protos, but it has not
    // been ruled out definitely yet. If lite protos were used on non-Android this code, in theory,
    // could be used. com.google.protobuf.tools.ExtensionRegistryGenerator still generates
    // service loader entries even for lite protos. This path is very much used with full protos,
    // though there is typically only one service loader entry outside of java_plugin.
    ServiceLoader<GeneratedExtensionRegistryLoader<T>> loader =
        ServiceLoader.load(
            (Class<GeneratedExtensionRegistryLoader<T>>)
                (Class<?>) GeneratedExtensionRegistryLoader.class,
            classLoader);
    Iterator<GeneratedExtensionRegistryLoader<T>> it = loader.iterator();
    ArrayList<T> registries = new ArrayList<T>();
    while (it.hasNext()) {
      try {
        registries.add(extensionRegistryClass.cast(it.next().getInstance()));
      } catch (ServiceConfigurationError e) {
        Logger.getLogger(CodedOutputStream.class.getName())
            .log(Level.SEVERE, "Unable to load " + extensionRegistryClass.getSimpleName(), e);
      }
    }
    if (registries.size() == 1) {
      return registries.get(0);
    }
    if (registries.size() == 0) {
      return null;
    }

    // targets that have java_plugins have all processors executed in a batch
    // and their classpath is combined. as such composed all extension registries
    // on the classpath.
    try {
      // this is only defined on ExtensionRegistry but we can't link to it
      // directly since this class is part of the lite runtime. the reflective
      // call will fail for lite runtime though it should never get here since
      // lite runtime uses a well known name.
      return (T)
          extensionRegistryClass.getMethod("combine", Collection.class).invoke(null, registries);
    } catch (ReflectiveOperationException e) {
      throw new IllegalStateException(e);
    }
  }
}
