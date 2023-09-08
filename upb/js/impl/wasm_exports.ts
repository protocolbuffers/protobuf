/**
 * @license
 * Copyright 2023 Google LLC
 *
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file or at
 * https://developers.google.com/open-source/licenses/bsd
 */

/**
 * @fileoverview Declares the functions implemented in WASM that will be
 * exported from the .wasm file. All JS->WASM calls come through this interface.
 */

import * as wasmMem from './wasm_mem';

/**
 * Functions that are exported from the .wasm file.
 *
 * Declaring this as an interface will prevent JSCompiler from minifying the
 * functions in the compiled output, which is important as they must match the
 * names in the .wasm file.
 */
export declare interface WasmExports {
  upb_Arena_New: () => wasmMem.Pointer;
  upb_Arena_Free: (arena: wasmMem.Pointer) => void;
  upb_Arena_Malloc: (arena: wasmMem.Pointer, size: number) => wasmMem.Pointer;
  upb_Arena_ShrinkLast:
      (arena: wasmMem.Pointer, ptr: wasmMem.Pointer, oldSize: number,
       size: number) => void;
  upb_Encode:
      (msg: wasmMem.Pointer, miniTable: wasmMem.Pointer, options: number,
       arena: wasmMem.Pointer, buf: wasmMem.Pointer,
       size: wasmMem.Pointer) => number;
  upb_Decode:
      (buf: wasmMem.Pointer, size: number, msg: wasmMem.Pointer,
       miniTable: wasmMem.Pointer, extensionRegistry: wasmMem.Pointer,
       options: number, arena: wasmMem.Pointer) => number;
  upb_Message_New:
      (miniTable: wasmMem.Pointer, arena: wasmMem.Pointer) => wasmMem.Pointer;
  upb_MiniTable_Build:
      (data: wasmMem.Pointer, size: number, arena: wasmMem.Pointer,
       status: wasmMem.Pointer) => wasmMem.Pointer;
  upb_MiniTableEnum_Build:
      (data: wasmMem.Pointer, size: number, arena: wasmMem.Pointer,
       status: wasmMem.Pointer) => wasmMem.Pointer;
  upb_MiniTableExtension_Build:
      (data: wasmMem.Pointer, size: number, extendee: wasmMem.Pointer,
       arena: wasmMem.Pointer, status: wasmMem.Pointer) => wasmMem.Pointer;
  upb_MiniTable_Link:
      (miniTable: wasmMem.Pointer, subTables: wasmMem.Pointer,
       subTableCount: number, subEnums: wasmMem.Pointer,
       subEnumCount: number) => number;
}

/**
 * The full set of exports from wasm when we're building with the 'no jsbits'
 * mode. When using the preferred 'with jsbits' mode these functions will not
 * be exported from the wasm to minimize binary size.
 */
export declare interface WasmExportsFull extends WasmExports {
  upb_Array_DataPtr: (arrayPtr: wasmMem.Pointer) => wasmMem.Pointer;
  upb_Array_Size: (arrayPtr: wasmMem.Pointer) => number;

  upb_Message_GetArray:
      (msg: wasmMem.Pointer, field: wasmMem.Pointer) => wasmMem.Pointer;
  upb_Message_GetBool:
      (msg: wasmMem.Pointer, field: wasmMem.Pointer,
       defaultValue: boolean) => number;
  upb_Message_GetDouble:
      (msg: wasmMem.Pointer, field: wasmMem.Pointer,
       defaultValue: number) => number;
  upb_Message_GetFloat:
      (msg: wasmMem.Pointer, field: wasmMem.Pointer,
       defaultValue: number) => number;
  upb_Message_GetInt32:
      (msg: wasmMem.Pointer, field: wasmMem.Pointer,
       defaultValue: number) => number;
  upb_Message_GetUInt32:
      (msg: wasmMem.Pointer, field: wasmMem.Pointer,
       defaultValue: number) => number;
  upb_Message_GetMessage:
      (msg: wasmMem.Pointer, field: wasmMem.Pointer,
       defaultValue: wasmMem.Pointer) => wasmMem.Pointer;
  upb_Message_GetInt64Hi:
      (msg: wasmMem.Pointer, field: wasmMem.Pointer) => number;
  upb_Message_GetInt64Lo:
      (msg: wasmMem.Pointer, field: wasmMem.Pointer) => number;
  upb_Message_GetUInt64Hi:
      (msg: wasmMem.Pointer, field: wasmMem.Pointer) => number;
  upb_Message_GetUInt64Lo:
      (msg: wasmMem.Pointer, field: wasmMem.Pointer) => number;

  upb_Message_SetBool:
      (msg: wasmMem.Pointer, field: wasmMem.Pointer, value: boolean,
       arena: wasmMem.Pointer) => void;
  upb_Message_SetInt32:
      (msg: wasmMem.Pointer, field: wasmMem.Pointer, value: number,
       arena: wasmMem.Pointer) => void;
  upb_Message_SetUInt32:
      (msg: wasmMem.Pointer, field: wasmMem.Pointer, value: number,
       arena: wasmMem.Pointer) => void;
  upb_Message_SetFloat:
      (msg: wasmMem.Pointer, field: wasmMem.Pointer, value: number,
       arena: wasmMem.Pointer) => void;
  upb_Message_SetDouble:
      (msg: wasmMem.Pointer, field: wasmMem.Pointer, value: number,
       arena: wasmMem.Pointer) => void;
  upb_Message_SetInt64Split:
      (msg: wasmMem.Pointer, field: wasmMem.Pointer, hi: number, lo: number,
       arena: wasmMem.Pointer) => void;
  upb_Message_SetUInt64Split:
      (msg: wasmMem.Pointer, field: wasmMem.Pointer, hi: number, lo: number,
       arena: wasmMem.Pointer) => void;
  upb_Message_SetString_FromDataAndSize:
      (msg: wasmMem.Pointer, field: wasmMem.Pointer, data: wasmMem.Pointer,
       size: number, arena: wasmMem.Pointer) => void;

  upb_Message_GetString_Data:
      (msg: wasmMem.Pointer, field: wasmMem.Pointer) => wasmMem.Pointer;
  upb_Message_GetString_Size:
      (msg: wasmMem.Pointer, field: wasmMem.Pointer) => number;

  upb_MiniTable_GetFieldByIndex:
      (miniTable: wasmMem.Pointer, index: number) => wasmMem.Pointer;

  upb_Message_HasField:
      (msg: wasmMem.Pointer, field: wasmMem.Pointer) => number;
  upb_MiniTableField_HasPresence: (miniTable: wasmMem.Pointer) => number;
}
