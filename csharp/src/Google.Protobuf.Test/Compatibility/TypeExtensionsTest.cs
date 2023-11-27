#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
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
