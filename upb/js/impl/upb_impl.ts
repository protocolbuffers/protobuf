/**
 * @license
 * Copyright 2023 Google LLC
 *
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file or at
 * https://developers.google.com/open-source/licenses/bsd
 */

/**
 * @fileoverview The top level interface
 */

import * as decl from '../upb';

import {AccessorRegistry} from './accessor_registry';
import {Arena} from './arena';
import {WasmExports} from './wasm_exports';
import {wasmImports} from './wasm_imports';

class UpbInstance implements decl.UpbInstance {
  cachedGlobalRegistry: AccessorRegistry;

  constructor(private readonly wasmExports: WasmExports) {
    this.cachedGlobalRegistry = new AccessorRegistry(this, this.newArena());
  }

  newArena(): Arena {
    return Arena.newArena(this.wasmExports);
  }

  newRegistry(): AccessorRegistry {
    return new AccessorRegistry(this, this.newArena());
  }
  globalRegistry(): AccessorRegistry {
    return this.cachedGlobalRegistry;
  }
}


// Like fetch(), but rejects the promise if the URL could not be loaded.
async function checkedFetch(url: string): Promise<Response> {
  const response = await fetch(url, {credentials: 'same-origin'});
  if (!response.ok) return Promise.reject();
  return response;
}

async function doGetUpb(): Promise<UpbInstance> {
  // TODO: find a way to make this path name relative instead of absolute.
  // TODO: handle errors.

  // Unfortuantely the Karma (JS) and Dart test runners put static files in
  // different places.  There is not an obvious way to know which environment we
  // are in, so just try both and use the first one that succeeds.
  const sourceNameKarma = 'base/third_party/upb/js/upb_js_wasm/upb_js_c.wasm';
  const sourceNameDart = '/third_party/upb/js/upb_js_wasm/upb_js_c.wasm';
  const fetchResponse = await Promise.any(
      [checkedFetch(sourceNameKarma), checkedFetch(sourceNameDart)]);
  const wasmModule =
      await WebAssembly.instantiateStreaming(fetchResponse, wasmImports);
  const exports = wasmModule.instance.exports as unknown;
  const instance = new UpbInstance(exports as WasmExports);
  return instance;
}

const promise = doGetUpb();

/**
 * This function handles the actual loading, compiling, linking, and
 * instantiating of the UPB global.
 */
export function getUpb(): Promise<UpbInstance> {
  return promise;
}

if ('getUpb' in window === false) {
  window['getUpb'] = getUpb;
}

export {};
