/**
 * @license
 * Copyright 2023 Google LLC
 *
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file or at
 * https://developers.google.com/open-source/licenses/bsd
 */

describe('js arena test', function() {
  it('creates and destroys an arena', async function() {
    let upb = await window.getUpb();
    let arena = upb.newArena();
    expect(arena).toBeDefined();
    // The arena's finalizer will automatically free the memory.
  });
});

describe('MiniTable test', function() {
  it('creates a single MiniTable', async function() {
    let upb = await window.getUpb();
    let msgAccessor = upb.globalRegistry().newMessageAccessor(
        '$P1f14/1d///a/b/c/c/d11a111/a11y|G');
    expect(msgAccessor).toBeDefined();
  });

  it('rejects an invalid MiniDescriptor', async function() {
    let upb = await window.getUpb();
    expect(function() {
      upb.globalRegistry().newMessageAccessor('\\');
    }).toThrowError('cannot build MiniTable');
  });

  it('allows linking of messages and enums', async function() {
    let upb = await window.getUpb();
    let arena = upb.newArena();

    // message MyMessage {
    //   FooMessage my_field = 1;
    //   BarEnum my_field2 = 2;
    // }
    let miniDescriptorMsg = '$O3.P';

    // enum MyEnum {
    //   ZERO = 0;
    // }
    let miniDescriptorEnum = "!!";

    let msgAccessor =
        upb.globalRegistry().newMessageAccessor(miniDescriptorMsg);
    let enumAccessor = upb.globalRegistry().newEnumAccessor(miniDescriptorEnum);

    expect(msgAccessor).toBeDefined();
    expect(enumAccessor).toBeDefined();

    msgAccessor.link([msgAccessor], [enumAccessor]);
  });
});
