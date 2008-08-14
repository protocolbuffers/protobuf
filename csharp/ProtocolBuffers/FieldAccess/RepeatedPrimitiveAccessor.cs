using System;
using System.Collections;
using System.Reflection;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers.FieldAccess {
  /// <summary>
  /// Accesor for a repeated field of type int, ByteString etc.
  /// </summary>
  internal class RepeatedPrimitiveAccessor : IFieldAccessor {

    private readonly PropertyInfo messageProperty;
    private readonly PropertyInfo builderProperty;
    private readonly PropertyInfo hasProperty;
    private readonly PropertyInfo countProperty;
    private readonly MethodInfo clearMethod;
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

    internal RepeatedPrimitiveAccessor(string name, Type messageType, Type builderType) {      
      messageProperty = messageType.GetProperty(name + "List");
      builderProperty = builderType.GetProperty(name + "List");
      hasProperty = messageType.GetProperty("Has" + name);
      countProperty = messageType.GetProperty(name + "Count");
      clearMethod = builderType.GetMethod("Clear" + name);
      addMethod = builderType.GetMethod("Add" + name);
      getElementMethod = messageType.GetMethod("Get" + name, new Type[] { typeof(int) });
      setElementMethod = builderType.GetMethod("Set" + name, new Type[] { typeof(int) });
      if (messageProperty == null 
          || builderProperty == null 
          || hasProperty == null 
          || countProperty == null
          || clearMethod == null
          || addMethod == null
          || getElementMethod == null
          || setElementMethod == null) {
        throw new ArgumentException("Not all required properties/methods available");
      }
    }

    public bool Has(IMessage message) {
      throw new InvalidOperationException();
    }
    
    public virtual IBuilder CreateBuilder() {
      throw new InvalidOperationException();
    }

    public virtual object GetValue(IMessage message) {
      return messageProperty.GetValue(message, null);
    }

    public void SetValue(IBuilder builder, object value) {
      // Add all the elements individually.  This serves two purposes:
      // 1) Verifies that each element has the correct type.
      // 2) Insures that the caller cannot modify the list later on and
      //    have the modifications be reflected in the message.
      Clear(builder);
      foreach (object element in (IEnumerable) value) {
        AddRepeated(builder, element);
      }
    }

    public void Clear(IBuilder builder) {
      clearMethod.Invoke(builder, null);
    }

    public int GetRepeatedCount(IMessage message) {
      return (int) countProperty.GetValue(null, null);
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