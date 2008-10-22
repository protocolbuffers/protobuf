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
  /// Describes a single method in a service.
  /// </summary>
  public sealed class MethodDescriptor : IndexedDescriptorBase<MethodDescriptorProto, MethodOptions> {

    private readonly ServiceDescriptor service;
    private MessageDescriptor inputType;
    private MessageDescriptor outputType;

    /// <value>
    /// The service this method belongs to.
    /// </value>
    public ServiceDescriptor Service {
      get { return service; }
    }

    /// <value>
    /// The method's input type.
    /// </value>
    public MessageDescriptor InputType {
      get { return inputType; }
    }

    /// <value>
    /// The method's input type.
    /// </value>
    public MessageDescriptor OutputType {
      get { return outputType; }
    }

    internal MethodDescriptor(MethodDescriptorProto proto, FileDescriptor file,
        ServiceDescriptor parent, int index) 
        : base(proto, file, parent.FullName + "." + proto.Name, index) {
      service = parent;
      file.DescriptorPool.AddSymbol(this);
    }

    internal void CrossLink() {
      IDescriptor lookup = File.DescriptorPool.LookupSymbol(Proto.InputType, this);
      if (!(lookup is MessageDescriptor)) {
        throw new DescriptorValidationException(this, "\"" + Proto.InputType + "\" is not a message type.");
      }
      inputType = (MessageDescriptor) lookup;

      lookup = File.DescriptorPool.LookupSymbol(Proto.OutputType, this);
      if (!(lookup is MessageDescriptor)) {
        throw new DescriptorValidationException(this, "\"" + Proto.OutputType + "\" is not a message type.");
      }
      outputType = (MessageDescriptor) lookup;
    }
  }
}
