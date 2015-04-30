#region Copyright notice and license

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://github.com/jskeet/dotnet-protobufs/
// Original C++/Java/Python code:
// http://code.google.com/p/protobuf/
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

using Xunit;

namespace Google.ProtocolBuffers
{
    public class NameHelpersTest
    {
        [Fact]
        public void UnderscoresToPascalCase()
        {
            Assert.Equal("FooBar", NameHelpers.UnderscoresToPascalCase("Foo_bar"));
            Assert.Equal("FooBar", NameHelpers.UnderscoresToPascalCase("foo_bar"));
            Assert.Equal("Foo0Bar", NameHelpers.UnderscoresToPascalCase("Foo0bar"));
            Assert.Equal("FooBar", NameHelpers.UnderscoresToPascalCase("Foo_+_Bar"));

            Assert.Equal("Bar", NameHelpers.UnderscoresToPascalCase("__+bar"));
            Assert.Equal("Bar", NameHelpers.UnderscoresToPascalCase("bar_"));
            Assert.Equal("_0Bar", NameHelpers.UnderscoresToPascalCase("_0bar"));
            Assert.Equal("_1Bar", NameHelpers.UnderscoresToPascalCase("_1_bar"));
        }

        [Fact]
        public void UnderscoresToCamelCase()
        {
            Assert.Equal("fooBar", NameHelpers.UnderscoresToCamelCase("Foo_bar"));
            Assert.Equal("fooBar", NameHelpers.UnderscoresToCamelCase("foo_bar"));
            Assert.Equal("foo0Bar", NameHelpers.UnderscoresToCamelCase("Foo0bar"));
            Assert.Equal("fooBar", NameHelpers.UnderscoresToCamelCase("Foo_+_Bar"));

            Assert.Equal("bar", NameHelpers.UnderscoresToCamelCase("__+bar"));
            Assert.Equal("bar", NameHelpers.UnderscoresToCamelCase("bar_"));
            Assert.Equal("_0Bar", NameHelpers.UnderscoresToCamelCase("_0bar"));
            Assert.Equal("_1Bar", NameHelpers.UnderscoresToCamelCase("_1_bar"));
        }

        [Fact]
        public void StripSuffix()
        {
            string text = "FooBar";
            Assert.False(NameHelpers.StripSuffix(ref text, "Foo"));
            Assert.Equal("FooBar", text);
            Assert.True(NameHelpers.StripSuffix(ref text, "Bar"));
            Assert.Equal("Foo", text);
        }
    }
}