using System.Collections.Generic;
using Google.ProtocolBuffers.Collections;
namespace Google.ProtocolBuffers.Descriptors {
  /// <summary>
  /// Internal class containing utility methods when working with descriptors.
  /// </summary>
  static class DescriptorUtil {
    /// <summary>
    /// Equivalent to Func[TInput, int, TOutput] but usable in .NET 2.0. Only used to convert
    /// arrays.
    /// </summary>
    internal delegate TOutput IndexedConverter<TInput, TOutput>(TInput element, int index);

    /// <summary>
    /// Converts the given array into a read-only list, applying the specified conversion to
    /// each input element.
    /// </summary>
    internal static IList<TOutput> ConvertAndMakeReadOnly<TInput, TOutput>(IList<TInput> input,
        IndexedConverter<TInput, TOutput> converter) {
      TOutput[] array = new TOutput[input.Count];
      for (int i = 0; i < array.Length; i++) {
        array[i] = converter(input[i], i);
      }
      return Lists<TOutput>.AsReadOnly(array);
    }
  }
}
