namespace Google.ProtocolBuffers.FieldAccess {

  /// <summary>
  /// Allows fields to be reflectively accessed in a smart manner.
  /// The property descriptors for each field are created once and then cached.
  /// In addition, this interface holds knowledge of repeated fields, builders etc.
  /// </summary>
  internal interface IFieldAccessor {

    /// <summary>
    /// Indicates whether the specified message contains the field.
    /// </summary>
    bool Has(IMessage message);

    /// <summary>
    /// Gets the count of the repeated field in the specified message.
    /// </summary>
    int GetRepeatedCount(IMessage message);

    /// <summary>
    /// Clears the field in the specified builder.
    /// </summary>
    /// <param name="builder"></param>
    void Clear(IBuilder builder);

    /// <summary>
    /// Creates a builder for the type of this field (which must be a message field).
    /// </summary>
    IBuilder CreateBuilder();

    /// <summary>
    /// Accessor for single fields
    /// </summary>
    object GetValue(IMessage message);
    /// <summary>
    /// Mutator for single fields
    /// </summary>
    void SetValue(IBuilder builder, object value);

    /// <summary>
    /// Accessor for repeated fields
    /// </summary>
    object GetRepeatedValue(IMessage message, int index);
    /// <summary>
    /// Mutator for repeated fields
    /// </summary>
    void SetRepeated(IBuilder builder, int index, object value);
    /// <summary>
    /// Adds the specified value to the field in the given builder.
    /// </summary>
    void AddRepeated(IBuilder builder, object value);
    /// <summary>
    /// Returns a read-only wrapper around the value of a repeated field.
    /// </summary>
    object GetRepeatedWrapper(IBuilder builder);
  }
}
