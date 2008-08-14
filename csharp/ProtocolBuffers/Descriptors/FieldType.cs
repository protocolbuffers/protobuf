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

namespace Google.ProtocolBuffers.Descriptors {
  /// <summary>
  /// Enumeration of all the possible field types. The odd formatting is to make it very clear
  /// which attribute applies to which value, while maintaining a compact format.
  /// </summary>
  public enum FieldType {
    [FieldMapping(MappedType.Double, WireFormat.WireType.Fixed64)] Double,
    [FieldMapping(MappedType.Single, WireFormat.WireType.Fixed32)] Float,
    [FieldMapping(MappedType.Int64, WireFormat.WireType.Varint)] Int64,
    [FieldMapping(MappedType.UInt64, WireFormat.WireType.Varint)] UInt64,
    [FieldMapping(MappedType.Int32, WireFormat.WireType.Varint)] Int32,
    [FieldMapping(MappedType.UInt64, WireFormat.WireType.Fixed64)] Fixed64,
    [FieldMapping(MappedType.UInt32, WireFormat.WireType.Fixed32)] Fixed32,
    [FieldMapping(MappedType.Boolean, WireFormat.WireType.Varint)] Bool,
    [FieldMapping(MappedType.String, WireFormat.WireType.LengthDelimited)] String,
    [FieldMapping(MappedType.Message, WireFormat.WireType.StartGroup)] Group,
    [FieldMapping(MappedType.Message, WireFormat.WireType.LengthDelimited)] Message,
    [FieldMapping(MappedType.ByteString, WireFormat.WireType.LengthDelimited)] Bytes,
    [FieldMapping(MappedType.UInt32, WireFormat.WireType.Varint)] UInt32,
    [FieldMapping(MappedType.Int32, WireFormat.WireType.Fixed32)] SFixed32,
    [FieldMapping(MappedType.Int64, WireFormat.WireType.Fixed64)] SFixed64,
    [FieldMapping(MappedType.Int32, WireFormat.WireType.Varint)] SInt32,
    [FieldMapping(MappedType.Int64, WireFormat.WireType.Varint)] SInt64,
    [FieldMapping(MappedType.Enum, WireFormat.WireType.Varint)] Enum
  }
}
