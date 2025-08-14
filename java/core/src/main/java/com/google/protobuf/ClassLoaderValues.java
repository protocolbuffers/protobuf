package com.google.protobuf;

import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.util.concurrent.ConcurrentHashMap;
import java.util.function.Function;

final class ClassLoaderValues {

  private static final Method createOrGetClassLoaderValueMapMethod;

  static {
    Method method;
    try {
      final Class<ClassLoader> clazz = ClassLoader.class;
      method = clazz.getDeclaredMethod("createOrGetClassLoaderValueMap");
      method.setAccessible(true);
      if (Modifier.isStatic(method.getModifiers())) {
        throw new IllegalStateException(
            "Expected " + clazz.getSimpleName() + "." + method.getName() + " to not be static");
      }
      if (Modifier.isAbstract(method.getModifiers())) {
        throw new IllegalStateException(
            "Expected " + clazz.getSimpleName() + "." + method.getName() + " to not be abstract");
      }
      if (method.getReturnType() != ConcurrentHashMap.class) {
        throw new IllegalStateException(
            "Expected "
                + clazz.getSimpleName()
                + "."
                + method.getName()
                + " to return "
                + ConcurrentHashMap.class.getSimpleName());
      }
    } catch (ReflectiveOperationException | RuntimeException ignored) {
      method = null;
    }
    createOrGetClassLoaderValueMapMethod = method;
  }

  @SuppressWarnings("unchecked")
  private static ConcurrentHashMap<Object, Object> map(ClassLoader classLoader) {
    if (createOrGetClassLoaderValueMapMethod == null) {
      return null;
    }
    try {
      return ((ConcurrentHashMap<Object, Object>)
          createOrGetClassLoaderValueMapMethod.invoke(classLoader));
    } catch (ReflectiveOperationException ignored) {
      return null;
    }
  }

  static <V> V get(ClassLoader classLoader, Object key, Class<V> valueClass) {
    ConcurrentHashMap<?, ?> values = map(classLoader);
    if (values == null) {
      return null;
    }
    return valueClass.cast(values.get(key));
  }

  @SuppressWarnings("unchecked")
  static <V> V computeIfAbsent(
      ClassLoader classLoader, Object key, Class<V> valueClass, Function<?, ? extends V> function) {
    ConcurrentHashMap<Object, Object> values = map(classLoader);
    if (values == null) {
      return null;
    }
    return valueClass.cast(values.computeIfAbsent(key, (Function<Object, Object>) function));
  }

  private ClassLoaderValues() {}
}
