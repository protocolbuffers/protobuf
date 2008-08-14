using System;
using System.Collections.Generic;
using System.Text;

namespace Google.ProtocolBuffers.Descriptors {
  public class DescriptorValidationException : Exception {

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
