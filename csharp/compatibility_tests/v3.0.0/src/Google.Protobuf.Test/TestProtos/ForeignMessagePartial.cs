#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2016 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

namespace Google.Protobuf.TestProtos
{
    /// <summary>
    /// A message with custom diagnostics (to test that they work).
    /// </summary>
    public partial class ForeignMessage : ICustomDiagnosticMessage
    {
        public string ToDiagnosticString()
        {
            return $"{{ \"c\": {C}, \"@cInHex\": \"{C:x}\" }}";
        }
    }
}
