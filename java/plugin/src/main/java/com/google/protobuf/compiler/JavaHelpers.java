package com.google.protobuf.compiler;

import com.google.protobuf.DescriptorProtos.FileOptions;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.EnumDescriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.Descriptors.FieldDescriptor.JavaType;
import com.google.protobuf.Descriptors.FileDescriptor;
import com.google.protobuf.Descriptors.MethodDescriptor;
import com.google.protobuf.Descriptors.ServiceDescriptor;
import java.util.Collections;
import java.util.HashMap;
import java.util.Locale;
import java.util.Map;

/**
 * Various helper methods, particularly useful when generating Java code.
 * 
 * @author t.broyer@ltgt.net Thomas Broyer
 * <br>Based on the initial work of:
 * @author kenton@google.com Kenton Varda
 * <br>Based on original Protocol Buffers design by
 * Sanjay Ghemawat, Jeff Dean, and others.
 */
public final class JavaHelpers {

  /**
   * Name of the outer class scope insertion point.
   */
  public static String getOuterClassScopeInsertionPoint() {
    return "outer_class_scope";
  }

  /**
   * Returns the insertion point name for a given message's class scope.
   */
  public static String getClassScopeInsertionPoint(Descriptor descriptor) {
    return "class_scope:" + descriptor.getFullName();
  }

  /**
   * Returns the insertion point name for a given message's builder class scope.
   */
  public static String getBuilderScopeInsertionPoint(Descriptor descriptor) {
    return "builder_scope:" + descriptor.getFullName();
  }

  /**
   * Returns the insertion point name for a given enum's scope.
   */
  public static String getEnumScopeInsertionPoint(EnumDescriptor descriptor) {
    return "enum_scope:" + descriptor.getFullName();
  }

  /**
   * Commonly-used separator comments (a line of '=').
   */
  public static final String THICK_SEPARATOR =
    "// ===================================================================\n";
  /**
   * Commonly-used separator comments (a line of '-').
   */
  public static final String THIN_SEPARATOR =
    "// -------------------------------------------------------------------\n";

  /**
   * Parses a set of comma-delimited name/value pairs.
   * <p>
   * Several code generators treat the parameter argument as holding a list of
   * options separated by commas: e.g., {@code "foo=bar,baz,qux=corge"} parses
   * to the pairs: {@code ("foo", "bar"), ("baz", ""), ("qux", "corge")}.
   * <p>
   * When a key is present several times, only the last value is retained.
   */
  public static Map<String, String> parseGeneratorParameter(
      String text) {
    if (text.isEmpty()) {
      return Collections.emptyMap();
    }
    Map<String, String> ret = new HashMap<String, String>();
    String[] parts = text.split(",");
    for (String part : parts) {
      if (part.isEmpty()) {
        continue;
      }
      int equalsPos = part.indexOf('=');
      String key, value;
      if (equalsPos < 0) {
        key = part;
        value = "";
      } else {
        key = part.substring(0, equalsPos);
        value = part.substring(equalsPos + 1);
      }
      ret.put(key, value);
    }
    return ret;
  }

  /**
   * Returns the Java file name (and path) for a given message.
   * <p>
   * This depends on the {@code java_package}, {@code package} and
   * {@code java_multiple_files} options specified in the .proto file.
   */
  public static String getJavaFileName(Descriptor descriptor) {
    String fullName;
    if (descriptor.getFile().getOptions().getJavaMultipleFiles()) {
      Descriptor containingType;
      while ((containingType = descriptor.getContainingType()) != null) {
        descriptor = containingType;
      }
      fullName = JavaHelpers.getClassName(descriptor);
    } else {
      fullName = JavaHelpers.getClassName(descriptor.getFile());
    }
    return fullName.replace('.', '/') + ".java";
  }

  /**
   * Returns the Java file name (and path) for a given enum.
   * <p>
   * This depends on the {@code java_package}, {@code package} and
   * {@code java_multiple_files} options specified in the .proto file.
   */
  public static String getJavaFileName(EnumDescriptor descriptor) {
    if (descriptor.getContainingType() != null) {
      return getJavaFileName(descriptor.getContainingType());
    }
    String fullName;
    if (descriptor.getFile().getOptions().getJavaMultipleFiles()) {
      fullName = JavaHelpers.getClassName(descriptor);
    } else {
      fullName = JavaHelpers.getClassName(descriptor.getFile());
    }
    return fullName.replace('.', '/') + ".java";
  }

  /**
   * Returns the Java file name (and path) for a given service.
   * <p>
   * This depends on the {@code java_package}, {@code package} and
   * {@code java_multiple_files} options specified in the .proto file.
   */
  public static String getJavaFileName(ServiceDescriptor descriptor) {
    String fullName;
    if (descriptor.getFile().getOptions().getJavaMultipleFiles()) {
      fullName = JavaHelpers.getClassName(descriptor);
    } else {
      fullName = JavaHelpers.getClassName(descriptor.getFile());
    }
    return fullName.replace('.', '/') + ".java";
  }

