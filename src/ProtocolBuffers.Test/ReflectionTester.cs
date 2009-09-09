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
using Google.ProtocolBuffers.Descriptors;
using Google.ProtocolBuffers.TestProtos;
using NUnit.Framework;

namespace Google.ProtocolBuffers {
  /// <summary>
  /// Performs the same things that the methods of TestUtil do, but
  /// via the reflection interface.  This is its own class because it needs
  /// to know what descriptor to use.
  /// </summary>
  internal class ReflectionTester {
    private readonly MessageDescriptor baseDescriptor;
    private readonly ExtensionRegistry extensionRegistry;

    private readonly FileDescriptor file;
    private readonly FileDescriptor importFile;

    private readonly MessageDescriptor optionalGroup;
    private readonly MessageDescriptor repeatedGroup;
    private readonly MessageDescriptor nestedMessage;
    private readonly MessageDescriptor foreignMessage;
    private readonly MessageDescriptor importMessage;

    private readonly FieldDescriptor groupA;
    private readonly FieldDescriptor repeatedGroupA;
    private readonly FieldDescriptor nestedB;
    private readonly FieldDescriptor foreignC;
    private readonly FieldDescriptor importD;

    private readonly EnumDescriptor nestedEnum;
    private readonly EnumDescriptor foreignEnum;
    private readonly EnumDescriptor importEnum;

    private readonly EnumValueDescriptor nestedFoo;
    private readonly EnumValueDescriptor nestedBar;
    private readonly EnumValueDescriptor nestedBaz;
    private readonly EnumValueDescriptor foreignFoo;
    private readonly EnumValueDescriptor foreignBar;
    private readonly EnumValueDescriptor foreignBaz;
    private readonly EnumValueDescriptor importFoo;
    private readonly EnumValueDescriptor importBar;
    private readonly EnumValueDescriptor importBaz;

    /// <summary>
    /// Constructs an instance that will expect messages using the given
    /// descriptor. Normally <paramref name="baseDescriptor"/> should be
    /// a descriptor for TestAllTypes. However, if extensionRegistry is non-null,
    /// then baseDescriptor should be for TestAllExtensions instead, and instead of
    /// reading and writing normal fields, the tester will read and write extensions.
    /// All of the TestAllExtensions extensions must be registered in the registry.
    /// </summary>
    private ReflectionTester(MessageDescriptor baseDescriptor,
                            ExtensionRegistry extensionRegistry) {
      this.baseDescriptor = baseDescriptor;
      this.extensionRegistry = extensionRegistry;

      this.file = baseDescriptor.File;
      // TODO(jonskeet): We've got 2 dependencies, not 1 - because of the C# options. Hmm.
      //      Assert.AreEqual(1, file.Dependencies.Count);
      // TODO(jonskeet): Find dependency by name instead of number?
      this.importFile = file.Dependencies[1];

      MessageDescriptor testAllTypes;
      if (baseDescriptor.Name == "TestAllTypes") {
        testAllTypes = baseDescriptor;
      } else {
        testAllTypes = file.FindTypeByName<MessageDescriptor>("TestAllTypes");
        Assert.IsNotNull(testAllTypes);
      }

      if (extensionRegistry == null) {
        // Use testAllTypes, rather than baseDescriptor, to allow
        // initialization using TestPackedTypes descriptors. These objects
        // won't be used by the methods for packed fields.
        this.optionalGroup =
          testAllTypes.FindDescriptor<MessageDescriptor>("OptionalGroup");
        this.repeatedGroup =
          testAllTypes.FindDescriptor<MessageDescriptor>("RepeatedGroup");
      } else {
        this.optionalGroup =
          file.FindTypeByName<MessageDescriptor>("OptionalGroup_extension");
        this.repeatedGroup =
          file.FindTypeByName<MessageDescriptor>("RepeatedGroup_extension");
      }
      this.nestedMessage = testAllTypes.FindDescriptor<MessageDescriptor>("NestedMessage");
      this.foreignMessage = file.FindTypeByName<MessageDescriptor>("ForeignMessage");
      this.importMessage = importFile.FindTypeByName<MessageDescriptor>("ImportMessage");

      this.nestedEnum = testAllTypes.FindDescriptor<EnumDescriptor>("NestedEnum");
      this.foreignEnum = file.FindTypeByName<EnumDescriptor>("ForeignEnum");
      this.importEnum = importFile.FindTypeByName<EnumDescriptor>("ImportEnum");

      Assert.IsNotNull(optionalGroup);
      Assert.IsNotNull(repeatedGroup);
      Assert.IsNotNull(nestedMessage);
      Assert.IsNotNull(foreignMessage);
      Assert.IsNotNull(importMessage);
      Assert.IsNotNull(nestedEnum);
      Assert.IsNotNull(foreignEnum);
      Assert.IsNotNull(importEnum);

      this.nestedB = nestedMessage.FindDescriptor<FieldDescriptor>("bb");
      this.foreignC = foreignMessage.FindDescriptor<FieldDescriptor>("c");
      this.importD = importMessage.FindDescriptor<FieldDescriptor>("d");
      this.nestedFoo = nestedEnum.FindValueByName("FOO");
      this.nestedBar = nestedEnum.FindValueByName("BAR");
      this.nestedBaz = nestedEnum.FindValueByName("BAZ");
      this.foreignFoo = foreignEnum.FindValueByName("FOREIGN_FOO");
      this.foreignBar = foreignEnum.FindValueByName("FOREIGN_BAR");
      this.foreignBaz = foreignEnum.FindValueByName("FOREIGN_BAZ");
      this.importFoo = importEnum.FindValueByName("IMPORT_FOO");
      this.importBar = importEnum.FindValueByName("IMPORT_BAR");
      this.importBaz = importEnum.FindValueByName("IMPORT_BAZ");

      this.groupA = optionalGroup.FindDescriptor<FieldDescriptor>("a");
      this.repeatedGroupA = repeatedGroup.FindDescriptor<FieldDescriptor>("a");

      Assert.IsNotNull(groupA);
      Assert.IsNotNull(repeatedGroupA);
      Assert.IsNotNull(nestedB);
      Assert.IsNotNull(foreignC);
      Assert.IsNotNull(importD);
      Assert.IsNotNull(nestedFoo);
      Assert.IsNotNull(nestedBar);
      Assert.IsNotNull(nestedBaz);
      Assert.IsNotNull(foreignFoo);
      Assert.IsNotNull(foreignBar);
      Assert.IsNotNull(foreignBaz);
      Assert.IsNotNull(importFoo);
      Assert.IsNotNull(importBar);
      Assert.IsNotNull(importBaz);
    }

