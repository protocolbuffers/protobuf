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
using System.Reflection;
using System.Text.RegularExpressions;

namespace Google.ProtocolBuffers
{
    /// <summary>
    /// Class containing helpful workarounds for various platform compatibility
    /// </summary>
    internal static class FrameworkPortability
    {
#if COMPACT_FRAMEWORK
        internal const string NewLine = "\n";
#else
        internal static readonly string NewLine = System.Environment.NewLine;
#endif

#if CLIENTPROFILE
        internal const RegexOptions CompiledRegexWhereAvailable = RegexOptions.Compiled;
#else
        internal const RegexOptions CompiledRegexWhereAvailable = RegexOptions.None;
#endif

        internal static CultureInfo InvariantCulture 
        { 
            get { return CultureInfo.InvariantCulture; } 
        }

        internal static double Int64ToDouble(long value)
        {
#if CLIENTPROFILE
            return BitConverter.Int64BitsToDouble(value);
#else
            double[] arresult = new double[1];
            Buffer.BlockCopy(new[] { value }, 0, arresult, 0, 8);
            return arresult[0];
#endif
        }

        internal static long DoubleToInt64(double value)
        {
#if CLIENTPROFILE
            return BitConverter.DoubleToInt64Bits(value);
#else
            long[] arresult = new long[1];
            Buffer.BlockCopy(new[] { value }, 0, arresult, 0, 8);
            return arresult[0];
#endif
        }

        internal static bool TryParseInt32(string text, out int number)
        {
            return TryParseInt32(text, NumberStyles.Any, InvariantCulture, out number);
        }

        internal static bool TryParseInt32(string text, NumberStyles style, IFormatProvider format, out int number)
        {
#if COMPACT_FRAMEWORK
                try 
                {
                    number = int.Parse(text, style, format); 
                    return true;
                }
                catch 
                {
                    number = 0;
                    return false; 
                }
#else
            return int.TryParse(text, style, format, out number);
#endif
        }
    }
}