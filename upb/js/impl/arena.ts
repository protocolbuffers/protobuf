/**
 * @license
 * Copyright 2023 Google LLC
 *
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file or at
 * https://developers.google.com/open-source/licenses/bsd
 */

/**
 * @fileoverview Defines the Arena class that wraps a upb_Arena.
 */

import * as decl from '../upb';

import * as upbBits from './upb_bits';
import {WasmExports} from './wasm_exports';
import * as wasmMem from './wasm_mem';

const utf8Encoder = new TextEncoder();

/**
 * An Arena object wraps a upb_Arena and will free its memory when freed.
 * We also use the arena object to store a few objects that we want to cache
 * long-term that store their data in the arena.
 */
export class Arena implements decl.Arena {
  utf8Arg: Utf8Arg;

  /**
   * We make the constructor private because we want all new instances to be
   * created with newArena() below, so that they are added to the finalizer
   * registry.
   */
  private constructor(
      readonly ptr: wasmMem.Pointer, readonly wasmExports: WasmExports) {
    this.utf8Arg = new Utf8Arg(this);
  }

  malloc(size: number): wasmMem.Pointer {
    const mem = this.wasmExports.upb_Arena_Malloc(this.ptr, size);
    if (!mem) throw new Error('cannot allocate from upb heap');
    return mem;
  }

  /*
   * Converts the given JavaScript string to UTF-8 data on the WASM heap.
   * The UTF-8 data is allocated from the arena and will have the same
   * lifetime as the arena itself
   */
  newUtf8Buffer(str: string): [wasmMem.Pointer, number] {
    // We can't predict how long the string will be once converted to UTF-8.
    // It can be between str.length and str.length*3 bytes, depending on what
    // kinds of characters are in the string.  So we don't know how many bytes
    // to allocate for it.  If we always allocated the max number, that would
    // be wasteful.
    //
    // Our solution is to try using the entire remaining arena buffer.  It may
    // be more than we need but once we figure out the true length, we can
    // allocate just the right amount of data for it.
    const [ptr, size] = upbBits.arenaBuf(this.ptr);
    const u8arr = wasmMem.u8Array(ptr, size);
    const result = utf8Encoder.encodeInto(str, u8arr);

    if (result.read !== str.length) {
      // The arena buffer didn't have enough space.  We need to call into WASM
      // to allocate another arena block.
      return this.newUtf8BufferSlow(str);
    }

    // Happy case, the arena had enough data.  We can allocate the data without
    // calling into WASM.
    const utf8Ptr = upbBits.arenaMalloc(this.ptr, result.written!);
    return [utf8Ptr, result.written!];
  }

  private newUtf8BufferSlow(str: string): [wasmMem.Pointer, number] {
    // We have to call into WASM to allocate another arena block.  Make sure it
    // is at least str.length * 3 (the worst case).
    const maxLength = str.length * 3;
    const ptr = this.wasmExports.upb_Arena_Malloc(this.ptr, maxLength);
    if (!ptr) throw new Error('cannot allocate from upb heap');
    const u8arr = wasmMem.u8Array(ptr, maxLength);
    const result = utf8Encoder.encodeInto(str, u8arr);
    this.wasmExports.upb_Arena_ShrinkLast(
        this.ptr, ptr, maxLength, result.written!);
    return [ptr, result.written!];
  }

  static newArena(wasmExports: WasmExports): Arena {
    const arenaPtr = wasmExports.upb_Arena_New();
    const ret = new Arena(arenaPtr, wasmExports);
    finalizers.register(ret, new ArenaCleanupInfo(wasmExports, arenaPtr));
    return ret;
  }
}

// We must set up a finalizer for each arena so that its memory is released from
// the WASM heap when it is no longer referenced.
class ArenaCleanupInfo {
  constructor(
      readonly wasmExports: WasmExports, readonly arenaPtr: wasmMem.Pointer) {}
}

const finalizers = new FinalizationRegistry<ArenaCleanupInfo>(
    (heldValue: ArenaCleanupInfo) => {
      heldValue.wasmExports.upb_Arena_Free(heldValue.arenaPtr);
    });

class Utf8Arg {
  ptr: wasmMem.Pointer;
  size: number;
  capacity: number;

  constructor(private readonly arena: Arena) {
    this.size = 0;
    this.capacity = 128;
    this.ptr = arena.malloc(this.capacity);
  }

  set(str: string) {
    while (1) {
      // Double our buffer size until the buffer is big enough.
      const ret =
          utf8Encoder.encodeInto(str, wasmMem.u8Array(this.ptr, this.capacity));
      if (ret.read! >= str.length) {
        this.size = ret.written!;
        return;
      }
      // We need to reallocate our buffer.
      this.capacity *= 2;
      this.ptr = this.arena.malloc(this.capacity);
    }
  }
}
