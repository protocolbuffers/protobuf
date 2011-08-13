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
using System.Text.RegularExpressions;

namespace Google.ProtocolBuffers
{
    /// <summary>
    /// Helpers for converting names to pascal case etc.
    /// </summary>
    public class NameHelpers
    {
        /// <summary>
        /// All characters that are not alpha-numeric
        /// </summary>
        private static readonly Regex NonAlphaNumericCharacters = new Regex(@"[^a-zA-Z0-9]+");

        /// <summary>
        /// Matches lower-case character that follow either an underscore, or a number
        /// </summary>
        private static readonly Regex UnderscoreOrNumberWithLowerCase = new Regex(@"[0-9_][a-z]");

        /// <summary>
        /// Removes non alpha numeric characters while capitalizing letters that follow
        /// a number or underscore.  The first letter is always upper case.
        /// </summary>
        public static string UnderscoresToPascalCase(string input)
        {
            string name = UnderscoresToUpperCase(input);

            // Pascal case always begins with upper-case letter
            if (Char.IsLower(name[0]))
            {
                char[] chars = name.ToCharArray();
                chars[0] = char.ToUpper(chars[0]);
                return new string(chars);
            }
            return name;
        }

        /// <summary>
        /// Removes non alpha numeric characters while capitalizing letters that follow
        /// a number or underscore.  The first letter is always lower case.
        /// </summary>
        public static string UnderscoresToCamelCase(string input)
        {
            string name = UnderscoresToUpperCase(input);

            // Camel case always begins with lower-case letter
            if (Char.IsUpper(name[0]))
            {
                char[] chars = name.ToCharArray();
                chars[0] = char.ToLower(chars[0]);
                return new string(chars);
            }
            return name;
        }

        /// <summary>
        /// Capitalizes any characters following an '_' or a number '0' - '9' and removes
        /// all non alpha-numeric characters.  If the resulting string begins with a number
        /// an '_' will be prefixed.  
        /// </summary>
        private static string UnderscoresToUpperCase(string input)
        {
            string name = UnderscoreOrNumberWithLowerCase.Replace(input, x => x.Value.ToUpper());
            name = NonAlphaNumericCharacters.Replace(name, String.Empty);

            if (name.Length == 0)
            {
                throw new ArgumentException(String.Format("The field name '{0}' is invalid.", input));
            }

            // Fields can not start with a number
            if (Char.IsNumber(name[0]))
            {
                name = '_' + name;
            }

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
    }
}