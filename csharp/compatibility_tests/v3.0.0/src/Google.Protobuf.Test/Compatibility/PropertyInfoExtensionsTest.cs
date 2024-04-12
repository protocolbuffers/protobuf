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
