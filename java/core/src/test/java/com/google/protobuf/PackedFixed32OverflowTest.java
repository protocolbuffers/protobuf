package com.google.protobuf;

import static org.junit.Assert.assertThrows;

import com.google.protobuf.PackedFieldTestProto.TestAllTypes;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/**
 * Regression test for integer overflow in ArrayDecoders packed field decoders.
 *
 * <p>A packed repeated field whose varint length equals Integer.MAX_VALUE causes
 * {@code fieldLimit = position + packedDataByteSize} to overflow to a negative value,
 * bypassing the bounds guard and leading to an excessive allocation or OOM.
 *
 * <p>The fix validates {@code packedDataByteSize} against the remaining buffer size
 * <em>before</em> the addition, using overflow-safe subtraction:
 * {@code packedDataByteSize > data.length - position}.
 *
 * <p>After the fix, all methods must throw {@link InvalidProtocolBufferException}.
 */
@RunWith(JUnit4.class)
public class PackedFixed32OverflowTest {

  /** Builds a minimal message: tag for {@code fieldNumber} (wire type 2) + varint MAX_VALUE. */
  private static byte[] craftOverflowPayload(int fieldNumber) throws IOException {
    ByteArrayOutputStream buf = new ByteArrayOutputStream();
    // Encode tag: (fieldNumber << 3) | 2  as a varint
    int tag = (fieldNumber << 3) | WireFormat.WIRETYPE_LENGTH_DELIMITED;
    while (tag > 0x7F) {
      buf.write((tag & 0x7F) | 0x80);
      tag >>>= 7;
    }
    buf.write(tag);
    // Encode length = Integer.MAX_VALUE as a 5-byte varint
    buf.write(0xFF);
    buf.write(0xFF);
    buf.write(0xFF);
    buf.write(0xFF);
    buf.write(0x07);
    return buf.toByteArray();
  }

  @Test
  public void packedFixed32_overflowLength_throws() throws Exception {
    byte[] crafted = craftOverflowPayload(37); // repeated_fixed32
    assertThrows(InvalidProtocolBufferException.class, () -> TestAllTypes.parseFrom(crafted));
  }

  @Test
  public void packedFixed64_overflowLength_throws() throws Exception {
    byte[] crafted = craftOverflowPayload(38); // repeated_fixed64
    assertThrows(InvalidProtocolBufferException.class, () -> TestAllTypes.parseFrom(crafted));
  }

  @Test
  public void packedFloat_overflowLength_throws() throws Exception {
    byte[] crafted = craftOverflowPayload(41); // repeated_float
    assertThrows(InvalidProtocolBufferException.class, () -> TestAllTypes.parseFrom(crafted));
  }

  @Test
  public void packedDouble_overflowLength_throws() throws Exception {
    byte[] crafted = craftOverflowPayload(42); // repeated_double
    assertThrows(InvalidProtocolBufferException.class, () -> TestAllTypes.parseFrom(crafted));
  }

  @Test
  public void packedVarint32_overflowLength_throws() throws Exception {
    byte[] crafted = craftOverflowPayload(31); // repeated_int32
    assertThrows(InvalidProtocolBufferException.class, () -> TestAllTypes.parseFrom(crafted));
  }

  @Test
  public void packedVarint64_overflowLength_throws() throws Exception {
    byte[] crafted = craftOverflowPayload(32); // repeated_int64
    assertThrows(InvalidProtocolBufferException.class, () -> TestAllTypes.parseFrom(crafted));
  }

  @Test
  public void packedSInt32_overflowLength_throws() throws Exception {
    byte[] crafted = craftOverflowPayload(35); // repeated_sint32
    assertThrows(InvalidProtocolBufferException.class, () -> TestAllTypes.parseFrom(crafted));
  }

  @Test
  public void packedSInt64_overflowLength_throws() throws Exception {
    byte[] crafted = craftOverflowPayload(36); // repeated_sint64
    assertThrows(InvalidProtocolBufferException.class, () -> TestAllTypes.parseFrom(crafted));
  }

  @Test
  public void packedBool_overflowLength_throws() throws Exception {
    byte[] crafted = craftOverflowPayload(43); // repeated_bool
    assertThrows(InvalidProtocolBufferException.class, () -> TestAllTypes.parseFrom(crafted));
  }
}
