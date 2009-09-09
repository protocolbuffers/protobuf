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

using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Text;
using System.Threading;
using Google.ProtocolBuffers.TestProtos;
using NUnit.Framework;

namespace Google.ProtocolBuffers {
  internal static class TestUtil {

    private static string testDataDirectory;
    private static ByteString goldenMessage = null;

    internal static string TestDataDirectory {
      get {
        if (testDataDirectory != null) {
          return testDataDirectory;
        }

        DirectoryInfo ancestor = new DirectoryInfo(".");
        // Search each parent directory looking for "testdata".
        while (ancestor != null) {
          string candidate = Path.Combine(ancestor.FullName, "testdata");
          if (Directory.Exists(candidate)) {
            testDataDirectory = candidate;
            return candidate;
          }
          ancestor = ancestor.Parent;
        }
        // TODO(jonskeet): Come up with a better exception to throw
        throw new Exception("Unable to find directory containing test files");
      }
    }

    internal static ByteString GoldenMessage {
      get {
        if (goldenMessage == null) {
          goldenMessage = ReadBytesFromFile("golden_message");
        }
        return goldenMessage;
      }
    }

    /// <summary>
    /// Creates an unmodifiable ExtensionRegistry containing all the extensions
    /// of TestAllExtensions.
    /// </summary>
    /// <returns></returns>
    internal static ExtensionRegistry CreateExtensionRegistry() {
      ExtensionRegistry registry = ExtensionRegistry.CreateInstance();
      RegisterAllExtensions(registry);
      return registry.AsReadOnly();
    }

    /// <summary>
    /// Registers all of the extensions in TestAllExtensions with the given
    /// ExtensionRegistry.
    /// </summary>
    internal static void RegisterAllExtensions(ExtensionRegistry registry) {
      registry.Add(UnitTestProtoFile.OptionalInt32Extension);
      registry.Add(UnitTestProtoFile.OptionalInt64Extension);
      registry.Add(UnitTestProtoFile.OptionalUint32Extension);
      registry.Add(UnitTestProtoFile.OptionalUint64Extension);
      registry.Add(UnitTestProtoFile.OptionalSint32Extension);
      registry.Add(UnitTestProtoFile.OptionalSint64Extension);
      registry.Add(UnitTestProtoFile.OptionalFixed32Extension);
      registry.Add(UnitTestProtoFile.OptionalFixed64Extension);
      registry.Add(UnitTestProtoFile.OptionalSfixed32Extension);
      registry.Add(UnitTestProtoFile.OptionalSfixed64Extension);
      registry.Add(UnitTestProtoFile.OptionalFloatExtension);
      registry.Add(UnitTestProtoFile.OptionalDoubleExtension);
      registry.Add(UnitTestProtoFile.OptionalBoolExtension);
      registry.Add(UnitTestProtoFile.OptionalStringExtension);
      registry.Add(UnitTestProtoFile.OptionalBytesExtension);
      registry.Add(UnitTestProtoFile.OptionalGroupExtension);
      registry.Add(UnitTestProtoFile.OptionalNestedMessageExtension);
      registry.Add(UnitTestProtoFile.OptionalForeignMessageExtension);
      registry.Add(UnitTestProtoFile.OptionalImportMessageExtension);
      registry.Add(UnitTestProtoFile.OptionalNestedEnumExtension);
      registry.Add(UnitTestProtoFile.OptionalForeignEnumExtension);
      registry.Add(UnitTestProtoFile.OptionalImportEnumExtension);
      registry.Add(UnitTestProtoFile.OptionalStringPieceExtension);
      registry.Add(UnitTestProtoFile.OptionalCordExtension);

      registry.Add(UnitTestProtoFile.RepeatedInt32Extension);
      registry.Add(UnitTestProtoFile.RepeatedInt64Extension);
      registry.Add(UnitTestProtoFile.RepeatedUint32Extension);
      registry.Add(UnitTestProtoFile.RepeatedUint64Extension);
      registry.Add(UnitTestProtoFile.RepeatedSint32Extension);
      registry.Add(UnitTestProtoFile.RepeatedSint64Extension);
      registry.Add(UnitTestProtoFile.RepeatedFixed32Extension);
      registry.Add(UnitTestProtoFile.RepeatedFixed64Extension);
      registry.Add(UnitTestProtoFile.RepeatedSfixed32Extension);
      registry.Add(UnitTestProtoFile.RepeatedSfixed64Extension);
      registry.Add(UnitTestProtoFile.RepeatedFloatExtension);
      registry.Add(UnitTestProtoFile.RepeatedDoubleExtension);
      registry.Add(UnitTestProtoFile.RepeatedBoolExtension);
      registry.Add(UnitTestProtoFile.RepeatedStringExtension);
      registry.Add(UnitTestProtoFile.RepeatedBytesExtension);
      registry.Add(UnitTestProtoFile.RepeatedGroupExtension);
      registry.Add(UnitTestProtoFile.RepeatedNestedMessageExtension);
      registry.Add(UnitTestProtoFile.RepeatedForeignMessageExtension);
      registry.Add(UnitTestProtoFile.RepeatedImportMessageExtension);
      registry.Add(UnitTestProtoFile.RepeatedNestedEnumExtension);
      registry.Add(UnitTestProtoFile.RepeatedForeignEnumExtension);
      registry.Add(UnitTestProtoFile.RepeatedImportEnumExtension);
      registry.Add(UnitTestProtoFile.RepeatedStringPieceExtension);
      registry.Add(UnitTestProtoFile.RepeatedCordExtension);

      registry.Add(UnitTestProtoFile.DefaultInt32Extension);
      registry.Add(UnitTestProtoFile.DefaultInt64Extension);
      registry.Add(UnitTestProtoFile.DefaultUint32Extension);
      registry.Add(UnitTestProtoFile.DefaultUint64Extension);
      registry.Add(UnitTestProtoFile.DefaultSint32Extension);
      registry.Add(UnitTestProtoFile.DefaultSint64Extension);
      registry.Add(UnitTestProtoFile.DefaultFixed32Extension);
      registry.Add(UnitTestProtoFile.DefaultFixed64Extension);
      registry.Add(UnitTestProtoFile.DefaultSfixed32Extension);
      registry.Add(UnitTestProtoFile.DefaultSfixed64Extension);
      registry.Add(UnitTestProtoFile.DefaultFloatExtension);
      registry.Add(UnitTestProtoFile.DefaultDoubleExtension);
      registry.Add(UnitTestProtoFile.DefaultBoolExtension);
      registry.Add(UnitTestProtoFile.DefaultStringExtension);
      registry.Add(UnitTestProtoFile.DefaultBytesExtension);
      registry.Add(UnitTestProtoFile.DefaultNestedEnumExtension);
      registry.Add(UnitTestProtoFile.DefaultForeignEnumExtension);
      registry.Add(UnitTestProtoFile.DefaultImportEnumExtension);
      registry.Add(UnitTestProtoFile.DefaultStringPieceExtension);
      registry.Add(UnitTestProtoFile.DefaultCordExtension);

      registry.Add(UnitTestProtoFile.PackedInt32Extension);
      registry.Add(UnitTestProtoFile.PackedInt64Extension);
      registry.Add(UnitTestProtoFile.PackedUint32Extension);
      registry.Add(UnitTestProtoFile.PackedUint64Extension);
      registry.Add(UnitTestProtoFile.PackedSint32Extension);
      registry.Add(UnitTestProtoFile.PackedSint64Extension);
      registry.Add(UnitTestProtoFile.PackedFixed32Extension);
      registry.Add(UnitTestProtoFile.PackedFixed64Extension);
      registry.Add(UnitTestProtoFile.PackedSfixed32Extension);
      registry.Add(UnitTestProtoFile.PackedSfixed64Extension);
      registry.Add(UnitTestProtoFile.PackedFloatExtension);
      registry.Add(UnitTestProtoFile.PackedDoubleExtension);
      registry.Add(UnitTestProtoFile.PackedBoolExtension);
      registry.Add(UnitTestProtoFile.PackedEnumExtension);
    }

    internal static string ReadTextFromFile(string filePath) {
      return ReadBytesFromFile(filePath).ToStringUtf8();
    }

    internal static ByteString ReadBytesFromFile(String filename) {
      byte[] data = File.ReadAllBytes(Path.Combine(TestDataDirectory, filename));
      return ByteString.CopyFrom(data);
    }

    /// <summary>
    /// Helper to convert a String to ByteString.
    /// </summary>
    internal static ByteString ToBytes(String str) {
      return ByteString.CopyFrom(Encoding.UTF8.GetBytes(str));
    }

    internal static TestAllTypes GetAllSet() {
      TestAllTypes.Builder builder = TestAllTypes.CreateBuilder();
      SetAllFields(builder);
      return builder.Build();
    }

    /// <summary>
    /// Sets every field of the specified message to the values expected by
    /// AssertAllFieldsSet.
    /// </summary>
    internal static void SetAllFields(TestAllTypes.Builder message) {
      message.SetOptionalInt32(101);
      message.SetOptionalInt64(102);
      message.SetOptionalUint32(103);
      message.SetOptionalUint64(104);
      message.SetOptionalSint32(105);
      message.SetOptionalSint64(106);
      message.SetOptionalFixed32(107);
      message.SetOptionalFixed64(108);
      message.SetOptionalSfixed32(109);
      message.SetOptionalSfixed64(110);
      message.SetOptionalFloat(111);
      message.SetOptionalDouble(112);
      message.SetOptionalBool(true);
      message.SetOptionalString("115");
      message.SetOptionalBytes(ToBytes("116"));
      
      message.SetOptionalGroup(TestAllTypes.Types.OptionalGroup.CreateBuilder().SetA(117).Build());
      message.SetOptionalNestedMessage(TestAllTypes.Types.NestedMessage.CreateBuilder().SetBb(118).Build());
      message.SetOptionalForeignMessage(ForeignMessage.CreateBuilder().SetC(119).Build());
      message.SetOptionalImportMessage(ImportMessage.CreateBuilder().SetD(120).Build());

      message.SetOptionalNestedEnum(TestAllTypes.Types.NestedEnum.BAZ);
      message.SetOptionalForeignEnum(ForeignEnum.FOREIGN_BAZ);
      message.SetOptionalImportEnum(ImportEnum.IMPORT_BAZ);

      message.SetOptionalStringPiece("124");
      message.SetOptionalCord("125");

      // -----------------------------------------------------------------

      message.AddRepeatedInt32(201);
      message.AddRepeatedInt64(202);
      message.AddRepeatedUint32(203);
      message.AddRepeatedUint64(204);
      message.AddRepeatedSint32(205);
      message.AddRepeatedSint64(206);
      message.AddRepeatedFixed32(207);
      message.AddRepeatedFixed64(208);
      message.AddRepeatedSfixed32(209);
      message.AddRepeatedSfixed64(210);
      message.AddRepeatedFloat(211);
      message.AddRepeatedDouble(212);
      message.AddRepeatedBool(true);
      message.AddRepeatedString("215");
      message.AddRepeatedBytes(ToBytes("216"));

      message.AddRepeatedGroup(TestAllTypes.Types.RepeatedGroup.CreateBuilder().SetA(217).Build());
      message.AddRepeatedNestedMessage(TestAllTypes.Types.NestedMessage.CreateBuilder().SetBb(218).Build());
      message.AddRepeatedForeignMessage(ForeignMessage.CreateBuilder().SetC(219).Build());
      message.AddRepeatedImportMessage(ImportMessage.CreateBuilder().SetD(220).Build());

      message.AddRepeatedNestedEnum(TestAllTypes.Types.NestedEnum.BAR);
      message.AddRepeatedForeignEnum(ForeignEnum.FOREIGN_BAR);
      message.AddRepeatedImportEnum(ImportEnum.IMPORT_BAR);

      message.AddRepeatedStringPiece("224");
      message.AddRepeatedCord("225");

      // Add a second one of each field.
      message.AddRepeatedInt32(301);
      message.AddRepeatedInt64(302);
      message.AddRepeatedUint32(303);
      message.AddRepeatedUint64(304);
      message.AddRepeatedSint32(305);
      message.AddRepeatedSint64(306);
      message.AddRepeatedFixed32(307);
      message.AddRepeatedFixed64(308);
      message.AddRepeatedSfixed32(309);
      message.AddRepeatedSfixed64(310);
      message.AddRepeatedFloat(311);
      message.AddRepeatedDouble(312);
      message.AddRepeatedBool(false);
      message.AddRepeatedString("315");
      message.AddRepeatedBytes(ToBytes("316"));

      message.AddRepeatedGroup(TestAllTypes.Types.RepeatedGroup.CreateBuilder().SetA(317).Build());
      message.AddRepeatedNestedMessage(TestAllTypes.Types.NestedMessage.CreateBuilder().SetBb(318).Build());
      message.AddRepeatedForeignMessage(ForeignMessage.CreateBuilder().SetC(319).Build());
      message.AddRepeatedImportMessage(ImportMessage.CreateBuilder().SetD(320).Build());

      message.AddRepeatedNestedEnum(TestAllTypes.Types.NestedEnum.BAZ);
      message.AddRepeatedForeignEnum(ForeignEnum.FOREIGN_BAZ);
      message.AddRepeatedImportEnum(ImportEnum.IMPORT_BAZ);

      message.AddRepeatedStringPiece("324");
      message.AddRepeatedCord("325");

      // -----------------------------------------------------------------

      message.SetDefaultInt32(401);
      message.SetDefaultInt64(402);
      message.SetDefaultUint32(403);
      message.SetDefaultUint64(404);
      message.SetDefaultSint32(405);
      message.SetDefaultSint64(406);
      message.SetDefaultFixed32(407);
      message.SetDefaultFixed64(408);
      message.SetDefaultSfixed32(409);
      message.SetDefaultSfixed64(410);
      message.SetDefaultFloat(411);
      message.SetDefaultDouble(412);
      message.SetDefaultBool(false);
      message.SetDefaultString("415");
      message.SetDefaultBytes(ToBytes("416"));
      
      message.SetDefaultNestedEnum(TestAllTypes.Types.NestedEnum.FOO);
      message.SetDefaultForeignEnum(ForeignEnum.FOREIGN_FOO);
      message.SetDefaultImportEnum(ImportEnum.IMPORT_FOO);

      message.SetDefaultStringPiece("424");
      message.SetDefaultCord("425");
    }

