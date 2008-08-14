using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using Google.ProtocolBuffers.Collections;
using Google.ProtocolBuffers.Descriptors;
using System.IO;
using Google.ProtocolBuffers.FieldAccess;

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
    protected abstract TMessage MessageBeingBuilt { get; }

    protected internal FieldAccessorTable InternalFieldAccessors {
      get { return MessageBeingBuilt.FieldAccesseorsFromBuilder; }
    }

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
          ? InternalFieldAccessors[field].GetRepeatedWrapper(this)
          : MessageBeingBuilt[field];
      }
      set {
        InternalFieldAccessors[field].SetValue(this, value);
      }
    }

    /// <summary>
    /// Adds all of the specified values to the given collection.
    /// </summary>
    protected void AddRange<T>(IEnumerable<T> source, IList<T> destination) {
      List<T> list = destination as List<T>;
      if (list != null) {
        list.AddRange(source);
      } else {
        foreach (T element in source) {
          destination.Add(element);
        }
      }
    }

    /// <summary>
    /// Called by derived classes to parse an unknown field.
    /// </summary>
    /// <returns>true unless the tag is an end-group tag</returns>
    protected bool ParseUnknownField(CodedInputStream input, UnknownFieldSet.Builder unknownFields,
                                     ExtensionRegistry extensionRegistry, uint tag) {
      return unknownFields.MergeFieldFrom(tag, input);
    }

    public override MessageDescriptor DescriptorForType {
      get { return MessageBeingBuilt.DescriptorForType; }
    }

    public override int GetRepeatedFieldCount(FieldDescriptor field) {
      return MessageBeingBuilt.GetRepeatedFieldCount(field);
    }

    public override object this[FieldDescriptor field, int index] {
      get { return MessageBeingBuilt[field, index]; }
      set { InternalFieldAccessors[field].SetRepeated(this, index, value); }
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

    public override IBuilder CreateBuilderForField(FieldDescriptor field) {
      return InternalFieldAccessors[field].CreateBuilder();
    }

    protected override IBuilder ClearFieldImpl(FieldDescriptor field) {
      return ClearField(field);
    }

    protected override IBuilder AddRepeatedFieldImpl(FieldDescriptor field, object value) {
      return AddRepeatedField(field, value);
    }

    public IBuilder<TMessage> ClearField(FieldDescriptor field) {
      InternalFieldAccessors[field].Clear(this);
      return this;
    }

    public virtual IBuilder<TMessage> MergeFrom(TMessage other) {
      if (other.DescriptorForType != InternalFieldAccessors.Descriptor) {
        throw new ArgumentException("Message type mismatch");
      }

      foreach (KeyValuePair<FieldDescriptor, object> entry in AllFields) {
        FieldDescriptor field = entry.Key;
        if (field.IsRepeated) {
          // Concatenate repeated fields
          foreach (object element in (IEnumerable)entry.Value) {
            AddRepeatedField(field, element);
          }
        } else if (field.MappedType == MappedType.Message && HasField(field)) {
          // Merge singular embedded messages
          IMessage oldValue = (IMessage)this[field];
          this[field] = oldValue.CreateBuilderForType()
              .MergeFrom(oldValue)
              .MergeFrom((IMessage)entry.Value)
              .BuildPartial();
        } else {
          // Just overwrite
          this[field] = entry.Value;
        }
      }
      return this;
    }

    public virtual IBuilder<TMessage> MergeUnknownFields(UnknownFieldSet unknownFields) {
      TMessage result = MessageBeingBuilt;
      result.SetUnknownFields(UnknownFieldSet.CreateBuilder(result.UnknownFields)
          .MergeFrom(unknownFields)
          .Build());
      return this;
    }

    public IBuilder<TMessage> AddRepeatedField(FieldDescriptor field, object value) {
      InternalFieldAccessors[field].AddRepeated(this, value);
      return this;
    }

    public IBuilder<TMessage> MergeFrom(ByteString data) {
      ((IBuilder) this).MergeFrom(data);
      return this;
    }

    public IBuilder<TMessage> MergeFrom(ByteString data, ExtensionRegistry extensionRegistry) {
      ((IBuilder) this).MergeFrom(data, extensionRegistry);
      return this;
    }

    public IBuilder<TMessage> MergeFrom(byte[] data) {
      ((IBuilder) this).MergeFrom(data);
      return this;
    }

    public IBuilder<TMessage> MergeFrom(byte[] data, ExtensionRegistry extensionRegistry) {
      ((IBuilder) this).MergeFrom(data, extensionRegistry);
      return this;
    }

    public IBuilder<TMessage> MergeFrom(Stream input) {
      ((IBuilder) this).MergeFrom(input);
      return this;
    }

    public IBuilder<TMessage> MergeFrom(Stream input, ExtensionRegistry extensionRegistry) {
      ((IBuilder) this).MergeFrom(input, extensionRegistry);
      return this;
    }

    public virtual IBuilder<TMessage> MergeFrom(CodedInputStream input) {
      ((IBuilder)this).MergeFrom(input);
      return this;
    }

    public virtual IBuilder<TMessage> MergeFrom(CodedInputStream input, ExtensionRegistry extensionRegistry) {
      ((IBuilder)this).MergeFrom(input, extensionRegistry);
      return this;
    }
    
    /// <summary>
    /// Like Build(), but will wrap UninitializedMessageException in
    /// InvalidProtocolBufferException.
    /// TODO(jonskeet): This used to be generated for each class. Find out why.
    /// </summary>
    public TMessage BuildParsed() {
      if (!Initialized) {
        throw new UninitializedMessageException(MessageBeingBuilt).AsInvalidProtocolBufferException();
      }
      return BuildPartial();
    }

    /// <summary>
    /// Implementation of <see cref="IBuilder{T}.Build" />.
    /// TODO(jonskeet): This used to be generated for each class. Find out why.
    /// </summary>
    public TMessage Build() {
      if (!Initialized) {
        throw new UninitializedMessageException(MessageBeingBuilt);
      }
      return BuildPartial();
    }
    
    public abstract TMessage BuildPartial();
    public abstract IBuilder<TMessage> Clone();
    public abstract new IBuilder<TMessage> Clear();
    public abstract TMessage DefaultInstanceForType { get; }
    
    public abstract class ExtendableBuilder : GeneratedBuilder<TMessage, TBuilder> {
    }
  }
}
