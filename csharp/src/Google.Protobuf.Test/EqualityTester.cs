#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
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

        public static void AssertInequality<T>(T first, T second, bool checkHashcode = true) where T : IEquatable<T>
        {
            Assert.IsFalse(first.Equals(second));
            Assert.IsFalse(first.Equals((object) second));
            // While this isn't a requirement, the chances of this test failing due to
            // coincidence rather than a bug are very small.
            // For such rare cases, an argument can be used to disable the check.
            if (checkHashcode && first != null && second != null)
            {
                Assert.AreNotEqual(first.GetHashCode(), second.GetHashCode());
            }
        }
    }
}
