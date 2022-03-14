// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import com.google.protobuf.Descriptors.FileDescriptor;
import com.google.protobuf.Descriptors.MethodDescriptor;
import protobuf_unittest.MessageWithNoOuter;
import protobuf_unittest.ServiceWithNoOuter;
import protobuf_unittest.UnittestProto.BarRequest;
import protobuf_unittest.UnittestProto.BarResponse;
import protobuf_unittest.UnittestProto.FooRequest;
import protobuf_unittest.UnittestProto.FooResponse;
import protobuf_unittest.UnittestProto.TestAllTypes;
import protobuf_unittest.UnittestProto.TestService;
import protobuf_unittest.no_generic_services_test.UnittestNoGenericServices;
import java.util.HashSet;
import java.util.Set;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.InOrder;
import org.mockito.Mockito;

@RunWith(JUnit4.class)
public class ServiceTest {
  private final Descriptors.MethodDescriptor fooDescriptor =
      TestService.getDescriptor().getMethods().get(0);
  private final Descriptors.MethodDescriptor barDescriptor =
      TestService.getDescriptor().getMethods().get(1);
  private static final FooRequest FOO_REQUEST = FooRequest.getDefaultInstance();
  private static final BarRequest BAR_REQUEST = BarRequest.getDefaultInstance();
  private static final RpcController MOCK_RPC_CONTROLLER = Mockito.mock(RpcController.class);
  private static final FooResponse FOO_RESPONSE = FooResponse.getDefaultInstance();
  private static final BarResponse BAR_RESPONSE = BarResponse.getDefaultInstance();
  private static final MessageWithNoOuter MESSAGE_WITH_NO_OUTER =
      MessageWithNoOuter.getDefaultInstance();

  /** Tests Service.callMethod(). */
  @Test
  @SuppressWarnings({"unchecked", "rawtypes"})
  public void testCallMethod() throws Exception {
    TestService mockService = Mockito.mock(TestService.class);
    RpcCallback mockFooRpcCallback = Mockito.mock(RpcCallback.class);
    RpcCallback mockBarRpcCallback = Mockito.mock(RpcCallback.class);
    InOrder order = Mockito.inOrder(mockService);

    mockService.callMethod(fooDescriptor, MOCK_RPC_CONTROLLER, FOO_REQUEST, mockFooRpcCallback);
    mockService.callMethod(barDescriptor, MOCK_RPC_CONTROLLER, BAR_REQUEST, mockBarRpcCallback);
    order
        .verify(mockService)
        .foo(
            Mockito.same(MOCK_RPC_CONTROLLER),
            Mockito.same(FOO_REQUEST),
            Mockito.same(mockFooRpcCallback));
    order
        .verify(mockService)
        .bar(
            Mockito.same(MOCK_RPC_CONTROLLER),
            Mockito.same(BAR_REQUEST),
            Mockito.same(mockBarRpcCallback));
  }

  /** Tests Service.get{Request,Response}Prototype(). */
  @Test
  public void testGetPrototype() throws Exception {
    Descriptors.MethodDescriptor fooDescriptor = TestService.getDescriptor().getMethods().get(0);
    Descriptors.MethodDescriptor barDescriptor = TestService.getDescriptor().getMethods().get(1);
    TestService mockService = Mockito.mock(TestService.class);

    assertThat(mockService.getRequestPrototype(fooDescriptor)).isSameInstanceAs(FOO_REQUEST);
    assertThat(mockService.getResponsePrototype(fooDescriptor)).isSameInstanceAs(FOO_RESPONSE);
    assertThat(mockService.getRequestPrototype(barDescriptor)).isSameInstanceAs(BAR_REQUEST);
    assertThat(mockService.getResponsePrototype(barDescriptor)).isSameInstanceAs(BAR_RESPONSE);
  }

  /** Tests generated stubs. */
  @Test
  @SuppressWarnings({"unchecked", "rawtypes"})
  public void testStub() throws Exception {
    RpcCallback mockFooRpcCallback = Mockito.mock(RpcCallback.class);
    RpcCallback mockBarRpcCallback = Mockito.mock(RpcCallback.class);
    RpcChannel mockRpcChannel = Mockito.mock(RpcChannel.class);
    InOrder order = Mockito.inOrder(mockRpcChannel);
    TestService stub = TestService.newStub(mockRpcChannel);

    stub.foo(MOCK_RPC_CONTROLLER, FOO_REQUEST, mockFooRpcCallback);
    stub.bar(MOCK_RPC_CONTROLLER, BAR_REQUEST, mockBarRpcCallback);

    order
        .verify(mockRpcChannel)
        .callMethod(
            Mockito.same(fooDescriptor),
            Mockito.same(MOCK_RPC_CONTROLLER),
            Mockito.same(FOO_REQUEST),
            Mockito.same(FOO_RESPONSE),
            Mockito.any(RpcCallback.class));
    order
        .verify(mockRpcChannel)
        .callMethod(
            Mockito.same(barDescriptor),
            Mockito.same(MOCK_RPC_CONTROLLER),
            Mockito.same(BAR_REQUEST),
            Mockito.same(BAR_RESPONSE),
            Mockito.any(RpcCallback.class));
  }

