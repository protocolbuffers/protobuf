using System;
using System.Text;
using Google.ProtocolBuffers.DescriptorProtos;
using Google.ProtocolBuffers.Descriptors;
namespace Google.ProtocolBuffers.ProtoGen {

  /// <summary>
  /// Helpers to resolve class names etc.
  /// </summary>
  internal static class Helpers {
    internal static string UnderscoresToPascalCase(string input) {
      return UnderscoresToPascalOrCamelCase(input, true);
    }

    internal static string UnderscoresToCamelCase(string input) {
      return UnderscoresToPascalOrCamelCase(input, false);
    }

    internal static void WriteNamespaces(TextGenerator writer) {
      writer.WriteLine("using pb = global::Google.ProtocolBuffers;");
      writer.WriteLine("using pbc = global::Google.ProtocolBuffers.Collections;");
      writer.WriteLine("using pbd = global::Google.ProtocolBuffers.Descriptors;");
      writer.WriteLine("using scg = global::System.Collections.Generic;");
    }

    /// <summary>
    /// Converts a string to Pascal or Camel case. The first letter is capitalized or
    /// lower-cased depending on <paramref name="pascal"/> is true. 
    /// After the first letter, any punctuation is removed but triggers capitalization
    /// of the next letter. Digits are preserved but trigger capitalization of the next
    /// letter.
    /// All capitalisation is done in the invariant culture. 
    /// </summary>
    private static string UnderscoresToPascalOrCamelCase(string input, bool pascal) {
      StringBuilder result = new StringBuilder();
      bool capitaliseNext = pascal;
      for (int i=0; i < input.Length; i++) {
        char c = input[i];
        if ('a' <= c && c <= 'z') {
          if (capitaliseNext) {
            result.Append(char.ToUpperInvariant(c));
          } else {
            result.Append(c);
          }
          capitaliseNext = false;
        } else if ('A' <= c && c <= 'Z') {
          if (i == 0 && !pascal) {
            // Force first letter to lower-case unless explicitly told to
            // capitalize it.
            result.Append(char.ToLowerInvariant(c));
          } else {
            // Capital letters after the first are left as-is.
            result.Append(c);
          }
          capitaliseNext = false;
        } else if ('0' <= c && c <= '9') {
          result.Append(c);
          capitaliseNext = true;
        } else {
          capitaliseNext = true;
        }
      }
      return result.ToString();
    }

    /// <summary>
    /// Attempts to strip a suffix from a string, returning whether
    /// or not the suffix was actually present.
    /// </summary>
    internal static bool StripSuffix(ref string text, string suffix) {
      if (text.EndsWith(suffix)) {
        text = text.Substring(0, text.Length - suffix.Length);
        return true;
      }
      return false;
    }
  }
}
