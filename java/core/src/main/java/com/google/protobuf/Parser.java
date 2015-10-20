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

import java.io.IOException;
import java.io.InputStream;

/**
 * Abstract interface for parsing Protocol Messages.
 *
 * The implementation should be stateless and thread-safe.
 *
 * <p>All methods may throw {@link InvalidProtocolBufferException}. In the event of invalid data,
 * like an encoding error, the cause of the thrown exception will be {@code null}. However, if an
 * I/O problem occurs, an exception is thrown with an {@link IOException} cause.
 *
 * @author liujisi@google.com (Pherl Liu)
 */
public interface Parser<MessageType> {

  // NB(jh): Other parts of the protobuf API that parse messages distinguish between an I/O problem
  // (like failure reading bytes from a socket) and invalid data (encoding error) via the type of
  // thrown exception. But it would be source-incompatible to make the methods in this interface do
  // so since they were originally spec'ed to only throw InvalidProtocolBufferException. So callers
  // must inspect the cause of the exception to distinguish these two cases.

  /**
   * Parses a message of {@code MessageType} from the input.
   *
   * <p>Note:  The caller should call
   * {@link CodedInputStream#checkLastTagWas(int)} after calling this to
   * verify that the last tag seen was the appropriate end-group tag,
   * or zero for EOF.
   */
  public MessageType parseFrom(CodedInputStream input)
      throws InvalidProtocolBufferException;

