/**
 * @license
 * Copyright 2023 Google LLC
 *
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file or at
 * https://developers.google.com/open-source/licenses/bsd
 */

/**
 * @fileoverview Definitions of functions we export to the .wasm file.
 * All WASM->JS calls come through here.
 */

import * as wasmMem from './wasm_mem';

function forEachBuffer(iov: wasmMem.Pointer, iovcnt: number, func: Function) {
  // typedef struct __wasi_ciovec_t {
  //   const uint8_t * buf;
  //   __wasi_size_t buf_len;
  // } __wasi_ciovec_t;
  let total = 0;
  for (let i = 0; i < iovcnt; i++) {
    const elem = iov + (i * 8);
    const buf = wasmMem.readU32(elem);
    const len = wasmMem.readU32(elem + 4);
    total += len;
    func(wasmMem.u8Array(buf, len));
  }
  return total;
}

const upbWasmWASIImports = {
  'fd_close': (fd: number) => {
    console.log('fd_close', fd);
  },

  'fd_seek':
      (fd: number, offsetLow: number, offsetHigh: number, whence: number,
       newOffset: number) => {
        console.log('fd_seek', fd);
      },

  'fd_write': (fd: number, iov: number, iovcnt: number, pnum: number) => {
    const written = forEachBuffer(iov, iovcnt, (buf: Uint8Array) => {
      console.log(new TextDecoder().decode(buf));
    });
    wasmMem.writeU32(pnum, written);
    return 0;
  },

  'proc_exit': (code: number) => {
    console.log('proc_exit!');
    throw new Error('exit');
  },
};


const upbWasmEnv = {
  'emscripten_notify_memory_growth': (memoryIndex: number) => {
    wasmMem.onMemoryGrowth();
  },
  'memory': wasmMem.MEMORY,
};

/**
 * Defines the functions that will be imported by the .wasm file.  We must
 * define all functions that the .wasm file declares as imports, otherwise .wasm
 * loading will fail with `WebAssembly.LinkError`.
 *
 * This is a subset of all of the functions defined in the WASI interface:
 *   https://github.com/WebAssembly/WASI/blob/main/phases/snapshot/witx/wasi_snapshot_preview1.witx
 *
 * We only want to implement the functions that upb will actually need.  This
 * will depend on the functionality that upb is actually using from the standard
 * C library.
 */
export const wasmImports = {
  'wasi_snapshot_preview1': upbWasmWASIImports,
  'env': upbWasmEnv,
};
