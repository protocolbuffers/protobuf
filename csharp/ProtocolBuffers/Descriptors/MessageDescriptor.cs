using System.Collections.Generic;
using Google.ProtocolBuffers.DescriptorProtos;

namespace Google.ProtocolBuffers.Descriptors {

  /// <summary>
  /// Describes a message type.
  /// </summary>
  public class MessageDescriptor : IndexedDescriptorBase<DescriptorProto, MessageOptions> {

    private readonly MessageDescriptor containingType;
    private readonly IList<MessageDescriptor> nestedTypes;
    private readonly IList<EnumDescriptor> enumTypes;
    private readonly IList<FieldDescriptor> fields;
    private readonly IList<FieldDescriptor> extensions;

    internal MessageDescriptor(DescriptorProto proto, FileDescriptor file, MessageDescriptor parent, int typeIndex)
        : base(proto, file, ComputeFullName(file, parent, proto.Name), typeIndex) {
      containingType = parent;

      nestedTypes = DescriptorUtil.ConvertAndMakeReadOnly(proto.NestedTypeList,
          (type, index) => new MessageDescriptor(type, file, null, index));

      enumTypes = DescriptorUtil.ConvertAndMakeReadOnly(proto.EnumTypeList,
          (type, index) => new EnumDescriptor(type, file, null, index));

      fields = DescriptorUtil.ConvertAndMakeReadOnly(proto.FieldList,
          (field, index) => new FieldDescriptor(field, file, null, index, false));

      extensions = DescriptorUtil.ConvertAndMakeReadOnly(proto.ExtensionList,
          (field, index) => new FieldDescriptor(field, file, null, index, true));

      file.DescriptorPool.AddSymbol(this);
    }

    /// <value>
    /// An unmodifiable list of this message type's fields.
    /// </value>
    public IList<FieldDescriptor> Fields {
      get { return fields; }
    }

    /// <value>
    /// An unmodifiable list of this message type's extensions.
    /// </value>
    public IList<FieldDescriptor> Extensions {
      get { return extensions; }
    }

    /// <value>
    /// An unmodifiable list of this message type's nested types.
    /// </value>
    public IList<MessageDescriptor> NestedTypes {
      get { return nestedTypes; }
    }

    /// <value>
    /// An unmodifiable list of this message type's enum types.
    /// </value>
    public IList<EnumDescriptor> EnumTypes {
      get { return enumTypes; }
    }

    /// <summary>
    /// Determines if the given field number is an extension.
    /// </summary>
    public bool IsExtensionNumber(int number) {
      foreach (DescriptorProto.Types.ExtensionRange range in Proto.ExtensionRangeList) {
        if (range.Start <= number && number < range.End) {
          return true;
        }
      }
      return false;
    }

    /// <summary>
    /// Finds a field by field number.
    /// </summary>
    /// <param name="number">The field number within this message type.</param>
    /// <returns>The field's descriptor, or null if not found.</returns>
    public FieldDescriptor FindFieldByNumber(int number) {
      return File.DescriptorPool.FindFieldByNumber(this, number);
    }

    /// <summary>
    /// Finds a nested descriptor by name. The is valid for fields, nested
    /// message types and enums.
    /// </summary>
    /// <param name="name">The unqualified name of the descriptor, e.g. "Foo"</param>
    /// <returns>The descriptor, or null if not found.</returns>
    public T FindDescriptor<T>(string name) 
        where T : class, IDescriptor {
      return File.DescriptorPool.FindSymbol<T>(FullName + "." + name);
    }

    /// <summary>
    /// Looks up and cross-links all fields, nested types, and extensions.
    /// </summary>
    internal void CrossLink() {
      foreach (MessageDescriptor message in nestedTypes) {
        message.CrossLink();
      }

      foreach (FieldDescriptor field in fields) {
        field.CrossLink();
      }

      foreach (FieldDescriptor extension in extensions) {
        extension.CrossLink();
      }
    }
  }
}
