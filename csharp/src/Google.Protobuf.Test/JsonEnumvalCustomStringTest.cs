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

using JsonEnumvalCustomString;

namespace Google.Protobuf.Test
{
    public class JsonEnumvalCustomStringTest
    {
        // Gorget does not have a custom json string set, so it defaults to ARMOR_GORGET.
        [Test]
        public void GorgetDefaultSerialization()
        {
            var msg = new Knight { Armor = Armor.Gorget };
            Assert.AreEqual(Armor.Gorget, msg.Armor);

            var json = JsonFormatter.Default.Format(msg);
            Assert.AreEqual("{ \"armor\": \"ARMOR_GORGET\" }", json);

            var msg2 = JsonParser.Default.Parse<Knight>(json);
            Assert.AreEqual(Armor.Gorget, msg2.Armor);
        }

        // The great helm does have a custom json enumval, so let's confirm.
        [Test]
        public void GreatHelmSerialization()
        {
            var msg = new Knight { Armor = Armor.GreatHelm };
            Assert.AreEqual(Armor.GreatHelm, msg.Armor);

            var json = JsonFormatter.Default.Format(msg);
            Assert.AreEqual("{ \"armor\": \"gr8 helm\" }", json);

            var msg2 = JsonParser.Default.Parse<Knight>(json);
            Assert.AreEqual(Armor.GreatHelm, msg2.Armor);
        }

        // The gauntlet has a double quote in the middle of its custom json enumval,
        // let's make sure special characters are escaped properly.
        [Test]
        public void GauntletSerialization()
        {
            var msg = new Knight { Armor = Armor.Gauntlet };
            Assert.AreEqual(Armor.Gauntlet, msg.Armor);

            var json = JsonFormatter.Default.Format(msg);
            Assert.AreEqual("{ \"armor\": \"a\\\"b\" }", json);

            var msg2 = JsonParser.Default.Parse<Knight>(json);
            Assert.AreEqual(Armor.Gauntlet, msg2.Armor);
        }

        // Test quotes surrounding the custom json enumval.
        [Test]
        public void DoubleQuoteEnumSerialization()
        {
            var msg = new Knight { Armor = Armor.Plate };
            Assert.AreEqual(Armor.Plate, msg.Armor);

            var json = JsonFormatter.Default.Format(msg);
            Assert.AreEqual("{ \"armor\": \"\\\"plate\\\"\" }", json);

            var msg2 = JsonParser.Default.Parse<Knight>(json);
            Assert.AreEqual(Armor.Plate, msg2.Armor);
        }

        // Coif is an empty custom string, let's make sure that works.
        [Test]
        public void CoifEmptySerialization()
        {
            var msg = new Knight { Armor = Armor.Coif };
            Assert.AreEqual(Armor.Coif, msg.Armor);

            var json = JsonFormatter.Default.Format(msg);
            Assert.AreEqual("{ \"armor\": \"\" }", json);

            var msg2 = JsonParser.Default.Parse<Knight>(json);
            Assert.AreEqual(Armor.Coif, msg2.Armor);
        }

        // Ensure newlines and tabs are properly escaped.
        [Test]
        public void PauldronEscapingSerialization()
        {
            var msg = new Knight { Armor = Armor.Pauldron };
            Assert.AreEqual(Armor.Pauldron, msg.Armor);

            var json = JsonFormatter.Default.Format(msg);
            Assert.AreEqual("{ \"armor\": \"p\\taul\\ndron\" }", json);

            var msg2 = JsonParser.Default.Parse<Knight>(json);
            Assert.AreEqual(Armor.Pauldron, msg2.Armor);
        }

        // Int overrides always win.
        [Test]
        public void GreatHelmIntOverride()
        {
            var msg = new Knight { Armor = Armor.GreatHelm };
            var formatter = new JsonFormatter(JsonFormatter.Settings.Default.WithFormatEnumsAsIntegers(true));
            var json = formatter.Format(msg);
            Assert.AreEqual("{ \"armor\": 1 }", json);
        }
    }
}
