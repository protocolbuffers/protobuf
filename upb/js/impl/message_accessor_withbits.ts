/**
 * @license
 * Copyright 2023 Google LLC
 *
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file or at
 * https://developers.google.com/open-source/licenses/bsd
 */

/**
 * @fileoverview Implementations of the *Accessor types in our public interface.
 */

import * as decl from '../upb';

import {AccessorRegistry} from './accessor_registry';
import {Arena} from './arena';
import {EnumAccessor} from './enum_accessor';
import {InternalMessage} from './internal_message';
import {MiniTable} from './mini_table';
import * as strings from './strings';
import * as upbBits from './upb_bits';
import * as wasmMem from './wasm_mem';

/**
 * Implementation of the MessageAccessor interface for upb.js.
 *
 * This class is in the critical path of generated code accessors (getters and
 * setters).  The performance of these methods is critical, and we have several
 * different possible strategies for implementing them, especially when it comes
 * to dispatch.  In other words, this code is some of the most interesting in
 * all of upb.js when it comes to optimizing the performance.
 *
 * It is likely that we could get some performance wins here by using a
 * specialized closure for each field rather than re-reading the field offset
 * and presence information out of WASM memory each time.  This is left for
 * future work, because we don't have benchmarks yet to validate the performance
 * impact of this kind of optimization.
 */
export class MessageAccessorWithBits implements decl.MessageAccessor {
  miniTable: MiniTable;
  fieldArray: wasmMem.Pointer;
  cTypes: number[];

  constructor(
      private readonly registry: AccessorRegistry, miniDescriptor: string,
      arena: Arena) {
    this.miniTable = new MiniTable(miniDescriptor, arena);
    const count = upbBits.fieldCount(this.miniTable.ptr);
    this.fieldArray = upbBits.fieldArray(this.miniTable.ptr);
    this.cTypes = new Array(count);
    for (let i = 0; i < count; i++) {
      const field = upbBits.fieldAtIndex(this.fieldArray, i);
      this.cTypes[i] = upbBits.fieldCType(field);
    }
  }

  link(
      submsgs?: Array<MessageAccessorWithBits|null>,
      subenums?: Array<EnumAccessor|null>): void {
    const submsgCount = submsgs ? submsgs.length : 0;
    const subenumCount = subenums ? subenums.length : 0;
    const buffer = this.registry.buffer;
    const buf = buffer.resize(Uint32Array, submsgCount + subenumCount);

    if (submsgs) {
      for (let i = 0; i < submsgCount; i++) {
        const submsg = submsgs[i];
        if (submsg) {
          // TODO(esrauch): This cast only necessary to work around the bits and
          // no-bits impl; remove this once we decide on which one to do (or
          // do this cleaner if we decide to keep both longer term).
          buf[i] = submsg.miniTable.ptr;
        } else {
          buf[i] = 0;
        }
      }
    }

    if (subenums) {
      for (let i = 0; i < subenumCount; i++) {
        const subenum = subenums[i];
        if (subenum) {
          buf[submsgCount + i] = subenum.miniTableEnum.ptr;
        } else {
          buf[submsgCount + i] = 0;
        }
      }
    }

    const result = this.registry.arena.wasmExports.upb_MiniTable_Link(
        this.miniTable.ptr, buffer.ptr, submsgCount,
        buffer.ptr + (submsgCount << 2), subenumCount);
    if (!result) {
      throw new Error('Failed to link');
    }
  }

  newMessage(arena: Arena): decl.InternalMessage {
    return InternalMessage.create(this.miniTable, arena);
  }

  private checkCtype(fieldIndex: number, ctype: upbBits.CType): void {
    const expected = this.cTypes[fieldIndex];
    if (ctype !== expected) {
      // This will catch field index out of range also, because the array
      // lookup will return `undefined` which will be unequal to `ctype`.
      throw new Error(`ctype (${ctype}) != expected (${expected}) fieldIndex=${
          fieldIndex}`);
    }
  }

  wireDecode(
      msg: InternalMessage, wire: Uint8Array,
      extreg?: decl.AccessorRegistry): boolean {
    if (wire.length > 0 && wire[0] === undefined) {
      throw new Error('Cannot parse from broken Uint8Array');
    }
    const buffer = this.registry.buffer;
    buffer.resize(Uint8Array, wire.length).set(wire, 0);
    const status = this.registry.arena.wasmExports.upb_Decode(
        buffer.ptr, wire.length, msg.ptr, this.miniTable.ptr, 0, 0,
        msg.arena.ptr);
    return status === 0;
  }

