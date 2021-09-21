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

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.fail;

import com.google.protobuf.DescriptorProtos.DescriptorProto;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.FilterInputStream;
import java.io.IOException;
import java.io.InputStream;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/**
 * Tests the exceptions thrown when parsing from a stream. The methods on the {@link Parser}
 * interface are specified to only throw {@link InvalidProtocolBufferException}. But we really want
 * to distinguish between invalid protos vs. actual I/O errors (like failures reading from a socket,
 * etc.). So, when we're not using the parser directly, an {@link IOException} should be thrown
 * where appropriate, instead of always an {@link InvalidProtocolBufferException}.
 *
 * @author jh@squareup.com (Joshua Humphries)
 */
@RunWith(JUnit4.class)
public class ParseExceptionsTest {

  private interface ParseTester {
    DescriptorProto parse(InputStream in) throws IOException;
  }

  private byte[] serializedProto;

  private void setup() {
    serializedProto = DescriptorProto.getDescriptor().toProto().toByteArray();
  }

  private void setupDelimited() {
    ByteArrayOutputStream bos = new ByteArrayOutputStream();
    try {
      DescriptorProto.getDescriptor().toProto().writeDelimitedTo(bos);
    } catch (IOException e) {
      fail("Exception not expected: " + e);
    }
    serializedProto = bos.toByteArray();
  }

  @Test
  public void message_parseFrom_InputStream() {
    setup();
    verifyExceptions(
        new ParseTester() {
          @Override
          public DescriptorProto parse(InputStream in) throws IOException {
            return DescriptorProto.parseFrom(in);
          }
        });
  }

  @Test
  public void message_parseFrom_InputStreamAndExtensionRegistry() {
    setup();
    verifyExceptions(
        new ParseTester() {
          @Override
          public DescriptorProto parse(InputStream in) throws IOException {
            return DescriptorProto.parseFrom(in, ExtensionRegistry.newInstance());
          }
        });
  }

  @Test
  public void message_parseFrom_CodedInputStream() {
    setup();
    verifyExceptions(
        new ParseTester() {
          @Override
          public DescriptorProto parse(InputStream in) throws IOException {
            return DescriptorProto.parseFrom(CodedInputStream.newInstance(in));
          }
        });
  }

  @Test
  public void message_parseFrom_CodedInputStreamAndExtensionRegistry() {
    setup();
    verifyExceptions(
        new ParseTester() {
          @Override
          public DescriptorProto parse(InputStream in) throws IOException {
            return DescriptorProto.parseFrom(
                CodedInputStream.newInstance(in), ExtensionRegistry.newInstance());
          }
        });
  }

  @Test
  public void message_parseDelimitedFrom_InputStream() {
    setupDelimited();
    verifyExceptions(
        new ParseTester() {
          @Override
          public DescriptorProto parse(InputStream in) throws IOException {
            return DescriptorProto.parseDelimitedFrom(in);
          }
        });
  }

  @Test
  public void message_parseDelimitedFrom_InputStreamAndExtensionRegistry() {
    setupDelimited();
    verifyExceptions(
        new ParseTester() {
          @Override
          public DescriptorProto parse(InputStream in) throws IOException {
            return DescriptorProto.parseDelimitedFrom(in, ExtensionRegistry.newInstance());
          }
        });
  }

  @Test
  public void messageBuilder_mergeFrom_InputStream() {
    setup();
    verifyExceptions(
        new ParseTester() {
          @Override
          public DescriptorProto parse(InputStream in) throws IOException {
            return DescriptorProto.newBuilder().mergeFrom(in).build();
          }
        });
  }

  @Test
  public void messageBuilder_mergeFrom_InputStreamAndExtensionRegistry() {
    setup();
    verifyExceptions(
        new ParseTester() {
          @Override
          public DescriptorProto parse(InputStream in) throws IOException {
            return DescriptorProto.newBuilder()
                .mergeFrom(in, ExtensionRegistry.newInstance())
                .build();
          }
        });
  }

