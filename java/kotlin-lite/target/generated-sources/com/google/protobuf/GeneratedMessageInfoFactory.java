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

/** A factory for message info that is generated into the message itself. */
@ExperimentalApi
class GeneratedMessageInfoFactory implements MessageInfoFactory {

  private static final GeneratedMessageInfoFactory instance = new GeneratedMessageInfoFactory();

  // Disallow construction - it's a singleton.
  private GeneratedMessageInfoFactory() {}

  public static GeneratedMessageInfoFactory getInstance() {
    return instance;
  }

  @Override
  public boolean isSupported(Class<?> messageType) {
    return GeneratedMessageLite.class.isAssignableFrom(messageType);
  }

  @Override
  public MessageInfo messageInfoFor(Class<?> messageType) {
    if (!GeneratedMessageLite.class.isAssignableFrom(messageType)) {
      throw new IllegalArgumentException("Unsupported message type: " + messageType.getName());
    }

    try {
      return (MessageInfo) GeneratedMessageLite.getDefaultInstance(
          messageType.asSubclass(GeneratedMessageLite.class))
          .buildMessageInfo();
    } catch (Exception e) {
      throw new RuntimeException("Unable to get message info for " + messageType.getName(), e);
    }
  }
}
