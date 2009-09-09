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
using System.Collections;
using System.Collections.Generic;
using System.IO;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers {
  /// <summary>
  /// Implementation of the non-generic IMessage interface as far as possible.
  /// </summary>
  public abstract class AbstractBuilder<TMessage, TBuilder> : IBuilder<TMessage, TBuilder> 
      where TMessage : AbstractMessage<TMessage, TBuilder>
      where TBuilder : AbstractBuilder<TMessage, TBuilder> {

    protected abstract TBuilder ThisBuilder { get; }
    
    #region Unimplemented members of IBuilder
    public abstract UnknownFieldSet UnknownFields { get; set; }
    public abstract TBuilder MergeFrom(TMessage other);
    public abstract bool IsInitialized { get; }
    public abstract IDictionary<FieldDescriptor, object> AllFields { get; }
    public abstract object this[FieldDescriptor field] { get; set; }
    public abstract MessageDescriptor DescriptorForType { get; }
    public abstract int GetRepeatedFieldCount(FieldDescriptor field);
    public abstract object this[FieldDescriptor field, int index] { get; set; }
    public abstract bool HasField(FieldDescriptor field);
    public abstract TMessage Build();
    public abstract TMessage BuildPartial();
    public abstract TBuilder Clone();
    public abstract TMessage DefaultInstanceForType { get; }
    public abstract IBuilder CreateBuilderForField(FieldDescriptor field);
    public abstract TBuilder ClearField(FieldDescriptor field);
    public abstract TBuilder AddRepeatedField(FieldDescriptor field, object value);
    #endregion

    #region Implementation of methods which don't require type parameter information
    public IMessage WeakBuild() {
      return Build();
    }

    public IBuilder WeakAddRepeatedField(FieldDescriptor field, object value) {
      return AddRepeatedField(field, value);
    }

    public IBuilder WeakClear() {
      return Clear();
    }

    public IBuilder WeakMergeFrom(IMessage message) {
      return MergeFrom(message);
    }

    public IBuilder WeakMergeFrom(CodedInputStream input) {
      return MergeFrom(input);
    }

    public IBuilder WeakMergeFrom(CodedInputStream input, ExtensionRegistry registry) {
      return MergeFrom(input, registry);
    }

    public IBuilder WeakMergeFrom(ByteString data) {
      return MergeFrom(data);
    }

    public IBuilder WeakMergeFrom(ByteString data, ExtensionRegistry registry) {
      return MergeFrom(data, registry);
    }

    public IMessage WeakBuildPartial() {
      return BuildPartial();
    }

    public IBuilder WeakClone() {
      return Clone();
    }

    public IMessage WeakDefaultInstanceForType {
      get { return DefaultInstanceForType; } 
    }

    public IBuilder WeakClearField(FieldDescriptor field) {
      return ClearField(field);
    }
    #endregion

    public TBuilder SetUnknownFields(UnknownFieldSet fields) {
      UnknownFields = fields;
      return ThisBuilder;
    }

    public virtual TBuilder Clear() {
      foreach(FieldDescriptor field in AllFields.Keys) {
        ClearField(field);
      }
      return ThisBuilder;
    }

    public virtual TBuilder MergeFrom(IMessage other) {
      if (other.DescriptorForType != DescriptorForType) {
        throw new ArgumentException("MergeFrom(IMessage) can only merge messages of the same type.");
      }

      // Note:  We don't attempt to verify that other's fields have valid
      //   types.  Doing so would be a losing battle.  We'd have to verify
      //   all sub-messages as well, and we'd have to make copies of all of
      //   them to insure that they don't change after verification (since
      //   the Message interface itself cannot enforce immutability of
      //   implementations).
      // TODO(jonskeet):  Provide a function somewhere called MakeDeepCopy()
      //   which allows people to make secure deep copies of messages.
      foreach (KeyValuePair<FieldDescriptor, object> entry in other.AllFields) {
        FieldDescriptor field = entry.Key;
        if (field.IsRepeated) {
          // Concatenate repeated fields
          foreach (object element in (IEnumerable) entry.Value) {
            AddRepeatedField(field, element);
          }
        } else if (field.MappedType == MappedType.Message) {
          // Merge singular messages
          IMessage existingValue = (IMessage) this[field];
          if (existingValue == existingValue.WeakDefaultInstanceForType) {
            this[field] = entry.Value;
          } else {
            this[field] = existingValue.WeakCreateBuilderForType()
                                       .WeakMergeFrom(existingValue)
                                       .WeakMergeFrom((IMessage) entry.Value)
                                       .WeakBuild();
          }
        } else {
          // Overwrite simple values
          this[field] = entry.Value;
        }
      }
      return ThisBuilder;
    }

    public virtual TBuilder MergeFrom(CodedInputStream input) {
      return MergeFrom(input, ExtensionRegistry.Empty);
    }

    public virtual TBuilder MergeFrom(CodedInputStream input, ExtensionRegistry extensionRegistry) {
      UnknownFieldSet.Builder unknownFields = UnknownFieldSet.CreateBuilder(UnknownFields);
      unknownFields.MergeFrom(input, extensionRegistry, this);
      UnknownFields = unknownFields.Build();
      return ThisBuilder;
    }

    public virtual TBuilder MergeUnknownFields(UnknownFieldSet unknownFields) {
      UnknownFields = UnknownFieldSet.CreateBuilder(UnknownFields)
          .MergeFrom(unknownFields)
          .Build();
      return ThisBuilder;
    }

    public virtual TBuilder MergeFrom(ByteString data) {
      CodedInputStream input = data.CreateCodedInput();
      MergeFrom(input);
      input.CheckLastTagWas(0);
      return ThisBuilder;
    }

    public virtual TBuilder MergeFrom(ByteString data, ExtensionRegistry extensionRegistry) {
      CodedInputStream input = data.CreateCodedInput();
      MergeFrom(input, extensionRegistry);
      input.CheckLastTagWas(0);
      return ThisBuilder;
    }

    public virtual TBuilder MergeFrom(byte[] data) {
      CodedInputStream input = CodedInputStream.CreateInstance(data);
      MergeFrom(input);
      input.CheckLastTagWas(0);
      return ThisBuilder;
    }

    public virtual TBuilder MergeFrom(byte[] data, ExtensionRegistry extensionRegistry) {
      CodedInputStream input = CodedInputStream.CreateInstance(data);
      MergeFrom(input, extensionRegistry);
      input.CheckLastTagWas(0);
      return ThisBuilder;
    }

    public virtual TBuilder MergeFrom(Stream input) {
      CodedInputStream codedInput = CodedInputStream.CreateInstance(input);
      MergeFrom(codedInput);
      codedInput.CheckLastTagWas(0);
      return ThisBuilder;
    }

    public virtual TBuilder MergeFrom(Stream input, ExtensionRegistry extensionRegistry) {
      CodedInputStream codedInput = CodedInputStream.CreateInstance(input);
      MergeFrom(codedInput, extensionRegistry);
      codedInput.CheckLastTagWas(0);
      return ThisBuilder;
    }

    public TBuilder MergeDelimitedFrom(Stream input, ExtensionRegistry extensionRegistry) {
      int size = (int) CodedInputStream.ReadRawVarint32(input);
      Stream limitedStream = new LimitedInputStream(input, size);
      return MergeFrom(limitedStream, extensionRegistry);
    }

    public TBuilder MergeDelimitedFrom(Stream input) {
      return MergeDelimitedFrom(input, ExtensionRegistry.Empty);
    }

    public virtual IBuilder SetField(FieldDescriptor field, object value) {
      this[field] = value;
      return ThisBuilder;
    }

    public virtual IBuilder SetRepeatedField(FieldDescriptor field, int index, object value) {
      this[field, index] = value;
      return ThisBuilder;
    }

    /// <summary>
    /// Stream implementation which proxies another stream, only allowing a certain amount
    /// of data to be read. Note that this is only used to read delimited streams, so it
    /// doesn't attempt to implement everything.
    /// </summary>
    private class LimitedInputStream : Stream {

      private readonly Stream proxied;
      private int bytesLeft;

      internal LimitedInputStream(Stream proxied, int size) {
        this.proxied = proxied;
        bytesLeft = size;
      }

      public override bool CanRead {
        get { return true; }
      }

      public override bool CanSeek {
        get { return false; }
      }

      public override bool CanWrite {
        get { return false; }
      }

      public override void Flush() {
      }

      public override long Length {
        get { throw new NotImplementedException(); }
      }

      public override long Position {
        get {
          throw new NotImplementedException();
        }
        set {
          throw new NotImplementedException();
        }
      }

      public override int Read(byte[] buffer, int offset, int count) {
        if (bytesLeft > 0) {
          int bytesRead = proxied.Read(buffer, offset, Math.Min(bytesLeft, count));
          bytesLeft -= bytesRead;
          return bytesRead;
        }
        return 0;
      }

      public override long Seek(long offset, SeekOrigin origin) {
        throw new NotImplementedException();
      }

      public override void SetLength(long value) {
        throw new NotImplementedException();
      }

      public override void Write(byte[] buffer, int offset, int count) {
        throw new NotImplementedException();
      }
    }
  }
}