  /**
   * Like {@link #parseFrom(CodedInputStream)}, but also parses extensions.
   * The extensions that you want to be able to parse must be registered in
   * {@code extensionRegistry}. Extensions not in the registry will be treated
   * as unknown fields.
   */
  public MessageType parseFrom(CodedInputStream input,
                               ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException;

  /**
   * Like {@link #parseFrom(CodedInputStream)}, but does not throw an
   * exception if the message is missing required fields. Instead, a partial
   * message is returned.
   */
  public MessageType parsePartialFrom(CodedInputStream input)
      throws InvalidProtocolBufferException;

  /**
   * Like {@link #parseFrom(CodedInputStream input, ExtensionRegistryLite)},
   * but does not throw an exception if the message is missing required fields.
   * Instead, a partial message is returned.
   */
  public MessageType parsePartialFrom(CodedInputStream input,
                                      ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException;

  // ---------------------------------------------------------------
  // Convenience methods.

  /**
   * Parses {@code data} as a message of {@code MessageType}.
   * This is just a small wrapper around {@link #parseFrom(CodedInputStream)}.
   */
  public MessageType parseFrom(ByteString data)
      throws InvalidProtocolBufferException;

  /**
   * Parses {@code data} as a message of {@code MessageType}.
   * This is just a small wrapper around
   * {@link #parseFrom(CodedInputStream, ExtensionRegistryLite)}.
   */
  public MessageType parseFrom(ByteString data,
                               ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException;

  /**
   * Like {@link #parseFrom(ByteString)}, but does not throw an
   * exception if the message is missing required fields. Instead, a partial
   * message is returned.
   */
  public MessageType parsePartialFrom(ByteString data)
      throws InvalidProtocolBufferException;

  /**
   * Like {@link #parseFrom(ByteString, ExtensionRegistryLite)},
   * but does not throw an exception if the message is missing required fields.
   * Instead, a partial message is returned.
   */
  public MessageType parsePartialFrom(ByteString data,
                                      ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException;

  /**
   * Parses {@code data} as a message of {@code MessageType}.
   * This is just a small wrapper around {@link #parseFrom(CodedInputStream)}.
   */
  public MessageType parseFrom(byte[] data, int off, int len)
      throws InvalidProtocolBufferException;

  /**
   * Parses {@code data} as a message of {@code MessageType}.
   * This is just a small wrapper around
   * {@link #parseFrom(CodedInputStream, ExtensionRegistryLite)}.
   */
  public MessageType parseFrom(byte[] data, int off, int len,
                               ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException;

  /**
   * Parses {@code data} as a message of {@code MessageType}.
   * This is just a small wrapper around {@link #parseFrom(CodedInputStream)}.
   */
  public MessageType parseFrom(byte[] data)
      throws InvalidProtocolBufferException;

  /**
   * Parses {@code data} as a message of {@code MessageType}.
   * This is just a small wrapper around
   * {@link #parseFrom(CodedInputStream, ExtensionRegistryLite)}.
   */
  public MessageType parseFrom(byte[] data,
                               ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException;

  /**
   * Like {@link #parseFrom(byte[], int, int)}, but does not throw an
   * exception if the message is missing required fields. Instead, a partial
   * message is returned.
   */
  public MessageType parsePartialFrom(byte[] data, int off, int len)
      throws InvalidProtocolBufferException;

  /**
   * Like {@link #parseFrom(ByteString, ExtensionRegistryLite)},
   * but does not throw an exception if the message is missing required fields.
   * Instead, a partial message is returned.
   */
  public MessageType parsePartialFrom(byte[] data, int off, int len,
                                      ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException;

  /**
   * Like {@link #parseFrom(byte[])}, but does not throw an
   * exception if the message is missing required fields. Instead, a partial
   * message is returned.
   */
  public MessageType parsePartialFrom(byte[] data)
      throws InvalidProtocolBufferException;

  /**
   * Like {@link #parseFrom(byte[], ExtensionRegistryLite)},
   * but does not throw an exception if the message is missing required fields.
   * Instead, a partial message is returned.
   */
  public MessageType parsePartialFrom(byte[] data,
                                      ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException;

  /**
   * Parse a message of {@code MessageType} from {@code input}.
   * This is just a small wrapper around {@link #parseFrom(CodedInputStream)}.
   * Note that this method always reads the <i>entire</i> input (unless it
   * throws an exception).  If you want it to stop earlier, you will need to
   * wrap your input in some wrapper stream that limits reading.  Or, use
   * {@link MessageLite#writeDelimitedTo(java.io.OutputStream)} to write your
   * message and {@link #parseDelimitedFrom(InputStream)} to read it.
   * <p>
   * Despite usually reading the entire input, this does not close the stream.
   */
  public MessageType parseFrom(InputStream input)
      throws InvalidProtocolBufferException;

  /**
   * Parses a message of {@code MessageType} from {@code input}.
   * This is just a small wrapper around
   * {@link #parseFrom(CodedInputStream, ExtensionRegistryLite)}.
   */
  public MessageType parseFrom(InputStream input,
                               ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException;

  /**
   * Like {@link #parseFrom(InputStream)}, but does not throw an
   * exception if the message is missing required fields. Instead, a partial
   * message is returned.
   */
  public MessageType parsePartialFrom(InputStream input)
      throws InvalidProtocolBufferException;

  /**
   * Like {@link #parseFrom(InputStream, ExtensionRegistryLite)},
   * but does not throw an exception if the message is missing required fields.
   * Instead, a partial message is returned.
   */
  public MessageType parsePartialFrom(InputStream input,
                                      ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException;

  /**
   * Like {@link #parseFrom(InputStream)}, but does not read util EOF.
   * Instead, the size of message (encoded as a varint) is read first,
   * then the message data. Use
   * {@link MessageLite#writeDelimitedTo(java.io.OutputStream)} to write
   * messages in this format.
   *
   * @return Parsed message if successful, or null if the stream is at EOF when
   *         the method starts. Any other error (including reaching EOF during
   *         parsing) will cause an exception to be thrown.
   */
  public MessageType parseDelimitedFrom(InputStream input)
      throws InvalidProtocolBufferException;

  /**
   * Like {@link #parseDelimitedFrom(InputStream)} but supporting extensions.
   */
  public MessageType parseDelimitedFrom(InputStream input,
                                        ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException;

  /**
   * Like {@link #parseDelimitedFrom(InputStream)}, but does not throw an
   * exception if the message is missing required fields. Instead, a partial
   * message is returned.
   */
  public MessageType parsePartialDelimitedFrom(InputStream input)
      throws InvalidProtocolBufferException;

  /**
   * Like {@link #parseDelimitedFrom(InputStream, ExtensionRegistryLite)},
   * but does not throw an exception if the message is missing required fields.
   * Instead, a partial message is returned.
   */
  public MessageType parsePartialDelimitedFrom(
      InputStream input,
      ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException;
}
