package com.google.protobuf;

import com.google.protobuf.DescriptorProtos.Edition;
import com.google.protobuf.DescriptorProtos.FileDescriptorProtoOrBuilder;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FileDescriptor;
import com.google.protobuf.Descriptors.ServiceDescriptor;
import com.google.protobuf.JavaFeaturesProto.JavaFeatures;
import com.google.protobuf.Proto1FeaturesProto.Proto1Features;
import com.google.protobuf.Proto1FeaturesProto.Proto1Features.NestInFileClassFeature.NestInFileClass;

/** */
public final class GeneratedProto1Helpers {

  private GeneratedProto1Helpers() {}

  private static boolean nestInFileClass(FileDescriptor file, Proto1Features resolvedFeatures) {
    return switch (resolvedFeatures.getNestInFileClass()) {
      case NestInFileClass.YES -> true;
      case NestInFileClass.NO -> false;
      case NestInFileClass.LEGACY ->
          !file.getOptions().getJavaMultipleFiles() && file.getOptions().hasJavaOuterClassname();
      default -> throw new IllegalArgumentException("Java features are not resolved");
    };
  }

  public static boolean nestInFileClass(Descriptor descriptor) {
    return nestInFileClass(
        descriptor.getFile(), descriptor.getFeatures().getExtension(Proto1FeaturesProto.proto1));
  }

  public static boolean nestInFileClass(ServiceDescriptor descriptor) {
    return nestInFileClass(
        descriptor.getFile(), descriptor.getFeatures().getExtension(Proto1FeaturesProto.proto1));
  }

  public static String getFileClassName(FileDescriptorProtoOrBuilder file) {
    if (file.getOptions().hasJavaOuterClassnameProto1()) {
      return file.getOptions().getJavaOuterClassnameProto1();
    }
    if (file.getOptions().hasJavaOuterClassname()) {
      return file.getOptions().getJavaOuterClassname();
    }

    JavaFeatures resolvedFeatures =
        GeneratedHelpers.getResolvedFileFeatures(JavaFeaturesProto.java_, file);

    String className =
        GeneratedHelpers.getDefaultFileClassName(
            file, resolvedFeatures.getUseOldOuterClassnameDefault());
    if (resolvedFeatures.getUseOldOuterClassnameDefault()) {
      return className + "OuterClass";
    }
    return className;
  }
}
