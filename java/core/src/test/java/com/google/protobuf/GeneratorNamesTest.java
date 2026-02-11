package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;

import com.google.common.collect.ImmutableList;
import com.google.protobuf.DescriptorProtos.DescriptorProto;
import com.google.protobuf.DescriptorProtos.Edition;
import com.google.protobuf.DescriptorProtos.FeatureSet;
import com.google.protobuf.DescriptorProtos.FileDescriptorProto;
import com.google.protobuf.DescriptorProtos.FileOptions;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.EnumDescriptor;
import com.google.protobuf.Descriptors.FileDescriptor;
import com.google.protobuf.Descriptors.ServiceDescriptor;
import com.google.protobuf.JavaFeaturesProto.JavaFeatures;
import protobuf.edition2024.defaults.Edition2024DefaultsEnum;
import protobuf.edition2024.defaults.Edition2024DefaultsMessage;
import protobuf.edition2024.defaults.Edition2024DefaultsService;
import protobuf.edition2024.defaults.GeneratorNamesEdition2024DefaultsProto;
import com.google.testing.junit.testparameterinjector.TestParameter;
import com.google.testing.junit.testparameterinjector.TestParameterInjector;
import com.google.testing.junit.testparameterinjector.TestParameterValuesProvider;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(TestParameterInjector.class)
public final class GeneratorNamesTest {
  private static final class FileClassProvider extends TestParameterValuesProvider {
    @Override
    public ImmutableList<FileParameter> provideValues(TestParameterValuesProvider.Context context) {
      return ImmutableList.of(
          new FileParameter(
              GeneratorNamesPre2024Defaults.getDescriptor(), GeneratorNamesPre2024Defaults.class),
          new FileParameter(
              GeneratorNamesEdition2024DefaultsProto.getDescriptor(),
              GeneratorNamesEdition2024DefaultsProto.class),
          new FileParameter(
              GeneratorNamesConflictingEnumOuterClass.getDescriptor(),
              GeneratorNamesConflictingEnumOuterClass.class),
          new FileParameter(
              GeneratorNamesConflictingNestedEnumOuterClass.getDescriptor(),
              GeneratorNamesConflictingNestedEnumOuterClass.class),
          new FileParameter(
              GeneratorNamesConflictingMessageOuterClass.getDescriptor(),
              GeneratorNamesConflictingMessageOuterClass.class),
          new FileParameter(
              GeneratorNamesConflictingNestedMessageOuterClass.getDescriptor(),
              GeneratorNamesConflictingNestedMessageOuterClass.class),
          new FileParameter(
              GeneratorNamesConflictingServiceOuterClass.getDescriptor(),
              GeneratorNamesConflictingServiceOuterClass.class),
          new FileParameter(
              GeneratorNamesMultipleFilesProto2.getDescriptor(),
              GeneratorNamesMultipleFilesProto2.class),
          new FileParameter(
              GeneratorNamesMultipleFilesProto3.getDescriptor(),
              GeneratorNamesMultipleFilesProto3.class),
          new FileParameter(CustomOuterClassname.getDescriptor(), CustomOuterClassname.class),
          new FileParameter(
              GeneratorNamesUnusualFile0Name.getDescriptor(),
              GeneratorNamesUnusualFile0Name.class));
    }
  }

  @Test
  public void getFileClassName(
      @TestParameter(valuesProvider = FileClassProvider.class) FileParameter parameter) {
    assertThat(GeneratorNames.getFileClassName(parameter.descriptor))
        .isEqualTo(parameter.clazz.getSimpleName());
  }

  @Test
  public void getFileClassName_proto(
      @TestParameter(valuesProvider = FileClassProvider.class) FileParameter parameter) {
    assertThat(GeneratorNames.getFileClassName(parameter.descriptor.toProto()))
        .isEqualTo(parameter.clazz.getSimpleName());
  }

  @Test
  public void getFileJavaPackage(
      @TestParameter(valuesProvider = FileClassProvider.class) FileParameter parameter) {
    assertThat(GeneratorNames.getFileJavaPackage(parameter.descriptor))
        .isEqualTo(parameter.clazz.getPackage().getName());
  }

  @Test
  public void getFileJavaPackage_proto(
      @TestParameter(valuesProvider = FileClassProvider.class) FileParameter parameter) {
    assertThat(GeneratorNames.getFileJavaPackage(parameter.descriptor.toProto()))
        .isEqualTo(parameter.clazz.getPackage().getName());
  }

  private static final class MessageClassNameProvider extends TestParameterValuesProvider {
    @Override
    public ImmutableList<MessageParameter> provideValues(
        TestParameterValuesProvider.Context context) {
      return ImmutableList.of(
          new MessageParameter(
              GeneratorNamesPre2024Defaults.TopLevelMessage.getDescriptor(),
              GeneratorNamesPre2024Defaults.TopLevelMessage.class),
          new MessageParameter(
              GeneratorNamesPre2024Defaults.TopLevelMessage.NestedMessage.getDescriptor(),
              GeneratorNamesPre2024Defaults.TopLevelMessage.NestedMessage.class),
          new MessageParameter(
              Edition2024DefaultsMessage.getDescriptor(), Edition2024DefaultsMessage.class),
          new MessageParameter(
              Edition2024DefaultsMessage.NestedMessage.getDescriptor(),
              Edition2024DefaultsMessage.NestedMessage.class),
          new MessageParameter(
              MultipleFilesProto2Message.getDescriptor(), MultipleFilesProto2Message.class),
          new MessageParameter(
              GeneratorNamesNestInFileClassProto.NestInFileClassMessage.getDescriptor(),
              GeneratorNamesNestInFileClassProto.NestInFileClassMessage.class));
    }
  }

