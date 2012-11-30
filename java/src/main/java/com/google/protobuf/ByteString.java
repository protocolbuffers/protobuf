// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

package com.google.protobuf;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.UnsupportedEncodingException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Iterator;
import java.util.List;
import java.util.NoSuchElementException;

/**
 * Immutable sequence of bytes.  Substring is supported by sharing the reference
 * to the immutable underlying bytes, as with {@link String}.  Concatenation is
 * likewise supported without copying (long strings) by building a tree of
 * pieces in {@link RopeByteString}.
 * <p>
 * Like {@link String}, the contents of a {@link ByteString} can never be
 * observed to change, not even in the presence of a data race or incorrect
 * API usage in the client code.
 *
 * @author crazybob@google.com Bob Lee
 * @author kenton@google.com Kenton Varda
 * @author carlanton@google.com Carl Haverl
 * @author martinrb@google.com Martin Buchholz
 */
public abstract class ByteString implements Iterable<Byte> {

  /**
   * When two strings to be concatenated have a combined length shorter than
   * this, we just copy their bytes on {@link #concat(ByteString)}.
   * The trade-off is copy size versus the overhead of creating tree nodes
   * in {@link RopeByteString}.
   */
  static final int CONCATENATE_BY_COPY_SIZE = 128;

  /**
   * When copying an InputStream into a ByteString with .readFrom(),
   * the chunks in the underlying rope start at 256 bytes, but double
   * each iteration up to 8192 bytes.
   */
  static final int MIN_READ_FROM_CHUNK_SIZE = 0x100;  // 256b
  static final int MAX_READ_FROM_CHUNK_SIZE = 0x2000;  // 8k

  /**
   * Empty {@code ByteString}.
   */
  public static final ByteString EMPTY = new LiteralByteString(new byte[0]);

  // This constructor is here to prevent subclassing outside of this package,
  ByteString() {}

  /**
   * Gets the byte at the given index. This method should be used only for
   * random access to individual bytes. To access bytes sequentially, use the
   * {@link ByteIterator} returned by {@link #iterator()}, and call {@link
   * #substring(int, int)} first if necessary.
   *
   * @param index index of byte
   * @return the value
   * @throws ArrayIndexOutOfBoundsException {@code index} is < 0 or >= size
   */
  public abstract byte byteAt(int index);

  /**
   * Return a {@link ByteString.ByteIterator} over the bytes in the ByteString.
   * To avoid auto-boxing, you may get the iterator manually and call
   * {@link ByteIterator#nextByte()}.
   *
   * @return the iterator
   */
  public abstract ByteIterator iterator();

  /**
   * This interface extends {@code Iterator<Byte>}, so that we can return an
   * unboxed {@code byte}.
   */
  public interface ByteIterator extends Iterator<Byte> {
    /**
     * An alternative to {@link Iterator#next()} that returns an
     * unboxed primitive {@code byte}.
     *
     * @return the next {@code byte} in the iteration
     * @throws NoSuchElementException if the iteration has no more elements
     */
    byte nextByte();
  }

  /**
   * Gets the number of bytes.
   *
   * @return size in bytes
   */
  public abstract int size();

  /**
   * Returns {@code true} if the size is {@code 0}, {@code false} otherwise.
   *
   * @return true if this is zero bytes long
   */
  public boolean isEmpty() {
    return size() == 0;
  }

  // =================================================================
  // ByteString -> substring

  /**
   * Return the substring from {@code beginIndex}, inclusive, to the end of the
   * string.
   *
   * @param beginIndex start at this index
   * @return substring sharing underlying data
   * @throws IndexOutOfBoundsException if {@code beginIndex < 0} or
   *     {@code beginIndex > size()}.
   */
  public ByteString substring(int beginIndex) {
    return substring(beginIndex, size());
  }

