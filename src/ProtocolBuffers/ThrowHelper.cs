using System;
using System.Collections.Generic;
using System.Text;

namespace Google.ProtocolBuffers {
  /// <summary>
  /// Helper methods for throwing exceptions
  /// </summary>
  public static class ThrowHelper {

    /// <summary>
    /// Throws an ArgumentNullException if the given value is null.
    /// </summary>
    public static void ThrowIfNull(object value, string name) {
      if (value == null) {
        throw new ArgumentNullException(name);
      }
    }

    /// <summary>
    /// Throws an ArgumentNullException if the given value is null.
    /// </summary>
    public static void ThrowIfNull(object value) {
      if (value == null) {
        throw new ArgumentNullException();
      }
    }

    /// <summary>
    /// Throws an ArgumentNullException if the given value or any element within it is null.
    /// </summary>
    public static void ThrowIfAnyNull<T>(IEnumerable<T> sequence) {
      foreach (T t in sequence) {
        if (t == null) {
          throw new ArgumentNullException();
        }
      }
    }
  }
}
