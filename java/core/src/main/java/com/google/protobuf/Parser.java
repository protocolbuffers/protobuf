// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

import java.io.InputStream;
import java.nio.ByteBuffer;

/**
 * Abstract interface for parsing Protocol Messages.
 *
 * <p>The implementation should be stateless and thread-safe.
 *
 * <p>All methods may throw {@link InvalidProtocolBufferException}. In the event of invalid data,
 * like an encoding error, the cause of the thrown exception will be {@code null}. However, if an
 * I/O problem occurs, an exception is thrown with an {@link java.io.IOException} cause.
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
   * <p>Note: The caller should call {@link CodedInputStream#checkLastTagWas(int)} after calling
   * this to verify that the last tag seen was the appropriate end-group tag, or zero for EOF.
   */
  public MessageType parseFrom(CodedInputStream input) throws InvalidProtocolBufferException;

  /**
   * Like {@link #parseFrom(CodedInputStream)}, but also parses extensions. The extensions that you
   * want to be able to parse must be registered in {@code extensionRegistry}. Extensions not in the
   * registry will be treated as unknown fields.
   */
  public MessageType parseFrom(CodedInputStream input, ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException;

  /**
   * Like {@link #parseFrom(CodedInputStream)}, but does not throw an exception if the message is
   * missing required fields. Instead, a partial message is returned.
   */
  public MessageType parsePartialFrom(CodedInputStream input) throws InvalidProtocolBufferException;

  /**
   * Like {@link #parseFrom(CodedInputStream input, ExtensionRegistryLite)}, but does not throw an
   * exception if the message is missing required fields. Instead, a partial message is returned.
   */
  public MessageType parsePartialFrom(
      CodedInputStream input, ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException;

  // ---------------------------------------------------------------
  // Convenience methods.

  /**
   * Parses {@code data} as a message of {@code MessageType}. This is just a small wrapper around
   * {@link #parseFrom(CodedInputStream)}.
   */
  public MessageType parseFrom(ByteBuffer data) throws InvalidProtocolBufferException;

  /**
   * Parses {@code data} as a message of {@code MessageType}. This is just a small wrapper around
   * {@link #parseFrom(CodedInputStream, ExtensionRegistryLite)}.
   */
  public MessageType parseFrom(ByteBuffer data, ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException;
  /**
   * Parses {@code data} as a message of {@code MessageType}. This is just a small wrapper around
   * {@link #parseFrom(CodedInputStream)}.
   */
  public MessageType parseFrom(ByteString data) throws InvalidProtocolBufferException;

  /**
   * Parses {@code data} as a message of {@code MessageType}. This is just a small wrapper around
   * {@link #parseFrom(CodedInputStream, ExtensionRegistryLite)}.
   */
  public MessageType parseFrom(ByteString data, ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException;

  /**
   * Like {@link #parseFrom(ByteString)}, but does not throw an exception if the message is missing
   * required fields. Instead, a partial message is returned.
   */
  public MessageType parsePartialFrom(ByteString data) throws InvalidProtocolBufferException;

  /**
   * Like {@link #parseFrom(ByteString, ExtensionRegistryLite)}, but does not throw an exception if
   * the message is missing required fields. Instead, a partial message is returned.
   */
  public MessageType parsePartialFrom(ByteString data, ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException;

  /**
   * Parses {@code data} as a message of {@code MessageType}. This is just a small wrapper around
   * {@link #parseFrom(CodedInputStream)}.
   */
  public MessageType parseFrom(byte[] data, int off, int len) throws InvalidProtocolBufferException;

  /**
   * Parses {@code data} as a message of {@code MessageType}. This is just a small wrapper around
   * {@link #parseFrom(CodedInputStream, ExtensionRegistryLite)}.
   */
  public MessageType parseFrom(
      byte[] data, int off, int len, ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException;

  /**
   * Parses {@code data} as a message of {@code MessageType}. This is just a small wrapper around
   * {@link #parseFrom(CodedInputStream)}.
   */
  public MessageType parseFrom(byte[] data) throws InvalidProtocolBufferException;

  /**
   * Parses {@code data} as a message of {@code MessageType}. This is just a small wrapper around
   * {@link #parseFrom(CodedInputStream, ExtensionRegistryLite)}.
   */
  public MessageType parseFrom(byte[] data, ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException;

  /**
   * Like {@link #parseFrom(byte[], int, int)}, but does not throw an exception if the message is
   * missing required fields. Instead, a partial message is returned.
   */
  public MessageType parsePartialFrom(byte[] data, int off, int len)
      throws InvalidProtocolBufferException;

  /**
   * Like {@link #parseFrom(ByteString, ExtensionRegistryLite)}, but does not throw an exception if
   * the message is missing required fields. Instead, a partial message is returned.
   */
  public MessageType parsePartialFrom(
      byte[] data, int off, int len, ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException;

  /**
   * Like {@link #parseFrom(byte[])}, but does not throw an exception if the message is missing
   * required fields. Instead, a partial message is returned.
   */
  public MessageType parsePartialFrom(byte[] data) throws InvalidProtocolBufferException;

  /**
   * Like {@link #parseFrom(byte[], ExtensionRegistryLite)}, but does not throw an exception if the
   * message is missing required fields. Instead, a partial message is returned.
   */
  public MessageType parsePartialFrom(byte[] data, ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException;

  /**
   * Parse a message of {@code MessageType} from {@code input}. This is just a small wrapper around
   * {@link #parseFrom(CodedInputStream)}. Note that this method always reads the <i>entire</i>
   * input (unless it throws an exception). If you want it to stop earlier, you will need to wrap
   * your input in some wrapper stream that limits reading. Or, use {@link
   * MessageLite#writeDelimitedTo(java.io.OutputStream)} to write your message and {@link
   * #parseDelimitedFrom(InputStream)} to read it.
   *
   * <p>Despite usually reading the entire input, this does not close the stream.
   */
  public MessageType parseFrom(InputStream input) throws InvalidProtocolBufferException;

  /**
   * Parses a message of {@code MessageType} from {@code input}. This is just a small wrapper around
   * {@link #parseFrom(CodedInputStream, ExtensionRegistryLite)}.
   */
  public MessageType parseFrom(InputStream input, ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException;

  /**
   * Like {@link #parseFrom(InputStream)}, but does not throw an exception if the message is missing
   * required fields. Instead, a partial message is returned.
   */
  public MessageType parsePartialFrom(InputStream input) throws InvalidProtocolBufferException;

  /**
   * Like {@link #parseFrom(InputStream, ExtensionRegistryLite)}, but does not throw an exception if
   * the message is missing required fields. Instead, a partial message is returned.
   */
  public MessageType parsePartialFrom(InputStream input, ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException;

  /**
   * Like {@link #parseFrom(InputStream)}, but does not read until EOF. Instead, the size of message
   * (encoded as a varint) is read first, then the message data. Use {@link
   * MessageLite#writeDelimitedTo(java.io.OutputStream)} to write messages in this format.
   *
   * @return Parsed message if successful, or null if the stream is at EOF when the method starts.
   *     Any other error (including reaching EOF during parsing) will cause an exception to be
   *     thrown.
   */
  public MessageType parseDelimitedFrom(InputStream input) throws InvalidProtocolBufferException;

  /** Like {@link #parseDelimitedFrom(InputStream)} but supporting extensions. */
  public MessageType parseDelimitedFrom(InputStream input, ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException;

  /**
   * Like {@link #parseDelimitedFrom(InputStream)}, but does not throw an exception if the message
   * is missing required fields. Instead, a partial message is returned.
   */
  public MessageType parsePartialDelimitedFrom(InputStream input)
      throws InvalidProtocolBufferException;

  /**
   * Like {@link #parseDelimitedFrom(InputStream, ExtensionRegistryLite)}, but does not throw an
   * exception if the message is missing required fields. Instead, a partial message is returned.
   */
  public MessageType parsePartialDelimitedFrom(
      InputStream input, ExtensionRegistryLite extensionRegistry)
      throws InvalidProtocolBufferException;
}
