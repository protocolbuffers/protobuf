#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
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

using Google.Protobuf.Reflection;
using NUnit.Framework;
using System;

namespace Google.Protobuf.Test.Collections;

public class RuntimeVersionTest
{
    // Example of the error message generated:
    // The runtime version expected by the generated code 3.3.4 is not compatible with the current runtime version 2.4.0.
    // The major versions much match exactly, the current minor version must be greater than or equal to the expected minor version,
    // and if the minor versions are the same, the current patch version must be greater than or equal to the expected patch version. Incompatibility type: major.

    [Test]
    [TestCase("2.3.4", "2.3.4", Description = "Exact match")]
    [TestCase("2.3.5", "2.3.4", Description = "Current version has later patch in same minor")]
    [TestCase("2.4.0", "2.3.4", Description = "Current version has zero patch in later minor")]
    public void ValidVersionCombinations(string currentVersion, string targetVersion)
    {
        Version current = new Version(currentVersion);
        Version target = new Version(targetVersion);
        RuntimeVersion.Validate(current, target);
    }

    [Test]
    [TestCase("3.4.5", "2.3.4", Description = "Current version has later major")]
    [TestCase("1.4.5", "2.3.4", Description = "Current version has earlier major")]
    [TestCase("2.2.6", "2.3.4", Description = "Current version has earlier minor")]
    [TestCase("2.3.3", "2.3.4", Description = "Current version has earlier patch")]
    public void InvalidVersionCombinations(string currentVersion, string targetVersion)
    {
        Version current = new Version(currentVersion);
        Version target = new Version(targetVersion);
        Assert.Throws<InvalidOperationException>(() => RuntimeVersion.Validate(current, target));
    }
}
