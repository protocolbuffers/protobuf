# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.
# http://code.google.com/p/protobuf/
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Tests for google.protobuf.internal.service_reflection."""

__author__ = 'petar@google.com (Petar Petrov)'

import unittest
from google.protobuf import unittest_pb2
from google.protobuf import service_reflection
from google.protobuf import service


class FooUnitTest(unittest.TestCase):

  def testService(self):
    class MockRpcChannel(service.RpcChannel):
      def CallMethod(self, method, controller, request, response, callback):
        self.method = method
        self.controller = controller
        self.request = request
        callback(response)

    class MockRpcController(service.RpcController):
      def SetFailed(self, msg):
        self.failure_message = msg

    self.callback_response = None

    class MyService(unittest_pb2.TestService):
      pass

    self.callback_response = None

    def MyCallback(response):
      self.callback_response = response

    rpc_controller = MockRpcController()
    channel = MockRpcChannel()
    srvc = MyService()
    srvc.Foo(rpc_controller, unittest_pb2.FooRequest(), MyCallback)
    self.assertEqual('Method Foo not implemented.',
                     rpc_controller.failure_message)
    self.assertEqual(None, self.callback_response)

    rpc_controller.failure_message = None

    service_descriptor = unittest_pb2.TestService.DESCRIPTOR
    srvc.CallMethod(service_descriptor.methods[1], rpc_controller,
                    unittest_pb2.BarRequest(), MyCallback)
    self.assertEqual('Method Bar not implemented.',
                     rpc_controller.failure_message)
    self.assertEqual(None, self.callback_response)

  def testServiceStub(self):
    class MockRpcChannel(service.RpcChannel):
      def CallMethod(self, method, controller, request,
                     response_class, callback):
        self.method = method
        self.controller = controller
        self.request = request
        callback(response_class())

    self.callback_response = None

    def MyCallback(response):
      self.callback_response = response

    channel = MockRpcChannel()
    stub = unittest_pb2.TestService_Stub(channel)
    rpc_controller = 'controller'
    request = 'request'

    # Invoke method.
    stub.Foo(rpc_controller, request, MyCallback)

    self.assertTrue(isinstance(self.callback_response,
                               unittest_pb2.FooResponse))
    self.assertEqual(request, channel.request)
    self.assertEqual(rpc_controller, channel.controller)
    self.assertEqual(stub.GetDescriptor().methods[0], channel.method)


if __name__ == '__main__':
  unittest.main()
