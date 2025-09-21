package com.google.protobuf;

import com.google.protobuf.DescriptorProtos.Edition;
import com.google.protobuf.DescriptorProtos.FileDescriptorProtoOrBuilder;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.EnumDescriptor;
import com.google.protobuf.Descriptors.ServiceDescriptor;
import com.google.protobuf.JavaMutableFeaturesProto.JavaMutableFeatures;
import com.google.protobuf.JavaMutableFeaturesProto.JavaMutableFeatures.NestInFileClassFeature.NestInFileClass;

/** */
public final class GeneratedMutableHelpers {

  private GeneratedMutableHelpers() {}

  private static boolean nestInFileClass(
      FileDescriptorProtoOrBuilder file, JavaMutableFeatures resolvedFeatures) {
    return switch (resolvedFeatures.getNestInFileClass()) {
      case NestInFileClass.YES -> true;
      case NestInFileClass.NO -> false;
      case NestInFileClass.LEGACY ->
          !file.getOptions().getJavaMultipleFiles()
              || !file.getOptions().hasJavaMultipleFilesMutablePackage();
      default -> throw new IllegalArgumentException("Java features are not resolved");
    };
  }

  public static boolean nestInFileClass(Descriptor descriptor) {
    return nestInFileClass(
        descriptor.getFile().toProto(),
        descriptor.getFeatures().getExtension(JavaMutableFeaturesProto.javaMutable));
  }

  public static boolean nestInFileClass(EnumDescriptor descriptor) {
    return nestInFileClass(
        descriptor.getFile().toProto(),
        descriptor.getFeatures().getExtension(JavaMutableFeaturesProto.javaMutable));
  }

  public static boolean nestInFileClass(ServiceDescriptor descriptor) {
    return nestInFileClass(
        descriptor.getFile().toProto(),
        descriptor.getFeatures().getExtension(JavaMutableFeaturesProto.javaMutable));
  }

  public static boolean nestInFileClass(FileDescriptorProtoOrBuilder file) {
    return nestInFileClass(
        file, GeneratedHelpers.getResolvedFileFeatures(JavaMutableFeaturesProto.javaMutable, file));
  }

  public static String getFileClassName(FileDescriptorProtoOrBuilder file) {
    return "Mutable" + GeneratedHelpers.getFileClassName(file);
  }
}