  wireEncode(msg: InternalMessage): Uint8Array {
    // TODO: see if we can use this.registry.buffer here instead of allocating
    // a new buffer every time.  This is tricky because the encoder allocates
    // from the arena during encoding.
    const arena = this.registry.instance.newArena() as Arena;
    const ptrPtr = arena.malloc(8);
    const sizePtr = ptrPtr + 4;
    const encodeStatus = this.registry.arena.wasmExports.upb_Encode(
        msg.ptr, this.miniTable.ptr, 0, arena.ptr, ptrPtr, sizePtr);
    const ptr = wasmMem.readU32(ptrPtr);
    const size = wasmMem.readU32(sizePtr);

    // Note: we MUST copy the data into another Uint8Array.  TypedArrays that
    // point to our WASM heap are invalidated whenever the WASM memory grows.
    const ret = new Uint8Array(size);
    ret.set(wasmMem.u8Array(ptr, size));

    return ret;
  }

  hasField(msg: InternalMessage, fieldIndex: number): boolean {
    return upbBits.hasField(
        msg.ptr, upbBits.fieldAtIndex(this.fieldArray, fieldIndex));
  }

  // ----------- Getters ------------

  private checkGet(
      msg: InternalMessage, fieldIndex: number, ctype: upbBits.CType): number
      |null {
    this.checkCtype(fieldIndex, ctype);
    const field = upbBits.fieldAtIndex(this.fieldArray, fieldIndex);
    if (!upbBits.hasField(msg.ptr, field)) return null;
    return upbBits.fieldOffset(field);
  }

  getBool(msg: InternalMessage, fieldIndex: number): boolean|null {
    const ofs = this.checkGet(msg, fieldIndex, upbBits.CType.BOOL);
    if (ofs == null) return null;
    return upbBits.getBoolField(msg.ptr, ofs);
  }

  getInt32(msg: InternalMessage, fieldIndex: number): number|null {
    const ofs = this.checkGet(msg, fieldIndex, upbBits.CType.INT32);
    if (ofs == null) return null;
    return upbBits.getInt32Field(msg.ptr, ofs);
  }

  getUint32(msg: InternalMessage, fieldIndex: number): number|null {
    const ofs = this.checkGet(msg, fieldIndex, upbBits.CType.UINT32);
    if (ofs == null) return null;
    return upbBits.getUint32Field(msg.ptr, ofs);
  }

  getInt64(msg: InternalMessage, fieldIndex: number): [number, number]|null {
    return this.getU64(msg, fieldIndex);
  }

  getUint64(msg: InternalMessage, fieldIndex: number): [number, number]|null {
    return this.getU64(msg, fieldIndex);
  }

  private getU64(msg: InternalMessage, fieldIndex: number):
      [number, number]|null {
    const ofs = this.checkGet(msg, fieldIndex, upbBits.CType.U64);
    if (ofs == null) return null;
    return upbBits.getU64Field(msg.ptr, ofs);
  }

  getFloat(msg: InternalMessage, fieldIndex: number): number|null {
    const ofs = this.checkGet(msg, fieldIndex, upbBits.CType.FLOAT);
    if (ofs == null) return null;
    return upbBits.getFloatField(msg.ptr, ofs);
  }

  getDouble(msg: InternalMessage, fieldIndex: number): number|null {
    const ofs = this.checkGet(msg, fieldIndex, upbBits.CType.DOUBLE);
    if (ofs == null) return null;
    return upbBits.getDoubleField(msg.ptr, ofs);
  }

  getString(msg: InternalMessage, fieldIndex: number): string|null {
    const ofs = this.checkGet(msg, fieldIndex, upbBits.CType.STRING);
    if (ofs == null) return null;
    const [ptr, size] = upbBits.getStringField(msg.ptr, ofs);
    return strings.fromWasmUtf8(ptr, size);
  }

  getMessage(msg: InternalMessage, fieldIndex: number): InternalMessage|null {
    const ofs = this.checkGet(msg, fieldIndex, upbBits.CType.MESSAGE);
    if (ofs == null) return null;
    const ptr = upbBits.getUint32Field(msg.ptr, ofs);
    return ptr ? InternalMessage.wrap(ptr, msg.arena) : null;
  }

  /** Gets the upb_Array* for the given field. */
  private checkGetArrayPtr(
      msg: InternalMessage, fieldIndex: number,
      ctype?: upbBits.CType): wasmMem.Pointer {
    if (ctype !== undefined) {
      this.checkCtype(fieldIndex, ctype);
    }
    const field = upbBits.fieldAtIndex(this.fieldArray, fieldIndex);
    upbBits.checkIsArray(field);
    const ofs = upbBits.fieldOffset(field);
    return upbBits.getUint32Field(msg.ptr, ofs);
  }

  getRepeatedSize(msg: InternalMessage, fieldIndex: number): number {
    const arrayPtr = this.checkGetArrayPtr(msg, fieldIndex);
    if (!arrayPtr) return 0;
    return upbBits.arraySize(arrayPtr);
  }