    /// <summary>
    /// Creates an instance for the TestAllTypes message, with no extension registry.
    /// </summary>
    public static ReflectionTester CreateTestAllTypesInstance() {
      return new ReflectionTester(TestAllTypes.Descriptor, null);
    }

    /// <summary>
    /// Creates an instance for the TestAllExtensions message, with an
    /// extension registry from TestUtil.CreateExtensionRegistry.
    /// </summary>
    public static ReflectionTester CreateTestAllExtensionsInstance() {
      return new ReflectionTester(TestAllExtensions.Descriptor, TestUtil.CreateExtensionRegistry());
    }

    /// <summary>
    /// Creates an instance for the TestPackedTypes message, with no extensions.
    /// </summary>
    public static ReflectionTester CreateTestPackedTypesInstance() {
      return new ReflectionTester(TestPackedTypes.Descriptor, null);
    }

    /// <summary>
    /// Shorthand to get a FieldDescriptor for a field of unittest::TestAllTypes.
    /// </summary>
    private FieldDescriptor f(String name) {
      FieldDescriptor result;
      if (extensionRegistry == null) {
        result = baseDescriptor.FindDescriptor<FieldDescriptor>(name);
      } else {
        result = file.FindTypeByName<FieldDescriptor>(name + "_extension");
      }
      Assert.IsNotNull(result);
      return result;
    }

    /// <summary>
    /// Calls parent.CreateBuilderForField() or uses the extension registry
    /// to find an appropriate builder, depending on what type is being tested.
    /// </summary>
    private IBuilder CreateBuilderForField(IBuilder parent, FieldDescriptor field) {
      if (extensionRegistry == null) {
        return parent.CreateBuilderForField(field);
      } else {
        ExtensionInfo extension = extensionRegistry[field.ContainingType, field.FieldNumber];
        Assert.IsNotNull(extension);
        Assert.IsNotNull(extension.DefaultInstance);
        return extension.DefaultInstance.WeakCreateBuilderForType();
      }
    }

    /// <summary>
    /// Sets every field of the message to the values expected by
    /// AssertAllFieldsSet, using the reflection interface.
    /// </summary>
    /// <param name="message"></param>
    internal void SetAllFieldsViaReflection(IBuilder message) {
      message[f("optional_int32")] = 101;
      message[f("optional_int64")] = 102L;
      message[f("optional_uint32")] = 103U;
      message[f("optional_uint64")] = 104UL;
      message[f("optional_sint32")] = 105;
      message[f("optional_sint64")] = 106L;
      message[f("optional_fixed32")] = 107U;
      message[f("optional_fixed64")] = 108UL;
      message[f("optional_sfixed32")] = 109;
      message[f("optional_sfixed64")] = 110L;
      message[f("optional_float")] = 111F;
      message[f("optional_double")] = 112D;
      message[f("optional_bool")] = true;
      message[f("optional_string")] = "115";
      message[f("optional_bytes")] = TestUtil.ToBytes("116");

      message[f("optionalgroup")] = CreateBuilderForField(message, f("optionalgroup")).SetField(groupA, 117).WeakBuild();
      message[f("optional_nested_message")] = CreateBuilderForField(message, f("optional_nested_message")).SetField(nestedB, 118).WeakBuild();
      message[f("optional_foreign_message")] = CreateBuilderForField(message, f("optional_foreign_message")).SetField(foreignC, 119).WeakBuild();
      message[f("optional_import_message")] = CreateBuilderForField(message, f("optional_import_message")).SetField(importD, 120).WeakBuild();

      message[f("optional_nested_enum")] = nestedBaz;
      message[f("optional_foreign_enum")] = foreignBaz;
      message[f("optional_import_enum")] = importBaz;

      message[f("optional_string_piece")] = "124";
      message[f("optional_cord")] = "125";

      // -----------------------------------------------------------------

      message.WeakAddRepeatedField(f("repeated_int32"), 201);
      message.WeakAddRepeatedField(f("repeated_int64"), 202L);
      message.WeakAddRepeatedField(f("repeated_uint32"), 203U);
      message.WeakAddRepeatedField(f("repeated_uint64"), 204UL);
      message.WeakAddRepeatedField(f("repeated_sint32"), 205);
      message.WeakAddRepeatedField(f("repeated_sint64"), 206L);
      message.WeakAddRepeatedField(f("repeated_fixed32"), 207U);
      message.WeakAddRepeatedField(f("repeated_fixed64"), 208UL);
      message.WeakAddRepeatedField(f("repeated_sfixed32"), 209);
      message.WeakAddRepeatedField(f("repeated_sfixed64"), 210L);
      message.WeakAddRepeatedField(f("repeated_float"), 211F);
      message.WeakAddRepeatedField(f("repeated_double"), 212D);
      message.WeakAddRepeatedField(f("repeated_bool"), true);
      message.WeakAddRepeatedField(f("repeated_string"), "215");
      message.WeakAddRepeatedField(f("repeated_bytes"), TestUtil.ToBytes("216"));


      message.WeakAddRepeatedField(f("repeatedgroup"), CreateBuilderForField(message, f("repeatedgroup")).SetField(repeatedGroupA, 217).WeakBuild());
      message.WeakAddRepeatedField(f("repeated_nested_message"), CreateBuilderForField(message, f("repeated_nested_message")).SetField(nestedB, 218).WeakBuild());
      message.WeakAddRepeatedField(f("repeated_foreign_message"), CreateBuilderForField(message, f("repeated_foreign_message")).SetField(foreignC, 219).WeakBuild());
      message.WeakAddRepeatedField(f("repeated_import_message"), CreateBuilderForField(message, f("repeated_import_message")).SetField(importD, 220).WeakBuild());

      message.WeakAddRepeatedField(f("repeated_nested_enum"), nestedBar);
      message.WeakAddRepeatedField(f("repeated_foreign_enum"), foreignBar);
      message.WeakAddRepeatedField(f("repeated_import_enum"), importBar);

      message.WeakAddRepeatedField(f("repeated_string_piece"), "224");
      message.WeakAddRepeatedField(f("repeated_cord"), "225");

      // Add a second one of each field.
      message.WeakAddRepeatedField(f("repeated_int32"), 301);
      message.WeakAddRepeatedField(f("repeated_int64"), 302L);
      message.WeakAddRepeatedField(f("repeated_uint32"), 303U);
      message.WeakAddRepeatedField(f("repeated_uint64"), 304UL);
      message.WeakAddRepeatedField(f("repeated_sint32"), 305);
      message.WeakAddRepeatedField(f("repeated_sint64"), 306L);
      message.WeakAddRepeatedField(f("repeated_fixed32"), 307U);
      message.WeakAddRepeatedField(f("repeated_fixed64"), 308UL);
      message.WeakAddRepeatedField(f("repeated_sfixed32"), 309);
      message.WeakAddRepeatedField(f("repeated_sfixed64"), 310L);
      message.WeakAddRepeatedField(f("repeated_float"), 311F);
      message.WeakAddRepeatedField(f("repeated_double"), 312D);
      message.WeakAddRepeatedField(f("repeated_bool"), false);
      message.WeakAddRepeatedField(f("repeated_string"), "315");
      message.WeakAddRepeatedField(f("repeated_bytes"), TestUtil.ToBytes("316"));

      message.WeakAddRepeatedField(f("repeatedgroup"),
        CreateBuilderForField(message, f("repeatedgroup"))
               .SetField(repeatedGroupA, 317).WeakBuild());
      message.WeakAddRepeatedField(f("repeated_nested_message"),
        CreateBuilderForField(message, f("repeated_nested_message"))
               .SetField(nestedB, 318).WeakBuild());
      message.WeakAddRepeatedField(f("repeated_foreign_message"),
        CreateBuilderForField(message, f("repeated_foreign_message"))
               .SetField(foreignC, 319).WeakBuild());
      message.WeakAddRepeatedField(f("repeated_import_message"),
        CreateBuilderForField(message, f("repeated_import_message"))
               .SetField(importD, 320).WeakBuild());

      message.WeakAddRepeatedField(f("repeated_nested_enum"), nestedBaz);
      message.WeakAddRepeatedField(f("repeated_foreign_enum"), foreignBaz);
      message.WeakAddRepeatedField(f("repeated_import_enum"), importBaz);

      message.WeakAddRepeatedField(f("repeated_string_piece"), "324");
      message.WeakAddRepeatedField(f("repeated_cord"), "325");

      // -----------------------------------------------------------------

      message[f("default_int32")] = 401;
      message[f("default_int64")] = 402L;
      message[f("default_uint32")] = 403U;
      message[f("default_uint64")] = 404UL;
      message[f("default_sint32")] = 405;
      message[f("default_sint64")] = 406L;
      message[f("default_fixed32")] = 407U;
      message[f("default_fixed64")] = 408UL;
      message[f("default_sfixed32")] = 409;
      message[f("default_sfixed64")] = 410L;
      message[f("default_float")] = 411F;
      message[f("default_double")] = 412D;
      message[f("default_bool")] = false;
      message[f("default_string")] = "415";
      message[f("default_bytes")] = TestUtil.ToBytes("416");

      message[f("default_nested_enum")] = nestedFoo;
      message[f("default_foreign_enum")] = foreignFoo;
      message[f("default_import_enum")] = importFoo;

      message[f("default_string_piece")] = "424";
      message[f("default_cord")] = "425";
    }

