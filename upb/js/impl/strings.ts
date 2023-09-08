/**
 * @license
 * Copyright 2023 Google LLC
 *
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file or at
 * https://developers.google.com/open-source/licenses/bsd
 */

/**
 * @fileoverview String interop between JavaScript and WASM.
 */

import {Arena} from './arena';
import * as upbBits from './upb_bits';
import * as wasmMem from './wasm_mem';

const utf8Decoder = new TextDecoder();

export function stringViewToJsString(stringViewPtr: wasmMem.Pointer): string {
  const dataPtr = upbBits.stringViewData(stringViewPtr);
  const size = upbBits.stringViewSize(stringViewPtr);
  return fromWasmUtf8(dataPtr, size);
}

/**
 * Converts a WASM UTF-8 string into a JavaScript string.
 */
export function fromWasmUtf8(ptr: wasmMem.Pointer, size: number): string {
  return utf8Decoder.decode(wasmMem.u8Array(ptr, size));
}

/**
 * Converts a JavaScript string into a WASM UTF-8 string.
 */
export function toWasmUtf8(
    str: string, arena: Arena): [wasmMem.Pointer, number] {
  // Due to the trickiness around allocating the memory, the actual
  // implementation lives in the Arena class.
  return arena.newUtf8Buffer(str);
}
