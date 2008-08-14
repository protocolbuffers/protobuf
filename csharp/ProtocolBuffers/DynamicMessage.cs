using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers {

  /// <summary>
  /// An implementation of IMessage that can represent arbitrary types, given a MessageaDescriptor.
  /// TODO: Implement appropriate generics.
  /// </summary>
  public class DynamicMessage : AbstractMessage<DynamicMessage, DynamicMessage.Builder> {

    private readonly MessageDescriptor type;
    private readonly FieldSet fields;
    private readonly UnknownFieldSet unknownFields;
    private int memoizedSize = -1;

    /// <summary>
    /// Creates a DynamicMessage with the given FieldSet.
    /// </summary>
    /// <param name="type"></param>
    /// <param name="fields"></param>
    /// <param name="unknownFields"></param>
    private DynamicMessage(MessageDescriptor type, FieldSet fields, UnknownFieldSet unknownFields) {
      this.type = type;
      this.fields = fields;
      this.unknownFields = unknownFields;
    }

    /// <summary>
    /// Returns a DynamicMessage representing the default instance of the given type.
    /// </summary>
    /// <param name="type"></param>
    /// <returns></returns>
    public static DynamicMessage GetDefaultInstance(MessageDescriptor type) {
      return new DynamicMessage(type, FieldSet.DefaultInstance, UnknownFieldSet.DefaultInstance);
    }

    /// <summary>
    /// Parses a message of the given type from the given stream.
    /// </summary>
    public static DynamicMessage ParseFrom(MessageDescriptor type, CodedInputStream input) {
      Builder builder = CreateBuilder(type);
      Builder dynamicBuilder = builder.MergeFrom(input);
      return dynamicBuilder.BuildParsed();

    }

    /// <summary>
    /// Parse a message of the given type from the given stream and extension registry.
    /// </summary>
    /// <param name="type"></param>
    /// <param name="input"></param>
    /// <param name="extensionRegistry"></param>
    /// <returns></returns>
    public static DynamicMessage ParseFrom(MessageDescriptor type, CodedInputStream input, ExtensionRegistry extensionRegistry) {
      Builder builder = CreateBuilder(type);
      Builder dynamicBuilder = builder.MergeFrom(input, extensionRegistry);
      return dynamicBuilder.BuildParsed();
    }

    /// <summary>
    /// Parses a message of the given type from the given stream.
    /// </summary>
    public static DynamicMessage ParseFrom(MessageDescriptor type, Stream input) {
      Builder builder = CreateBuilder(type);
      Builder dynamicBuilder = builder.MergeFrom(input);
      return dynamicBuilder.BuildParsed();
    }

    /// <summary>
    /// Parse a message of the given type from the given stream and extension registry.
    /// </summary>
    /// <param name="type"></param>
    /// <param name="input"></param>
    /// <param name="extensionRegistry"></param>
    /// <returns></returns>
    public static DynamicMessage ParseFrom(MessageDescriptor type, Stream input, ExtensionRegistry extensionRegistry) {
      Builder builder = CreateBuilder(type);
      Builder dynamicBuilder = builder.MergeFrom(input, extensionRegistry);
      return dynamicBuilder.BuildParsed();
    }

    /// <summary>
    /// Parse <paramref name="data"/> as a message of the given type and return it.
    /// </summary>
    public static DynamicMessage ParseFrom(MessageDescriptor type, ByteString data) {
      Builder builder = CreateBuilder(type);
      Builder dynamicBuilder = builder.MergeFrom(data);
      return dynamicBuilder.BuildParsed();
    }

    /// <summary>
    /// Parse <paramref name="data"/> as a message of the given type and return it.
    /// </summary>
    public static DynamicMessage ParseFrom(MessageDescriptor type, ByteString data, ExtensionRegistry extensionRegistry) {
      Builder builder = CreateBuilder(type);
      Builder dynamicBuilder = builder.MergeFrom(data, extensionRegistry);
      return dynamicBuilder.BuildParsed();

    }

    /// <summary>
    /// Parse <paramref name="data"/> as a message of the given type and return it.
    /// </summary>
    public static DynamicMessage ParseFrom(MessageDescriptor type, byte[] data) {
      Builder builder = CreateBuilder(type);
      Builder dynamicBuilder = builder.MergeFrom(data);
      return dynamicBuilder.BuildParsed();
    }

    /// <summary>
    /// Parse <paramref name="data"/> as a message of the given type and return it.
    /// </summary>
    public static DynamicMessage ParseFrom(MessageDescriptor type, byte[] data, ExtensionRegistry extensionRegistry) {
      Builder builder = CreateBuilder(type);
      Builder dynamicBuilder = builder.MergeFrom(data, extensionRegistry);
      return dynamicBuilder.BuildParsed();
    }

    /// <summary>
    /// Constructs a builder for the given type.
    /// </summary>
    public static Builder CreateBuilder(MessageDescriptor type) {
      return new Builder(type);
    }

    /// <summary>
    /// Constructs a builder for a message of the same type as <paramref name="prototype"/>,
    /// and initializes it with the same contents.
    /// </summary>
    /// <param name="prototype"></param>
    /// <returns></returns>
    public static Builder CreateBuilder(IMessage prototype) {
      return new Builder(prototype.DescriptorForType).MergeFrom(prototype);
    }

    // -----------------------------------------------------------------
    // Implementation of IMessage interface.

    public override MessageDescriptor DescriptorForType {
      get { return type; }
    }

    public override DynamicMessage DefaultInstanceForType {
      get { return GetDefaultInstance(type); }
    }

    public override IDictionary<FieldDescriptor, object> AllFields {
      get { return fields.AllFields; }
    }

    public override bool HasField(FieldDescriptor field) {
      VerifyContainingType(field);
      return fields.HasField(field);
    }

    public override object this[FieldDescriptor field] {
      get {
        VerifyContainingType(field);
        object result = fields[field];
        if (result == null) {
          result = GetDefaultInstance(field.MessageType);
        }
        return result;
      }
    }

    public override int GetRepeatedFieldCount(FieldDescriptor field) {
      VerifyContainingType(field);
      return fields.GetRepeatedFieldCount(field);
    }

    public override object this[FieldDescriptor field, int index] {
      get {
        VerifyContainingType(field);
        return fields[field, index];
      }
    }

    public override UnknownFieldSet UnknownFields {
      get { return unknownFields; }
    }

    public bool Initialized {
      get { return fields.IsInitializedWithRespectTo(type); }
    }

    public override void WriteTo(CodedOutputStream output) {
      fields.WriteTo(output);
      if (type.Options.MessageSetWireFormat) {
        unknownFields.WriteAsMessageSetTo(output);
      } else {
        unknownFields.WriteTo(output);
      }
    }

    public override int SerializedSize {
      get {
        int size = memoizedSize;
        if (size != -1) return size;

        size = fields.SerializedSize;
        if (type.Options.MessageSetWireFormat) {
          size += unknownFields.SerializedSizeAsMessageSet;
        } else {
          size += unknownFields.SerializedSize;
        }

        memoizedSize = size;
        return size;
      }
    }

    public override Builder CreateBuilderForType() {
      return new Builder(type);
    }

    /// <summary>
    /// Verifies that the field is a field of this message.
    /// </summary>
    private void VerifyContainingType(FieldDescriptor field) {
      if (field.ContainingType != type) {
        throw new ArgumentException("FieldDescriptor does not match message type.");
      }
    }

    public class Builder : AbstractBuilder<DynamicMessage, DynamicMessage.Builder> {
      private readonly MessageDescriptor type;
      private FieldSet fields;
      private UnknownFieldSet unknownFields;

      internal Builder(MessageDescriptor type) {
        this.type = type;
        this.fields = FieldSet.CreateFieldSet();
        this.unknownFields = UnknownFieldSet.DefaultInstance;
      }

      protected override Builder ThisBuilder {
        get { return this; }
      }

      public override Builder Clear() {
        fields.Clear();
        return this;
      }

      public override Builder MergeFrom(IMessage other) {
        if (other.DescriptorForType != type) {
          throw new ArgumentException("MergeFrom(IMessage) can only merge messages of the same type.");
        }
        fields.MergeFrom(other);
        return this;
      }

      public override Builder MergeFrom(DynamicMessage other) {
        if (other.DescriptorForType != type) {
          throw new ArgumentException("MergeFrom(IMessage) can only merge messages of the same type.");
        }
        fields.MergeFrom(other);
        return this;
      }

      public override DynamicMessage Build() {
   	    if (!IsInitialized) {
          throw new UninitializedMessageException(new DynamicMessage(type, fields, unknownFields));
        }
        return BuildPartial();
      }

      /// <summary>
      /// Helper for DynamicMessage.ParseFrom() methods to call. Throws
      /// InvalidProtocolBufferException 
      /// </summary>
      /// <returns></returns>
      internal DynamicMessage BuildParsed() {
        if (!IsInitialized) {
          throw new UninitializedMessageException(new DynamicMessage(type, fields, unknownFields)).AsInvalidProtocolBufferException();
        }
        return BuildPartial();
      }

      public override DynamicMessage BuildPartial() {
        fields.MakeImmutable();
        DynamicMessage result = new DynamicMessage(type, fields, unknownFields);
        fields = null;
        unknownFields = null;
        return result;
      }

      public override Builder Clone() {
        Builder result = new Builder(type);
        result.fields.MergeFrom(fields);
        return result;
      }

      public override bool IsInitialized {
      	get { return fields.IsInitializedWithRespectTo(type); }
      }

      public override Builder MergeFrom(CodedInputStream input, ExtensionRegistry extensionRegistry) {
        UnknownFieldSet.Builder unknownFieldsBuilder = UnknownFieldSet.CreateBuilder(unknownFields);
        FieldSet.MergeFrom(input, unknownFieldsBuilder, extensionRegistry, this);
        unknownFields = unknownFieldsBuilder.Build();
        return this;
      }

      public override MessageDescriptor DescriptorForType {
        get { return type; }
      }

      public override DynamicMessage DefaultInstanceForType {
        get { return GetDefaultInstance(type); }
      }

      public override IDictionary<FieldDescriptor, object> AllFields {
        get { return fields.AllFields; }
      }

      public override IBuilder CreateBuilderForField(FieldDescriptor field) {
        VerifyContainingType(field);
        if (field.MappedType != MappedType.Message) {
          throw new ArgumentException("CreateBuilderForField is only valid for fields with message type.");
        }
        return new Builder(field.MessageType);     
      }

      public override bool HasField(FieldDescriptor field) {
        VerifyContainingType(field);
        return fields.HasField(field);
      }

      public override object this[FieldDescriptor field, int index] {
        get {
          VerifyContainingType(field);
          return fields[field, index];
        }
        set {
          VerifyContainingType(field);
          fields[field, index] = value;
        }
      }

      public override object this[FieldDescriptor field] {
        get {
          VerifyContainingType(field);
          object result = fields[field];
          if (result == null) {
            result = GetDefaultInstance(field.MessageType);
          }
          return result;
        }
        set {
          VerifyContainingType(field);
          fields[field] = value;
        }
      }

      public override Builder ClearField(FieldDescriptor field) {
        VerifyContainingType(field);
        fields.ClearField(field);
        return this;
      }

      public override int GetRepeatedFieldCount(FieldDescriptor field) {
        VerifyContainingType(field);
        return fields.GetRepeatedFieldCount(field);
      }

      public override Builder AddRepeatedField(FieldDescriptor field, object value) {
        VerifyContainingType(field);
        fields.AddRepeatedField(field, value);
        return this;
      }

      public override UnknownFieldSet UnknownFields {
        get {
          return unknownFields;
        }
        set {
          unknownFields = value;
        }
      }

      /// <summary>
      /// Verifies that the field is a field of this message.
      /// </summary>
      /// <param name="field"></param>
      private void VerifyContainingType(FieldDescriptor field) {
        if (field.ContainingType != type) {
          throw new ArgumentException("FieldDescriptor does not match message type.");
        }
      }
    }
  }
}
