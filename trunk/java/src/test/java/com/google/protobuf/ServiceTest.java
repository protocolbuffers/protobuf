// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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
