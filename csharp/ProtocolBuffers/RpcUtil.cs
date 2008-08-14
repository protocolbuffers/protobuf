using System;
using System.Collections.Generic;
using System.Text;

namespace Google.ProtocolBuffers {
  public static class RpcUtil {

    /// <summary>
    /// Converts an Action[IMessage] to an Action[T].
    /// </summary>
    public static Action<T> SpecializeCallback<T>(Action<IMessage> action) 
        where T : IMessage<T> {
      return message => action(message);
    }

    /// <summary>
    /// Converts an Action[T] to an Action[IMessage].
    /// The generalized action will accept any message object which has
    /// the same descriptor, and will convert it to the correct class
    /// before calling the original action. However, if the generalized
    /// callback is given a message with a different descriptor, an
    /// exception will be thrown.
    /// </summary>
    public static Action<IMessage> GeneralizeCallback<TMessage, TBuilder>(Action<TMessage> action, TMessage defaultInstance)
        where TMessage : class, IMessage<TMessage, TBuilder> 
        where TBuilder : IBuilder<TMessage, TBuilder> {
      return message => {
        TMessage castMessage = message as TMessage;
        if (castMessage == null) {
          castMessage = defaultInstance.CreateBuilderForType().MergeFrom(message).Build();
        }
        action(castMessage);
      };
    }
  }
}