  /** Tests generated blocking stubs. */
  @Test
  public void testBlockingStub() throws Exception {
    BlockingRpcChannel mockBlockingRpcChannel = Mockito.mock(BlockingRpcChannel.class);
    TestService.BlockingInterface stub = TestService.newBlockingStub(mockBlockingRpcChannel);

    when(mockBlockingRpcChannel.callBlockingMethod(
            Mockito.same(fooDescriptor),
            Mockito.same(MOCK_RPC_CONTROLLER),
            Mockito.same(FOO_REQUEST),
            Mockito.same(FOO_RESPONSE)))
        .thenReturn(FOO_RESPONSE);
    when(mockBlockingRpcChannel.callBlockingMethod(
            Mockito.same(barDescriptor),
            Mockito.same(MOCK_RPC_CONTROLLER),
            Mockito.same(BAR_REQUEST),
            Mockito.same(BAR_RESPONSE)))
        .thenReturn(BAR_RESPONSE);

    assertThat(FOO_RESPONSE).isSameInstanceAs(stub.foo(MOCK_RPC_CONTROLLER, FOO_REQUEST));
    assertThat(BAR_RESPONSE).isSameInstanceAs(stub.bar(MOCK_RPC_CONTROLLER, BAR_REQUEST));
  }

  @Test
  public void testNewReflectiveService() {
    ServiceWithNoOuter.Interface impl = Mockito.mock(ServiceWithNoOuter.Interface.class);
    Service service = ServiceWithNoOuter.newReflectiveService(impl);

    MethodDescriptor fooMethod = ServiceWithNoOuter.getDescriptor().findMethodByName("Foo");
    RpcCallback<Message> callback =
        new RpcCallback<Message>() {
          @Override
          public void run(Message parameter) {
            // No reason this should be run.
            assertWithMessage("should not run").fail();
          }
        };
    RpcCallback<TestAllTypes> specializedCallback = RpcUtil.specializeCallback(callback);

    service.callMethod(fooMethod, MOCK_RPC_CONTROLLER, MESSAGE_WITH_NO_OUTER, callback);
    verify(impl)
        .foo(
            Mockito.same(MOCK_RPC_CONTROLLER),
            Mockito.same(MESSAGE_WITH_NO_OUTER),
            Mockito.same(specializedCallback));
  }

  @Test
  public void testNewReflectiveBlockingService() throws ServiceException {
    ServiceWithNoOuter.BlockingInterface impl =
        Mockito.mock(ServiceWithNoOuter.BlockingInterface.class);

    BlockingService service = ServiceWithNoOuter.newReflectiveBlockingService(impl);

    MethodDescriptor fooMethod = ServiceWithNoOuter.getDescriptor().findMethodByName("Foo");

    TestAllTypes expectedResponse = TestAllTypes.getDefaultInstance();

    when(impl.foo(Mockito.same(MOCK_RPC_CONTROLLER), Mockito.same(MESSAGE_WITH_NO_OUTER)))
        .thenReturn(expectedResponse);
    Message response =
        service.callBlockingMethod(fooMethod, MOCK_RPC_CONTROLLER, MESSAGE_WITH_NO_OUTER);
    assertThat(response).isEqualTo(expectedResponse);
  }

  @Test
  public void testNoGenericServices() throws Exception {
    // Non-services should be usable.
    UnittestNoGenericServices.TestMessage message =
        UnittestNoGenericServices.TestMessage.newBuilder()
            .setA(123)
            .setExtension(UnittestNoGenericServices.testExtension, 456)
            .build();
    assertThat(message.getA()).isEqualTo(123);
    assertThat(UnittestNoGenericServices.TestEnum.FOO.getNumber()).isEqualTo(1);

    // Build a list of the class names nested in UnittestNoGenericServices.
    String outerName =
        "protobuf_unittest.no_generic_services_test.UnittestNoGenericServices";
    Class<?> outerClass = Class.forName(outerName);

    Set<String> innerClassNames = new HashSet<>();
    for (Class<?> innerClass : outerClass.getClasses()) {
      String fullName = innerClass.getName();
      // Figure out the unqualified name of the inner class.
      // Note:  Surprisingly, the full name of an inner class will be separated
      //   from the outer class name by a '$' rather than a '.'.  This is not
      //   mentioned in the documentation for java.lang.Class.  I don't want to
      //   make assumptions, so I'm just going to accept any character as the
      //   separator.
      assertThat(fullName).startsWith(outerName);

      if (!Service.class.isAssignableFrom(innerClass)
          && !Message.class.isAssignableFrom(innerClass)
          && !ProtocolMessageEnum.class.isAssignableFrom(innerClass)) {
        // Ignore any classes not generated by the base code generator.
        continue;
      }

      innerClassNames.add(fullName.substring(outerName.length() + 1));
    }

    // No service class should have been generated.
    assertThat(innerClassNames).contains("TestMessage");
    assertThat(innerClassNames).contains("TestEnum");
    assertThat(innerClassNames).doesNotContain("TestService");

    // But descriptors are there.
    FileDescriptor file = UnittestNoGenericServices.getDescriptor();
    assertThat(file.getServices()).hasSize(1);
    assertThat(file.getServices().get(0).getName()).isEqualTo("TestService");
    assertThat(file.getServices().get(0).getMethods()).hasSize(1);
    assertThat(file.getServices().get(0).getMethods().get(0).getName()).isEqualTo("Foo");
  }
}