  /**
   * Converts the field's name to camel-case, e.g. "foo_bar_baz" becomes
   * "fooBarBaz".
   */
  public static String underscoresToCamelCase(FieldDescriptor field) {
    return underscoresToCamelCaseImpl(getFieldName(field), false);
  }

  /**
   * Converts the field's name to camel-case, e.g. "foo_bar_baz" becomes
   * "FooBarBaz".
   */
  public static String underscoresToCapitalizedCamelCase(
      FieldDescriptor field) {
    return underscoresToCamelCaseImpl(getFieldName(field), true);
  }

  /**
   * Converts the method's name to camel-case. (Typically, this merely has the
   * effect of lower-casing the first letter of the name.)
   */
  public static String underscoresToCamelCase(MethodDescriptor method) {
    return underscoresToCamelCaseImpl(method.getName(), false);
  }

  /**
   * Strips ".proto" or ".protodevel" from the end of a filename.
   */
  public static String stripProto(String filename) {
    if (filename.endsWith(".protodevel")) {
      return filename.substring(0, filename.length() - ".protodevel".length());
    } else if (filename.endsWith(".proto")) {
      return filename.substring(0, filename.length() - ".proto".length());
    } else {
      return filename;
    }
  }

  /**
   * Gets the unqualified class name for the file.
   * <p>
   * Each .proto file becomes a single Java class, with all its contents nested
   * in that class, unless the {@code java_multiple_files} option has been set
   * to {@value true}.
   */
  public static String getFileClassName(FileDescriptor file) {
    if (file.getOptions().hasJavaOuterClassname()) {
      return file.getOptions().getJavaOuterClassname();
    } else {
      String basename = file.getName();
      int lastSlash = basename.lastIndexOf('/');
      if (lastSlash >= 0) {
        basename = basename.substring(lastSlash + 1);
      }
      return underscoresToCamelCaseImpl(stripProto(basename), true);
    }
  }

  /**
   * Returns the file's Java package name.
   * <p>
   * This depends on the {@code java_package} and {@code package} and options
   * specified in the .proto file.
   */
  public static String getFileJavaPackage(FileDescriptor file) {
    if (file.getOptions().hasJavaPackage()) {
      return file.getOptions().getJavaPackage();
    } else {
      return file.getPackage();
    }
  }

  /**
   * Converts the given fully-qualified name in the proto namespace to its
   * fully-qualified name in the Java namespace, given that it is in the given
   * file.
   */
  public static String toJavaName(String fullName, FileDescriptor file) {
    StringBuilder result = new StringBuilder();
    if (file.getOptions().getJavaMultipleFiles()) {
      result.append(getFileJavaPackage(file));
    } else {
      result.append(getClassName(file));
    }
    if (result.length() > 0) {
      result.append('.');
    }
    if (file.getPackage().isEmpty()) {
      result.append(fullName);
    } else {
      // Strip the proto package from full_name since we've replaced it
      // with the Java package.
      result.append(fullName.substring(file.getPackage().length() + 1));
    }
    return result.toString();
  }

  /**
   * Returns the fully-qualified class name corresponding to the given
   * message descriptor.
   */
  public static String getClassName(Descriptor descriptor) {
    return toJavaName(descriptor.getFullName(), descriptor.getFile());
  }

  /**
   * Returns the fully-qualified class name corresponding to the given
   * enum descriptor.
   */
  public static String getClassName(EnumDescriptor descriptor) {
    return toJavaName(descriptor.getFullName(), descriptor.getFile());
  }

  /**
   * Returns the fully-qualified class name corresponding to the given
   * service descriptor.
   */
  public static String getClassName(ServiceDescriptor descriptor) {
    return toJavaName(descriptor.getFullName(), descriptor.getFile());
  }

  /**
   * Returns the fully-qualified class name corresponding to the given
   * file descriptor.
   */
  public static String getClassName(FileDescriptor descriptor) {
    StringBuilder result = new StringBuilder(getFileJavaPackage(descriptor));
    if (result.length() > 0) {
      result.append('.');
    }
    result.append(getFileClassName(descriptor));
    return result.toString();
  }

  /**
   * Returns the unqualified name that should be used for a field's field
   * number constant.
   */
  public static String getFieldConstantName(FieldDescriptor field) {
    return field.getName().toUpperCase(Locale.ROOT) + "_FIELD_NUMBER";
  }

  /**
   * Returns the type name for a boxed primitive type, e.g. "int" for
   * {@link JavaType#INT}. Returns {@code null} for enum and message types.
   */
  public static String getPrimitiveTypeName(JavaType type) {
    switch (type) {
      case INT:
        return "int";
      case LONG:
        return "long";
      case FLOAT:
        return "float";
      case DOUBLE:
        return "double";
      case BOOLEAN:
        return "boolean";
      case STRING:
        return "java.lang.String";
      case BYTE_STRING:
        return "com.google.protobuf.ByteString";
      case ENUM:
        return null;
      case MESSAGE:
        return null;
      default:
        throw new IllegalArgumentException();
    }
  }

