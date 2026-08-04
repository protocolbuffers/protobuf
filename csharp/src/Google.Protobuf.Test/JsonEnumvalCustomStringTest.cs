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
using System.Reflection;

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
        // Aliased enum values.
        [TestCase(Armor.Sabaton, "sabaton")]
        [TestCase(Armor.Solleret, "sabaton")]
        // Numeric string custom value.
        [TestCase(Armor.HachiMaiDo, "8")]
        // Custom value same as enum name.
        [TestCase(Armor.Greaves, "ARMOR_GREAVES")]
        public void Serialize(Armor value, string expectedSerializedJsonValue)
        {
            var msg = new Knight { Armor = value };
            var actualJson = JsonFormatter.Default.Format(msg);
            var expectedJson =
                $"{{ \"armor\": \"{expectedSerializedJsonValue}\" }}";
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
        [TestCase(Armor.Sabaton, "sabaton", "ARMOR_SABATON", "ARMOR_SOLLERET")]
        [TestCase(
            Armor.Solleret, "sabaton", "ARMOR_SOLLERET", "ARMOR_SABATON")]
        [TestCase(Armor.HachiMaiDo, "8", "ARMOR_HACHI_MAI_DO")]
        [TestCase(Armor.Greaves, "ARMOR_GREAVES")]
        [TestCase(Armor.Unknown, "ARMOR_UNKNOWN")]
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
            var settings = JsonFormatter.Settings.Default
                .WithFormatEnumsAsIntegers(true);
            var formatter = new JsonFormatter(settings);

            var json = formatter.Format(msg);
            Assert.AreEqual("{ \"armor\": 1 }", json);
        }

        [Test]
        [TestCase("\"UNKNOWN_1\"")]
        [TestCase("\"ARMOR_INVALID\"")]
        [TestCase("\"A\\\"b\"")]
        [TestCase("\"ARMOR_great_helm\"")]
        [TestCase("\"GR8 HELM\"")]
        [TestCase("true")]
        [TestCase("123.456")]
        [TestCase("{}")]
        [TestCase("[ \"gr8 helm\" ]")]
        [TestCase("[ \"ARMOR_GREAT_HELM\" ]")]
        public void ParseInvalidValueFails(string jsonValue)
        {
            string json = $"{{ \"armor\": {jsonValue} }}";
            Assert.Throws<InvalidProtocolBufferException>(
                () => JsonParser.Default.Parse<Knight>(json));
        }

        [Test]
        [TestCase("UNKNOWN_1")]
        [TestCase("ARMOR_great_helm")]
        [TestCase("GR8 HELM")]
        public void ParseUnknownString_IgnoreUnknownFields(
            string unrecognizedJsonValue)
        {
            var settings = JsonParser.Settings.Default
                .WithIgnoreUnknownFields(true);
            var parser = new JsonParser(settings);
            string json = $"{{ \"armor\": \"{unrecognizedJsonValue}\" }}";
            var parsed = parser.Parse<Knight>(json);
            Assert.AreEqual(Armor.Unknown, parsed.Armor);
        }

        [Test]
        [TestCase("Unknown", null, "ARMOR_UNKNOWN", true)]
        [TestCase("GreatHelm", "gr8 helm", "ARMOR_GREAT_HELM", true)]
        [TestCase("Gorget", null, "ARMOR_GORGET", true)]
        [TestCase("Gauntlet", "a\"b", "ARMOR_GAUNTLET", true)]
        [TestCase("Plate", "\"plate\"", "ARMOR_PLATE", true)]
        [TestCase("Coif", "", "ARMOR_COIF", true)]
        [TestCase("Pauldron", "p\taul\ndron", "ARMOR_PAULDRON", true)]
        [TestCase("Sabaton", "sabaton", "ARMOR_SABATON", true)]
        [TestCase("Solleret", "sabaton", "ARMOR_SOLLERET", false)]
        [TestCase("HachiMaiDo", "8", "ARMOR_HACHI_MAI_DO", true)]
        [TestCase("Greaves", "ARMOR_GREAVES", "ARMOR_GREAVES", true)]
        public void OriginalNameAttribute(
            string fieldName,
            string expectedJsonEnumValueName,
            string expectedName,
            bool expectedPreferredAlias)
        {
            var field = typeof(Armor).GetTypeInfo()
                .GetDeclaredField(fieldName);
            Assert.IsNotNull(
                field, $"Field {fieldName} should exist in Armor enum.");
            var attr = field.GetCustomAttributes<OriginalNameAttribute>()
                .FirstOrDefault();
            Assert.IsNotNull(
                attr, $"Field {fieldName} should have OriginalNameAttribute.");
            Assert.AreEqual(expectedName, attr.Name);
            Assert.AreEqual(expectedJsonEnumValueName, attr.JsonEnumValueName);
            Assert.AreEqual(expectedPreferredAlias, attr.PreferredAlias);
        }
    }
}
