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

using System;
using NUnit.Framework;

namespace Google.Protobuf
{
    /// <summary>
    /// Helper methods when testing equality. NUnit's Assert.AreEqual and
    /// Assert.AreNotEqual methods try to be clever with collections, which can
    /// be annoying...
    /// </summary>
    internal static class EqualityTester
    {
        public static void AssertEquality<T>(T first, T second) where T : IEquatable<T>
        {
            Assert.IsTrue(first.Equals(second));
            Assert.IsTrue(first.Equals((object) second));
            Assert.AreEqual(first.GetHashCode(), second.GetHashCode());
        }

        public static void AssertInequality<T>(T first, T second) where T : IEquatable<T>
        {
            Assert.IsFalse(first.Equals(second));
            Assert.IsFalse(first.Equals((object) second));
            // While this isn't a requirement, the chances of this test failing due to
            // coincidence rather than a bug are very small.
            if (first != null && second != null)
            {
                Assert.AreNotEqual(first.GetHashCode(), second.GetHashCode());
            }
        }
    }
}
