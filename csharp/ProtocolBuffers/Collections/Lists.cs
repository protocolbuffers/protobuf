using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Text;

namespace Google.ProtocolBuffers.Collections {
  /// <summary>
  /// Utilities class for dealing with lists.
  /// </summary>
  static class Lists<T> {

    static readonly ReadOnlyCollection<T> empty = new ReadOnlyCollection<T>(new T[0]);

    /// <summary>
    /// Returns an immutable empty list.
    /// </summary>
    internal static ReadOnlyCollection<T> Empty {
      get { return empty; }
    }
  }
}
