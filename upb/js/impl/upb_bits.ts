/**
 * @license
 * Copyright 2023 Google LLC
 *
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file or at
 * https://developers.google.com/open-source/licenses/bsd
 */

/**
 * @fileoverview Functions for twiddling the raw bits and bytes of internal upb
 * data structures.
 *
 * This code is highly dependent on the internal implementation details of upb,
 * and therefore must be updated if upb's internals change.  Keeping this code
 * in sync is a significant cost, so we should keep this code to a minimum.  The
 * main purpose of this code is to reduce overhead by avoiding calls across the
 * WASM boundary for message getters and setters.
 */

export * from 'google3/third_party/upb/bits/typescript/arena';
export * from 'google3/third_party/upb/bits/typescript/array';
export * from 'google3/third_party/upb/bits/typescript/message';
export * from 'google3/third_party/upb/bits/typescript/mini_table';
export * from 'google3/third_party/upb/bits/typescript/mini_table_field';
export * from 'google3/third_party/upb/bits/typescript/presence';
export * from 'google3/third_party/upb/bits/typescript/string_view';