  /**
   * Return the substring from {@code beginIndex}, inclusive, to {@code
   * endIndex}, exclusive.
   *
   * @param beginIndex start at this index
   * @param endIndex   the last character is the one before this index
   * @return substring sharing underlying data
   * @throws IndexOutOfBoundsException if {@code beginIndex < 0},
   *     {@code endIndex > size()}, or {@code beginIndex > endIndex}.
   */
  public abstract ByteString substring(int beginIndex, int endIndex);

  /**
   * Tests if this bytestring starts with the specified prefix.
   * Similar to {@link String#startsWith(String)}
   *
   * @param prefix the prefix.
   * @return <code>true</code> if the byte sequence represented by the
   *         argument is a prefix of the byte sequence represented by
   *         this string; <code>false</code> otherwise.
   */
  public boolean startsWith(ByteString prefix) {
    return size() >= prefix.size() &&
           substring(0, prefix.size()).equals(prefix);
  }

  // =================================================================
  // byte[] -> ByteString

  /**
   * Copies the given bytes into a {@code ByteString}.
   *
   * @param bytes source array
   * @param offset offset in source array
   * @param size number of bytes to copy
   * @return new {@code ByteString}
   */
  public static ByteString copyFrom(byte[] bytes, int offset, int size) {
    byte[] copy = new byte[size];
    System.arraycopy(bytes, offset, copy, 0, size);
    return new LiteralByteString(copy);
  }

  /**
   * Copies the given bytes into a {@code ByteString}.
   *
   * @param bytes to copy
   * @return new {@code ByteString}
   */
  public static ByteString copyFrom(byte[] bytes) {
    return copyFrom(bytes, 0, bytes.length);
  }

  /**
   * Copies the next {@code size} bytes from a {@code java.nio.ByteBuffer} into
   * a {@code ByteString}.
   *
   * @param bytes source buffer
   * @param size number of bytes to copy
   * @return new {@code ByteString}
   */
  public static ByteString copyFrom(ByteBuffer bytes, int size) {
    byte[] copy = new byte[size];
    bytes.get(copy);
    return new LiteralByteString(copy);
  }

  /**
   * Copies the remaining bytes from a {@code java.nio.ByteBuffer} into
   * a {@code ByteString}.
   *
   * @param bytes sourceBuffer
   * @return new {@code ByteString}
   */
  public static ByteString copyFrom(ByteBuffer bytes) {
    return copyFrom(bytes, bytes.remaining());
  }

  /**
   * Encodes {@code text} into a sequence of bytes using the named charset
   * and returns the result as a {@code ByteString}.
   *
   * @param text source string
   * @param charsetName encoding to use
   * @return new {@code ByteString}
   * @throws UnsupportedEncodingException if the encoding isn't found
   */
  public static ByteString copyFrom(String text, String charsetName)
      throws UnsupportedEncodingException {
    return new LiteralByteString(text.getBytes(charsetName));
  }

  /**
   * Encodes {@code text} into a sequence of UTF-8 bytes and returns the
   * result as a {@code ByteString}.
   *
   * @param text source string
   * @return new {@code ByteString}
   */
  public static ByteString copyFromUtf8(String text) {
    try {
      return new LiteralByteString(text.getBytes("UTF-8"));
    } catch (UnsupportedEncodingException e) {
      throw new RuntimeException("UTF-8 not supported?", e);
    }
  }

  // =================================================================
  // InputStream -> ByteString

  /**
   * Completely reads the given stream's bytes into a
   * {@code ByteString}, blocking if necessary until all bytes are
   * read through to the end of the stream.
   *
   * <b>Performance notes:</b> The returned {@code ByteString} is an
   * immutable tree of byte arrays ("chunks") of the stream data.  The
   * first chunk is small, with subsequent chunks each being double
   * the size, up to 8K.  If the caller knows the precise length of
   * the stream and wishes to avoid all unnecessary copies and
   * allocations, consider using the two-argument version of this
   * method, below.
   *
   * @param streamToDrain The source stream, which is read completely
   *     but not closed.
   * @return A new {@code ByteString} which is made up of chunks of
   *     various sizes, depending on the behavior of the underlying
   *     stream.
   * @throws IOException IOException is thrown if there is a problem
   *     reading the underlying stream.
   */
  public static ByteString readFrom(InputStream streamToDrain)
      throws IOException {
    return readFrom(
        streamToDrain, MIN_READ_FROM_CHUNK_SIZE, MAX_READ_FROM_CHUNK_SIZE);
  }

