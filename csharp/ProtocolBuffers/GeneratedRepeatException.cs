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
using System.Collections.Generic;
using Google.ProtocolBuffers.Descriptors;
using System.Collections;

namespace Google.ProtocolBuffers {
  /// <summary>
  /// Class used to represent repeat extensions in generated classes.
  /// </summary>
  public sealed class GeneratedRepeatExtension<TExtensionElement> : GeneratedExtensionBase<IList<TExtensionElement>> {
    private GeneratedRepeatExtension(FieldDescriptor field) : base(field, typeof(TExtensionElement)) {
    }

    public static GeneratedExtensionBase<IList<TExtensionElement>> CreateInstance(FieldDescriptor descriptor) {
      if (!descriptor.IsRepeated) {
        throw new ArgumentException("Must call GeneratedRepeatExtension.CreateInstance() for repeated types.");
      }
      return new GeneratedRepeatExtension<TExtensionElement>(descriptor);
    }

    /// <summary>
    /// Converts the list to the right type.
    /// TODO(jonskeet): Check where this is used, and whether we need to convert
    /// for primitive types.
    /// </summary>
    /// <param name="value"></param>
    /// <returns></returns>
    public override object FromReflectionType(object value) {
      if (Descriptor.MappedType == MappedType.Message ||
          Descriptor.MappedType == MappedType.Enum) {
        // Must convert the whole list.
        List<TExtensionElement> result = new List<TExtensionElement>();
        foreach (object element in (IEnumerable) value) {
          result.Add((TExtensionElement) SingularFromReflectionType(element));
        }
        return result;
      } else {
        return value;
      }
    }
  }
}
