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