  /**
   * Completely reads the given stream's bytes into a
   * {@code ByteString}, blocking if necessary until all bytes are
   * read through to the end of the stream.
   *
   * <b>Performance notes:</b> The returned {@code ByteString} is an
   * immutable tree of byte arrays ("chunks") of the stream data.  The
   * chunkSize parameter sets the size of these byte arrays. In
   * particular, if the chunkSize is precisely the same as the length
   * of the stream, unnecessary allocations and copies will be
   * avoided. Otherwise, the chunks will be of the given size, except
   * for the last chunk, which will be resized (via a reallocation and
   * copy) to contain the remainder of the stream.
   *
   * @param streamToDrain The source stream, which is read completely
   *     but not closed.
   * @param chunkSize The size of the chunks in which to read the
   *     stream.
   * @return A new {@code ByteString} which is made up of chunks of
   *     the given size.
   * @throws IOException IOException is thrown if there is a problem
   *     reading the underlying stream.
   */
  public static ByteString readFrom(InputStream streamToDrain, int chunkSize)
      throws IOException {
    return readFrom(streamToDrain, chunkSize, chunkSize);
  }

  // Helper method that takes the chunk size range as a parameter.
  public static ByteString readFrom(InputStream streamToDrain, int minChunkSize,
      int maxChunkSize) throws IOException {
    Collection<ByteString> results = new ArrayList<ByteString>();

    // copy the inbound bytes into a list of chunks; the chunk size
    // grows exponentially to support both short and long streams.
    int chunkSize = minChunkSize;
    while (true) {
      ByteString chunk = readChunk(streamToDrain, chunkSize);
      if (chunk == null) {
        break;
      }
      results.add(chunk);
      chunkSize = Math.min(chunkSize * 2, maxChunkSize);
    }

    return ByteString.copyFrom(results);
  }

  /**
   * Blocks until a chunk of the given size can be made from the
   * stream, or EOF is reached.  Calls read() repeatedly in case the
   * given stream implementation doesn't completely fill the given
   * buffer in one read() call.
   *
   * @return A chunk of the desired size, or else a chunk as large as
   * was available when end of stream was reached. Returns null if the
   * given stream had no more data in it.
   */
  private static ByteString readChunk(InputStream in, final int chunkSize)
      throws IOException {
      final byte[] buf = new byte[chunkSize];
      int bytesRead = 0;
      while (bytesRead < chunkSize) {
        final int count = in.read(buf, bytesRead, chunkSize - bytesRead);
        if (count == -1) {
          break;
        }
        bytesRead += count;
      }

      if (bytesRead == 0) {
        return null;
      } else {
        return ByteString.copyFrom(buf, 0, bytesRead);
      }
  }

  // =================================================================
  // Multiple ByteStrings -> One ByteString

  /**
   * Concatenate the given {@code ByteString} to this one. Short concatenations,
   * of total size smaller than {@link ByteString#CONCATENATE_BY_COPY_SIZE}, are
   * produced by copying the underlying bytes (as per Rope.java, <a
   * href="http://www.cs.ubc.ca/local/reading/proceedings/spe91-95/spe/vol25/issue12/spe986.pdf">
   * BAP95 </a>. In general, the concatenate involves no copying.
   *
   * @param other string to concatenate
   * @return a new {@code ByteString} instance
   */
  public ByteString concat(ByteString other) {
    int thisSize = size();
    int otherSize = other.size();
    if ((long) thisSize + otherSize >= Integer.MAX_VALUE) {
      throw new IllegalArgumentException("ByteString would be too long: " +
                                         thisSize + "+" + otherSize);
    }

    return RopeByteString.concatenate(this, other);
  }

