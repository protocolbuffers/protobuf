
using System;

namespace Google.ProtocolBuffers {
  public class Descriptors {
    public class Descriptor {
    }
    public class FieldDescriptor {
      public enum Type {
        Double,
        Float,
        Int64,
        UInt64,
        Int32,
        Fixed64,
        Fixed32,
        Bool,
        String,
        Group,
        Message,
        Bytes,
        UInt32,
        SFixed32,
        SFixed64,
        SInt32,
        SInt64,
        Enum
      }
    }

    public class EnumValueDescriptor
    {
      public int Number
      {
        get { throw new NotImplementedException(); }
      }
    }
  }
}
