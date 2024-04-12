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

using NUnit.Framework;
using System;
using System.Collections.Generic;
using System.Reflection;

namespace Google.Protobuf.Compatibility
{
    public class TypeExtensionsTest
    {
        public class DerivedList : List<string> { }
        public string PublicProperty { get; set; }
        private string PrivateProperty { get; set; }

        public void PublicMethod()
        {
        }

        private void PrivateMethod()
        {
        }

        [Test]
        [TestCase(typeof(object), typeof(string), true)]
        [TestCase(typeof(object), typeof(int), true)]
        [TestCase(typeof(string), typeof(string), true)]
        [TestCase(typeof(string), typeof(object), false)]
        [TestCase(typeof(string), typeof(int), false)]
        [TestCase(typeof(int), typeof(int), true)]
        [TestCase(typeof(ValueType), typeof(int), true)]
        [TestCase(typeof(long), typeof(int), false)] //
        public void IsAssignableFrom(Type target, Type argument, bool expected)
        {
            Assert.AreEqual(expected, TypeExtensions.IsAssignableFrom(target, argument));
        }

        [Test]
        [TestCase(typeof(DerivedList), "Count")] // Go up the type hierarchy
        [TestCase(typeof(List<string>), "Count")]
        [TestCase(typeof(List<>), "Count")]
        [TestCase(typeof(TypeExtensionsTest), "PublicProperty")]
        public void GetProperty_Success(Type type, string name)
        {
            var property = TypeExtensions.GetProperty(type, name);
            Assert.IsNotNull(property);
            Assert.AreEqual(name, property.Name);
        }

        [Test]
        [TestCase(typeof(TypeExtensionsTest), "PrivateProperty")]
        [TestCase(typeof(TypeExtensionsTest), "Garbage")]
        public void GetProperty_NoSuchProperty(Type type, string name)
        {
            var property = TypeExtensions.GetProperty(type, name);
            Assert.IsNull(property);
        }

        [Test]
        [TestCase(typeof(DerivedList), "RemoveAt")] // Go up the type hierarchy
        [TestCase(typeof(List<>), "RemoveAt")]
        [TestCase(typeof(TypeExtensionsTest), "PublicMethod")]
        public void GetMethod_Success(Type type, string name)
        {
            var method = TypeExtensions.GetMethod(type, name);
            Assert.IsNotNull(method);
            Assert.AreEqual(name, method.Name);
        }

        [Test]
        [TestCase(typeof(TypeExtensionsTest), "PrivateMethod")]
        [TestCase(typeof(TypeExtensionsTest), "GarbageMethod")]
        public void GetMethod_NoSuchMethod(Type type, string name)
        {
            var method = TypeExtensions.GetMethod(type, name);
            Assert.IsNull(method);
        }

        [Test]
        [TestCase(typeof(List<string>), "IndexOf")]
        public void GetMethod_Ambiguous(Type type, string name)
        {
            Assert.Throws<AmbiguousMatchException>(() => TypeExtensions.GetMethod(type, name));
        }
    }
}