  /**
   * Concatenates all byte strings in the iterable and returns the result.
   * This is designed to run in O(list size), not O(total bytes).
   *
   * <p>The returned {@code ByteString} is not necessarily a unique object.
   * If the list is empty, the returned object is the singleton empty
   * {@code ByteString}.  If the list has only one element, that
   * {@code ByteString} will be returned without copying.
   *
   * @param byteStrings strings to be concatenated
   * @return new {@code ByteString}
   */
  public static ByteString copyFrom(Iterable<ByteString> byteStrings) {
    Collection<ByteString> collection;
    if (!(byteStrings instanceof Collection)) {
      collection = new ArrayList<ByteString>();
      for (ByteString byteString : byteStrings) {
        collection.add(byteString);
      }
    } else {
      collection = (Collection<ByteString>) byteStrings;
    }
    ByteString result;
    if (collection.isEmpty()) {
      result = EMPTY;
    } else {
      result = balancedConcat(collection.iterator(), collection.size());
    }
    return result;
  }

  // Internal function used by copyFrom(Iterable<ByteString>).
  // Create a balanced concatenation of the next "length" elements from the
  // iterable.
  private static ByteString balancedConcat(Iterator<ByteString> iterator,
      int length) {
    assert length >= 1;
    ByteString result;
    if (length == 1) {
      result = iterator.next();
    } else {
      int halfLength = length >>> 1;
      ByteString left = balancedConcat(iterator, halfLength);
      ByteString right = balancedConcat(iterator, length - halfLength);
      result = left.concat(right);
    }
    return result;
  }

  // =================================================================
  // ByteString -> byte[]

  /**
   * Copies bytes into a buffer at the given offset.
   *
   * @param target buffer to copy into
   * @param offset in the target buffer
   * @throws IndexOutOfBoundsException if the offset is negative or too large
   */
  public void copyTo(byte[] target, int offset) {
    copyTo(target, 0, offset, size());
  }

  /**
   * Copies bytes into a buffer.
   *
   * @param target       buffer to copy into
   * @param sourceOffset offset within these bytes
   * @param targetOffset offset within the target buffer
   * @param numberToCopy number of bytes to copy
   * @throws IndexOutOfBoundsException if an offset or size is negative or too
   *     large
   */
  public void copyTo(byte[] target, int sourceOffset, int targetOffset,
      int numberToCopy) {
    if (sourceOffset < 0) {
      throw new IndexOutOfBoundsException("Source offset < 0: " + sourceOffset);
    }
    if (targetOffset < 0) {
      throw new IndexOutOfBoundsException("Target offset < 0: " + targetOffset);
    }
    if (numberToCopy < 0) {
      throw new IndexOutOfBoundsException("Length < 0: " + numberToCopy);
    }
    if (sourceOffset + numberToCopy > size()) {
      throw new IndexOutOfBoundsException(
          "Source end offset < 0: " + (sourceOffset + numberToCopy));
    }
    if (targetOffset + numberToCopy > target.length) {
      throw new IndexOutOfBoundsException(
          "Target end offset < 0: " + (targetOffset + numberToCopy));
    }
    if (numberToCopy > 0) {
      copyToInternal(target, sourceOffset, targetOffset, numberToCopy);
    }
  }

  /**
   * Internal (package private) implementation of
   * @link{#copyTo(byte[],int,int,int}.
   * It assumes that all error checking has already been performed and that 
   * @code{numberToCopy > 0}.
   */
  protected abstract void copyToInternal(byte[] target, int sourceOffset,
      int targetOffset, int numberToCopy);

