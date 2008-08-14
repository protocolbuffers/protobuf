using System;
using System.Collections.Generic;
using System.Text;

namespace Google.ProtocolBuffers {
  public class UnknownFieldSet {
    public void WriteTo(CodedOutputStream output) {
      throw new NotImplementedException();
    }

    public int SerializedSize { get { return 0; } }
  }
}
