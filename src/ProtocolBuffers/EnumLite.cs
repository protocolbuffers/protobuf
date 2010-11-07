using System;
using System.Collections.Generic;
using System.Text;

namespace Google.ProtocolBuffers {

  ///<summary>
  ///Interface for an enum value or value descriptor, to be used in FieldSet.
  ///The lite library stores enum values directly in FieldSets but the full
  ///library stores EnumValueDescriptors in order to better support reflection.
  ///</summary>
  public interface IEnumLite {
    int Number { get; }
  }

  ///<summary>
  ///Interface for an object which maps integers to {@link EnumLite}s.
  ///{@link Descriptors.EnumDescriptor} implements this interface by mapping
  ///numbers to {@link Descriptors.EnumValueDescriptor}s.  Additionally,
  ///every generated enum type has a static method internalGetValueMap() which
  ///returns an implementation of this type that maps numbers to enum values.
  ///</summary>
  public interface IEnumLiteMap<T> : IEnumLiteMap
    where T : IEnumLite {
    T FindValueByNumber(int number);
  }

  public interface IEnumLiteMap {
    bool IsValidValue(IEnumLite value);
  }
}
