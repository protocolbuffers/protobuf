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

package com.google.protobuf.kotlin

import com.google.protobuf.ExtensionLite
import com.google.protobuf.GeneratedMessageV3

/** Sets the current value of the proto extension in this builder. */
operator fun <
  M : GeneratedMessageV3.ExtendableMessage<M>,
  B : GeneratedMessageV3.ExtendableBuilder<M, B>,
  T : Any> B.set(extension: ExtensionLite<M, T>, value: T) {
  setExtension(extension, value)
}

/** Gets the current value of the proto extension. */
operator fun <
  M : GeneratedMessageV3.ExtendableMessage<M>,
  MorBT : GeneratedMessageV3.ExtendableMessageOrBuilder<M>,
  T : Any> MorBT.get(extension: ExtensionLite<M, T>): T = getExtension(extension)

/** Returns true if the specified extension is set on this builder. */
operator fun <
  M : GeneratedMessageV3.ExtendableMessage<M>,
  MorBT : GeneratedMessageV3.ExtendableMessageOrBuilder<M>> MorBT.contains(
  extension: ExtensionLite<M, *>
): Boolean = hasExtension(extension)
