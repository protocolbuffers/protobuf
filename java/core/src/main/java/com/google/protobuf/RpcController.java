// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

package com.google.protobuf;

/**
 * An {@code RpcController} mediates a single method call. The primary purpose of the controller is
 * to provide a way to manipulate settings specific to the RPC implementation and to find out about
 * RPC-level errors.
 *
 * <p>Starting with version 2.3.0, RPC implementations should not try to build on this, but should
 * instead provide code generator plugins which generate code specific to the particular RPC
 * implementation. This way the generated code can be more appropriate for the implementation in use
 * and can avoid unnecessary layers of indirection.
 *
 * <p>The methods provided by the {@code RpcController} interface are intended to be a "least common
 * denominator" set of features which we expect all implementations to support. Specific
 * implementations may provide more advanced features (e.g. deadline propagation).
 *
 * @author kenton@google.com Kenton Varda
 */
public interface RpcController {
  // -----------------------------------------------------------------
  // These calls may be made from the client side only.  Their results
  // are undefined on the server side (may throw RuntimeExceptions).

  /**
   * Resets the RpcController to its initial state so that it may be reused in a new call. This can
   * be called from the client side only. It must not be called while an RPC is in progress.
   */
  void reset();

  /**
   * After a call has finished, returns true if the call failed. The possible reasons for failure
   * depend on the RPC implementation. {@code failed()} most only be called on the client side, and
   * must not be called before a call has finished.
   */
  boolean failed();

  /** If {@code failed()} is {@code true}, returns a human-readable description of the error. */
  String errorText();

  /**
   * Advises the RPC system that the caller desires that the RPC call be canceled. The RPC system
   * may cancel it immediately, may wait awhile and then cancel it, or may not even cancel the call
   * at all. If the call is canceled, the "done" callback will still be called and the RpcController
   * will indicate that the call failed at that time.
   */
  void startCancel();

  // -----------------------------------------------------------------
  // These calls may be made from the server side only.  Their results
  // are undefined on the client side (may throw RuntimeExceptions).

  /**
   * Causes {@code failed()} to return true on the client side. {@code reason} will be incorporated
   * into the message returned by {@code errorText()}. If you find you need to return
   * machine-readable information about failures, you should incorporate it into your response
   * protocol buffer and should NOT call {@code setFailed()}.
   */
  void setFailed(String reason);

  /**
   * If {@code true}, indicates that the client canceled the RPC, so the server may as well give up
   * on replying to it. This method must be called on the server side only. The server should still
   * call the final "done" callback.
   */
  boolean isCanceled();

  /**
   * Asks that the given callback be called when the RPC is canceled. The parameter passed to the
   * callback will always be {@code null}. The callback will always be called exactly once. If the
   * RPC completes without being canceled, the callback will be called after completion. If the RPC
   * has already been canceled when NotifyOnCancel() is called, the callback will be called
   * immediately.
   *
   * <p>{@code notifyOnCancel()} must be called no more than once per request. It must be called on
   * the server side only.
   */
  void notifyOnCancel(RpcCallback<Object> callback);
}
