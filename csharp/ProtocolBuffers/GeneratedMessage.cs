using System;
using System.Collections.Generic;
using Google.ProtocolBuffers.Descriptors;
using Google.ProtocolBuffers.FieldAccess;

namespace Google.ProtocolBuffers {
  
  /// <summary>
  /// All generated protocol message classes extend this class. It implements
  /// most of the IMessage and IBuilder interfaces using reflection. Users
  /// can ignore this class as an implementation detail.
  /// </summary>
  public abstract class GeneratedMessage<TMessage, TBuilder> : AbstractMessage, IMessage<TMessage>
      where TMessage : GeneratedMessage<TMessage, TBuilder> where TBuilder : IBuilder<TMessage> {

    private readonly UnknownFieldSet unknownFields = UnknownFieldSet.DefaultInstance;

    protected abstract FieldAccessorTable<TMessage, TBuilder> InternalFieldAccessors { get; }

    public override MessageDescriptor DescriptorForType {
      get { return InternalFieldAccessors.Descriptor; }
    }

    protected override IMessage DefaultInstanceForTypeImpl {
      get { return DefaultInstanceForType; }
    }

    protected override IBuilder CreateBuilderForTypeImpl() {
      return CreateBuilderForType();
    }

    public IMessage<TMessage> DefaultInstanceForType {
      get { throw new NotImplementedException(); }
    }

    public IBuilder<TMessage> CreateBuilderForType() {
      throw new NotImplementedException();
    }

    private IDictionary<FieldDescriptor, Object> GetMutableFieldMap() {
      var ret = new Dictionary<FieldDescriptor, object>();
      MessageDescriptor descriptor = DescriptorForType;
      foreach (FieldDescriptor field in descriptor.Fields) {
        IFieldAccessor<TMessage, TBuilder> accessor = InternalFieldAccessors[field];
        if ((field.IsRepeated && accessor.GetRepeatedCount(this) != 0)
            || accessor.Has(this)) {
          ret[field] = accessor[this];
        }
      }
      return ret;
    }

    public override IDictionary<FieldDescriptor, object> AllFields {
      // FIXME: Make it immutable
      get { return GetMutableFieldMap(); }
    }

    public override bool HasField(FieldDescriptor field) {
      return InternalFieldAccessors[field].Has(this);
    }

    public override int GetRepeatedFieldCount(FieldDescriptor field) {
      return InternalFieldAccessors[field].GetRepeatedCount(this);
    }

    public override object this[FieldDescriptor field, int index] {
      get { return InternalFieldAccessors[field][this, index]; }
    }

    public override object this[FieldDescriptor field] {
      get { return InternalFieldAccessors[field][this]; }
    }

    public override UnknownFieldSet UnknownFields {
      get { return unknownFields; }
    }
  }
}
