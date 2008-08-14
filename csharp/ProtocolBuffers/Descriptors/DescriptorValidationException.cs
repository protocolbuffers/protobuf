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
  /// Thrown when building descriptors fails because the source DescriptorProtos
  /// are not valid.
  /// </summary>
  public sealed class DescriptorValidationException : Exception {

    private readonly String name;
    private readonly IMessage proto;
    private readonly string description;

    /// <value>
    /// The full name of the descriptor where the error occurred.
    /// </value>
    public String ProblemSymbolName { 
      get { return name; }
    }

    /// <value>
    /// The protocol message representation of the invalid descriptor.
    /// </value>
    public IMessage ProblemProto {
      get { return proto; }
    }

    /// <value>
    /// A human-readable description of the error. (The Message property
    /// is made up of the descriptor's name and this description.)
    /// </value>
    public string Description {
      get { return description; }
    }

    internal DescriptorValidationException(IDescriptor problemDescriptor, string description) :
        base(problemDescriptor.FullName + ": " + description) {

      // Note that problemDescriptor may be partially uninitialized, so we
      // don't want to expose it directly to the user.  So, we only provide
      // the name and the original proto.
      name = problemDescriptor.FullName;
      proto = problemDescriptor.Proto;
      this.description = description;
    }

    internal DescriptorValidationException(IDescriptor problemDescriptor, string description, Exception cause) :
        base(problemDescriptor.FullName + ": " + description, cause) {

      name = problemDescriptor.FullName;
      proto = problemDescriptor.Proto;
      this.description = description;
    }
  }
}
