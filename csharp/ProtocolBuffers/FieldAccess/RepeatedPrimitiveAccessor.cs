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
using System.Collections;
using System.Reflection;

namespace Google.ProtocolBuffers.FieldAccess {
  /// <summary>
  /// Accesor for a repeated field of type int, ByteString etc.
  /// </summary>
  internal class RepeatedPrimitiveAccessor<TMessage, TBuilder> : IFieldAccessor<TMessage, TBuilder>
      where TMessage : IMessage<TMessage, TBuilder>
      where TBuilder : IBuilder<TMessage, TBuilder> {

    private readonly PropertyInfo messageProperty;
    private readonly PropertyInfo builderProperty;
    private readonly RepeatedCountDelegate<TMessage> countDelegate;
    private readonly ClearDelegate<TBuilder> clearDelegate;
    private readonly MethodInfo addMethod;
    private readonly MethodInfo getElementMethod;
    private readonly MethodInfo setElementMethod;

    /// <summary>
    /// The CLR type of the field (int, the enum type, ByteString, the message etc).
    /// This is taken from the return type of the method used to retrieve a single
    /// value.
    /// </summary>
    protected Type ClrType {
      get { return getElementMethod.ReturnType; }
    }

    internal RepeatedPrimitiveAccessor(string name) {      
      messageProperty = typeof(TMessage).GetProperty(name + "List");
      builderProperty = typeof(TBuilder).GetProperty(name + "List");
      PropertyInfo countProperty = typeof(TMessage).GetProperty(name + "Count");
      MethodInfo clearMethod = typeof(TBuilder).GetMethod("Clear" + name);
      getElementMethod = typeof(TMessage).GetMethod("Get" + name, new Type[] { typeof(int) });
      addMethod = typeof(TBuilder).GetMethod("Add" + name, new Type[] { ClrType });
      setElementMethod = typeof(TBuilder).GetMethod("Set" + name, new Type[] { typeof(int), ClrType });
      if (messageProperty == null 
          || builderProperty == null 
          || countProperty == null
          || clearMethod == null
          || addMethod == null
          || getElementMethod == null
          || setElementMethod == null) {
        throw new ArgumentException("Not all required properties/methods available");
      }
      clearDelegate = (ClearDelegate<TBuilder>)Delegate.CreateDelegate(typeof(ClearDelegate<TBuilder>), clearMethod);
      countDelegate = (RepeatedCountDelegate<TMessage>)Delegate.CreateDelegate
          (typeof(RepeatedCountDelegate<TMessage>), countProperty.GetGetMethod());
    }

    public bool Has(TMessage message) {
      throw new InvalidOperationException();
    }
    
    public virtual IBuilder CreateBuilder() {
      throw new InvalidOperationException();
    }

    public virtual object GetValue(TMessage message) {
      return messageProperty.GetValue(message, null);
    }

    public void SetValue(TBuilder builder, object value) {
      // Add all the elements individually.  This serves two purposes:
      // 1) Verifies that each element has the correct type.
      // 2) Insures that the caller cannot modify the list later on and
      //    have the modifications be reflected in the message.
      Clear(builder);
      foreach (object element in (IEnumerable) value) {
        AddRepeated(builder, element);
      }
    }

    public void Clear(TBuilder builder) {
      clearDelegate(builder);
    }

    public int GetRepeatedCount(TMessage message) {
      return countDelegate(message);
    }

    public virtual object GetRepeatedValue(IMessage message, int index) {
      return getElementMethod.Invoke(message, new object[] {index } );
    }

    public virtual void SetRepeated(IBuilder builder, int index, object value) {
      setElementMethod.Invoke(builder, new object[] {index, value} );
    }

    public virtual void AddRepeated(IBuilder builder, object value) {
      addMethod.Invoke(builder, new object[] { value });
    }

    /// <summary>
    /// The builder class's accessor already builds a read-only wrapper for
    /// us, which is exactly what we want.
    /// </summary>
    public object GetRepeatedWrapper(IBuilder builder) {
      return builderProperty.GetValue(builder, null);
    }
  }
}