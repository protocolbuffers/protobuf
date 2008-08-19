using System.IO;
namespace Google.ProtocolBuffers {

  /// <summary>
  /// Delegate to return a stream when asked, used by MessageStreamIterator.
  /// </summary>
  public delegate Stream StreamProvider();
}
