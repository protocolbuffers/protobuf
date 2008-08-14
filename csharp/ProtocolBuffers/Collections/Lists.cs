using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Text;

namespace Google.ProtocolBuffers.Collections {

  public static class Lists {

    public static IList<T> AsReadOnly<T>(IList<T> list) {
      return Lists<T>.AsReadOnly(list);
    }
  }

  /// <summary>
  /// Utilities class for dealing with lists.
  /// </summary>
  public static class Lists<T> {

    static readonly ReadOnlyCollection<T> empty = new ReadOnlyCollection<T>(new T[0]);

    /// <summary>
    /// Returns an immutable empty list.
    /// </summary>
    public static ReadOnlyCollection<T> Empty {
      get { return empty; }
    }

    /// <summary>
    /// Returns either the original reference if it's already read-only,
    /// or a new ReadOnlyCollection wrapping the original list.
    /// </summary>
    public static IList<T> AsReadOnly(IList<T> list) {
      return list.IsReadOnly ? list : new ReadOnlyCollection<T>(list);
    }
  }
}