    /// <summary>
    /// Asserts that all fields of the specified message are set to the values
    /// assigned by SetAllFields.
    /// </summary>
    internal static void AssertAllFieldsSet(TestAllTypes message) {
      Assert.IsTrue(message.HasOptionalInt32);
      Assert.IsTrue(message.HasOptionalInt64);
      Assert.IsTrue(message.HasOptionalUint32);
      Assert.IsTrue(message.HasOptionalUint64);
      Assert.IsTrue(message.HasOptionalSint32);
      Assert.IsTrue(message.HasOptionalSint64);
      Assert.IsTrue(message.HasOptionalFixed32);
      Assert.IsTrue(message.HasOptionalFixed64);
      Assert.IsTrue(message.HasOptionalSfixed32);
      Assert.IsTrue(message.HasOptionalSfixed64);
      Assert.IsTrue(message.HasOptionalFloat);
      Assert.IsTrue(message.HasOptionalDouble);
      Assert.IsTrue(message.HasOptionalBool);
      Assert.IsTrue(message.HasOptionalString);
      Assert.IsTrue(message.HasOptionalBytes);

      Assert.IsTrue(message.HasOptionalGroup);
      Assert.IsTrue(message.HasOptionalNestedMessage);
      Assert.IsTrue(message.HasOptionalForeignMessage);
      Assert.IsTrue(message.HasOptionalImportMessage);

      Assert.IsTrue(message.OptionalGroup.HasA);
      Assert.IsTrue(message.OptionalNestedMessage.HasBb);
      Assert.IsTrue(message.OptionalForeignMessage.HasC);
      Assert.IsTrue(message.OptionalImportMessage.HasD);

      Assert.IsTrue(message.HasOptionalNestedEnum);
      Assert.IsTrue(message.HasOptionalForeignEnum);
      Assert.IsTrue(message.HasOptionalImportEnum);

      Assert.IsTrue(message.HasOptionalStringPiece);
      Assert.IsTrue(message.HasOptionalCord);

      Assert.AreEqual(101, message.OptionalInt32);
      Assert.AreEqual(102, message.OptionalInt64);
      Assert.AreEqual(103, message.OptionalUint32);
      Assert.AreEqual(104, message.OptionalUint64);
      Assert.AreEqual(105, message.OptionalSint32);
      Assert.AreEqual(106, message.OptionalSint64);
      Assert.AreEqual(107, message.OptionalFixed32);
      Assert.AreEqual(108, message.OptionalFixed64);
      Assert.AreEqual(109, message.OptionalSfixed32);
      Assert.AreEqual(110, message.OptionalSfixed64);
      Assert.AreEqual(111, message.OptionalFloat);
      Assert.AreEqual(112, message.OptionalDouble);
      Assert.AreEqual(true, message.OptionalBool);
      Assert.AreEqual("115", message.OptionalString);
      Assert.AreEqual(ToBytes("116"), message.OptionalBytes);

      Assert.AreEqual(117, message.OptionalGroup.A);
      Assert.AreEqual(118, message.OptionalNestedMessage.Bb);
      Assert.AreEqual(119, message.OptionalForeignMessage.C);
      Assert.AreEqual(120, message.OptionalImportMessage.D);

      Assert.AreEqual(TestAllTypes.Types.NestedEnum.BAZ, message.OptionalNestedEnum);
      Assert.AreEqual(ForeignEnum.FOREIGN_BAZ, message.OptionalForeignEnum);
      Assert.AreEqual(ImportEnum.IMPORT_BAZ, message.OptionalImportEnum);

      Assert.AreEqual("124", message.OptionalStringPiece);
      Assert.AreEqual("125", message.OptionalCord);

      // -----------------------------------------------------------------

      Assert.AreEqual(2, message.RepeatedInt32Count);
      Assert.AreEqual(2, message.RepeatedInt64Count);
      Assert.AreEqual(2, message.RepeatedUint32Count);
      Assert.AreEqual(2, message.RepeatedUint64Count);
      Assert.AreEqual(2, message.RepeatedSint32Count);
      Assert.AreEqual(2, message.RepeatedSint64Count);
      Assert.AreEqual(2, message.RepeatedFixed32Count);
      Assert.AreEqual(2, message.RepeatedFixed64Count);
      Assert.AreEqual(2, message.RepeatedSfixed32Count);
      Assert.AreEqual(2, message.RepeatedSfixed64Count);
      Assert.AreEqual(2, message.RepeatedFloatCount);
      Assert.AreEqual(2, message.RepeatedDoubleCount);
      Assert.AreEqual(2, message.RepeatedBoolCount);
      Assert.AreEqual(2, message.RepeatedStringCount);
      Assert.AreEqual(2, message.RepeatedBytesCount);

      Assert.AreEqual(2, message.RepeatedGroupCount         );
      Assert.AreEqual(2, message.RepeatedNestedMessageCount );
      Assert.AreEqual(2, message.RepeatedForeignMessageCount);
      Assert.AreEqual(2, message.RepeatedImportMessageCount );
      Assert.AreEqual(2, message.RepeatedNestedEnumCount    );
      Assert.AreEqual(2, message.RepeatedForeignEnumCount   );
      Assert.AreEqual(2, message.RepeatedImportEnumCount    );

      Assert.AreEqual(2, message.RepeatedStringPieceCount);
      Assert.AreEqual(2, message.RepeatedCordCount);

      Assert.AreEqual(201, message.GetRepeatedInt32(0));
      Assert.AreEqual(202, message.GetRepeatedInt64(0));
      Assert.AreEqual(203, message.GetRepeatedUint32(0));
      Assert.AreEqual(204, message.GetRepeatedUint64(0));
      Assert.AreEqual(205, message.GetRepeatedSint32(0));
      Assert.AreEqual(206, message.GetRepeatedSint64(0));
      Assert.AreEqual(207, message.GetRepeatedFixed32(0));
      Assert.AreEqual(208, message.GetRepeatedFixed64(0));
      Assert.AreEqual(209, message.GetRepeatedSfixed32(0));
      Assert.AreEqual(210, message.GetRepeatedSfixed64(0));
      Assert.AreEqual(211, message.GetRepeatedFloat(0));
      Assert.AreEqual(212, message.GetRepeatedDouble(0));
      Assert.AreEqual(true , message.GetRepeatedBool(0));
      Assert.AreEqual("215", message.GetRepeatedString(0));
      Assert.AreEqual(ToBytes("216"), message.GetRepeatedBytes(0));

      Assert.AreEqual(217, message.GetRepeatedGroup(0).A);
      Assert.AreEqual(218, message.GetRepeatedNestedMessage (0).Bb);
      Assert.AreEqual(219, message.GetRepeatedForeignMessage(0).C);
      Assert.AreEqual(220, message.GetRepeatedImportMessage (0).D);

      Assert.AreEqual(TestAllTypes.Types.NestedEnum.BAR, message.GetRepeatedNestedEnum (0));
      Assert.AreEqual(ForeignEnum.FOREIGN_BAR, message.GetRepeatedForeignEnum(0));
      Assert.AreEqual(ImportEnum.IMPORT_BAR, message.GetRepeatedImportEnum(0));

      Assert.AreEqual("224", message.GetRepeatedStringPiece(0));
      Assert.AreEqual("225", message.GetRepeatedCord(0));

      Assert.AreEqual(301, message.GetRepeatedInt32   (1));
      Assert.AreEqual(302, message.GetRepeatedInt64   (1));
      Assert.AreEqual(303, message.GetRepeatedUint32  (1));
      Assert.AreEqual(304, message.GetRepeatedUint64  (1));
      Assert.AreEqual(305, message.GetRepeatedSint32  (1));
      Assert.AreEqual(306, message.GetRepeatedSint64  (1));
      Assert.AreEqual(307, message.GetRepeatedFixed32 (1));
      Assert.AreEqual(308, message.GetRepeatedFixed64 (1));
      Assert.AreEqual(309, message.GetRepeatedSfixed32(1));
      Assert.AreEqual(310, message.GetRepeatedSfixed64(1));
      Assert.AreEqual(311, message.GetRepeatedFloat   (1), 0.0);
      Assert.AreEqual(312, message.GetRepeatedDouble  (1), 0.0);
      Assert.AreEqual(false, message.GetRepeatedBool    (1));
      Assert.AreEqual("315", message.GetRepeatedString  (1));
      Assert.AreEqual(ToBytes("316"), message.GetRepeatedBytes(1));

      Assert.AreEqual(317, message.GetRepeatedGroup         (1).A);
      Assert.AreEqual(318, message.GetRepeatedNestedMessage (1).Bb);
      Assert.AreEqual(319, message.GetRepeatedForeignMessage(1).C);
      Assert.AreEqual(320, message.GetRepeatedImportMessage (1).D);

      Assert.AreEqual(TestAllTypes.Types.NestedEnum.BAZ, message.GetRepeatedNestedEnum (1));
      Assert.AreEqual(ForeignEnum.FOREIGN_BAZ, message.GetRepeatedForeignEnum(1));
      Assert.AreEqual(ImportEnum.IMPORT_BAZ, message.GetRepeatedImportEnum(1));

      Assert.AreEqual("324", message.GetRepeatedStringPiece(1));
      Assert.AreEqual("325", message.GetRepeatedCord(1));

      // -----------------------------------------------------------------

      Assert.IsTrue(message.HasDefaultInt32   );
      Assert.IsTrue(message.HasDefaultInt64   );
      Assert.IsTrue(message.HasDefaultUint32  );
      Assert.IsTrue(message.HasDefaultUint64  );
      Assert.IsTrue(message.HasDefaultSint32  );
      Assert.IsTrue(message.HasDefaultSint64  );
      Assert.IsTrue(message.HasDefaultFixed32 );
      Assert.IsTrue(message.HasDefaultFixed64 );
      Assert.IsTrue(message.HasDefaultSfixed32);
      Assert.IsTrue(message.HasDefaultSfixed64);
      Assert.IsTrue(message.HasDefaultFloat   );
      Assert.IsTrue(message.HasDefaultDouble  );
      Assert.IsTrue(message.HasDefaultBool    );
      Assert.IsTrue(message.HasDefaultString  );
      Assert.IsTrue(message.HasDefaultBytes   );

      Assert.IsTrue(message.HasDefaultNestedEnum );
      Assert.IsTrue(message.HasDefaultForeignEnum);
      Assert.IsTrue(message.HasDefaultImportEnum );

      Assert.IsTrue(message.HasDefaultStringPiece);
      Assert.IsTrue(message.HasDefaultCord);

      Assert.AreEqual(401, message.DefaultInt32);
      Assert.AreEqual(402, message.DefaultInt64);
      Assert.AreEqual(403, message.DefaultUint32);
      Assert.AreEqual(404, message.DefaultUint64);
      Assert.AreEqual(405, message.DefaultSint32);
      Assert.AreEqual(406, message.DefaultSint64);
      Assert.AreEqual(407, message.DefaultFixed32);
      Assert.AreEqual(408, message.DefaultFixed64);
      Assert.AreEqual(409, message.DefaultSfixed32);
      Assert.AreEqual(410, message.DefaultSfixed64);
      Assert.AreEqual(411, message.DefaultFloat);
      Assert.AreEqual(412, message.DefaultDouble);
      Assert.AreEqual(false, message.DefaultBool    );
      Assert.AreEqual("415", message.DefaultString  );
      Assert.AreEqual(ToBytes("416"), message.DefaultBytes);

      Assert.AreEqual(TestAllTypes.Types.NestedEnum.FOO, message.DefaultNestedEnum);
      Assert.AreEqual(ForeignEnum.FOREIGN_FOO, message.DefaultForeignEnum);
      Assert.AreEqual(ImportEnum.IMPORT_FOO, message.DefaultImportEnum);

      Assert.AreEqual("424", message.DefaultStringPiece);
      Assert.AreEqual("425", message.DefaultCord);
    }

