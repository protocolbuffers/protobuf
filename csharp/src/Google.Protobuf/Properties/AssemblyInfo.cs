#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using System.Runtime.CompilerServices;
using System.Security;

// General Information about an assembly is controlled through the following 
// set of attributes. Change these attribute values to modify the information
// associated with an assembly.

#if !NCRUNCH
[assembly: AllowPartiallyTrustedCallers]
#endif

#if SIGNING_DISABLED
[assembly: InternalsVisibleTo("Google.Protobuf.Test")]
#else
[assembly: InternalsVisibleTo("Google.Protobuf.Test, PublicKey=" +
    "002400000480000094000000060200000024000052534131000400000100010025800fbcfc63a1" +
    "7c66b303aae80b03a6beaa176bb6bef883be436f2a1579edd80ce23edf151a1f4ced97af83abcd" +
    "981207041fd5b2da3b498346fcfcd94910d52f25537c4a43ce3fbe17dc7d43e6cbdb4d8f1242dc" +
    "b6bd9b5906be74da8daa7d7280f97130f318a16c07baf118839b156299a48522f9fae2371c9665" +
    "c5ae9cb6")]
#endif