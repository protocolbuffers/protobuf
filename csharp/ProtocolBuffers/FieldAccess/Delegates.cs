using System;
using System.Collections.Generic;
using System.Text;

namespace Google.ProtocolBuffers.FieldAccess {
  /// <summary>
  /// Declarations of delegate types used for field access. Can't
  /// use Func and Action (other than one parameter) as we can't guarantee .NET 3.5.
  /// </summary>
  delegate bool HasFunction<TMessage>(TMessage message);
  
}
