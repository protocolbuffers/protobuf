package com.google.protobuf;

import com.google.protobuf.DescriptorProtos.DescriptorProto;
import com.google.protobuf.DescriptorProtos.Edition;
import com.google.protobuf.DescriptorProtos.EnumDescriptorProto;
import com.google.protobuf.DescriptorProtos.FeatureSet;
import com.google.protobuf.DescriptorProtos.FileDescriptorProtoOrBuilder;
import com.google.protobuf.DescriptorProtos.FileOptions;
import com.google.protobuf.DescriptorProtos.ServiceDescriptorProto;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.EnumDescriptor;
import com.google.protobuf.Descriptors.FileDescriptor;
import com.google.protobuf.Descriptors.ServiceDescriptor;
import com.google.protobuf.GeneratedMessage.GeneratedExtension;
import com.google.protobuf.JavaFeaturesProto.JavaFeatures;

/** Class containing helper methods for predicting names of generated java classes. */
public final class GeneratorNames {

  private GeneratorNames() {}

  /** Returns the generated package for the given file descriptor proto. */
  public static String getFileJavaPackage(FileDescriptorProtoOrBuilder file) {
    return getProto2ApiDefaultJavaPackage(file.getOptions(), file.getPackage());
  }

  /** Returns the generated package for the given file descriptor. */
  public static String getFileJavaPackage(FileDescriptor file) {
    return getProto2ApiDefaultJavaPackage(file.getOptions(), file.getPackage());
  }

  /** Returns the default java package for the given file. */
  static String getDefaultJavaPackage(FileOptions fileOptions, String filePackage) {
    // Replicates the logic from DefaultJavaPackage in java/names_internal.h.
    if (fileOptions.hasJavaPackage()) {
      return fileOptions.getJavaPackage();
    } else {
      return filePackage;
    }
  }

  /** Joins two package segments into a single package name with a dot separator. */
  static String joinPackage(String a, String b) {
    if (a.isEmpty()) {
      return b;
    } else if (b.isEmpty()) {
      return a;
    } else {
      return a + '.' + b;
    }
  }

  /**
   * Returns the package name to use for a file that is being compiled as proto2-API. If the file is
   * declared as proto1-API, this may involve using the alternate package name.
   */
  static String getProto2ApiDefaultJavaPackage(FileOptions fileOptions, String filePackage) {
    // Replicates the logic from Proto2DefaultJavaPackage in java/names_internal.h.
    return getDefaultJavaPackage(fileOptions, filePackage);
  }

  /**
   * Returns the generated unqualified outer file class name for the given file descriptor proto.
   */
  public static String getFileClassName(FileDescriptorProtoOrBuilder file) {
    return getFileClassNameImpl(file, getResolvedFileFeatures(JavaFeaturesProto.java_, file));
  }

  /** Returns the generated unqualified outer file class name for the given file descriptor. */
  public static String getFileClassName(FileDescriptor file) {
    return getFileClassNameImpl(
        file.toProto(), file.getFeatures().getExtension(JavaFeaturesProto.java_));
  }

  private static String getFileClassNameImpl(
      FileDescriptorProtoOrBuilder file, JavaFeatures resolvedFeatures) {
    // Replicates the logic of ClassNameResolver::GetFileImmutableClassName.
    if (file.getOptions().hasJavaOuterClassname()) {
      return file.getOptions().getJavaOuterClassname();
    }

    String className =
        getDefaultFileClassName(file, resolvedFeatures.getUseOldOuterClassnameDefault());
    if (resolvedFeatures.getUseOldOuterClassnameDefault()
        && hasConflictingClassName(file, className)) {
      return className + "OuterClass";
    }
    return className;
  }

  /**
   * Returns the resolved features for the given descriptor proto.
   *
   * <p>This method isn't actually naming-specific, but lives here for now because it's the only
   * place it's needed. Once we have a better home for dealing with features in code generators,
   * this should move.
   */
  @SuppressWarnings("unchecked")
  static <T extends Message> T getResolvedFileFeatures(
      GeneratedExtension<FeatureSet, T> ext, FileDescriptorProtoOrBuilder file) {
    Edition edition;
    if (file.getSyntax().equals("proto3")) {
      edition = Edition.EDITION_PROTO3;
    } else if (!file.hasEdition()) {
      edition = Edition.EDITION_PROTO2;
    } else {
      edition = file.getEdition();
    }
    FeatureSet features = file.getOptions().getFeatures();
    if (features.getUnknownFields().hasField(ext.getNumber())) {
      // The incoming descriptor proto may not have been parsed with the right extension registry,
      // in which case we need to parse it again with the right extensions.
      ExtensionRegistry registry = ExtensionRegistry.newInstance();
      registry.add(ext);
      try {
        features =
            FeatureSet.newBuilder()
                .mergeFrom(features.getUnknownFields().toByteString(), registry)
                .build();
      } catch (InvalidProtocolBufferException e) {
        // This should never happen, since the unknown fields have already been parsed and features
        // are always integral types.
        throw new IllegalArgumentException("Failed to parse features", e);
      }
    }
    return (T)
        Descriptors.getEditionDefaults(edition).getExtension(ext).toBuilder()
            .mergeFrom(features.getExtension(ext))
            .build();
  }

