#region Copyright notice and license

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
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

using Google.Protobuf.Reflection;
using NUnit.Framework;

// For WrapInQuotes

namespace Google.Protobuf
{
    public class JsonFormatterSettingsTest
    {
        [Test]
        public void WithIndentation()
        {
            var settings = JsonFormatter.Settings.Default.WithIndentation("\t");
            Assert.AreEqual("\t", settings.Indentation);
        }

        [Test]
        public void WithTypeRegistry()
        {
            var typeRegistry = TypeRegistry.Empty;
            var settings = JsonFormatter.Settings.Default.WithTypeRegistry(typeRegistry);
            Assert.AreEqual(typeRegistry, settings.TypeRegistry);
        }

        [Test]
        public void WithFormatDefaultValues()
        {
            var settingsWith = JsonFormatter.Settings.Default.WithFormatDefaultValues(true);
            Assert.AreEqual(true, settingsWith.FormatDefaultValues);

            var settingsWithout = JsonFormatter.Settings.Default.WithFormatDefaultValues(false);
            Assert.AreEqual(false, settingsWithout.FormatDefaultValues);
        }

        [Test]
        public void WithFormatEnumsAsIntegers()
        {
            var settingsWith = JsonFormatter.Settings.Default.WithFormatEnumsAsIntegers(true);
            Assert.AreEqual(true, settingsWith.FormatEnumsAsIntegers);

            var settingsWithout = JsonFormatter.Settings.Default.WithFormatEnumsAsIntegers(false);
            Assert.AreEqual(false, settingsWithout.FormatEnumsAsIntegers);
        }

        [Test]
        public void WithMethodsPreserveExistingSettings()
        {
            var typeRegistry = TypeRegistry.Empty;
            var baseSettings = JsonFormatter.Settings.Default
                .WithIndentation("\t")
                .WithFormatDefaultValues(true)
                .WithFormatEnumsAsIntegers(true)
                .WithTypeRegistry(typeRegistry)
                .WithPreserveProtoFieldNames(true);

            var settings1 = baseSettings.WithIndentation("\t");
            var settings2 = baseSettings.WithFormatDefaultValues(true);
            var settings3 = baseSettings.WithFormatEnumsAsIntegers(true);
            var settings4 = baseSettings.WithTypeRegistry(typeRegistry);
            var settings5 = baseSettings.WithPreserveProtoFieldNames(true);

            AssertAreEqual(baseSettings, settings1);
            AssertAreEqual(baseSettings, settings2);
            AssertAreEqual(baseSettings, settings3);
            AssertAreEqual(baseSettings, settings4);
            AssertAreEqual(baseSettings, settings5);
        }

        private static void AssertAreEqual(JsonFormatter.Settings settings, JsonFormatter.Settings other)
        {
            Assert.AreEqual(settings.Indentation, other.Indentation);
            Assert.AreEqual(settings.FormatDefaultValues, other.FormatDefaultValues);
            Assert.AreEqual(settings.FormatEnumsAsIntegers, other.FormatEnumsAsIntegers);
            Assert.AreEqual(settings.TypeRegistry, other.TypeRegistry);
        }
    }
}
