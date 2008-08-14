// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.
// http://code.google.com/p/protobuf/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package com.google.protobuf;

import protobuf_unittest.UnittestProto.TestService;
import protobuf_unittest.UnittestProto.FooRequest;
import protobuf_unittest.UnittestProto.FooResponse;
import protobuf_unittest.UnittestProto.BarRequest;
import protobuf_unittest.UnittestProto.BarResponse;

import org.easymock.classextension.EasyMock;
import org.easymock.classextension.IMocksControl;
import org.easymock.IArgumentMatcher;

import junit.framework.TestCase;

/**
 * Tests services and stubs.
 *
 * @author kenton@google.com Kenton Varda
 */
public class ServiceTest extends TestCase {
  private IMocksControl control;
  private RpcController mockController;

  private final Descriptors.MethodDescriptor fooDescriptor =
    TestService.getDescriptor().getMethods().get(0);
  private final Descriptors.MethodDescriptor barDescriptor =
    TestService.getDescriptor().getMethods().get(1);

  protected void setUp() throws Exception {
    super.setUp();
    control = EasyMock.createStrictControl();
    mockController = control.createMock(RpcController.class);
  }

  // =================================================================

  /** Tests Service.callMethod(). */
  public void testCallMethod() throws Exception {
    FooRequest fooRequest = FooRequest.newBuilder().build();
    BarRequest barRequest = BarRequest.newBuilder().build();
    MockCallback<Message> fooCallback = new MockCallback<Message>();
    MockCallback<Message> barCallback = new MockCallback<Message>();
    TestService mockService = control.createMock(TestService.class);

    mockService.foo(EasyMock.same(mockController), EasyMock.same(fooRequest),
                    this.<FooResponse>wrapsCallback(fooCallback));
    mockService.bar(EasyMock.same(mockController), EasyMock.same(barRequest),
                    this.<BarResponse>wrapsCallback(barCallback));
    control.replay();

    mockService.callMethod(fooDescriptor, mockController,
                           fooRequest, fooCallback);
    mockService.callMethod(barDescriptor, mockController,
                           barRequest, barCallback);
    control.verify();
  }

  /** Tests Service.get{Request,Response}Prototype(). */
  public void testGetPrototype() throws Exception {
    TestService mockService = control.createMock(TestService.class);

    assertSame(mockService.getRequestPrototype(fooDescriptor),
               FooRequest.getDefaultInstance());
    assertSame(mockService.getResponsePrototype(fooDescriptor),
               FooResponse.getDefaultInstance());
    assertSame(mockService.getRequestPrototype(barDescriptor),
               BarRequest.getDefaultInstance());
    assertSame(mockService.getResponsePrototype(barDescriptor),
               BarResponse.getDefaultInstance());
  }

  /** Tests generated stubs. */
  public void testStub() throws Exception {
    FooRequest fooRequest = FooRequest.newBuilder().build();
    BarRequest barRequest = BarRequest.newBuilder().build();
    MockCallback<FooResponse> fooCallback = new MockCallback<FooResponse>();
    MockCallback<BarResponse> barCallback = new MockCallback<BarResponse>();
    RpcChannel mockChannel = control.createMock(RpcChannel.class);
    TestService stub = TestService.newStub(mockChannel);

    mockChannel.callMethod(
      EasyMock.same(fooDescriptor),
      EasyMock.same(mockController),
      EasyMock.same(fooRequest),
      EasyMock.same(FooResponse.getDefaultInstance()),
      this.<Message>wrapsCallback(fooCallback));
    mockChannel.callMethod(
      EasyMock.same(barDescriptor),
      EasyMock.same(mockController),
      EasyMock.same(barRequest),
      EasyMock.same(BarResponse.getDefaultInstance()),
      this.<Message>wrapsCallback(barCallback));
    control.replay();

    stub.foo(mockController, fooRequest, fooCallback);
    stub.bar(mockController, barRequest, barCallback);
    control.verify();
  }

  // =================================================================

  /**
   * wrapsCallback() is an EasyMock argument predicate.  wrapsCallback(c)
   * matches a callback if calling that callback causes c to be called.
   * In other words, c wraps the given callback.
   */
  private <Type extends Message> RpcCallback<Type> wrapsCallback(
      MockCallback callback) {
    EasyMock.reportMatcher(new WrapsCallback(callback));
    return null;
  }

  /** The parameter to wrapsCallback() must be a MockCallback. */
  private static class MockCallback<Type extends Message>
      implements RpcCallback<Type> {
    private boolean called = false;

    public boolean isCalled() { return called; }

    public void reset() { called = false; }
    public void run(Type message) { called = true; }
  }

  /** Implementation of the wrapsCallback() argument matcher. */
  private static class WrapsCallback implements IArgumentMatcher {
    private MockCallback callback;

    public WrapsCallback(MockCallback callback) {
      this.callback = callback;
    }

    @SuppressWarnings("unchecked")
    public boolean matches(Object actual) {
      if (!(actual instanceof RpcCallback)) {
        return false;
      }
      RpcCallback actualCallback = (RpcCallback)actual;

      callback.reset();
      actualCallback.run(null);
      return callback.isCalled();
    }

    public void appendTo(StringBuffer buffer) {
      buffer.append("wrapsCallback(mockCallback)");
    }
  }
}