    internal static void AssertClear(TestAllTypes message) {
      // HasBlah() should initially be false for all optional fields.
      Assert.IsFalse(message.HasOptionalInt32);
      Assert.IsFalse(message.HasOptionalInt64);
      Assert.IsFalse(message.HasOptionalUint32);
      Assert.IsFalse(message.HasOptionalUint64);
      Assert.IsFalse(message.HasOptionalSint32);
      Assert.IsFalse(message.HasOptionalSint64);
      Assert.IsFalse(message.HasOptionalFixed32);
      Assert.IsFalse(message.HasOptionalFixed64);
      Assert.IsFalse(message.HasOptionalSfixed32);
      Assert.IsFalse(message.HasOptionalSfixed64);
      Assert.IsFalse(message.HasOptionalFloat);
      Assert.IsFalse(message.HasOptionalDouble);
      Assert.IsFalse(message.HasOptionalBool);
      Assert.IsFalse(message.HasOptionalString);
      Assert.IsFalse(message.HasOptionalBytes);

      Assert.IsFalse(message.HasOptionalGroup);
      Assert.IsFalse(message.HasOptionalNestedMessage);
      Assert.IsFalse(message.HasOptionalForeignMessage);
      Assert.IsFalse(message.HasOptionalImportMessage);

      Assert.IsFalse(message.HasOptionalNestedEnum);
      Assert.IsFalse(message.HasOptionalForeignEnum);
      Assert.IsFalse(message.HasOptionalImportEnum);

      Assert.IsFalse(message.HasOptionalStringPiece);
      Assert.IsFalse(message.HasOptionalCord);

      // Optional fields without defaults are set to zero or something like it.
      Assert.AreEqual(0, message.OptionalInt32);
      Assert.AreEqual(0, message.OptionalInt64);
      Assert.AreEqual(0, message.OptionalUint32);
      Assert.AreEqual(0, message.OptionalUint64);
      Assert.AreEqual(0, message.OptionalSint32);
      Assert.AreEqual(0, message.OptionalSint64);
      Assert.AreEqual(0, message.OptionalFixed32);
      Assert.AreEqual(0, message.OptionalFixed64);
      Assert.AreEqual(0, message.OptionalSfixed32);
      Assert.AreEqual(0, message.OptionalSfixed64);
      Assert.AreEqual(0, message.OptionalFloat);
      Assert.AreEqual(0, message.OptionalDouble);
      Assert.AreEqual(false, message.OptionalBool);
      Assert.AreEqual("", message.OptionalString);
      Assert.AreEqual(ByteString.Empty, message.OptionalBytes);

      // Embedded messages should also be clear.
      Assert.IsFalse(message.OptionalGroup.HasA);
      Assert.IsFalse(message.OptionalNestedMessage.HasBb);
      Assert.IsFalse(message.OptionalForeignMessage.HasC);
      Assert.IsFalse(message.OptionalImportMessage.HasD);

      Assert.AreEqual(0, message.OptionalGroup.A);
      Assert.AreEqual(0, message.OptionalNestedMessage.Bb);
      Assert.AreEqual(0, message.OptionalForeignMessage.C);
      Assert.AreEqual(0, message.OptionalImportMessage.D);

      // Enums without defaults are set to the first value in the enum.
      Assert.AreEqual(TestAllTypes.Types.NestedEnum.FOO, message.OptionalNestedEnum);
      Assert.AreEqual(ForeignEnum.FOREIGN_FOO, message.OptionalForeignEnum);
      Assert.AreEqual(ImportEnum.IMPORT_FOO, message.OptionalImportEnum);

      Assert.AreEqual("", message.OptionalStringPiece);
      Assert.AreEqual("", message.OptionalCord);

      // Repeated fields are empty.
      Assert.AreEqual(0, message.RepeatedInt32Count);
      Assert.AreEqual(0, message.RepeatedInt64Count);
      Assert.AreEqual(0, message.RepeatedUint32Count);
      Assert.AreEqual(0, message.RepeatedUint64Count);
      Assert.AreEqual(0, message.RepeatedSint32Count);
      Assert.AreEqual(0, message.RepeatedSint64Count);
      Assert.AreEqual(0, message.RepeatedFixed32Count);
      Assert.AreEqual(0, message.RepeatedFixed64Count);
      Assert.AreEqual(0, message.RepeatedSfixed32Count);
      Assert.AreEqual(0, message.RepeatedSfixed64Count);
      Assert.AreEqual(0, message.RepeatedFloatCount);
      Assert.AreEqual(0, message.RepeatedDoubleCount);
      Assert.AreEqual(0, message.RepeatedBoolCount);
      Assert.AreEqual(0, message.RepeatedStringCount);
      Assert.AreEqual(0, message.RepeatedBytesCount);

      Assert.AreEqual(0, message.RepeatedGroupCount);
      Assert.AreEqual(0, message.RepeatedNestedMessageCount);
      Assert.AreEqual(0, message.RepeatedForeignMessageCount);
      Assert.AreEqual(0, message.RepeatedImportMessageCount);
      Assert.AreEqual(0, message.RepeatedNestedEnumCount);
      Assert.AreEqual(0, message.RepeatedForeignEnumCount);
      Assert.AreEqual(0, message.RepeatedImportEnumCount);

      Assert.AreEqual(0, message.RepeatedStringPieceCount);
      Assert.AreEqual(0, message.RepeatedCordCount);

      // HasBlah() should also be false for all default fields.
      Assert.IsFalse(message.HasDefaultInt32);
      Assert.IsFalse(message.HasDefaultInt64);
      Assert.IsFalse(message.HasDefaultUint32);
      Assert.IsFalse(message.HasDefaultUint64);
      Assert.IsFalse(message.HasDefaultSint32);
      Assert.IsFalse(message.HasDefaultSint64);
      Assert.IsFalse(message.HasDefaultFixed32);
      Assert.IsFalse(message.HasDefaultFixed64);
      Assert.IsFalse(message.HasDefaultSfixed32);
      Assert.IsFalse(message.HasDefaultSfixed64);
      Assert.IsFalse(message.HasDefaultFloat);
      Assert.IsFalse(message.HasDefaultDouble);
      Assert.IsFalse(message.HasDefaultBool);
      Assert.IsFalse(message.HasDefaultString);
      Assert.IsFalse(message.HasDefaultBytes);

      Assert.IsFalse(message.HasDefaultNestedEnum);
      Assert.IsFalse(message.HasDefaultForeignEnum);
      Assert.IsFalse(message.HasDefaultImportEnum);

      Assert.IsFalse(message.HasDefaultStringPiece);
      Assert.IsFalse(message.HasDefaultCord);

      // Fields with defaults have their default values (duh).
      Assert.AreEqual(41, message.DefaultInt32);
      Assert.AreEqual(42, message.DefaultInt64);
      Assert.AreEqual(43, message.DefaultUint32);
      Assert.AreEqual(44, message.DefaultUint64);
      Assert.AreEqual(-45, message.DefaultSint32);
      Assert.AreEqual(46, message.DefaultSint64);
      Assert.AreEqual(47, message.DefaultFixed32);
      Assert.AreEqual(48, message.DefaultFixed64);
      Assert.AreEqual(49, message.DefaultSfixed32);
      Assert.AreEqual(-50, message.DefaultSfixed64);
      Assert.AreEqual(51.5, message.DefaultFloat, 0.0);
      Assert.AreEqual(52e3, message.DefaultDouble, 0.0);
      Assert.AreEqual(true, message.DefaultBool);
      Assert.AreEqual("hello", message.DefaultString);
      Assert.AreEqual(ToBytes("world"), message.DefaultBytes);

      Assert.AreEqual(TestAllTypes.Types.NestedEnum.BAR, message.DefaultNestedEnum);
      Assert.AreEqual(ForeignEnum.FOREIGN_BAR, message.DefaultForeignEnum);
      Assert.AreEqual(ImportEnum.IMPORT_BAR, message.DefaultImportEnum);

      Assert.AreEqual("abc", message.DefaultStringPiece);
      Assert.AreEqual("123", message.DefaultCord);
    }

    /// <summary>
    /// Get a TestAllExtensions with all fields set as they would be by
    /// SetAllExtensions(TestAllExtensions.Builder).
    /// </summary>
    internal static TestAllExtensions GetAllExtensionsSet() {
      TestAllExtensions.Builder builder = TestAllExtensions.CreateBuilder();
      SetAllExtensions(builder);
      return builder.Build();
    }

    public static TestPackedTypes GetPackedSet() {
      TestPackedTypes.Builder builder = TestPackedTypes.CreateBuilder();
      SetPackedFields(builder);
      return builder.Build();
    }

    public static TestPackedExtensions GetPackedExtensionsSet() {
      TestPackedExtensions.Builder builder = TestPackedExtensions.CreateBuilder();
      SetPackedExtensions(builder);
      return builder.Build();
    }

    /// <summary>
    /// Sets every field of the specified builder to the values expected by
    /// AssertAllExtensionsSet.
    /// </summary>
    internal static void SetAllExtensions(TestAllExtensions.Builder message) {
      message.SetExtension(UnitTestProtoFile.OptionalInt32Extension, 101);
      message.SetExtension(UnitTestProtoFile.OptionalInt64Extension, 102L);
      message.SetExtension(UnitTestProtoFile.OptionalUint32Extension, 103U);
      message.SetExtension(UnitTestProtoFile.OptionalUint64Extension, 104UL);
      message.SetExtension(UnitTestProtoFile.OptionalSint32Extension, 105);
      message.SetExtension(UnitTestProtoFile.OptionalSint64Extension, 106L);
      message.SetExtension(UnitTestProtoFile.OptionalFixed32Extension, 107U);
      message.SetExtension(UnitTestProtoFile.OptionalFixed64Extension, 108UL);
      message.SetExtension(UnitTestProtoFile.OptionalSfixed32Extension, 109);
      message.SetExtension(UnitTestProtoFile.OptionalSfixed64Extension, 110L);
      message.SetExtension(UnitTestProtoFile.OptionalFloatExtension, 111F);
      message.SetExtension(UnitTestProtoFile.OptionalDoubleExtension, 112D);
      message.SetExtension(UnitTestProtoFile.OptionalBoolExtension, true);
      message.SetExtension(UnitTestProtoFile.OptionalStringExtension, "115");
      message.SetExtension(UnitTestProtoFile.OptionalBytesExtension, ToBytes("116"));

      message.SetExtension(UnitTestProtoFile.OptionalGroupExtension, OptionalGroup_extension.CreateBuilder().SetA(117).Build());
      message.SetExtension(UnitTestProtoFile.OptionalNestedMessageExtension, TestAllTypes.Types.NestedMessage.CreateBuilder().SetBb(118).Build());
      message.SetExtension(UnitTestProtoFile.OptionalForeignMessageExtension, ForeignMessage.CreateBuilder().SetC(119).Build());
      message.SetExtension(UnitTestProtoFile.OptionalImportMessageExtension, ImportMessage.CreateBuilder().SetD(120).Build());

      message.SetExtension(UnitTestProtoFile.OptionalNestedEnumExtension, TestAllTypes.Types.NestedEnum.BAZ);
      message.SetExtension(UnitTestProtoFile.OptionalForeignEnumExtension, ForeignEnum.FOREIGN_BAZ);
      message.SetExtension(UnitTestProtoFile.OptionalImportEnumExtension, ImportEnum.IMPORT_BAZ);

      message.SetExtension(UnitTestProtoFile.OptionalStringPieceExtension, "124");
      message.SetExtension(UnitTestProtoFile.OptionalCordExtension, "125");

      // -----------------------------------------------------------------

      message.AddExtension(UnitTestProtoFile.RepeatedInt32Extension, 201);
      message.AddExtension(UnitTestProtoFile.RepeatedInt64Extension, 202L);
      message.AddExtension(UnitTestProtoFile.RepeatedUint32Extension, 203U);
      message.AddExtension(UnitTestProtoFile.RepeatedUint64Extension, 204UL);
      message.AddExtension(UnitTestProtoFile.RepeatedSint32Extension, 205);
      message.AddExtension(UnitTestProtoFile.RepeatedSint64Extension, 206L);
      message.AddExtension(UnitTestProtoFile.RepeatedFixed32Extension, 207U);
      message.AddExtension(UnitTestProtoFile.RepeatedFixed64Extension, 208UL);
      message.AddExtension(UnitTestProtoFile.RepeatedSfixed32Extension, 209);
      message.AddExtension(UnitTestProtoFile.RepeatedSfixed64Extension, 210L);
      message.AddExtension(UnitTestProtoFile.RepeatedFloatExtension, 211F);
      message.AddExtension(UnitTestProtoFile.RepeatedDoubleExtension, 212D);
      message.AddExtension(UnitTestProtoFile.RepeatedBoolExtension, true);
      message.AddExtension(UnitTestProtoFile.RepeatedStringExtension, "215");
      message.AddExtension(UnitTestProtoFile.RepeatedBytesExtension, ToBytes("216"));

      message.AddExtension(UnitTestProtoFile.RepeatedGroupExtension, RepeatedGroup_extension.CreateBuilder().SetA(217).Build());
      message.AddExtension(UnitTestProtoFile.RepeatedNestedMessageExtension, TestAllTypes.Types.NestedMessage.CreateBuilder().SetBb(218).Build());
      message.AddExtension(UnitTestProtoFile.RepeatedForeignMessageExtension, ForeignMessage.CreateBuilder().SetC(219).Build());
      message.AddExtension(UnitTestProtoFile.RepeatedImportMessageExtension, ImportMessage.CreateBuilder().SetD(220).Build());

      message.AddExtension(UnitTestProtoFile.RepeatedNestedEnumExtension, TestAllTypes.Types.NestedEnum.BAR);
      message.AddExtension(UnitTestProtoFile.RepeatedForeignEnumExtension, ForeignEnum.FOREIGN_BAR);
      message.AddExtension(UnitTestProtoFile.RepeatedImportEnumExtension, ImportEnum.IMPORT_BAR);

      message.AddExtension(UnitTestProtoFile.RepeatedStringPieceExtension, "224");
      message.AddExtension(UnitTestProtoFile.RepeatedCordExtension, "225");

      // Add a second one of each field.
      message.AddExtension(UnitTestProtoFile.RepeatedInt32Extension, 301);
      message.AddExtension(UnitTestProtoFile.RepeatedInt64Extension, 302L);
      message.AddExtension(UnitTestProtoFile.RepeatedUint32Extension, 303U);
      message.AddExtension(UnitTestProtoFile.RepeatedUint64Extension, 304UL);
      message.AddExtension(UnitTestProtoFile.RepeatedSint32Extension, 305);
      message.AddExtension(UnitTestProtoFile.RepeatedSint64Extension, 306L);
      message.AddExtension(UnitTestProtoFile.RepeatedFixed32Extension, 307U);
      message.AddExtension(UnitTestProtoFile.RepeatedFixed64Extension, 308UL);
      message.AddExtension(UnitTestProtoFile.RepeatedSfixed32Extension, 309);
      message.AddExtension(UnitTestProtoFile.RepeatedSfixed64Extension, 310L);
      message.AddExtension(UnitTestProtoFile.RepeatedFloatExtension, 311F);
      message.AddExtension(UnitTestProtoFile.RepeatedDoubleExtension, 312D);
      message.AddExtension(UnitTestProtoFile.RepeatedBoolExtension, false);
      message.AddExtension(UnitTestProtoFile.RepeatedStringExtension, "315");
      message.AddExtension(UnitTestProtoFile.RepeatedBytesExtension, ToBytes("316"));

      message.AddExtension(UnitTestProtoFile.RepeatedGroupExtension, RepeatedGroup_extension.CreateBuilder().SetA(317).Build());
      message.AddExtension(UnitTestProtoFile.RepeatedNestedMessageExtension, TestAllTypes.Types.NestedMessage.CreateBuilder().SetBb(318).Build());
      message.AddExtension(UnitTestProtoFile.RepeatedForeignMessageExtension, ForeignMessage.CreateBuilder().SetC(319).Build());
      message.AddExtension(UnitTestProtoFile.RepeatedImportMessageExtension, ImportMessage.CreateBuilder().SetD(320).Build());

      message.AddExtension(UnitTestProtoFile.RepeatedNestedEnumExtension, TestAllTypes.Types.NestedEnum.BAZ);
      message.AddExtension(UnitTestProtoFile.RepeatedForeignEnumExtension, ForeignEnum.FOREIGN_BAZ);
      message.AddExtension(UnitTestProtoFile.RepeatedImportEnumExtension, ImportEnum.IMPORT_BAZ);

      message.AddExtension(UnitTestProtoFile.RepeatedStringPieceExtension, "324");
      message.AddExtension(UnitTestProtoFile.RepeatedCordExtension, "325");

      // -----------------------------------------------------------------

      message.SetExtension(UnitTestProtoFile.DefaultInt32Extension, 401);
      message.SetExtension(UnitTestProtoFile.DefaultInt64Extension, 402L);
      message.SetExtension(UnitTestProtoFile.DefaultUint32Extension, 403U);
      message.SetExtension(UnitTestProtoFile.DefaultUint64Extension, 404UL);
      message.SetExtension(UnitTestProtoFile.DefaultSint32Extension, 405);
      message.SetExtension(UnitTestProtoFile.DefaultSint64Extension, 406L);
      message.SetExtension(UnitTestProtoFile.DefaultFixed32Extension, 407U);
      message.SetExtension(UnitTestProtoFile.DefaultFixed64Extension, 408UL);
      message.SetExtension(UnitTestProtoFile.DefaultSfixed32Extension, 409);
      message.SetExtension(UnitTestProtoFile.DefaultSfixed64Extension, 410L);
      message.SetExtension(UnitTestProtoFile.DefaultFloatExtension, 411F);
      message.SetExtension(UnitTestProtoFile.DefaultDoubleExtension, 412D);
      message.SetExtension(UnitTestProtoFile.DefaultBoolExtension, false);
      message.SetExtension(UnitTestProtoFile.DefaultStringExtension, "415");
      message.SetExtension(UnitTestProtoFile.DefaultBytesExtension, ToBytes("416"));

      message.SetExtension(UnitTestProtoFile.DefaultNestedEnumExtension, TestAllTypes.Types.NestedEnum.FOO);
      message.SetExtension(UnitTestProtoFile.DefaultForeignEnumExtension, ForeignEnum.FOREIGN_FOO);
      message.SetExtension(UnitTestProtoFile.DefaultImportEnumExtension, ImportEnum.IMPORT_FOO);

      message.SetExtension(UnitTestProtoFile.DefaultStringPieceExtension, "424");
      message.SetExtension(UnitTestProtoFile.DefaultCordExtension, "425");
    }

