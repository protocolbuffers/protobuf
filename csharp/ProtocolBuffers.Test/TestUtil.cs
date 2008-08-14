using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
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
        // Search each parent directory looking for "src/google/protobuf".
        while (ancestor != null) {
          string candidate = Path.Combine(ancestor.FullName, "src/google/protobuf");
          if (Directory.Exists(candidate)) {
            testDataDirectory = Path.Combine(ancestor.FullName, "src/google/protobuf/testdata");
            return testDataDirectory;
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
  }
}
