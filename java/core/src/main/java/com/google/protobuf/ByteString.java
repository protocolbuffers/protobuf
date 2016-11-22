// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
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

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InvalidObjectException;
import java.io.ObjectInputStream;
import java.io.OutputStream;
import java.io.Serializable;
import java.io.UnsupportedEncodingException;
import java.nio.ByteBuffer;
import java.nio.charset.Charset;
import java.nio.charset.UnsupportedCharsetException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.NoSuchElementException;

/**
 * Immutable sequence of bytes. Substring is supported by sharing the reference to the immutable
 * underlying bytes. Concatenation is likewise supported without copying (long strings) by building
 * a tree of pieces in {@link RopeByteString}.
 *
 * <p>Like {@link String}, the contents of a {@link ByteString} can never be observed to change, not
 * even in the presence of a data race or incorrect API usage in the client code.
 *
 * @author crazybob@google.com Bob Lee
 * @author kenton@google.com Kenton Varda
 * @author carlanton@google.com Carl Haverl
 * @author martinrb@google.com Martin Buchholz
 */
public abstract class ByteString implements Iterable<Byte>, Serializable {

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
  public static final ByteString EMPTY = new LiteralByteString(Internal.EMPTY_BYTE_ARRAY);

  /**
   * An interface to efficiently copy {@code byte[]}.
   *
   * <p>One of the noticeable costs of copying a byte[] into a new array using
   * {@code System.arraycopy} is nullification of a new buffer before the copy. It has been shown
   * the Hotspot VM is capable to intrisicfy {@code Arrays.copyOfRange} operation to avoid this
   * expensive nullification and provide substantial performance gain. Unfortunately this does not
   * hold on Android runtimes and could make the copy slightly slower due to additional code in
   * the {@code Arrays.copyOfRange}. Thus we provide two different implementation for array copier
   * for Hotspot and Android runtimes.
   */
  private interface ByteArrayCopier {
    /**
     * Copies the specified range of the specified array into a new array
     */
    byte[] copyFrom(byte[] bytes, int offset, int size);
  }

  /** Implementation of {@code ByteArrayCopier} which uses {@link System#arraycopy}. */
  private static final class SystemByteArrayCopier implements ByteArrayCopier {
    @Override
    public byte[] copyFrom(byte[] bytes, int offset, int size) {
      byte[] copy = new byte[size];
      System.arraycopy(bytes, offset, copy, 0, size);
      return copy;
    }
  }

  /** Implementation of {@code ByteArrayCopier} which uses {@link Arrays#copyOfRange}. */
  private static final class ArraysByteArrayCopier implements ByteArrayCopier {
    @Override
    public byte[] copyFrom(byte[] bytes, int offset, int size) {
      return Arrays.copyOfRange(bytes, offset, offset + size);
    }
  }

  private static final ByteArrayCopier byteArrayCopier;
  static {
    boolean isAndroid = true;
    try {
      Class.forName("android.content.Context");
    } catch (ClassNotFoundException e) {
      isAndroid = false;
    }

    byteArrayCopier = isAndroid ? new SystemByteArrayCopier() : new ArraysByteArrayCopier();
  }

  /**
   * Cached hash value. Intentionally accessed via a data race, which
   * is safe because of the Java Memory Model's "no out-of-thin-air values"
   * guarantees for ints. A value of 0 implies that the hash has not been set.
   */
  private int hash = 0;

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
   * @throws IndexOutOfBoundsException {@code index < 0 or index >= size}
   */
  public abstract byte byteAt(int index);

