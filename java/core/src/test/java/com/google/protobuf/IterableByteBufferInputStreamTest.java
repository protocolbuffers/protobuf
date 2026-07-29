package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;

import java.io.InputStream;
import java.nio.ByteBuffer;
import java.util.Arrays;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class IterableByteBufferInputStreamTest {

  /**
   * Tests that empty buffers are skipped when provided as input when constructing an {@link
   * IterableByteBufferInputStream}.
   */
  @Test
  public void testEmptyBuffers() throws Exception {
    byte[] expected = {1, 2, 3, 4};
    byte[] half1 = Arrays.copyOf(expected, expected.length / 2);
    byte[] half2 = Arrays.copyOfRange(expected, expected.length / 2, expected.length);

    try (InputStream in =
        new IterableByteBufferInputStream(
            Arrays.asList(
                ByteBuffer.wrap(new byte[0]),
                ByteBuffer.wrap(half1),
                ByteBuffer.wrap(new byte[0]),
                ByteBuffer.wrap(new byte[0]),
                ByteBuffer.wrap(half2),
                ByteBuffer.wrap(new byte[0])))) {
      for (byte expectedByte : expected) {
        assertThat(in.read()).isEqualTo(expectedByte);
      }
      assertThat(in.read()).isEqualTo(-1);
    }
  }

  @Test
  public void testHeapBuffers() throws Exception {
    byte[] expected = {1, 2, 3, 4, 5, 6};
    try (InputStream in =
        new IterableByteBufferInputStream(
            Arrays.asList(
                ByteBuffer.wrap(new byte[] {1, 2}),
                ByteBuffer.wrap(new byte[] {3, 4}),
                ByteBuffer.wrap(new byte[] {5, 6})))) {
      for (byte expectedByte : expected) {
        assertThat(in.read()).isEqualTo(expectedByte);
      }
      assertThat(in.read()).isEqualTo(-1);
    }
  }

  @Test
  public void testDirectBuffers() throws Exception {
    byte[] expected = {1, 2, 3, 4, 5, 6};
    ByteBuffer buf1 = ByteBuffer.allocateDirect(2);
    buf1.put(new byte[] {1, 2}).flip();
    ByteBuffer buf2 = ByteBuffer.allocateDirect(2);
    buf2.put(new byte[] {3, 4}).flip();
    ByteBuffer buf3 = ByteBuffer.allocateDirect(2);
    buf3.put(new byte[] {5, 6}).flip();

    try (InputStream in = new IterableByteBufferInputStream(Arrays.asList(buf1, buf2, buf3))) {
      for (byte expectedByte : expected) {
        assertThat(in.read()).isEqualTo(expectedByte);
      }
      assertThat(in.read()).isEqualTo(-1);
    }
  }

  @Test
  public void testMixedBuffers() throws Exception {
    byte[] expected = {1, 2, 3, 4, 5, 6};
    ByteBuffer buf1 = ByteBuffer.allocateDirect(2);
    buf1.put(new byte[] {1, 2}).flip();
    ByteBuffer buf2 = ByteBuffer.wrap(new byte[] {3, 4});
    ByteBuffer buf3 = ByteBuffer.allocateDirect(2);
    buf3.put(new byte[] {5, 6}).flip();

    try (InputStream in = new IterableByteBufferInputStream(Arrays.asList(buf1, buf2, buf3))) {
      for (byte expectedByte : expected) {
        assertThat(in.read()).isEqualTo(expectedByte);
      }
      assertThat(in.read()).isEqualTo(-1);
    }
  }

  @Test
  public void testReadArray() throws Exception {
    ByteBuffer buf1 = ByteBuffer.allocateDirect(3);
    buf1.put(new byte[] {1, 2, 3}).flip();
    ByteBuffer buf2 = ByteBuffer.wrap(new byte[] {4, 5, 6});

    try (InputStream in = new IterableByteBufferInputStream(Arrays.asList(buf1, buf2))) {
      byte[] output = new byte[6];
      int read = in.read(output, 0, 4);
      assertThat(read).isEqualTo(3);
      assertThat(output[0]).isEqualTo((byte) 1);
      assertThat(output[1]).isEqualTo((byte) 2);
      assertThat(output[2]).isEqualTo((byte) 3);

      read = in.read(output, 3, 3);
      assertThat(read).isEqualTo(3);
      assertThat(output[3]).isEqualTo((byte) 4);
      assertThat(output[4]).isEqualTo((byte) 5);
      assertThat(output[5]).isEqualTo((byte) 6);

      assertThat(in.read()).isEqualTo(-1);
    }
  }
}
