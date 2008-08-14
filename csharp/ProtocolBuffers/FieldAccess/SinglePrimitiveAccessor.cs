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

    private readonly Type clrType;
    private readonly Func<TMessage, object> getValueDelegate;
    private readonly Action<TBuilder, object> setValueDelegate;
    private readonly Func<TMessage, bool> hasDelegate;
    private readonly Func<TBuilder, IBuilder> clearDelegate;
    delegate void SingleValueDelegate<TSource>(TSource source, object value);

    /// <summary>
    /// The CLR type of the field (int, the enum type, ByteString, the message etc).
    /// As declared by the property.
    /// </summary>
    protected Type ClrType {
      get { return clrType; }
    }

    internal SinglePrimitiveAccessor(string name) {
      PropertyInfo messageProperty = typeof(TMessage).GetProperty(name);
      PropertyInfo builderProperty = typeof(TBuilder).GetProperty(name);
      PropertyInfo hasProperty = typeof(TMessage).GetProperty("Has" + name);
      MethodInfo clearMethod = typeof(TBuilder).GetMethod("Clear" + name);
      if (messageProperty == null || builderProperty == null || hasProperty == null || clearMethod == null) {
        throw new ArgumentException("Not all required properties/methods available");
      }
      clrType = messageProperty.PropertyType;
      hasDelegate = (Func<TMessage, bool>)Delegate.CreateDelegate(typeof(Func<TMessage, bool>), hasProperty.GetGetMethod());
      clearDelegate = (Func<TBuilder, IBuilder>)Delegate.CreateDelegate(typeof(Func<TBuilder, IBuilder>), clearMethod);
      getValueDelegate = ReflectionUtil.CreateUpcastDelegate<TMessage>(messageProperty.GetGetMethod());
      setValueDelegate = ReflectionUtil.CreateDowncastDelegate<TBuilder>(builderProperty.GetSetMethod());
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
      return getValueDelegate(message);
    }

    public virtual void SetValue(TBuilder builder, object value) {
      setValueDelegate(builder, value);
    }

    #region Methods only related to repeated values
    public int GetRepeatedCount(TMessage message) {
      throw new InvalidOperationException();
    }

    public object GetRepeatedValue(TMessage message, int index) {
      throw new InvalidOperationException();
    }

    public void SetRepeated(TBuilder builder, int index, object value) {
      throw new InvalidOperationException();
    }

    public void AddRepeated(TBuilder builder, object value) {
      throw new InvalidOperationException();
    }

    public object GetRepeatedWrapper(TBuilder builder) {
      throw new InvalidOperationException();
    }
    #endregion
  }
}