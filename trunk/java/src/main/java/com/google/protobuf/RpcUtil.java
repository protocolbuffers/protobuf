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

package com.google.protobuf;

/**
 * Grab-bag of utility functions useful when dealing with RPCs.
 *
 * @author kenton@google.com Kenton Varda
 */
public final class RpcUtil {
  private RpcUtil() {}

  /**
   * Take an {@code RcpCallabck<Message>} and convert it to an
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
   * Take an {@code RcpCallabck} accepting a specific message type and convert
   * it to an {@code RcpCallabck<Message>}.  The generalized callback will
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
