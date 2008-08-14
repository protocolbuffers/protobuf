using System;
using System.Collections.Generic;
using Google.ProtocolBuffers.Collections;
using Google.ProtocolBuffers.Descriptors;
using Google.ProtocolBuffers.FieldAccess;

namespace Google.ProtocolBuffers {
  
  /// <summary>
  /// All generated protocol message classes extend this class. It implements
  /// most of the IMessage interface using reflection. Users
  /// can ignore this class as an implementation detail.
  /// </summary>
  public abstract class GeneratedMessage<TMessage, TBuilder> : AbstractMessage, IMessage<TMessage>
      where TMessage : GeneratedMessage<TMessage, TBuilder> where TBuilder : IBuilder<TMessage> {

    private readonly UnknownFieldSet unknownFields = UnknownFieldSet.DefaultInstance;

    protected internal abstract FieldAccessorTable<TMessage, TBuilder> InternalFieldAccessors { get; }

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

      // Use a SortedList so we'll end up serializing fields in order
      var ret = new SortedList<FieldDescriptor, object>();
      MessageDescriptor descriptor = DescriptorForType;
      foreach (FieldDescriptor field in descriptor.Fields) {
        IFieldAccessor<TMessage, TBuilder> accessor = InternalFieldAccessors[field];
        if ((field.IsRepeated && accessor.GetRepeatedCount(this) != 0)
            || accessor.Has(this)) {
          ret[field] = accessor.GetValue(this);
        }
      }
      return ret;
    }

    public override IDictionary<FieldDescriptor, object> AllFields {
      get { return Dictionaries.AsReadOnly(GetMutableFieldMap()); }
    }

    public override bool HasField(FieldDescriptor field) {
      return InternalFieldAccessors[field].Has(this);
    }

    public override int GetRepeatedFieldCount(FieldDescriptor field) {
      return InternalFieldAccessors[field].GetRepeatedCount(this);
    }

    public override object this[FieldDescriptor field, int index] {
      get { return InternalFieldAccessors[field].GetRepeatedValue(this, index); }
    }

    public override object this[FieldDescriptor field] {
      get { return InternalFieldAccessors[field].GetValue(this); }
    }

    public override UnknownFieldSet UnknownFields {
      get { return unknownFields; }
    }
  }
}
