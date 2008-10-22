using System;
using System.Collections.Generic;
using System.Text;

namespace Google.ProtocolBuffers.ProtoGen {
  /// <summary>
  /// Exception thrown when dependencies within a descriptor set can't be resolved.
  /// </summary>
  public sealed class DependencyResolutionException : Exception {
    public DependencyResolutionException(string message) : base(message) {
    }

    public DependencyResolutionException(string format, params object[] args) 
        : base(string.Format(format, args)) {
    }
  }
}
