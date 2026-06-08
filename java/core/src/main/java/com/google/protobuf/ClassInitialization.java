package com.google.protobuf;

final class ClassInitialization {
  private ClassInitialization() {}

  static void forceInit(Class<?> clazz) {
    try {
      Class.forName(clazz.getName(), true, clazz.getClassLoader());
    } catch (ClassNotFoundException e) {
      throw new IllegalStateException("Class initialization cannot fail.", e);
    }
  }
}