    // -------------------------------------------------------------------

    /// <summary>
    /// Modify the repeated fields of the specified message to contain the
    /// values expected by AssertRepeatedFieldsModified, using the IBuilder
    /// reflection interface.
    /// </summary>
    internal void ModifyRepeatedFieldsViaReflection(IBuilder message) {
      message[f("repeated_int32"), 1] = 501;
      message[f("repeated_int64"), 1] = 502L;
      message[f("repeated_uint32"), 1] = 503U;
      message[f("repeated_uint64"), 1] = 504UL;
      message[f("repeated_sint32"), 1] = 505;
      message[f("repeated_sint64"), 1] = 506L;
      message[f("repeated_fixed32"), 1] = 507U;
      message[f("repeated_fixed64"), 1] = 508UL;
      message[f("repeated_sfixed32"), 1] = 509;
      message[f("repeated_sfixed64"), 1] = 510L;
      message[f("repeated_float"), 1] = 511F;
      message[f("repeated_double"), 1] = 512D;
      message[f("repeated_bool"), 1] = true;
      message[f("repeated_string"), 1] = "515";
      message.SetRepeatedField(f("repeated_bytes"), 1, TestUtil.ToBytes("516"));

      message.SetRepeatedField(f("repeatedgroup"), 1, CreateBuilderForField(message, f("repeatedgroup")).SetField(repeatedGroupA, 517).WeakBuild());
      message.SetRepeatedField(f("repeated_nested_message"), 1, CreateBuilderForField(message, f("repeated_nested_message")).SetField(nestedB, 518).WeakBuild());
      message.SetRepeatedField(f("repeated_foreign_message"), 1, CreateBuilderForField(message, f("repeated_foreign_message")).SetField(foreignC, 519).WeakBuild());
      message.SetRepeatedField(f("repeated_import_message"), 1, CreateBuilderForField(message, f("repeated_import_message")).SetField(importD, 520).WeakBuild());

      message[f("repeated_nested_enum"), 1] = nestedFoo;
      message[f("repeated_foreign_enum"), 1] = foreignFoo;
      message[f("repeated_import_enum"), 1] = importFoo;

      message[f("repeated_string_piece"), 1] = "524";
      message[f("repeated_cord"), 1] = "525";
    }

    // -------------------------------------------------------------------

