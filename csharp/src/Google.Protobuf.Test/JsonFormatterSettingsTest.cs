#region Copyright notice and license

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

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