  @Test
  public void messageBuilder_mergeFrom_CodedInputStream() {
    setup();
    verifyExceptions(
        new ParseTester() {
          @Override
          public DescriptorProto parse(InputStream in) throws IOException {
            return DescriptorProto.newBuilder().mergeFrom(CodedInputStream.newInstance(in)).build();
          }
        });
  }

  @Test
  public void messageBuilder_mergeFrom_CodedInputStreamAndExtensionRegistry() {
    setup();
    verifyExceptions(
        new ParseTester() {
          @Override
          public DescriptorProto parse(InputStream in) throws IOException {
            return DescriptorProto.newBuilder()
                .mergeFrom(CodedInputStream.newInstance(in), ExtensionRegistry.newInstance())
                .build();
          }
        });
  }

  @Test
  public void messageBuilder_mergeDelimitedFrom_InputStream() {
    setupDelimited();
    verifyExceptions(
        new ParseTester() {
          @Override
          public DescriptorProto parse(InputStream in) throws IOException {
            DescriptorProto.Builder builder = DescriptorProto.newBuilder();
            builder.mergeDelimitedFrom(in);
            return builder.build();
          }
        });
  }

  @Test
  public void messageBuilder_mergeDelimitedFrom_InputStream_malformed() throws Exception {
    byte[] body = new byte[80];
    CodedOutputStream cos = CodedOutputStream.newInstance(body);
    cos.writeRawVarint32(90); // Greater than bytes in stream
    cos.writeTag(DescriptorProto.ENUM_TYPE_FIELD_NUMBER, WireFormat.WIRETYPE_LENGTH_DELIMITED);
    cos.writeRawVarint32(98); // Nested message with size larger than parent
    cos.writeTag(1000, WireFormat.WIRETYPE_LENGTH_DELIMITED);
    cos.writeRawVarint32(100); // Unknown field with size larger than parent
    ByteArrayInputStream bais = new ByteArrayInputStream(body);
    try {
      DescriptorProto.parseDelimitedFrom(bais);
      fail();
    } catch (InvalidProtocolBufferException expected) {
    }
  }

  @Test
  public void messageBuilder_mergeDelimitedFrom_InputStreamAndExtensionRegistry() {
    setupDelimited();
    verifyExceptions(
        new ParseTester() {
          @Override
          public DescriptorProto parse(InputStream in) throws IOException {
            DescriptorProto.Builder builder = DescriptorProto.newBuilder();
            builder.mergeDelimitedFrom(in, ExtensionRegistry.newInstance());
            return builder.build();
          }
        });
  }

  private void verifyExceptions(ParseTester parseTester) {
    // No exception
    try {
      assertThat(parseTester.parse(new ByteArrayInputStream(serializedProto)))
          .isEqualTo(DescriptorProto.getDescriptor().toProto());
    } catch (IOException e) {
      fail("No exception expected: " + e);
    }

    // IOException
    try {
      // using a "broken" stream that will throw part-way through reading the message
      parseTester.parse(broken(new ByteArrayInputStream(serializedProto)));
      fail("IOException expected but not thrown");
    } catch (IOException e) {
      assertThat(e).isNotInstanceOf(InvalidProtocolBufferException.class);
    }

    // InvalidProtocolBufferException
    try {
      // make the serialized proto invalid
      for (int i = 0; i < 50; i++) {
        serializedProto[i] = -1;
      }
      parseTester.parse(new ByteArrayInputStream(serializedProto));
      fail("InvalidProtocolBufferException expected but not thrown");
    } catch (IOException e) {
      assertThat(e).isInstanceOf(InvalidProtocolBufferException.class);
    }
  }

  private InputStream broken(InputStream i) {
    return new FilterInputStream(i) {
      int count = 0;

      @Override
      public int read() throws IOException {
        if (count++ >= 50) {
          throw new IOException("I'm broken!");
        }
        return super.read();
      }

      @Override
      public int read(byte[] b, int off, int len) throws IOException {
        if ((count += len) >= 50) {
          throw new IOException("I'm broken!");
        }
        return super.read(b, off, len);
      }
    };
  }
}
