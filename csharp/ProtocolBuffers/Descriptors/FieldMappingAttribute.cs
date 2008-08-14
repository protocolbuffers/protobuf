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

namespace Google.ProtocolBuffers.Descriptors {

  /// <summary>
  /// Defined specifically for the <see cref="FieldType" /> enumeration,
  /// this allows each field type to specify the mapped type and wire type.
  /// </summary>
  [AttributeUsage(AttributeTargets.Field)]
  internal sealed class FieldMappingAttribute : Attribute {
    internal FieldMappingAttribute(MappedType mappedType, WireFormat.WireType wireType) {
      MappedType = mappedType;
      WireType = wireType;
    }

    internal MappedType MappedType { get; private set; }
    internal WireFormat.WireType WireType { get; private set; }
  }
}
