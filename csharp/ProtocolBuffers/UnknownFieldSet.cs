using System;
using System.Collections.Generic;
using System.Text;

namespace Google.ProtocolBuffers {
  public class UnknownFieldSet {
    public void WriteTo(CodedOutputStream output) {
      throw new NotImplementedException();
    }

    public int SerializedSize { get { return 0; } }

    public class Builder
    {
      internal void MergeFrom(CodedInputStream codedInputStream) {
        throw new NotImplementedException();
      }
    }
  }
}
