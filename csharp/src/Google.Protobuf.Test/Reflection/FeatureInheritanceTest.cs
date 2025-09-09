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
using Pb;
using static Google.Protobuf.Reflection.FieldDescriptorProto.Types;
using Type = Google.Protobuf.Reflection.FieldDescriptorProto.Types.Type;

namespace Google.Protobuf.Test.Reflection;

public class FeatureInheritanceTest
{
    // Note: there's no test for file defaults, as we don't have the same access to modify the
    // global defaults in C# that exists in Java.

    [Test]
    public void FileOverrides()
    {
        var fileProto = CreateFileProto();
        SetTestFeature(fileProto, 3);
        var fileDescriptor = Build(fileProto);
        Assert.AreEqual(3, GetTestFeature(fileDescriptor.Features));
    }

    [Test]
    public void FileMessageInherit()
    {
        var fileProto = CreateFileProto();
        SetTestFeature(fileProto, 3);
        var fileDescriptor = Build(fileProto);
        Assert.AreEqual(3, GetTestFeature(fileDescriptor.MessageTypes[0].Features));
    }

    [Test]
    public void FileMessageOverride()
    {
        var fileProto = CreateFileProto();
        SetTestFeature(fileProto, 3);
        SetTestFeature(fileProto.MessageType[0], 5);
        var fileDescriptor = Build(fileProto);
        Assert.AreEqual(5, GetTestFeature(fileDescriptor.MessageTypes[0].Features));
    }

    [Test]
    public void FileEnumInherit()
    {
        var fileProto = CreateFileProto();
        SetTestFeature(fileProto, 3);
        var fileDescriptor = Build(fileProto);
        Assert.AreEqual(3, GetTestFeature(fileDescriptor.EnumTypes[0].Features));
    }

    [Test]
    public void FileEnumOverride()
    {
        var fileProto = CreateFileProto();
        SetTestFeature(fileProto, 3);
        SetTestFeature(fileProto.EnumType[0], 5);
        var fileDescriptor = Build(fileProto);
        Assert.AreEqual(5, GetTestFeature(fileDescriptor.EnumTypes[0].Features));
    }

    [Test]
    public void FileExtensionInherit()
    {
        var fileProto = CreateFileProto();
        SetTestFeature(fileProto, 3);
        var fileDescriptor = Build(fileProto);
        Assert.AreEqual(3, GetTestFeature(fileDescriptor.EnumTypes[0].Features));
    }

    [Test]
    public void FileExtensionOverride()
    {
        var fileProto = CreateFileProto();
        SetTestFeature(fileProto, 3);
        SetTestFeature(fileProto.Extension[0], 5);
        var fileDescriptor = Build(fileProto);
        Assert.AreEqual(5, GetTestFeature(fileDescriptor.Extensions.UnorderedExtensions[0].Features));
    }

    [Test]
    public void FileServiceInherit()
    {
        var fileProto = CreateFileProto();
        SetTestFeature(fileProto, 3);
        var fileDescriptor = Build(fileProto);
        Assert.AreEqual(3, GetTestFeature(fileDescriptor.Services[0].Features));
    }

    [Test]
    public void FileServiceOverride()
    {
        var fileProto = CreateFileProto();
        SetTestFeature(fileProto, 3);
        SetTestFeature(fileProto.Service[0], 5);
        var fileDescriptor = Build(fileProto);
        Assert.AreEqual(5, GetTestFeature(fileDescriptor.Services[0].Features));
    }

    [Test]
    public void MessageFieldInherit()
    {
        var fileProto = CreateFileProto();
        SetTestFeature(fileProto.MessageType[0], 3);
        var fileDescriptor = Build(fileProto);
        Assert.AreEqual(3, GetTestFeature(fileDescriptor.MessageTypes[0].Fields.InFieldNumberOrder()[0].Features));
    }

    [Test]
    public void MessageFieldOverride()
    {
        var fileProto = CreateFileProto();
        SetTestFeature(fileProto.MessageType[0], 3);
        SetTestFeature(fileProto.MessageType[0].Field[0], 5);
        var fileDescriptor = Build(fileProto);
        Assert.AreEqual(5, GetTestFeature(fileDescriptor.MessageTypes[0].Fields.InFieldNumberOrder()[0].Features));
    }

    [Test]
    public void MessageEnumInherit()
    {
        var fileProto = CreateFileProto();
        SetTestFeature(fileProto.MessageType[0], 3);
        var fileDescriptor = Build(fileProto);
        Assert.AreEqual(3, GetTestFeature(fileDescriptor.MessageTypes[0].EnumTypes[0].Features));
    }

