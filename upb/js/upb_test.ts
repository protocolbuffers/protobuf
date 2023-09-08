/**
 * @license
 * Copyright 2023 Google LLC
 *
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file or at
 * https://developers.google.com/open-source/licenses/bsd
 */

describe('arena test', () => {
  it('creates and destroys an arena', async () => {
    const upb = await window.getUpb();
    const arena = upb.newArena();
    expect(arena).toBeDefined();
    // The arena's finalizer will automatically free the memory.
  });
});
