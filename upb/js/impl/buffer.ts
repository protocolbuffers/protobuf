/**
 * @license
 * Copyright 2023 Google LLC
 *
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file or at
 * https://developers.google.com/open-source/licenses/bsd
 */

/**
 * @fileoverview Defines the Buffer class, which is a growable buffer of data on
 * the WASM heap.
 */

import {Arena} from './arena';
import * as wasmMem from './wasm_mem';

// Expand this list as buffers of different types are needed.
type TypedArray = Uint8Array;
type Constructor<T> = Function&{prototype: T};
type TypedArrayConstructor = Constructor<TypedArray>;

type TypeInfo = [lg2: number, factory: Function];

const typeInfo = new Map<TypedArrayConstructor, TypeInfo>([
  [Uint8Array, [0, wasmMem.u8Array] as TypeInfo],
  [Uint32Array, [2, wasmMem.u32Array] as TypeInfo],
]);

/**
 * The Buffer class is a growable buffer of data on the WASM heap.  We cache
 * buffer instances to avoid repeated allocating and freeing temporary buffers.
 */
export class Buffer {
  ptr: wasmMem.Pointer;
  byteSize: number;

  constructor(private readonly arena: Arena) {
    this.ptr = 0;
    this.byteSize = 0;
  }

  /**
   * Returns a buffer of the given type, in whatever size the buffer currently
   * has available.
   *
   * WARNING: the returned buffer will be invalidated if the WebAssembly memory
   * grows!  If this occurs, a new TypedArray can be safely obtained using
   * get() above.
   */
  get<T>(c: Constructor<T>): T {
    const info = typeInfo.get(c)!;
    return info[1](this.ptr, this.byteSize >> info[0]);
  }

  /**
   * Returns a buffer of the given type, which must be at least `size` elements
   * in length.  The buffer will grow if necessary.
   *
   * Example call:
   *   const arr = buffer.resize(Uint32Array, 128);
   *
   * WARNING: the returned buffer will be invalidated if the WebAssembly memory
   * grows!  If this occurs, a new TypedArray can be safely obtained using
   * get() above.
   */
  resize<T>(c: Constructor<T>, size: number): T {
    const info = typeInfo.get(c)!;
    const needBytes = size << info[0];
    if (this.byteSize >= needBytes) return this.get(c);

    let newByteSize = Math.max(this.byteSize, 128);
    while (newByteSize < needBytes) newByteSize *= 2;
    this.ptr = this.arena.malloc(newByteSize);
    this.byteSize = newByteSize;
    return this.get(c);
  }
}