  /**
   * Copies bytes into a ByteBuffer.
   *
   * @param target ByteBuffer to copy into.
   * @throws java.nio.ReadOnlyBufferException if the {@code target} is read-only
   * @throws java.nio.BufferOverflowException if the {@code target}'s
   *     remaining() space is not large enough to hold the data.
   */
  public abstract void copyTo(ByteBuffer target);

  /**
   * Copies bytes to a {@code byte[]}.
   *
   * @return copied bytes
   */
  public byte[] toByteArray() {
    int size = size();
    byte[] result = new byte[size];
    copyToInternal(result, 0, 0, size);
    return result;
  }

  /**
   * Writes the complete contents of this byte string to
   * the specified output stream argument.
   *
   * @param  out  the output stream to which to write the data.
   * @throws IOException  if an I/O error occurs.
   */
  public abstract void writeTo(OutputStream out) throws IOException;

  /**
   * Constructs a read-only {@code java.nio.ByteBuffer} whose content
   * is equal to the contents of this byte string.
   * The result uses the same backing array as the byte string, if possible.
   *
   * @return wrapped bytes
   */
  public abstract ByteBuffer asReadOnlyByteBuffer();

  /**
   * Constructs a list of read-only {@code java.nio.ByteBuffer} objects
   * such that the concatenation of their contents is equal to the contents
   * of this byte string.  The result uses the same backing arrays as the
   * byte string.
   * <p>
   * By returning a list, implementations of this method may be able to avoid
   * copying even when there are multiple backing arrays.
   * 
   * @return a list of wrapped bytes
   */
  public abstract List<ByteBuffer> asReadOnlyByteBufferList();

  /**
   * Constructs a new {@code String} by decoding the bytes using the
   * specified charset.
   *
   * @param charsetName encode using this charset
   * @return new string
   * @throws UnsupportedEncodingException if charset isn't recognized
   */
  public abstract String toString(String charsetName)
      throws UnsupportedEncodingException;

  // =================================================================
  // UTF-8 decoding

  /**
   * Constructs a new {@code String} by decoding the bytes as UTF-8.
   *
   * @return new string using UTF-8 encoding
   */
  public String toStringUtf8() {
    try {
      return toString("UTF-8");
    } catch (UnsupportedEncodingException e) {
      throw new RuntimeException("UTF-8 not supported?", e);
    }
  }

  /**
   * Tells whether this {@code ByteString} represents a well-formed UTF-8
   * byte sequence, such that the original bytes can be converted to a
   * String object and then round tripped back to bytes without loss.
   *
   * <p>More precisely, returns {@code true} whenever: <pre> {@code
   * Arrays.equals(byteString.toByteArray(),
   *     new String(byteString.toByteArray(), "UTF-8").getBytes("UTF-8"))
   * }</pre>
   *
   * <p>This method returns {@code false} for "overlong" byte sequences,
   * as well as for 3-byte sequences that would map to a surrogate
   * character, in accordance with the restricted definition of UTF-8
   * introduced in Unicode 3.1.  Note that the UTF-8 decoder included in
   * Oracle's JDK has been modified to also reject "overlong" byte
   * sequences, but (as of 2011) still accepts 3-byte surrogate
   * character byte sequences.
   *
   * <p>See the Unicode Standard,</br>
   * Table 3-6. <em>UTF-8 Bit Distribution</em>,</br>
   * Table 3-7. <em>Well Formed UTF-8 Byte Sequences</em>.
   *
   * @return whether the bytes in this {@code ByteString} are a
   * well-formed UTF-8 byte sequence
   */
  public abstract boolean isValidUtf8();

