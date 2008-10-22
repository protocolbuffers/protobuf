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
  /// Base class for nearly all descriptors, providing common functionality.
  /// </summary>
  /// <typeparam name="TProto">Type of the protocol buffer form of this descriptor</typeparam>
  /// <typeparam name="TOptions">Type of the options protocol buffer for this descriptor</typeparam>
  public abstract class DescriptorBase<TProto, TOptions> : IDescriptor<TProto>
      where TProto : IMessage, IDescriptorProto<TOptions> {

    private readonly TProto proto;
    private readonly FileDescriptor file;
    private readonly string fullName;

    protected DescriptorBase(TProto proto, FileDescriptor file, string fullName) {
      this.proto = proto;
      this.file = file;
      this.fullName = fullName;
    }

    protected static string ComputeFullName(FileDescriptor file, MessageDescriptor parent, string name) {
      if (parent != null) {
        return parent.FullName + "." + name;
      }
      if (file.Package.Length > 0) {
        return file.Package + "." + name;
      }
      return name;
    }

    IMessage IDescriptor.Proto {
      get { return proto; }
    }

    /// <summary>
    /// Returns the protocol buffer form of this descriptor
    /// </summary>
    public TProto Proto {
      get { return proto; }
    }

    public TOptions Options {
      get { return proto.Options; }
    }

    /// <summary>
    /// The fully qualified name of the descriptor's target.
    /// </summary>
    public string FullName {
      get { return fullName; }
    }

    /// <summary>
    /// The brief name of the descriptor's target.
    /// </summary>
    public string Name {
      get { return proto.Name; }
    }

    /// <value>
    /// The file this descriptor was declared in.
    /// </value>
    public FileDescriptor File {
      get { return file; }
    }
  }
}
