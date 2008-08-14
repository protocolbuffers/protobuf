using System;
using System.Collections.Generic;
using System.Text;

namespace Google.ProtocolBuffers {
  public class WireFormat {
    public enum WireType : uint {
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

    private const int TagTypeBits = 3;
    private const uint TagTypeMask = (1 << TagTypeBits) - 1;

    /// <summary>
    /// Given a tag value, determines the wire type (lower 3 bits).
    /// </summary>
    public static WireType GetTagWireType(uint tag) {
      return (WireType) (tag & TagTypeMask);
    }

    /// <summary>
    /// Given a tag value, determines the field number (the upper 29 bits).
    /// </summary>
    public static uint GetTagFieldNumber(uint tag) {
      return tag >> TagTypeBits;
    }

    /// <summary>
    /// Makes a tag value given a field number and wire type.
    /// </summary>
    public static uint MakeTag(int fieldNumber, WireType wireType) {
      return (uint) (fieldNumber << TagTypeBits) | (uint) wireType;
    }
  }
}
