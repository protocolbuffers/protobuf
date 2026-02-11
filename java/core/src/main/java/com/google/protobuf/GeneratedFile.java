package com.google.protobuf;

/**
 * All generated outer classes extend this class. This class implements shared logic that should
 * only be called by generated code.
 *
 * <p>Users should generally ignore this class and use the generated Outer class directly instead.
 *
 * <p>This class is intended to only be extended by protoc created gencode. It is not intended or
 * supported to extend this class, and any protected methods may be removed without it being
 * considered a breaking change as long as all supported gencode does not depend on the changed
 * methods.
 */
public abstract class GeneratedFile {

  protected GeneratedFile() {}

  /** Add an optional extension from a file that may or may not be in the classpath. */
  protected static void addOptionalExtension(
      ExtensionRegistry registry, final String className, final String fieldName) {
    try {
      GeneratedMessage.GeneratedExtension<?, ?> ext =
          (GeneratedMessage.GeneratedExtension<?, ?>)
              Class.forName(className).getField(fieldName).get(null);
      registry.add(ext);
    } catch (ClassNotFoundException e) {
      // ignore missing class if it's not in the classpath.
    } catch (NoSuchFieldException e) {
      // ignore missing field if it's not in the classpath.
    } catch (IllegalAccessException e) {
      // ignore malformed field if it's not what we expect.
    }
  }
}
