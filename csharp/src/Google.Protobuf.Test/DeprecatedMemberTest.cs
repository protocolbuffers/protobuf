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
using UnitTest.Issues.TestProtos;

#pragma warning disable CS0612 // Type or member is obsolete
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
        public void TestDepreatedPrimitiveValue() =>
            AssertIsDeprecated(typeof(TestDeprecatedFields).GetProperty(nameof(TestDeprecatedFields.DeprecatedInt32)));

        [Test]
        public void TestDeprecatedMessage() =>
            AssertIsDeprecated(typeof(DeprecatedChild));

        [Test]
        public void TestDeprecatedEnum() =>
            AssertIsDeprecated(typeof(DeprecatedEnum));

        [Test]
        public void TestDeprecatedEnumValue() =>
            AssertIsDeprecated(typeof(DeprecatedEnum).GetField(nameof(DeprecatedEnum.DeprecatedZero)));
    }
}
#pragma warning restore CS0612 // Type or member is obsolete