  private getCopyOfRepeated<T>(
      msg: InternalMessage, fieldIndex: number, ctype: upbBits.CType,
      elemSizeLg2: number, readFn: (ptr: wasmMem.Pointer) => T): T[] {
    const arrayPtr = this.checkGetArrayPtr(msg, fieldIndex, ctype);
    if (!arrayPtr) return [];
    const arraySize = upbBits.arraySize(arrayPtr);
    const arrayDataPtr = upbBits.arrayData(arrayPtr, elemSizeLg2);
    const jsArray = new Array<T>(arraySize);
    for (let i = 0; i < arraySize; i++) {
      jsArray[i] = readFn(arrayDataPtr + (i << elemSizeLg2));
    }
    return jsArray;
  }
  getRepeatedBool(msg: InternalMessage, fieldIndex: number): boolean[] {
    return this.getCopyOfRepeated(
        msg, fieldIndex, upbBits.CType.BOOL, 0, wasmMem.readBool);
  }
  getRepeatedInt32(msg: InternalMessage, fieldIndex: number): number[] {
    return this.getCopyOfRepeated(
        msg, fieldIndex, upbBits.CType.INT32, 2, wasmMem.read32);
  }
  getRepeatedUint32(msg: InternalMessage, fieldIndex: number): number[] {
    return this.getCopyOfRepeated(
        msg, fieldIndex, upbBits.CType.UINT32, 2, wasmMem.readU32);
  }
  getRepeatedFloat(msg: InternalMessage, fieldIndex: number): number[] {
    return this.getCopyOfRepeated(
        msg, fieldIndex, upbBits.CType.FLOAT, 2, wasmMem.readF32);
  }
  getRepeatedDouble(msg: InternalMessage, fieldIndex: number): number[] {
    return this.getCopyOfRepeated(
        msg, fieldIndex, upbBits.CType.DOUBLE, 3, wasmMem.readF64);
  }
  getRepeatedString(msg: InternalMessage, fieldIndex: number): string[] {
    return this.getCopyOfRepeated(
        msg, fieldIndex, upbBits.CType.STRING, 3, strings.stringViewToJsString);
  }
  getRepeatedMessage(msg: InternalMessage, fieldIndex: number):
      InternalMessage[] {
    return this.getCopyOfRepeated(
        msg, fieldIndex, upbBits.CType.MESSAGE, 2,
        (ptr: wasmMem.Pointer) =>
            InternalMessage.wrap(wasmMem.readU32(ptr), msg.arena));
  }

  getRepeatedUint64(msg: InternalMessage, fieldIndex: number):
      Array<[number, number]> {
    return this.getCopyOfRepeated(
        msg, fieldIndex, upbBits.CType.U64, 3, wasmMem.read64AsPair);
  }

  getRepeatedInt64(msg: InternalMessage, fieldIndex: number):
      Array<[number, number]> {
    return this.getCopyOfRepeated(
        msg, fieldIndex, upbBits.CType.U64, 3, wasmMem.read64AsPair);
  }

  // ----------- Setters ------------

  private setPresence(
      msg: InternalMessage, fieldIndex: number, ctype: upbBits.CType): number {
    this.checkCtype(fieldIndex, ctype);
    const field = upbBits.fieldAtIndex(this.fieldArray, fieldIndex);
    upbBits.setPresence(msg.ptr, field);
    return upbBits.fieldOffset(field);
  }

  setBool(msg: InternalMessage, fieldIndex: number, value: boolean): void {
    const ofs = this.setPresence(msg, fieldIndex, upbBits.CType.BOOL);
    upbBits.setBoolField(msg.ptr, ofs, value);
  }

  setInt32(msg: InternalMessage, fieldIndex: number, value: number): void {
    const ofs = this.setPresence(msg, fieldIndex, upbBits.CType.INT32);
    upbBits.setInt32Field(msg.ptr, ofs, value);
  }

  setUint32(msg: InternalMessage, fieldIndex: number, value: number): void {
    const ofs = this.setPresence(msg, fieldIndex, upbBits.CType.UINT32);
    upbBits.setUint32Field(msg.ptr, ofs, value);
  }

  private setU64(
      msg: InternalMessage, fieldIndex: number, hi: number, lo: number): void {
    const ofs = this.setPresence(msg, fieldIndex, upbBits.CType.U64);
    upbBits.setU64Field(msg.ptr, ofs, hi, lo);
  }

  setInt64 = this.setU64;
  setUint64 = this.setU64;

  setFloat(msg: InternalMessage, fieldIndex: number, value: number): void {
    const ofs = this.setPresence(msg, fieldIndex, upbBits.CType.FLOAT);
    upbBits.setFloatField(msg.ptr, ofs, value);
  }

  setDouble(msg: InternalMessage, fieldIndex: number, value: number): void {
    const ofs = this.setPresence(msg, fieldIndex, upbBits.CType.DOUBLE);
    upbBits.setDoubleField(msg.ptr, ofs, value);
  }

  setString(msg: InternalMessage, fieldIndex: number, value: string): void {
    const ofs = this.setPresence(msg, fieldIndex, upbBits.CType.STRING);
    const [ptr, size] = strings.toWasmUtf8(value, msg.arena);
    upbBits.setStringField(msg.ptr, ofs, ptr, size);
  }
}