  /**
   * Tells whether the given byte sequence is a well-formed, malformed, or
   * incomplete UTF-8 byte sequence.  This method accepts and returns a partial
   * state result, allowing the bytes for a complete UTF-8 byte sequence to be
   * composed from multiple {@code ByteString} segments.
   *
   * @param state either {@code 0} (if this is the initial decoding operation)
   *     or the value returned from a call to a partial decoding method for the
   *     previous bytes
   * @param offset offset of the first byte to check
   * @param length number of bytes to check
   *
   * @return {@code -1} if the partial byte sequence is definitely malformed,
   * {@code 0} if it is well-formed (no additional input needed), or, if the
   * byte sequence is "incomplete", i.e. apparently terminated in the middle of
   * a character, an opaque integer "state" value containing enough information
   * to decode the character when passed to a subsequent invocation of a
   * partial decoding method.
   */
  protected abstract int partialIsValidUtf8(int state, int offset, int length);

  // =================================================================
  // equals() and hashCode()

  @Override
  public abstract boolean equals(Object o);

  /**
   * Return a non-zero hashCode depending only on the sequence of bytes
   * in this ByteString.
   *
   * @return hashCode value for this object
   */
  @Override
  public abstract int hashCode();

  // =================================================================
  // Input stream

  /**
   * Creates an {@code InputStream} which can be used to read the bytes.
   * <p>
   * The {@link InputStream} returned by this method is guaranteed to be
   * completely non-blocking.  The method {@link InputStream#available()}
   * returns the number of bytes remaining in the stream. The methods
   * {@link InputStream#read(byte[]), {@link InputStream#read(byte[],int,int)}
   * and {@link InputStream#skip(long)} will read/skip as many bytes as are
   * available.
   * <p>
   * The methods in the returned {@link InputStream} might <b>not</b> be
   * thread safe.
   *
   * @return an input stream that returns the bytes of this byte string.
   */
  public abstract InputStream newInput();

  /**
   * Creates a {@link CodedInputStream} which can be used to read the bytes.
   * Using this is often more efficient than creating a {@link CodedInputStream}
   * that wraps the result of {@link #newInput()}.
   *
   * @return stream based on wrapped data
   */
  public abstract CodedInputStream newCodedInput();

  // =================================================================
  // Output stream

  /**
   * Creates a new {@link Output} with the given initial capacity. Call {@link
   * Output#toByteString()} to create the {@code ByteString} instance.
   * <p>
   * A {@link ByteString.Output} offers the same functionality as a
   * {@link ByteArrayOutputStream}, except that it returns a {@link ByteString}
   * rather than a {@code byte} array.
   *
   * @param initialCapacity estimate of number of bytes to be written
   * @return {@code OutputStream} for building a {@code ByteString}
   */
  public static Output newOutput(int initialCapacity) {
    return new Output(initialCapacity);
  }

  /**
   * Creates a new {@link Output}. Call {@link Output#toByteString()} to create
   * the {@code ByteString} instance.
   * <p>
   * A {@link ByteString.Output} offers the same functionality as a
   * {@link ByteArrayOutputStream}, except that it returns a {@link ByteString}
   * rather than a {@code byte array}.
   *
   * @return {@code OutputStream} for building a {@code ByteString}
   */
  public static Output newOutput() {
    return new Output(CONCATENATE_BY_COPY_SIZE);
  }

  /**
   * Outputs to a {@code ByteString} instance. Call {@link #toByteString()} to
   * create the {@code ByteString} instance.
   */
  public static final class Output extends OutputStream {
    // Implementation note.
    // The public methods of this class must be synchronized.  ByteStrings
    // are guaranteed to be immutable.  Without some sort of locking, it could
    // be possible for one thread to call toByteSring(), while another thread
    // is still modifying the underlying byte array.

    private static final byte[] EMPTY_BYTE_ARRAY = new byte[0];
    // argument passed by user, indicating initial capacity.
    private final int initialCapacity;
    // ByteStrings to be concatenated to create the result
    private final ArrayList<ByteString> flushedBuffers;
    // Total number of bytes in the ByteStrings of flushedBuffers
    private int flushedBuffersTotalBytes;
    // Current buffer to which we are writing
    private byte[] buffer;
    // Location in buffer[] to which we write the next byte.
    private int bufferPos;

