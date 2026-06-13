#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

using Google.Protobuf.Reflection;
using NUnit.Framework;
using System;
using System.Linq;

using JsonEnumvalCustomString;

namespace Google.Protobuf.Test
{
    public class JsonEnumvalCustomStringTest
    {
        [Test]
        // No custom value.
        [TestCase(Armor.Gorget, "ARMOR_GORGET")]
        // Simple custom value.
        [TestCase(Armor.GreatHelm, "gr8 helm")]
        // Escaping of quotes mid-value.
        [TestCase(Armor.Gauntlet, "a\\\"b")]
        // Escaping of quotes at start and end.
        [TestCase(Armor.Plate, "\\\"plate\\\"")]
        // Empty string.
        [TestCase(Armor.Coif, "")]
        // Escaping of tab and newline.
        [TestCase(Armor.Pauldron, "p\\taul\\ndron")]
        public void Serialize(Armor value, string expectedSerializedJsonValue)
        {
            var msg = new Knight { Armor = value };
            var actualJson = JsonFormatter.Default.Format(msg);
            var expectedJson = $"{{ \"armor\": \"{expectedSerializedJsonValue}\" }}";
            Assert.AreEqual(expectedJson, actualJson);
        }

        [Test]
        public void SerializeUnknownValue()
        {
            var msg = new Knight { Armor = (Armor) 12345 };
            var actualJson = JsonFormatter.Default.Format(msg);
            var expectedJson = $"{{ \"armor\": 12345 }}";
            Assert.AreEqual(expectedJson, actualJson);
        }

        [Test]
        [TestCase(Armor.Gorget, "ARMOR_GORGET")]
        [TestCase(Armor.GreatHelm, "gr8 helm", "ARMOR_GREAT_HELM")]
        [TestCase(Armor.Gauntlet, "a\\\"b", "ARMOR_GAUNTLET")]
        [TestCase(Armor.Plate, "\\\"plate\\\"", "ARMOR_PLATE")]
        [TestCase(Armor.Coif, "", "ARMOR_COIF")]
        [TestCase(Armor.Pauldron, "p\\taul\\ndron", "ARMOR_PAULDRON")]
        public void ParseString(Armor value, params string[] validJsonValues)
        {
            foreach (var validJsonValue in validJsonValues)
            {
                string json = $"{{ \"armor\": \"{validJsonValue}\" }}";
                var parsed = JsonParser.Default.Parse<Knight>(json);
                Assert.AreEqual(value, parsed.Armor);
            }
        }

        [Test]
        public void ParseInteger()
        {
            foreach (var value in Enum.GetValues(typeof(Armor)).Cast<Armor>())
            {
                string json = $"{{ \"armor\": {(int) value} }}";
                var parsed = JsonParser.Default.Parse<Knight>(json);
                Assert.AreEqual(value, parsed.Armor);
            }
        }

        [Test]
        public void IntegerFormatSettingOverridesCustomString()
        {
            var msg = new Knight { Armor = Armor.GreatHelm };
            var formatter = new JsonFormatter(JsonFormatter.Settings.Default.WithFormatEnumsAsIntegers(true));
            
            var json = formatter.Format(msg);
            Assert.AreEqual("{ \"armor\": 1 }", json);
        }
    }
}