    internal static void ModifyRepeatedFields(TestAllTypes.Builder message) {
      message.SetRepeatedInt32(1, 501);
      message.SetRepeatedInt64(1, 502);
      message.SetRepeatedUint32(1, 503);
      message.SetRepeatedUint64(1, 504);
      message.SetRepeatedSint32(1, 505);
      message.SetRepeatedSint64(1, 506);
      message.SetRepeatedFixed32(1, 507);
      message.SetRepeatedFixed64(1, 508);
      message.SetRepeatedSfixed32(1, 509);
      message.SetRepeatedSfixed64(1, 510);
      message.SetRepeatedFloat(1, 511);
      message.SetRepeatedDouble(1, 512);
      message.SetRepeatedBool(1, true);
      message.SetRepeatedString(1, "515");
      message.SetRepeatedBytes(1, ToBytes("516"));

      message.SetRepeatedGroup(1, TestAllTypes.Types.RepeatedGroup.CreateBuilder().SetA(517).Build());
      message.SetRepeatedNestedMessage(1, TestAllTypes.Types.NestedMessage.CreateBuilder().SetBb(518).Build());
      message.SetRepeatedForeignMessage(1, ForeignMessage.CreateBuilder().SetC(519).Build());
      message.SetRepeatedImportMessage(1, ImportMessage.CreateBuilder().SetD(520).Build());

      message.SetRepeatedNestedEnum(1, TestAllTypes.Types.NestedEnum.FOO);
      message.SetRepeatedForeignEnum(1, ForeignEnum.FOREIGN_FOO);
      message.SetRepeatedImportEnum(1, ImportEnum.IMPORT_FOO);

      message.SetRepeatedStringPiece(1, "524");
      message.SetRepeatedCord(1, "525");
    }

    internal static void AssertRepeatedFieldsModified(TestAllTypes message) {
      // ModifyRepeatedFields only sets the second repeated element of each
      // field.  In addition to verifying this, we also verify that the first
      // element and size were *not* modified.
      Assert.AreEqual(2, message.RepeatedInt32Count);
      Assert.AreEqual(2, message.RepeatedInt64Count);
      Assert.AreEqual(2, message.RepeatedUint32Count);
      Assert.AreEqual(2, message.RepeatedUint64Count);
      Assert.AreEqual(2, message.RepeatedSint32Count);
      Assert.AreEqual(2, message.RepeatedSint64Count);
      Assert.AreEqual(2, message.RepeatedFixed32Count);
      Assert.AreEqual(2, message.RepeatedFixed64Count);
      Assert.AreEqual(2, message.RepeatedSfixed32Count);
      Assert.AreEqual(2, message.RepeatedSfixed64Count);
      Assert.AreEqual(2, message.RepeatedFloatCount);
      Assert.AreEqual(2, message.RepeatedDoubleCount);
      Assert.AreEqual(2, message.RepeatedBoolCount);
      Assert.AreEqual(2, message.RepeatedStringCount);
      Assert.AreEqual(2, message.RepeatedBytesCount);

      Assert.AreEqual(2, message.RepeatedGroupCount);
      Assert.AreEqual(2, message.RepeatedNestedMessageCount);
      Assert.AreEqual(2, message.RepeatedForeignMessageCount);
      Assert.AreEqual(2, message.RepeatedImportMessageCount);
      Assert.AreEqual(2, message.RepeatedNestedEnumCount);
      Assert.AreEqual(2, message.RepeatedForeignEnumCount);
      Assert.AreEqual(2, message.RepeatedImportEnumCount);

      Assert.AreEqual(2, message.RepeatedStringPieceCount);
      Assert.AreEqual(2, message.RepeatedCordCount);

      Assert.AreEqual(201, message.GetRepeatedInt32(0));
      Assert.AreEqual(202L, message.GetRepeatedInt64(0));
      Assert.AreEqual(203U, message.GetRepeatedUint32(0));
      Assert.AreEqual(204UL, message.GetRepeatedUint64(0));
      Assert.AreEqual(205, message.GetRepeatedSint32(0));
      Assert.AreEqual(206L, message.GetRepeatedSint64(0));
      Assert.AreEqual(207U, message.GetRepeatedFixed32(0));
      Assert.AreEqual(208UL, message.GetRepeatedFixed64(0));
      Assert.AreEqual(209, message.GetRepeatedSfixed32(0));
      Assert.AreEqual(210L, message.GetRepeatedSfixed64(0));
      Assert.AreEqual(211F, message.GetRepeatedFloat(0));
      Assert.AreEqual(212D, message.GetRepeatedDouble(0));
      Assert.AreEqual(true, message.GetRepeatedBool(0));
      Assert.AreEqual("215", message.GetRepeatedString(0));
      Assert.AreEqual(ToBytes("216"), message.GetRepeatedBytes(0));

      Assert.AreEqual(217, message.GetRepeatedGroup(0).A);
      Assert.AreEqual(218, message.GetRepeatedNestedMessage(0).Bb);
      Assert.AreEqual(219, message.GetRepeatedForeignMessage(0).C);
      Assert.AreEqual(220, message.GetRepeatedImportMessage(0).D);

      Assert.AreEqual(TestAllTypes.Types.NestedEnum.BAR, message.GetRepeatedNestedEnum(0));
      Assert.AreEqual(ForeignEnum.FOREIGN_BAR, message.GetRepeatedForeignEnum(0));
      Assert.AreEqual(ImportEnum.IMPORT_BAR, message.GetRepeatedImportEnum(0));

      Assert.AreEqual("224", message.GetRepeatedStringPiece(0));
      Assert.AreEqual("225", message.GetRepeatedCord(0));

      // Actually verify the second (modified) elements now.
      Assert.AreEqual(501, message.GetRepeatedInt32(1));
      Assert.AreEqual(502L, message.GetRepeatedInt64(1));
      Assert.AreEqual(503U, message.GetRepeatedUint32(1));
      Assert.AreEqual(504UL, message.GetRepeatedUint64(1));
      Assert.AreEqual(505, message.GetRepeatedSint32(1));
      Assert.AreEqual(506L, message.GetRepeatedSint64(1));
      Assert.AreEqual(507U, message.GetRepeatedFixed32(1));
      Assert.AreEqual(508UL, message.GetRepeatedFixed64(1));
      Assert.AreEqual(509, message.GetRepeatedSfixed32(1));
      Assert.AreEqual(510L, message.GetRepeatedSfixed64(1));
      Assert.AreEqual(511F, message.GetRepeatedFloat(1));
      Assert.AreEqual(512D, message.GetRepeatedDouble(1));
      Assert.AreEqual(true, message.GetRepeatedBool(1));
      Assert.AreEqual("515", message.GetRepeatedString(1));
      Assert.AreEqual(ToBytes("516"), message.GetRepeatedBytes(1));

      Assert.AreEqual(517, message.GetRepeatedGroup(1).A);
      Assert.AreEqual(518, message.GetRepeatedNestedMessage(1).Bb);
      Assert.AreEqual(519, message.GetRepeatedForeignMessage(1).C);
      Assert.AreEqual(520, message.GetRepeatedImportMessage(1).D);

      Assert.AreEqual(TestAllTypes.Types.NestedEnum.FOO, message.GetRepeatedNestedEnum(1));
      Assert.AreEqual(ForeignEnum.FOREIGN_FOO, message.GetRepeatedForeignEnum(1));
      Assert.AreEqual(ImportEnum.IMPORT_FOO, message.GetRepeatedImportEnum(1));

      Assert.AreEqual("524", message.GetRepeatedStringPiece(1));
      Assert.AreEqual("525", message.GetRepeatedCord(1));
    }

    /// <summary>
    /// Helper to assert that sequences are equal.
    /// </summary>
    internal static void AssertEqual<T>(IEnumerable<T> first, IEnumerable<T> second) {
      using (IEnumerator<T> firstEnumerator = first.GetEnumerator()) {
        foreach (T secondElement in second) {
          Assert.IsTrue(firstEnumerator.MoveNext(), "First enumerator ran out of elements too early.");
          Assert.AreEqual(firstEnumerator.Current, secondElement);
        }
        Assert.IsFalse(firstEnumerator.MoveNext(), "Second enumerator ran out of elements too early.");
      }
    }

    internal static void AssertEqualBytes(byte[] expected, byte[] actual) {
      Assert.AreEqual(ByteString.CopyFrom(expected), ByteString.CopyFrom(actual));
    }