  @Test
  public void getBytecodeClassName_message(
      @TestParameter(valuesProvider = MessageClassNameProvider.class) MessageParameter parameter) {
    assertThat(GeneratorNames.getBytecodeClassName(parameter.descriptor))
        .isEqualTo(parameter.clazz.getName());
  }

  @Test
  public void getQualifiedClassName_message(
      @TestParameter(valuesProvider = MessageClassNameProvider.class) MessageParameter parameter) {
    assertThat(GeneratorNames.getQualifiedClassName(parameter.descriptor))
        .isEqualTo(parameter.clazz.getCanonicalName());
  }

  private static final class EnumClassNameProvider extends TestParameterValuesProvider {
    @Override
    public ImmutableList<EnumParameter> provideValues(TestParameterValuesProvider.Context context) {
      return ImmutableList.of(
          new EnumParameter(
              GeneratorNamesPre2024Defaults.TopLevelEnum.getDescriptor(),
              GeneratorNamesPre2024Defaults.TopLevelEnum.class),
          new EnumParameter(
              GeneratorNamesPre2024Defaults.TopLevelMessage.NestedEnum.getDescriptor(),
              GeneratorNamesPre2024Defaults.TopLevelMessage.NestedEnum.class),
          new EnumParameter(Edition2024DefaultsEnum.getDescriptor(), Edition2024DefaultsEnum.class),
          new EnumParameter(
              Edition2024DefaultsMessage.NestedEnum.getDescriptor(),
              Edition2024DefaultsMessage.NestedEnum.class),
          new EnumParameter(MultipleFilesProto2Enum.getDescriptor(), MultipleFilesProto2Enum.class),
          new EnumParameter(
              GeneratorNamesNestInFileClassProto.NestInFileClassEnum.getDescriptor(),
              GeneratorNamesNestInFileClassProto.NestInFileClassEnum.class));
    }
  }

  @Test
  public void getBytecodeClassName_enum(
      @TestParameter(valuesProvider = EnumClassNameProvider.class) EnumParameter parameter) {
    assertThat(GeneratorNames.getBytecodeClassName(parameter.descriptor))
        .isEqualTo(parameter.clazz.getName());
  }

  @Test
  public void getQualifiedClassName_enum(
      @TestParameter(valuesProvider = EnumClassNameProvider.class) EnumParameter parameter) {
    assertThat(GeneratorNames.getQualifiedClassName(parameter.descriptor))
        .isEqualTo(parameter.clazz.getCanonicalName());
  }

  private static final class ServiceClassNameProvider extends TestParameterValuesProvider {
    @Override
    public ImmutableList<ServiceParameter> provideValues(
        TestParameterValuesProvider.Context context) {
      return ImmutableList.of(
          new ServiceParameter(
              Edition2024DefaultsService.getDescriptor(),
          Edition2024DefaultsService.class),
          new ServiceParameter(
              MultipleFilesProto2Service.getDescriptor(),
          MultipleFilesProto2Service.class),
          new ServiceParameter(
              GeneratorNamesPre2024Defaults.Pre2024DefaultsService.getDescriptor(),
          GeneratorNamesPre2024Defaults.Pre2024DefaultsService.class),
          new ServiceParameter(
              GeneratorNamesGenericServices.TestGenericService.getDescriptor(),
          GeneratorNamesGenericServices.TestGenericService.class),
          new ServiceParameter(
              GeneratorNamesNestInFileClassProto.NestInFileClassService.getDescriptor(),
          GeneratorNamesNestInFileClassProto.NestInFileClassService.class)
          );
    }
  }

  @Test
  public void getBytecodeClassName_service(
      @TestParameter(valuesProvider = ServiceClassNameProvider.class) ServiceParameter parameter) {
    assertThat(GeneratorNames.getBytecodeClassName(parameter.descriptor))
        .isEqualTo(parameter.clazz.getName());
  }

  @Test
  public void getQualifiedClassName_service(
      @TestParameter(valuesProvider = ServiceClassNameProvider.class) ServiceParameter parameter) {
    assertThat(GeneratorNames.getQualifiedClassName(parameter.descriptor))
        .isEqualTo(parameter.clazz.getCanonicalName());
  }

  @Test
  public void joinPackage() {
    assertThat(GeneratorNames.joinPackage("com.google.foo", "Bar")).isEqualTo("com.google.foo.Bar");
    assertThat(GeneratorNames.joinPackage("com.google.foo", "")).isEqualTo("com.google.foo");
    assertThat(GeneratorNames.joinPackage("", "Bar")).isEqualTo("Bar");
    assertThat(GeneratorNames.joinPackage("", "")).isEmpty();
  }