    /**
     * Creates a new ByteString output stream with the specified
     * initial capacity.
     *
     * @param initialCapacity  the initial capacity of the output stream.
     */
    Output(int initialCapacity) {
      if (initialCapacity < 0) {
        throw new IllegalArgumentException("Buffer size < 0");
      }
      this.initialCapacity = initialCapacity;
      this.flushedBuffers = new ArrayList<ByteString>();
      this.buffer = new byte[initialCapacity];
    }

    @Override
    public synchronized void write(int b) {
      if (bufferPos == buffer.length) {
        flushFullBuffer(1);
      }
      buffer[bufferPos++] = (byte)b;
    }

    @Override
    public synchronized void write(byte[] b, int offset, int length)  {
      if (length <= buffer.length - bufferPos) {
        // The bytes can fit into the current buffer.
        System.arraycopy(b, offset, buffer, bufferPos, length);
        bufferPos += length;
      } else {
        // Use up the current buffer
        int copySize  = buffer.length - bufferPos;
        System.arraycopy(b, offset, buffer, bufferPos, copySize);
        offset += copySize;
        length -= copySize;
        // Flush the buffer, and get a new buffer at least big enough to cover
        // what we still need to output
        flushFullBuffer(length);
        System.arraycopy(b, offset, buffer, 0 /* count */, length);
        bufferPos = length;
      }
    }

    /**
     * Creates a byte string. Its size is the current size of this output
     * stream and its output has been copied to it.
     *
     * @return  the current contents of this output stream, as a byte string.
     */
    public synchronized ByteString toByteString() {
      flushLastBuffer();
      return ByteString.copyFrom(flushedBuffers);
    }
    
    /**
     * Implement java.util.Arrays.copyOf() for jdk 1.5.
     */
    private byte[] copyArray(byte[] buffer, int length) {
      byte[] result = new byte[length];
      System.arraycopy(buffer, 0, result, 0, Math.min(buffer.length, length));
      return result;
    }

    /**
     * Writes the complete contents of this byte array output stream to
     * the specified output stream argument.
     *
     * @param out the output stream to which to write the data.
     * @throws IOException  if an I/O error occurs.
     */
    public void writeTo(OutputStream out) throws IOException {
      ByteString[] cachedFlushBuffers;
      byte[] cachedBuffer;
      int cachedBufferPos;
      synchronized (this) {
        // Copy the information we need into local variables so as to hold
        // the lock for as short a time as possible.
        cachedFlushBuffers =
            flushedBuffers.toArray(new ByteString[flushedBuffers.size()]);
        cachedBuffer = buffer;
        cachedBufferPos = bufferPos;
      }
      for (ByteString byteString : cachedFlushBuffers) {
        byteString.writeTo(out);
      }

      out.write(copyArray(cachedBuffer, cachedBufferPos));
    }

    /**
     * Returns the current size of the output stream.
     *
     * @return  the current size of the output stream
     */
    public synchronized int size() {
      return flushedBuffersTotalBytes + bufferPos;
    }

    /**
     * Resets this stream, so that all currently accumulated output in the
     * output stream is discarded. The output stream can be used again,
     * reusing the already allocated buffer space.
     */
    public synchronized void reset() {
      flushedBuffers.clear();
      flushedBuffersTotalBytes = 0;
      bufferPos = 0;
    }

    @Override
    public String toString() {
      return String.format("<ByteString.Output@%s size=%d>",
          Integer.toHexString(System.identityHashCode(this)), size());
    }

    /**
     * Internal function used by writers.  The current buffer is full, and the
     * writer needs a new buffer whose size is at least the specified minimum
     * size.
     */
    private void flushFullBuffer(int minSize)  {
      flushedBuffers.add(new LiteralByteString(buffer));
      flushedBuffersTotalBytes += buffer.length;
      // We want to increase our total capacity by 50%, but as a minimum,
      // the new buffer should also at least be >= minSize and
      // >= initial Capacity.
      int newSize = Math.max(initialCapacity,
          Math.max(minSize, flushedBuffersTotalBytes >>> 1));
      buffer = new byte[newSize];
      bufferPos = 0;
    }

