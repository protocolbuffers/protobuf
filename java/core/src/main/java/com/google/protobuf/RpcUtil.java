// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

/**
 * Grab-bag of utility functions useful when dealing with RPCs.
 *
 * @author kenton@google.com Kenton Varda
 */
public final class RpcUtil {
  private RpcUtil() {}

  /**
   * Take an {@code RpcCallback<Message>} and convert it to an {@code RpcCallback} accepting a
   * specific message type. This is always type-safe (parameter type contravariance).
   */
  @SuppressWarnings("unchecked")
  public static <Type extends Message> RpcCallback<Type> specializeCallback(
      final RpcCallback<Message> originalCallback) {
    return (RpcCallback<Type>) originalCallback;
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
   * Take an {@code RpcCallback} accepting a specific message type and convert it to an {@code
   * RpcCallback<Message>}. The generalized callback will accept any message object which has the
   * same descriptor, and will convert it to the correct class before calling the original callback.
   * However, if the generalized callback is given a message with a different descriptor, an
   * exception will be thrown.
   */
  public static <Type extends Message> RpcCallback<Message> generalizeCallback(
      final RpcCallback<Type> originalCallback,
      final Class<Type> originalClass,
      final Type defaultInstance) {
    return new RpcCallback<Message>() {
      @Override
      public void run(final Message parameter) {
        Type typedParameter;
        try {
          typedParameter = originalClass.cast(parameter);
        } catch (ClassCastException ignored) {
          typedParameter = copyAsType(defaultInstance, parameter);
        }
        originalCallback.run(typedParameter);
      }
    };
  }

  /**
   * Creates a new message of type "Type" which is a copy of "source". "source" must have the same
   * descriptor but may be a different class (e.g. DynamicMessage).
   */
  @SuppressWarnings("unchecked")
  private static <Type extends Message> Type copyAsType(
      final Type typeDefaultInstance, final Message source) {
    return (Type) typeDefaultInstance.newBuilderForType().mergeFrom(source).build();
  }

  /**
   * Creates a callback which can only be called once. This may be useful for security, when passing
   * a callback to untrusted code: most callbacks do not expect to be called more than once, so
   * doing so may expose bugs if it is not prevented.
   */
  public static <ParameterType> RpcCallback<ParameterType> newOneTimeCallback(
      final RpcCallback<ParameterType> originalCallback) {
    return new RpcCallback<ParameterType>() {
      private boolean alreadyCalled = false;

      @Override
      public void run(final ParameterType parameter) {
        synchronized (this) {
          if (alreadyCalled) {
            throw new AlreadyCalledException();
          }
          alreadyCalled = true;
        }

        originalCallback.run(parameter);
      }
    };
  }

  /** Exception thrown when a one-time callback is called more than once. */
  public static final class AlreadyCalledException extends RuntimeException {
    private static final long serialVersionUID = 5469741279507848266L;

    public AlreadyCalledException() {
      super("This RpcCallback was already called and cannot be called multiple times.");
    }
  }
}
