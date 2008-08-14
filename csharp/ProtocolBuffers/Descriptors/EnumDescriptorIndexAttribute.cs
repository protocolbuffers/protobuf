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
  /// Allows enum values to express the index within their descriptor.
  /// TODO(jonskeet): Consider removing this. I don't think we need it after all.
  /// </summary>
  [AttributeUsage(AttributeTargets.Field)]
  public class EnumDescriptorIndexAttribute : Attribute {
    readonly int index;

    internal int Index {
      get { return index; }
    }

    internal EnumDescriptorIndexAttribute(int index) {
      this.index = index;
    }
  }
}
