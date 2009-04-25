// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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

/**
 * Grab-bag of utility functions useful when dealing with RPCs.
 *
 * @author kenton@google.com Kenton Varda
 */
public final class RpcUtil {
  private RpcUtil() {}

  /**
   * Take an {@code RpcCallback<Message>} and convert it to an
   * {@code RpcCallback} accepting a specific message type.  This is always
   * type-safe (parameter type contravariance).
   */
  @SuppressWarnings("unchecked")
  public static <Type extends Message> RpcCallback<Type>
  specializeCallback(final RpcCallback<Message> originalCallback) {
    return (RpcCallback<Type>)originalCallback;
    // The above cast works, but only due to technical details of the Java
    // implementation.  A more theoretically correct -- but less efficient --
    // implementation would be as follows:
    //   return new RpcCallback<Type>() {
    //     public void run(Type parameter) {
    //       originalCallback.run(parameter);
    //     }
    //   };
  }

  /**
   * Take an {@code RpcCallback} accepting a specific message type and convert
   * it to an {@code RpcCallback<Message>}.  The generalized callback will
   * accept any message object which has the same descriptor, and will convert
   * it to the correct class before calling the original callback.  However,
   * if the generalized callback is given a message with a different descriptor,
   * an exception will be thrown.
   */
  public static <Type extends Message>
  RpcCallback<Message> generalizeCallback(
      final RpcCallback<Type> originalCallback,
      final Class<Type> originalClass,
      final Type defaultInstance) {
    return new RpcCallback<Message>() {
      public void run(Message parameter) {
        Type typedParameter;
        try {
          typedParameter = originalClass.cast(parameter);
        } catch (ClassCastException e) {
          typedParameter = copyAsType(defaultInstance, parameter);
        }
        originalCallback.run(typedParameter);
      }
    };
  }

  /**
   * Creates a new message of type "Type" which is a copy of "source".  "source"
   * must have the same descriptor but may be a different class (e.g.
   * DynamicMessage).
   */
  @SuppressWarnings("unchecked")
  private static <Type extends Message> Type copyAsType(
      Type typeDefaultInstance, Message source) {
    return (Type)typeDefaultInstance.newBuilderForType()
                                    .mergeFrom(source)
                                    .build();
  }

  /**
   * Creates a callback which can only be called once.  This may be useful for
   * security, when passing a callback to untrusted code:  most callbacks do
   * not expect to be called more than once, so doing so may expose bugs if it
   * is not prevented.
   */
  public static <ParameterType>
    RpcCallback<ParameterType> newOneTimeCallback(
      final RpcCallback<ParameterType> originalCallback) {
    return new RpcCallback<ParameterType>() {
      boolean alreadyCalled = false;
      public void run(ParameterType parameter) {
        synchronized(this) {
          if (alreadyCalled) {
            throw new AlreadyCalledException();
          }
          alreadyCalled = true;
        }

        originalCallback.run(parameter);
      }
    };
  }

  /**
   * Exception thrown when a one-time callback is called more than once.
   */
  public static final class AlreadyCalledException extends RuntimeException {
    public AlreadyCalledException() {
      super("This RpcCallback was already called and cannot be called " +
            "multiple times.");
    }
  }
}
