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
using Google.ProtocolBuffers.DescriptorProtos;

namespace Google.ProtocolBuffers.Descriptors {
  
  /// <summary>
  /// Descriptor for a single enum value within an enum in a .proto file.
  /// </summary>
  public sealed class EnumValueDescriptor : IndexedDescriptorBase<EnumValueDescriptorProto, EnumValueOptions> {

    private readonly EnumDescriptor enumDescriptor;

    internal EnumValueDescriptor(EnumValueDescriptorProto proto, FileDescriptor file,
        EnumDescriptor parent, int index) 
        : base (proto, file, parent.FullName + "." + proto.Name, index) {
      enumDescriptor = parent;
      file.DescriptorPool.AddSymbol(this);
      file.DescriptorPool.AddEnumValueByNumber(this);
    }

    public int Number {
      get { return Proto.Number; }
    }

    public EnumDescriptor EnumDescriptor {
      get { return enumDescriptor; }
    }
  }
}
