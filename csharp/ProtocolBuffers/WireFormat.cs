using System;
using System.Collections.Generic;
using System.Text;

namespace Google.ProtocolBuffers {
  public class WireFormat {
    public enum WireType {
      Varint = 0,
      Fixed64 = 1,
      LengthDelimited = 2,
      StartGroup = 3,
      EndGroup = 4,
      Fixed32 = 5
    }

    internal class MessageSetField {
      internal const int Item = 1;
      internal const int TypeID = 2;
      internal const int Message = 3;
    }

    public static uint MakeTag(int fieldNumber, WireType type) {

      // FIXME
      return 0;
    }
  }
}
