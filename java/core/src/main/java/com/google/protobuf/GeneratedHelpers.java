package com.google.protobuf;

import com.google.protobuf.DescriptorProtos.DescriptorProto;
import com.google.protobuf.DescriptorProtos.Edition;
import com.google.protobuf.DescriptorProtos.EnumDescriptorProto;
import com.google.protobuf.DescriptorProtos.FeatureSet;
import com.google.protobuf.DescriptorProtos.FileDescriptorProtoOrBuilder;
import com.google.protobuf.DescriptorProtos.ServiceDescriptorProto;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.EnumDescriptor;
import com.google.protobuf.Descriptors.FileDescriptor;
import com.google.protobuf.Descriptors.ServiceDescriptor;
import com.google.protobuf.GeneratedMessage.GeneratedExtension;
import com.google.protobuf.JavaFeaturesProto.JavaFeatures;
import com.google.protobuf.JavaFeaturesProto.JavaFeatures.NestInFileClassFeature.NestInFileClass;

/** */
public final class GeneratedHelpers {

  private GeneratedHelpers() {}

  private static boolean nestInFileClass(FileDescriptor file, JavaFeatures resolvedFeatures) {
    return switch (resolvedFeatures.getNestInFileClass()) {
      case NestInFileClass.YES -> true;
      case NestInFileClass.NO -> false;
      case NestInFileClass.LEGACY -> !file.getOptions().getJavaMultipleFiles();
      default -> throw new IllegalArgumentException("Java features are not resolved");
    };
  }

  public static boolean nestInFileClass(Descriptor descriptor) {
    return nestInFileClass(
        descriptor.getFile(), descriptor.getFeatures().getExtension(JavaFeaturesProto.java_));
  }

  public static boolean nestInFileClass(EnumDescriptor descriptor) {
    return nestInFileClass(
        descriptor.getFile(), descriptor.getFeatures().getExtension(JavaFeaturesProto.java_));
  }

  public static boolean nestInFileClass(ServiceDescriptor descriptor) {
    return nestInFileClass(
        descriptor.getFile(), descriptor.getFeatures().getExtension(JavaFeaturesProto.java_));
  }

  public static String getFileClassName(FileDescriptorProtoOrBuilder file) {
    if (file.getOptions().hasJavaOuterClassname()) {
      return file.getOptions().getJavaOuterClassname();
    }

    JavaFeatures resolvedFeatures = getResolvedFileFeatures(JavaFeaturesProto.java_, file);

    String className =
        getDefaultFileClassName(file, resolvedFeatures.getUseOldOuterClassnameDefault());
    if (resolvedFeatures.getUseOldOuterClassnameDefault()
        && hasConflictingClassName(file, className)) {
      return className + "OuterClass";
    }
    return className;
  }

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
    return (T)
        Descriptors.getEditionDefaults(edition).getExtension(ext).toBuilder()
            .mergeFrom(file.getOptions().getFeatures().getExtension(ext))
            .build();
  }

  static String getDefaultFileClassName(
      FileDescriptorProtoOrBuilder file, boolean useOldOuterClassnameDefault) {
    String name = file.getName();
    int lastSlash = name.lastIndexOf('/');
    if (lastSlash >= 0) {
      name = name.substring(lastSlash + 1);
    }
    name = underscoresToCamelCase(stripProto(name), true);
    return useOldOuterClassnameDefault ? name : name + "Proto";
  }

  private static String stripProto(String filename) {
    if (filename.endsWith(".protodevel")) {
      return filename.substring(0, filename.length() - ".protodevel".length());
    }
    if (filename.endsWith(".proto")) {
      return filename.substring(0, filename.length() - ".proto".length());
    }

    return filename;
  }

  /** Converts underscore field names to camel case, while preserving camel case field names. */
  public static String underscoresToCamelCase(String input, boolean capitalizeNextLetter) {
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
}
