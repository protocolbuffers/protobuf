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
using System.Collections;
using System.Collections.Generic;
using Google.ProtocolBuffers.Collections;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers.FieldAccess {

  /// <summary>
  /// Accessor for a repeated enum field.
  /// </summary>
  internal sealed class RepeatedEnumAccessor : RepeatedPrimitiveAccessor {

    private readonly EnumDescriptor enumDescriptor;

    internal RepeatedEnumAccessor(FieldDescriptor field, string name, Type messageType, Type builderType)
        : base(name, messageType, builderType) {

      enumDescriptor = field.EnumType;
    }

    public override object GetValue(IMessage message) {
      List<EnumValueDescriptor> ret = new List<EnumValueDescriptor>();
      foreach (int rawValue in (IEnumerable) base.GetValue(message)) {
        ret.Add(enumDescriptor.FindValueByNumber(rawValue));
      }
      return Lists.AsReadOnly(ret);
    }

    public override object GetRepeatedValue(IMessage message, int index) {
      // Note: This relies on the fact that the CLR allows unboxing from an enum to
      // its underlying value
      int rawValue = (int) base.GetRepeatedValue(message, index);
      return enumDescriptor.FindValueByNumber(rawValue);
    }

    public override void AddRepeated(IBuilder builder, object value) {
      base.AddRepeated(builder, ((EnumValueDescriptor) value).Number);
    }

    public override void SetRepeated(IBuilder builder, int index, object value) {
      base.SetRepeated(builder, index, ((EnumValueDescriptor) value).Number);
    }
  }
}
