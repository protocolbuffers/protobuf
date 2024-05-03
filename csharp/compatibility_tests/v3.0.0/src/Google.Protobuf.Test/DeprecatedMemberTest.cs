#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion
    
using System;
using System.Reflection;
using Google.Protobuf.TestProtos;
using NUnit.Framework;

namespace Google.Protobuf
{
    public class DeprecatedMemberTest
    {
        private static void AssertIsDeprecated(MemberInfo member)
        {
            Assert.NotNull(member);
            Assert.IsTrue(member.IsDefined(typeof(ObsoleteAttribute), false), "Member not obsolete: " + member);
        }

        [Test]
        public void TestDepreatedPrimitiveValue()
        {
            AssertIsDeprecated(typeof(TestDeprecatedFields).GetProperty("DeprecatedInt32"));
        }

    }
}
