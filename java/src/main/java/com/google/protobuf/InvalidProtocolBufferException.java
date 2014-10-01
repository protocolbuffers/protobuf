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

/**
 * Thrown when a protocol message being parsed is invalid in some way,
 * e.g. it contains a malformed varint or a negative byte length.
 *
 * @author kenton@google.com Kenton Varda
 */
public class InvalidProtocolBufferException extends IOException {
  private static final long serialVersionUID = -1616151763072450476L;
  private MessageLite unfinishedMessage = null;

  public InvalidProtocolBufferException(final String description) {
    super(description);
  }

  /**
   * Attaches an unfinished message to the exception to support best-effort
   * parsing in {@code Parser} interface.
   *
   * @return this
   */
  public InvalidProtocolBufferException setUnfinishedMessage(
      MessageLite unfinishedMessage) {
    this.unfinishedMessage = unfinishedMessage;
    return this;
  }

  /**
   * Returns the unfinished message attached to the exception, or null if
   * no message is attached.
   */
  public MessageLite getUnfinishedMessage() {
    return unfinishedMessage;
  }

  static InvalidProtocolBufferException truncatedMessage() {
    return new InvalidProtocolBufferException(
      "While parsing a protocol message, the input ended unexpectedly " +
      "in the middle of a field.  This could mean either than the " +
      "input has been truncated or that an embedded message " +
      "misreported its own length.");
  }

  static InvalidProtocolBufferException negativeSize() {
    return new InvalidProtocolBufferException(
      "CodedInputStream encountered an embedded string or message " +
      "which claimed to have negative size.");
  }

  static InvalidProtocolBufferException malformedVarint() {
    return new InvalidProtocolBufferException(
      "CodedInputStream encountered a malformed varint.");
  }

  static InvalidProtocolBufferException invalidTag() {
    return new InvalidProtocolBufferException(
      "Protocol message contained an invalid tag (zero).");
  }

  static InvalidProtocolBufferException invalidEndTag() {
    return new InvalidProtocolBufferException(
      "Protocol message end-group tag did not match expected tag.");
  }

  static InvalidProtocolBufferException invalidWireType() {
    return new InvalidProtocolBufferException(
      "Protocol message tag had invalid wire type.");
  }

  static InvalidProtocolBufferException recursionLimitExceeded() {
    return new InvalidProtocolBufferException(
      "Protocol message had too many levels of nesting.  May be malicious.  " +
      "Use CodedInputStream.setRecursionLimit() to increase the depth limit.");
  }

  static InvalidProtocolBufferException sizeLimitExceeded() {
    return new InvalidProtocolBufferException(
      "Protocol message was too large.  May be malicious.  " +
      "Use CodedInputStream.setSizeLimit() to increase the size limit.");
  }

  static InvalidProtocolBufferException parseFailure() {
    return new InvalidProtocolBufferException("Failed to parse the message.");
  }

  static InvalidProtocolBufferException invalidUtf8() {
    return new InvalidProtocolBufferException("Protocol message had invalid UTF-8.");
  }
}
