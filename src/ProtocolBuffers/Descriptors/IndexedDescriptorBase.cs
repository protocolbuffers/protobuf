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
  /// Base class for descriptors which are also indexed. This is all of them other than
  /// <see cref="FileDescriptor" />.
  /// </summary>
  public abstract class IndexedDescriptorBase<TProto, TOptions> : DescriptorBase<TProto, TOptions>
      where TProto : IMessage<TProto>, IDescriptorProto<TOptions> {

    private readonly int index;

    protected IndexedDescriptorBase(TProto proto, FileDescriptor file, string fullName, int index)
        : base(proto, file, fullName) {
      this.index = index;
    }

    /// <value>
    /// The index of this descriptor within its parent descriptor. 
    /// </value>
    /// <remarks>
    /// This returns the index of this descriptor within its parent, for
    /// this descriptor's type. (There can be duplicate values for different
    /// types, e.g. one enum type with index 0 and one message type with index 0.)
    /// </remarks>
    public int Index {
      get { return index; }
    }
  }
}
