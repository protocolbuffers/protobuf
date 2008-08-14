
namespace Google.ProtocolBuffers {
  public sealed class CodedInputStream {

    /// <summary>
    /// Decode a 32-bit value with ZigZag encoding.
    /// </summary>
    /// <remarks>
    /// ZigZag encodes signed integers into values that can be efficiently
    /// encoded with varint.  (Otherwise, negative values must be 
    /// sign-extended to 64 bits to be varint encoded, thus always taking
    /// 10 bytes on the wire.)
    /// </remarks>
    public static int DecodeZigZag32(uint n) {
      return (int)(n >> 1) ^ -(int)(n & 1);
    }

    /// <summary>
    /// Decode a 32-bit value with ZigZag encoding.
    /// </summary>
    /// <remarks>
    /// ZigZag encodes signed integers into values that can be efficiently
    /// encoded with varint.  (Otherwise, negative values must be 
    /// sign-extended to 64 bits to be varint encoded, thus always taking
    /// 10 bytes on the wire.)
    /// </remarks>
    public static long DecodeZigZag64(ulong n) {
      return (long)(n >> 1) ^ -(long)(n & 1);
    }
  }
}
