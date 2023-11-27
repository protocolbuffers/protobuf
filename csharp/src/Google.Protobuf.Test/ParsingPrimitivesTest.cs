#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2022 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
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
