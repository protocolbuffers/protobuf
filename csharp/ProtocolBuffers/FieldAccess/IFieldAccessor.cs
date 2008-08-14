namespace Google.ProtocolBuffers.FieldAccess {

  /// <summary>
  /// Allows fields to be reflectively accessed in a smart manner.
  /// The property descriptors for each field are created once and then cached.
  /// In addition, this interface holds knowledge of repeated fields, builders etc.
  /// </summary>
  internal interface IFieldAccessor<TMessage, TBuilder> 
      where TMessage : IMessage<TMessage> 
      where TBuilder : IBuilder<TMessage> {

    /// <summary>
    /// Indicates whether the specified message contains the field.
    /// </summary>
    bool Has(IMessage<TMessage> message);

    /// <summary>
    /// Gets the count of the repeated field in the specified message.
    /// </summary>
    int GetRepeatedCount(IMessage<TMessage> message);

    /// <summary>
    /// Clears the field in the specified builder.
    /// </summary>
    /// <param name="builder"></param>
    void Clear(IBuilder<TMessage> builder);

    /// <summary>
    /// Creates a builder for the type of this field (which must be a message field).
    /// </summary>
    IBuilder CreateBuilder();

    /// <summary>
    /// Accessor for single fields
    /// </summary>
    object GetValue(IMessage<TMessage> message);
    /// <summary>
    /// Mutator for single fields
    /// </summary>
    void SetValue(IBuilder<TMessage> builder, object value);

    /// <summary>
    /// Accessor for repeated fields
    /// </summary>
    object GetRepeatedValue(IMessage<TMessage> message, int index);
    /// <summary>
    /// Mutator for repeated fields
    /// </summary>
    void SetRepeated(IBuilder<TMessage> builder, int index, object value);
    /// <summary>
    /// Adds the specified value to the field in the given builder.
    /// </summary>
    void AddRepeated(IBuilder<TMessage> builder, object value);
    /// <summary>
    /// Returns a read-only wrapper around the value of a repeated field.
    /// </summary>
    object GetRepeatedWrapper(IBuilder<TMessage> builder);
  }
}
