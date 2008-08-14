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
using System;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers {
  /// <summary>
  /// Interface for an RPC channel. A channel represents a communication line to
  /// a service (IService implementation) which can be used to call that service's
  /// methods. The service may be running on another machine. Normally, you should
  /// not call an IRpcChannel directly, but instead construct a stub wrapping it.
  /// Generated service classes contain a CreateStub method for precisely this purpose.
  /// </summary>
  public interface IRpcChannel {
    /// <summary>
    /// Calls the given method of the remote service. This method is similar
    /// to <see cref="IService.CallMethod" /> with one important difference: the
    /// caller decides the types of the IMessage objects, not the implementation.
    /// The request may be of any type as long as <c>request.Descriptor == method.InputType</c>.
    /// The response passed to the callback will be of the same type as
    /// <paramref name="responsePrototype"/> (which must be such that
    /// <c>responsePrototype.Descriptor == method.OutputType</c>).
    /// </summary>
    void CallMethod(MethodDescriptor method, IRpcController controller,
        IMessage request, IMessage responsePrototype, Action<IMessage> done);
  }
}
