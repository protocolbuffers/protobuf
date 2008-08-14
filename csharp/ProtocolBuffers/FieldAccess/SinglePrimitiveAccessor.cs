// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.
// http://code.google.com/p/protobuf/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
using System;
using System.Reflection;

namespace Google.ProtocolBuffers.FieldAccess {
  /// <summary>
  /// Access for a non-repeated field of a "primitive" type (i.e. not another message or an enum).
  /// </summary>
  internal class SinglePrimitiveAccessor<TMessage, TBuilder> : IFieldAccessor<TMessage, TBuilder>
      where TMessage : IMessage<TMessage, TBuilder>
      where TBuilder : IBuilder<TMessage, TBuilder> {

    private readonly PropertyInfo messageProperty;
    private readonly PropertyInfo builderProperty;
    private readonly HasDelegate<TMessage> hasDelegate;
    private readonly ClearDelegate<TBuilder> clearDelegate;

    /// <summary>
    /// The CLR type of the field (int, the enum type, ByteString, the message etc).
    /// As declared by the property.
    /// </summary>
    protected Type ClrType {
      get { return messageProperty.PropertyType; }
    }

    internal SinglePrimitiveAccessor(string name) {
      messageProperty = typeof(TMessage).GetProperty(name);
      builderProperty = typeof(TBuilder).GetProperty(name);
      PropertyInfo hasProperty = typeof(TMessage).GetProperty("Has" + name);
      MethodInfo clearMethod = typeof(TBuilder).GetMethod("Clear" + name);
      if (messageProperty == null || builderProperty == null || hasProperty == null || clearMethod == null) {
        throw new ArgumentException("Not all required properties/methods available");
      }
      hasDelegate = (HasDelegate<TMessage>)Delegate.CreateDelegate(typeof(HasDelegate<TMessage>), hasProperty.GetGetMethod());
      clearDelegate = (ClearDelegate<TBuilder>)Delegate.CreateDelegate(typeof(ClearDelegate<TBuilder>), clearMethod);
    }

    public bool Has(TMessage message) {
      return hasDelegate(message);
    }

    public void Clear(TBuilder builder) {
      clearDelegate(builder);
    }

    /// <summary>
    /// Only valid for message types - this implementation throws InvalidOperationException.
    /// </summary>
    public virtual IBuilder CreateBuilder() {
      throw new InvalidOperationException();
    }

    public virtual object GetValue(TMessage message) {
      return messageProperty.GetValue(message, null);
    }

    public virtual void SetValue(TBuilder builder, object value) {
      builderProperty.SetValue(builder, value, null);
    }

    #region Methods only related to repeated values
    public int GetRepeatedCount(TMessage message) {
      throw new InvalidOperationException();
    }

    public object GetRepeatedValue(IMessage message, int index) {
      throw new InvalidOperationException();
    }

    public void SetRepeated(IBuilder builder, int index, object value) {
      throw new InvalidOperationException();
    }

    public void AddRepeated(IBuilder builder, object value) {
      throw new InvalidOperationException();
    }

    public object GetRepeatedWrapper(IBuilder builder) {
      throw new InvalidOperationException();
    }
    #endregion
  }
}