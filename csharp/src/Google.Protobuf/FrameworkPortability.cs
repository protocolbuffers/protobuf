#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System;
using System.Text.RegularExpressions;

namespace Google.Protobuf
{
    /// <summary>
    /// Class containing helpful workarounds for various platform compatibility
    /// </summary>
    internal static class FrameworkPortability
    {
        // The value of RegexOptions.Compiled is 8. We can test for the presence at
        // execution time using Enum.IsDefined, so a single build will do the right thing
        // on each platform. (RegexOptions.Compiled isn't supported by PCLs.)
        internal static readonly RegexOptions CompiledRegexWhereAvailable =
            Enum.IsDefined(typeof(RegexOptions), 8) ? (RegexOptions)8 : RegexOptions.None;
    }
}