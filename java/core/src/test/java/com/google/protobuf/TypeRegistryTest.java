// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import com.google.protobuf.DescriptorProtos.DescriptorProto;
import com.google.protobuf.DescriptorProtos.FileDescriptorProto;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.DescriptorValidationException;
import com.google.protobuf.Descriptors.FileDescriptor;
import proto2_unittest.UnittestProto;
import java.util.concurrent.Callable;
import java.util.concurrent.FutureTask;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public final class TypeRegistryTest {

  @Test
  public void getDescriptorForTypeUrl_throwsExceptionForUnknownTypes() throws Exception {
    assertThrows(
        InvalidProtocolBufferException.class,
        () -> TypeRegistry.getEmptyTypeRegistry().getDescriptorForTypeUrl("UnknownType"));
    assertThrows(
        InvalidProtocolBufferException.class,
        () -> TypeRegistry.getEmptyTypeRegistry().getDescriptorForTypeUrl("///"));
  }

  @Test
  public void findDescriptorByFullName() throws Exception {
    Descriptor descriptor = UnittestProto.TestAllTypes.getDescriptor();
    assertThat(TypeRegistry.getEmptyTypeRegistry().find(descriptor.getFullName())).isNull();

    assertThat(TypeRegistry.newBuilder().add(descriptor).build().find(descriptor.getFullName()))
        .isSameInstanceAs(descriptor);
  }

  @Test
  public void findDescriptorByTypeUrl() throws Exception {
    Descriptor descriptor = UnittestProto.TestAllTypes.getDescriptor();
    assertThat(
            TypeRegistry.getEmptyTypeRegistry()
                .getDescriptorForTypeUrl("type.googleapis.com/" + descriptor.getFullName()))
        .isNull();

    assertThat(
            TypeRegistry.newBuilder()
                .add(descriptor)
                .build()
                .getDescriptorForTypeUrl("type.googleapis.com/" + descriptor.getFullName()))
        .isSameInstanceAs(descriptor);
  }

  @Test
  public void add_deepImportChain_addsTypesDepthFirst() throws Exception {
    // first.proto, second.proto, and top.proto all define test.Foo. top.proto imports a long chain
    // of files that ends with first.proto, and then second.proto. Files are added depth-first in
    // import order, each after the files it imports, so first.proto's test.Foo is the one kept.
    FileDescriptor first = buildFile("first.proto", "Foo");
    FileDescriptor file = first;
    for (int i = 0; i < 10000; i++) {
      file = buildFile("import_" + i + ".proto", "Import" + i, file);
    }
    Descriptor top =
        buildFile("top.proto", "Foo", file, buildFile("second.proto", "Foo"))
            .findMessageTypeByName("Foo");
    TypeRegistry.Builder builder = TypeRegistry.newBuilder();

    TypeRegistry registry = startWithSmallStack(() -> builder.add(top).build()).get();

    assertThat(registry.find("test.Foo")).isSameInstanceAs(first.findMessageTypeByName("Foo"));
  }

  /** Builds a file in package "test" that defines one message and imports {@code dependencies}. */
  private static FileDescriptor buildFile(
      String name, String messageName, FileDescriptor... dependencies)
      throws DescriptorValidationException {
    FileDescriptorProto.Builder proto =
        FileDescriptorProto.newBuilder()
            .setName(name)
            .setPackage("test")
            .addMessageType(DescriptorProto.newBuilder().setName(messageName));
    for (FileDescriptor dependency : dependencies) {
      proto.addDependency(dependency.getName());
    }
    return FileDescriptor.buildFrom(proto.build(), dependencies);
  }

  /**
   * Starts {@code callable} on a thread that requests a small stack, which recursion over a long
   * import chain would overflow. The JVM may round the size up to a platform minimum.
   */
  private static <T> FutureTask<T> startWithSmallStack(Callable<T> callable) {
    FutureTask<T> task = new FutureTask<>(callable);
    new Thread(null, task, "small-stack", 128 * 1024).start();
    return task;
  }
}
