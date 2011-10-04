#region Copyright notice and license

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://github.com/jskeet/dotnet-protobufs/
// Original C++/Java/Python code:
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

#endregion

using System;
using Google.ProtocolBuffers.Descriptors;
using Google.ProtocolBuffers.TestProtos;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace Google.ProtocolBuffers
{
    /// <summary>
    /// Tests for generated service classes.
    /// TODO(jonskeet): Convert the mocking tests using Rhino.Mocks.
    /// </summary>
    [TestClass]
    public class ServiceTest
    {
        private delegate void Action<T1, T2>(T1 t1, T2 t2);

        private static readonly MethodDescriptor FooDescriptor = TestGenericService.Descriptor.Methods[0];
        private static readonly MethodDescriptor BarDescriptor = TestGenericService.Descriptor.Methods[1];

        [TestMethod]
        public void GetRequestPrototype()
        {
            TestGenericService service = new TestServiceImpl();

            Assert.AreSame(service.GetRequestPrototype(FooDescriptor), FooRequest.DefaultInstance);
            Assert.AreSame(service.GetRequestPrototype(BarDescriptor), BarRequest.DefaultInstance);
        }

        [TestMethod]
        public void GetResponsePrototype()
        {
            TestGenericService service = new TestServiceImpl();

            Assert.AreSame(service.GetResponsePrototype(FooDescriptor), FooResponse.DefaultInstance);
            Assert.AreSame(service.GetResponsePrototype(BarDescriptor), BarResponse.DefaultInstance);
        }

        [TestMethod]
        public void CallMethodFoo()
        {
            FooRequest fooRequest = FooRequest.CreateBuilder().Build();
            FooResponse fooResponse = FooResponse.CreateBuilder().Build();
            IRpcController controller = new RpcTestController();

            bool fooCalled = false;

            TestGenericService service = new TestServiceImpl((request, responseAction) =>
                                                                 {
                                                                     Assert.AreSame(fooRequest, request);
                                                                     fooCalled = true;
                                                                     responseAction(fooResponse);
                                                                 }, null, controller);

            bool doneHandlerCalled = false;
            Action<IMessage> doneHandler = (response =>
                                                {
                                                    Assert.AreSame(fooResponse, response);
                                                    doneHandlerCalled = true;
                                                });

            service.CallMethod(FooDescriptor, controller, fooRequest, doneHandler);

            Assert.IsTrue(doneHandlerCalled);
            Assert.IsTrue(fooCalled);
        }

        [TestMethod]
        public void CallMethodBar()
        {
            BarRequest barRequest = BarRequest.CreateBuilder().Build();
            BarResponse barResponse = BarResponse.CreateBuilder().Build();
            IRpcController controller = new RpcTestController();

            bool barCalled = false;

            TestGenericService service = new TestServiceImpl(null, (request, responseAction) =>
                                                                       {
                                                                           Assert.AreSame(barRequest, request);
                                                                           barCalled = true;
                                                                           responseAction(barResponse);
                                                                       }, controller);

            bool doneHandlerCalled = false;
            Action<IMessage> doneHandler = (response =>
                                                {
                                                    Assert.AreSame(barResponse, response);
                                                    doneHandlerCalled = true;
                                                });

            service.CallMethod(BarDescriptor, controller, barRequest, doneHandler);

            Assert.IsTrue(doneHandlerCalled);
            Assert.IsTrue(barCalled);
        }

        [TestMethod]
        public void GeneratedStubFooCall()
        {
            IRpcChannel channel = new RpcTestChannel();
            IRpcController controller = new RpcTestController();
            TestGenericService service = TestGenericService.CreateStub(channel);
            FooResponse fooResponse = null;
            Action<FooResponse> doneHandler = r => fooResponse = r;

            service.Foo(controller, FooRequest.DefaultInstance, doneHandler);

            Assert.IsNotNull(fooResponse);
            Assert.IsFalse(controller.Failed);
        }

        [TestMethod]
        public void GeneratedStubBarCallFails()
        {
            IRpcChannel channel = new RpcTestChannel();
            IRpcController controller = new RpcTestController();
            TestGenericService service = TestGenericService.CreateStub(channel);
            BarResponse barResponse = null;
            Action<BarResponse> doneHandler = r => barResponse = r;

            service.Bar(controller, BarRequest.DefaultInstance, doneHandler);

            Assert.IsNull(barResponse);
            Assert.IsTrue(controller.Failed);
        }

        #region RpcTestController
        private class RpcTestController : IRpcController
        {
            public string TestFailedReason { get; set; }
            public bool TestCancelled { get; set; }
            public Action<object> TestCancelledCallback { get; set; }

            void IRpcController.Reset()
            {
                TestFailedReason = null;
                TestCancelled = false;
                TestCancelledCallback = null;
            }

            bool IRpcController.Failed
            {
                get { return TestFailedReason != null; }
            }

            string IRpcController.ErrorText
            {
                get { return TestFailedReason; }
            }

            void IRpcController.StartCancel()
            {
                TestCancelled = true;
                if (TestCancelledCallback != null)
                    TestCancelledCallback(this);
            }

            void IRpcController.SetFailed(string reason)
            {
                TestFailedReason = reason;
            }

            bool IRpcController.IsCanceled()
            {
                return TestCancelled;
            }

            void IRpcController.NotifyOnCancel(Action<object> callback)
            {
                TestCancelledCallback = callback;
            }
        }
        #endregion
        #region RpcTestChannel
        private class RpcTestChannel : IRpcChannel
        {
            public MethodDescriptor TestMethodCalled { get; set; }

            void IRpcChannel.CallMethod(MethodDescriptor method, IRpcController controller, IMessage request, IMessage responsePrototype, Action<IMessage> done)
            {
                TestMethodCalled = method;
                try
                {
                    done(FooResponse.DefaultInstance);
                }
                catch (Exception e)
                {
                    controller.SetFailed(e.Message);
                }
            }
        }
        #endregion
        #region TestServiceImpl
        private class TestServiceImpl : TestGenericService
        {
            private readonly Action<FooRequest, Action<FooResponse>> fooHandler;
            private readonly Action<BarRequest, Action<BarResponse>> barHandler;
            private readonly IRpcController expectedController;

            internal TestServiceImpl()
            {
            }

            internal TestServiceImpl(Action<FooRequest, Action<FooResponse>> fooHandler,
                                     Action<BarRequest, Action<BarResponse>> barHandler,
                                     IRpcController expectedController)
            {
                this.fooHandler = fooHandler;
                this.barHandler = barHandler;
                this.expectedController = expectedController;
            }

            public override void Foo(IRpcController controller, FooRequest request, Action<FooResponse> done)
            {
                Assert.AreSame(expectedController, controller);
                fooHandler(request, done);
            }

            public override void Bar(IRpcController controller, BarRequest request, Action<BarResponse> done)
            {
                Assert.AreSame(expectedController, controller);
                barHandler(request, done);
            }
        }
        #endregion
    }
}