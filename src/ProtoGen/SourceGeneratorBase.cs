using System;
using System.Collections.Generic;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers.ProtoGen {
  internal abstract class SourceGeneratorBase<T> where T : IDescriptor {

    private readonly T descriptor;

    protected SourceGeneratorBase(T descriptor) {
      this.descriptor = descriptor;
    }

    protected T Descriptor {
      get { return descriptor; }
    }

    internal static string GetClassName(IDescriptor descriptor) {
      return ToCSharpName(descriptor.FullName, descriptor.File);
    }

    // Groups are hacky:  The name of the field is just the lower-cased name
    // of the group type.  In C#, though, we would like to retain the original
    // capitalization of the type name.
    internal static string GetFieldName(FieldDescriptor descriptor) {
      if (descriptor.FieldType == FieldType.Group) {
        return descriptor.MessageType.Name;
      } else {
        return descriptor.Name;
      }
    }

    internal static string GetFieldConstantName(FieldDescriptor field) {
      return field.CSharpOptions.PropertyName + "FieldNumber";
    }

    private static string ToCSharpName(string name, FileDescriptor file) {
      string result = file.CSharpOptions.Namespace;
      if (file.CSharpOptions.NestClasses) {
        if (result != "") {
          result += ".";
        }
        result += file.CSharpOptions.UmbrellaClassname;
      }
      if (result != "") {
        result += '.';
      }
      string classname;
      if (file.Package == "") {
        classname = name;
      } else {
        // Strip the proto package from full_name since we've replaced it with
        // the C# namespace.
        classname = name.Substring(file.Package.Length + 1);
      }
      result += classname.Replace(".", ".Types.");
      return "global::" + result;
    }

    protected string ClassAccessLevel {
      get { 
        return descriptor.File.CSharpOptions.PublicClasses ? "public" : "internal";
      }
    }

    protected void WriteChildren<TChild>(TextGenerator writer, string region, IEnumerable<TChild> children) 
        where TChild : IDescriptor {
      // Copy the set of children; makes access easier
      List<TChild> copy = new List<TChild>(children);
      if (copy.Count == 0) {
        return;
      }

      if (region != null) {
        writer.WriteLine("#region {0}", region);
      }
      foreach (TChild child in children) {
        SourceGenerators.CreateGenerator(child).Generate(writer);
      }
      if (region != null) {
        writer.WriteLine("#endregion");
        writer.WriteLine();
      }
    }
  }
}
