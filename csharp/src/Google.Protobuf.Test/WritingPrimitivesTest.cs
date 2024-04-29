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
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Google.Protobuf.Test;

internal class WritingPrimitivesTest
{
    [Test]
    public void WriteRawString_IllFormedUnicodeString()
    {
        // See https://codeblog.jonskeet.uk/2014/11/07/when-is-a-string-not-a-string/
        char c1 = '\u0058';
        char c2 = '\ud800';
        char c3 = '\u0059';
        string text = new string(new[] { c1, c2, c3 });
        Span<byte> buffer = new byte[10];
        WriteContext.Initialize(ref buffer, out var context);
        WritingPrimitives.WriteString(ref context.buffer, ref context.state, text);

        // The high surrogate is written out in a "raw" form, surrounded by the ASCII
        // characters.
        byte[] expectedBytes = { 0x5, 0x58, 0xef, 0xbf, 0xbd, 0x59 };
        Assert.AreEqual(expectedBytes, buffer.Slice(0, context.state.position).ToArray());
    }
}
