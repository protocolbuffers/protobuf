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
using System.Reflection;
using Google.ProtocolBuffers.Descriptors;
using System.Collections.Generic;
using Google.ProtocolBuffers.Collections;
namespace Google.ProtocolBuffers {
  
  /// <summary>
  /// This class is used internally by the Protocol Buffer Library and generated
  /// message implementations. It is public only for the sake of those generated
  /// messages. Others should not use this class directly.
  /// <para>
  /// This class contains constants and helper functions useful for dealing with
  /// the Protocol Buffer wire format.
  /// </para>
  /// </summary>
  public static class WireFormat {
    public enum WireType : uint {
      Varint = 0,
      Fixed64 = 1,
      LengthDelimited = 2,
      StartGroup = 3,
      EndGroup = 4,
      Fixed32 = 5
    }

    internal static class MessageSetField {
      internal const int Item = 1;
      internal const int TypeID = 2;
      internal const int Message = 3;
    }

    internal static class MessageSetTag {
      internal static readonly uint ItemStart = MakeTag(MessageSetField.Item, WireType.StartGroup);
      internal static readonly uint ItemEnd = MakeTag(MessageSetField.Item, WireType.EndGroup);
      internal static readonly uint TypeID = MakeTag(MessageSetField.TypeID, WireType.Varint);
      internal static readonly uint Message = MakeTag(MessageSetField.Message, WireType.LengthDelimited);
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
    public static int GetTagFieldNumber(uint tag) {
      return (int) tag >> TagTypeBits;
    }

    /// <summary>
    /// Makes a tag value given a field number and wire type.
    /// TODO(jonskeet): Should we just have a Tag structure?
    /// </summary>
    public static uint MakeTag(int fieldNumber, WireType wireType) {
      return (uint) (fieldNumber << TagTypeBits) | (uint) wireType;
    }

    /// <summary>
    /// Converts a field type to its wire type. Done with a switch for the sake
    /// of speed - this is significantly faster than a dictionary lookup.
    /// </summary>
    public static WireType GetWireType(FieldType fieldType) {
      switch (fieldType) {
        case FieldType.Double:
          return WireType.Fixed64;
        case FieldType.Float:
          return WireType.Fixed32;
        case FieldType.Int64:
        case FieldType.UInt64:
        case FieldType.Int32:
          return WireType.Varint;
        case FieldType.Fixed64:
          return WireType.Fixed64;
        case FieldType.Fixed32:
          return WireType.Fixed32;
        case FieldType.Bool:
          return WireType.Varint;
        case FieldType.String:
          return WireType.LengthDelimited;
        case FieldType.Group:
          return WireType.StartGroup;
        case FieldType.Message:
        case FieldType.Bytes:
          return WireType.LengthDelimited;
        case FieldType.UInt32:
          return WireType.Varint;
        case FieldType.SFixed32:
          return WireType.Fixed32;
        case FieldType.SFixed64:
          return WireType.Fixed64;
        case FieldType.SInt32:
        case FieldType.SInt64:
        case FieldType.Enum:
          return WireType.Varint;
        default:
          throw new ArgumentOutOfRangeException("No such field type");
      }
    }
  }
}
