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
 *
 * Contrasted with accessor.ts this implementation delegates to UPB C for all
 * methods rather utilizing upb bits. This is notably slower, and is currently
 * only used as a reference implementation; the implementation that uses bits
 * should not have observable behavior differences compared to this.
 */

import * as decl from '../upb';

import {AccessorRegistry} from './accessor_registry';
import {Arena} from './arena';
import {EnumAccessor} from './enum_accessor';
import {InternalMessage} from './internal_message';
import {MiniTable} from './mini_table';
import * as strings from './strings';
import {WasmExportsFull} from './wasm_exports';
import * as wasmMem from './wasm_mem';


/**
 * Implementation of the MessageAccessor interface for upb.js which doesn't
 * use 'bits' (uses UPB's exposed functions for all calls).
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
export class MessageAccessorNoBits implements decl.MessageAccessor {
  readonly miniTable: MiniTable;
  readonly upbC: WasmExportsFull;

  constructor(
      private readonly registry: AccessorRegistry, miniDescriptor: string,
      arena: Arena) {
    this.miniTable = new MiniTable(miniDescriptor, arena);
    this.upbC = registry.arena.wasmExports as WasmExportsFull;
  }

  link(
      submsgs?: Array<MessageAccessorNoBits|null>,
      subenums?: Array<EnumAccessor|null>): void {
    const submsgCount = submsgs ? submsgs.length : 0;
    const subenumCount = subenums ? subenums.length : 0;
    const buffer = this.registry.buffer;
    const buf = buffer.resize(Uint32Array, submsgCount + subenumCount);

    if (submsgs) {
      for (let i = 0; i < submsgCount; i++) {
        const submsg = submsgs[i];
        if (submsg) {
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

  wireDecode(
      msg: InternalMessage, wire: Uint8Array,
      extreg?: decl.AccessorRegistry): boolean {
    if (wire.length > 0 && wire[0] === undefined) {
      throw new Error('Cannot parse from broken Uint8Array');
    }
    const buffer = this.registry.buffer;
    buffer.resize(Uint8Array, wire.length).set(wire, 0);
    const status = this.upbC.upb_Decode(
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
    const encodeStatus = this.upbC.upb_Encode(
        msg.ptr, this.miniTable.ptr, 0, arena.ptr, ptrPtr, sizePtr);
    if (encodeStatus !== 0) {
      throw new Error('Failed to encode');
    }
    const ptr = wasmMem.readU32(ptrPtr);
    const size = wasmMem.readU32(sizePtr);

    // Note: we MUST copy the data into another Uint8Array.  TypedArrays that
    // point to our WASM heap are invalidated whenever the WASM memory grows.
    const ret = new Uint8Array(size);
    ret.set(wasmMem.u8Array(ptr, size));

    return ret;
  }

  private fieldAt(fieldIndex: number): wasmMem.Pointer {
    return this.upbC.upb_MiniTable_GetFieldByIndex(
        this.miniTable.ptr, fieldIndex);
  }

  hasField(msg: InternalMessage, fieldIndex: number): boolean {
    const f = this.fieldAt(fieldIndex);
    return this.upbC.upb_MiniTableField_HasPresence(f) === 0 ||
        this.upbC.upb_Message_HasField(msg.ptr, f) !== 0;
  }

  // ----------- Getters ------------
  getBool(msg: InternalMessage, fieldIndex: number): boolean|null {
    if (!this.hasField(msg, fieldIndex)) return null;
    return !!this.upbC.upb_Message_GetBool(
        msg.ptr, this.fieldAt(fieldIndex), false);
  }

  getInt32(msg: InternalMessage, fieldIndex: number): number|null {
    if (!this.hasField(msg, fieldIndex)) return null;
    return this.upbC.upb_Message_GetInt32(msg.ptr, this.fieldAt(fieldIndex), 0);
  }

  getUint32(msg: InternalMessage, fieldIndex: number): number|null {
    if (!this.hasField(msg, fieldIndex)) return null;
    const i32 =
        this.upbC.upb_Message_GetUInt32(msg.ptr, this.fieldAt(fieldIndex), 0);
    // u32 return values come through js as i32, >>>0 will cast to u32.
    return i32 >>> 0;
  }

  getInt64(msg: InternalMessage, fieldIndex: number): [number, number]|null {
    if (!this.hasField(msg, fieldIndex)) return null;
    const field = this.fieldAt(fieldIndex);
    return [
      this.upbC.upb_Message_GetInt64Hi(msg.ptr, field),
      this.upbC.upb_Message_GetInt64Lo(msg.ptr, field),
    ];
  }

  getUint64(msg: InternalMessage, fieldIndex: number): [number, number]|null {
    if (!this.hasField(msg, fieldIndex)) return null;
    const field = this.fieldAt(fieldIndex);
    return [
      this.upbC.upb_Message_GetUInt64Hi(msg.ptr, field),
      this.upbC.upb_Message_GetUInt64Lo(msg.ptr, field),
    ];
  }

  getFloat(msg: InternalMessage, fieldIndex: number): number|null {
    if (!this.hasField(msg, fieldIndex)) return null;
    return this.upbC.upb_Message_GetFloat(msg.ptr, this.fieldAt(fieldIndex), 0);
  }

  getDouble(msg: InternalMessage, fieldIndex: number): number|null {
    if (!this.hasField(msg, fieldIndex)) return null;
    return this.upbC.upb_Message_GetDouble(
        msg.ptr, this.fieldAt(fieldIndex), 0);
  }

  getString(msg: InternalMessage, fieldIndex: number): string|null {
    if (!this.hasField(msg, fieldIndex)) return null;
    const f = this.fieldAt(fieldIndex);
    const ptr = this.upbC.upb_Message_GetString_Data(msg.ptr, f);
    const size = this.upbC.upb_Message_GetString_Size(msg.ptr, f);
    return strings.fromWasmUtf8(ptr, size);
  }

  getMessage(msg: InternalMessage, fieldIndex: number): InternalMessage|null {
    if (!this.hasField(msg, fieldIndex)) return null;
    const field = this.fieldAt(fieldIndex);
    const msgPtr = this.upbC.upb_Message_GetMessage(msg.ptr, field, 0);
    if (msgPtr === 0) return null;
    return InternalMessage.wrap(msgPtr, msg.arena);
  }

  getRepeatedSize(msg: InternalMessage, fieldIndex: number): number {
    const field = this.fieldAt(fieldIndex);
    const array = this.upbC.upb_Message_GetArray(msg.ptr, field);
    return (array === 0) ? 0 : this.upbC.upb_Array_Size(array);
  }

  private getRepeatedData<T>(
      msg: InternalMessage, fieldIndex: number, elemSizeLg2: number,
      readFn: (ptr: wasmMem.Pointer) => T): T[] {
    const field = this.fieldAt(fieldIndex);
    const arrayPtr = this.upbC.upb_Message_GetArray(msg.ptr, field);
    if (arrayPtr === 0) return [];
    const data = this.upbC.upb_Array_DataPtr(arrayPtr);
    const size = this.upbC.upb_Array_Size(arrayPtr);
    if (data === 0 || size === 0) return [];
    const ret = [];
    for (let i = 0; i < size; i++) {
      ret.push(readFn(data + (i << elemSizeLg2)));
    }
    return ret;
  }

  getRepeatedBool(msg: InternalMessage, fieldIndex: number): boolean[] {
    return this.getRepeatedData(msg, fieldIndex, 0, wasmMem.readBool);
  }
  getRepeatedInt32(msg: InternalMessage, fieldIndex: number): number[] {
    return this.getRepeatedData(msg, fieldIndex, 2, wasmMem.read32);
  }
  getRepeatedUint32(msg: InternalMessage, fieldIndex: number): number[] {
    return this.getRepeatedData(msg, fieldIndex, 2, wasmMem.readU32);
  }
  getRepeatedFloat(msg: InternalMessage, fieldIndex: number): number[] {
    return this.getRepeatedData(msg, fieldIndex, 2, wasmMem.readF32);
  }
  getRepeatedDouble(msg: InternalMessage, fieldIndex: number): number[] {
    return this.getRepeatedData(msg, fieldIndex, 3, wasmMem.readF64);
  }

  getRepeatedString(msg: InternalMessage, fieldIndex: number): string[] {
    return this.getRepeatedData(
        msg, fieldIndex, 3, strings.stringViewToJsString);
  }
  getRepeatedMessage(msg: InternalMessage, fieldIndex: number):
      InternalMessage[] {
    return this.getRepeatedData(
        msg, fieldIndex, 2,
        (ptr: wasmMem.Pointer) =>
            InternalMessage.wrap(wasmMem.readU32(ptr), msg.arena));
  }

  getRepeatedInt64(msg: InternalMessage, fieldIndex: number):
      Array<[number, number]> {
    return this.getRepeatedData(msg, fieldIndex, 3, wasmMem.read64AsPair);
  }

  getRepeatedUint64(msg: InternalMessage, fieldIndex: number):
      Array<[number, number]> {
    return this.getRepeatedInt64(msg, fieldIndex);
  }

  // ----------- Setters ------------
  setBool(msg: InternalMessage, fieldIndex: number, value: boolean): void {
    this.upbC.upb_Message_SetBool(
        msg.ptr, this.fieldAt(fieldIndex), value, msg.arena.ptr);
  }

  setInt32(msg: InternalMessage, fieldIndex: number, value: number): void {
    this.upbC.upb_Message_SetInt32(
        msg.ptr, this.fieldAt(fieldIndex), value, msg.arena.ptr);
  }

  setUint32(msg: InternalMessage, fieldIndex: number, value: number): void {
    this.upbC.upb_Message_SetUInt32(
        msg.ptr, this.fieldAt(fieldIndex), value, msg.arena.ptr);
  }

  setInt64(msg: InternalMessage, fieldIndex: number, hi: number, lo: number):
      void {
    this.upbC.upb_Message_SetInt64Split(
        msg.ptr, this.fieldAt(fieldIndex), hi, lo, msg.arena.ptr);
  }

  setUint64(msg: InternalMessage, fieldIndex: number, hi: number, lo: number):
      void {
    this.upbC.upb_Message_SetUInt64Split(
        msg.ptr, this.fieldAt(fieldIndex), hi, lo, msg.arena.ptr);
  }

  setFloat(msg: InternalMessage, fieldIndex: number, value: number): void {
    this.upbC.upb_Message_SetFloat(
        msg.ptr, this.fieldAt(fieldIndex), value, msg.arena.ptr);
  }

  setDouble(msg: InternalMessage, fieldIndex: number, value: number): void {
    this.upbC.upb_Message_SetDouble(
        msg.ptr, this.fieldAt(fieldIndex), value, msg.arena.ptr);
  }

  setString(msg: InternalMessage, fieldIndex: number, value: string): void {
    const [ptr, size] = strings.toWasmUtf8(value, msg.arena);
    this.upbC.upb_Message_SetString_FromDataAndSize(
        msg.ptr, this.fieldAt(fieldIndex), ptr, size, msg.arena.ptr);
  }
}
