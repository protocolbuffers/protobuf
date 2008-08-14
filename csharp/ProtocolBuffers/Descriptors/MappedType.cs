using System;
using System.Collections.Generic;
using System.Text;

namespace Google.ProtocolBuffers.Descriptors {
  /// <summary>
  /// Type as it's mapped onto a .NET type.
  /// </summary>
  public enum MappedType {
    Int32,
    Int64,
    UInt32,
    UInt64,
    Single,
    Double,
    Boolean,
    String,
    ByteString,
    Message,
    Enum
  }
}
