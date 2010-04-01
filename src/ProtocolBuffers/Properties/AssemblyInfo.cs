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
using System;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;

// General Information about an assembly is controlled through the following 
// set of attributes. Change these attribute values to modify the information
// associated with an assembly.
[assembly: AssemblyTitle("ProtocolBuffers")]
[assembly: AssemblyDescription("")]
[assembly: AssemblyConfiguration("")]
[assembly: AssemblyCompany("")]
[assembly: AssemblyProduct("ProtocolBuffers")]
[assembly: AssemblyCopyright("Copyright ©  2008")]
[assembly: AssemblyTrademark("")]
[assembly: AssemblyCulture("")]
// Setting ComVisible to false makes the types in this assembly not visible 
// to COM components.  If you need to access a type in this assembly from 
// COM, set the ComVisible attribute to true on that type.
[assembly: ComVisible(false)]

// The following GUID is for the ID of the typelib if this project is exposed to COM
[assembly: Guid("279b643d-70e8-47ae-9eb1-500d1c48bab6")]

// Version information for an assembly consists of the following four values:
//
//      Major Version
//      Minor Version 
//      Build Number
//      Revision
//
// You can specify all the values or you can default the Build and Revision Numbers 
// by using the '*' as shown below:
// [assembly: AssemblyVersion("1.0.*")]
[assembly: AssemblyVersion("0.9.0.0")]
#if !COMPACT_FRAMEWORK_35
[assembly: AssemblyFileVersion("0.9.0.0")]
#endif

[assembly:InternalsVisibleTo("Google.ProtocolBuffers.Test,PublicKey="+
"00240000048000009400000006020000002400005253413100040000010001008179f2dd31a648"+
"2a2359dbe33e53701167a888e7c369a9ae3210b64f93861d8a7d286447e58bc167e3d99483beda"+
"72f738140072bb69990bc4f98a21365de2c105e848974a3d210e938b0a56103c0662901efd6b78"+
"0ee6dbe977923d46a8fda18fb25c65dd73b149a5cd9f3100668b56649932dadd8cf5be52eb1dce"+
"ad5cedbf")]
[assembly: InternalsVisibleTo("ProtoGen,PublicKey=" +
"00240000048000009400000006020000002400005253413100040000010001006d739020e13bdc" +
"038e86fa8aa5e1b13aae65d3ae79d622816c6067ab5b6955be50cc887130117582349208c13a55" +
"5e09a6084558f989ccde66094f07822808d3a9b922b0e85b912070032e90bb35360be7efb7982b" +
"702d7a5c6ed1e21d8ca587b4f4c9d2b81210d3641cc75f506cdfc628ac5453ff0a6886986c981d" +
"12245bc7")]
[assembly: CLSCompliant(true)]