using System;
using System.Collections.Generic;
using System.Text;

namespace Google.ProtocolBuffers.FieldAccess {
  internal interface IFieldAccessor<TMessage, TBuilder> 
      where TMessage : IMessage<TMessage> 
      where TBuilder : IBuilder<TMessage> {

    void AddRepeated(IBuilder<TMessage> builder, object value);
    bool Has(IMessage<TMessage> message);
    int GetRepeatedCount(IMessage<TMessage> message);
    void Clear(TBuilder builder);
    TBuilder CreateBuilder();

    /// <summary>
    /// Accessor for single fields
    /// </summary>
    object this[IMessage<TMessage> message] { get; }
    /// <summary>
    /// Mutator for single fields
    /// </summary>
    object this[IBuilder<TMessage> builder] { set; }

    /// <summary>
    /// Accessor for repeated fields
    /// </summary>
    object this[IMessage<TMessage> message, int index] { get; }
    /// <summary>
    /// Mutator for repeated fields
    /// </summary>
    object this[IBuilder<TMessage> builder, int index] { set; }
  }
}
