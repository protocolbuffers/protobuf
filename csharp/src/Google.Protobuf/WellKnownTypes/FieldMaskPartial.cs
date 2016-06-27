#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2016 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
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
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace Google.Protobuf.WellKnownTypes
{
    // Manually-written partial class for the FieldMask well-known type.
    public partial class FieldMask : ICustomDiagnosticMessage
    {
        /// <summary>
        /// Converts a timestamp  specified in seconds/nanoseconds to a string.
        /// </summary>
        /// <remarks>
        /// If the value is a normalized duration in the range described in <c>field_mask.proto</c>,
        /// <paramref name="diagnosticOnly"/> is ignored. Otherwise, if the parameter is <c>true</c>,
        /// a JSON object with a warning is returned; if it is <c>false</c>, an <see cref="InvalidOperationException"/> is thrown.
        /// </remarks>
        /// <param name="paths">Paths in the field mask</param>
        /// <param name="diagnosticOnly">Determines the handling of non-normalized values</param>
        /// <exception cref="InvalidOperationException">The represented duration is invalid, and <paramref name="diagnosticOnly"/> is <c>false</c>.</exception>
        internal static string ToJson(IList<string> paths, bool diagnosticOnly)
        {
            var firstInvalid = paths.FirstOrDefault(p => !ValidatePath(p));
            if (firstInvalid == null)
            {
                var writer = new StringWriter();
#if DOTNET35
                var query = paths.Select(JsonFormatter.ToCamelCase);
                JsonFormatter.WriteString(writer, string.Join(",", query.ToArray()));
#else
                JsonFormatter.WriteString(writer, string.Join(",", paths.Select(JsonFormatter.ToCamelCase)));
#endif
                return writer.ToString();
            }
            else
            {
                if (diagnosticOnly)
                {
                    var writer = new StringWriter();
                    writer.Write("{ \"@warning\": \"Invalid FieldMask\", \"paths\": ");
                    JsonFormatter.Default.WriteList(writer, (IList)paths);
                    writer.Write(" }");
                    return writer.ToString();
                }
                else
                {
                    throw new InvalidOperationException($"Invalid field mask to be converted to JSON: {firstInvalid}");
                }
            }
        }

        /// <summary>
        /// Camel-case converter with added strictness for field mask formatting.
        /// </summary>
        /// <exception cref="InvalidOperationException">The field mask is invalid for JSON representation</exception>
        private static bool ValidatePath(string input)
        {
            for (int i = 0; i < input.Length; i++)
            {
                char c = input[i];
                if (c >= 'A' && c <= 'Z')
                {
                    return false;
                }
                if (c == '_' && i < input.Length - 1)
                {
                    char next = input[i + 1];
                    if (next < 'a' || next > 'z')
                    {
                        return false;
                    }
                }
            }
            return true;
        }

        /// <summary>
        /// Returns a string representation of this <see cref="FieldMask"/> for diagnostic purposes.
        /// </summary>
        /// <remarks>
        /// Normally the returned value will be a JSON string value (including leading and trailing quotes) but
        /// when the value is non-normalized or out of range, a JSON object representation will be returned
        /// instead, including a warning. This is to avoid exceptions being thrown when trying to
        /// diagnose problems - the regular JSON formatter will still throw an exception for non-normalized
        /// values.
        /// </remarks>
        /// <returns>A string representation of this value.</returns>
        public string ToDiagnosticString()
        {
            return ToJson(Paths, true);
        }
    }
}
