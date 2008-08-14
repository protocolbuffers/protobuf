using System.Collections.Generic;

namespace Google.ProtocolBuffers.Descriptors {
  /// <summary>
  /// Contains lookup tables containing all the descriptors defined in a particular file.
  /// </summary>
  internal class DescriptorPool {

    IDictionary<string, object> byName;

    /// <summary>
    /// Finds a symbol of the given name within the pool.
    /// </summary>
    /// <typeparam name="T">The type of symbol to look for</typeparam>
    /// <param name="name">Name to look up</param>
    /// <returns>The symbol with the given name and type,
    /// or null if the symbol doesn't exist or has the wrong type</returns>
    internal T FindSymbol<T>(string name) where T : class {
      return default(T);
    }
  }
}
