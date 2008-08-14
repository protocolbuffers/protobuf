using System;
using System.Collections.Generic;
using System.Text;

namespace Google.ProtocolBuffers {
  public class FieldSet {
    public static void MergeFrom(CodedInputStream input,
         UnknownFieldSet.Builder unknownFields,
         ExtensionRegistry extensionRegistry,
         IBuilder builder) {

      while (true) {
        uint tag = input.ReadTag();
        if (tag == 0) {
          break;
        }
        if (!MergeFieldFrom(input, unknownFields, extensionRegistry,
                        builder, tag)) {
          // end group tag
          break;
        }
      }
    }

    public static bool MergeFieldFrom(CodedInputStream input,
      UnknownFieldSet.Builder unknownFields,
      ExtensionRegistry extensionRegistry,
      IBuilder builder,
      uint tag) {
      throw new NotImplementedException();
    }
  }
}
