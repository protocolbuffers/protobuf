#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using NUnit.Framework;
using System.Reflection;

namespace Google.Protobuf.Compatibility
{
    public class PropertyInfoExtensionsTest
    {
        public string PublicReadWrite { get; set; }
        private string PrivateReadWrite { get; set; }
        public string PublicReadPrivateWrite { get; private set; }
        public string PrivateReadPublicWrite { private get; set; }
        public string PublicReadOnly { get { return null; } }
        private string PrivateReadOnly { get { return null; } }
        public string PublicWriteOnly { set { } }
        private string PrivateWriteOnly { set {  } }

        [Test]
        [TestCase("PublicReadWrite")]
        [TestCase("PublicReadPrivateWrite")]
        [TestCase("PublicReadOnly")]
        public void GetGetMethod_Success(string name)
        {
            var propertyInfo = typeof(PropertyInfoExtensionsTest)
                .GetProperty(name, BindingFlags.Instance | BindingFlags.NonPublic | BindingFlags.Public);
            Assert.IsNotNull(PropertyInfoExtensions.GetGetMethod(propertyInfo));
        }

        [Test]
        [TestCase("PrivateReadWrite")]
        [TestCase("PrivateReadPublicWrite")]
        [TestCase("PrivateReadOnly")]
        [TestCase("PublicWriteOnly")]
        [TestCase("PrivateWriteOnly")]
        public void GetGetMethod_NoAccessibleGetter(string name)
        {
            var propertyInfo = typeof(PropertyInfoExtensionsTest)
                .GetProperty(name, BindingFlags.Instance | BindingFlags.NonPublic | BindingFlags.Public);
            Assert.IsNull(PropertyInfoExtensions.GetGetMethod(propertyInfo));
        }

        [Test]
        [TestCase("PublicReadWrite")]
        [TestCase("PrivateReadPublicWrite")]
        [TestCase("PublicWriteOnly")]
        public void GetSetMethod_Success(string name)
        {
            var propertyInfo = typeof(PropertyInfoExtensionsTest)
                .GetProperty(name, BindingFlags.Instance | BindingFlags.NonPublic | BindingFlags.Public);
            Assert.IsNotNull(PropertyInfoExtensions.GetSetMethod(propertyInfo));
        }

        [Test]
        [TestCase("PublicReadPrivateWrite")]
        [TestCase("PrivateReadWrite")]
        [TestCase("PrivateReadOnly")]
        [TestCase("PublicReadOnly")]
        [TestCase("PrivateWriteOnly")]
        public void GetSetMethod_NoAccessibleGetter(string name)
        {
            var propertyInfo = typeof(PropertyInfoExtensionsTest)
                .GetProperty(name, BindingFlags.Instance | BindingFlags.NonPublic | BindingFlags.Public);
            Assert.IsNull(PropertyInfoExtensions.GetSetMethod(propertyInfo));
        }
    }

}