  /**
   * Returns the fully-qualified class name for a boxed primitive type, e.g.
   * "java.lang.Integer" for {@link JavaType#INT}. Returns {@code null} for
   * enum and message types.
   */
  public static String getBoxedPrimitiveTypeName(
      FieldDescriptor.JavaType type) {
    switch (type) {
      case INT:
        return "java.lang.Integer";
      case LONG:
        return "java.lang.Long";
      case FLOAT:
        return "java.lang.Float";
      case DOUBLE:
        return "java.lang.Double";
      case BOOLEAN:
        return "java.lang.Boolean";
      case STRING:
        return "java.lang.String";
      case BYTE_STRING:
        return "com.google.protobuf.ByteString";
      case ENUM:
        return null;
      case MESSAGE:
        return null;
      default:
        throw new IllegalArgumentException();
    }
  }

  /**
   * Returns whether this message keeps track of unknown fields.
   */
  public static boolean hasUnknownField(Descriptor descriptor) {
    return descriptor.getFile().getOptions().getOptimizeFor()
      != FileOptions.OptimizeMode.LITE_RUNTIME;
  }

  /**
   * Returns whether this message has generated parsing, serialization,
   * and other standard methods for which reflection-based fallback
   * implementations exist?
   */
  public static boolean hasGeneratedMethods(Descriptor descriptor) {
    return descriptor.getFile().getOptions().getOptimizeFor()
      != FileOptions.OptimizeMode.CODE_SIZE;
  }

  /**
   * Returns whether this message has descriptor and reflection methods?
   */
  public static boolean hasDescriptorMethods(Descriptor descriptor) {
    return descriptor.getFile().getOptions().getOptimizeFor()
      != FileOptions.OptimizeMode.LITE_RUNTIME;
  }

  /**
   * Returns whether this enum has descriptor and reflection methods?
   */
  public static boolean hasDescriptorMethods(EnumDescriptor descriptor) {
    return descriptor.getFile().getOptions().getOptimizeFor()
      != FileOptions.OptimizeMode.LITE_RUNTIME;
  }

  /**
   * Returns whether this service has descriptor and reflection methods?
   */
  public static boolean hasDescriptorMethods(ServiceDescriptor descriptor) {
    return descriptor.getFile().getOptions().getOptimizeFor()
      != FileOptions.OptimizeMode.LITE_RUNTIME;
  }

  /**
   * Returns whether this file has descriptor and reflection methods?
   */
  public static boolean hasDescriptorMethods(FileDescriptor file) {
    return file.getOptions().getOptimizeFor()
      != FileOptions.OptimizeMode.LITE_RUNTIME;
  }

  /**
   * Returns whether generic services should be generated for this file.
   */
  public static boolean hasGenericServices(FileDescriptor file) {
    return !file.getServices().isEmpty()
        && file.getOptions().getOptimizeFor()
            != FileOptions.OptimizeMode.LITE_RUNTIME
        && file.getOptions().getJavaGenericServices();
  }

  /**
   * Returns the name for the given field (special-casing groups).
   */
  private static String getFieldName(FieldDescriptor field) {
    // Groups are hacky: The name of the field is just the lower-cased name
    // of the group type. In Java, though, we would like to retain the
    // original capitalization of the type name.
    if (field.getType() == FieldDescriptor.Type.GROUP) {
      return field.getMessageType().getName();
    } else {
      return field.getName();
    }
  }

  /**
   * Converts the given input to camel-case, specifying whether the first
   * letter should be upper-case or lower-case.
   * 
   * @param input         string to be converted
   * @param capNextLetter {@code true} if the first letter should be turned to
   *                      upper-case.
   * @return the camel-cased string
   */
  private static String underscoresToCamelCaseImpl(String input,
      boolean capNextLetter) {
    StringBuilder result = new StringBuilder(input.length());
    for (int i = 0, l = input.length(); i < l; i++) {
      char c = input.charAt(i);
      if ('a' <= c && c <= 'z') {
        if (capNextLetter) {
          result.append((char) (c + ('A' - 'a')));
        } else {
          result.append(c);
        }
        capNextLetter = false;
      } else if ('A' <= c && c <= 'Z') {
        if (i == 0 && !capNextLetter) {
          // Force first letter to lower-case unless explicitly told
          // to capitalize it.
          result.append((char) (c + ('a' - 'A')));
        } else {
          // Capital letters after the first are left as-is.
          result.append(c);
        }
        capNextLetter = false;
      } else if ('0' <= c && c <= '9') {
        result.append(c);
        capNextLetter = true;
      } else {
        capNextLetter = true;
      }
    }
    return result.toString();
  }

  private JavaHelpers() {
  }
}
