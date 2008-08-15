// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.
// http://code.google.com/p/protobuf/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
using System.IO;

namespace Google.ProtocolBuffers {
  /// <summary>
  /// Thrown when a protocol message being parsed is invalid in some way,
  /// e.g. it contains a malformed varint or a negative byte length.
  /// </summary>
  public sealed class InvalidProtocolBufferException : IOException {

    internal InvalidProtocolBufferException(string message)
      : base(message) {
    }

    internal static InvalidProtocolBufferException TruncatedMessage() {
      return new InvalidProtocolBufferException(
        "While parsing a protocol message, the input ended unexpectedly " +
        "in the middle of a field.  This could mean either than the " +
        "input has been truncated or that an embedded message " +
        "misreported its own length.");
    }

    internal static InvalidProtocolBufferException NegativeSize() {
      return new InvalidProtocolBufferException(
        "CodedInputStream encountered an embedded string or message " +
        "which claimed to have negative size.");
    }

    public static InvalidProtocolBufferException MalformedVarint() {
      return new InvalidProtocolBufferException(
        "CodedInputStream encountered a malformed varint.");
    }

    internal static InvalidProtocolBufferException InvalidTag() {
      return new InvalidProtocolBufferException(
        "Protocol message contained an invalid tag (zero).");
    }

    internal static InvalidProtocolBufferException InvalidEndTag() {
      return new InvalidProtocolBufferException(
        "Protocol message end-group tag did not match expected tag.");
    }

    internal static InvalidProtocolBufferException InvalidWireType() {
      return new InvalidProtocolBufferException(
        "Protocol message tag had invalid wire type.");
    }

    internal static InvalidProtocolBufferException RecursionLimitExceeded() {
      return new InvalidProtocolBufferException(
        "Protocol message had too many levels of nesting.  May be malicious.  " +
        "Use CodedInputStream.SetRecursionLimit() to increase the depth limit.");
    }

    internal static InvalidProtocolBufferException SizeLimitExceeded() {
      return new InvalidProtocolBufferException(
        "Protocol message was too large.  May be malicious.  " +
        "Use CodedInputStream.SetSizeLimit() to increase the size limit.");
    }
  }
}
