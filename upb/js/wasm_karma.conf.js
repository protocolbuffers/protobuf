/**
 * @license
 * Copyright 2023 Google LLC
 *
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file or at
 * https://developers.google.com/open-source/licenses/bsd
 */

/**
 * Config for Karma test runner to serve Maldoca WebAssembly binaries.
 *
 * @param {!Object} config
 */
module.exports = ((config) => {
  const SERVE_ARGS =
      {included: false, nocache: false, served: true, watched: false};
  const serve = (pattern) => {
    config.files.push({pattern: pattern, ...SERVE_ARGS});
  };

  serve('third_party/upb/js/upb.js');
  serve('third_party/upb/js/upb_js_wasm/upb_js_c.wasm');
});
