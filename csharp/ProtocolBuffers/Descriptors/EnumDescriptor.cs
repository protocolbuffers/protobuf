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
using System.Collections.Generic;
using Google.ProtocolBuffers.DescriptorProtos;

namespace Google.ProtocolBuffers.Descriptors {

  /// <summary>
  /// Descriptor for an enum type in a .proto file.
  /// </summary>
  public sealed class EnumDescriptor : IndexedDescriptorBase<EnumDescriptorProto, EnumOptions> {

    private readonly MessageDescriptor containingType;
    private readonly IList<EnumValueDescriptor> values;

    internal EnumDescriptor(EnumDescriptorProto proto, FileDescriptor file, MessageDescriptor parent, int index)
        : base(proto, file, ComputeFullName(file, parent, proto.Name), index) {
      containingType = parent;

      if (proto.ValueCount == 0) {
        // We cannot allow enums with no values because this would mean there
        // would be no valid default value for fields of this type.
        throw new DescriptorValidationException(this, "Enums must contain at least one value.");
      }
      
      values = DescriptorUtil.ConvertAndMakeReadOnly(proto.ValueList,
          (value, i) => new EnumValueDescriptor(value, file, this, i));

      File.DescriptorPool.AddSymbol(this);
    }

    /// <value>
    /// If this is a nested type, get the outer descriptor, otherwise null.
    /// </value>
    public MessageDescriptor ContainingType {
      get { return containingType; }
    }

    /// <value>
    /// An unmodifiable list of defined value descriptors for this enum.
    /// </value>
    public IList<EnumValueDescriptor> Values {
      get { return values; }
    }

    /// <summary>
    /// Finds an enum value by number. If multiple enum values have the
    /// same number, this returns the first defined value with that number.
    /// </summary>
    // TODO(jonskeet): Make internal and use InternalsVisibleTo?
    public EnumValueDescriptor FindValueByNumber(int number) {
      return File.DescriptorPool.FindEnumValueByNumber(this, number);
    }

    /// <summary>
    /// Finds an enum value by name.
    /// </summary>
    /// <param name="name">The unqualified name of the value (e.g. "FOO").</param>
    /// <returns>The value's descriptor, or null if not found.</returns>
    // TODO(jonskeet): Make internal and use InternalsVisibleTo?
    public EnumValueDescriptor FindValueByName(string name) {
      return File.DescriptorPool.FindSymbol<EnumValueDescriptor>(FullName + "." + name);
    }
  }
}