  /**
   * Returns the default unqualified file class name for the given file.
   *
   * @param file the file descriptor proto
   * @param useOldOuterClassnameDefault whether to use the old default for the outer classname
   */
  static String getDefaultFileClassName(
      FileDescriptorProtoOrBuilder file, boolean useOldOuterClassnameDefault) {
    // Replicates the logic of ClassNameResolver::GetFileDefaultImmutableClassName.
    String name = file.getName();
    // The `+ 1` includes the case where no '/' is present.
    name = name.substring(name.lastIndexOf('/') + 1);
    name = underscoresToCamelCase(stripProto(name));
    return useOldOuterClassnameDefault ? name : name + "Proto";
  }

  private static String stripProto(String filename) {
    // Replicates the logic of ClassNameResolver::StripProto.
    if (filename.endsWith(".protodevel")) {
      return filename.substring(0, filename.length() - ".protodevel".length());
    }
    if (filename.endsWith(".proto")) {
      return filename.substring(0, filename.length() - ".proto".length());
    }

    return filename;
  }

  /** Checks whether any generated classes conflict with the given name. */
  private static boolean hasConflictingClassName(FileDescriptorProtoOrBuilder file, String name) {
    for (EnumDescriptorProto enumDesc : file.getEnumTypeList()) {
      if (name.equals(enumDesc.getName())) {
        return true;
      }
    }
    for (ServiceDescriptorProto serviceDesc : file.getServiceList()) {
      if (name.equals(serviceDesc.getName())) {
        return true;
      }
    }
    for (DescriptorProto messageDesc : file.getMessageTypeList()) {
      if (hasConflictingClassName(messageDesc, name)) {
        return true;
      }
    }
    return false;
  }

  /** Used by the other overload, descends recursively into messages. */
  private static boolean hasConflictingClassName(DescriptorProto messageDesc, String name) {
    if (name.equals(messageDesc.getName())) {
      return true;
    }
    for (EnumDescriptorProto enumDesc : messageDesc.getEnumTypeList()) {
      if (name.equals(enumDesc.getName())) {
        return true;
      }
    }
    for (DescriptorProto nestedMessageDesc : messageDesc.getNestedTypeList()) {
      if (hasConflictingClassName(nestedMessageDesc, name)) {
        return true;
      }
    }
    return false;
  }

  /**
   * Converts a name to camel-case.
   *
   * @param input the input string to convert
   * @param capitalizeNextLetter whether to capitalize the first letter
   */
  static String underscoresToCamelCase(String input, boolean capitalizeNextLetter) {
    // Replicates the logic of ClassNameResolver::UnderscoresToCamelCase.
    StringBuilder result = new StringBuilder();
    for (int i = 0; i < input.length(); i++) {
      char ch = input.charAt(i);
      if ('a' <= ch && ch <= 'z') {
        if (capitalizeNextLetter) {
          result.append((char) (ch + ('A' - 'a')));
        } else {
          result.append(ch);
        }
        capitalizeNextLetter = false;
      } else if ('A' <= ch && ch <= 'Z') {
        if (i == 0 && !capitalizeNextLetter) {
          // Force first letter to lower-case unless explicitly told to
          // capitalize it.
          result.append((char) (ch + ('a' - 'A')));
        } else {
          // Capital letters after the first are left as-is.
          result.append(ch);
        }
        capitalizeNextLetter = false;
      } else if ('0' <= ch && ch <= '9') {
        result.append(ch);
        capitalizeNextLetter = true;
      } else {
        capitalizeNextLetter = true;
      }
    }
    return result.toString();
  }

  static String underscoresToCamelCase(String input) {
    return underscoresToCamelCase(input, /* capitalizeNextLetter= */ true);
  }

  /**
   * Returns the fully qualified Java bytecode class name for the given message descriptor.
   *
   * <p>Nested classes will use '$' as the separator, rather than '.'.
   */
  public static String getBytecodeClassName(Descriptor message) {
    // Replicates the logic for ClassName from immutable/names.h
    return getClassFullName(
        getClassNameWithoutPackage(message), message.getFile(), !getNestInFileClass(message));
  }

