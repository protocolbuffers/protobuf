namespace Google.ProtocolBuffers.DescriptorProtos {

  /// <summary>
  /// Interface implemented by all DescriptorProtos. The generator doesn't
  /// emit the interface implementation claim, so PartialClasses.cs contains
  /// partial class declarations for each of them.
  /// </summary>
  /// <typeparam name="TOptions">The associated options protocol buffer type</typeparam>
  public interface IDescriptorProto<TOptions> {
    /// <summary>
    /// The fully qualified name of the descriptor's target.
    /// </summary>
    string FullName { get; }

    /// <summary>
    /// The brief name of the descriptor's target.
    /// </summary>
    string Name { get; }

    TOptions Options { get; }
  }
}