    [Test]
    public void MessageEnumOverride()
    {
        var fileProto = CreateFileProto();
        SetTestFeature(fileProto.MessageType[0], 3);
        SetTestFeature(fileProto.MessageType[0].EnumType[0], 5);
        var fileDescriptor = Build(fileProto);
        Assert.AreEqual(5, GetTestFeature(fileDescriptor.MessageTypes[0].EnumTypes[0].Features));
    }

    [Test]
    public void MessageMessageInherit()
    {
        var fileProto = CreateFileProto();
        SetTestFeature(fileProto.MessageType[0], 3);
        var fileDescriptor = Build(fileProto);
        Assert.AreEqual(3, GetTestFeature(fileDescriptor.MessageTypes[0].NestedTypes[0].Features));
    }

    [Test]
    public void MessageMessageOverride()
    {
        var fileProto = CreateFileProto();
        SetTestFeature(fileProto.MessageType[0], 3);
        SetTestFeature(fileProto.MessageType[0].NestedType[0], 5);
        var fileDescriptor = Build(fileProto);
        Assert.AreEqual(5, GetTestFeature(fileDescriptor.MessageTypes[0].NestedTypes[0].Features));
    }

    [Test]
    public void MessageExtensionInherit()
    {
        var fileProto = CreateFileProto();
        SetTestFeature(fileProto.MessageType[0], 3);
        var fileDescriptor = Build(fileProto);
        Assert.AreEqual(3, GetTestFeature(fileDescriptor.MessageTypes[0].Extensions.UnorderedExtensions[0].Features));
    }

    [Test]
    public void MessageExtensionOverride()
    {
        var fileProto = CreateFileProto();
        SetTestFeature(fileProto.MessageType[0], 3);
        SetTestFeature(fileProto.MessageType[0].Extension[0], 5);
        var fileDescriptor = Build(fileProto);
        Assert.AreEqual(5, GetTestFeature(fileDescriptor.MessageTypes[0].Extensions.UnorderedExtensions[0].Features));
    }

    [Test]
    public void MessageOneofInherit()
    {
        var fileProto = CreateFileProto();
        SetTestFeature(fileProto.MessageType[0], 3);
        var fileDescriptor = Build(fileProto);
        Assert.AreEqual(3, GetTestFeature(fileDescriptor.MessageTypes[0].Oneofs[0].Features));
    }

    [Test]
    public void MessageOneofOverride()
    {
        var fileProto = CreateFileProto();
        SetTestFeature(fileProto.MessageType[0], 3);
        SetTestFeature(fileProto.MessageType[0].OneofDecl[0], 5);
        var fileDescriptor = Build(fileProto);
        Assert.AreEqual(5, GetTestFeature(fileDescriptor.MessageTypes[0].Oneofs[0].Fields[0].Features));
    }

    [Test]
    public void OneofFieldInherit()
    {
        var fileProto = CreateFileProto();
        SetTestFeature(fileProto.MessageType[0], 3);
        var fileDescriptor = Build(fileProto);
        Assert.AreEqual(3, GetTestFeature(fileDescriptor.MessageTypes[0].Oneofs[0].Fields[0].Features));
    }

    [Test]
    public void OneofFieldOverride()
    {
        var fileProto = CreateFileProto();
        SetTestFeature(fileProto.MessageType[0], 3);
        SetTestFeature(fileProto.MessageType[0].OneofDecl[0], 5);
        var fileDescriptor = Build(fileProto);
        Assert.AreEqual(5, GetTestFeature(fileDescriptor.MessageTypes[0].Oneofs[0].Features));
    }

    [Test]
    public void EnumValueInherit()
    {
        var fileProto = CreateFileProto();
        SetTestFeature(fileProto.EnumType[0], 3);
        var fileDescriptor = Build(fileProto);
        Assert.AreEqual(3, GetTestFeature(fileDescriptor.EnumTypes[0].Values[0].Features));
    }

    [Test]
    public void EnumValueOverride()
    {
        var fileProto = CreateFileProto();
        SetTestFeature(fileProto.MessageType[0], 3);
        SetTestFeature(fileProto.EnumType[0].Value[0], 5);
        var fileDescriptor = Build(fileProto);
        Assert.AreEqual(5, GetTestFeature(fileDescriptor.EnumTypes[0].Values[0].Features));
    }

    [Test]
    public void ServiceMethodInherit()
    {
        var fileProto = CreateFileProto();
        SetTestFeature(fileProto.Service[0], 3);
        var fileDescriptor = Build(fileProto);
        Assert.AreEqual(3, GetTestFeature(fileDescriptor.Services[0].Methods[0].Features));
    }

