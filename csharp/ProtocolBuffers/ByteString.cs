using System.Text;
using System;

namespace Google.ProtocolBuffers {
  /// <summary>
  /// Immutable array of bytes.
  /// TODO(jonskeet): Implement the common collection interfaces?
  /// </summary>
  public sealed class ByteString {

    private static readonly ByteString empty = new ByteString(new byte[0]);

    private byte[] bytes;

    /// <summary>
    /// Constructs a new ByteString from the given byte array. The array is
    /// *not* copied, and must not be modified after this constructor is called.
    /// </summary>
    private ByteString(byte[] bytes) {
      this.bytes = bytes;
    }

    /// <summary>
    /// Returns an empty ByteString.
    /// </summary>
    public static ByteString Empty {
      get { return empty; }
    }

    /// <summary>
    /// Returns the length of this ByteString in bytes.
    /// </summary>
    public int Length {
      get { return bytes.Length; }
    }

    public bool IsEmpty {
      get { return Length == 0; }
    }

    public byte[] ToByteArray() {
      return (byte[])bytes.Clone();
    }

    /// <summary>
    /// Constructs a ByteString from the given array. The contents
    /// are copied, so further modifications to the array will not
    /// be reflected in the returned ByteString.
    /// </summary>
    public static ByteString CopyFrom(byte[] bytes) {
      return new ByteString((byte[]) bytes.Clone());
    }

    /// <summary>
    /// Constructs a ByteString from a portion of a byte array.
    /// </summary>
    public static ByteString CopyFrom(byte[] bytes, int offset, int count) {
      byte[] portion = new byte[count];
      Array.Copy(bytes, offset, portion, 0, count);
      return new ByteString(portion);
    }

    /// <summary>
    /// Creates a new ByteString by encoding the specified text with
    /// the given encoding.
    /// </summary>
    public static ByteString CopyFrom(string text, Encoding encoding) {
      return new ByteString(encoding.GetBytes(text));
    }

    /// <summary>
    /// Creates a new ByteString by encoding the specified text in UTF-8.
    /// </summary>
    public static ByteString CopyFromUtf8(string text) {
      return CopyFrom(text, Encoding.UTF8);
    }
    
    /// <summary>
    /// Retuns the byte at the given index.
    /// </summary>
    public byte this[int index] {
      get { return bytes[index]; }
    }

    public string ToString(Encoding encoding) {
      return encoding.GetString(bytes);
    }

    public string ToStringUtf8() {
      return ToString(Encoding.UTF8);
    }

    // TODO(jonskeet): CopyTo, Equals, GetHashCode if they turn out to be required
    // TODO(jonskeet): Input/Output stuff
  }
}
