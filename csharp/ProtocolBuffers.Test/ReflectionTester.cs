using System;
using System.Collections.Generic;
using System.Text;
using Google.ProtocolBuffers.Descriptors;
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
    /// TODO(jonskeet): Enforce all of these with two factory methods.
    /// </summary>
    public ReflectionTester(MessageDescriptor baseDescriptor,
                            ExtensionRegistry extensionRegistry) {
      this.baseDescriptor = baseDescriptor;
      this.extensionRegistry = extensionRegistry;

      this.file = baseDescriptor.File;
      Assert.AreEqual(1, file.Dependencies.Count);
      this.importFile = file.Dependencies[0];

      MessageDescriptor testAllTypes;
      if (extensionRegistry == null) {
        testAllTypes = baseDescriptor;
      } else {
        testAllTypes = file.FindTypeByName<MessageDescriptor>("TestAllTypes");
        Assert.IsNotNull(testAllTypes);
      }

      if (extensionRegistry == null) {
        this.optionalGroup =
          baseDescriptor.FindDescriptor<MessageDescriptor>("OptionalGroup");
        this.repeatedGroup =
          baseDescriptor.FindDescriptor<MessageDescriptor>("RepeatedGroup");
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

      Assert.IsNotNull(optionalGroup );
      Assert.IsNotNull(repeatedGroup );
      Assert.IsNotNull(nestedMessage );
      Assert.IsNotNull(foreignMessage);
      Assert.IsNotNull(importMessage );
      Assert.IsNotNull(nestedEnum    );
      Assert.IsNotNull(foreignEnum   );
      Assert.IsNotNull(importEnum    );

      this.nestedB  = nestedMessage.FindDescriptor<FieldDescriptor>("bb");
      this.foreignC = foreignMessage.FindDescriptor<FieldDescriptor>("c");
      this.importD  = importMessage .FindDescriptor<FieldDescriptor>("d");
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

      Assert.IsNotNull(groupA        );
      Assert.IsNotNull(repeatedGroupA);
      Assert.IsNotNull(nestedB       );
      Assert.IsNotNull(foreignC      );
      Assert.IsNotNull(importD       );
      Assert.IsNotNull(nestedFoo     );
      Assert.IsNotNull(nestedBar     );
      Assert.IsNotNull(nestedBaz     );
      Assert.IsNotNull(foreignFoo    );
      Assert.IsNotNull(foreignBar    );
      Assert.IsNotNull(foreignBaz    );
      Assert.IsNotNull(importFoo     );
      Assert.IsNotNull(importBar     );
      Assert.IsNotNull(importBaz     );
    }

    /**
     * Shorthand to get a FieldDescriptor for a field of unittest::TestAllTypes.
     */
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

    /**
     * Calls {@code parent.CreateBuilderForField()} or uses the
     * {@code ExtensionRegistry} to find an appropriateIBuilder, depending
     * on what type is being tested.
     */
    private IBuilder CreateBuilderForField(IBuilder parent, FieldDescriptor field) {
      if (extensionRegistry == null) {
        return parent.CreateBuilderForField(field);
      } else {
        ExtensionInfo extension = extensionRegistry[field.ContainingType, field.FieldNumber];
        Assert.IsNotNull(extension);
        Assert.IsNotNull(extension.DefaultInstance);
        return extension.DefaultInstance.CreateBuilderForType();
      }
    }

    // -------------------------------------------------------------------

    /**
     * Set every field of {@code message} to the values expected by
     * {@code assertAllFieldsSet()}, using the {@link MessageIBuilder}
     * reflection interface.
     */
    internal void SetAllFieldsViaReflection(IBuilder message) {
      message[f("optional_int32"   )] = 101 ;
      message[f("optional_int64"   )] = 102L;
      message[f("optional_uint32"  )] = 103 ;
      message[f("optional_uint64"  )] = 104L;
      message[f("optional_sint32"  )] = 105 ;
      message[f("optional_sint64"  )] = 106L;
      message[f("optional_fixed32" )] = 107 ;
      message[f("optional_fixed64" )] = 108L;
      message[f("optional_sfixed32")] = 109 ;
      message[f("optional_sfixed64")] = 110L;
      message[f("optional_float"   )] = 111F;
      message[f("optional_double"  )] = 112D;
      message[f("optional_bool"    )] = true;
      message[f("optional_string"  )] = "115";
      message[f("optional_bytes")] = TestUtil.ToBytes("116");

      message[f("optionalgroup")] = CreateBuilderForField(message, f("optionalgroup")).SetField(groupA, 117).Build();
      message[f("optional_nested_message")] = CreateBuilderForField(message, f("optional_nested_message")).SetField(nestedB, 118).Build();
      message[f("optional_foreign_message")] = CreateBuilderForField(message, f("optional_foreign_message")).SetField(foreignC, 119).Build();
      message[f("optional_import_message")] = CreateBuilderForField(message, f("optional_import_message")).SetField(importD, 120).Build();

      message[f("optional_nested_enum" )] =  nestedBaz;
      message[f("optional_foreign_enum")] = foreignBaz;
      message[f("optional_import_enum" )] =  importBaz;

      message[f("optional_string_piece" )] = "124";
      message[f("optional_cord" )] = "125";

      // -----------------------------------------------------------------

      message.AddRepeatedField(f("repeated_int32"   ), 201 );
      message.AddRepeatedField(f("repeated_int64"   ), 202L);
      message.AddRepeatedField(f("repeated_uint32"  ), 203 );
      message.AddRepeatedField(f("repeated_uint64"  ), 204L);
      message.AddRepeatedField(f("repeated_sint32"  ), 205 );
      message.AddRepeatedField(f("repeated_sint64"  ), 206L);
      message.AddRepeatedField(f("repeated_fixed32" ), 207 );
      message.AddRepeatedField(f("repeated_fixed64" ), 208L);
      message.AddRepeatedField(f("repeated_sfixed32"), 209 );
      message.AddRepeatedField(f("repeated_sfixed64"), 210L);
      message.AddRepeatedField(f("repeated_float"   ), 211F);
      message.AddRepeatedField(f("repeated_double"  ), 212D);
      message.AddRepeatedField(f("repeated_bool"    ), true);
      message.AddRepeatedField(f("repeated_string"  ), "215");
      message.AddRepeatedField(f("repeated_bytes"   ), TestUtil.ToBytes("216"));


      message.AddRepeatedField(f("repeatedgroup"), CreateBuilderForField(message, f("repeatedgroup")).SetField(repeatedGroupA, 217).Build());
      message.AddRepeatedField(f("repeated_nested_message"), CreateBuilderForField(message, f("repeated_nested_message")).SetField(nestedB, 218).Build());
      message.AddRepeatedField(f("repeated_foreign_message"), CreateBuilderForField(message, f("repeated_foreign_message")).SetField(foreignC, 219).Build());
      message.AddRepeatedField(f("repeated_import_message"), CreateBuilderForField(message, f("repeated_import_message")).SetField(importD, 220).Build());

      message.AddRepeatedField(f("repeated_nested_enum" ),  nestedBar);
      message.AddRepeatedField(f("repeated_foreign_enum"), foreignBar);
      message.AddRepeatedField(f("repeated_import_enum" ),  importBar);

      message.AddRepeatedField(f("repeated_string_piece" ), "224");
      message.AddRepeatedField(f("repeated_cord" ), "225");

      // Add a second one of each field.
      message.AddRepeatedField(f("repeated_int32"   ), 301 );
      message.AddRepeatedField(f("repeated_int64"   ), 302L);
      message.AddRepeatedField(f("repeated_uint32"  ), 303 );
      message.AddRepeatedField(f("repeated_uint64"  ), 304L);
      message.AddRepeatedField(f("repeated_sint32"  ), 305 );
      message.AddRepeatedField(f("repeated_sint64"  ), 306L);
      message.AddRepeatedField(f("repeated_fixed32" ), 307 );
      message.AddRepeatedField(f("repeated_fixed64" ), 308L);
      message.AddRepeatedField(f("repeated_sfixed32"), 309 );
      message.AddRepeatedField(f("repeated_sfixed64"), 310L);
      message.AddRepeatedField(f("repeated_float"   ), 311F);
      message.AddRepeatedField(f("repeated_double"  ), 312D);
      message.AddRepeatedField(f("repeated_bool"    ), false);
      message.AddRepeatedField(f("repeated_string"  ), "315");
      message.AddRepeatedField(f("repeated_bytes"   ), TestUtil.ToBytes("316"));

      message.AddRepeatedField(f("repeatedgroup"),
        CreateBuilderForField(message, f("repeatedgroup"))
               .SetField(repeatedGroupA, 317).Build());
      message.AddRepeatedField(f("repeated_nested_message"),
        CreateBuilderForField(message, f("repeated_nested_message"))
               .SetField(nestedB, 318).Build());
      message.AddRepeatedField(f("repeated_foreign_message"),
        CreateBuilderForField(message, f("repeated_foreign_message"))
               .SetField(foreignC, 319).Build());
      message.AddRepeatedField(f("repeated_import_message"),
        CreateBuilderForField(message, f("repeated_import_message"))
               .SetField(importD, 320).Build());

      message.AddRepeatedField(f("repeated_nested_enum" ),  nestedBaz);
      message.AddRepeatedField(f("repeated_foreign_enum"), foreignBaz);
      message.AddRepeatedField(f("repeated_import_enum" ),  importBaz);

      message.AddRepeatedField(f("repeated_string_piece" ), "324");
      message.AddRepeatedField(f("repeated_cord" ), "325");

      // -----------------------------------------------------------------

      message[f("default_int32"   )] = 401 ;
      message[f("default_int64"   )] = 402L;
      message[f("default_uint32"  )] = 403 ;
      message[f("default_uint64"  )] = 404L;
      message[f("default_sint32"  )] = 405 ;
      message[f("default_sint64"  )] = 406L;
      message[f("default_fixed32" )] = 407 ;
      message[f("default_fixed64" )] = 408L;
      message[f("default_sfixed32")] = 409 ;
      message[f("default_sfixed64")] = 410L;
      message[f("default_float"   )] = 411F;
      message[f("default_double"  )] = 412D;
      message[f("default_bool"    )] = false;
      message[f("default_string"  )] = "415";
      message[f("default_bytes"   )] = TestUtil.ToBytes("416");

      message[f("default_nested_enum" )] =  nestedFoo;
      message[f("default_foreign_enum")] = foreignFoo;
      message[f("default_import_enum" )] =  importFoo;

      message[f("default_string_piece" )] = "424";
      message[f("default_cord" )] = "425";
    }

    // -------------------------------------------------------------------

    /// <summary>
    /// Modify the repeated fields of the specified message to contain the
    /// values expected by AssertRepeatedFieldsModified, using the IBuilder
    /// reflection interface.
    /// </summary>
    internal void ModifyRepeatedFieldsViaReflection(IBuilder message) {
      message[f("repeated_int32"   ), 1] = 501 ;
      message[f("repeated_int64"   ), 1] = 502L;
      message[f("repeated_uint32"  ), 1] = 503 ;
      message[f("repeated_uint64"  ), 1] = 504L;
      message[f("repeated_sint32"  ), 1] = 505 ;
      message[f("repeated_sint64"  ), 1] = 506L;
      message[f("repeated_fixed32" ), 1] = 507 ;
      message[f("repeated_fixed64" ), 1] = 508L;
      message[f("repeated_sfixed32"), 1] = 509 ;
      message[f("repeated_sfixed64"), 1] = 510L;
      message[f("repeated_float"   ), 1] = 511F;
      message[f("repeated_double"  ), 1] = 512D;
      message[f("repeated_bool"    ), 1] = true;
      message[f("repeated_string"  ), 1] = "515";
      message.SetRepeatedField(f("repeated_bytes"   ), 1, TestUtil.ToBytes("516"));

      message.SetRepeatedField(f("repeatedgroup"), 1, CreateBuilderForField(message, f("repeatedgroup")).SetField(repeatedGroupA, 517).Build());
      message.SetRepeatedField(f("repeated_nested_message"), 1, CreateBuilderForField(message, f("repeated_nested_message")).SetField(nestedB, 518).Build());
      message.SetRepeatedField(f("repeated_foreign_message"), 1, CreateBuilderForField(message, f("repeated_foreign_message")).SetField(foreignC, 519).Build());
      message.SetRepeatedField(f("repeated_import_message"), 1, CreateBuilderForField(message, f("repeated_import_message")).SetField(importD, 520).Build());

      message[f("repeated_nested_enum" ), 1] =  nestedFoo;
      message[f("repeated_foreign_enum"), 1] = foreignFoo;
      message[f("repeated_import_enum" ), 1] =  importFoo;

      message[f("repeated_string_piece"), 1] = "524";
      message[f("repeated_cord"), 1] = "525";
    }

    // -------------------------------------------------------------------

    /// <summary>
    /// Asserts that all fields of the specified message are set to the values
    /// assigned by SetAllFields, using the IMessage reflection interface.
    /// </summary>
    public void assertAllFieldsSetViaReflection(IMessage message) {
      Assert.IsTrue(message.HasField(f("optional_int32"   )));
      Assert.IsTrue(message.HasField(f("optional_int64"   )));
      Assert.IsTrue(message.HasField(f("optional_uint32"  )));
      Assert.IsTrue(message.HasField(f("optional_uint64"  )));
      Assert.IsTrue(message.HasField(f("optional_sint32"  )));
      Assert.IsTrue(message.HasField(f("optional_sint64"  )));
      Assert.IsTrue(message.HasField(f("optional_fixed32" )));
      Assert.IsTrue(message.HasField(f("optional_fixed64" )));
      Assert.IsTrue(message.HasField(f("optional_sfixed32")));
      Assert.IsTrue(message.HasField(f("optional_sfixed64")));
      Assert.IsTrue(message.HasField(f("optional_float"   )));
      Assert.IsTrue(message.HasField(f("optional_double"  )));
      Assert.IsTrue(message.HasField(f("optional_bool"    )));
      Assert.IsTrue(message.HasField(f("optional_string"  )));
      Assert.IsTrue(message.HasField(f("optional_bytes"   )));

      Assert.IsTrue(message.HasField(f("optionalgroup"           )));
      Assert.IsTrue(message.HasField(f("optional_nested_message" )));
      Assert.IsTrue(message.HasField(f("optional_foreign_message")));
      Assert.IsTrue(message.HasField(f("optional_import_message" )));

      Assert.IsTrue(((IMessage)message[f("optionalgroup")]).HasField(groupA));
      Assert.IsTrue(((IMessage)message[f("optional_nested_message")]).HasField(nestedB));
      Assert.IsTrue(((IMessage)message[f("optional_foreign_message")]).HasField(foreignC));
      Assert.IsTrue(((IMessage)message[f("optional_import_message")]).HasField(importD));

      Assert.IsTrue(message.HasField(f("optional_nested_enum" )));
      Assert.IsTrue(message.HasField(f("optional_foreign_enum")));
      Assert.IsTrue(message.HasField(f("optional_import_enum" )));

      Assert.IsTrue(message.HasField(f("optional_string_piece")));
      Assert.IsTrue(message.HasField(f("optional_cord")));

      Assert.AreEqual(101  , message[f("optional_int32"   )]);
      Assert.AreEqual(102L , message[f("optional_int64"   )]);
      Assert.AreEqual(103  , message[f("optional_uint32"  )]);
      Assert.AreEqual(104L , message[f("optional_uint64"  )]);
      Assert.AreEqual(105  , message[f("optional_sint32"  )]);
      Assert.AreEqual(106L , message[f("optional_sint64"  )]);
      Assert.AreEqual(107  , message[f("optional_fixed32" )]);
      Assert.AreEqual(108L , message[f("optional_fixed64" )]);
      Assert.AreEqual(109  , message[f("optional_sfixed32")]);
      Assert.AreEqual(110L , message[f("optional_sfixed64")]);
      Assert.AreEqual(111F , message[f("optional_float"   )]);
      Assert.AreEqual(112D , message[f("optional_double"  )]);
      Assert.AreEqual(true , message[f("optional_bool"    )]);
      Assert.AreEqual("115", message[f("optional_string"  )]);
      Assert.AreEqual(TestUtil.ToBytes("116"), message[f("optional_bytes")]);

      Assert.AreEqual(117,((IMessage)message[f("optionalgroup")])[groupA]);
      Assert.AreEqual(118,((IMessage)message[f("optional_nested_message")])[nestedB]);
      Assert.AreEqual(119,((IMessage)message[f("optional_foreign_message")])[foreignC]);
      Assert.AreEqual(120,((IMessage)message[f("optional_import_message")])[importD]);

      Assert.AreEqual( nestedBaz, message[f("optional_nested_enum" )]);
      Assert.AreEqual(foreignBaz, message[f("optional_foreign_enum")]);
      Assert.AreEqual( importBaz, message[f("optional_import_enum" )]);

      Assert.AreEqual("124", message[f("optional_string_piece")]);
      Assert.AreEqual("125", message[f("optional_cord")]);

      // -----------------------------------------------------------------

      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_int32"   )));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_int64"   )));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_uint32"  )));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_uint64"  )));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_sint32"  )));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_sint64"  )));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_fixed32" )));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_fixed64" )));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_sfixed32")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_sfixed64")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_float"   )));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_double"  )));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_bool"    )));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_string"  )));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_bytes"   )));

      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeatedgroup"           )));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_nested_message" )));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_foreign_message")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_import_message" )));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_nested_enum"    )));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_foreign_enum"   )));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_import_enum"    )));

      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_string_piece")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_cord")));

      Assert.AreEqual(201  , message[f("repeated_int32"   ), 0]);
      Assert.AreEqual(202L , message[f("repeated_int64"   ), 0]);
      Assert.AreEqual(203  , message[f("repeated_uint32"  ), 0]);
      Assert.AreEqual(204L , message[f("repeated_uint64"  ), 0]);
      Assert.AreEqual(205  , message[f("repeated_sint32"  ), 0]);
      Assert.AreEqual(206L , message[f("repeated_sint64"  ), 0]);
      Assert.AreEqual(207  , message[f("repeated_fixed32" ), 0]);
      Assert.AreEqual(208L , message[f("repeated_fixed64" ), 0]);
      Assert.AreEqual(209  , message[f("repeated_sfixed32"), 0]);
      Assert.AreEqual(210L , message[f("repeated_sfixed64"), 0]);
      Assert.AreEqual(211F , message[f("repeated_float"   ), 0]);
      Assert.AreEqual(212D , message[f("repeated_double"  ), 0]);
      Assert.AreEqual(true , message[f("repeated_bool"    ), 0]);
      Assert.AreEqual("215", message[f("repeated_string"  ), 0]);
      Assert.AreEqual(TestUtil.ToBytes("216"), message[f("repeated_bytes"), 0]);

      Assert.AreEqual(217,((IMessage)message[f("repeatedgroup"), 0])[repeatedGroupA]);
      Assert.AreEqual(218,((IMessage)message[f("repeated_nested_message"), 0])[nestedB]);
      Assert.AreEqual(219,((IMessage)message[f("repeated_foreign_message"), 0])[foreignC]);
      Assert.AreEqual(220,((IMessage)message[f("repeated_import_message"), 0])[importD]);

      Assert.AreEqual( nestedBar, message[f("repeated_nested_enum" ),0]);
      Assert.AreEqual(foreignBar, message[f("repeated_foreign_enum"),0]);
      Assert.AreEqual( importBar, message[f("repeated_import_enum" ),0]);

      Assert.AreEqual("224", message[f("repeated_string_piece"), 0]);
      Assert.AreEqual("225", message[f("repeated_cord"), 0]);

      Assert.AreEqual(301  , message[f("repeated_int32"   ), 1]);
      Assert.AreEqual(302L , message[f("repeated_int64"   ), 1]);
      Assert.AreEqual(303  , message[f("repeated_uint32"  ), 1]);
      Assert.AreEqual(304L , message[f("repeated_uint64"  ), 1]);
      Assert.AreEqual(305  , message[f("repeated_sint32"  ), 1]);
      Assert.AreEqual(306L , message[f("repeated_sint64"  ), 1]);
      Assert.AreEqual(307  , message[f("repeated_fixed32" ), 1]);
      Assert.AreEqual(308L , message[f("repeated_fixed64" ), 1]);
      Assert.AreEqual(309  , message[f("repeated_sfixed32"), 1]);
      Assert.AreEqual(310L , message[f("repeated_sfixed64"), 1]);
      Assert.AreEqual(311F , message[f("repeated_float"   ), 1]);
      Assert.AreEqual(312D , message[f("repeated_double"  ), 1]);
      Assert.AreEqual(false, message[f("repeated_bool"    ), 1]);
      Assert.AreEqual("315", message[f("repeated_string"  ), 1]);
      Assert.AreEqual(TestUtil.ToBytes("316"), message[f("repeated_bytes"), 1]);

      Assert.AreEqual(317,((IMessage)message[f("repeatedgroup"), 1])[repeatedGroupA]);
      Assert.AreEqual(318,((IMessage)message[f("repeated_nested_message"), 1])[nestedB]);
      Assert.AreEqual(319,
        ((IMessage)message[f("repeated_foreign_message"), 1])
                         [foreignC]);
      Assert.AreEqual(320,
        ((IMessage)message[f("repeated_import_message"), 1])
                         [importD]);

      Assert.AreEqual( nestedBaz, message[f("repeated_nested_enum" ),1]);
      Assert.AreEqual(foreignBaz, message[f("repeated_foreign_enum"),1]);
      Assert.AreEqual( importBaz, message[f("repeated_import_enum" ),1]);

      Assert.AreEqual("324", message[f("repeated_string_piece"), 1]);
      Assert.AreEqual("325", message[f("repeated_cord"), 1]);

      // -----------------------------------------------------------------

      Assert.IsTrue(message.HasField(f("default_int32"   )));
      Assert.IsTrue(message.HasField(f("default_int64"   )));
      Assert.IsTrue(message.HasField(f("default_uint32"  )));
      Assert.IsTrue(message.HasField(f("default_uint64"  )));
      Assert.IsTrue(message.HasField(f("default_sint32"  )));
      Assert.IsTrue(message.HasField(f("default_sint64"  )));
      Assert.IsTrue(message.HasField(f("default_fixed32" )));
      Assert.IsTrue(message.HasField(f("default_fixed64" )));
      Assert.IsTrue(message.HasField(f("default_sfixed32")));
      Assert.IsTrue(message.HasField(f("default_sfixed64")));
      Assert.IsTrue(message.HasField(f("default_float"   )));
      Assert.IsTrue(message.HasField(f("default_double"  )));
      Assert.IsTrue(message.HasField(f("default_bool"    )));
      Assert.IsTrue(message.HasField(f("default_string"  )));
      Assert.IsTrue(message.HasField(f("default_bytes"   )));

      Assert.IsTrue(message.HasField(f("default_nested_enum" )));
      Assert.IsTrue(message.HasField(f("default_foreign_enum")));
      Assert.IsTrue(message.HasField(f("default_import_enum" )));

      Assert.IsTrue(message.HasField(f("default_string_piece")));
      Assert.IsTrue(message.HasField(f("default_cord")));

      Assert.AreEqual(401  , message[f("default_int32"   )]);
      Assert.AreEqual(402L , message[f("default_int64"   )]);
      Assert.AreEqual(403  , message[f("default_uint32"  )]);
      Assert.AreEqual(404L , message[f("default_uint64"  )]);
      Assert.AreEqual(405  , message[f("default_sint32"  )]);
      Assert.AreEqual(406L , message[f("default_sint64"  )]);
      Assert.AreEqual(407  , message[f("default_fixed32" )]);
      Assert.AreEqual(408L , message[f("default_fixed64" )]);
      Assert.AreEqual(409  , message[f("default_sfixed32")]);
      Assert.AreEqual(410L , message[f("default_sfixed64")]);
      Assert.AreEqual(411F , message[f("default_float"   )]);
      Assert.AreEqual(412D , message[f("default_double"  )]);
      Assert.AreEqual(false, message[f("default_bool"    )]);
      Assert.AreEqual("415", message[f("default_string"  )]);
      Assert.AreEqual(TestUtil.ToBytes("416"), message[f("default_bytes")]);

      Assert.AreEqual( nestedFoo, message[f("default_nested_enum" )]);
      Assert.AreEqual(foreignFoo, message[f("default_foreign_enum")]);
      Assert.AreEqual( importFoo, message[f("default_import_enum" )]);

      Assert.AreEqual("424", message[f("default_string_piece")]);
      Assert.AreEqual("425", message[f("default_cord")]);
    }

    // -------------------------------------------------------------------

    /**
     * Assert (using {@code junit.framework.Assert}} that all fields of
     * {@code message} are cleared, and that getting the fields returns their
     * default values, using the {@link Message} reflection interface.
     */
    public void assertClearViaReflection(IMessage message) {
      // has_blah() should initially be false for all optional fields.
      Assert.IsFalse(message.HasField(f("optional_int32"   )));
      Assert.IsFalse(message.HasField(f("optional_int64"   )));
      Assert.IsFalse(message.HasField(f("optional_uint32"  )));
      Assert.IsFalse(message.HasField(f("optional_uint64"  )));
      Assert.IsFalse(message.HasField(f("optional_sint32"  )));
      Assert.IsFalse(message.HasField(f("optional_sint64"  )));
      Assert.IsFalse(message.HasField(f("optional_fixed32" )));
      Assert.IsFalse(message.HasField(f("optional_fixed64" )));
      Assert.IsFalse(message.HasField(f("optional_sfixed32")));
      Assert.IsFalse(message.HasField(f("optional_sfixed64")));
      Assert.IsFalse(message.HasField(f("optional_float"   )));
      Assert.IsFalse(message.HasField(f("optional_double"  )));
      Assert.IsFalse(message.HasField(f("optional_bool"    )));
      Assert.IsFalse(message.HasField(f("optional_string"  )));
      Assert.IsFalse(message.HasField(f("optional_bytes"   )));

      Assert.IsFalse(message.HasField(f("optionalgroup"           )));
      Assert.IsFalse(message.HasField(f("optional_nested_message" )));
      Assert.IsFalse(message.HasField(f("optional_foreign_message")));
      Assert.IsFalse(message.HasField(f("optional_import_message" )));

      Assert.IsFalse(message.HasField(f("optional_nested_enum" )));
      Assert.IsFalse(message.HasField(f("optional_foreign_enum")));
      Assert.IsFalse(message.HasField(f("optional_import_enum" )));

      Assert.IsFalse(message.HasField(f("optional_string_piece")));
      Assert.IsFalse(message.HasField(f("optional_cord")));

      // Optional fields without defaults are set to zero or something like it.
      Assert.AreEqual(0    , message[f("optional_int32"   )]);
      Assert.AreEqual(0L   , message[f("optional_int64"   )]);
      Assert.AreEqual(0    , message[f("optional_uint32"  )]);
      Assert.AreEqual(0L   , message[f("optional_uint64"  )]);
      Assert.AreEqual(0    , message[f("optional_sint32"  )]);
      Assert.AreEqual(0L   , message[f("optional_sint64"  )]);
      Assert.AreEqual(0    , message[f("optional_fixed32" )]);
      Assert.AreEqual(0L   , message[f("optional_fixed64" )]);
      Assert.AreEqual(0    , message[f("optional_sfixed32")]);
      Assert.AreEqual(0L   , message[f("optional_sfixed64")]);
      Assert.AreEqual(0F   , message[f("optional_float"   )]);
      Assert.AreEqual(0D   , message[f("optional_double"  )]);
      Assert.AreEqual(false, message[f("optional_bool"    )]);
      Assert.AreEqual(""   , message[f("optional_string"  )]);
      Assert.AreEqual(ByteString.Empty, message[f("optional_bytes")]);

      // Embedded messages should also be clear.
      Assert.IsFalse(
        ((IMessage)message[f("optionalgroup")]).HasField(groupA));
      Assert.IsFalse(
        ((IMessage)message[f("optional_nested_message")])
                         .HasField(nestedB));
      Assert.IsFalse(
        ((IMessage)message[f("optional_foreign_message")])
                         .HasField(foreignC));
      Assert.IsFalse(
        ((IMessage)message[f("optional_import_message")])
                         .HasField(importD));

      Assert.AreEqual(0,((IMessage)message[f("optionalgroup")])[groupA]);
      Assert.AreEqual(0,((IMessage)message[f("optional_nested_message")])[nestedB]);
      Assert.AreEqual(0,((IMessage)message[f("optional_foreign_message")])[foreignC]);
      Assert.AreEqual(0,((IMessage)message[f("optional_import_message")])[importD]);

      // Enums without defaults are set to the first value in the enum.
      Assert.AreEqual( nestedFoo, message[f("optional_nested_enum" )]);
      Assert.AreEqual(foreignFoo, message[f("optional_foreign_enum")]);
      Assert.AreEqual( importFoo, message[f("optional_import_enum" )]);

      Assert.AreEqual("", message[f("optional_string_piece")]);
      Assert.AreEqual("", message[f("optional_cord")]);

      // Repeated fields are empty.
      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_int32"   )));
      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_int64"   )));
      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_uint32"  )));
      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_uint64"  )));
      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_sint32"  )));
      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_sint64"  )));
      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_fixed32" )));
      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_fixed64" )));
      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_sfixed32")));
      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_sfixed64")));
      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_float"   )));
      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_double"  )));
      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_bool"    )));
      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_string"  )));
      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_bytes"   )));

      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeatedgroup"           )));
      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_nested_message" )));
      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_foreign_message")));
      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_import_message" )));
      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_nested_enum"    )));
      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_foreign_enum"   )));
      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_import_enum"    )));

      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_string_piece")));
      Assert.AreEqual(0, message.GetRepeatedFieldCount(f("repeated_cord")));

      // has_blah() should also be false for all default fields.
      Assert.IsFalse(message.HasField(f("default_int32"   )));
      Assert.IsFalse(message.HasField(f("default_int64"   )));
      Assert.IsFalse(message.HasField(f("default_uint32"  )));
      Assert.IsFalse(message.HasField(f("default_uint64"  )));
      Assert.IsFalse(message.HasField(f("default_sint32"  )));
      Assert.IsFalse(message.HasField(f("default_sint64"  )));
      Assert.IsFalse(message.HasField(f("default_fixed32" )));
      Assert.IsFalse(message.HasField(f("default_fixed64" )));
      Assert.IsFalse(message.HasField(f("default_sfixed32")));
      Assert.IsFalse(message.HasField(f("default_sfixed64")));
      Assert.IsFalse(message.HasField(f("default_float"   )));
      Assert.IsFalse(message.HasField(f("default_double"  )));
      Assert.IsFalse(message.HasField(f("default_bool"    )));
      Assert.IsFalse(message.HasField(f("default_string"  )));
      Assert.IsFalse(message.HasField(f("default_bytes"   )));

      Assert.IsFalse(message.HasField(f("default_nested_enum" )));
      Assert.IsFalse(message.HasField(f("default_foreign_enum")));
      Assert.IsFalse(message.HasField(f("default_import_enum" )));

      Assert.IsFalse(message.HasField(f("default_string_piece" )));
      Assert.IsFalse(message.HasField(f("default_cord" )));

      // Fields with defaults have their default values (duh).
      Assert.AreEqual( 41    , message[f("default_int32"   )]);
      Assert.AreEqual( 42L   , message[f("default_int64"   )]);
      Assert.AreEqual( 43    , message[f("default_uint32"  )]);
      Assert.AreEqual( 44L   , message[f("default_uint64"  )]);
      Assert.AreEqual(-45    , message[f("default_sint32"  )]);
      Assert.AreEqual( 46L   , message[f("default_sint64"  )]);
      Assert.AreEqual( 47    , message[f("default_fixed32" )]);
      Assert.AreEqual( 48L   , message[f("default_fixed64" )]);
      Assert.AreEqual( 49    , message[f("default_sfixed32")]);
      Assert.AreEqual(-50L   , message[f("default_sfixed64")]);
      Assert.AreEqual( 51.5F , message[f("default_float"   )]);
      Assert.AreEqual( 52e3D , message[f("default_double"  )]);
      Assert.AreEqual(true   , message[f("default_bool"    )]);
      Assert.AreEqual("hello", message[f("default_string"  )]);
      Assert.AreEqual(TestUtil.ToBytes("world"), message[f("default_bytes")]);

      Assert.AreEqual( nestedBar, message[f("default_nested_enum" )]);
      Assert.AreEqual(foreignBar, message[f("default_foreign_enum")]);
      Assert.AreEqual( importBar, message[f("default_import_enum" )]);

      Assert.AreEqual("abc", message[f("default_string_piece")]);
      Assert.AreEqual("123", message[f("default_cord")]);
    }

    // ---------------------------------------------------------------

    internal void AssertRepeatedFieldsModifiedViaReflection(IMessage message) {
      // ModifyRepeatedFields only sets the second repeated element of each
      // field.  In addition to verifying this, we also verify that the first
      // element and size were *not* modified.
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_int32"   )));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_int64"   )));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_uint32"  )));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_uint64"  )));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_sint32"  )));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_sint64"  )));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_fixed32" )));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_fixed64" )));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_sfixed32")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_sfixed64")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_float"   )));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_double"  )));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_bool"    )));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_string"  )));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_bytes"   )));

      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeatedgroup"           )));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_nested_message" )));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_foreign_message")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_import_message" )));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_nested_enum"    )));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_foreign_enum"   )));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_import_enum"    )));

      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_string_piece")));
      Assert.AreEqual(2, message.GetRepeatedFieldCount(f("repeated_cord")));

      Assert.AreEqual(201  , message[f("repeated_int32"   ), 0]);
      Assert.AreEqual(202L , message[f("repeated_int64"   ), 0]);
      Assert.AreEqual(203  , message[f("repeated_uint32"  ), 0]);
      Assert.AreEqual(204L , message[f("repeated_uint64"  ), 0]);
      Assert.AreEqual(205  , message[f("repeated_sint32"  ), 0]);
      Assert.AreEqual(206L , message[f("repeated_sint64"  ), 0]);
      Assert.AreEqual(207  , message[f("repeated_fixed32" ), 0]);
      Assert.AreEqual(208L , message[f("repeated_fixed64" ), 0]);
      Assert.AreEqual(209  , message[f("repeated_sfixed32"), 0]);
      Assert.AreEqual(210L , message[f("repeated_sfixed64"), 0]);
      Assert.AreEqual(211F , message[f("repeated_float"   ), 0]);
      Assert.AreEqual(212D , message[f("repeated_double"  ), 0]);
      Assert.AreEqual(true , message[f("repeated_bool"    ), 0]);
      Assert.AreEqual("215", message[f("repeated_string"  ), 0]);
      Assert.AreEqual(TestUtil.ToBytes("216"), message[f("repeated_bytes"), 0]);

      Assert.AreEqual(217,((IMessage)message[f("repeatedgroup"), 0])[repeatedGroupA]);
      Assert.AreEqual(218,((IMessage)message[f("repeated_nested_message"), 0])[nestedB]);
      Assert.AreEqual(219,((IMessage)message[f("repeated_foreign_message"), 0])[foreignC]);
      Assert.AreEqual(220,((IMessage)message[f("repeated_import_message"), 0])[importD]);

      Assert.AreEqual( nestedBar, message[f("repeated_nested_enum" ),0]);
      Assert.AreEqual(foreignBar, message[f("repeated_foreign_enum"),0]);
      Assert.AreEqual( importBar, message[f("repeated_import_enum" ),0]);

      Assert.AreEqual("224", message[f("repeated_string_piece"), 0]);
      Assert.AreEqual("225", message[f("repeated_cord"), 0]);

      Assert.AreEqual(501  , message[f("repeated_int32"   ), 1]);
      Assert.AreEqual(502L , message[f("repeated_int64"   ), 1]);
      Assert.AreEqual(503  , message[f("repeated_uint32"  ), 1]);
      Assert.AreEqual(504L , message[f("repeated_uint64"  ), 1]);
      Assert.AreEqual(505  , message[f("repeated_sint32"  ), 1]);
      Assert.AreEqual(506L , message[f("repeated_sint64"  ), 1]);
      Assert.AreEqual(507  , message[f("repeated_fixed32" ), 1]);
      Assert.AreEqual(508L , message[f("repeated_fixed64" ), 1]);
      Assert.AreEqual(509  , message[f("repeated_sfixed32"), 1]);
      Assert.AreEqual(510L , message[f("repeated_sfixed64"), 1]);
      Assert.AreEqual(511F , message[f("repeated_float"   ), 1]);
      Assert.AreEqual(512D , message[f("repeated_double"  ), 1]);
      Assert.AreEqual(true , message[f("repeated_bool"    ), 1]);
      Assert.AreEqual("515", message[f("repeated_string"  ), 1]);
      Assert.AreEqual(TestUtil.ToBytes("516"), message[f("repeated_bytes"), 1]);

      Assert.AreEqual(517,((IMessage)message[f("repeatedgroup"), 1])[repeatedGroupA]);
      Assert.AreEqual(518,((IMessage)message[f("repeated_nested_message"), 1])[nestedB]);
      Assert.AreEqual(519,((IMessage)message[f("repeated_foreign_message"), 1])[foreignC]);
      Assert.AreEqual(520,((IMessage)message[f("repeated_import_message"), 1])[importD]);

      Assert.AreEqual( nestedFoo, message[f("repeated_nested_enum" ),1]);
      Assert.AreEqual(foreignFoo, message[f("repeated_foreign_enum"),1]);
      Assert.AreEqual( importFoo, message[f("repeated_import_enum" ),1]);

      Assert.AreEqual("524", message[f("repeated_string_piece"), 1]);
      Assert.AreEqual("525", message[f("repeated_cord"), 1]);
    }
  }
}
