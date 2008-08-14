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
