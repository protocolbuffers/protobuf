#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://github.com/jskeet/dotnet-protobufs/
// Original C++/Java/Python code:
// http://code.google.com/p/protobuf/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#endregion

using System.Text;
using System.Globalization;

namespace Google.ProtocolBuffers {
  /// <summary>
  /// Helpers for converting names to pascal case etc.
  /// </summary>
  internal class NameHelpers {

    internal static string UnderscoresToPascalCase(string input) {
      return UnderscoresToPascalOrCamelCase(input, true);
    }

    internal static string UnderscoresToCamelCase(string input) {
      return UnderscoresToPascalOrCamelCase(input, false);
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
      for (int i = 0; i < input.Length; i++) {
        char c = input[i];
        if ('a' <= c && c <= 'z') {
          if (capitaliseNext) {
            result.Append(char.ToUpper(c, CultureInfo.InvariantCulture));
          } else {
            result.Append(c);
          }
          capitaliseNext = false;
        } else if ('A' <= c && c <= 'Z') {
          if (i == 0 && !pascal) {
            // Force first letter to lower-case unless explicitly told to
            // capitalize it.
            result.Append(char.ToLower(c, CultureInfo.InvariantCulture));
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

    internal static string StripProto(string text) {
      if (!StripSuffix(ref text, ".protodevel")) {
        StripSuffix(ref text, ".proto");
      }
      return text;
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