    /// <summary>
    /// Asserts that all fields of the specified message are set to the values
    /// assigned by SetAllFields, using the IMessage reflection interface.
    /// </summary>
    public void AssertAllFieldsSetViaReflection(IMessage message) {
      Assert.IsTrue(message.HasField(f("optional_int32")));
      Assert.IsTrue(message.HasField(f("optional_int64")));
      Assert.IsTrue(message.HasField(f("optional_uint32")));
      Assert.IsTrue(message.HasField(f("optional_uint64")));
      Assert.IsTrue(message.HasField(f("optional_sint32")));
      Assert.IsTrue(message.HasField(f("optional_sint64")));
      Assert.IsTrue(message.HasField(f("optional_fixed32")));
      Assert.IsTrue(message.HasField(f("optional_fixed64")));
      Assert.IsTrue(message.HasField(f("optional_sfixed32")));
      Assert.IsTrue(message.HasField(f("optional_sfixed64")));
      Assert.IsTrue(message.HasField(f("optional_float")));
      Assert.IsTrue(message.HasField(f("optional_double")));
      Assert.IsTrue(message.HasField(f("optional_bool")));
      Assert.IsTrue(message.HasField(f("optional_string")));
      Assert.IsTrue(message.HasField(f("optional_bytes")));

      Assert.IsTrue(message.HasField(f("optionalgroup")));
      Assert.IsTrue(message.HasField(f("optional_nested_message")));
      Assert.IsTrue(message.HasField(f("optional_foreign_message")));
      Assert.IsTrue(message.HasField(f("optional_import_message")));

      Assert.IsTrue(((IMessage)message[f("optionalgroup")]).HasField(groupA));
      Assert.IsTrue(((IMessage)message[f("optional_nested_message")]).HasField(nestedB));
      Assert.IsTrue(((IMessage)message[f("optional_foreign_message")]).HasField(foreignC));
      Assert.IsTrue(((IMessage)message[f("optional_import_message")]).HasField(importD));

      Assert.IsTrue(message.HasField(f("optional_nested_enum")));
      Assert.IsTrue(message.HasField(f("optional_foreign_enum")));
      Assert.IsTrue(message.HasField(f("optional_import_enum")));

      Assert.IsTrue(message.HasField(f("optional_string_piece")));
      Assert.IsTrue(message.HasField(f("optional_cord")));

      Assert.AreEqual(101, message[f("optional_int32")]);
      Assert.AreEqual(102L, message[f("optional_int64")]);
      Assert.AreEqual(103U, message[f("optional_uint32")]);
      Assert.AreEqual(104UL, message[f("optional_uint64")]);
      Assert.AreEqual(105, message[f("optional_sint32")]);
      Assert.AreEqual(106L, message[f("optional_sint64")]);
      Assert.AreEqual(107U, message[f("optional_fixed32")]);
      Assert.AreEqual(108UL, message[f("optional_fixed64")]);
      Assert.AreEqual(109, message[f("optional_sfixed32")]);
      Assert.AreEqual(110L, message[f("optional_sfixed64")]);
      Assert.AreEqual(111F, message[f("optional_float")]);
      Assert.AreEqual(112D, message[f("optional_double")]);
      Assert.AreEqual(true, message[f("optional_bool")]);
      Assert.AreEqual("115", message[f("optional_string")]);
      Assert.AreEqual(TestUtil.ToBytes("116"), message[f("optional_bytes")]);

      Assert.AreEqual(117, ((IMessage)message[f("optionalgroup")])[groupA]);
      Assert.AreEqual(118, ((IMessage)message[f("optional_nested_message")])[nestedB]);
      Assert.AreEqual(119, ((IMessage)message[f("optional_foreign_message")])[foreignC]);
      Assert.AreEqual(120, ((IMessage)message[f("optional_import_message")])[importD]);

      Assert.AreEqual(nestedBaz, message[f("optional_nested_enum")]);
      Assert.AreEqual(foreignBaz, message[f("optional_foreign_enum")]);
      Assert.AreEqual(importBaz, message[f("optional_import_enum")]);

      Assert.AreEqual("124", message[f("optional_string_piece")]);
      Assert.AreEqual("125", message[f("optional_cord")]);

      // -----------------------------------------------------------------

      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_int32")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_int64")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_uint32")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_uint64")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_sint32")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_sint64")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_fixed32")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_fixed64")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_sfixed32")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_sfixed64")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_float")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_double")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_bool")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_string")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_bytes")));

      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeatedgroup")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_nested_message")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_foreign_message")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_import_message")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_nested_enum")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_foreign_enum")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_import_enum")));

      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_string_piece")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_cord")));

      Assert.AreEqual(201, message[f("repeated_int32"), 0]);
      Assert.AreEqual(202L, message[f("repeated_int64"), 0]);
      Assert.AreEqual(203U, message[f("repeated_uint32"), 0]);
      Assert.AreEqual(204UL, message[f("repeated_uint64"), 0]);
      Assert.AreEqual(205, message[f("repeated_sint32"), 0]);
      Assert.AreEqual(206L, message[f("repeated_sint64"), 0]);
      Assert.AreEqual(207U, message[f("repeated_fixed32"), 0]);
      Assert.AreEqual(208UL, message[f("repeated_fixed64"), 0]);
      Assert.AreEqual(209, message[f("repeated_sfixed32"), 0]);
      Assert.AreEqual(210L, message[f("repeated_sfixed64"), 0]);
      Assert.AreEqual(211F, message[f("repeated_float"), 0]);
      Assert.AreEqual(212D, message[f("repeated_double"), 0]);
      Assert.AreEqual(true, message[f("repeated_bool"), 0]);
      Assert.AreEqual("215", message[f("repeated_string"), 0]);
      Assert.AreEqual(TestUtil.ToBytes("216"), message[f("repeated_bytes"), 0]);

      Assert.AreEqual(217, ((IMessage)message[f("repeatedgroup"), 0])[repeatedGroupA]);
      Assert.AreEqual(218, ((IMessage)message[f("repeated_nested_message"), 0])[nestedB]);
      Assert.AreEqual(219, ((IMessage)message[f("repeated_foreign_message"), 0])[foreignC]);
      Assert.AreEqual(220, ((IMessage)message[f("repeated_import_message"), 0])[importD]);

      Assert.AreEqual(nestedBar, message[f("repeated_nested_enum"), 0]);
      Assert.AreEqual(foreignBar, message[f("repeated_foreign_enum"), 0]);
      Assert.AreEqual(importBar, message[f("repeated_import_enum"), 0]);

      Assert.AreEqual("224", message[f("repeated_string_piece"), 0]);
      Assert.AreEqual("225", message[f("repeated_cord"), 0]);

      Assert.AreEqual(301, message[f("repeated_int32"), 1]);
      Assert.AreEqual(302L, message[f("repeated_int64"), 1]);
      Assert.AreEqual(303U, message[f("repeated_uint32"), 1]);
      Assert.AreEqual(304UL, message[f("repeated_uint64"), 1]);
      Assert.AreEqual(305, message[f("repeated_sint32"), 1]);
      Assert.AreEqual(306L, message[f("repeated_sint64"), 1]);
      Assert.AreEqual(307U, message[f("repeated_fixed32"), 1]);
      Assert.AreEqual(308UL, message[f("repeated_fixed64"), 1]);
      Assert.AreEqual(309, message[f("repeated_sfixed32"), 1]);
      Assert.AreEqual(310L, message[f("repeated_sfixed64"), 1]);
      Assert.AreEqual(311F, message[f("repeated_float"), 1]);
      Assert.AreEqual(312D, message[f("repeated_double"), 1]);
      Assert.AreEqual(false, message[f("repeated_bool"), 1]);
      Assert.AreEqual("315", message[f("repeated_string"), 1]);
      Assert.AreEqual(TestUtil.ToBytes("316"), message[f("repeated_bytes"), 1]);

      Assert.AreEqual(317, ((IMessage)message[f("repeatedgroup"), 1])[repeatedGroupA]);
      Assert.AreEqual(318, ((IMessage)message[f("repeated_nested_message"), 1])[nestedB]);
      Assert.AreEqual(319, ((IMessage)message[f("repeated_foreign_message"), 1])[foreignC]);
      Assert.AreEqual(320, ((IMessage)message[f("repeated_import_message"), 1])[importD]);

      Assert.AreEqual(nestedBaz, message[f("repeated_nested_enum"), 1]);
      Assert.AreEqual(foreignBaz, message[f("repeated_foreign_enum"), 1]);
      Assert.AreEqual(importBaz, message[f("repeated_import_enum"), 1]);

      Assert.AreEqual("324", message[f("repeated_string_piece"), 1]);
      Assert.AreEqual("325", message[f("repeated_cord"), 1]);

      // -----------------------------------------------------------------

      Assert.IsTrue(message.HasField(f("default_int32")));
      Assert.IsTrue(message.HasField(f("default_int64")));
      Assert.IsTrue(message.HasField(f("default_uint32")));
      Assert.IsTrue(message.HasField(f("default_uint64")));
      Assert.IsTrue(message.HasField(f("default_sint32")));
      Assert.IsTrue(message.HasField(f("default_sint64")));
      Assert.IsTrue(message.HasField(f("default_fixed32")));
      Assert.IsTrue(message.HasField(f("default_fixed64")));
      Assert.IsTrue(message.HasField(f("default_sfixed32")));
      Assert.IsTrue(message.HasField(f("default_sfixed64")));
      Assert.IsTrue(message.HasField(f("default_float")));
      Assert.IsTrue(message.HasField(f("default_double")));
      Assert.IsTrue(message.HasField(f("default_bool")));
      Assert.IsTrue(message.HasField(f("default_string")));
      Assert.IsTrue(message.HasField(f("default_bytes")));

      Assert.IsTrue(message.HasField(f("default_nested_enum")));
      Assert.IsTrue(message.HasField(f("default_foreign_enum")));
      Assert.IsTrue(message.HasField(f("default_import_enum")));

      Assert.IsTrue(message.HasField(f("default_string_piece")));
      Assert.IsTrue(message.HasField(f("default_cord")));

      Assert.AreEqual(401, message[f("default_int32")]);
      Assert.AreEqual(402L, message[f("default_int64")]);
      Assert.AreEqual(403U, message[f("default_uint32")]);
      Assert.AreEqual(404UL, message[f("default_uint64")]);
      Assert.AreEqual(405, message[f("default_sint32")]);
      Assert.AreEqual(406L, message[f("default_sint64")]);
      Assert.AreEqual(407U, message[f("default_fixed32")]);
      Assert.AreEqual(408UL, message[f("default_fixed64")]);
      Assert.AreEqual(409, message[f("default_sfixed32")]);
      Assert.AreEqual(410L, message[f("default_sfixed64")]);
      Assert.AreEqual(411F, message[f("default_float")]);
      Assert.AreEqual(412D, message[f("default_double")]);
      Assert.AreEqual(false, message[f("default_bool")]);
      Assert.AreEqual("415", message[f("default_string")]);
      Assert.AreEqual(TestUtil.ToBytes("416"), message[f("default_bytes")]);

      Assert.AreEqual(nestedFoo, message[f("default_nested_enum")]);
      Assert.AreEqual(foreignFoo, message[f("default_foreign_enum")]);
      Assert.AreEqual(importFoo, message[f("default_import_enum")]);

      Assert.AreEqual("424", message[f("default_string_piece")]);
      Assert.AreEqual("425", message[f("default_cord")]);
    }

    /// <summary>
    /// Assert that all fields of the message are cleared, and that
    /// getting the fields returns their default values, using the reflection interface.
    /// </summary>
    public void AssertClearViaReflection(IMessage message) {
      // has_blah() should initially be false for all optional fields.
      Assert.IsFalse(message.HasField(f("optional_int32")));
      Assert.IsFalse(message.HasField(f("optional_int64")));
      Assert.IsFalse(message.HasField(f("optional_uint32")));
      Assert.IsFalse(message.HasField(f("optional_uint64")));
      Assert.IsFalse(message.HasField(f("optional_sint32")));
      Assert.IsFalse(message.HasField(f("optional_sint64")));
      Assert.IsFalse(message.HasField(f("optional_fixed32")));
      Assert.IsFalse(message.HasField(f("optional_fixed64")));
      Assert.IsFalse(message.HasField(f("optional_sfixed32")));
      Assert.IsFalse(message.HasField(f("optional_sfixed64")));
      Assert.IsFalse(message.HasField(f("optional_float")));
      Assert.IsFalse(message.HasField(f("optional_double")));
      Assert.IsFalse(message.HasField(f("optional_bool")));
      Assert.IsFalse(message.HasField(f("optional_string")));
      Assert.IsFalse(message.HasField(f("optional_bytes")));

      Assert.IsFalse(message.HasField(f("optionalgroup")));
      Assert.IsFalse(message.HasField(f("optional_nested_message")));
      Assert.IsFalse(message.HasField(f("optional_foreign_message")));
      Assert.IsFalse(message.HasField(f("optional_import_message")));

      Assert.IsFalse(message.HasField(f("optional_nested_enum")));
      Assert.IsFalse(message.HasField(f("optional_foreign_enum")));
      Assert.IsFalse(message.HasField(f("optional_import_enum")));

      Assert.IsFalse(message.HasField(f("optional_string_piece")));
      Assert.IsFalse(message.HasField(f("optional_cord")));

      // Optional fields without defaults are set to zero or something like it.
      Assert.AreEqual(0, message[f("optional_int32")]);
      Assert.AreEqual(0L, message[f("optional_int64")]);
      Assert.AreEqual(0U, message[f("optional_uint32")]);
      Assert.AreEqual(0UL, message[f("optional_uint64")]);
      Assert.AreEqual(0, message[f("optional_sint32")]);
      Assert.AreEqual(0L, message[f("optional_sint64")]);
      Assert.AreEqual(0U, message[f("optional_fixed32")]);
      Assert.AreEqual(0UL, message[f("optional_fixed64")]);
      Assert.AreEqual(0, message[f("optional_sfixed32")]);
      Assert.AreEqual(0L, message[f("optional_sfixed64")]);
      Assert.AreEqual(0F, message[f("optional_float")]);
      Assert.AreEqual(0D, message[f("optional_double")]);
      Assert.AreEqual(false, message[f("optional_bool")]);
      Assert.AreEqual("", message[f("optional_string")]);
      Assert.AreEqual(ByteString.Empty, message[f("optional_bytes")]);

      // Embedded messages should also be clear.
      Assert.IsFalse(((IMessage)message[f("optionalgroup")]).HasField(groupA));
      Assert.IsFalse(((IMessage)message[f("optional_nested_message")])
                         .HasField(nestedB));
      Assert.IsFalse(((IMessage)message[f("optional_foreign_message")])
                         .HasField(foreignC));
      Assert.IsFalse(((IMessage)message[f("optional_import_message")])
                         .HasField(importD));

      Assert.AreEqual(0, ((IMessage)message[f("optionalgroup")])[groupA]);
      Assert.AreEqual(0, ((IMessage)message[f("optional_nested_message")])[nestedB]);
      Assert.AreEqual(0, ((IMessage)message[f("optional_foreign_message")])[foreignC]);
      Assert.AreEqual(0, ((IMessage)message[f("optional_import_message")])[importD]);

      // Enums without defaults are set to the first value in the enum.
      Assert.AreEqual(nestedFoo, message[f("optional_nested_enum")]);
      Assert.AreEqual(foreignFoo, message[f("optional_foreign_enum")]);
      Assert.AreEqual(importFoo, message[f("optional_import_enum")]);

      Assert.AreEqual("", message[f("optional_string_piece")]);
      Assert.AreEqual("", message[f("optional_cord")]);

      // Repeated fields are empty.
      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_int32")));
      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_int64")));
      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_uint32")));
      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_uint64")));
      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_sint32")));
      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_sint64")));
      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_fixed32")));
      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_fixed64")));
      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_sfixed32")));
      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_sfixed64")));
      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_float")));
      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_double")));
      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_bool")));
      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_string")));
      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_bytes")));

      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeatedgroup")));
      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_nested_message")));
      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_foreign_message")));
      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_import_message")));
      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_nested_enum")));
      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_foreign_enum")));
      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_import_enum")));

      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_string_piece")));
      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_cord")));

      // has_blah() should also be false for all default fields.
      Assert.IsFalse(message.HasField(f("default_int32")));
      Assert.IsFalse(message.HasField(f("default_int64")));
      Assert.IsFalse(message.HasField(f("default_uint32")));
      Assert.IsFalse(message.HasField(f("default_uint64")));
      Assert.IsFalse(message.HasField(f("default_sint32")));
      Assert.IsFalse(message.HasField(f("default_sint64")));
      Assert.IsFalse(message.HasField(f("default_fixed32")));
      Assert.IsFalse(message.HasField(f("default_fixed64")));
      Assert.IsFalse(message.HasField(f("default_sfixed32")));
      Assert.IsFalse(message.HasField(f("default_sfixed64")));
      Assert.IsFalse(message.HasField(f("default_float")));
      Assert.IsFalse(message.HasField(f("default_double")));
      Assert.IsFalse(message.HasField(f("default_bool")));
      Assert.IsFalse(message.HasField(f("default_string")));
      Assert.IsFalse(message.HasField(f("default_bytes")));

      Assert.IsFalse(message.HasField(f("default_nested_enum")));
      Assert.IsFalse(message.HasField(f("default_foreign_enum")));
      Assert.IsFalse(message.HasField(f("default_import_enum")));

      Assert.IsFalse(message.HasField(f("default_string_piece")));
      Assert.IsFalse(message.HasField(f("default_cord")));

      // Fields with defaults have their default values (duh).
      Assert.AreEqual(41, message[f("default_int32")]);
      Assert.AreEqual(42L, message[f("default_int64")]);
      Assert.AreEqual(43U, message[f("default_uint32")]);
      Assert.AreEqual(44UL, message[f("default_uint64")]);
      Assert.AreEqual(-45, message[f("default_sint32")]);
      Assert.AreEqual(46L, message[f("default_sint64")]);
      Assert.AreEqual(47U, message[f("default_fixed32")]);
      Assert.AreEqual(48UL, message[f("default_fixed64")]);
      Assert.AreEqual(49, message[f("default_sfixed32")]);
      Assert.AreEqual(-50L, message[f("default_sfixed64")]);
      Assert.AreEqual(51.5F, message[f("default_float")]);
      Assert.AreEqual(52e3D, message[f("default_double")]);
      Assert.AreEqual(true, message[f("default_bool")]);
      Assert.AreEqual("hello", message[f("default_string")]);
      Assert.AreEqual(TestUtil.ToBytes("world"), message[f("default_bytes")]);

      Assert.AreEqual(nestedBar, message[f("default_nested_enum")]);
      Assert.AreEqual(foreignBar, message[f("default_foreign_enum")]);
      Assert.AreEqual(importBar, message[f("default_import_enum")]);

      Assert.AreEqual("abc", message[f("default_string_piece")]);
      Assert.AreEqual("123", message[f("default_cord")]);
    }

    // ---------------------------------------------------------------

    internal void AssertRepeatedFieldsModifiedViaReflection(IMessage message) {
      // ModifyRepeatedFields only sets the second repeated element of each
      // field.  In addition to verifying this, we also verify that the first
      // element and size were *not* modified.
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_int32")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_int64")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_uint32")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_uint64")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_sint32")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_sint64")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_fixed32")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_fixed64")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_sfixed32")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_sfixed64")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_float")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_double")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_bool")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_string")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_bytes")));

      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeatedgroup")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_nested_message")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_foreign_message")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_import_message")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_nested_enum")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_foreign_enum")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_import_enum")));

      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_string_piece")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_cord")));

      Assert.AreEqual(201, message[f("repeated_int32"), 0]);
      Assert.AreEqual(202L, message[f("repeated_int64"), 0]);
      Assert.AreEqual(203U, message[f("repeated_uint32"), 0]);
      Assert.AreEqual(204UL, message[f("repeated_uint64"), 0]);
      Assert.AreEqual(205, message[f("repeated_sint32"), 0]);
      Assert.AreEqual(206L, message[f("repeated_sint64"), 0]);
      Assert.AreEqual(207U, message[f("repeated_fixed32"), 0]);
      Assert.AreEqual(208UL, message[f("repeated_fixed64"), 0]);
      Assert.AreEqual(209, message[f("repeated_sfixed32"), 0]);
      Assert.AreEqual(210L, message[f("repeated_sfixed64"), 0]);
      Assert.AreEqual(211F, message[f("repeated_float"), 0]);
      Assert.AreEqual(212D, message[f("repeated_double"), 0]);
      Assert.AreEqual(true, message[f("repeated_bool"), 0]);
      Assert.AreEqual("215", message[f("repeated_string"), 0]);
      Assert.AreEqual(TestUtil.ToBytes("216"), message[f("repeated_bytes"), 0]);

      Assert.AreEqual(217, ((IMessage)message[f("repeatedgroup"), 0])[repeatedGroupA]);
      Assert.AreEqual(218, ((IMessage)message[f("repeated_nested_message"), 0])[nestedB]);
      Assert.AreEqual(219, ((IMessage)message[f("repeated_foreign_message"), 0])[foreignC]);
      Assert.AreEqual(220, ((IMessage)message[f("repeated_import_message"), 0])[importD]);

      Assert.AreEqual(nestedBar, message[f("repeated_nested_enum"), 0]);
      Assert.AreEqual(foreignBar, message[f("repeated_foreign_enum"), 0]);
      Assert.AreEqual(importBar, message[f("repeated_import_enum"), 0]);

      Assert.AreEqual("224", message[f("repeated_string_piece"), 0]);
      Assert.AreEqual("225", message[f("repeated_cord"), 0]);

      Assert.AreEqual(501, message[f("repeated_int32"), 1]);
      Assert.AreEqual(502L, message[f("repeated_int64"), 1]);
      Assert.AreEqual(503U, message[f("repeated_uint32"), 1]);
      Assert.AreEqual(504UL, message[f("repeated_uint64"), 1]);
      Assert.AreEqual(505, message[f("repeated_sint32"), 1]);
      Assert.AreEqual(506L, message[f("repeated_sint64"), 1]);
      Assert.AreEqual(507U, message[f("repeated_fixed32"), 1]);
      Assert.AreEqual(508UL, message[f("repeated_fixed64"), 1]);
      Assert.AreEqual(509, message[f("repeated_sfixed32"), 1]);
      Assert.AreEqual(510L, message[f("repeated_sfixed64"), 1]);
      Assert.AreEqual(511F, message[f("repeated_float"), 1]);
      Assert.AreEqual(512D, message[f("repeated_double"), 1]);
      Assert.AreEqual(true, message[f("repeated_bool"), 1]);
      Assert.AreEqual("515", message[f("repeated_string"), 1]);
      Assert.AreEqual(TestUtil.ToBytes("516"), message[f("repeated_bytes"), 1]);

      Assert.AreEqual(517, ((IMessage)message[f("repeatedgroup"), 1])[repeatedGroupA]);
      Assert.AreEqual(518, ((IMessage)message[f("repeated_nested_message"), 1])[nestedB]);
      Assert.AreEqual(519, ((IMessage)message[f("repeated_foreign_message"), 1])[foreignC]);
      Assert.AreEqual(520, ((IMessage)message[f("repeated_import_message"), 1])[importD]);

      Assert.AreEqual(nestedFoo, message[f("repeated_nested_enum"), 1]);
      Assert.AreEqual(foreignFoo, message[f("repeated_foreign_enum"), 1]);
      Assert.AreEqual(importFoo, message[f("repeated_import_enum"), 1]);

      Assert.AreEqual("524", message[f("repeated_string_piece"), 1]);
      Assert.AreEqual("525", message[f("repeated_cord"), 1]);
    }

    /// <summary>
    /// Verifies that the reflection setters for the given Builder object throw an
    /// ArgumentNullException if they are passed a null value. 
    /// </summary>
    public void AssertReflectionSettersRejectNull(IBuilder builder) {
      TestUtil.AssertArgumentNullException(() => builder[f("optional_string")] = null);
      TestUtil.AssertArgumentNullException(() => builder[f("optional_bytes")] = null);
      TestUtil.AssertArgumentNullException(() => builder[f("optional_nested_enum")] = null);
      TestUtil.AssertArgumentNullException(() => builder[f("optional_nested_message")] = null);
      TestUtil.AssertArgumentNullException(() => builder[f("optional_nested_message")] = null);
      TestUtil.AssertArgumentNullException(() => builder.WeakAddRepeatedField(f("repeated_string"), null));
      TestUtil.AssertArgumentNullException(() => builder.WeakAddRepeatedField(f("repeated_bytes"), null));
      TestUtil.AssertArgumentNullException(() => builder.WeakAddRepeatedField(f("repeated_nested_enum"), null));
      TestUtil.AssertArgumentNullException(() => builder.WeakAddRepeatedField(f("repeated_nested_message"), null));
    }

    /// <summary>
    /// Verifies that the reflection repeated setters for the given Builder object throw an
    /// ArgumentNullException if they are passed a null value.
    /// </summary>
    public void AssertReflectionRepeatedSettersRejectNull(IBuilder builder) {
      builder.WeakAddRepeatedField(f("repeated_string"), "one");
      TestUtil.AssertArgumentNullException(() => builder.SetRepeatedField(f("repeated_string"), 0, null));
      builder.WeakAddRepeatedField(f("repeated_bytes"), TestUtil.ToBytes("one"));
      TestUtil.AssertArgumentNullException(() => builder.SetRepeatedField(f("repeated_bytes"), 0, null));
      builder.WeakAddRepeatedField(f("repeated_nested_enum"), nestedBaz);
      TestUtil.AssertArgumentNullException(() => builder.SetRepeatedField(f("repeated_nested_enum"), 0, null));
      builder.WeakAddRepeatedField(f("repeated_nested_message"),
          new TestAllTypes.Types.NestedMessage.Builder { Bb = 218 }.Build());
      TestUtil.AssertArgumentNullException(() => builder.SetRepeatedField(f("repeated_nested_message"), 0, null));
    }

    public void SetPackedFieldsViaReflection(IBuilder message) {
      message.WeakAddRepeatedField(f("packed_int32"), 601);
      message.WeakAddRepeatedField(f("packed_int64"), 602L);
      message.WeakAddRepeatedField(f("packed_uint32"), 603U);
      message.WeakAddRepeatedField(f("packed_uint64"), 604UL);
      message.WeakAddRepeatedField(f("packed_sint32"), 605);
      message.WeakAddRepeatedField(f("packed_sint64"), 606L);
      message.WeakAddRepeatedField(f("packed_fixed32"), 607U);
      message.WeakAddRepeatedField(f("packed_fixed64"), 608UL);
      message.WeakAddRepeatedField(f("packed_sfixed32"), 609);
      message.WeakAddRepeatedField(f("packed_sfixed64"), 610L);
      message.WeakAddRepeatedField(f("packed_float"), 611F);
      message.WeakAddRepeatedField(f("packed_double"), 612D);
      message.WeakAddRepeatedField(f("packed_bool"), true);
      message.WeakAddRepeatedField(f("packed_enum"), foreignBar);
      // Add a second one of each field.
      message.WeakAddRepeatedField(f("packed_int32"), 701);
      message.WeakAddRepeatedField(f("packed_int64"), 702L);
      message.WeakAddRepeatedField(f("packed_uint32"), 703U);
      message.WeakAddRepeatedField(f("packed_uint64"), 704UL);
      message.WeakAddRepeatedField(f("packed_sint32"), 705);
      message.WeakAddRepeatedField(f("packed_sint64"), 706L);
      message.WeakAddRepeatedField(f("packed_fixed32"), 707U);
      message.WeakAddRepeatedField(f("packed_fixed64"), 708UL);
      message.WeakAddRepeatedField(f("packed_sfixed32"), 709);
      message.WeakAddRepeatedField(f("packed_sfixed64"), 710L);
      message.WeakAddRepeatedField(f("packed_float"), 711F);
      message.WeakAddRepeatedField(f("packed_double"), 712D);
      message.WeakAddRepeatedField(f("packed_bool"), false);
      message.WeakAddRepeatedField(f("packed_enum"), foreignBaz);
    }

    public void AssertPackedFieldsSetViaReflection(IMessage message) {
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("packed_int32")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("packed_int64")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("packed_uint32")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("packed_uint64")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("packed_sint32")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("packed_sint64")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("packed_fixed32")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("packed_fixed64")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("packed_sfixed32")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("packed_sfixed64")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("packed_float")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("packed_double")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("packed_bool")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("packed_enum")));
      Assert.AreEqual(601, message[f("packed_int32"), 0]);
      Assert.AreEqual(602L, message[f("packed_int64"), 0]);
      Assert.AreEqual(603, message[f("packed_uint32"), 0]);
      Assert.AreEqual(604L, message[f("packed_uint64"), 0]);
      Assert.AreEqual(605, message[f("packed_sint32"), 0]);
      Assert.AreEqual(606L, message[f("packed_sint64"), 0]);
      Assert.AreEqual(607, message[f("packed_fixed32"), 0]);
      Assert.AreEqual(608L, message[f("packed_fixed64"), 0]);
      Assert.AreEqual(609, message[f("packed_sfixed32"), 0]);
      Assert.AreEqual(610L, message[f("packed_sfixed64"), 0]);
      Assert.AreEqual(611F, message[f("packed_float"), 0]);
      Assert.AreEqual(612D, message[f("packed_double"), 0]);
      Assert.AreEqual(true, message[f("packed_bool"), 0]);
      Assert.AreEqual(foreignBar, message[f("packed_enum"), 0]);
      Assert.AreEqual(701, message[f("packed_int32"), 1]);
      Assert.AreEqual(702L, message[f("packed_int64"), 1]);
      Assert.AreEqual(703, message[f("packed_uint32"), 1]);
      Assert.AreEqual(704L, message[f("packed_uint64"), 1]);
      Assert.AreEqual(705, message[f("packed_sint32"), 1]);
      Assert.AreEqual(706L, message[f("packed_sint64"), 1]);
      Assert.AreEqual(707, message[f("packed_fixed32"), 1]);
      Assert.AreEqual(708L, message[f("packed_fixed64"), 1]);
      Assert.AreEqual(709, message[f("packed_sfixed32"), 1]);
      Assert.AreEqual(710L, message[f("packed_sfixed64"), 1]);
      Assert.AreEqual(711F, message[f("packed_float"), 1]);
      Assert.AreEqual(712D, message[f("packed_double"), 1]);
      Assert.AreEqual(false, message[f("packed_bool"), 1]);
      Assert.AreEqual(foreignBaz, message[f("packed_enum"), 1]);
    }
  }
}
