using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers {

  /// <summary>
  /// Base class for all generated extensions.
  /// </summary>
  /// <remarks>
  /// The protocol compiler generates a static singleton instance of this
  /// class for each extension. For exmaple, imagine a .proto file with:
  /// <code>
  /// message Foo {
  ///   extensions 1000 to max
  /// }
  /// 
  /// extend Foo {
  ///   optional int32 bar;
  /// }
  /// </code>
  /// Then MyProto.Foo.Bar has type GeneratedExtension&lt;MyProto.Foo,int&gt;.
  /// <para />
  /// In general, users should ignore the details of this type, and
  /// simply use the static singletons as parmaeters to the extension accessors
  /// in ExtendableMessage and ExtendableBuilder.
  /// </remarks>
  public class GeneratedExtension<TContainer, TExtension> where TContainer : IMessage<TContainer> {
    public FieldDescriptor Descriptor;

    public IMessage MessageDefaultInstance;
  }
}
