using System;
using System.Collections.Generic;
using System.Text;
using Google.ProtocolBuffers.Collections;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers {
  /// <summary>
  /// All generated protocol message builder classes extend this class. It implements
  /// most of the IBuilder interface using reflection. Users can ignore this class
  /// as an implementation detail.
  /// </summary>
  public abstract class GeneratedBuilder<TMessage, TBuilder> : AbstractBuilder, IBuilder<TMessage>
      where TMessage : GeneratedMessage <TMessage, TBuilder>
      where TBuilder : GeneratedBuilder<TMessage, TBuilder>, IBuilder<TMessage> {

    /// <summary>
    /// Returns the message being built at the moment.
    /// </summary>
    protected abstract GeneratedMessage<TMessage,TBuilder> MessageBeingBuilt { get; }

    public override bool Initialized {
      get { return MessageBeingBuilt.IsInitialized; }
    }

    public override IDictionary<FieldDescriptor, object> AllFields {
      get { return MessageBeingBuilt.AllFields; }
    }

    public override object this[FieldDescriptor field] {
      get {
        // For repeated fields, the underlying list object is still modifiable at this point.
        // Make sure not to expose the modifiable list to the caller.
        return field.IsRepeated
          ? MessageBeingBuilt.InternalFieldAccessors[field].GetRepeatedWrapper(this)
          : MessageBeingBuilt[field];
      }
      set {
        MessageBeingBuilt.InternalFieldAccessors[field].SetValue(this, value);
      }
    }

    public override MessageDescriptor DescriptorForType {
      get { return MessageBeingBuilt.DescriptorForType; }
    }

    public override int GetRepeatedFieldCount(FieldDescriptor field) {
      return MessageBeingBuilt.GetRepeatedFieldCount(field);
    }

    public override object this[FieldDescriptor field, int index] {
      get { return MessageBeingBuilt[field, index]; }
      set { MessageBeingBuilt.InternalFieldAccessors[field].SetRepeated(this, index, value); }
    }

    public override bool HasField(FieldDescriptor field) {
      return MessageBeingBuilt.HasField(field);
    }

    protected override IMessage BuildImpl() {
      return Build();
    }

    protected override IMessage BuildPartialImpl() {
      return BuildPartial();
    }

    protected override IBuilder CloneImpl() {
      return Clone();
    }

    protected override IMessage DefaultInstanceForTypeImpl {
      get { return DefaultInstanceForType; }
    }

    protected override IBuilder NewBuilderForFieldImpl(FieldDescriptor field) {
      return NewBuilderForField(field);
    }

    protected override IBuilder ClearFieldImpl(FieldDescriptor field) {
      return ClearField(field);
    }

    protected override IBuilder AddRepeatedFieldImpl(FieldDescriptor field, object value) {
      return AddRepeatedField(field, value);
    }

    public new abstract IBuilder<TMessage> Clear();

    public IBuilder<TMessage> MergeFrom(IMessage<TMessage> other) {
      throw new NotImplementedException();
    }

    public abstract IMessage<TMessage> Build();

    public abstract IMessage<TMessage> BuildPartial();

    public abstract IBuilder<TMessage> Clone();

    IBuilder<TMessage> IBuilder<TMessage>.MergeFrom(CodedInputStream input) {
      throw new NotImplementedException();
    }

    IBuilder<TMessage> IBuilder<TMessage>.MergeFrom(CodedInputStream input, ExtensionRegistry extensionRegistry) {
      throw new NotImplementedException();
    }

    public IMessage<TMessage> DefaultInstanceForType {
      get { throw new NotImplementedException(); }
    }

    public IBuilder NewBuilderForField(FieldDescriptor field) {
      throw new NotImplementedException();
    }

    public IBuilder<TMessage> ClearField(FieldDescriptor field) {
      MessageBeingBuilt.InternalFieldAccessors[field].Clear(this);
      return this;
    }

    public IBuilder<TMessage> AddRepeatedField(FieldDescriptor field, object value) {
      return this;
    }

    public new IBuilder<TMessage> MergeUnknownFields(UnknownFieldSet unknownFields) {
      throw new NotImplementedException();
    }

    IBuilder<TMessage> IBuilder<TMessage>.MergeFrom(ByteString data) {
      throw new NotImplementedException();
    }

    IBuilder<TMessage> IBuilder<TMessage>.MergeFrom(ByteString data, ExtensionRegistry extensionRegistry) {
      throw new NotImplementedException();
    }

    IBuilder<TMessage> IBuilder<TMessage>.MergeFrom(byte[] data) {
      throw new NotImplementedException();
    }

    IBuilder<TMessage> IBuilder<TMessage>.MergeFrom(byte[] data, ExtensionRegistry extensionRegistry) {
      throw new NotImplementedException();
    }

    IBuilder<TMessage> IBuilder<TMessage>.MergeFrom(System.IO.Stream input) {
      throw new NotImplementedException();
    }

    IBuilder<TMessage> IBuilder<TMessage>.MergeFrom(System.IO.Stream input, ExtensionRegistry extensionRegistry) {
      throw new NotImplementedException();
    }
  }
}
