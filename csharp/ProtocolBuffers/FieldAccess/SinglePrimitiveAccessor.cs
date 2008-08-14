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
  internal class SinglePrimitiveAccessor : IFieldAccessor {

    private readonly PropertyInfo messageProperty;
    private readonly PropertyInfo builderProperty;
    private readonly PropertyInfo hasProperty;
    private readonly MethodInfo clearMethod;

    /// <summary>
    /// The CLR type of the field (int, the enum type, ByteString, the message etc).
    /// As declared by the property.
    /// </summary>
    protected Type ClrType {
      get { return messageProperty.PropertyType; }
    }

    internal SinglePrimitiveAccessor(string name, Type messageType, Type builderType) {
      messageProperty = messageType.GetProperty(name);
      builderProperty = builderType.GetProperty(name);
      hasProperty = messageType.GetProperty("Has" + name);
      clearMethod = builderType.GetMethod("Clear" + name);
      if (messageProperty == null || builderProperty == null || hasProperty == null || clearMethod == null) {
        throw new ArgumentException("Not all required properties/methods available");
      }
    }

    public bool Has(IMessage message) {
      return (bool) hasProperty.GetValue(message, null);
    }

    public void Clear(IBuilder builder) {
      clearMethod.Invoke(builder, null);
    }

    /// <summary>
    /// Only valid for message types - this implementation throws InvalidOperationException.
    /// </summary>
    public virtual IBuilder CreateBuilder() {
      throw new InvalidOperationException();
    }

    public virtual object GetValue(IMessage message) {
      return messageProperty.GetValue(message, null);
    }

    public virtual void SetValue(IBuilder builder, object value) {
      builderProperty.SetValue(builder, value, null);
    }

    #region Methods only related to repeated values
    public int GetRepeatedCount(IMessage message) {
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