    /**
     * Internal function used by {@link #toByteString()}. The current buffer may
     * or may not be full, but it needs to be flushed.
     */
    private void flushLastBuffer()  {
      if (bufferPos < buffer.length) {
        if (bufferPos > 0) {
          byte[] bufferCopy = copyArray(buffer, bufferPos);
          flushedBuffers.add(new LiteralByteString(bufferCopy));
        }
        // We reuse this buffer for further writes.
      } else {
        // Buffer is completely full.  Huzzah.
        flushedBuffers.add(new LiteralByteString(buffer));
        // 99% of the time, we're not going to use this OutputStream again.
        // We set buffer to an empty byte stream so that we're handling this
        // case without wasting space.  In the rare case that more writes
        // *do* occur, this empty buffer will be flushed and an appropriately
        // sized new buffer will be created.
        buffer = EMPTY_BYTE_ARRAY;
      }
      flushedBuffersTotalBytes += bufferPos;
      bufferPos = 0;
    }
  }

  /**
   * Constructs a new {@code ByteString} builder, which allows you to
   * efficiently construct a {@code ByteString} by writing to a {@link
   * CodedOutputStream}. Using this is much more efficient than calling {@code
   * newOutput()} and wrapping that in a {@code CodedOutputStream}.
   *
   * <p>This is package-private because it's a somewhat confusing interface.
   * Users can call {@link Message#toByteString()} instead of calling this
   * directly.
   *
   * @param size The target byte size of the {@code ByteString}.  You must write
   *     exactly this many bytes before building the result.
   * @return the builder
   */
  static CodedBuilder newCodedBuilder(int size) {
    return new CodedBuilder(size);
  }

  /** See {@link ByteString#newCodedBuilder(int)}. */
  static final class CodedBuilder {
    private final CodedOutputStream output;
    private final byte[] buffer;

    private CodedBuilder(int size) {
      buffer = new byte[size];
      output = CodedOutputStream.newInstance(buffer);
    }

    public ByteString build() {
      output.checkNoSpaceLeft();

      // We can be confident that the CodedOutputStream will not modify the
      // underlying bytes anymore because it already wrote all of them.  So,
      // no need to make a copy.
      return new LiteralByteString(buffer);
    }

    public CodedOutputStream getCodedOutput() {
      return output;
    }
  }

  // =================================================================
  // Methods {@link RopeByteString} needs on instances, which aren't part of the
  // public API.

  /**
   * Return the depth of the tree representing this {@code ByteString}, if any,
   * whose root is this node. If this is a leaf node, return 0.
   *
   * @return tree depth or zero
   */
  protected abstract int getTreeDepth();

  /**
   * Return {@code true} if this ByteString is literal (a leaf node) or a
   * flat-enough tree in the sense of {@link RopeByteString}.
   *
   * @return true if the tree is flat enough
   */
  protected abstract boolean isBalanced();

  /**
   * Return the cached hash code if available.
   *
   * @return value of cached hash code or 0 if not computed yet
   */
  protected abstract int peekCachedHashCode();

  /**
   * Compute the hash across the value bytes starting with the given hash, and
   * return the result.  This is used to compute the hash across strings
   * represented as a set of pieces by allowing the hash computation to be
   * continued from piece to piece.
   *
   * @param h starting hash value
   * @param offset offset into this value to start looking at data values
   * @param length number of data values to include in the hash computation
   * @return ending hash value
   */
  protected abstract int partialHash(int h, int offset, int length);

  @Override
  public String toString() {
    return String.format("<ByteString@%s size=%d>",
        Integer.toHexString(System.identityHashCode(this)), size());
  }
}
