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

using System;
using System.Globalization;
using System.Text;
using System.Text.RegularExpressions;

namespace Google.ProtocolBuffers
{
    /// <summary>
    /// Helpers for converting names to pascal case etc.
    /// </summary>
    public class NameHelpers
    {
        public static string UnderscoresToPascalCase(string input)
        {
            return UnderscoresToPascalOrCamelCase(input, true);
        }

        public static string UnderscoresToCamelCase(string input)
        {
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
        private static string UnderscoresToPascalOrCamelCase(string input, bool pascal)
        {
            string name = Transform(input, pascal ? UnderlineToPascal : UnderlineToCamel, x => x.Value.TrimStart('_').ToUpper());

            if (name.Length == 0)
                throw new ArgumentException(String.Format("The field name '{0}' is invalid.", input));

            // Pascal case always begins with lower-case letter
            if (!pascal && Char.IsUpper(name[0]))
            {
                char[] chars = name.ToCharArray();
                chars[0] = char.ToLower(chars[0]);
                return new string(chars);
            }

            // Fields can not start with a number
            if (Char.IsNumber(name[0]))
                name = '_' + name;

            return name;
        }

        internal static string StripProto(string text)
        {
            if (!StripSuffix(ref text, ".protodevel"))
            {
                StripSuffix(ref text, ".proto");
            }
            return text;
        }

        /// <summary>
        /// Attempts to strip a suffix from a string, returning whether
        /// or not the suffix was actually present.
        /// </summary>
        public static bool StripSuffix(ref string text, string suffix)
        {
            if (text.EndsWith(suffix))
            {
                text = text.Substring(0, text.Length - suffix.Length);
                return true;
            }
            return false;
        }

        /// <summary>
        /// Similar to UnderlineToCamel, but also matches the first character if it is lower-case
        /// </summary>
        private static Regex UnderlineToPascal = new Regex(@"(?:(?:^|[0-9_])[a-z])|_");

        /// <summary>
        /// Matches lower-case character that follow either an underscore, or a number
        /// </summary>
        private static Regex UnderlineToCamel = new Regex(@"(?:[0-9_][a-z])|_");

        /// <summary>
        /// Used for text-template transformation where a regex match is replaced in the input string.
        /// </summary>
        /// <param name="input">The text to perform the replacement upon</param>
        /// <param name="pattern">The regex used to perform the match</param>
        /// <param name="fnReplace">A delegate that selects the appropriate replacement text</param>
        /// <returns>The newly formed text after all replacements are made</returns>
        /// <remarks>
        /// Originally found at http://csharptest.net/browse/src/Library/Utils/StringUtils.cs#120
        /// Republished here by the original author under this project's licensing.
        /// </remarks>
        private static string Transform(string input, Regex pattern, Converter<Match, string> fnReplace)
        {
            int currIx = 0;
            StringBuilder sb = new StringBuilder();

            foreach (Match match in pattern.Matches(input))
            {
                sb.Append(input, currIx, match.Index - currIx);
                string replace = fnReplace(match);
                sb.Append(replace);

                currIx = match.Index + match.Length;
            }

            sb.Append(input, currIx, input.Length - currIx);
            return sb.ToString();
        }
    }
}