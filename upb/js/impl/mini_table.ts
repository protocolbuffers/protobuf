/**
 * @license
 * Copyright 2023 Google LLC
 *
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file or at
 * https://developers.google.com/open-source/licenses/bsd
 */

/**
 * @fileoverview MiniTable types, which are wrappers around upb_MiniTable*.
 */

import {Arena} from './arena';
import * as wasmMem from './wasm_mem';

/** A wrapper around upb_MiniTable*. */
export class MiniTable {
  ptr: wasmMem.Pointer;

  constructor(miniDescriptor: string, private readonly arena: Arena) {
    const arg = this.arena.utf8Arg;
    arg.set(miniDescriptor);
    this.ptr = this.arena.wasmExports.upb_MiniTable_Build(
        arg.ptr, arg.size, arena.ptr, 0);
    // TODO(b/276799361): surface upb error message.
    if (this.ptr === 0) throw new Error('cannot build MiniTable');
  }
}

/** A wrapper around upb_MiniTableEnum*. */
export class MiniTableEnum {
  ptr: wasmMem.Pointer;

  constructor(miniDescriptor: string, private readonly arena: Arena) {
    const arg = this.arena.utf8Arg;
    arg.set(miniDescriptor);
    this.ptr = this.arena.wasmExports.upb_MiniTableEnum_Build(
        arg.ptr, arg.size, arena.ptr, 0);
    // TODO(b/276799361): surface upb error message.
    if (this.ptr === 0) throw new Error('cannot build MiniTableEnum');
  }
}

/** A wrapper around upb_MiniTableExtension*. */
export class MiniTableExtension {
  ptr: wasmMem.Pointer;

  constructor(miniDescriptor: string, private readonly arena: Arena) {
    const arg = this.arena.utf8Arg;
    arg.set(miniDescriptor);
    this.ptr = this.arena.wasmExports.upb_MiniTableExtension_Build(
        arg.ptr, arg.size, 0, arena.ptr, 0);
    // TODO(b/276799361): surface upb error message.
    if (this.ptr === 0) throw new Error('cannot build MiniTableExtension');
  }
}
