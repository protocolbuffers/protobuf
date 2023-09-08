/**
 * @license
 * Copyright 2023 Google LLC
 *
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file or at
 * https://developers.google.com/open-source/licenses/bsd
 */

import * as decl from '../upb';

import {Arena} from './arena';
import {MiniTable} from './mini_table';
import * as wasmMem from './wasm_mem';

export class InternalMessage implements decl.InternalMessage {
  private constructor(readonly ptr: wasmMem.Pointer, readonly arena: Arena) {}

  static wrap(ptr: wasmMem.Pointer, arena: Arena): InternalMessage {
    return new InternalMessage(ptr, arena);
  }

  static create(miniTable: MiniTable, arena: Arena): InternalMessage {
    return new InternalMessage(
        arena.wasmExports.upb_Message_New(miniTable.ptr, arena.ptr), arena);
  }
}
