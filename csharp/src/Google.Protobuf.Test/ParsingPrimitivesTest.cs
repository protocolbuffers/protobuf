#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2022 Google Inc.  All rights reserved.
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

using NUnit.Framework;
using System;
using System.Linq;

namespace Google.Protobuf.Test;

internal class ParsingPrimitivesTest
{
    // Note: test cases use integers rather than bytes as they're easier
    // to specify in attributes.

    [Test]
    [TestCase("\ufffd", 255)]
    [TestCase("A\ufffd", 65, 255)]
    [TestCase("A\ufffd\ufffdB", 65, 255, 255, 66)]
    // Overlong form of "space"
    [TestCase("\ufffd\ufffd", 0xc0, 0xa0)]
    public void ReadRawString_NonUtf8(string expectedText, params int[] bytes)
    {
        var context = CreateContext(bytes);
        string text = ParsingPrimitives.ReadRawString(ref context.buffer, ref context.state, bytes.Length);
        Assert.AreEqual(expectedText, text);
    }

    private static ParseContext CreateContext(int[] bytes)
    {
        byte[] actualBytes = bytes.Select(b => (byte) b).ToArray();
        ParseContext.Initialize(actualBytes.AsSpan(), out var context);
        return context;
    }
}
