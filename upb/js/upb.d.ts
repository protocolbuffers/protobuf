/**
 * @license
 * Copyright 2023 Google LLC
 *
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file or at
 * https://developers.google.com/open-source/licenses/bsd
 */

/**
 * @fileoverview Public interface for upb.js.
 *
 * Since upb.js will be deployed from a CDN (and not with each application) the
 * stability of this interface is very important.
 */

// LINT.IfChange

/**
 * A handle to a upb instance, and all of its associated memory.
 *
 * To obtain an instance, call
 *   const upb = await getUpb();
 */
export declare interface UpbInstance {
  newArena(): Arena;
  newRegistry(): AccessorRegistry;
  globalRegistry(): AccessorRegistry;
}

/**
 * An EnumAccessor contains the schema for a single closed enum type.
 */
export declare interface EnumAccessor {}

/**
 * An ExtensionAccessor contains the schema for a single extension type.
 */
export declare interface ExtensionAccessor {}

/**
 * A MessageAccessor is an object for accessing a given set of fields in a given
 * message type.
 */
export declare interface MessageAccessor {
  link(
      submsgs?: Array<MessageAccessor|null>,
      subenums?: Array<EnumAccessor|null>): void;

  newMessage(arena: Arena): InternalMessage;

  wireDecode(msg: InternalMessage, wire: Uint8Array, extreg?: AccessorRegistry):
      boolean;
  wireEncode(msg: InternalMessage): Uint8Array;

  hasField(msg: InternalMessage, fieldIndex: number): boolean;

  getBool(msg: InternalMessage, fieldIndex: number): boolean|null;
  getInt32(msg: InternalMessage, fieldIndex: number): number|null;
  getUint32(msg: InternalMessage, fieldIndex: number): number|null;
  getInt64(msg: InternalMessage, fieldIndex: number): [number, number]|null;
  getUint64(msg: InternalMessage, fieldIndex: number): [number, number]|null;

  getFloat(msg: InternalMessage, fieldIndex: number): number|null;
  getDouble(msg: InternalMessage, fieldIndex: number): number|null;
  getString(msg: InternalMessage, fieldIndex: number): string|null;
  getMessage(msg: InternalMessage, fieldIndex: number): InternalMessage|null;

  getRepeatedSize(msg: InternalMessage, fieldIndex: number): number;
  getRepeatedBool(msg: InternalMessage, fieldIndex: number): boolean[];
  getRepeatedInt32(msg: InternalMessage, fieldIndex: number): number[];
  getRepeatedUint32(msg: InternalMessage, fieldIndex: number): number[];
  getRepeatedInt64(msg: InternalMessage, fieldIndex: number):
      Array<[number, number]>;
  getRepeatedUint64(msg: InternalMessage, fieldIndex: number):
      Array<[number, number]>;
  getRepeatedFloat(msg: InternalMessage, fieldIndex: number): number[];
  getRepeatedDouble(msg: InternalMessage, fieldIndex: number): number[];
  getRepeatedString(msg: InternalMessage, fieldIndex: number): string[];
  getRepeatedMessage(msg: InternalMessage, fieldIndex: number):
      InternalMessage[];

  setBool(msg: InternalMessage, fieldIndex: number, value: boolean): void;
  setInt32(msg: InternalMessage, fieldIndex: number, value: number): void;
  setUint32(msg: InternalMessage, fieldIndex: number, value: number): void;
  setInt64(msg: InternalMessage, fieldIndex: number, hi: number, lo: number):
      void;
  setUint64(msg: InternalMessage, fieldIndex: number, hi: number, lo: number):
      void;
  setFloat(msg: InternalMessage, fieldIndex: number, value: number): void;
  setDouble(msg: InternalMessage, fieldIndex: number, value: number): void;
  setString(msg: InternalMessage, fieldIndex: number, value: string): void;
}

/**
 * An AccessorRegistry is a set of meessage schemas.
 */
export declare interface AccessorRegistry {
  newEnumAccessor(miniDescriptor: string): EnumAccessor;
  newExtensionAccessor(miniDescriptor: string): ExtensionAccessor;
  newMessageAccessor(miniDescriptor: string): MessageAccessor;
}

/**
 * An InternalMessage is a single message instance.
 */
export declare interface InternalMessage {}

/**
 * An arena lets you allocate memory on the upb heap.  When the arena is
 * garbage collected, the associated memory will be reclaimed.
 */
export declare interface Arena {}

/**
 * Defines the interface of values that we will add to the `window` object.
 * These function as GC roots for dead code elimination performed by Closure
 * Compiler.  Anything not reachable from one of these will be dead-code
 * eliminated (tree shaken).
 */
declare global {
  interface Window {
    getUpb: () => Promise<UpbInstance>;
  }
}

// LINT.ThenChange(//depot/google3/third_party/dart/pb_runtime/lib/js/upbjs_externs.dart)
