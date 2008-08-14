using System;
using System.Collections.Generic;
using System.Text;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers {
  /// <summary>
  /// TODO(jonskeet): Do this properly.
  /// </summary>
  public interface IRpcChannel {
    void CallMethod<T>(MethodDescriptor method, IRpcController controller,
        IMessage request, IMessage responsePrototype, Action<T> done);
  }
}