  /**
   * Return a {@link ByteString.ByteIterator} over the bytes in the ByteString.
   * To avoid auto-boxing, you may get the iterator manually and call
   * {@link ByteIterator#nextByte()}.
   *
   * @return the iterator
   */
  @Override
  public final ByteIterator iterator() {
    return new ByteIterator() {
      private int position = 0;
      private final int limit = size();

      @Override
      public boolean hasNext() {
        return position < limit;
      }

      @Override
      public Byte next() {
        // Boxing calls Byte.valueOf(byte), which does not instantiate.
        return nextByte();
      }

      @Override
      public byte nextByte() {
        try {
          return byteAt(position++);
        } catch (IndexOutOfBoundsException e) {
          throw new NoSuchElementException(e.getMessage());
        }
      }

      @Override
      public void remove() {
        throw new UnsupportedOperationException();
      }
    };
  }

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
  public final boolean isEmpty() {
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
  public final ByteString substring(int beginIndex) {
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
  public final boolean startsWith(ByteString prefix) {
    return size() >= prefix.size() &&
           substring(0, prefix.size()).equals(prefix);
  }

  /**
   * Tests if this bytestring ends with the specified suffix.
   * Similar to {@link String#endsWith(String)}
   *
   * @param suffix the suffix.
   * @return <code>true</code> if the byte sequence represented by the
   *         argument is a suffix of the byte sequence represented by
   *         this string; <code>false</code> otherwise.
   */
  public final boolean endsWith(ByteString suffix) {
    return size() >= suffix.size() &&
        substring(size() - suffix.size()).equals(suffix);
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
    return new LiteralByteString(byteArrayCopier.copyFrom(bytes, offset, size));
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
   * Wraps the given bytes into a {@code ByteString}. Intended for internal only usage.
   */
  static ByteString wrap(ByteBuffer buffer) {
    if (buffer.hasArray()) {
      final int offset = buffer.arrayOffset();
      return ByteString.wrap(buffer.array(), offset + buffer.position(), buffer.remaining());
    } else {
      return new NioByteString(buffer);
    }
  }

  /**
   * Wraps the given bytes into a {@code ByteString}. Intended for internal only
   * usage to force a classload of ByteString before LiteralByteString.
   */
  static ByteString wrap(byte[] bytes) {
    // TODO(dweis): Return EMPTY when bytes are empty to reduce allocations?
    return new LiteralByteString(bytes);
  }

  /**
   * Wraps the given bytes into a {@code ByteString}. Intended for internal only
   * usage to force a classload of ByteString before BoundedByteString and
   * LiteralByteString.
   */
  static ByteString wrap(byte[] bytes, int offset, int length) {
    return new BoundedByteString(bytes, offset, length);
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
   * Encodes {@code text} into a sequence of bytes using the named charset
   * and returns the result as a {@code ByteString}.
   *
   * @param text source string
   * @param charset encode using this charset
   * @return new {@code ByteString}
   */
  public static ByteString copyFrom(String text, Charset charset) {
    return new LiteralByteString(text.getBytes(charset));
  }

  /**
   * Encodes {@code text} into a sequence of UTF-8 bytes and returns the
   * result as a {@code ByteString}.
   *
   * @param text source string
   * @return new {@code ByteString}
   */
  public static ByteString copyFromUtf8(String text) {
    return new LiteralByteString(text.getBytes(Internal.UTF_8));
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
   * the size, up to 8K.
   *
   * <p>Each byte read from the input stream will be copied twice to ensure
   * that the resulting ByteString is truly immutable.
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
    return readFrom(streamToDrain, MIN_READ_FROM_CHUNK_SIZE, MAX_READ_FROM_CHUNK_SIZE);
  }

  /**
   * Completely reads the given stream's bytes into a
   * {@code ByteString}, blocking if necessary until all bytes are
   * read through to the end of the stream.
   *
   * <b>Performance notes:</b> The returned {@code ByteString} is an
   * immutable tree of byte arrays ("chunks") of the stream data.  The
   * chunkSize parameter sets the size of these byte arrays.
   *
   * <p>Each byte read from the input stream will be copied twice to ensure
   * that the resulting ByteString is truly immutable.
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
      }

      // Always make a copy since InputStream could steal a reference to buf.
      return ByteString.copyFrom(buf, 0, bytesRead);
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
  public final ByteString concat(ByteString other) {
    if (Integer.MAX_VALUE - size() < other.size()) {
      throw new IllegalArgumentException("ByteString would be too long: " +
          size() + "+" + other.size());
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
    // Determine the size;
    final int size;
    if (!(byteStrings instanceof Collection)) {
      int tempSize = 0;
      for (Iterator<ByteString> iter = byteStrings.iterator(); iter.hasNext();
          iter.next(), ++tempSize) {
      }
      size = tempSize;
    } else {
      size = ((Collection<ByteString>) byteStrings).size();
    }

    if (size == 0) {
      return EMPTY;
    }

    return balancedConcat(byteStrings.iterator(), size);
  }

  // Internal function used by copyFrom(Iterable<ByteString>).
  // Create a balanced concatenation of the next "length" elements from the
  // iterable.
  private static ByteString balancedConcat(Iterator<ByteString> iterator, int length) {
    if (length < 1) {
      throw new IllegalArgumentException(String.format("length (%s) must be >= 1", length));
    }
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
  public final void copyTo(byte[] target, int sourceOffset, int targetOffset,
      int numberToCopy) {
    checkRange(sourceOffset, sourceOffset + numberToCopy, size());
    checkRange(targetOffset, targetOffset + numberToCopy, target.length);
    if (numberToCopy > 0) {
      copyToInternal(target, sourceOffset, targetOffset, numberToCopy);
    }
  }

  /**
   * Internal (package private) implementation of
   * {@link #copyTo(byte[],int,int,int)}.
   * It assumes that all error checking has already been performed and that
   * {@code numberToCopy > 0}.
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
  public final byte[] toByteArray() {
    final int size = size();
    if (size == 0) {
      return Internal.EMPTY_BYTE_ARRAY;
    }
    byte[] result = new byte[size];
    copyToInternal(result, 0, 0, size);
    return result;
  }

  /**
   * Writes a copy of the contents of this byte string to the specified output stream argument.
   *
   * @param  out  the output stream to which to write the data.
   * @throws IOException  if an I/O error occurs.
   */
  public abstract void writeTo(OutputStream out) throws IOException;

  /**
   * Writes a specified part of this byte string to an output stream.
   *
   * @param  out  the output stream to which to write the data.
   * @param  sourceOffset offset within these bytes
   * @param  numberToWrite number of bytes to write
   * @throws IOException  if an I/O error occurs.
   * @throws IndexOutOfBoundsException if an offset or size is negative or too large
   */
  final void writeTo(OutputStream out, int sourceOffset, int numberToWrite)
      throws IOException {
    checkRange(sourceOffset, sourceOffset + numberToWrite, size());
    if (numberToWrite > 0) {
      writeToInternal(out, sourceOffset, numberToWrite);
    }
  }

  /**
   * Internal version of {@link #writeTo(OutputStream,int,int)} that assumes
   * all error checking has already been done.
   */
  abstract void writeToInternal(OutputStream out, int sourceOffset, int numberToWrite)
      throws IOException;

  /**
   * Writes this {@link ByteString} to the provided {@link ByteOutput}. Calling
   * this method may result in multiple operations on the target {@link ByteOutput}.
   *
   * <p>This method may expose internal backing buffers of the {@link ByteString} to the {@link
   * ByteOutput} in order to avoid additional copying overhead. It would be possible for a malicious
   * {@link ByteOutput} to corrupt the {@link ByteString}. Use with caution!
   *
   * @param  byteOutput  the output target to receive the bytes
   * @throws IOException  if an I/O error occurs
   * @see UnsafeByteOperations#unsafeWriteTo(ByteString, ByteOutput)
   */
  abstract void writeTo(ByteOutput byteOutput) throws IOException;


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
  public final String toString(String charsetName)
      throws UnsupportedEncodingException {
    try {
      return toString(Charset.forName(charsetName));
    } catch (UnsupportedCharsetException e) {
      UnsupportedEncodingException exception = new UnsupportedEncodingException(charsetName);
      exception.initCause(e);
      throw exception;
    }
  }

  /**
   * Constructs a new {@code String} by decoding the bytes using the
   * specified charset. Returns the same empty String if empty.
   *
   * @param charset encode using this charset
   * @return new string
   */
  public final String toString(Charset charset) {
    return size() == 0 ? "" : toStringInternal(charset);
  }

  /**
   * Constructs a new {@code String} by decoding the bytes using the
   * specified charset.
   *
   * @param charset encode using this charset
   * @return new string
   */
  protected abstract String toStringInternal(Charset charset);

  // =================================================================
  // UTF-8 decoding

  /**
   * Constructs a new {@code String} by decoding the bytes as UTF-8.
   *
   * @return new string using UTF-8 encoding
   */
  public final String toStringUtf8() {
    return toString(Internal.UTF_8);
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
   * <p>See the Unicode Standard,<br>
   * Table 3-6. <em>UTF-8 Bit Distribution</em>,<br>
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
   * Base class for leaf {@link ByteString}s (i.e. non-ropes).
   */
  abstract static class LeafByteString extends ByteString {
    @Override
    protected final int getTreeDepth() {
      return 0;
    }

    @Override
    protected final boolean isBalanced() {
      return true;
    }


    /**
     * Check equality of the substring of given length of this object starting at
     * zero with another {@code ByteString} substring starting at offset.
     *
     * @param other  what to compare a substring in
     * @param offset offset into other
     * @param length number of bytes to compare
     * @return true for equality of substrings, else false.
     */
    abstract boolean equalsRange(ByteString other, int offset, int length);
  }

  /**
   * Compute the hashCode using the traditional algorithm from {@link
   * ByteString}.
   *
   * @return hashCode value
   */
  @Override
  public final int hashCode() {
    int h = hash;

    if (h == 0) {
      int size = size();
      h = partialHash(size, 0, size);
      if (h == 0) {
        h = 1;
      }
      hash = h;
    }
    return h;
  }

  // =================================================================
  // Input stream

  /**
   * Creates an {@code InputStream} which can be used to read the bytes.
   * <p>
   * The {@link InputStream} returned by this method is guaranteed to be
   * completely non-blocking.  The method {@link InputStream#available()}
   * returns the number of bytes remaining in the stream. The methods
   * {@link InputStream#read(byte[])}, {@link InputStream#read(byte[],int,int)}
   * and {@link InputStream#skip(long)} will read/skip as many bytes as are
   * available.  The method {@link InputStream#markSupported()} returns
   * {@code true}.
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
  protected final int peekCachedHashCode() {
    return hash;
  }

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

  /**
   * Checks that the given index falls within the specified array size.
   *
   * @param index the index position to be tested
   * @param size the length of the array
   * @throws IndexOutOfBoundsException if the index does not fall within the array.
   */
  static void checkIndex(int index, int size) {
    if ((index | (size - (index + 1))) < 0) {
      if (index < 0) {
        throw new ArrayIndexOutOfBoundsException("Index < 0: " + index);
      }
      throw new ArrayIndexOutOfBoundsException("Index > length: " + index + ", " + size);
    }
  }

  /**
   * Checks that the given range falls within the bounds of an array
   *
   * @param startIndex the start index of the range (inclusive)
   * @param endIndex the end index of the range (exclusive)
   * @param size the size of the array.
   * @return the length of the range.
   * @throws IndexOutOfBoundsException some or all of the range falls outside of the array.
   */
  static int checkRange(int startIndex, int endIndex, int size) {
    final int length = endIndex - startIndex;
    if ((startIndex | endIndex | length | (size - endIndex)) < 0) {
      if (startIndex < 0) {
        throw new IndexOutOfBoundsException("Beginning index: " + startIndex + " < 0");
      }
      if (endIndex < startIndex) {
        throw new IndexOutOfBoundsException(
            "Beginning index larger than ending index: " + startIndex + ", " + endIndex);
      }
      // endIndex >= size
      throw new IndexOutOfBoundsException("End index: " + endIndex + " >= " + size);
    }
    return length;
  }

  @Override
  public final String toString() {
    return String.format("<ByteString@%s size=%d>",
        Integer.toHexString(System.identityHashCode(this)), size());
  }

  /**
   * This class implements a {@link com.google.protobuf.ByteString} backed by a
   * single array of bytes, contiguous in memory. It supports substring by
   * pointing to only a sub-range of the underlying byte array, meaning that a
   * substring will reference the full byte-array of the string it's made from,
   * exactly as with {@link String}.
   *
   * @author carlanton@google.com (Carl Haverl)
   */
  // Keep this class private to avoid deadlocks in classloading across threads as ByteString's
  // static initializer loads LiteralByteString and another thread loads LiteralByteString.
  private static class LiteralByteString extends ByteString.LeafByteString {
    private static final long serialVersionUID = 1L;

    protected final byte[] bytes;

    /**
     * Creates a {@code LiteralByteString} backed by the given array, without
     * copying.
     *
     * @param bytes array to wrap
     */
    LiteralByteString(byte[] bytes) {
      this.bytes = bytes;
    }

    @Override
    public byte byteAt(int index) {
      // Unlike most methods in this class, this one is a direct implementation
      // ignoring the potential offset because we need to do range-checking in the
      // substring case anyway.
      return bytes[index];
    }

    @Override
    public int size() {
      return bytes.length;
    }

    // =================================================================
    // ByteString -> substring

    @Override
    public final ByteString substring(int beginIndex, int endIndex) {
      final int length = checkRange(beginIndex, endIndex, size());

      if (length == 0) {
        return ByteString.EMPTY;
      }

      return new BoundedByteString(bytes, getOffsetIntoBytes() + beginIndex, length);
    }

    // =================================================================
    // ByteString -> byte[]

    @Override
    protected void copyToInternal(
        byte[] target, int sourceOffset, int targetOffset, int numberToCopy) {
      // Optimized form, not for subclasses, since we don't call
      // getOffsetIntoBytes() or check the 'numberToCopy' parameter.
      // TODO(nathanmittler): Is not calling getOffsetIntoBytes really saving that much?
      System.arraycopy(bytes, sourceOffset, target, targetOffset, numberToCopy);
    }

    @Override
    public final void copyTo(ByteBuffer target) {
      target.put(bytes, getOffsetIntoBytes(), size()); // Copies bytes
    }

    @Override
    public final ByteBuffer asReadOnlyByteBuffer() {
      return ByteBuffer.wrap(bytes, getOffsetIntoBytes(), size()).asReadOnlyBuffer();
    }

    @Override
    public final List<ByteBuffer> asReadOnlyByteBufferList() {
      return Collections.singletonList(asReadOnlyByteBuffer());
    }

    @Override
    public final void writeTo(OutputStream outputStream) throws IOException {
      outputStream.write(toByteArray());
    }

    @Override
    final void writeToInternal(OutputStream outputStream, int sourceOffset, int numberToWrite)
        throws IOException {
      outputStream.write(bytes, getOffsetIntoBytes() + sourceOffset, numberToWrite);
    }

    @Override
    final void writeTo(ByteOutput output) throws IOException {
      output.writeLazy(bytes, getOffsetIntoBytes(), size());
    }

    @Override
    protected final String toStringInternal(Charset charset) {
      return new String(bytes, getOffsetIntoBytes(), size(), charset);
    }

    // =================================================================
    // UTF-8 decoding

    @Override
    public final boolean isValidUtf8() {
      int offset = getOffsetIntoBytes();
      return Utf8.isValidUtf8(bytes, offset, offset + size());
    }

    @Override
    protected final int partialIsValidUtf8(int state, int offset, int length) {
      int index = getOffsetIntoBytes() + offset;
      return Utf8.partialIsValidUtf8(state, bytes, index, index + length);
    }

    // =================================================================
    // equals() and hashCode()

    @Override
    public final boolean equals(Object other) {
      if (other == this) {
        return true;
      }
      if (!(other instanceof ByteString)) {
        return false;
      }

      if (size() != ((ByteString) other).size()) {
        return false;
      }
      if (size() == 0) {
        return true;
      }

      if (other instanceof LiteralByteString) {
        LiteralByteString otherAsLiteral = (LiteralByteString) other;
        // If we know the hash codes and they are not equal, we know the byte
        // strings are not equal.
        int thisHash = peekCachedHashCode();
        int thatHash = otherAsLiteral.peekCachedHashCode();
        if (thisHash != 0 && thatHash != 0 && thisHash != thatHash) {
          return false;
        }

        return equalsRange((LiteralByteString) other, 0, size());
      } else {
        // RopeByteString and NioByteString.
        return other.equals(this);
      }
    }

    /**
     * Check equality of the substring of given length of this object starting at
     * zero with another {@code LiteralByteString} substring starting at offset.
     *
     * @param other  what to compare a substring in
     * @param offset offset into other
     * @param length number of bytes to compare
     * @return true for equality of substrings, else false.
     */
    @Override
    final boolean equalsRange(ByteString other, int offset, int length) {
      if (length > other.size()) {
        throw new IllegalArgumentException("Length too large: " + length + size());
      }
      if (offset + length > other.size()) {
        throw new IllegalArgumentException(
            "Ran off end of other: " + offset + ", " + length + ", " + other.size());
      }

      if (other instanceof LiteralByteString) {
        LiteralByteString lbsOther = (LiteralByteString) other;
        byte[] thisBytes = bytes;
        byte[] otherBytes = lbsOther.bytes;
        int thisLimit = getOffsetIntoBytes() + length;
        for (
            int thisIndex = getOffsetIntoBytes(),
                otherIndex = lbsOther.getOffsetIntoBytes() + offset;
            (thisIndex < thisLimit); ++thisIndex, ++otherIndex) {
          if (thisBytes[thisIndex] != otherBytes[otherIndex]) {
            return false;
          }
        }
        return true;
      }

      return other.substring(offset, offset + length).equals(substring(0, length));
    }

    @Override
    protected final int partialHash(int h, int offset, int length) {
      return Internal.partialHash(h, bytes, getOffsetIntoBytes() + offset, length);
    }

    // =================================================================
    // Input stream

    @Override
    public final InputStream newInput() {
      return new ByteArrayInputStream(bytes, getOffsetIntoBytes(), size()); // No copy
    }

    @Override
    public final CodedInputStream newCodedInput() {
      // We trust CodedInputStream not to modify the bytes, or to give anyone
      // else access to them.
      return CodedInputStream.newInstance(
          bytes, getOffsetIntoBytes(), size(), true /* bufferIsImmutable */);
    }

    // =================================================================
    // Internal methods

    /**
     * Offset into {@code bytes[]} to use, non-zero for substrings.
     *
     * @return always 0 for this class
     */
    protected int getOffsetIntoBytes() {
      return 0;
    }
  }

  /**
   * This class is used to represent the substring of a {@link ByteString} over a
   * single byte array. In terms of the public API of {@link ByteString}, you end
   * up here by calling {@link ByteString#copyFrom(byte[])} followed by {@link
   * ByteString#substring(int, int)}.
   *
   * <p>This class contains most of the overhead involved in creating a substring
   * from a {@link LiteralByteString}.  The overhead involves some range-checking
   * and two extra fields.
   *
   * @author carlanton@google.com (Carl Haverl)
   */
  // Keep this class private to avoid deadlocks in classloading across threads as ByteString's
  // static initializer loads LiteralByteString and another thread loads BoundedByteString.
  private static final class BoundedByteString extends LiteralByteString {

    private final int bytesOffset;
    private final int bytesLength;

    /**
     * Creates a {@code BoundedByteString} backed by the sub-range of given array,
     * without copying.
     *
     * @param bytes  array to wrap
     * @param offset index to first byte to use in bytes
     * @param length number of bytes to use from bytes
     * @throws IllegalArgumentException if {@code offset < 0}, {@code length < 0},
     *                                  or if {@code offset + length >
     *                                  bytes.length}.
     */
    BoundedByteString(byte[] bytes, int offset, int length) {
      super(bytes);
      checkRange(offset, offset + length, bytes.length);

      this.bytesOffset = offset;
      this.bytesLength = length;
    }

    /**
     * Gets the byte at the given index.
     * Throws {@link ArrayIndexOutOfBoundsException}
     * for backwards-compatibility reasons although it would more properly be
     * {@link IndexOutOfBoundsException}.
     *
     * @param index index of byte
     * @return the value
     * @throws ArrayIndexOutOfBoundsException {@code index} is < 0 or >= size
     */
    @Override
    public byte byteAt(int index) {
      // We must check the index ourselves as we cannot rely on Java array index
      // checking for substrings.
      checkIndex(index, size());
      return bytes[bytesOffset + index];
    }

    @Override
    public int size() {
      return bytesLength;
    }

    @Override
    protected int getOffsetIntoBytes() {
      return bytesOffset;
    }

    // =================================================================
    // ByteString -> byte[]

    @Override
    protected void copyToInternal(byte[] target, int sourceOffset, int targetOffset,
        int numberToCopy) {
      System.arraycopy(bytes, getOffsetIntoBytes() + sourceOffset, target,
          targetOffset, numberToCopy);
    }

    // =================================================================
    // Serializable

    private static final long serialVersionUID = 1L;

    Object writeReplace() {
      return ByteString.wrap(toByteArray());
    }

    private void readObject(@SuppressWarnings("unused") ObjectInputStream in) throws IOException {
      throw new InvalidObjectException(
          "BoundedByteStream instances are not to be serialized directly");
    }
  }
}