    internal static void AssertAllExtensionsSet(TestAllExtensions message) {
      Assert.IsTrue(message.HasExtension(UnitTestProtoFile.OptionalInt32Extension   ));
      Assert.IsTrue(message.HasExtension(UnitTestProtoFile.OptionalInt64Extension   ));
      Assert.IsTrue(message.HasExtension(UnitTestProtoFile.OptionalUint32Extension  ));
      Assert.IsTrue(message.HasExtension(UnitTestProtoFile.OptionalUint64Extension  ));
      Assert.IsTrue(message.HasExtension(UnitTestProtoFile.OptionalSint32Extension  ));
      Assert.IsTrue(message.HasExtension(UnitTestProtoFile.OptionalSint64Extension  ));
      Assert.IsTrue(message.HasExtension(UnitTestProtoFile.OptionalFixed32Extension ));
      Assert.IsTrue(message.HasExtension(UnitTestProtoFile.OptionalFixed64Extension ));
      Assert.IsTrue(message.HasExtension(UnitTestProtoFile.OptionalSfixed32Extension));
      Assert.IsTrue(message.HasExtension(UnitTestProtoFile.OptionalSfixed64Extension));
      Assert.IsTrue(message.HasExtension(UnitTestProtoFile.OptionalFloatExtension   ));
      Assert.IsTrue(message.HasExtension(UnitTestProtoFile.OptionalDoubleExtension  ));
      Assert.IsTrue(message.HasExtension(UnitTestProtoFile.OptionalBoolExtension    ));
      Assert.IsTrue(message.HasExtension(UnitTestProtoFile.OptionalStringExtension  ));
      Assert.IsTrue(message.HasExtension(UnitTestProtoFile.OptionalBytesExtension   ));

      Assert.IsTrue(message.HasExtension(UnitTestProtoFile.OptionalGroupExtension         ));
      Assert.IsTrue(message.HasExtension(UnitTestProtoFile.OptionalNestedMessageExtension ));
      Assert.IsTrue(message.HasExtension(UnitTestProtoFile.OptionalForeignMessageExtension));
      Assert.IsTrue(message.HasExtension(UnitTestProtoFile.OptionalImportMessageExtension ));

      Assert.IsTrue(message.GetExtension(UnitTestProtoFile.OptionalGroupExtension         ).HasA);
      Assert.IsTrue(message.GetExtension(UnitTestProtoFile.OptionalNestedMessageExtension ).HasBb);
      Assert.IsTrue(message.GetExtension(UnitTestProtoFile.OptionalForeignMessageExtension).HasC);
      Assert.IsTrue(message.GetExtension(UnitTestProtoFile.OptionalImportMessageExtension ).HasD);

      Assert.IsTrue(message.HasExtension(UnitTestProtoFile.OptionalNestedEnumExtension ));
      Assert.IsTrue(message.HasExtension(UnitTestProtoFile.OptionalForeignEnumExtension));
      Assert.IsTrue(message.HasExtension(UnitTestProtoFile.OptionalImportEnumExtension ));

      Assert.IsTrue(message.HasExtension(UnitTestProtoFile.OptionalStringPieceExtension));
      Assert.IsTrue(message.HasExtension(UnitTestProtoFile.OptionalCordExtension));

      Assert.AreEqual(101  , message.GetExtension(UnitTestProtoFile.OptionalInt32Extension   ));
      Assert.AreEqual(102L , message.GetExtension(UnitTestProtoFile.OptionalInt64Extension   ));
      Assert.AreEqual(103U  , message.GetExtension(UnitTestProtoFile.OptionalUint32Extension  ));
      Assert.AreEqual(104UL , message.GetExtension(UnitTestProtoFile.OptionalUint64Extension  ));
      Assert.AreEqual(105  , message.GetExtension(UnitTestProtoFile.OptionalSint32Extension  ));
      Assert.AreEqual(106L , message.GetExtension(UnitTestProtoFile.OptionalSint64Extension  ));
      Assert.AreEqual(107U  , message.GetExtension(UnitTestProtoFile.OptionalFixed32Extension ));
      Assert.AreEqual(108UL , message.GetExtension(UnitTestProtoFile.OptionalFixed64Extension ));
      Assert.AreEqual(109  , message.GetExtension(UnitTestProtoFile.OptionalSfixed32Extension));
      Assert.AreEqual(110L , message.GetExtension(UnitTestProtoFile.OptionalSfixed64Extension));
      Assert.AreEqual(111F , message.GetExtension(UnitTestProtoFile.OptionalFloatExtension   ));
      Assert.AreEqual(112D , message.GetExtension(UnitTestProtoFile.OptionalDoubleExtension  ));
      Assert.AreEqual(true , message.GetExtension(UnitTestProtoFile.OptionalBoolExtension    ));
      Assert.AreEqual("115", message.GetExtension(UnitTestProtoFile.OptionalStringExtension  ));
      Assert.AreEqual(ToBytes("116"), message.GetExtension(UnitTestProtoFile.OptionalBytesExtension));

      Assert.AreEqual(117, message.GetExtension(UnitTestProtoFile.OptionalGroupExtension         ).A);
      Assert.AreEqual(118, message.GetExtension(UnitTestProtoFile.OptionalNestedMessageExtension ).Bb);
      Assert.AreEqual(119, message.GetExtension(UnitTestProtoFile.OptionalForeignMessageExtension).C);
      Assert.AreEqual(120, message.GetExtension(UnitTestProtoFile.OptionalImportMessageExtension ).D);

      Assert.AreEqual(TestAllTypes.Types.NestedEnum.BAZ, message.GetExtension(UnitTestProtoFile.OptionalNestedEnumExtension));
      Assert.AreEqual(ForeignEnum.FOREIGN_BAZ, message.GetExtension(UnitTestProtoFile.OptionalForeignEnumExtension));
      Assert.AreEqual(ImportEnum.IMPORT_BAZ, message.GetExtension(UnitTestProtoFile.OptionalImportEnumExtension));

      Assert.AreEqual("124", message.GetExtension(UnitTestProtoFile.OptionalStringPieceExtension));
      Assert.AreEqual("125", message.GetExtension(UnitTestProtoFile.OptionalCordExtension));

      // -----------------------------------------------------------------

      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedInt32Extension   ));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedInt64Extension   ));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedUint32Extension  ));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedUint64Extension  ));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedSint32Extension  ));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedSint64Extension  ));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedFixed32Extension ));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedFixed64Extension ));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedSfixed32Extension));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedSfixed64Extension));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedFloatExtension   ));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedDoubleExtension  ));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedBoolExtension    ));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedStringExtension  ));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedBytesExtension   ));

      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedGroupExtension         ));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedNestedMessageExtension ));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedForeignMessageExtension));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedImportMessageExtension ));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedNestedEnumExtension    ));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedForeignEnumExtension   ));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedImportEnumExtension    ));

      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedStringPieceExtension));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedCordExtension));

      Assert.AreEqual(201  , message.GetExtension(UnitTestProtoFile.RepeatedInt32Extension   , 0));
      Assert.AreEqual(202L , message.GetExtension(UnitTestProtoFile.RepeatedInt64Extension   , 0));
      Assert.AreEqual(203U  , message.GetExtension(UnitTestProtoFile.RepeatedUint32Extension  , 0));
      Assert.AreEqual(204UL , message.GetExtension(UnitTestProtoFile.RepeatedUint64Extension  , 0));
      Assert.AreEqual(205  , message.GetExtension(UnitTestProtoFile.RepeatedSint32Extension  , 0));
      Assert.AreEqual(206L , message.GetExtension(UnitTestProtoFile.RepeatedSint64Extension  , 0));
      Assert.AreEqual(207U  , message.GetExtension(UnitTestProtoFile.RepeatedFixed32Extension , 0));
      Assert.AreEqual(208UL , message.GetExtension(UnitTestProtoFile.RepeatedFixed64Extension , 0));
      Assert.AreEqual(209  , message.GetExtension(UnitTestProtoFile.RepeatedSfixed32Extension, 0));
      Assert.AreEqual(210L , message.GetExtension(UnitTestProtoFile.RepeatedSfixed64Extension, 0));
      Assert.AreEqual(211F , message.GetExtension(UnitTestProtoFile.RepeatedFloatExtension   , 0));
      Assert.AreEqual(212D , message.GetExtension(UnitTestProtoFile.RepeatedDoubleExtension  , 0));
      Assert.AreEqual(true , message.GetExtension(UnitTestProtoFile.RepeatedBoolExtension    , 0));
      Assert.AreEqual("215", message.GetExtension(UnitTestProtoFile.RepeatedStringExtension  , 0));
      Assert.AreEqual(ToBytes("216"), message.GetExtension(UnitTestProtoFile.RepeatedBytesExtension, 0));

      Assert.AreEqual(217, message.GetExtension(UnitTestProtoFile.RepeatedGroupExtension         , 0).A);
      Assert.AreEqual(218, message.GetExtension(UnitTestProtoFile.RepeatedNestedMessageExtension , 0).Bb);
      Assert.AreEqual(219, message.GetExtension(UnitTestProtoFile.RepeatedForeignMessageExtension, 0).C);
      Assert.AreEqual(220, message.GetExtension(UnitTestProtoFile.RepeatedImportMessageExtension , 0).D);

      Assert.AreEqual(TestAllTypes.Types.NestedEnum.BAR, message.GetExtension(UnitTestProtoFile.RepeatedNestedEnumExtension, 0));
      Assert.AreEqual(ForeignEnum.FOREIGN_BAR, message.GetExtension(UnitTestProtoFile.RepeatedForeignEnumExtension, 0));
      Assert.AreEqual(ImportEnum.IMPORT_BAR, message.GetExtension(UnitTestProtoFile.RepeatedImportEnumExtension, 0));

      Assert.AreEqual("224", message.GetExtension(UnitTestProtoFile.RepeatedStringPieceExtension, 0));
      Assert.AreEqual("225", message.GetExtension(UnitTestProtoFile.RepeatedCordExtension, 0));

      Assert.AreEqual(301  , message.GetExtension(UnitTestProtoFile.RepeatedInt32Extension   , 1));
      Assert.AreEqual(302L , message.GetExtension(UnitTestProtoFile.RepeatedInt64Extension   , 1));
      Assert.AreEqual(303U  , message.GetExtension(UnitTestProtoFile.RepeatedUint32Extension  , 1));
      Assert.AreEqual(304UL , message.GetExtension(UnitTestProtoFile.RepeatedUint64Extension  , 1));
      Assert.AreEqual(305  , message.GetExtension(UnitTestProtoFile.RepeatedSint32Extension  , 1));
      Assert.AreEqual(306L , message.GetExtension(UnitTestProtoFile.RepeatedSint64Extension  , 1));
      Assert.AreEqual(307U  , message.GetExtension(UnitTestProtoFile.RepeatedFixed32Extension , 1));
      Assert.AreEqual(308UL , message.GetExtension(UnitTestProtoFile.RepeatedFixed64Extension , 1));
      Assert.AreEqual(309  , message.GetExtension(UnitTestProtoFile.RepeatedSfixed32Extension, 1));
      Assert.AreEqual(310L , message.GetExtension(UnitTestProtoFile.RepeatedSfixed64Extension, 1));
      Assert.AreEqual(311F , message.GetExtension(UnitTestProtoFile.RepeatedFloatExtension   , 1));
      Assert.AreEqual(312D , message.GetExtension(UnitTestProtoFile.RepeatedDoubleExtension  , 1));
      Assert.AreEqual(false, message.GetExtension(UnitTestProtoFile.RepeatedBoolExtension    , 1));
      Assert.AreEqual("315", message.GetExtension(UnitTestProtoFile.RepeatedStringExtension  , 1));
      Assert.AreEqual(ToBytes("316"), message.GetExtension(UnitTestProtoFile.RepeatedBytesExtension, 1));

      Assert.AreEqual(317, message.GetExtension(UnitTestProtoFile.RepeatedGroupExtension         , 1).A);
      Assert.AreEqual(318, message.GetExtension(UnitTestProtoFile.RepeatedNestedMessageExtension , 1).Bb);
      Assert.AreEqual(319, message.GetExtension(UnitTestProtoFile.RepeatedForeignMessageExtension, 1).C);
      Assert.AreEqual(320, message.GetExtension(UnitTestProtoFile.RepeatedImportMessageExtension , 1).D);

      Assert.AreEqual(TestAllTypes.Types.NestedEnum.BAZ, message.GetExtension(UnitTestProtoFile.RepeatedNestedEnumExtension, 1));
      Assert.AreEqual(ForeignEnum.FOREIGN_BAZ, message.GetExtension(UnitTestProtoFile.RepeatedForeignEnumExtension, 1));
      Assert.AreEqual(ImportEnum.IMPORT_BAZ, message.GetExtension(UnitTestProtoFile.RepeatedImportEnumExtension, 1));

      Assert.AreEqual("324", message.GetExtension(UnitTestProtoFile.RepeatedStringPieceExtension, 1));
      Assert.AreEqual("325", message.GetExtension(UnitTestProtoFile.RepeatedCordExtension, 1));

      // -----------------------------------------------------------------

      Assert.IsTrue(message.HasExtension(UnitTestProtoFile.DefaultInt32Extension   ));
      Assert.IsTrue(message.HasExtension(UnitTestProtoFile.DefaultInt64Extension   ));
      Assert.IsTrue(message.HasExtension(UnitTestProtoFile.DefaultUint32Extension  ));
      Assert.IsTrue(message.HasExtension(UnitTestProtoFile.DefaultUint64Extension  ));
      Assert.IsTrue(message.HasExtension(UnitTestProtoFile.DefaultSint32Extension  ));
      Assert.IsTrue(message.HasExtension(UnitTestProtoFile.DefaultSint64Extension  ));
      Assert.IsTrue(message.HasExtension(UnitTestProtoFile.DefaultFixed32Extension ));
      Assert.IsTrue(message.HasExtension(UnitTestProtoFile.DefaultFixed64Extension ));
      Assert.IsTrue(message.HasExtension(UnitTestProtoFile.DefaultSfixed32Extension));
      Assert.IsTrue(message.HasExtension(UnitTestProtoFile.DefaultSfixed64Extension));
      Assert.IsTrue(message.HasExtension(UnitTestProtoFile.DefaultFloatExtension   ));
      Assert.IsTrue(message.HasExtension(UnitTestProtoFile.DefaultDoubleExtension  ));
      Assert.IsTrue(message.HasExtension(UnitTestProtoFile.DefaultBoolExtension    ));
      Assert.IsTrue(message.HasExtension(UnitTestProtoFile.DefaultStringExtension  ));
      Assert.IsTrue(message.HasExtension(UnitTestProtoFile.DefaultBytesExtension   ));

      Assert.IsTrue(message.HasExtension(UnitTestProtoFile.DefaultNestedEnumExtension ));
      Assert.IsTrue(message.HasExtension(UnitTestProtoFile.DefaultForeignEnumExtension));
      Assert.IsTrue(message.HasExtension(UnitTestProtoFile.DefaultImportEnumExtension ));

      Assert.IsTrue(message.HasExtension(UnitTestProtoFile.DefaultStringPieceExtension));
      Assert.IsTrue(message.HasExtension(UnitTestProtoFile.DefaultCordExtension));

      Assert.AreEqual(401  , message.GetExtension(UnitTestProtoFile.DefaultInt32Extension   ));
      Assert.AreEqual(402L , message.GetExtension(UnitTestProtoFile.DefaultInt64Extension   ));
      Assert.AreEqual(403U  , message.GetExtension(UnitTestProtoFile.DefaultUint32Extension  ));
      Assert.AreEqual(404UL , message.GetExtension(UnitTestProtoFile.DefaultUint64Extension  ));
      Assert.AreEqual(405  , message.GetExtension(UnitTestProtoFile.DefaultSint32Extension  ));
      Assert.AreEqual(406L , message.GetExtension(UnitTestProtoFile.DefaultSint64Extension  ));
      Assert.AreEqual(407U  , message.GetExtension(UnitTestProtoFile.DefaultFixed32Extension ));
      Assert.AreEqual(408UL , message.GetExtension(UnitTestProtoFile.DefaultFixed64Extension ));
      Assert.AreEqual(409  , message.GetExtension(UnitTestProtoFile.DefaultSfixed32Extension));
      Assert.AreEqual(410L , message.GetExtension(UnitTestProtoFile.DefaultSfixed64Extension));
      Assert.AreEqual(411F , message.GetExtension(UnitTestProtoFile.DefaultFloatExtension   ));
      Assert.AreEqual(412D , message.GetExtension(UnitTestProtoFile.DefaultDoubleExtension  ));
      Assert.AreEqual(false, message.GetExtension(UnitTestProtoFile.DefaultBoolExtension    ));
      Assert.AreEqual("415", message.GetExtension(UnitTestProtoFile.DefaultStringExtension  ));
      Assert.AreEqual(ToBytes("416"), message.GetExtension(UnitTestProtoFile.DefaultBytesExtension));

      Assert.AreEqual(TestAllTypes.Types.NestedEnum.FOO, message.GetExtension(UnitTestProtoFile.DefaultNestedEnumExtension ));
      Assert.AreEqual(ForeignEnum.FOREIGN_FOO, message.GetExtension(UnitTestProtoFile.DefaultForeignEnumExtension));
      Assert.AreEqual(ImportEnum.IMPORT_FOO, message.GetExtension(UnitTestProtoFile.DefaultImportEnumExtension));

      Assert.AreEqual("424", message.GetExtension(UnitTestProtoFile.DefaultStringPieceExtension));
      Assert.AreEqual("425", message.GetExtension(UnitTestProtoFile.DefaultCordExtension));
    }

    /// <summary>
    /// Modifies the repeated extensions of the given message to contain the values
    /// expected by AssertRepeatedExtensionsModified.
    /// </summary>
    internal static void ModifyRepeatedExtensions(TestAllExtensions.Builder message) {
      message.SetExtension(UnitTestProtoFile.RepeatedInt32Extension, 1, 501);
      message.SetExtension(UnitTestProtoFile.RepeatedInt64Extension, 1, 502L);
      message.SetExtension(UnitTestProtoFile.RepeatedUint32Extension, 1, 503U);
      message.SetExtension(UnitTestProtoFile.RepeatedUint64Extension, 1, 504UL);
      message.SetExtension(UnitTestProtoFile.RepeatedSint32Extension, 1, 505);
      message.SetExtension(UnitTestProtoFile.RepeatedSint64Extension, 1, 506L);
      message.SetExtension(UnitTestProtoFile.RepeatedFixed32Extension, 1, 507U);
      message.SetExtension(UnitTestProtoFile.RepeatedFixed64Extension, 1, 508UL);
      message.SetExtension(UnitTestProtoFile.RepeatedSfixed32Extension, 1, 509);
      message.SetExtension(UnitTestProtoFile.RepeatedSfixed64Extension, 1, 510L);
      message.SetExtension(UnitTestProtoFile.RepeatedFloatExtension, 1, 511F);
      message.SetExtension(UnitTestProtoFile.RepeatedDoubleExtension, 1, 512D);
      message.SetExtension(UnitTestProtoFile.RepeatedBoolExtension, 1, true);
      message.SetExtension(UnitTestProtoFile.RepeatedStringExtension, 1, "515");
      message.SetExtension(UnitTestProtoFile.RepeatedBytesExtension, 1, ToBytes("516"));

      message.SetExtension(UnitTestProtoFile.RepeatedGroupExtension, 1, RepeatedGroup_extension.CreateBuilder().SetA(517).Build());
      message.SetExtension(UnitTestProtoFile.RepeatedNestedMessageExtension, 1, TestAllTypes.Types.NestedMessage.CreateBuilder().SetBb(518).Build());
      message.SetExtension(UnitTestProtoFile.RepeatedForeignMessageExtension, 1, ForeignMessage.CreateBuilder().SetC(519).Build());
      message.SetExtension(UnitTestProtoFile.RepeatedImportMessageExtension, 1, ImportMessage.CreateBuilder().SetD(520).Build());

      message.SetExtension(UnitTestProtoFile.RepeatedNestedEnumExtension, 1, TestAllTypes.Types.NestedEnum.FOO);
      message.SetExtension(UnitTestProtoFile.RepeatedForeignEnumExtension, 1, ForeignEnum.FOREIGN_FOO);
      message.SetExtension(UnitTestProtoFile.RepeatedImportEnumExtension, 1, ImportEnum.IMPORT_FOO);

      message.SetExtension(UnitTestProtoFile.RepeatedStringPieceExtension, 1, "524");
      message.SetExtension(UnitTestProtoFile.RepeatedCordExtension, 1, "525");
    }

    /// <summary>
    /// Asserts that all repeated extensions are set to the values assigned by
    /// SetAllExtensions follwed by ModifyRepeatedExtensions.
    /// </summary>
    internal static void AssertRepeatedExtensionsModified(TestAllExtensions message) {
      // ModifyRepeatedFields only sets the second repeated element of each
      // field.  In addition to verifying this, we also verify that the first
      // element and size were *not* modified.
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedInt32Extension));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedInt64Extension));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedUint32Extension));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedUint64Extension));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedSint32Extension));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedSint64Extension));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedFixed32Extension));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedFixed64Extension));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedSfixed32Extension));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedSfixed64Extension));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedFloatExtension));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedDoubleExtension));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedBoolExtension));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedStringExtension));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedBytesExtension));

      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedGroupExtension));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedNestedMessageExtension));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedForeignMessageExtension));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedImportMessageExtension));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedNestedEnumExtension));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedForeignEnumExtension));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedImportEnumExtension));

      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedStringPieceExtension));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.RepeatedCordExtension));

      Assert.AreEqual(201, message.GetExtension(UnitTestProtoFile.RepeatedInt32Extension, 0));
      Assert.AreEqual(202L, message.GetExtension(UnitTestProtoFile.RepeatedInt64Extension, 0));
      Assert.AreEqual(203U, message.GetExtension(UnitTestProtoFile.RepeatedUint32Extension, 0));
      Assert.AreEqual(204UL, message.GetExtension(UnitTestProtoFile.RepeatedUint64Extension, 0));
      Assert.AreEqual(205, message.GetExtension(UnitTestProtoFile.RepeatedSint32Extension, 0));
      Assert.AreEqual(206L, message.GetExtension(UnitTestProtoFile.RepeatedSint64Extension, 0));
      Assert.AreEqual(207U, message.GetExtension(UnitTestProtoFile.RepeatedFixed32Extension, 0));
      Assert.AreEqual(208UL, message.GetExtension(UnitTestProtoFile.RepeatedFixed64Extension, 0));
      Assert.AreEqual(209, message.GetExtension(UnitTestProtoFile.RepeatedSfixed32Extension, 0));
      Assert.AreEqual(210L, message.GetExtension(UnitTestProtoFile.RepeatedSfixed64Extension, 0));
      Assert.AreEqual(211F, message.GetExtension(UnitTestProtoFile.RepeatedFloatExtension, 0));
      Assert.AreEqual(212D, message.GetExtension(UnitTestProtoFile.RepeatedDoubleExtension, 0));
      Assert.AreEqual(true, message.GetExtension(UnitTestProtoFile.RepeatedBoolExtension, 0));
      Assert.AreEqual("215", message.GetExtension(UnitTestProtoFile.RepeatedStringExtension, 0));
      Assert.AreEqual(ToBytes("216"), message.GetExtension(UnitTestProtoFile.RepeatedBytesExtension, 0));

      Assert.AreEqual(217, message.GetExtension(UnitTestProtoFile.RepeatedGroupExtension, 0).A);
      Assert.AreEqual(218, message.GetExtension(UnitTestProtoFile.RepeatedNestedMessageExtension, 0).Bb);
      Assert.AreEqual(219, message.GetExtension(UnitTestProtoFile.RepeatedForeignMessageExtension, 0).C);
      Assert.AreEqual(220, message.GetExtension(UnitTestProtoFile.RepeatedImportMessageExtension, 0).D);

      Assert.AreEqual(TestAllTypes.Types.NestedEnum.BAR, message.GetExtension(UnitTestProtoFile.RepeatedNestedEnumExtension, 0));
      Assert.AreEqual(ForeignEnum.FOREIGN_BAR, message.GetExtension(UnitTestProtoFile.RepeatedForeignEnumExtension, 0));
      Assert.AreEqual(ImportEnum.IMPORT_BAR, message.GetExtension(UnitTestProtoFile.RepeatedImportEnumExtension, 0));

      Assert.AreEqual("224", message.GetExtension(UnitTestProtoFile.RepeatedStringPieceExtension, 0));
      Assert.AreEqual("225", message.GetExtension(UnitTestProtoFile.RepeatedCordExtension, 0));

      // Actually verify the second (modified) elements now.
      Assert.AreEqual(501, message.GetExtension(UnitTestProtoFile.RepeatedInt32Extension, 1));
      Assert.AreEqual(502L, message.GetExtension(UnitTestProtoFile.RepeatedInt64Extension, 1));
      Assert.AreEqual(503U, message.GetExtension(UnitTestProtoFile.RepeatedUint32Extension, 1));
      Assert.AreEqual(504UL, message.GetExtension(UnitTestProtoFile.RepeatedUint64Extension, 1));
      Assert.AreEqual(505, message.GetExtension(UnitTestProtoFile.RepeatedSint32Extension, 1));
      Assert.AreEqual(506L, message.GetExtension(UnitTestProtoFile.RepeatedSint64Extension, 1));
      Assert.AreEqual(507U, message.GetExtension(UnitTestProtoFile.RepeatedFixed32Extension, 1));
      Assert.AreEqual(508UL, message.GetExtension(UnitTestProtoFile.RepeatedFixed64Extension, 1));
      Assert.AreEqual(509, message.GetExtension(UnitTestProtoFile.RepeatedSfixed32Extension, 1));
      Assert.AreEqual(510L, message.GetExtension(UnitTestProtoFile.RepeatedSfixed64Extension, 1));
      Assert.AreEqual(511F, message.GetExtension(UnitTestProtoFile.RepeatedFloatExtension, 1));
      Assert.AreEqual(512D, message.GetExtension(UnitTestProtoFile.RepeatedDoubleExtension, 1));
      Assert.AreEqual(true, message.GetExtension(UnitTestProtoFile.RepeatedBoolExtension, 1));
      Assert.AreEqual("515", message.GetExtension(UnitTestProtoFile.RepeatedStringExtension, 1));
      Assert.AreEqual(ToBytes("516"), message.GetExtension(UnitTestProtoFile.RepeatedBytesExtension, 1));

      Assert.AreEqual(517, message.GetExtension(UnitTestProtoFile.RepeatedGroupExtension, 1).A);
      Assert.AreEqual(518, message.GetExtension(UnitTestProtoFile.RepeatedNestedMessageExtension, 1).Bb);
      Assert.AreEqual(519, message.GetExtension(UnitTestProtoFile.RepeatedForeignMessageExtension, 1).C);
      Assert.AreEqual(520, message.GetExtension(UnitTestProtoFile.RepeatedImportMessageExtension, 1).D);

      Assert.AreEqual(TestAllTypes.Types.NestedEnum.FOO, message.GetExtension(UnitTestProtoFile.RepeatedNestedEnumExtension, 1));
      Assert.AreEqual(ForeignEnum.FOREIGN_FOO, message.GetExtension(UnitTestProtoFile.RepeatedForeignEnumExtension, 1));
      Assert.AreEqual(ImportEnum.IMPORT_FOO, message.GetExtension(UnitTestProtoFile.RepeatedImportEnumExtension, 1));

      Assert.AreEqual("524", message.GetExtension(UnitTestProtoFile.RepeatedStringPieceExtension, 1));
      Assert.AreEqual("525", message.GetExtension(UnitTestProtoFile.RepeatedCordExtension, 1));
    }

    internal static void AssertExtensionsClear(TestAllExtensions message) {
      // HasBlah() should initially be false for all optional fields.
      Assert.IsFalse(message.HasExtension(UnitTestProtoFile.OptionalInt32Extension));
      Assert.IsFalse(message.HasExtension(UnitTestProtoFile.OptionalInt64Extension));
      Assert.IsFalse(message.HasExtension(UnitTestProtoFile.OptionalUint32Extension));
      Assert.IsFalse(message.HasExtension(UnitTestProtoFile.OptionalUint64Extension));
      Assert.IsFalse(message.HasExtension(UnitTestProtoFile.OptionalSint32Extension));
      Assert.IsFalse(message.HasExtension(UnitTestProtoFile.OptionalSint64Extension));
      Assert.IsFalse(message.HasExtension(UnitTestProtoFile.OptionalFixed32Extension));
      Assert.IsFalse(message.HasExtension(UnitTestProtoFile.OptionalFixed64Extension));
      Assert.IsFalse(message.HasExtension(UnitTestProtoFile.OptionalSfixed32Extension));
      Assert.IsFalse(message.HasExtension(UnitTestProtoFile.OptionalSfixed64Extension));
      Assert.IsFalse(message.HasExtension(UnitTestProtoFile.OptionalFloatExtension));
      Assert.IsFalse(message.HasExtension(UnitTestProtoFile.OptionalDoubleExtension));
      Assert.IsFalse(message.HasExtension(UnitTestProtoFile.OptionalBoolExtension));
      Assert.IsFalse(message.HasExtension(UnitTestProtoFile.OptionalStringExtension));
      Assert.IsFalse(message.HasExtension(UnitTestProtoFile.OptionalBytesExtension));

      Assert.IsFalse(message.HasExtension(UnitTestProtoFile.OptionalGroupExtension));
      Assert.IsFalse(message.HasExtension(UnitTestProtoFile.OptionalNestedMessageExtension));
      Assert.IsFalse(message.HasExtension(UnitTestProtoFile.OptionalForeignMessageExtension));
      Assert.IsFalse(message.HasExtension(UnitTestProtoFile.OptionalImportMessageExtension));

      Assert.IsFalse(message.HasExtension(UnitTestProtoFile.OptionalNestedEnumExtension));
      Assert.IsFalse(message.HasExtension(UnitTestProtoFile.OptionalForeignEnumExtension));
      Assert.IsFalse(message.HasExtension(UnitTestProtoFile.OptionalImportEnumExtension));

      Assert.IsFalse(message.HasExtension(UnitTestProtoFile.OptionalStringPieceExtension));
      Assert.IsFalse(message.HasExtension(UnitTestProtoFile.OptionalCordExtension));

      // Optional fields without defaults are set to zero or something like it.
      Assert.AreEqual(0, message.GetExtension(UnitTestProtoFile.OptionalInt32Extension));
      Assert.AreEqual(0L, message.GetExtension(UnitTestProtoFile.OptionalInt64Extension));
      Assert.AreEqual(0U, message.GetExtension(UnitTestProtoFile.OptionalUint32Extension));
      Assert.AreEqual(0UL, message.GetExtension(UnitTestProtoFile.OptionalUint64Extension));
      Assert.AreEqual(0, message.GetExtension(UnitTestProtoFile.OptionalSint32Extension));
      Assert.AreEqual(0L, message.GetExtension(UnitTestProtoFile.OptionalSint64Extension));
      Assert.AreEqual(0U, message.GetExtension(UnitTestProtoFile.OptionalFixed32Extension));
      Assert.AreEqual(0UL, message.GetExtension(UnitTestProtoFile.OptionalFixed64Extension));
      Assert.AreEqual(0, message.GetExtension(UnitTestProtoFile.OptionalSfixed32Extension));
      Assert.AreEqual(0L, message.GetExtension(UnitTestProtoFile.OptionalSfixed64Extension));
      Assert.AreEqual(0F, message.GetExtension(UnitTestProtoFile.OptionalFloatExtension));
      Assert.AreEqual(0D, message.GetExtension(UnitTestProtoFile.OptionalDoubleExtension));
      Assert.AreEqual(false, message.GetExtension(UnitTestProtoFile.OptionalBoolExtension));
      Assert.AreEqual("", message.GetExtension(UnitTestProtoFile.OptionalStringExtension));
      Assert.AreEqual(ByteString.Empty, message.GetExtension(UnitTestProtoFile.OptionalBytesExtension));

      // Embedded messages should also be clear.
      Assert.IsFalse(message.GetExtension(UnitTestProtoFile.OptionalGroupExtension).HasA);
      Assert.IsFalse(message.GetExtension(UnitTestProtoFile.OptionalNestedMessageExtension).HasBb);
      Assert.IsFalse(message.GetExtension(UnitTestProtoFile.OptionalForeignMessageExtension).HasC);
      Assert.IsFalse(message.GetExtension(UnitTestProtoFile.OptionalImportMessageExtension).HasD);

      Assert.AreEqual(0, message.GetExtension(UnitTestProtoFile.OptionalGroupExtension).A);
      Assert.AreEqual(0, message.GetExtension(UnitTestProtoFile.OptionalNestedMessageExtension).Bb);
      Assert.AreEqual(0, message.GetExtension(UnitTestProtoFile.OptionalForeignMessageExtension).C);
      Assert.AreEqual(0, message.GetExtension(UnitTestProtoFile.OptionalImportMessageExtension).D);

      // Enums without defaults are set to the first value in the enum.
      Assert.AreEqual(TestAllTypes.Types.NestedEnum.FOO, message.GetExtension(UnitTestProtoFile.OptionalNestedEnumExtension));
      Assert.AreEqual(ForeignEnum.FOREIGN_FOO, message.GetExtension(UnitTestProtoFile.OptionalForeignEnumExtension));
      Assert.AreEqual(ImportEnum.IMPORT_FOO, message.GetExtension(UnitTestProtoFile.OptionalImportEnumExtension));

      Assert.AreEqual("", message.GetExtension(UnitTestProtoFile.OptionalStringPieceExtension));
      Assert.AreEqual("", message.GetExtension(UnitTestProtoFile.OptionalCordExtension));

      // Repeated fields are empty.
      Assert.AreEqual(0, message.GetExtensionCount(UnitTestProtoFile.RepeatedInt32Extension));
      Assert.AreEqual(0, message.GetExtensionCount(UnitTestProtoFile.RepeatedInt64Extension));
      Assert.AreEqual(0, message.GetExtensionCount(UnitTestProtoFile.RepeatedUint32Extension));
      Assert.AreEqual(0, message.GetExtensionCount(UnitTestProtoFile.RepeatedUint64Extension));
      Assert.AreEqual(0, message.GetExtensionCount(UnitTestProtoFile.RepeatedSint32Extension));
      Assert.AreEqual(0, message.GetExtensionCount(UnitTestProtoFile.RepeatedSint64Extension));
      Assert.AreEqual(0, message.GetExtensionCount(UnitTestProtoFile.RepeatedFixed32Extension));
      Assert.AreEqual(0, message.GetExtensionCount(UnitTestProtoFile.RepeatedFixed64Extension));
      Assert.AreEqual(0, message.GetExtensionCount(UnitTestProtoFile.RepeatedSfixed32Extension));
      Assert.AreEqual(0, message.GetExtensionCount(UnitTestProtoFile.RepeatedSfixed64Extension));
      Assert.AreEqual(0, message.GetExtensionCount(UnitTestProtoFile.RepeatedFloatExtension));
      Assert.AreEqual(0, message.GetExtensionCount(UnitTestProtoFile.RepeatedDoubleExtension));
      Assert.AreEqual(0, message.GetExtensionCount(UnitTestProtoFile.RepeatedBoolExtension));
      Assert.AreEqual(0, message.GetExtensionCount(UnitTestProtoFile.RepeatedStringExtension));
      Assert.AreEqual(0, message.GetExtensionCount(UnitTestProtoFile.RepeatedBytesExtension));

      Assert.AreEqual(0, message.GetExtensionCount(UnitTestProtoFile.RepeatedGroupExtension));
      Assert.AreEqual(0, message.GetExtensionCount(UnitTestProtoFile.RepeatedNestedMessageExtension));
      Assert.AreEqual(0, message.GetExtensionCount(UnitTestProtoFile.RepeatedForeignMessageExtension));
      Assert.AreEqual(0, message.GetExtensionCount(UnitTestProtoFile.RepeatedImportMessageExtension));
      Assert.AreEqual(0, message.GetExtensionCount(UnitTestProtoFile.RepeatedNestedEnumExtension));
      Assert.AreEqual(0, message.GetExtensionCount(UnitTestProtoFile.RepeatedForeignEnumExtension));
      Assert.AreEqual(0, message.GetExtensionCount(UnitTestProtoFile.RepeatedImportEnumExtension));

      Assert.AreEqual(0, message.GetExtensionCount(UnitTestProtoFile.RepeatedStringPieceExtension));
      Assert.AreEqual(0, message.GetExtensionCount(UnitTestProtoFile.RepeatedCordExtension));

      // HasBlah() should also be false for all default fields.
      Assert.IsFalse(message.HasExtension(UnitTestProtoFile.DefaultInt32Extension));
      Assert.IsFalse(message.HasExtension(UnitTestProtoFile.DefaultInt64Extension));
      Assert.IsFalse(message.HasExtension(UnitTestProtoFile.DefaultUint32Extension));
      Assert.IsFalse(message.HasExtension(UnitTestProtoFile.DefaultUint64Extension));
      Assert.IsFalse(message.HasExtension(UnitTestProtoFile.DefaultSint32Extension));
      Assert.IsFalse(message.HasExtension(UnitTestProtoFile.DefaultSint64Extension));
      Assert.IsFalse(message.HasExtension(UnitTestProtoFile.DefaultFixed32Extension));
      Assert.IsFalse(message.HasExtension(UnitTestProtoFile.DefaultFixed64Extension));
      Assert.IsFalse(message.HasExtension(UnitTestProtoFile.DefaultSfixed32Extension));
      Assert.IsFalse(message.HasExtension(UnitTestProtoFile.DefaultSfixed64Extension));
      Assert.IsFalse(message.HasExtension(UnitTestProtoFile.DefaultFloatExtension));
      Assert.IsFalse(message.HasExtension(UnitTestProtoFile.DefaultDoubleExtension));
      Assert.IsFalse(message.HasExtension(UnitTestProtoFile.DefaultBoolExtension));
      Assert.IsFalse(message.HasExtension(UnitTestProtoFile.DefaultStringExtension));
      Assert.IsFalse(message.HasExtension(UnitTestProtoFile.DefaultBytesExtension));

      Assert.IsFalse(message.HasExtension(UnitTestProtoFile.DefaultNestedEnumExtension));
      Assert.IsFalse(message.HasExtension(UnitTestProtoFile.DefaultForeignEnumExtension));
      Assert.IsFalse(message.HasExtension(UnitTestProtoFile.DefaultImportEnumExtension));

      Assert.IsFalse(message.HasExtension(UnitTestProtoFile.DefaultStringPieceExtension));
      Assert.IsFalse(message.HasExtension(UnitTestProtoFile.DefaultCordExtension));

      // Fields with defaults have their default values (duh).
      Assert.AreEqual(41, message.GetExtension(UnitTestProtoFile.DefaultInt32Extension));
      Assert.AreEqual(42L, message.GetExtension(UnitTestProtoFile.DefaultInt64Extension));
      Assert.AreEqual(43U, message.GetExtension(UnitTestProtoFile.DefaultUint32Extension));
      Assert.AreEqual(44UL, message.GetExtension(UnitTestProtoFile.DefaultUint64Extension));
      Assert.AreEqual(-45, message.GetExtension(UnitTestProtoFile.DefaultSint32Extension));
      Assert.AreEqual(46L, message.GetExtension(UnitTestProtoFile.DefaultSint64Extension));
      Assert.AreEqual(47U, message.GetExtension(UnitTestProtoFile.DefaultFixed32Extension));
      Assert.AreEqual(48UL, message.GetExtension(UnitTestProtoFile.DefaultFixed64Extension));
      Assert.AreEqual(49, message.GetExtension(UnitTestProtoFile.DefaultSfixed32Extension));
      Assert.AreEqual(-50L, message.GetExtension(UnitTestProtoFile.DefaultSfixed64Extension));
      Assert.AreEqual(51.5F, message.GetExtension(UnitTestProtoFile.DefaultFloatExtension));
      Assert.AreEqual(52e3D, message.GetExtension(UnitTestProtoFile.DefaultDoubleExtension));
      Assert.AreEqual(true, message.GetExtension(UnitTestProtoFile.DefaultBoolExtension));
      Assert.AreEqual("hello", message.GetExtension(UnitTestProtoFile.DefaultStringExtension));
      Assert.AreEqual(ToBytes("world"), message.GetExtension(UnitTestProtoFile.DefaultBytesExtension));

      Assert.AreEqual(TestAllTypes.Types.NestedEnum.BAR, message.GetExtension(UnitTestProtoFile.DefaultNestedEnumExtension));
      Assert.AreEqual(ForeignEnum.FOREIGN_BAR, message.GetExtension(UnitTestProtoFile.DefaultForeignEnumExtension));
      Assert.AreEqual(ImportEnum.IMPORT_BAR, message.GetExtension(UnitTestProtoFile.DefaultImportEnumExtension));

      Assert.AreEqual("abc", message.GetExtension(UnitTestProtoFile.DefaultStringPieceExtension));
      Assert.AreEqual("123", message.GetExtension(UnitTestProtoFile.DefaultCordExtension));
    }

    /// <summary>
    /// Set every field of the specified message to a unique value.
    /// </summary>
    public static void SetPackedFields(TestPackedTypes.Builder message) {
      message.AddPackedInt32(601);
      message.AddPackedInt64(602);
      message.AddPackedUint32(603);
      message.AddPackedUint64(604);
      message.AddPackedSint32(605);
      message.AddPackedSint64(606);
      message.AddPackedFixed32(607);
      message.AddPackedFixed64(608);
      message.AddPackedSfixed32(609);
      message.AddPackedSfixed64(610);
      message.AddPackedFloat(611);
      message.AddPackedDouble(612);
      message.AddPackedBool(true);
      message.AddPackedEnum(ForeignEnum.FOREIGN_BAR);
      // Add a second one of each field.
      message.AddPackedInt32(701);
      message.AddPackedInt64(702);
      message.AddPackedUint32(703);
      message.AddPackedUint64(704);
      message.AddPackedSint32(705);
      message.AddPackedSint64(706);
      message.AddPackedFixed32(707);
      message.AddPackedFixed64(708);
      message.AddPackedSfixed32(709);
      message.AddPackedSfixed64(710);
      message.AddPackedFloat(711);
      message.AddPackedDouble(712);
      message.AddPackedBool(false);
      message.AddPackedEnum(ForeignEnum.FOREIGN_BAZ);
    }

    /// <summary>
    /// Asserts that all the fields of the specified message are set to the values assigned
    /// in SetPackedFields.
    /// </summary>
    public static void AssertPackedFieldsSet(TestPackedTypes message) {
      Assert.AreEqual(2, message.PackedInt32Count);
      Assert.AreEqual(2, message.PackedInt64Count);
      Assert.AreEqual(2, message.PackedUint32Count);
      Assert.AreEqual(2, message.PackedUint64Count);
      Assert.AreEqual(2, message.PackedSint32Count);
      Assert.AreEqual(2, message.PackedSint64Count);
      Assert.AreEqual(2, message.PackedFixed32Count);
      Assert.AreEqual(2, message.PackedFixed64Count);
      Assert.AreEqual(2, message.PackedSfixed32Count);
      Assert.AreEqual(2, message.PackedSfixed64Count);
      Assert.AreEqual(2, message.PackedFloatCount);
      Assert.AreEqual(2, message.PackedDoubleCount);
      Assert.AreEqual(2, message.PackedBoolCount);
      Assert.AreEqual(2, message.PackedEnumCount);
      Assert.AreEqual(601, message.GetPackedInt32(0));
      Assert.AreEqual(602, message.GetPackedInt64(0));
      Assert.AreEqual(603, message.GetPackedUint32(0));
      Assert.AreEqual(604, message.GetPackedUint64(0));
      Assert.AreEqual(605, message.GetPackedSint32(0));
      Assert.AreEqual(606, message.GetPackedSint64(0));
      Assert.AreEqual(607, message.GetPackedFixed32(0));
      Assert.AreEqual(608, message.GetPackedFixed64(0));
      Assert.AreEqual(609, message.GetPackedSfixed32(0));
      Assert.AreEqual(610, message.GetPackedSfixed64(0));
      Assert.AreEqual(611, message.GetPackedFloat(0), 0.0);
      Assert.AreEqual(612, message.GetPackedDouble(0), 0.0);
      Assert.AreEqual(true, message.GetPackedBool(0));
      Assert.AreEqual(ForeignEnum.FOREIGN_BAR, message.GetPackedEnum(0));
      Assert.AreEqual(701, message.GetPackedInt32(1));
      Assert.AreEqual(702, message.GetPackedInt64(1));
      Assert.AreEqual(703, message.GetPackedUint32(1));
      Assert.AreEqual(704, message.GetPackedUint64(1));
      Assert.AreEqual(705, message.GetPackedSint32(1));
      Assert.AreEqual(706, message.GetPackedSint64(1));
      Assert.AreEqual(707, message.GetPackedFixed32(1));
      Assert.AreEqual(708, message.GetPackedFixed64(1));
      Assert.AreEqual(709, message.GetPackedSfixed32(1));
      Assert.AreEqual(710, message.GetPackedSfixed64(1));
      Assert.AreEqual(711, message.GetPackedFloat(1), 0.0);
      Assert.AreEqual(712, message.GetPackedDouble(1), 0.0);
      Assert.AreEqual(false, message.GetPackedBool(1));
      Assert.AreEqual(ForeignEnum.FOREIGN_BAZ, message.GetPackedEnum(1));
    }

    public static void SetPackedExtensions(TestPackedExtensions.Builder message) {
      message.AddExtension(UnitTestProtoFile.PackedInt32Extension, 601);
      message.AddExtension(UnitTestProtoFile.PackedInt64Extension, 602L);
      message.AddExtension(UnitTestProtoFile.PackedUint32Extension, 603U);
      message.AddExtension(UnitTestProtoFile.PackedUint64Extension, 604UL);
      message.AddExtension(UnitTestProtoFile.PackedSint32Extension, 605);
      message.AddExtension(UnitTestProtoFile.PackedSint64Extension, 606L);
      message.AddExtension(UnitTestProtoFile.PackedFixed32Extension, 607U);
      message.AddExtension(UnitTestProtoFile.PackedFixed64Extension, 608UL);
      message.AddExtension(UnitTestProtoFile.PackedSfixed32Extension, 609);
      message.AddExtension(UnitTestProtoFile.PackedSfixed64Extension, 610L);
      message.AddExtension(UnitTestProtoFile.PackedFloatExtension, 611F);
      message.AddExtension(UnitTestProtoFile.PackedDoubleExtension, 612D);
      message.AddExtension(UnitTestProtoFile.PackedBoolExtension, true);
      message.AddExtension(UnitTestProtoFile.PackedEnumExtension, ForeignEnum.FOREIGN_BAR);
      // Add a second one of each field.
      message.AddExtension(UnitTestProtoFile.PackedInt32Extension, 701);
      message.AddExtension(UnitTestProtoFile.PackedInt64Extension, 702L);
      message.AddExtension(UnitTestProtoFile.PackedUint32Extension, 703U);
      message.AddExtension(UnitTestProtoFile.PackedUint64Extension, 704UL);
      message.AddExtension(UnitTestProtoFile.PackedSint32Extension, 705);
      message.AddExtension(UnitTestProtoFile.PackedSint64Extension, 706L);
      message.AddExtension(UnitTestProtoFile.PackedFixed32Extension, 707U);
      message.AddExtension(UnitTestProtoFile.PackedFixed64Extension, 708UL);
      message.AddExtension(UnitTestProtoFile.PackedSfixed32Extension, 709);
      message.AddExtension(UnitTestProtoFile.PackedSfixed64Extension, 710L);
      message.AddExtension(UnitTestProtoFile.PackedFloatExtension, 711F);
      message.AddExtension(UnitTestProtoFile.PackedDoubleExtension, 712D);
      message.AddExtension(UnitTestProtoFile.PackedBoolExtension, false);
      message.AddExtension(UnitTestProtoFile.PackedEnumExtension, ForeignEnum.FOREIGN_BAZ);
    }

    public static void AssertPackedExtensionsSet(TestPackedExtensions message) {
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.PackedInt32Extension));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.PackedInt64Extension));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.PackedUint32Extension));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.PackedUint64Extension));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.PackedSint32Extension));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.PackedSint64Extension));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.PackedFixed32Extension));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.PackedFixed64Extension));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.PackedSfixed32Extension));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.PackedSfixed64Extension));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.PackedFloatExtension));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.PackedDoubleExtension));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.PackedBoolExtension));
      Assert.AreEqual(2, message.GetExtensionCount(UnitTestProtoFile.PackedEnumExtension));
      Assert.AreEqual(601, message.GetExtension(UnitTestProtoFile.PackedInt32Extension, 0));
      Assert.AreEqual(602L, message.GetExtension(UnitTestProtoFile.PackedInt64Extension, 0));
      Assert.AreEqual(603, message.GetExtension(UnitTestProtoFile.PackedUint32Extension, 0));
      Assert.AreEqual(604L, message.GetExtension(UnitTestProtoFile.PackedUint64Extension, 0));
      Assert.AreEqual(605, message.GetExtension(UnitTestProtoFile.PackedSint32Extension, 0));
      Assert.AreEqual(606L, message.GetExtension(UnitTestProtoFile.PackedSint64Extension, 0));
      Assert.AreEqual(607, message.GetExtension(UnitTestProtoFile.PackedFixed32Extension, 0));
      Assert.AreEqual(608L, message.GetExtension(UnitTestProtoFile.PackedFixed64Extension, 0));
      Assert.AreEqual(609, message.GetExtension(UnitTestProtoFile.PackedSfixed32Extension, 0));
      Assert.AreEqual(610L, message.GetExtension(UnitTestProtoFile.PackedSfixed64Extension, 0));
      Assert.AreEqual(611F, message.GetExtension(UnitTestProtoFile.PackedFloatExtension, 0));
      Assert.AreEqual(612D, message.GetExtension(UnitTestProtoFile.PackedDoubleExtension, 0));
      Assert.AreEqual(true, message.GetExtension(UnitTestProtoFile.PackedBoolExtension, 0));
      Assert.AreEqual(ForeignEnum.FOREIGN_BAR,
                            message.GetExtension(UnitTestProtoFile.PackedEnumExtension, 0));
      Assert.AreEqual(701, message.GetExtension(UnitTestProtoFile.PackedInt32Extension, 1));
      Assert.AreEqual(702L, message.GetExtension(UnitTestProtoFile.PackedInt64Extension, 1));
      Assert.AreEqual(703, message.GetExtension(UnitTestProtoFile.PackedUint32Extension, 1));
      Assert.AreEqual(704L, message.GetExtension(UnitTestProtoFile.PackedUint64Extension, 1));
      Assert.AreEqual(705, message.GetExtension(UnitTestProtoFile.PackedSint32Extension, 1));
      Assert.AreEqual(706L, message.GetExtension(UnitTestProtoFile.PackedSint64Extension, 1));
      Assert.AreEqual(707, message.GetExtension(UnitTestProtoFile.PackedFixed32Extension, 1));
      Assert.AreEqual(708L, message.GetExtension(UnitTestProtoFile.PackedFixed64Extension, 1));
      Assert.AreEqual(709, message.GetExtension(UnitTestProtoFile.PackedSfixed32Extension, 1));
      Assert.AreEqual(710L, message.GetExtension(UnitTestProtoFile.PackedSfixed64Extension, 1));
      Assert.AreEqual(711F, message.GetExtension(UnitTestProtoFile.PackedFloatExtension, 1));
      Assert.AreEqual(712D, message.GetExtension(UnitTestProtoFile.PackedDoubleExtension, 1));
      Assert.AreEqual(false, message.GetExtension(UnitTestProtoFile.PackedBoolExtension, 1));
      Assert.AreEqual(ForeignEnum.FOREIGN_BAZ, message.GetExtension(UnitTestProtoFile.PackedEnumExtension, 1));
    }

    private static ByteString goldenPackedFieldsMessage = null;

    /// <summary>
    /// Get the bytes of the "golden packed fields message".  This is a serialized
    /// TestPackedTypes with all fields set as they would be by SetPackedFields,
    /// but it is loaded from a file on disk rather than generated dynamically.
    /// The file is actually generated by C++ code, so testing against it verifies compatibility
    /// with C++.
    /// </summary>
    public static ByteString GetGoldenPackedFieldsMessage() {
      if (goldenPackedFieldsMessage == null) {
        goldenPackedFieldsMessage = ReadBytesFromFile("golden_packed_fields_message");
      }
      return goldenPackedFieldsMessage;
    }

    private static readonly string[] TestCultures = { "en-US", "en-GB", "fr-FR", "de-DE" };

    public static void TestInMultipleCultures(Action test) {
      CultureInfo originalCulture = Thread.CurrentThread.CurrentCulture;
      foreach (string culture in TestCultures) {
        try {
          Thread.CurrentThread.CurrentCulture = new CultureInfo(culture);
          test();
        } finally {
          Thread.CurrentThread.CurrentCulture = originalCulture;
        }
      }
    }
    
    /// <summary>
    /// Helper to construct a byte array from a bunch of bytes.
    /// </summary>
    internal static byte[] Bytes(params byte[] bytesAsInts) {
      byte[] bytes = new byte[bytesAsInts.Length];
      for (int i = 0; i < bytesAsInts.Length; i++) {
        bytes[i] = (byte)bytesAsInts[i];
      }
      return bytes;
    }

    internal static void AssertArgumentNullException(Action action) {
      try {
        action();
        Assert.Fail("Exception was not thrown");
      } catch (ArgumentNullException) {
        // We expect this exception.
      }
    }
  }
}