    [Test]
    public void ServiceMethodOverride()
    {
        var fileProto = CreateFileProto();
        SetTestFeature(fileProto.Service[0], 3);
        SetTestFeature(fileProto.Service[0].Method[0], 5);
        var fileDescriptor = Build(fileProto);
        Assert.AreEqual(5, GetTestFeature(fileDescriptor.Services[0].Methods[0].Features));
    }

    private static int GetTestFeature(FeatureSetDescriptor features) =>
        (int)(features.Proto.GetExtension(UnittestFeaturesExtensions.Test) ?? new TestFeatures())
            .MultipleFeature;

    private static void SetTestFeature(FileDescriptorProto proto, int value)
    {
        proto.Options ??= new FileOptions();
        proto.Options.Features ??= new FeatureSet();
        SetTestFeature(proto.Options.Features, value);
    }

    private static void SetTestFeature(DescriptorProto proto, int value)
    {
        proto.Options ??= new MessageOptions();
        proto.Options.Features ??= new FeatureSet();
        SetTestFeature(proto.Options.Features, value);
    }

    private static void SetTestFeature(EnumDescriptorProto proto, int value)
    {
        proto.Options ??= new EnumOptions();
        proto.Options.Features ??= new FeatureSet();
        SetTestFeature(proto.Options.Features, value);
    }

    private static void SetTestFeature(EnumValueDescriptorProto proto, int value)
    {
        proto.Options ??= new EnumValueOptions();
        proto.Options.Features ??= new FeatureSet();
        SetTestFeature(proto.Options.Features, value);
    }

    private static void SetTestFeature(FieldDescriptorProto proto, int value)
    {
        proto.Options ??= new FieldOptions();
        proto.Options.Features ??= new FeatureSet();
        SetTestFeature(proto.Options.Features, value);
    }

    private static void SetTestFeature(ServiceDescriptorProto proto, int value)
    {
        proto.Options ??= new ServiceOptions();
        proto.Options.Features ??= new FeatureSet();
        SetTestFeature(proto.Options.Features, value);
    }

    private static void SetTestFeature(OneofDescriptorProto proto, int value)
    {
        proto.Options ??= new OneofOptions();
        proto.Options.Features ??= new FeatureSet();
        SetTestFeature(proto.Options.Features, value);
    }

    private static void SetTestFeature(MethodDescriptorProto proto, int value)
    {
        proto.Options ??= new MethodOptions();
        proto.Options.Features ??= new FeatureSet();
        SetTestFeature(proto.Options.Features, value);
    }

    private static void SetTestFeature(FeatureSet features, int value) =>
        features.SetExtension(UnittestFeaturesExtensions.Test,
                              new TestFeatures { MultipleFeature = (Pb.EnumFeature)value });

    private static FileDescriptor Build(FileDescriptorProto fileProto) =>
        FileDescriptor.BuildFromByteStrings(new[] { fileProto.ToByteString() }, new ExtensionRegistry { UnittestFeaturesExtensions.Test })[0];

    private static FileDescriptorProto CreateFileProto() => new FileDescriptorProto
    {
        Name = "some/filename/some.proto",
        Package = "proto2_unittest",
        Edition = Edition._2023,
        Syntax = "editions",
        Extension =
        {
            new FieldDescriptorProto { Name = "top_extension", Number = 10, Type = Type.Int32, Label = Label.Optional, Extendee = ".proto2_unittest.TopMessage" }
        },
        EnumType =
        {
            new EnumDescriptorProto { Name = "TopEnum", Value = { new EnumValueDescriptorProto { Name = "TOP_VALUE", Number = 0 } } }
        },
        MessageType =
        {
            new DescriptorProto
            {
                Name = "TopMessage",
                Field =
                {
                    new FieldDescriptorProto { Name = "field", Number = 1, Type = Type.Int32, Label = Label.Optional },
                    new FieldDescriptorProto { Name = "oneof_field", Number = 2, Type = Type.Int32, Label = Label.Optional, OneofIndex = 0 }
                },
                Extension =
                {
                    new FieldDescriptorProto { Name = "nested_extension", Number = 11, Type = Type.Int32, Label = Label.Optional, Extendee = ".proto2_unittest.TopMessage" }
                },
                NestedType =
                {
                    new DescriptorProto { Name = "NestedMessage" },
                },
                EnumType =
                {
                    new EnumDescriptorProto { Name = "NestedEnum", Value = { new EnumValueDescriptorProto { Name = "NESTED_VALUE", Number = 0 } } }
                },
                OneofDecl = { new OneofDescriptorProto { Name = "Oneof" } }
            }
        },
        Service =
        {
            new ServiceDescriptorProto
            {
                Name = "TestService",
                Method = { new MethodDescriptorProto { Name = "CallMethod", InputType = ".proto2_unittest.TopMessage", OutputType = ".proto2_unittest.TopMessage" } }
            }
        }
    };
}
