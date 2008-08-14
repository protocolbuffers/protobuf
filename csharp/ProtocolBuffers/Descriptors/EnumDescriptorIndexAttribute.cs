using System;

namespace Google.ProtocolBuffers.Descriptors {
  /// <summary>
  /// Allows enum values to express the index within their descriptor.
  /// </summary>
  [AttributeUsage(AttributeTargets.Field)]
  internal class EnumDescriptorIndexAttribute : Attribute {
    readonly int index;

    internal int Index {
      get { return index; }
    }

    internal EnumDescriptorIndexAttribute(int index) {
      this.index = index;
    }
  }
}