  /**
   * Returns the fully qualified Java bytecode class name for the given enum descriptor.
   *
   * <p>Nested classes will use '$' as the separator, rather than '.'.
   */
  public static String getBytecodeClassName(EnumDescriptor enm) {
    // Replicates the logic for ClassName from immutable/names.h
    return getClassFullName(
        getClassNameWithoutPackage(enm), enm.getFile(), !getNestInFileClass(enm));
  }

  /**
   * Returns the fully qualified Java bytecode class name for the given service descriptor.
   *
   * <p>Nested classes will use '$' as the separator, rather than '.'.
   */
  static String getBytecodeClassName(ServiceDescriptor service) {
    // Replicates the logic for ClassName from immutable/names.h
    String suffix = "";
    boolean isOwnFile = !getNestInFileClass(service);
    return getClassFullName(getClassNameWithoutPackage(service), service.getFile(), isOwnFile)
        + suffix;
  }

  static String getQualifiedFromBytecodeClassName(String bytecodeClassName) {
    return bytecodeClassName.replace('$', '.');
  }

  /**
   * Returns the fully qualified Java class name for the given message descriptor.
   *
   * <p>Nested classes will use '.' as the separator, rather than '$'.
   */
  public static String getQualifiedClassName(Descriptor message) {
    return getQualifiedFromBytecodeClassName(getBytecodeClassName(message));
  }

  /**
   * Returns the fully qualified Java class name for the given enum descriptor.
   *
   * <p>Nested classes will use '.' as the separator, rather than '$'.
   */
  public static String getQualifiedClassName(EnumDescriptor enm) {
    return getQualifiedFromBytecodeClassName(getBytecodeClassName(enm));
  }

  /**
   * Returns the fully qualified Java class name for the given service descriptor.
   *
   * <p>Nested classes will use '.' as the separator, rather than '$'.
   */
  public static String getQualifiedClassName(ServiceDescriptor service) {
    return getQualifiedFromBytecodeClassName(getBytecodeClassName(service));
  }

  private static String getClassFullName(
      String nameWithoutPackage, FileDescriptor file, boolean isOwnFile) {
    // Replicates the logic for ClassNameResolver::GetJavaClassFullName from immutable/names.cc
    StringBuilder result = new StringBuilder();
    if (isOwnFile) {
      result.append(getFileJavaPackage(file.toProto()));
      if (result.length() > 0) {
        result.append(".");
      }
    } else {
      result.append(joinPackage(getFileJavaPackage(file.toProto()), getFileClassName(file)));
      if (result.length() > 0) {
        result.append("$");
      }
    }
    result.append(nameWithoutPackage.replace('.', '$'));
    return result.toString();
  }

  /** Returns the nest_in_file_class behavior for a given set of features in a specific file. */
  // Switch expressions were released in Java 14, and we support Java 8.
  @SuppressWarnings("StatementSwitchToExpressionSwitch")
  private static boolean getNestInFileClass(FileDescriptor file, JavaFeatures resolvedFeatures) {
    switch (resolvedFeatures.getNestInFileClass()) {
      case YES:
        return true;
      case NO:
        return false;
      case LEGACY:
        return !file.getOptions().getJavaMultipleFiles();
      default:
        throw new IllegalArgumentException("Java features are not resolved");
    }
  }

  public static boolean getNestInFileClass(Descriptor descriptor) {
    return getNestInFileClass(
        descriptor.getFile(), descriptor.getFeatures().getExtension(JavaFeaturesProto.java_));
  }

  public static boolean getNestInFileClass(EnumDescriptor descriptor) {
    return getNestInFileClass(
        descriptor.getFile(), descriptor.getFeatures().getExtension(JavaFeaturesProto.java_));
  }

  private static boolean getNestInFileClass(ServiceDescriptor descriptor) {
   return getNestInFileClass(
       descriptor.getFile(), descriptor.getFeatures().getExtension(JavaFeaturesProto.java_));
  }

  /** Returns the name of the given descriptor without the package name prefix. */
  static String stripPackageName(String fullName, FileDescriptor file) {
    if (file.getPackage().isEmpty()) {
      return fullName;
    } else {
      return fullName.substring(file.getPackage().length() + 1);
    }
  }

  /** Returns the name of the given message descriptor without the package name prefix. */
  static String getClassNameWithoutPackage(Descriptor message) {
    return stripPackageName(message.getFullName(), message.getFile());
  }

  /** Returns the name of the given enum descriptor without the package name prefix. */
  static String getClassNameWithoutPackage(EnumDescriptor enm) {
    Descriptor containingType = enm.getContainingType();
    if (containingType == null) {
      return enm.getName();
    }
    return joinPackage(getClassNameWithoutPackage(containingType), enm.getName());
  }

  /** Returns the name of the given service descriptor without the package name prefix. */
  static String getClassNameWithoutPackage(ServiceDescriptor service) {
    return stripPackageName(service.getFullName(), service.getFile());
  }
}
