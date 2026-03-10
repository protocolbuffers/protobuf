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
}
