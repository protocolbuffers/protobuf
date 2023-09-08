/**
 * @license
 * Copyright 2023 Google LLC
 *
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file or at
 * https://developers.google.com/open-source/licenses/bsd
 */

/**
 * @fileoverview Low-level raw memory functionality.  This file contains both
 * globals state (HEAP* variables) and functions for reading/writing to the
 * WASM heap.
 */

import {assert} from 'google3/javascript/common/asserts/asserts';
import {isLiteral} from 'google3/javascript/common/asserts/guards';

/** A type for a pointer to WASM memory. */
export type Pointer = number;

/** The global WASM linear memory object for upb.js. */
export const MEMORY = new WebAssembly.Memory({initial: 10, maximum: 65535});

let HEAP16 = new Int16Array(MEMORY.buffer, 0);
let HEAP32 = new Int32Array(MEMORY.buffer, 0);
let HEAP64 = new BigInt64Array(MEMORY.buffer, 0);
let HEAPU8 = new Uint8Array(MEMORY.buffer, 0);
let HEAPU16 = new Uint16Array(MEMORY.buffer, 0);
let HEAPU32 = new Uint32Array(MEMORY.buffer, 0);
let HEAPF32 = new Float32Array(MEMORY.buffer, 0);
let HEAPF64 = new Float64Array(MEMORY.buffer, 0);

function getOffset16(ptr: Pointer): number {
  assert(ptr & 1, isLiteral(0));
  return ptr >> 1;
}

function getOffset32(ptr: Pointer): number {
  assert(ptr & 3, isLiteral(0));
  return ptr >> 2;
}

function getOffset64(ptr: Pointer): number {
  assert(ptr & 7, isLiteral(0));
  return ptr >> 3;
}

/** Reads a single int16_t from WASM memory. */
export function read16(ptr: Pointer): number {
  return HEAP16[getOffset16(ptr)];
}

/** Reads a single int32_t from WASM memory. */
export function read32(ptr: Pointer): number {
  return HEAP32[getOffset32(ptr)];
}

/** Reads a single int64_t from WASM memory. */
export function read64(ptr: Pointer): bigint {
  return HEAP64[getOffset64(ptr)];
}

/**
 * Reads a single int64_t from WASM memory as two u32s (since u64 doesn't fit
 * into JS number type directly).
 */
export function read64AsPair(ptr: Pointer): [number, number] {
  const hi = readU32(ptr + 4);
  const lo = readU32(ptr);
  return [hi, lo];
}

/**
 * Reads a single bool from WASM memory.
 */
export function readBool(ptr: Pointer): boolean {
  return !!readU8(ptr);
}

/** Reads a single uint8_t from WASM memory. */
export function readU8(ptr: Pointer): number {
  return HEAPU8[ptr];
}

/** Reads a single uint16_t from WASM memory. */
export function readU16(ptr: Pointer): number {
  return HEAPU16[getOffset16(ptr)];
}

/** Reads a single uint32_t from WASM memory. */
export function readU32(ptr: Pointer): number {
  return HEAPU32[getOffset32(ptr)];
}

/** Reads a single float from WASM memory. */
export function readF32(ptr: Pointer): number {
  return HEAPF32[getOffset32(ptr)];
}

/** Reads a single double from WASM memory. */
export function readF64(ptr: Pointer): number {
  return HEAPF64[getOffset64(ptr)];
}

/** Writes a single int32_t to WASM memory. */
export function write32(ptr: Pointer, val: number): void {
  HEAP32[getOffset32(ptr)] = val;
}

/** Writes a single int64_t to WASM memory. */
export function write64(ptr: Pointer, val: bigint): void {
  HEAP64[getOffset64(ptr)] = val;
}

/** Writes a single uint8_t to WASM memory. */
export function writeU8(ptr: Pointer, val: number): void {
  HEAPU8[ptr] = val;
}

/** Writes a single uint32_t to WASM memory. */
export function writeU32(ptr: Pointer, val: number): void {
  HEAPU32[getOffset32(ptr)] = val;
}

/** Writes a single float to WASM memory. */
export function writeF32(ptr: Pointer, val: number): void {
  HEAPF32[getOffset32(ptr)] = val;
}

/** Writes a single double to WASM memory. */
export function writeF64(ptr: Pointer, val: number): void {
  HEAPF64[getOffset64(ptr)] = val;
}

/**
 * Returns a Uint8Array at a given pointer and size (in bytes).
 *
 * Note that the returned value is invalidated whenever WASM memory grows,
 * which can occur for any call into WASM.
 */
export function u8Array(ptr: Pointer, size: number): Uint8Array {
  return HEAPU8.subarray(ptr, ptr + size);
}

/**
 * Returns a Uint32Array at a given pointer and size (in bytes).
 *
 * Note that the returned value is invalidated whenever WASM memory grows,
 * which can occur for any call into WASM.
 */
export function u32Array(ptr: Pointer, size: number): Uint8Array {
  ptr = getOffset32(ptr);
  return HEAPU32.subarray(ptr, ptr + size);
}

/**
 * Function that must be called whenever MEMORY changes. Our typed arrays
 * must be attached to the new ArrayBuffer.
 */
export function onMemoryGrowth(): void {
  HEAP16 = new Int16Array(MEMORY.buffer, 0);
  HEAP32 = new Int32Array(MEMORY.buffer, 0);
  HEAP64 = new BigInt64Array(MEMORY.buffer, 0);
  HEAPU8 = new Uint8Array(MEMORY.buffer, 0);
  HEAPU16 = new Uint16Array(MEMORY.buffer, 0);
  HEAPU32 = new Uint32Array(MEMORY.buffer, 0);
  HEAPF32 = new Float32Array(MEMORY.buffer, 0);
  HEAPF64 = new Float64Array(MEMORY.buffer, 0);
}