  @Test
  public void underscoresToCamelCase() {
    assertThat(GeneratorNames.underscoresToCamelCase("")).isEmpty();
    assertThat(GeneratorNames.underscoresToCamelCase("f")).isEqualTo("F");
    assertThat(GeneratorNames.underscoresToCamelCase("f", false)).isEqualTo("f");
    assertThat(GeneratorNames.underscoresToCamelCase("foo_bar")).isEqualTo("FooBar");
    assertThat(GeneratorNames.underscoresToCamelCase("foo_bar", false)).isEqualTo("fooBar");
    assertThat(GeneratorNames.underscoresToCamelCase("fooBar", true)).isEqualTo("FooBar");
    assertThat(GeneratorNames.underscoresToCamelCase("fooBar", false)).isEqualTo("fooBar");
    assertThat(GeneratorNames.underscoresToCamelCase("foo12bar", false)).isEqualTo("foo12Bar");
    assertThat(GeneratorNames.underscoresToCamelCase("FooBar", false)).isEqualTo("fooBar");
    assertThat(GeneratorNames.underscoresToCamelCase("FooBar", true)).isEqualTo("FooBar");
    assertThat(GeneratorNames.underscoresToCamelCase("FOO_BAR", true)).isEqualTo("FOOBAR");
    assertThat(GeneratorNames.underscoresToCamelCase("`a{b@c[d", false)).isEqualTo("ABCD");
  }

  @Test
  public void getFileClassName_weirdExtension() {
    // This isn't exercisable in blazel because we require .proto extensions.
    assertThat(
            GeneratorNames.getFileClassName(
                FileDescriptorProto.newBuilder().setName("foo/bar.notproto").build()))
        .isEqualTo("BarNotproto");
  }

  @Test
  public void getFileClassName_noDirectory() {
    // This isn't exercisable in blazel because all our tests are in a directory.
    assertThat(
            GeneratorNames.getFileClassName(
                FileDescriptorProto.newBuilder().setName("bar.proto").build()))
        .isEqualTo("Bar");
  }

  @Test
  public void getFileClassName_conflictingName2024() {
    // This isn't exercisable in blazel because conflicts trigger a protoc error in edition 2024.
    FileDescriptorProto file =
        FileDescriptorProto.newBuilder()
            .setEdition(Edition.EDITION_2024)
            .setName("foo/foo.proto")
            .addMessageType(DescriptorProto.newBuilder().setName("FooProto").build())
            .build();
    assertThat(GeneratorNames.getFileClassName(file)).isEqualTo("FooProto");
  }

  @Test
  public void getResolvedFileFeatures_unparsedExtensions() throws Exception {
    FileDescriptorProto file =
        FileDescriptorProto.newBuilder()
            .setEdition(Edition.EDITION_2024)
            .setOptions(
                FileOptions.newBuilder()
                    .setFeatures(
                        FeatureSet.newBuilder()
                            .setExtension(
                                JavaFeaturesProto.java_,
                                JavaFeatures.newBuilder().setLargeEnum(true).build())
                            .build())
                    .build())
            .build();
    file = FileDescriptorProto.parseFrom(file.toByteString(), ExtensionRegistry.getEmptyRegistry());
    assertThat(GeneratorNames.getResolvedFileFeatures(JavaFeaturesProto.java_, file).getLargeEnum())
        .isTrue();
  }

  // Helper classes to contain parameterized test data.

  private static String getShortFilename(FileDescriptor descriptor) {
    return descriptor.getName().substring(descriptor.getName().lastIndexOf('/') + 1);
  }

  private static final class FileParameter {
    final FileDescriptor descriptor;
    final Class<?> clazz;

    @Override
    public String toString() {
      return getShortFilename(descriptor);
    }

    FileParameter(FileDescriptor descriptor, Class<?> clazz) {
      this.descriptor = descriptor;
      this.clazz = clazz;
    }
  }

  private static final class MessageParameter {
    final Descriptor descriptor;
    final Class<?> clazz;

    @Override
    public String toString() {
      return getShortFilename(descriptor.getFile()) + ":" + descriptor.getName();
    }

    MessageParameter(Descriptor descriptor, Class<?> clazz) {
      this.descriptor = descriptor;
      this.clazz = clazz;
    }
  }

  private static final class EnumParameter {
    final EnumDescriptor descriptor;
    final Class<?> clazz;

    @Override
    public String toString() {
      return getShortFilename(descriptor.getFile()) + ":" + descriptor.getName();
    }

    EnumParameter(EnumDescriptor descriptor, Class<?> clazz) {
      this.descriptor = descriptor;
      this.clazz = clazz;
    }
  }

  private static final class ServiceParameter {
    final ServiceDescriptor descriptor;
    final Class<?> clazz;

    @Override
    public String toString() {
      return getShortFilename(descriptor.getFile()) + ":" + descriptor.getName();
    }

    ServiceParameter(ServiceDescriptor descriptor, Class<?> clazz) {
      this.descriptor = descriptor;
      this.clazz = clazz;
    }
  }
}
