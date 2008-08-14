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
    /// Immutable mapping from field type to wire type. Built using the attributes on
    /// FieldType values.
    /// </summary>
    public static readonly IDictionary<FieldType, WireType> FieldTypeToWireFormatMap = MapFieldTypes();

    private static IDictionary<FieldType, WireType> MapFieldTypes() {
      var map = new Dictionary<FieldType, WireType>();
      foreach (FieldInfo field in typeof(FieldType).GetFields(BindingFlags.Static | BindingFlags.Public)) {
        FieldType fieldType = (FieldType) field.GetValue(null);
        FieldMappingAttribute mapping = (FieldMappingAttribute)field.GetCustomAttributes(typeof(FieldMappingAttribute), false)[0];
        map[fieldType] = mapping.WireType;
      }
      return Dictionaries.AsReadOnly(map);
    }
  }
}
