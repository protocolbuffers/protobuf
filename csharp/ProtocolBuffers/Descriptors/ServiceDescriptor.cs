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
using Google.ProtocolBuffers.DescriptorProtos;

namespace Google.ProtocolBuffers.Descriptors {

  /// <summary>
  /// Describes a service type.
  /// </summary>
  public sealed class ServiceDescriptor : IndexedDescriptorBase<ServiceDescriptorProto, ServiceOptions> {

    private readonly IList<MethodDescriptor> methods;

    public ServiceDescriptor(ServiceDescriptorProto proto, FileDescriptor file, int index)
        : base(proto, file, ComputeFullName(file, null, proto.Name), index) {

      methods = DescriptorUtil.ConvertAndMakeReadOnly(proto.MethodList,
        (method, i) => new MethodDescriptor(method, file, this, i));

      file.DescriptorPool.AddSymbol(this);
    }

    /// <value>
    /// An unmodifiable list of methods in this service.
    /// </value>
    public IList<MethodDescriptor> Methods {
      get { return methods; }
    }
   
    /// <summary>
    /// Finds a method by name.
    /// </summary>
    /// <param name="name">The unqualified name of the method (e.g. "Foo").</param>
    /// <returns>The method's decsriptor, or null if not found.</returns>
    public MethodDescriptor FindMethodByName(String name) {
      return File.DescriptorPool.FindSymbol<MethodDescriptor>(FullName + "." + name);
    }

    internal void CrossLink() {
      foreach (MethodDescriptor method in methods) {
        method.CrossLink();
      }
    }
  }
}
