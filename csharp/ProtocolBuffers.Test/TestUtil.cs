using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using Google.ProtocolBuffers.TestProtos;
using NUnit.Framework;

namespace Google.ProtocolBuffers {
  internal static class TestUtil {

    private static DirectoryInfo testDataDirectory;

    internal static DirectoryInfo TestDataDirectory {
      get {
        if (testDataDirectory != null) {
          return testDataDirectory;
        }

        DirectoryInfo ancestor = new DirectoryInfo(".");
        // Search each parent directory looking for "src/google/protobuf".
        while (ancestor != null) {
          string candidate = Path.Combine(ancestor.FullName, "src/google/protobuf");
          if (Directory.Exists(candidate)) {
            testDataDirectory = new DirectoryInfo(candidate);
            return testDataDirectory;
          }
          ancestor = ancestor.Parent;
        }
        // TODO(jonskeet): Come up with a better exception to throw
        throw new Exception("Unable to find directory containing test files");
      }
    }

    /// <summary>
    /// Helper to convert a String to ByteString.
    /// </summary>
    private static ByteString ToBytes(String str) {
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

  }
}
