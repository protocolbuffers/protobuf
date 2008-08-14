using System;
using System.Collections.Generic;
using System.Text;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers.FieldAccess {
  internal class SingularFieldAccessor<TMessage, TBuilder> : IFieldAccessor<TMessage, TBuilder>
    where TMessage : IMessage<TMessage>
    where TBuilder : IBuilder<TMessage> {

    readonly HasFunction<TMessage> hasProxy;
    readonly Action<TBuilder> clearProxy;

    internal SingularFieldAccessor(FieldDescriptor descriptor, String pascalCaseName) {

      /*          Class<? extends GeneratedMessage> messageClass,
                Class<? extends GeneratedMessage.Builder> builderClass) {
              getMethod = getMethodOrDie(messageClass, "get" + camelCaseName);
              type = getMethod.getReturnType();
              setMethod = getMethodOrDie(builderClass, "set" + camelCaseName, type);
              hasMethod =
                getMethodOrDie(messageClass, "has" + camelCaseName);
              clearMethod = getMethodOrDie(builderClass, "clear" + camelCaseName); */
    }

    public bool Has(IMessage<TMessage> message) {
      return false;// hasProxy(message);
    }

    public void Clear(TBuilder builder) {
//      clearProxy(builder);
    }

    public TBuilder CreateBuilder() {
//      return createBuilderProxy(builder);
      return default(TBuilder);
    }

    public object this[IMessage<TMessage> message] {
      get { return null;/* getProxy(message);*/ }
    }

    public object this[IBuilder<TMessage> builder] {
      set { /*setProxy(builder, value);*/ }
    }

    #region Repeated operations (which just throw an exception)
    public object this[IMessage<TMessage> message, int index] {
      get { throw new InvalidOperationException("Repeated operation called on singular field"); }
    }

    public object this[IBuilder<TMessage> builder, int index] {
      set { throw new InvalidOperationException("Repeated operation called on singular field"); }
    }

    public int GetRepeatedCount(IMessage<TMessage> message) {
      throw new InvalidOperationException("Repeated operation called on singular field");
    }

    public void AddRepeated(IBuilder<TMessage> builder, object value) {
      throw new InvalidOperationException("Repeated operation called on singular field");
    }
    #endregion
  }
}
