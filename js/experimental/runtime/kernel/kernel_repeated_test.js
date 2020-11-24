/**
 * @fileoverview Tests for repeated methods in kernel.js.
 */
goog.module('protobuf.runtime.KernelTest');

goog.setTestOnly();

const ByteString = goog.require('protobuf.ByteString');
const Int64 = goog.require('protobuf.Int64');
const InternalMessage = goog.require('protobuf.binary.InternalMessage');
const Kernel = goog.require('protobuf.runtime.Kernel');
const TestMessage = goog.require('protobuf.testing.binary.TestMessage');
// Note to the reader:
// Since the lazy accessor behavior changes with the checking level some of the
// tests in this file have to know which checking level is enable to make
// correct assertions.
const {CHECK_CRITICAL_STATE} = goog.require('protobuf.internal.checks');

/**
 * @param {...number} bytes
 * @return {!ArrayBuffer}
 */
function createArrayBuffer(...bytes) {
  return new Uint8Array(bytes).buffer;
}

/**
 * Expects the Iterable instance yield the same values as the expected array.
 * @param {!Iterable<T>} iterable
 * @param {!Array<T>} expected
 * @template T
 * TODO: Implement this as a custom matcher.
 */
function expectEqualToArray(iterable, expected) {
  const array = Array.from(iterable);
  expect(array).toEqual(expected);
}

/**
 * Expects the Iterable instance yield qualified values.
 * @param {!Iterable<T>} iterable
 * @param {(function(T): boolean)=} verify
 * @template T
 */
function expectQualifiedIterable(iterable, verify) {
  if (verify) {
    for (const value of iterable) {
      expect(verify(value)).toBe(true);
    }
  }
}

/**
 * Expects the Iterable instance yield the same values as the expected array of
 * messages.
 * @param {!Iterable<!TestMessage>} iterable
 * @param {!Array<!TestMessage>} expected
 * @template T
 * TODO: Implement this as a custom matcher.
 */
function expectEqualToMessageArray(iterable, expected) {
  const array = Array.from(iterable);
  expect(array.length).toEqual(expected.length);
  for (let i = 0; i < array.length; i++) {
    const value = array[i].getBoolWithDefault(1, false);
    const expectedValue = expected[i].getBoolWithDefault(1, false);
    expect(value).toBe(expectedValue);
  }
}

describe('Kernel for repeated boolean does', () => {
  it('return empty array for the empty input', () => {
    const accessor = Kernel.createEmpty();
    expectEqualToArray(accessor.getRepeatedBoolIterable(1), []);
  });

  it('ensure not the same instance returned for the empty input', () => {
    const accessor = Kernel.createEmpty();
    const list1 = accessor.getRepeatedBoolIterable(1);
    const list2 = accessor.getRepeatedBoolIterable(1);
    expect(list1).not.toBe(list2);
  });

  it('return size for the empty input', () => {
    const accessor = Kernel.createEmpty();
    expect(accessor.getRepeatedBoolSize(1)).toEqual(0);
  });

  it('return unpacked values from the input', () => {
    const bytes = createArrayBuffer(0x08, 0x01, 0x08, 0x00);
    const accessor = Kernel.fromArrayBuffer(bytes);
    expectEqualToArray(accessor.getRepeatedBoolIterable(1), [true, false]);
  });

  it('ensure not the same instance returned for unpacked values', () => {
    const bytes = createArrayBuffer(0x08, 0x01, 0x08, 0x00);
    const accessor = Kernel.fromArrayBuffer(bytes);
    const list1 = accessor.getRepeatedBoolIterable(1);
    const list2 = accessor.getRepeatedBoolIterable(1);
    expect(list1).not.toBe(list2);
  });

  it('return unpacked multibytes values from the input', () => {
    const bytes = createArrayBuffer(0x08, 0x80, 0x01, 0x08, 0x80, 0x00);
    const accessor = Kernel.fromArrayBuffer(bytes);
    expectEqualToArray(accessor.getRepeatedBoolIterable(1), [true, false]);
  });

  it('return for adding single unpacked value', () => {
    const accessor = Kernel.createEmpty();
    accessor.addUnpackedBoolElement(1, true);
    expectEqualToArray(accessor.getRepeatedBoolIterable(1), [true]);
    accessor.addUnpackedBoolElement(1, false);
    expectEqualToArray(accessor.getRepeatedBoolIterable(1), [true, false]);
  });

  it('return for adding unpacked values', () => {
    const accessor = Kernel.createEmpty();
    accessor.addUnpackedBoolIterable(1, [true]);
    expectEqualToArray(accessor.getRepeatedBoolIterable(1), [true]);
    accessor.addUnpackedBoolIterable(1, [false]);
    expectEqualToArray(accessor.getRepeatedBoolIterable(1), [true, false]);
  });

  it('return for setting single unpacked value', () => {
    const accessor =
        Kernel.fromArrayBuffer(createArrayBuffer(0x08, 0x00, 0x08, 0x01));
    accessor.setUnpackedBoolElement(1, 0, true);
    expectEqualToArray(accessor.getRepeatedBoolIterable(1), [true, true]);
  });

  it('return for setting unpacked values', () => {
    const accessor = Kernel.createEmpty();
    accessor.setUnpackedBoolIterable(1, [true]);
    expectEqualToArray(accessor.getRepeatedBoolIterable(1), [true]);
    accessor.setUnpackedBoolIterable(1, [false]);
    expectEqualToArray(accessor.getRepeatedBoolIterable(1), [false]);
  });

  it('encode for adding single unpacked value', () => {
    const accessor = Kernel.createEmpty();
    const bytes = createArrayBuffer(0x08, 0x01, 0x08, 0x00);
    accessor.addUnpackedBoolElement(1, true);
    accessor.addUnpackedBoolElement(1, false);
    expect(accessor.serialize()).toEqual(bytes);
  });

  it('encode for adding unpacked values', () => {
    const accessor = Kernel.createEmpty();
    const bytes = createArrayBuffer(0x08, 0x01, 0x08, 0x00);
    accessor.addUnpackedBoolIterable(1, [true, false]);
    expect(accessor.serialize()).toEqual(bytes);
  });

  it('encode for setting single unpacked value', () => {
    const accessor =
        Kernel.fromArrayBuffer(createArrayBuffer(0x0A, 0x02, 0x00, 0x01));
    const bytes = createArrayBuffer(0x08, 0x01, 0x08, 0x01);
    accessor.setUnpackedBoolElement(1, 0, true);
    expect(accessor.serialize()).toEqual(bytes);
  });

  it('encode for setting unpacked values', () => {
    const accessor = Kernel.createEmpty();
    const bytes = createArrayBuffer(0x08, 0x01, 0x08, 0x00);
    accessor.setUnpackedBoolIterable(1, [true, false]);
    expect(accessor.serialize()).toEqual(bytes);
  });

  it('return packed values from the input', () => {
    const bytes = createArrayBuffer(0x0A, 0x02, 0x01, 0x00);
    const accessor = Kernel.fromArrayBuffer(bytes);
    expectEqualToArray(accessor.getRepeatedBoolIterable(1), [true, false]);
  });

  it('ensure not the same instance returned for packed values', () => {
    const bytes = createArrayBuffer(0x0A, 0x02, 0x01, 0x00);
    const accessor = Kernel.fromArrayBuffer(bytes);
    const list1 = accessor.getRepeatedBoolIterable(1);
    const list2 = accessor.getRepeatedBoolIterable(1);
    expect(list1).not.toBe(list2);
  });

  it('return packed multibytes values from the input', () => {
    const bytes = createArrayBuffer(0x0A, 0x04, 0x80, 0x01, 0x80, 0x00);
    const accessor = Kernel.fromArrayBuffer(bytes);
    expectEqualToArray(accessor.getRepeatedBoolIterable(1), [true, false]);
  });

  it('return for adding single packed value', () => {
    const accessor = Kernel.createEmpty();
    accessor.addPackedBoolElement(1, true);
    expectEqualToArray(accessor.getRepeatedBoolIterable(1), [true]);
    accessor.addPackedBoolElement(1, false);
    expectEqualToArray(accessor.getRepeatedBoolIterable(1), [true, false]);
  });

  it('return for adding packed values', () => {
    const accessor = Kernel.createEmpty();
    accessor.addPackedBoolIterable(1, [true]);
    expectEqualToArray(accessor.getRepeatedBoolIterable(1), [true]);
    accessor.addPackedBoolIterable(1, [false]);
    expectEqualToArray(accessor.getRepeatedBoolIterable(1), [true, false]);
  });

  it('return for setting single packed value', () => {
    const accessor =
        Kernel.fromArrayBuffer(createArrayBuffer(0x08, 0x00, 0x08, 0x01));
    accessor.setPackedBoolElement(1, 0, true);
    expectEqualToArray(accessor.getRepeatedBoolIterable(1), [true, true]);
  });

  it('return for setting packed values', () => {
    const accessor = Kernel.createEmpty();
    accessor.setPackedBoolIterable(1, [true]);
    expectEqualToArray(accessor.getRepeatedBoolIterable(1), [true]);
    accessor.setPackedBoolIterable(1, [false]);
    expectEqualToArray(accessor.getRepeatedBoolIterable(1), [false]);
  });

  it('encode for adding single packed value', () => {
    const accessor = Kernel.createEmpty();
    const bytes = createArrayBuffer(0x0A, 0x02, 0x01, 0x00);
    accessor.addPackedBoolElement(1, true);
    accessor.addPackedBoolElement(1, false);
    expect(accessor.serialize()).toEqual(bytes);
  });

  it('encode for adding packed values', () => {
    const accessor = Kernel.createEmpty();
    const bytes = createArrayBuffer(0x0A, 0x02, 0x01, 0x00);
    accessor.addPackedBoolIterable(1, [true, false]);
    expect(accessor.serialize()).toEqual(bytes);
  });

  it('encode for setting single packed value', () => {
    const accessor =
        Kernel.fromArrayBuffer(createArrayBuffer(0x08, 0x00, 0x08, 0x01));
    const bytes = createArrayBuffer(0x0A, 0x02, 0x01, 0x01);
    accessor.setPackedBoolElement(1, 0, true);
    expect(accessor.serialize()).toEqual(bytes);
  });

  it('encode for setting packed values', () => {
    const accessor = Kernel.createEmpty();
    const bytes = createArrayBuffer(0x0A, 0x02, 0x01, 0x00);
    accessor.setPackedBoolIterable(1, [true, false]);
    expect(accessor.serialize()).toEqual(bytes);
  });

  it('return combined values from the input', () => {
    const bytes =
        createArrayBuffer(0x08, 0x01, 0x0A, 0x02, 0x01, 0x00, 0x08, 0x00);
    const accessor = Kernel.fromArrayBuffer(bytes);
    expectEqualToArray(
        accessor.getRepeatedBoolIterable(1), [true, true, false, false]);
  });

  it('return the repeated field element from the input', () => {
    const bytes = createArrayBuffer(0x08, 0x01, 0x08, 0x00);
    const accessor = Kernel.fromArrayBuffer(bytes);
    expect(accessor.getRepeatedBoolElement(
               /* fieldNumber= */ 1, /* index= */ 0))
        .toEqual(true);
    expect(accessor.getRepeatedBoolElement(
               /* fieldNumber= */ 1, /* index= */ 1))
        .toEqual(false);
  });

  it('return the size from the input', () => {
    const bytes = createArrayBuffer(0x08, 0x01, 0x08, 0x00);
    const accessor = Kernel.fromArrayBuffer(bytes);
    expect(accessor.getRepeatedBoolSize(1)).toEqual(2);
  });

  it('fail when getting unpacked bool value with other wire types', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(
        0x09, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => {
        accessor.getRepeatedBoolIterable(1);
      }).toThrowError('Expected wire type: 0 but found: 1');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      expectEqualToArray(accessor.getRepeatedBoolIterable(1), [true]);
    }
  });

  it('fail when adding unpacked bool values with number value', () => {
    const accessor = Kernel.createEmpty();
    const fakeBoolean = /** @type {boolean} */ (/** @type {*} */ (2));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addUnpackedBoolIterable(1, [fakeBoolean]))
          .toThrowError('Must be a boolean, but got: 2');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addUnpackedBoolIterable(1, [fakeBoolean]);
      expectEqualToArray(accessor.getRepeatedBoolIterable(1), [fakeBoolean]);
    }
  });

  it('fail when adding single unpacked bool value with number value', () => {
    const accessor = Kernel.createEmpty();
    const fakeBoolean = /** @type {boolean} */ (/** @type {*} */ (2));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addUnpackedBoolElement(1, fakeBoolean))
          .toThrowError('Must be a boolean, but got: 2');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addUnpackedBoolElement(1, fakeBoolean);
      expectEqualToArray(accessor.getRepeatedBoolIterable(1), [fakeBoolean]);
    }
  });

  it('fail when setting unpacked bool values with number value', () => {
    const accessor = Kernel.createEmpty();
    const fakeBoolean = /** @type {boolean} */ (/** @type {*} */ (2));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setUnpackedBoolIterable(1, [fakeBoolean]))
          .toThrowError('Must be a boolean, but got: 2');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setUnpackedBoolIterable(1, [fakeBoolean]);
      expectEqualToArray(accessor.getRepeatedBoolIterable(1), [fakeBoolean]);
    }
  });

  it('fail when setting single unpacked bool value with number value', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(0x08, 0x00));
    const fakeBoolean = /** @type {boolean} */ (/** @type {*} */ (2));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setUnpackedBoolElement(1, 0, fakeBoolean))
          .toThrowError('Must be a boolean, but got: 2');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setUnpackedBoolElement(1, 0, fakeBoolean);
      expectEqualToArray(accessor.getRepeatedBoolIterable(1), [fakeBoolean]);
    }
  });

  it('fail when adding packed bool values with number value', () => {
    const accessor = Kernel.createEmpty();
    const fakeBoolean = /** @type {boolean} */ (/** @type {*} */ (2));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addPackedBoolIterable(1, [fakeBoolean]))
          .toThrowError('Must be a boolean, but got: 2');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addPackedBoolIterable(1, [fakeBoolean]);
      expectEqualToArray(accessor.getRepeatedBoolIterable(1), [fakeBoolean]);
    }
  });

  it('fail when adding single packed bool value with number value', () => {
    const accessor = Kernel.createEmpty();
    const fakeBoolean = /** @type {boolean} */ (/** @type {*} */ (2));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addPackedBoolElement(1, fakeBoolean))
          .toThrowError('Must be a boolean, but got: 2');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addPackedBoolElement(1, fakeBoolean);
      expectEqualToArray(accessor.getRepeatedBoolIterable(1), [fakeBoolean]);
    }
  });

  it('fail when setting packed bool values with number value', () => {
    const accessor = Kernel.createEmpty();
    const fakeBoolean = /** @type {boolean} */ (/** @type {*} */ (2));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setPackedBoolIterable(1, [fakeBoolean]))
          .toThrowError('Must be a boolean, but got: 2');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setPackedBoolIterable(1, [fakeBoolean]);
      expectEqualToArray(accessor.getRepeatedBoolIterable(1), [fakeBoolean]);
    }
  });

  it('fail when setting single packed bool value with number value', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(0x08, 0x00));
    const fakeBoolean = /** @type {boolean} */ (/** @type {*} */ (2));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setPackedBoolElement(1, 0, fakeBoolean))
          .toThrowError('Must be a boolean, but got: 2');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setPackedBoolElement(1, 0, fakeBoolean);
      expectEqualToArray(accessor.getRepeatedBoolIterable(1), [fakeBoolean]);
    }
  });

  it('fail when setting single unpacked with out-of-bound index', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(0x08, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setUnpackedBoolElement(1, 1, true))
          .toThrowError('Index out of bounds: index: 1 size: 1');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setUnpackedBoolElement(1, 1, true);
      expectEqualToArray(accessor.getRepeatedBoolIterable(1), [false, true]);
    }
  });

  it('fail when setting single packed with out-of-bound index', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(0x08, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setPackedBoolElement(1, 1, true))
          .toThrowError('Index out of bounds: index: 1 size: 1');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setPackedBoolElement(1, 1, true);
      expectEqualToArray(accessor.getRepeatedBoolIterable(1), [false, true]);
    }
  });

  it('fail when getting element with out-of-range index', () => {
    const accessor = Kernel.createEmpty();
    if (CHECK_CRITICAL_STATE) {
      expect(() => {
        accessor.getRepeatedBoolElement(
            /* fieldNumber= */ 1, /* index= */ 0);
      }).toThrowError('Index out of bounds: index: 0 size: 0');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      expect(accessor.getRepeatedBoolElement(
                 /* fieldNumber= */ 1, /* index= */ 0))
          .toBe(undefined);
    }
  });
});

describe('Kernel for repeated double does', () => {
  const value1 = 1;
  const value2 = 0;

  const unpackedValue1Value2 = createArrayBuffer(
      0x09,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0xF0,
      0x3F,  // value1
      0x09,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,  // value2
  );
  const unpackedValue2Value1 = createArrayBuffer(
      0x09,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,  // value1
      0x09,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0xF0,
      0x3F,  // value2
  );

  const packedValue1Value2 = createArrayBuffer(
      0x0A,
      0x10,  // tag
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0xF0,
      0x3F,  // value1
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,  // value2
  );
  const packedValue2Value1 = createArrayBuffer(
      0x0A,
      0x10,  // tag
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,  // value1
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0xF0,
      0x3F,  // value2
  );

  it('return empty array for the empty input', () => {
    const accessor = Kernel.createEmpty();

    const list = accessor.getRepeatedDoubleIterable(1);

    expectEqualToArray(list, []);
  });

  it('ensure not the same instance returned for the empty input', () => {
    const accessor = Kernel.createEmpty();

    const list1 = accessor.getRepeatedDoubleIterable(1);
    const list2 = accessor.getRepeatedDoubleIterable(1);

    expect(list1).not.toBe(list2);
  });

  it('return size for the empty input', () => {
    const accessor = Kernel.createEmpty();

    const size = accessor.getRepeatedDoubleSize(1);

    expect(size).toEqual(0);
  });

  it('return unpacked values from the input', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const list = accessor.getRepeatedDoubleIterable(1);

    expectEqualToArray(list, [value1, value2]);
  });

  it('ensure not the same instance returned for unpacked values', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const list1 = accessor.getRepeatedDoubleIterable(1);
    const list2 = accessor.getRepeatedDoubleIterable(1);

    expect(list1).not.toBe(list2);
  });

  it('add single unpacked value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedDoubleElement(1, value1);
    const list1 = accessor.getRepeatedDoubleIterable(1);
    accessor.addUnpackedDoubleElement(1, value2);
    const list2 = accessor.getRepeatedDoubleIterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('add unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedDoubleIterable(1, [value1]);
    const list1 = accessor.getRepeatedDoubleIterable(1);
    accessor.addUnpackedDoubleIterable(1, [value2]);
    const list2 = accessor.getRepeatedDoubleIterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('set a single unpacked value', () => {
    const accessor = Kernel.fromArrayBuffer(packedValue1Value2);

    accessor.setUnpackedDoubleElement(1, 1, value1);
    const list = accessor.getRepeatedDoubleIterable(1);

    expectEqualToArray(list, [value1, value1]);
  });

  it('set unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setUnpackedDoubleIterable(1, [value1]);
    const list = accessor.getRepeatedDoubleIterable(1);

    expectEqualToArray(list, [value1]);
  });

  it('encode for adding single unpacked value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedDoubleElement(1, value1);
    accessor.addUnpackedDoubleElement(1, value2);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue1Value2);
  });

  it('encode for adding unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedDoubleIterable(1, [value1]);
    accessor.addUnpackedDoubleIterable(1, [value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue1Value2);
  });

  it('encode for setting single unpacked value', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    accessor.setUnpackedDoubleElement(1, 0, value2);
    accessor.setUnpackedDoubleElement(1, 1, value1);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue2Value1);
  });

  it('encode for setting unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setUnpackedDoubleIterable(1, [value1, value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue1Value2);
  });

  it('return packed values from the input', () => {
    const accessor = Kernel.fromArrayBuffer(packedValue1Value2);

    const list = accessor.getRepeatedDoubleIterable(1);

    expectEqualToArray(list, [value1, value2]);
  });

  it('ensure not the same instance returned for packed values', () => {
    const accessor = Kernel.fromArrayBuffer(packedValue1Value2);

    const list1 = accessor.getRepeatedDoubleIterable(1);
    const list2 = accessor.getRepeatedDoubleIterable(1);

    expect(list1).not.toBe(list2);
  });

  it('add single packed value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedDoubleElement(1, value1);
    const list1 = accessor.getRepeatedDoubleIterable(1);
    accessor.addPackedDoubleElement(1, value2);
    const list2 = accessor.getRepeatedDoubleIterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('add packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedDoubleIterable(1, [value1]);
    const list1 = accessor.getRepeatedDoubleIterable(1);
    accessor.addPackedDoubleIterable(1, [value2]);
    const list2 = accessor.getRepeatedDoubleIterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('set a single packed value', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    accessor.setPackedDoubleElement(1, 1, value1);
    const list = accessor.getRepeatedDoubleIterable(1);

    expectEqualToArray(list, [value1, value1]);
  });

  it('set packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setPackedDoubleIterable(1, [value1]);
    const list1 = accessor.getRepeatedDoubleIterable(1);
    accessor.setPackedDoubleIterable(1, [value2]);
    const list2 = accessor.getRepeatedDoubleIterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value2]);
  });

  it('encode for adding single packed value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedDoubleElement(1, value1);
    accessor.addPackedDoubleElement(1, value2);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue1Value2);
  });

  it('encode for adding packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedDoubleIterable(1, [value1, value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue1Value2);
  });

  it('encode for setting single packed value', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    accessor.setPackedDoubleElement(1, 0, value2);
    accessor.setPackedDoubleElement(1, 1, value1);

    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue2Value1);
  });

  it('encode for setting packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setPackedDoubleIterable(1, [value1, value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue1Value2);
  });

  it('return combined values from the input', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(
        0x09,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0xF0,
        0x3F,  // value1
        0x0A,
        0x10,  // tag
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0xF0,
        0x3F,  // value1
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,  // value2
        0x09,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,  // value2
        ));

    const list = accessor.getRepeatedDoubleIterable(1);

    expectEqualToArray(list, [value1, value1, value2, value2]);
  });

  it('return the repeated field element from the input', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const result1 = accessor.getRepeatedDoubleElement(
        /* fieldNumber= */ 1, /* index= */ 0);
    const result2 = accessor.getRepeatedDoubleElement(
        /* fieldNumber= */ 1, /* index= */ 1);

    expect(result1).toEqual(value1);
    expect(result2).toEqual(value2);
  });

  it('return the size from the input', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const size = accessor.getRepeatedDoubleSize(1);

    expect(size).toEqual(2);
  });

  it('fail when getting unpacked double value with other wire types', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(
        0x08, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => {
        accessor.getRepeatedDoubleIterable(1);
      }).toThrowError('Expected wire type: 1 but found: 0');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      expectEqualToArray(
          accessor.getRepeatedDoubleIterable(1), [2.937446524422997e-306]);
    }
  });

  it('fail when adding unpacked double values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeDouble = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addUnpackedDoubleIterable(1, [fakeDouble]))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addUnpackedDoubleIterable(1, [fakeDouble]);
      expectEqualToArray(accessor.getRepeatedDoubleIterable(1), [fakeDouble]);
    }
  });

  it('fail when adding single unpacked double value with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeDouble = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addUnpackedDoubleElement(1, fakeDouble))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addUnpackedDoubleElement(1, fakeDouble);
      expectEqualToArray(accessor.getRepeatedDoubleIterable(1), [fakeDouble]);
    }
  });

  it('fail when setting unpacked double values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeDouble = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setUnpackedDoubleIterable(1, [fakeDouble]))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setUnpackedDoubleIterable(1, [fakeDouble]);
      expectEqualToArray(accessor.getRepeatedDoubleIterable(1), [fakeDouble]);
    }
  });

  it('fail when setting single unpacked double value with null value', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(
        0x08, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00));
    const fakeDouble = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setUnpackedDoubleElement(1, 0, fakeDouble))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setUnpackedDoubleElement(1, 0, fakeDouble);
      expectEqualToArray(accessor.getRepeatedDoubleIterable(1), [fakeDouble]);
    }
  });

  it('fail when adding packed double values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeDouble = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addPackedDoubleIterable(1, [fakeDouble]))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addPackedDoubleIterable(1, [fakeDouble]);
      expectEqualToArray(accessor.getRepeatedDoubleIterable(1), [fakeDouble]);
    }
  });

  it('fail when adding single packed double value with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeDouble = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addPackedDoubleElement(1, fakeDouble))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addPackedDoubleElement(1, fakeDouble);
      expectEqualToArray(accessor.getRepeatedDoubleIterable(1), [fakeDouble]);
    }
  });

  it('fail when setting packed double values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeDouble = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setPackedDoubleIterable(1, [fakeDouble]))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setPackedDoubleIterable(1, [fakeDouble]);
      expectEqualToArray(accessor.getRepeatedDoubleIterable(1), [fakeDouble]);
    }
  });

  it('fail when setting single packed double value with null value', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(
        0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00));
    const fakeDouble = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setPackedDoubleElement(1, 0, fakeDouble))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setPackedDoubleElement(1, 0, fakeDouble);
      expectEqualToArray(accessor.getRepeatedDoubleIterable(1), [fakeDouble]);
    }
  });

  it('fail when setting single unpacked with out-of-bound index', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(
        0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setUnpackedDoubleElement(1, 1, 1))
          .toThrowError('Index out of bounds: index: 1 size: 1');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setUnpackedDoubleElement(1, 1, 1);
      expectEqualToArray(accessor.getRepeatedDoubleIterable(1), [0, 1]);
    }
  });

  it('fail when setting single packed with out-of-bound index', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(
        0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setPackedDoubleElement(1, 1, 1))
          .toThrowError('Index out of bounds: index: 1 size: 1');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setPackedDoubleElement(1, 1, 1);
      expectEqualToArray(accessor.getRepeatedDoubleIterable(1), [0, 1]);
    }
  });

  it('fail when getting element with out-of-range index', () => {
    const accessor = Kernel.createEmpty();
    if (CHECK_CRITICAL_STATE) {
      expect(() => {
        accessor.getRepeatedDoubleElement(
            /* fieldNumber= */ 1, /* index= */ 0);
      }).toThrowError('Index out of bounds: index: 0 size: 0');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      expect(accessor.getRepeatedDoubleElement(
                 /* fieldNumber= */ 1, /* index= */ 0))
          .toBe(undefined);
    }
  });
});

describe('Kernel for repeated fixed32 does', () => {
  const value1 = 1;
  const value2 = 0;

  const unpackedValue1Value2 = createArrayBuffer(
      0x0D, 0x01, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00);
  const unpackedValue2Value1 = createArrayBuffer(
      0x0D, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x01, 0x00, 0x00, 0x00);

  const packedValue1Value2 = createArrayBuffer(
      0x0A, 0x08, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
  const packedValue2Value1 = createArrayBuffer(
      0x0A, 0x08, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00);

  it('return empty array for the empty input', () => {
    const accessor = Kernel.createEmpty();

    const list = accessor.getRepeatedFixed32Iterable(1);

    expectEqualToArray(list, []);
  });

  it('ensure not the same instance returned for the empty input', () => {
    const accessor = Kernel.createEmpty();

    const list1 = accessor.getRepeatedFixed32Iterable(1);
    const list2 = accessor.getRepeatedFixed32Iterable(1);

    expect(list1).not.toBe(list2);
  });

  it('return size for the empty input', () => {
    const accessor = Kernel.createEmpty();

    const size = accessor.getRepeatedFixed32Size(1);

    expect(size).toEqual(0);
  });

  it('return unpacked values from the input', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const list = accessor.getRepeatedFixed32Iterable(1);

    expectEqualToArray(list, [value1, value2]);
  });

  it('ensure not the same instance returned for unpacked values', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const list1 = accessor.getRepeatedFixed32Iterable(1);
    const list2 = accessor.getRepeatedFixed32Iterable(1);

    expect(list1).not.toBe(list2);
  });

  it('add single unpacked value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedFixed32Element(1, value1);
    const list1 = accessor.getRepeatedFixed32Iterable(1);
    accessor.addUnpackedFixed32Element(1, value2);
    const list2 = accessor.getRepeatedFixed32Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('add unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedFixed32Iterable(1, [value1]);
    const list1 = accessor.getRepeatedFixed32Iterable(1);
    accessor.addUnpackedFixed32Iterable(1, [value2]);
    const list2 = accessor.getRepeatedFixed32Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('set a single unpacked value', () => {
    const accessor = Kernel.fromArrayBuffer(packedValue1Value2);

    accessor.setUnpackedFixed32Element(1, 1, value1);
    const list = accessor.getRepeatedFixed32Iterable(1);

    expectEqualToArray(list, [value1, value1]);
  });

  it('set unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setUnpackedFixed32Iterable(1, [value1]);
    const list = accessor.getRepeatedFixed32Iterable(1);

    expectEqualToArray(list, [value1]);
  });

  it('encode for adding single unpacked value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedFixed32Element(1, value1);
    accessor.addUnpackedFixed32Element(1, value2);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue1Value2);
  });

  it('encode for adding unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedFixed32Iterable(1, [value1]);
    accessor.addUnpackedFixed32Iterable(1, [value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue1Value2);
  });

  it('encode for setting single unpacked value', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    accessor.setUnpackedFixed32Element(1, 0, value2);
    accessor.setUnpackedFixed32Element(1, 1, value1);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue2Value1);
  });

  it('encode for setting unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setUnpackedFixed32Iterable(1, [value1, value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue1Value2);
  });

  it('return packed values from the input', () => {
    const accessor = Kernel.fromArrayBuffer(packedValue1Value2);

    const list = accessor.getRepeatedFixed32Iterable(1);

    expectEqualToArray(list, [value1, value2]);
  });

  it('ensure not the same instance returned for packed values', () => {
    const accessor = Kernel.fromArrayBuffer(packedValue1Value2);

    const list1 = accessor.getRepeatedFixed32Iterable(1);
    const list2 = accessor.getRepeatedFixed32Iterable(1);

    expect(list1).not.toBe(list2);
  });

  it('add single packed value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedFixed32Element(1, value1);
    const list1 = accessor.getRepeatedFixed32Iterable(1);
    accessor.addPackedFixed32Element(1, value2);
    const list2 = accessor.getRepeatedFixed32Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('add packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedFixed32Iterable(1, [value1]);
    const list1 = accessor.getRepeatedFixed32Iterable(1);
    accessor.addPackedFixed32Iterable(1, [value2]);
    const list2 = accessor.getRepeatedFixed32Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('set a single packed value', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    accessor.setPackedFixed32Element(1, 1, value1);
    const list = accessor.getRepeatedFixed32Iterable(1);

    expectEqualToArray(list, [value1, value1]);
  });

  it('set packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setPackedFixed32Iterable(1, [value1]);
    const list1 = accessor.getRepeatedFixed32Iterable(1);
    accessor.setPackedFixed32Iterable(1, [value2]);
    const list2 = accessor.getRepeatedFixed32Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value2]);
  });

  it('encode for adding single packed value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedFixed32Element(1, value1);
    accessor.addPackedFixed32Element(1, value2);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue1Value2);
  });

  it('encode for adding packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedFixed32Iterable(1, [value1, value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue1Value2);
  });

  it('encode for setting single packed value', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    accessor.setPackedFixed32Element(1, 0, value2);
    accessor.setPackedFixed32Element(1, 1, value1);

    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue2Value1);
  });

  it('encode for setting packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setPackedFixed32Iterable(1, [value1, value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue1Value2);
  });

  it('return combined values from the input', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(
        0x0D,
        0x01,
        0x00,
        0x00,
        0x00,  // value1
        0x0A,
        0x08,  // tag
        0x01,
        0x00,
        0x00,
        0x00,  // value1
        0x00,
        0x00,
        0x00,
        0x00,  // value2
        0x0D,
        0x00,
        0x00,
        0x00,
        0x00,  // value2
        ));

    const list = accessor.getRepeatedFixed32Iterable(1);

    expectEqualToArray(list, [value1, value1, value2, value2]);
  });

  it('return the repeated field element from the input', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const result1 = accessor.getRepeatedFixed32Element(
        /* fieldNumber= */ 1, /* index= */ 0);
    const result2 = accessor.getRepeatedFixed32Element(
        /* fieldNumber= */ 1, /* index= */ 1);

    expect(result1).toEqual(value1);
    expect(result2).toEqual(value2);
  });

  it('return the size from the input', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const size = accessor.getRepeatedFixed32Size(1);

    expect(size).toEqual(2);
  });

  it('fail when getting unpacked fixed32 value with other wire types', () => {
    const accessor =
        Kernel.fromArrayBuffer(createArrayBuffer(0x08, 0x80, 0x80, 0x80, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => {
        accessor.getRepeatedFixed32Iterable(1);
      }).toThrowError('Expected wire type: 5 but found: 0');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      expectQualifiedIterable(
          accessor.getRepeatedFixed32Iterable(1),
          (value) => Number.isInteger(value));
    }
  });

  it('fail when adding unpacked fixed32 values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeFixed32 = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addUnpackedFixed32Iterable(1, [fakeFixed32]))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addUnpackedFixed32Iterable(1, [fakeFixed32]);
      expectQualifiedIterable(accessor.getRepeatedFixed32Iterable(1));
    }
  });

  it('fail when adding single unpacked fixed32 value with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeFixed32 = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addUnpackedFixed32Element(1, fakeFixed32))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addUnpackedFixed32Element(1, fakeFixed32);
      expectQualifiedIterable(accessor.getRepeatedFixed32Iterable(1));
    }
  });

  it('fail when setting unpacked fixed32 values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeFixed32 = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setUnpackedFixed32Iterable(1, [fakeFixed32]))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setUnpackedFixed32Iterable(1, [fakeFixed32]);
      expectQualifiedIterable(accessor.getRepeatedFixed32Iterable(1));
    }
  });

  it('fail when setting single unpacked fixed32 value with null value', () => {
    const accessor =
        Kernel.fromArrayBuffer(createArrayBuffer(0x08, 0x80, 0x80, 0x80, 0x00));
    const fakeFixed32 = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setUnpackedFixed32Element(1, 0, fakeFixed32))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setUnpackedFixed32Element(1, 0, fakeFixed32);
      expectQualifiedIterable(
          accessor.getRepeatedFixed32Iterable(1),
      );
    }
  });

  it('fail when adding packed fixed32 values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeFixed32 = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addPackedFixed32Iterable(1, [fakeFixed32]))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addPackedFixed32Iterable(1, [fakeFixed32]);
      expectQualifiedIterable(accessor.getRepeatedFixed32Iterable(1));
    }
  });

  it('fail when adding single packed fixed32 value with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeFixed32 = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addPackedFixed32Element(1, fakeFixed32))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addPackedFixed32Element(1, fakeFixed32);
      expectQualifiedIterable(accessor.getRepeatedFixed32Iterable(1));
    }
  });

  it('fail when setting packed fixed32 values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeFixed32 = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setPackedFixed32Iterable(1, [fakeFixed32]))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setPackedFixed32Iterable(1, [fakeFixed32]);
      expectQualifiedIterable(accessor.getRepeatedFixed32Iterable(1));
    }
  });

  it('fail when setting single packed fixed32 value with null value', () => {
    const accessor =
        Kernel.fromArrayBuffer(createArrayBuffer(0x0D, 0x00, 0x00, 0x00, 0x00));
    const fakeFixed32 = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setPackedFixed32Element(1, 0, fakeFixed32))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setPackedFixed32Element(1, 0, fakeFixed32);
      expectQualifiedIterable(accessor.getRepeatedFixed32Iterable(1));
    }
  });

  it('fail when setting single unpacked with out-of-bound index', () => {
    const accessor = Kernel.fromArrayBuffer(
        createArrayBuffer(0x0A, 0x04, 0x00, 0x00, 0x00, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setUnpackedFixed32Element(1, 1, 1))
          .toThrowError('Index out of bounds: index: 1 size: 1');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setUnpackedFixed32Element(1, 1, 1);
      expectQualifiedIterable(
          accessor.getRepeatedFixed32Iterable(1),
          (value) => Number.isInteger(value));
    }
  });

  it('fail when setting single packed with out-of-bound index', () => {
    const accessor =
        Kernel.fromArrayBuffer(createArrayBuffer(0x0D, 0x00, 0x00, 0x00, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setPackedFixed32Element(1, 1, 1))
          .toThrowError('Index out of bounds: index: 1 size: 1');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setPackedFixed32Element(1, 1, 1);
      expectQualifiedIterable(
          accessor.getRepeatedFixed32Iterable(1),
          (value) => Number.isInteger(value));
    }
  });

  it('fail when getting element with out-of-range index', () => {
    const accessor = Kernel.createEmpty();
    if (CHECK_CRITICAL_STATE) {
      expect(() => {
        accessor.getRepeatedFixed32Element(
            /* fieldNumber= */ 1, /* index= */ 0);
      }).toThrowError('Index out of bounds: index: 0 size: 0');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      expect(accessor.getRepeatedFixed32Element(
                 /* fieldNumber= */ 1, /* index= */ 0))
          .toBe(undefined);
    }
  });
});

describe('Kernel for repeated fixed64 does', () => {
  const value1 = Int64.fromInt(1);
  const value2 = Int64.fromInt(0);

  const unpackedValue1Value2 = createArrayBuffer(
      0x09,
      0x01,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,  // value1
      0x09,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,  // value2
  );
  const unpackedValue2Value1 = createArrayBuffer(
      0x09,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,  // value1
      0x09,
      0x01,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,  // value2
  );

  const packedValue1Value2 = createArrayBuffer(
      0x0A,
      0x10,  // tag
      0x01,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,  // value1
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,  // value2
  );
  const packedValue2Value1 = createArrayBuffer(
      0x0A,
      0x10,  // tag
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,  // value2
      0x01,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,  // value1
  );

  it('return empty array for the empty input', () => {
    const accessor = Kernel.createEmpty();

    const list = accessor.getRepeatedFixed64Iterable(1);

    expectEqualToArray(list, []);
  });

  it('ensure not the same instance returned for the empty input', () => {
    const accessor = Kernel.createEmpty();

    const list1 = accessor.getRepeatedFixed64Iterable(1);
    const list2 = accessor.getRepeatedFixed64Iterable(1);

    expect(list1).not.toBe(list2);
  });

  it('return size for the empty input', () => {
    const accessor = Kernel.createEmpty();

    const size = accessor.getRepeatedFixed64Size(1);

    expect(size).toEqual(0);
  });

  it('return unpacked values from the input', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const list = accessor.getRepeatedFixed64Iterable(1);

    expectEqualToArray(list, [value1, value2]);
  });

  it('ensure not the same instance returned for unpacked values', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const list1 = accessor.getRepeatedFixed64Iterable(1);
    const list2 = accessor.getRepeatedFixed64Iterable(1);

    expect(list1).not.toBe(list2);
  });

  it('add single unpacked value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedFixed64Element(1, value1);
    const list1 = accessor.getRepeatedFixed64Iterable(1);
    accessor.addUnpackedFixed64Element(1, value2);
    const list2 = accessor.getRepeatedFixed64Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('add unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedFixed64Iterable(1, [value1]);
    const list1 = accessor.getRepeatedFixed64Iterable(1);
    accessor.addUnpackedFixed64Iterable(1, [value2]);
    const list2 = accessor.getRepeatedFixed64Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('set a single unpacked value', () => {
    const accessor = Kernel.fromArrayBuffer(packedValue1Value2);

    accessor.setUnpackedFixed64Element(1, 1, value1);
    const list = accessor.getRepeatedFixed64Iterable(1);

    expectEqualToArray(list, [value1, value1]);
  });

  it('set unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setUnpackedFixed64Iterable(1, [value1]);
    const list = accessor.getRepeatedFixed64Iterable(1);

    expectEqualToArray(list, [value1]);
  });

  it('encode for adding single unpacked value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedFixed64Element(1, value1);
    accessor.addUnpackedFixed64Element(1, value2);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue1Value2);
  });

  it('encode for adding unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedFixed64Iterable(1, [value1]);
    accessor.addUnpackedFixed64Iterable(1, [value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue1Value2);
  });

  it('encode for setting single unpacked value', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    accessor.setUnpackedFixed64Element(1, 0, value2);
    accessor.setUnpackedFixed64Element(1, 1, value1);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue2Value1);
  });

  it('encode for setting unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setUnpackedFixed64Iterable(1, [value1, value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue1Value2);
  });

  it('return packed values from the input', () => {
    const accessor = Kernel.fromArrayBuffer(packedValue1Value2);

    const list = accessor.getRepeatedFixed64Iterable(1);

    expectEqualToArray(list, [value1, value2]);
  });

  it('ensure not the same instance returned for packed values', () => {
    const accessor = Kernel.fromArrayBuffer(packedValue1Value2);

    const list1 = accessor.getRepeatedFixed64Iterable(1);
    const list2 = accessor.getRepeatedFixed64Iterable(1);

    expect(list1).not.toBe(list2);
  });

  it('add single packed value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedFixed64Element(1, value1);
    const list1 = accessor.getRepeatedFixed64Iterable(1);
    accessor.addPackedFixed64Element(1, value2);
    const list2 = accessor.getRepeatedFixed64Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('add packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedFixed64Iterable(1, [value1]);
    const list1 = accessor.getRepeatedFixed64Iterable(1);
    accessor.addPackedFixed64Iterable(1, [value2]);
    const list2 = accessor.getRepeatedFixed64Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('set a single packed value', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    accessor.setPackedFixed64Element(1, 1, value1);
    const list = accessor.getRepeatedFixed64Iterable(1);

    expectEqualToArray(list, [value1, value1]);
  });

  it('set packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setPackedFixed64Iterable(1, [value1]);
    const list1 = accessor.getRepeatedFixed64Iterable(1);
    accessor.setPackedFixed64Iterable(1, [value2]);
    const list2 = accessor.getRepeatedFixed64Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value2]);
  });

  it('encode for adding single packed value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedFixed64Element(1, value1);
    accessor.addPackedFixed64Element(1, value2);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue1Value2);
  });

  it('encode for adding packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedFixed64Iterable(1, [value1, value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue1Value2);
  });

  it('encode for setting single packed value', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    accessor.setPackedFixed64Element(1, 0, value2);
    accessor.setPackedFixed64Element(1, 1, value1);

    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue2Value1);
  });

  it('encode for setting packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setPackedFixed64Iterable(1, [value1, value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue1Value2);
  });

  it('return combined values from the input', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(
        0x09, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // value1
        0x0A, 0x10,                                            // tag
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,        // value1
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,        // value2
        0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00   // value2
        ));

    const list = accessor.getRepeatedFixed64Iterable(1);

    expectEqualToArray(list, [value1, value1, value2, value2]);
  });

  it('return the repeated field element from the input', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const result1 = accessor.getRepeatedFixed64Element(
        /* fieldNumber= */ 1, /* index= */ 0);
    const result2 = accessor.getRepeatedFixed64Element(
        /* fieldNumber= */ 1, /* index= */ 1);

    expect(result1).toEqual(value1);
    expect(result2).toEqual(value2);
  });

  it('return the size from the input', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const size = accessor.getRepeatedFixed64Size(1);

    expect(size).toEqual(2);
  });

  it('fail when getting unpacked fixed64 value with other wire types', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(
        0x08, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => {
        accessor.getRepeatedFixed64Iterable(1);
      }).toThrowError('Expected wire type: 1 but found: 0');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      expectQualifiedIterable(
          accessor.getRepeatedFixed64Iterable(1),
          (value) => value instanceof Int64);
    }
  });

  it('fail when adding unpacked fixed64 values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeFixed64 = /** @type {!Int64} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addUnpackedFixed64Iterable(1, [fakeFixed64]))
          .toThrowError('Must be Int64 instance, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addUnpackedFixed64Iterable(1, [fakeFixed64]);
      expectQualifiedIterable(accessor.getRepeatedFixed64Iterable(1));
    }
  });

  it('fail when adding single unpacked fixed64 value with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeFixed64 = /** @type {!Int64} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addUnpackedFixed64Element(1, fakeFixed64))
          .toThrowError('Must be Int64 instance, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addUnpackedFixed64Element(1, fakeFixed64);
      expectQualifiedIterable(accessor.getRepeatedFixed64Iterable(1));
    }
  });

  it('fail when setting unpacked fixed64 values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeFixed64 = /** @type {!Int64} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setUnpackedFixed64Iterable(1, [fakeFixed64]))
          .toThrowError('Must be Int64 instance, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setUnpackedFixed64Iterable(1, [fakeFixed64]);
      expectQualifiedIterable(accessor.getRepeatedFixed64Iterable(1));
    }
  });

  it('fail when setting single unpacked fixed64 value with null value', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(
        0x08, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00));
    const fakeFixed64 = /** @type {!Int64} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setUnpackedFixed64Element(1, 0, fakeFixed64))
          .toThrowError('Must be Int64 instance, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setUnpackedFixed64Element(1, 0, fakeFixed64);
      expectQualifiedIterable(accessor.getRepeatedFixed64Iterable(1));
    }
  });

  it('fail when adding packed fixed64 values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeFixed64 = /** @type {!Int64} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addPackedFixed64Iterable(1, [fakeFixed64]))
          .toThrowError('Must be Int64 instance, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addPackedFixed64Iterable(1, [fakeFixed64]);
      expectQualifiedIterable(accessor.getRepeatedFixed64Iterable(1));
    }
  });

  it('fail when adding single packed fixed64 value with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeFixed64 = /** @type {!Int64} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addPackedFixed64Element(1, fakeFixed64))
          .toThrowError('Must be Int64 instance, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addPackedFixed64Element(1, fakeFixed64);
      expectQualifiedIterable(accessor.getRepeatedFixed64Iterable(1));
    }
  });

  it('fail when setting packed fixed64 values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeFixed64 = /** @type {!Int64} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setPackedFixed64Iterable(1, [fakeFixed64]))
          .toThrowError('Must be Int64 instance, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setPackedFixed64Iterable(1, [fakeFixed64]);
      expectQualifiedIterable(accessor.getRepeatedFixed64Iterable(1));
    }
  });

  it('fail when setting single packed fixed64 value with null value', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(
        0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00));
    const fakeFixed64 = /** @type {!Int64} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setPackedFixed64Element(1, 0, fakeFixed64))
          .toThrowError('Must be Int64 instance, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setPackedFixed64Element(1, 0, fakeFixed64);
      expectQualifiedIterable(accessor.getRepeatedFixed64Iterable(1));
    }
  });

  it('fail when setting single unpacked with out-of-bound index', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(
        0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setUnpackedFixed64Element(1, 1, Int64.fromInt(1)))
          .toThrowError('Index out of bounds: index: 1 size: 1');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setUnpackedFixed64Element(1, 1, Int64.fromInt(1));
      expectQualifiedIterable(
          accessor.getRepeatedFixed64Iterable(1),
          (value) => value instanceof Int64);
    }
  });

  it('fail when setting single packed with out-of-bound index', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(
        0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setPackedFixed64Element(1, 1, Int64.fromInt(1)))
          .toThrowError('Index out of bounds: index: 1 size: 1');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setPackedFixed64Element(1, 1, Int64.fromInt(1));
      expectQualifiedIterable(
          accessor.getRepeatedFixed64Iterable(1),
          (value) => value instanceof Int64);
    }
  });

  it('fail when getting element with out-of-range index', () => {
    const accessor = Kernel.createEmpty();
    if (CHECK_CRITICAL_STATE) {
      expect(() => {
        accessor.getRepeatedFixed64Element(
            /* fieldNumber= */ 1, /* index= */ 0);
      }).toThrowError('Index out of bounds: index: 0 size: 0');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      expect(accessor.getRepeatedFixed64Element(
                 /* fieldNumber= */ 1, /* index= */ 0))
          .toBe(undefined);
    }
  });
});

describe('Kernel for repeated float does', () => {
  const value1 = 1.6;
  const value1Float = Math.fround(1.6);
  const value2 = 0;

  const unpackedValue1Value2 = createArrayBuffer(
      0x0D, 0xCD, 0xCC, 0xCC, 0x3F, 0x0D, 0x00, 0x00, 0x00, 0x00);
  const unpackedValue2Value1 = createArrayBuffer(
      0x0D, 0x00, 0x00, 0x00, 0x00, 0x0D, 0xCD, 0xCC, 0xCC, 0x3F);

  const packedValue1Value2 = createArrayBuffer(
      0x0A, 0x08, 0xCD, 0xCC, 0xCC, 0x3F, 0x00, 0x00, 0x00, 0x00);
  const packedValue2Value1 = createArrayBuffer(
      0x0A, 0x08, 0x00, 0x00, 0x00, 0x00, 0xCD, 0xCC, 0xCC, 0x3F);

  it('return empty array for the empty input', () => {
    const accessor = Kernel.createEmpty();

    const list = accessor.getRepeatedFloatIterable(1);

    expectEqualToArray(list, []);
  });

  it('ensure not the same instance returned for the empty input', () => {
    const accessor = Kernel.createEmpty();

    const list1 = accessor.getRepeatedFloatIterable(1);
    const list2 = accessor.getRepeatedFloatIterable(1);

    expect(list1).not.toBe(list2);
  });

  it('return size for the empty input', () => {
    const accessor = Kernel.createEmpty();

    const size = accessor.getRepeatedFloatSize(1);

    expect(size).toEqual(0);
  });

  it('return unpacked values from the input', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const list = accessor.getRepeatedFloatIterable(1);

    expectEqualToArray(list, [value1Float, value2]);
  });

  it('ensure not the same instance returned for unpacked values', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const list1 = accessor.getRepeatedFloatIterable(1);
    const list2 = accessor.getRepeatedFloatIterable(1);

    expect(list1).not.toBe(list2);
  });

  it('add single unpacked value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedFloatElement(1, value1);
    const list1 = accessor.getRepeatedFloatIterable(1);
    accessor.addUnpackedFloatElement(1, value2);
    const list2 = accessor.getRepeatedFloatIterable(1);

    expectEqualToArray(list1, [value1Float]);
    expectEqualToArray(list2, [value1Float, value2]);
  });

  it('add unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedFloatIterable(1, [value1]);
    const list1 = accessor.getRepeatedFloatIterable(1);
    accessor.addUnpackedFloatIterable(1, [value2]);
    const list2 = accessor.getRepeatedFloatIterable(1);

    expectEqualToArray(list1, [value1Float]);
    expectEqualToArray(list2, [value1Float, value2]);
  });

  it('set a single unpacked value', () => {
    const accessor = Kernel.fromArrayBuffer(packedValue1Value2);

    accessor.setUnpackedFloatElement(1, 1, value1);
    const list = accessor.getRepeatedFloatIterable(1);

    expectEqualToArray(list, [value1Float, value1Float]);
  });

  it('set unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setUnpackedFloatIterable(1, [value1]);
    const list = accessor.getRepeatedFloatIterable(1);

    expectEqualToArray(list, [value1Float]);
  });

  it('encode for adding single unpacked value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedFloatElement(1, value1);
    accessor.addUnpackedFloatElement(1, value2);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue1Value2);
  });

  it('encode for adding unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedFloatIterable(1, [value1]);
    accessor.addUnpackedFloatIterable(1, [value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue1Value2);
  });

  it('encode for setting single unpacked value', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    accessor.setUnpackedFloatElement(1, 0, value2);
    accessor.setUnpackedFloatElement(1, 1, value1);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue2Value1);
  });

  it('encode for setting unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setUnpackedFloatIterable(1, [value1, value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue1Value2);
  });

  it('return packed values from the input', () => {
    const accessor = Kernel.fromArrayBuffer(packedValue1Value2);

    const list = accessor.getRepeatedFloatIterable(1);

    expectEqualToArray(list, [value1Float, value2]);
  });

  it('ensure not the same instance returned for packed values', () => {
    const accessor = Kernel.fromArrayBuffer(packedValue1Value2);

    const list1 = accessor.getRepeatedFloatIterable(1);
    const list2 = accessor.getRepeatedFloatIterable(1);

    expect(list1).not.toBe(list2);
  });

  it('add single packed value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedFloatElement(1, value1);
    const list1 = accessor.getRepeatedFloatIterable(1);
    accessor.addPackedFloatElement(1, value2);
    const list2 = accessor.getRepeatedFloatIterable(1);

    expectEqualToArray(list1, [value1Float]);
    expectEqualToArray(list2, [value1Float, value2]);
  });

  it('add packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedFloatIterable(1, [value1]);
    const list1 = accessor.getRepeatedFloatIterable(1);
    accessor.addPackedFloatIterable(1, [value2]);
    const list2 = accessor.getRepeatedFloatIterable(1);

    expectEqualToArray(list1, [value1Float]);
    expectEqualToArray(list2, [value1Float, value2]);
  });

  it('set a single packed value', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    accessor.setPackedFloatElement(1, 1, value1);
    const list = accessor.getRepeatedFloatIterable(1);

    expectEqualToArray(list, [value1Float, value1Float]);
  });

  it('set packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setPackedFloatIterable(1, [value1]);
    const list1 = accessor.getRepeatedFloatIterable(1);
    accessor.setPackedFloatIterable(1, [value2]);
    const list2 = accessor.getRepeatedFloatIterable(1);

    expectEqualToArray(list1, [value1Float]);
    expectEqualToArray(list2, [value2]);
  });

  it('encode for adding single packed value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedFloatElement(1, value1);
    accessor.addPackedFloatElement(1, value2);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue1Value2);
  });

  it('encode for adding packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedFloatIterable(1, [value1, value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue1Value2);
  });

  it('encode for setting single packed value', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    accessor.setPackedFloatElement(1, 0, value2);
    accessor.setPackedFloatElement(1, 1, value1);

    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue2Value1);
  });

  it('encode for setting packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setPackedFloatIterable(1, [value1, value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue1Value2);
  });

  it('return combined values from the input', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(
        0x0D,
        0xCD,
        0xCC,
        0xCC,
        0x3F,  // value1
        0x0A,
        0x08,  // tag
        0xCD,
        0xCC,
        0xCC,
        0x3F,  // value1
        0x00,
        0x00,
        0x00,
        0x00,  // value2
        0x0D,
        0x00,
        0x00,
        0x00,
        0x00,  // value2
        ));

    const list = accessor.getRepeatedFloatIterable(1);

    expectEqualToArray(list, [value1Float, value1Float, value2, value2]);
  });

  it('return the repeated field element from the input', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const result1 = accessor.getRepeatedFloatElement(
        /* fieldNumber= */ 1, /* index= */ 0);
    const result2 = accessor.getRepeatedFloatElement(
        /* fieldNumber= */ 1, /* index= */ 1);

    expect(result1).toEqual(value1Float);
    expect(result2).toEqual(value2);
  });

  it('return the size from the input', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const size = accessor.getRepeatedFloatSize(1);

    expect(size).toEqual(2);
  });

  it('fail when getting unpacked float value with other wire types', () => {
    const accessor =
        Kernel.fromArrayBuffer(createArrayBuffer(0x08, 0x80, 0x80, 0x80, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => {
        accessor.getRepeatedFloatIterable(1);
      }).toThrowError('Expected wire type: 5 but found: 0');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      expectQualifiedIterable(
          accessor.getRepeatedFloatIterable(1),
          (value) => typeof value === 'number');
    }
  });

  it('fail when adding unpacked float values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeFloat = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addUnpackedFloatIterable(1, [fakeFloat]))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addUnpackedFloatIterable(1, [fakeFloat]);
      expectQualifiedIterable(accessor.getRepeatedFloatIterable(1));
    }
  });

  it('fail when adding single unpacked float value with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeFloat = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addUnpackedFloatElement(1, fakeFloat))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addUnpackedFloatElement(1, fakeFloat);
      expectQualifiedIterable(accessor.getRepeatedFloatIterable(1));
    }
  });

  it('fail when setting unpacked float values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeFloat = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setUnpackedFloatIterable(1, [fakeFloat]))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setUnpackedFloatIterable(1, [fakeFloat]);
      expectQualifiedIterable(accessor.getRepeatedFloatIterable(1));
    }
  });

  it('fail when setting single unpacked float value with null value', () => {
    const accessor =
        Kernel.fromArrayBuffer(createArrayBuffer(0x08, 0x80, 0x80, 0x80, 0x00));
    const fakeFloat = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setUnpackedFloatElement(1, 0, fakeFloat))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setUnpackedFloatElement(1, 0, fakeFloat);
      expectQualifiedIterable(accessor.getRepeatedFloatIterable(1));
    }
  });

  it('fail when adding packed float values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeFloat = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addPackedFloatIterable(1, [fakeFloat]))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addPackedFloatIterable(1, [fakeFloat]);
      expectQualifiedIterable(accessor.getRepeatedFloatIterable(1));
    }
  });

  it('fail when adding single packed float value with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeFloat = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addPackedFloatElement(1, fakeFloat))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addPackedFloatElement(1, fakeFloat);
      expectQualifiedIterable(accessor.getRepeatedFloatIterable(1));
    }
  });

  it('fail when setting packed float values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeFloat = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setPackedFloatIterable(1, [fakeFloat]))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setPackedFloatIterable(1, [fakeFloat]);
      expectQualifiedIterable(accessor.getRepeatedFloatIterable(1));
    }
  });

  it('fail when setting single packed float value with null value', () => {
    const accessor =
        Kernel.fromArrayBuffer(createArrayBuffer(0x0D, 0x00, 0x00, 0x00, 0x00));
    const fakeFloat = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setPackedFloatElement(1, 0, fakeFloat))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setPackedFloatElement(1, 0, fakeFloat);
      expectQualifiedIterable(accessor.getRepeatedFloatIterable(1));
    }
  });

  it('fail when setting single unpacked with out-of-bound index', () => {
    const accessor =
        Kernel.fromArrayBuffer(createArrayBuffer(0x0D, 0x00, 0x00, 0x00, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setUnpackedFloatElement(1, 1, 1))
          .toThrowError('Index out of bounds: index: 1 size: 1');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setUnpackedFloatElement(1, 1, 1);
      expectQualifiedIterable(
          accessor.getRepeatedFloatIterable(1),
          (value) => typeof value === 'number');
    }
  });

  it('fail when setting single packed with out-of-bound index', () => {
    const accessor =
        Kernel.fromArrayBuffer(createArrayBuffer(0x0D, 0x00, 0x00, 0x00, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setPackedFloatElement(1, 1, 1))
          .toThrowError('Index out of bounds: index: 1 size: 1');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setPackedFloatElement(1, 1, 1);
      expectQualifiedIterable(
          accessor.getRepeatedFloatIterable(1),
          (value) => typeof value === 'number');
    }
  });

  it('fail when getting element with out-of-range index', () => {
    const accessor = Kernel.createEmpty();
    if (CHECK_CRITICAL_STATE) {
      expect(() => {
        accessor.getRepeatedFloatElement(
            /* fieldNumber= */ 1, /* index= */ 0);
      }).toThrowError('Index out of bounds: index: 0 size: 0');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      expect(accessor.getRepeatedFloatElement(
                 /* fieldNumber= */ 1, /* index= */ 0))
          .toBe(undefined);
    }
  });
});

describe('Kernel for repeated int32 does', () => {
  const value1 = 1;
  const value2 = 0;

  const unpackedValue1Value2 = createArrayBuffer(0x08, 0x01, 0x08, 0x00);
  const unpackedValue2Value1 = createArrayBuffer(0x08, 0x00, 0x08, 0x01);

  const packedValue1Value2 = createArrayBuffer(0x0A, 0x02, 0x01, 0x00);
  const packedValue2Value1 = createArrayBuffer(0x0A, 0x02, 0x00, 0x01);

  it('return empty array for the empty input', () => {
    const accessor = Kernel.createEmpty();

    const list = accessor.getRepeatedInt32Iterable(1);

    expectEqualToArray(list, []);
  });

  it('ensure not the same instance returned for the empty input', () => {
    const accessor = Kernel.createEmpty();

    const list1 = accessor.getRepeatedInt32Iterable(1);
    const list2 = accessor.getRepeatedInt32Iterable(1);

    expect(list1).not.toBe(list2);
  });

  it('return size for the empty input', () => {
    const accessor = Kernel.createEmpty();

    const size = accessor.getRepeatedInt32Size(1);

    expect(size).toEqual(0);
  });

  it('return unpacked values from the input', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const list = accessor.getRepeatedInt32Iterable(1);

    expectEqualToArray(list, [value1, value2]);
  });

  it('ensure not the same instance returned for unpacked values', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const list1 = accessor.getRepeatedInt32Iterable(1);
    const list2 = accessor.getRepeatedInt32Iterable(1);

    expect(list1).not.toBe(list2);
  });

  it('add single unpacked value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedInt32Element(1, value1);
    const list1 = accessor.getRepeatedInt32Iterable(1);
    accessor.addUnpackedInt32Element(1, value2);
    const list2 = accessor.getRepeatedInt32Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('add unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedInt32Iterable(1, [value1]);
    const list1 = accessor.getRepeatedInt32Iterable(1);
    accessor.addUnpackedInt32Iterable(1, [value2]);
    const list2 = accessor.getRepeatedInt32Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('set a single unpacked value', () => {
    const accessor = Kernel.fromArrayBuffer(packedValue1Value2);

    accessor.setUnpackedInt32Element(1, 1, value1);
    const list = accessor.getRepeatedInt32Iterable(1);

    expectEqualToArray(list, [value1, value1]);
  });

  it('set unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setUnpackedInt32Iterable(1, [value1]);
    const list = accessor.getRepeatedInt32Iterable(1);

    expectEqualToArray(list, [value1]);
  });

  it('encode for adding single unpacked value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedInt32Element(1, value1);
    accessor.addUnpackedInt32Element(1, value2);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue1Value2);
  });

  it('encode for adding unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedInt32Iterable(1, [value1]);
    accessor.addUnpackedInt32Iterable(1, [value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue1Value2);
  });

  it('encode for setting single unpacked value', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    accessor.setUnpackedInt32Element(1, 0, value2);
    accessor.setUnpackedInt32Element(1, 1, value1);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue2Value1);
  });

  it('encode for setting unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setUnpackedInt32Iterable(1, [value1, value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue1Value2);
  });

  it('return packed values from the input', () => {
    const accessor = Kernel.fromArrayBuffer(packedValue1Value2);

    const list = accessor.getRepeatedInt32Iterable(1);

    expectEqualToArray(list, [value1, value2]);
  });

  it('ensure not the same instance returned for packed values', () => {
    const accessor = Kernel.fromArrayBuffer(packedValue1Value2);

    const list1 = accessor.getRepeatedInt32Iterable(1);
    const list2 = accessor.getRepeatedInt32Iterable(1);

    expect(list1).not.toBe(list2);
  });

  it('add single packed value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedInt32Element(1, value1);
    const list1 = accessor.getRepeatedInt32Iterable(1);
    accessor.addPackedInt32Element(1, value2);
    const list2 = accessor.getRepeatedInt32Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('add packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedInt32Iterable(1, [value1]);
    const list1 = accessor.getRepeatedInt32Iterable(1);
    accessor.addPackedInt32Iterable(1, [value2]);
    const list2 = accessor.getRepeatedInt32Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('set a single packed value', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    accessor.setPackedInt32Element(1, 1, value1);
    const list = accessor.getRepeatedInt32Iterable(1);

    expectEqualToArray(list, [value1, value1]);
  });

  it('set packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setPackedInt32Iterable(1, [value1]);
    const list1 = accessor.getRepeatedInt32Iterable(1);
    accessor.setPackedInt32Iterable(1, [value2]);
    const list2 = accessor.getRepeatedInt32Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value2]);
  });

  it('encode for adding single packed value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedInt32Element(1, value1);
    accessor.addPackedInt32Element(1, value2);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue1Value2);
  });

  it('encode for adding packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedInt32Iterable(1, [value1, value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue1Value2);
  });

  it('encode for setting single packed value', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    accessor.setPackedInt32Element(1, 0, value2);
    accessor.setPackedInt32Element(1, 1, value1);

    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue2Value1);
  });

  it('encode for setting packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setPackedInt32Iterable(1, [value1, value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue1Value2);
  });

  it('return combined values from the input', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(
        0x08,
        0x01,  // unpacked value1
        0x0A,
        0x02,
        0x01,
        0x00,  // packed value1 and value2
        0x08,
        0x00,  // unpacked value2
        ));

    const list = accessor.getRepeatedInt32Iterable(1);

    expectEqualToArray(list, [value1, value1, value2, value2]);
  });

  it('return the repeated field element from the input', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const result1 = accessor.getRepeatedInt32Element(
        /* fieldNumber= */ 1, /* index= */ 0);
    const result2 = accessor.getRepeatedInt32Element(
        /* fieldNumber= */ 1, /* index= */ 1);

    expect(result1).toEqual(value1);
    expect(result2).toEqual(value2);
  });

  it('return the size from the input', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const size = accessor.getRepeatedInt32Size(1);

    expect(size).toEqual(2);
  });

  it('fail when getting unpacked int32 value with other wire types', () => {
    const accessor =
        Kernel.fromArrayBuffer(createArrayBuffer(0x0D, 0x80, 0x80, 0x80, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => {
        accessor.getRepeatedInt32Iterable(1);
      }).toThrowError('Expected wire type: 0 but found: 5');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      expectQualifiedIterable(
          accessor.getRepeatedInt32Iterable(1),
          (value) => Number.isInteger(value));
    }
  });

  it('fail when adding unpacked int32 values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeInt32 = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addUnpackedInt32Iterable(1, [fakeInt32]))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addUnpackedInt32Iterable(1, [fakeInt32]);
      expectQualifiedIterable(accessor.getRepeatedInt32Iterable(1));
    }
  });

  it('fail when adding single unpacked int32 value with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeInt32 = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addUnpackedInt32Element(1, fakeInt32))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addUnpackedInt32Element(1, fakeInt32);
      expectQualifiedIterable(accessor.getRepeatedInt32Iterable(1));
    }
  });

  it('fail when setting unpacked int32 values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeInt32 = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setUnpackedInt32Iterable(1, [fakeInt32]))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setUnpackedInt32Iterable(1, [fakeInt32]);
      expectQualifiedIterable(accessor.getRepeatedInt32Iterable(1));
    }
  });

  it('fail when setting single unpacked int32 value with null value', () => {
    const accessor =
        Kernel.fromArrayBuffer(createArrayBuffer(0x0D, 0x80, 0x80, 0x80, 0x00));
    const fakeInt32 = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setUnpackedInt32Element(1, 0, fakeInt32))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setUnpackedInt32Element(1, 0, fakeInt32);
      expectQualifiedIterable(
          accessor.getRepeatedInt32Iterable(1),
      );
    }
  });

  it('fail when adding packed int32 values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeInt32 = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addPackedInt32Iterable(1, [fakeInt32]))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addPackedInt32Iterable(1, [fakeInt32]);
      expectQualifiedIterable(accessor.getRepeatedInt32Iterable(1));
    }
  });

  it('fail when adding single packed int32 value with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeInt32 = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addPackedInt32Element(1, fakeInt32))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addPackedInt32Element(1, fakeInt32);
      expectQualifiedIterable(accessor.getRepeatedInt32Iterable(1));
    }
  });

  it('fail when setting packed int32 values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeInt32 = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setPackedInt32Iterable(1, [fakeInt32]))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setPackedInt32Iterable(1, [fakeInt32]);
      expectQualifiedIterable(accessor.getRepeatedInt32Iterable(1));
    }
  });

  it('fail when setting single packed int32 value with null value', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(0x08, 0x00));
    const fakeInt32 = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setPackedInt32Element(1, 0, fakeInt32))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setPackedInt32Element(1, 0, fakeInt32);
      expectQualifiedIterable(accessor.getRepeatedInt32Iterable(1));
    }
  });

  it('fail when setting single unpacked with out-of-bound index', () => {
    const accessor =
        Kernel.fromArrayBuffer(createArrayBuffer(0x0A, 0x01, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setUnpackedInt32Element(1, 1, 1))
          .toThrowError('Index out of bounds: index: 1 size: 1');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setUnpackedInt32Element(1, 1, 1);
      expectQualifiedIterable(
          accessor.getRepeatedInt32Iterable(1),
          (value) => Number.isInteger(value));
    }
  });

  it('fail when setting single packed with out-of-bound index', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(0x08, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setPackedInt32Element(1, 1, 1))
          .toThrowError('Index out of bounds: index: 1 size: 1');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setPackedInt32Element(1, 1, 1);
      expectQualifiedIterable(
          accessor.getRepeatedInt32Iterable(1),
          (value) => Number.isInteger(value));
    }
  });

  it('fail when getting element with out-of-range index', () => {
    const accessor = Kernel.createEmpty();
    if (CHECK_CRITICAL_STATE) {
      expect(() => {
        accessor.getRepeatedInt32Element(
            /* fieldNumber= */ 1, /* index= */ 0);
      }).toThrowError('Index out of bounds: index: 0 size: 0');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      expect(accessor.getRepeatedInt32Element(
                 /* fieldNumber= */ 1, /* index= */ 0))
          .toBe(undefined);
    }
  });
});

describe('Kernel for repeated int64 does', () => {
  const value1 = Int64.fromInt(1);
  const value2 = Int64.fromInt(0);

  const unpackedValue1Value2 = createArrayBuffer(0x08, 0x01, 0x08, 0x00);
  const unpackedValue2Value1 = createArrayBuffer(0x08, 0x00, 0x08, 0x01);

  const packedValue1Value2 = createArrayBuffer(0x0A, 0x02, 0x01, 0x00);
  const packedValue2Value1 = createArrayBuffer(0x0A, 0x02, 0x00, 0x01);

  it('return empty array for the empty input', () => {
    const accessor = Kernel.createEmpty();

    const list = accessor.getRepeatedInt64Iterable(1);

    expectEqualToArray(list, []);
  });

  it('ensure not the same instance returned for the empty input', () => {
    const accessor = Kernel.createEmpty();

    const list1 = accessor.getRepeatedInt64Iterable(1);
    const list2 = accessor.getRepeatedInt64Iterable(1);

    expect(list1).not.toBe(list2);
  });

  it('return size for the empty input', () => {
    const accessor = Kernel.createEmpty();

    const size = accessor.getRepeatedInt64Size(1);

    expect(size).toEqual(0);
  });

  it('return unpacked values from the input', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const list = accessor.getRepeatedInt64Iterable(1);

    expectEqualToArray(list, [value1, value2]);
  });

  it('ensure not the same instance returned for unpacked values', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const list1 = accessor.getRepeatedInt64Iterable(1);
    const list2 = accessor.getRepeatedInt64Iterable(1);

    expect(list1).not.toBe(list2);
  });

  it('add single unpacked value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedInt64Element(1, value1);
    const list1 = accessor.getRepeatedInt64Iterable(1);
    accessor.addUnpackedInt64Element(1, value2);
    const list2 = accessor.getRepeatedInt64Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('add unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedInt64Iterable(1, [value1]);
    const list1 = accessor.getRepeatedInt64Iterable(1);
    accessor.addUnpackedInt64Iterable(1, [value2]);
    const list2 = accessor.getRepeatedInt64Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('set a single unpacked value', () => {
    const accessor = Kernel.fromArrayBuffer(packedValue1Value2);

    accessor.setUnpackedInt64Element(1, 1, value1);
    const list = accessor.getRepeatedInt64Iterable(1);

    expectEqualToArray(list, [value1, value1]);
  });

  it('set unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setUnpackedInt64Iterable(1, [value1]);
    const list = accessor.getRepeatedInt64Iterable(1);

    expectEqualToArray(list, [value1]);
  });

  it('encode for adding single unpacked value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedInt64Element(1, value1);
    accessor.addUnpackedInt64Element(1, value2);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue1Value2);
  });

  it('encode for adding unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedInt64Iterable(1, [value1]);
    accessor.addUnpackedInt64Iterable(1, [value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue1Value2);
  });

  it('encode for setting single unpacked value', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    accessor.setUnpackedInt64Element(1, 0, value2);
    accessor.setUnpackedInt64Element(1, 1, value1);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue2Value1);
  });

  it('encode for setting unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setUnpackedInt64Iterable(1, [value1, value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue1Value2);
  });

  it('return packed values from the input', () => {
    const accessor = Kernel.fromArrayBuffer(packedValue1Value2);

    const list = accessor.getRepeatedInt64Iterable(1);

    expectEqualToArray(list, [value1, value2]);
  });

  it('ensure not the same instance returned for packed values', () => {
    const accessor = Kernel.fromArrayBuffer(packedValue1Value2);

    const list1 = accessor.getRepeatedInt64Iterable(1);
    const list2 = accessor.getRepeatedInt64Iterable(1);

    expect(list1).not.toBe(list2);
  });

  it('add single packed value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedInt64Element(1, value1);
    const list1 = accessor.getRepeatedInt64Iterable(1);
    accessor.addPackedInt64Element(1, value2);
    const list2 = accessor.getRepeatedInt64Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('add packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedInt64Iterable(1, [value1]);
    const list1 = accessor.getRepeatedInt64Iterable(1);
    accessor.addPackedInt64Iterable(1, [value2]);
    const list2 = accessor.getRepeatedInt64Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('set a single packed value', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    accessor.setPackedInt64Element(1, 1, value1);
    const list = accessor.getRepeatedInt64Iterable(1);

    expectEqualToArray(list, [value1, value1]);
  });

  it('set packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setPackedInt64Iterable(1, [value1]);
    const list1 = accessor.getRepeatedInt64Iterable(1);
    accessor.setPackedInt64Iterable(1, [value2]);
    const list2 = accessor.getRepeatedInt64Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value2]);
  });

  it('encode for adding single packed value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedInt64Element(1, value1);
    accessor.addPackedInt64Element(1, value2);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue1Value2);
  });

  it('encode for adding packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedInt64Iterable(1, [value1, value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue1Value2);
  });

  it('encode for setting single packed value', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    accessor.setPackedInt64Element(1, 0, value2);
    accessor.setPackedInt64Element(1, 1, value1);

    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue2Value1);
  });

  it('encode for setting packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setPackedInt64Iterable(1, [value1, value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue1Value2);
  });

  it('return combined values from the input', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(
        0x08,
        0x01,  // unpacked value1
        0x0A,
        0x02,
        0x01,
        0x00,  // packed value1 and value2
        0x08,
        0x00,  // unpacked value2
        ));

    const list = accessor.getRepeatedInt64Iterable(1);

    expectEqualToArray(list, [value1, value1, value2, value2]);
  });

  it('return the repeated field element from the input', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const result1 = accessor.getRepeatedInt64Element(
        /* fieldNumber= */ 1, /* index= */ 0);
    const result2 = accessor.getRepeatedInt64Element(
        /* fieldNumber= */ 1, /* index= */ 1);

    expect(result1).toEqual(value1);
    expect(result2).toEqual(value2);
  });

  it('return the size from the input', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const size = accessor.getRepeatedInt64Size(1);

    expect(size).toEqual(2);
  });

  it('fail when getting unpacked int64 value with other wire types', () => {
    const accessor =
        Kernel.fromArrayBuffer(createArrayBuffer(0x0D, 0x80, 0x80, 0x80, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => {
        accessor.getRepeatedInt64Iterable(1);
      }).toThrowError('Expected wire type: 0 but found: 5');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      expectQualifiedIterable(
          accessor.getRepeatedInt64Iterable(1),
          (value) => value instanceof Int64);
    }
  });

  it('fail when adding unpacked int64 values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeInt64 = /** @type {!Int64} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addUnpackedInt64Iterable(1, [fakeInt64]))
          .toThrowError('Must be Int64 instance, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addUnpackedInt64Iterable(1, [fakeInt64]);
      expectQualifiedIterable(accessor.getRepeatedInt64Iterable(1));
    }
  });

  it('fail when adding single unpacked int64 value with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeInt64 = /** @type {!Int64} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addUnpackedInt64Element(1, fakeInt64))
          .toThrowError('Must be Int64 instance, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addUnpackedInt64Element(1, fakeInt64);
      expectQualifiedIterable(accessor.getRepeatedInt64Iterable(1));
    }
  });

  it('fail when setting unpacked int64 values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeInt64 = /** @type {!Int64} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setUnpackedInt64Iterable(1, [fakeInt64]))
          .toThrowError('Must be Int64 instance, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setUnpackedInt64Iterable(1, [fakeInt64]);
      expectQualifiedIterable(accessor.getRepeatedInt64Iterable(1));
    }
  });

  it('fail when setting single unpacked int64 value with null value', () => {
    const accessor =
        Kernel.fromArrayBuffer(createArrayBuffer(0x0D, 0x80, 0x80, 0x80, 0x00));
    const fakeInt64 = /** @type {!Int64} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setUnpackedInt64Element(1, 0, fakeInt64))
          .toThrowError('Must be Int64 instance, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setUnpackedInt64Element(1, 0, fakeInt64);
      expectQualifiedIterable(accessor.getRepeatedInt64Iterable(1));
    }
  });

  it('fail when adding packed int64 values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeInt64 = /** @type {!Int64} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addPackedInt64Iterable(1, [fakeInt64]))
          .toThrowError('Must be Int64 instance, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addPackedInt64Iterable(1, [fakeInt64]);
      expectQualifiedIterable(accessor.getRepeatedInt64Iterable(1));
    }
  });

  it('fail when adding single packed int64 value with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeInt64 = /** @type {!Int64} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addPackedInt64Element(1, fakeInt64))
          .toThrowError('Must be Int64 instance, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addPackedInt64Element(1, fakeInt64);
      expectQualifiedIterable(accessor.getRepeatedInt64Iterable(1));
    }
  });

  it('fail when setting packed int64 values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeInt64 = /** @type {!Int64} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setPackedInt64Iterable(1, [fakeInt64]))
          .toThrowError('Must be Int64 instance, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setPackedInt64Iterable(1, [fakeInt64]);
      expectQualifiedIterable(accessor.getRepeatedInt64Iterable(1));
    }
  });

  it('fail when setting single packed int64 value with null value', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(0x08, 0x00));
    const fakeInt64 = /** @type {!Int64} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setPackedInt64Element(1, 0, fakeInt64))
          .toThrowError('Must be Int64 instance, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setPackedInt64Element(1, 0, fakeInt64);
      expectQualifiedIterable(accessor.getRepeatedInt64Iterable(1));
    }
  });

  it('fail when setting single unpacked with out-of-bound index', () => {
    const accessor =
        Kernel.fromArrayBuffer(createArrayBuffer(0x0A, 0x01, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setUnpackedInt64Element(1, 1, Int64.fromInt(1)))
          .toThrowError('Index out of bounds: index: 1 size: 1');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setUnpackedInt64Element(1, 1, Int64.fromInt(1));
      expectQualifiedIterable(
          accessor.getRepeatedInt64Iterable(1),
          (value) => value instanceof Int64);
    }
  });

  it('fail when setting single packed with out-of-bound index', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(0x08, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setPackedInt64Element(1, 1, Int64.fromInt(1)))
          .toThrowError('Index out of bounds: index: 1 size: 1');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setPackedInt64Element(1, 1, Int64.fromInt(1));
      expectQualifiedIterable(
          accessor.getRepeatedInt64Iterable(1),
          (value) => value instanceof Int64);
    }
  });

  it('fail when getting element with out-of-range index', () => {
    const accessor = Kernel.createEmpty();
    if (CHECK_CRITICAL_STATE) {
      expect(() => {
        accessor.getRepeatedInt64Element(
            /* fieldNumber= */ 1, /* index= */ 0);
      }).toThrowError('Index out of bounds: index: 0 size: 0');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      expect(accessor.getRepeatedInt64Element(
                 /* fieldNumber= */ 1, /* index= */ 0))
          .toBe(undefined);
    }
  });
});

describe('Kernel for repeated sfixed32 does', () => {
  const value1 = 1;
  const value2 = 0;

  const unpackedValue1Value2 = createArrayBuffer(
      0x0D, 0x01, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00);
  const unpackedValue2Value1 = createArrayBuffer(
      0x0D, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x01, 0x00, 0x00, 0x00);

  const packedValue1Value2 = createArrayBuffer(
      0x0A, 0x08, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
  const packedValue2Value1 = createArrayBuffer(
      0x0A, 0x08, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00);

  it('return empty array for the empty input', () => {
    const accessor = Kernel.createEmpty();

    const list = accessor.getRepeatedSfixed32Iterable(1);

    expectEqualToArray(list, []);
  });

  it('ensure not the same instance returned for the empty input', () => {
    const accessor = Kernel.createEmpty();

    const list1 = accessor.getRepeatedSfixed32Iterable(1);
    const list2 = accessor.getRepeatedSfixed32Iterable(1);

    expect(list1).not.toBe(list2);
  });

  it('return size for the empty input', () => {
    const accessor = Kernel.createEmpty();

    const size = accessor.getRepeatedSfixed32Size(1);

    expect(size).toEqual(0);
  });

  it('return unpacked values from the input', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const list = accessor.getRepeatedSfixed32Iterable(1);

    expectEqualToArray(list, [value1, value2]);
  });

  it('ensure not the same instance returned for unpacked values', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const list1 = accessor.getRepeatedSfixed32Iterable(1);
    const list2 = accessor.getRepeatedSfixed32Iterable(1);

    expect(list1).not.toBe(list2);
  });

  it('add single unpacked value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedSfixed32Element(1, value1);
    const list1 = accessor.getRepeatedSfixed32Iterable(1);
    accessor.addUnpackedSfixed32Element(1, value2);
    const list2 = accessor.getRepeatedSfixed32Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('add unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedSfixed32Iterable(1, [value1]);
    const list1 = accessor.getRepeatedSfixed32Iterable(1);
    accessor.addUnpackedSfixed32Iterable(1, [value2]);
    const list2 = accessor.getRepeatedSfixed32Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('set a single unpacked value', () => {
    const accessor = Kernel.fromArrayBuffer(packedValue1Value2);

    accessor.setUnpackedSfixed32Element(1, 1, value1);
    const list = accessor.getRepeatedSfixed32Iterable(1);

    expectEqualToArray(list, [value1, value1]);
  });

  it('set unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setUnpackedSfixed32Iterable(1, [value1]);
    const list = accessor.getRepeatedSfixed32Iterable(1);

    expectEqualToArray(list, [value1]);
  });

  it('encode for adding single unpacked value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedSfixed32Element(1, value1);
    accessor.addUnpackedSfixed32Element(1, value2);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue1Value2);
  });

  it('encode for adding unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedSfixed32Iterable(1, [value1]);
    accessor.addUnpackedSfixed32Iterable(1, [value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue1Value2);
  });

  it('encode for setting single unpacked value', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    accessor.setUnpackedSfixed32Element(1, 0, value2);
    accessor.setUnpackedSfixed32Element(1, 1, value1);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue2Value1);
  });

  it('encode for setting unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setUnpackedSfixed32Iterable(1, [value1, value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue1Value2);
  });

  it('return packed values from the input', () => {
    const accessor = Kernel.fromArrayBuffer(packedValue1Value2);

    const list = accessor.getRepeatedSfixed32Iterable(1);

    expectEqualToArray(list, [value1, value2]);
  });

  it('ensure not the same instance returned for packed values', () => {
    const accessor = Kernel.fromArrayBuffer(packedValue1Value2);

    const list1 = accessor.getRepeatedSfixed32Iterable(1);
    const list2 = accessor.getRepeatedSfixed32Iterable(1);

    expect(list1).not.toBe(list2);
  });

  it('add single packed value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedSfixed32Element(1, value1);
    const list1 = accessor.getRepeatedSfixed32Iterable(1);
    accessor.addPackedSfixed32Element(1, value2);
    const list2 = accessor.getRepeatedSfixed32Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('add packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedSfixed32Iterable(1, [value1]);
    const list1 = accessor.getRepeatedSfixed32Iterable(1);
    accessor.addPackedSfixed32Iterable(1, [value2]);
    const list2 = accessor.getRepeatedSfixed32Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('set a single packed value', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    accessor.setPackedSfixed32Element(1, 1, value1);
    const list = accessor.getRepeatedSfixed32Iterable(1);

    expectEqualToArray(list, [value1, value1]);
  });

  it('set packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setPackedSfixed32Iterable(1, [value1]);
    const list1 = accessor.getRepeatedSfixed32Iterable(1);
    accessor.setPackedSfixed32Iterable(1, [value2]);
    const list2 = accessor.getRepeatedSfixed32Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value2]);
  });

  it('encode for adding single packed value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedSfixed32Element(1, value1);
    accessor.addPackedSfixed32Element(1, value2);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue1Value2);
  });

  it('encode for adding packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedSfixed32Iterable(1, [value1, value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue1Value2);
  });

  it('encode for setting single packed value', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    accessor.setPackedSfixed32Element(1, 0, value2);
    accessor.setPackedSfixed32Element(1, 1, value1);

    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue2Value1);
  });

  it('encode for setting packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setPackedSfixed32Iterable(1, [value1, value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue1Value2);
  });

  it('return combined values from the input', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(
        0x0D,
        0x01,
        0x00,
        0x00,
        0x00,  // value1
        0x0A,
        0x08,  // tag
        0x01,
        0x00,
        0x00,
        0x00,  // value1
        0x00,
        0x00,
        0x00,
        0x00,  // value2
        0x0D,
        0x00,
        0x00,
        0x00,
        0x00,  // value2
        ));

    const list = accessor.getRepeatedSfixed32Iterable(1);

    expectEqualToArray(list, [value1, value1, value2, value2]);
  });

  it('return the repeated field element from the input', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const result1 = accessor.getRepeatedSfixed32Element(
        /* fieldNumber= */ 1, /* index= */ 0);
    const result2 = accessor.getRepeatedSfixed32Element(
        /* fieldNumber= */ 1, /* index= */ 1);

    expect(result1).toEqual(value1);
    expect(result2).toEqual(value2);
  });

  it('return the size from the input', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const size = accessor.getRepeatedSfixed32Size(1);

    expect(size).toEqual(2);
  });

  it('fail when getting unpacked sfixed32 value with other wire types', () => {
    const accessor =
        Kernel.fromArrayBuffer(createArrayBuffer(0x08, 0x80, 0x80, 0x80, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => {
        accessor.getRepeatedSfixed32Iterable(1);
      }).toThrowError('Expected wire type: 5 but found: 0');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      expectQualifiedIterable(
          accessor.getRepeatedSfixed32Iterable(1),
          (value) => typeof value === 'number');
    }
  });

  it('fail when adding unpacked sfixed32 values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeSfixed32 = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addUnpackedSfixed32Iterable(1, [fakeSfixed32]))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addUnpackedSfixed32Iterable(1, [fakeSfixed32]);
      expectQualifiedIterable(accessor.getRepeatedSfixed32Iterable(1));
    }
  });

  it('fail when adding single unpacked sfixed32 value with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeSfixed32 = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addUnpackedSfixed32Element(1, fakeSfixed32))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addUnpackedSfixed32Element(1, fakeSfixed32);
      expectQualifiedIterable(accessor.getRepeatedSfixed32Iterable(1));
    }
  });

  it('fail when setting unpacked sfixed32 values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeSfixed32 = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setUnpackedSfixed32Iterable(1, [fakeSfixed32]))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setUnpackedSfixed32Iterable(1, [fakeSfixed32]);
      expectQualifiedIterable(accessor.getRepeatedSfixed32Iterable(1));
    }
  });

  it('fail when setting single unpacked sfixed32 value with null value', () => {
    const accessor =
        Kernel.fromArrayBuffer(createArrayBuffer(0x08, 0x80, 0x80, 0x80, 0x00));
    const fakeSfixed32 = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setUnpackedSfixed32Element(1, 0, fakeSfixed32))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setUnpackedSfixed32Element(1, 0, fakeSfixed32);
      expectQualifiedIterable(accessor.getRepeatedSfixed32Iterable(1));
    }
  });

  it('fail when adding packed sfixed32 values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeSfixed32 = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addPackedSfixed32Iterable(1, [fakeSfixed32]))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addPackedSfixed32Iterable(1, [fakeSfixed32]);
      expectQualifiedIterable(accessor.getRepeatedSfixed32Iterable(1));
    }
  });

  it('fail when adding single packed sfixed32 value with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeSfixed32 = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addPackedSfixed32Element(1, fakeSfixed32))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addPackedSfixed32Element(1, fakeSfixed32);
      expectQualifiedIterable(accessor.getRepeatedSfixed32Iterable(1));
    }
  });

  it('fail when setting packed sfixed32 values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeSfixed32 = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setPackedSfixed32Iterable(1, [fakeSfixed32]))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setPackedSfixed32Iterable(1, [fakeSfixed32]);
      expectQualifiedIterable(accessor.getRepeatedSfixed32Iterable(1));
    }
  });

  it('fail when setting single packed sfixed32 value with null value', () => {
    const accessor =
        Kernel.fromArrayBuffer(createArrayBuffer(0x0D, 0x00, 0x00, 0x00, 0x00));
    const fakeSfixed32 = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setPackedSfixed32Element(1, 0, fakeSfixed32))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setPackedSfixed32Element(1, 0, fakeSfixed32);
      expectQualifiedIterable(accessor.getRepeatedSfixed32Iterable(1));
    }
  });

  it('fail when setting single unpacked with out-of-bound index', () => {
    const accessor =
        Kernel.fromArrayBuffer(createArrayBuffer(0x0D, 0x00, 0x00, 0x00, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setUnpackedSfixed32Element(1, 1, 1))
          .toThrowError('Index out of bounds: index: 1 size: 1');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setUnpackedSfixed32Element(1, 1, 1);
      expectQualifiedIterable(
          accessor.getRepeatedSfixed32Iterable(1),
          (value) => typeof value === 'number');
    }
  });

  it('fail when setting single packed with out-of-bound index', () => {
    const accessor =
        Kernel.fromArrayBuffer(createArrayBuffer(0x0D, 0x00, 0x00, 0x00, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setPackedSfixed32Element(1, 1, 1))
          .toThrowError('Index out of bounds: index: 1 size: 1');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setPackedSfixed32Element(1, 1, 1);
      expectQualifiedIterable(
          accessor.getRepeatedSfixed32Iterable(1),
          (value) => typeof value === 'number');
    }
  });

  it('fail when getting element with out-of-range index', () => {
    const accessor = Kernel.createEmpty();
    if (CHECK_CRITICAL_STATE) {
      expect(() => {
        accessor.getRepeatedSfixed32Element(
            /* fieldNumber= */ 1, /* index= */ 0);
      }).toThrowError('Index out of bounds: index: 0 size: 0');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      expect(accessor.getRepeatedSfixed32Element(
                 /* fieldNumber= */ 1, /* index= */ 0))
          .toBe(undefined);
    }
  });
});

describe('Kernel for repeated sfixed64 does', () => {
  const value1 = Int64.fromInt(1);
  const value2 = Int64.fromInt(0);

  const unpackedValue1Value2 = createArrayBuffer(
      0x09,
      0x01,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,  // value1
      0x09,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,  // value2
  );
  const unpackedValue2Value1 = createArrayBuffer(
      0x09,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,  // value1
      0x09,
      0x01,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,  // value2
  );

  const packedValue1Value2 = createArrayBuffer(
      0x0A,
      0x10,  // tag
      0x01,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,  // value1
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,  // value2
  );
  const packedValue2Value1 = createArrayBuffer(
      0x0A,
      0x10,  // tag
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,  // value2
      0x01,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,  // value1
  );

  it('return empty array for the empty input', () => {
    const accessor = Kernel.createEmpty();

    const list = accessor.getRepeatedSfixed64Iterable(1);

    expectEqualToArray(list, []);
  });

  it('ensure not the same instance returned for the empty input', () => {
    const accessor = Kernel.createEmpty();

    const list1 = accessor.getRepeatedSfixed64Iterable(1);
    const list2 = accessor.getRepeatedSfixed64Iterable(1);

    expect(list1).not.toBe(list2);
  });

  it('return size for the empty input', () => {
    const accessor = Kernel.createEmpty();

    const size = accessor.getRepeatedSfixed64Size(1);

    expect(size).toEqual(0);
  });

  it('return unpacked values from the input', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const list = accessor.getRepeatedSfixed64Iterable(1);

    expectEqualToArray(list, [value1, value2]);
  });

  it('ensure not the same instance returned for unpacked values', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const list1 = accessor.getRepeatedSfixed64Iterable(1);
    const list2 = accessor.getRepeatedSfixed64Iterable(1);

    expect(list1).not.toBe(list2);
  });

  it('add single unpacked value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedSfixed64Element(1, value1);
    const list1 = accessor.getRepeatedSfixed64Iterable(1);
    accessor.addUnpackedSfixed64Element(1, value2);
    const list2 = accessor.getRepeatedSfixed64Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('add unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedSfixed64Iterable(1, [value1]);
    const list1 = accessor.getRepeatedSfixed64Iterable(1);
    accessor.addUnpackedSfixed64Iterable(1, [value2]);
    const list2 = accessor.getRepeatedSfixed64Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('set a single unpacked value', () => {
    const accessor = Kernel.fromArrayBuffer(packedValue1Value2);

    accessor.setUnpackedSfixed64Element(1, 1, value1);
    const list = accessor.getRepeatedSfixed64Iterable(1);

    expectEqualToArray(list, [value1, value1]);
  });

  it('set unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setUnpackedSfixed64Iterable(1, [value1]);
    const list = accessor.getRepeatedSfixed64Iterable(1);

    expectEqualToArray(list, [value1]);
  });

  it('encode for adding single unpacked value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedSfixed64Element(1, value1);
    accessor.addUnpackedSfixed64Element(1, value2);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue1Value2);
  });

  it('encode for adding unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedSfixed64Iterable(1, [value1]);
    accessor.addUnpackedSfixed64Iterable(1, [value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue1Value2);
  });

  it('encode for setting single unpacked value', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    accessor.setUnpackedSfixed64Element(1, 0, value2);
    accessor.setUnpackedSfixed64Element(1, 1, value1);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue2Value1);
  });

  it('encode for setting unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setUnpackedSfixed64Iterable(1, [value1, value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue1Value2);
  });

  it('return packed values from the input', () => {
    const accessor = Kernel.fromArrayBuffer(packedValue1Value2);

    const list = accessor.getRepeatedSfixed64Iterable(1);

    expectEqualToArray(list, [value1, value2]);
  });

  it('ensure not the same instance returned for packed values', () => {
    const accessor = Kernel.fromArrayBuffer(packedValue1Value2);

    const list1 = accessor.getRepeatedSfixed64Iterable(1);
    const list2 = accessor.getRepeatedSfixed64Iterable(1);

    expect(list1).not.toBe(list2);
  });

  it('add single packed value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedSfixed64Element(1, value1);
    const list1 = accessor.getRepeatedSfixed64Iterable(1);
    accessor.addPackedSfixed64Element(1, value2);
    const list2 = accessor.getRepeatedSfixed64Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('add packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedSfixed64Iterable(1, [value1]);
    const list1 = accessor.getRepeatedSfixed64Iterable(1);
    accessor.addPackedSfixed64Iterable(1, [value2]);
    const list2 = accessor.getRepeatedSfixed64Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('set a single packed value', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    accessor.setPackedSfixed64Element(1, 1, value1);
    const list = accessor.getRepeatedSfixed64Iterable(1);

    expectEqualToArray(list, [value1, value1]);
  });

  it('set packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setPackedSfixed64Iterable(1, [value1]);
    const list1 = accessor.getRepeatedSfixed64Iterable(1);
    accessor.setPackedSfixed64Iterable(1, [value2]);
    const list2 = accessor.getRepeatedSfixed64Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value2]);
  });

  it('encode for adding single packed value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedSfixed64Element(1, value1);
    accessor.addPackedSfixed64Element(1, value2);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue1Value2);
  });

  it('encode for adding packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedSfixed64Iterable(1, [value1, value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue1Value2);
  });

  it('encode for setting single packed value', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    accessor.setPackedSfixed64Element(1, 0, value2);
    accessor.setPackedSfixed64Element(1, 1, value1);

    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue2Value1);
  });

  it('encode for setting packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setPackedSfixed64Iterable(1, [value1, value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue1Value2);
  });

  it('return combined values from the input', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(
        0x09, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // value1
        0x0A, 0x10,                                            // tag
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,        // value1
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,        // value2
        0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00   // value2
        ));

    const list = accessor.getRepeatedSfixed64Iterable(1);

    expectEqualToArray(list, [value1, value1, value2, value2]);
  });

  it('return the repeated field element from the input', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const result1 = accessor.getRepeatedSfixed64Element(
        /* fieldNumber= */ 1, /* index= */ 0);
    const result2 = accessor.getRepeatedSfixed64Element(
        /* fieldNumber= */ 1, /* index= */ 1);

    expect(result1).toEqual(value1);
    expect(result2).toEqual(value2);
  });

  it('return the size from the input', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const size = accessor.getRepeatedSfixed64Size(1);

    expect(size).toEqual(2);
  });

  it('fail when getting unpacked sfixed64 value with other wire types', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(
        0x08, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => {
        accessor.getRepeatedSfixed64Iterable(1);
      }).toThrowError('Expected wire type: 1 but found: 0');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      expectQualifiedIterable(
          accessor.getRepeatedSfixed64Iterable(1),
          (value) => value instanceof Int64);
    }
  });

  it('fail when adding unpacked sfixed64 values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeSfixed64 = /** @type {!Int64} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addUnpackedSfixed64Iterable(1, [fakeSfixed64]))
          .toThrowError('Must be Int64 instance, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addUnpackedSfixed64Iterable(1, [fakeSfixed64]);
      expectQualifiedIterable(accessor.getRepeatedSfixed64Iterable(1));
    }
  });

  it('fail when adding single unpacked sfixed64 value with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeSfixed64 = /** @type {!Int64} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addUnpackedSfixed64Element(1, fakeSfixed64))
          .toThrowError('Must be Int64 instance, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addUnpackedSfixed64Element(1, fakeSfixed64);
      expectQualifiedIterable(accessor.getRepeatedSfixed64Iterable(1));
    }
  });

  it('fail when setting unpacked sfixed64 values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeSfixed64 = /** @type {!Int64} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setUnpackedSfixed64Iterable(1, [fakeSfixed64]))
          .toThrowError('Must be Int64 instance, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setUnpackedSfixed64Iterable(1, [fakeSfixed64]);
      expectQualifiedIterable(accessor.getRepeatedSfixed64Iterable(1));
    }
  });

  it('fail when setting single unpacked sfixed64 value with null value', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(
        0x08, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00));
    const fakeSfixed64 = /** @type {!Int64} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setUnpackedSfixed64Element(1, 0, fakeSfixed64))
          .toThrowError('Must be Int64 instance, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setUnpackedSfixed64Element(1, 0, fakeSfixed64);
      expectQualifiedIterable(accessor.getRepeatedSfixed64Iterable(1));
    }
  });

  it('fail when adding packed sfixed64 values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeSfixed64 = /** @type {!Int64} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addPackedSfixed64Iterable(1, [fakeSfixed64]))
          .toThrowError('Must be Int64 instance, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addPackedSfixed64Iterable(1, [fakeSfixed64]);
      expectQualifiedIterable(accessor.getRepeatedSfixed64Iterable(1));
    }
  });

  it('fail when adding single packed sfixed64 value with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeSfixed64 = /** @type {!Int64} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addPackedSfixed64Element(1, fakeSfixed64))
          .toThrowError('Must be Int64 instance, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addPackedSfixed64Element(1, fakeSfixed64);
      expectQualifiedIterable(accessor.getRepeatedSfixed64Iterable(1));
    }
  });

  it('fail when setting packed sfixed64 values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeSfixed64 = /** @type {!Int64} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setPackedSfixed64Iterable(1, [fakeSfixed64]))
          .toThrowError('Must be Int64 instance, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setPackedSfixed64Iterable(1, [fakeSfixed64]);
      expectQualifiedIterable(accessor.getRepeatedSfixed64Iterable(1));
    }
  });

  it('fail when setting single packed sfixed64 value with null value', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(
        0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00));
    const fakeSfixed64 = /** @type {!Int64} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setPackedSfixed64Element(1, 0, fakeSfixed64))
          .toThrowError('Must be Int64 instance, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setPackedSfixed64Element(1, 0, fakeSfixed64);
      expectQualifiedIterable(accessor.getRepeatedSfixed64Iterable(1));
    }
  });

  it('fail when setting single unpacked with out-of-bound index', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(
        0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setUnpackedSfixed64Element(1, 1, Int64.fromInt(1)))
          .toThrowError('Index out of bounds: index: 1 size: 1');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setUnpackedSfixed64Element(1, 1, Int64.fromInt(1));
      expectQualifiedIterable(
          accessor.getRepeatedSfixed64Iterable(1),
          (value) => value instanceof Int64);
    }
  });

  it('fail when setting single packed with out-of-bound index', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(
        0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setPackedSfixed64Element(1, 1, Int64.fromInt(1)))
          .toThrowError('Index out of bounds: index: 1 size: 1');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setPackedSfixed64Element(1, 1, Int64.fromInt(1));
      expectQualifiedIterable(
          accessor.getRepeatedSfixed64Iterable(1),
          (value) => value instanceof Int64);
    }
  });

  it('fail when getting element with out-of-range index', () => {
    const accessor = Kernel.createEmpty();
    if (CHECK_CRITICAL_STATE) {
      expect(() => {
        accessor.getRepeatedSfixed64Element(
            /* fieldNumber= */ 1, /* index= */ 0);
      }).toThrowError('Index out of bounds: index: 0 size: 0');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      expect(accessor.getRepeatedSfixed64Element(
                 /* fieldNumber= */ 1, /* index= */ 0))
          .toBe(undefined);
    }
  });
});

describe('Kernel for repeated sint32 does', () => {
  const value1 = -1;
  const value2 = 0;

  const unpackedValue1Value2 = createArrayBuffer(0x08, 0x01, 0x08, 0x00);
  const unpackedValue2Value1 = createArrayBuffer(0x08, 0x00, 0x08, 0x01);

  const packedValue1Value2 = createArrayBuffer(0x0A, 0x02, 0x01, 0x00);
  const packedValue2Value1 = createArrayBuffer(0x0A, 0x02, 0x00, 0x01);

  it('return empty array for the empty input', () => {
    const accessor = Kernel.createEmpty();

    const list = accessor.getRepeatedSint32Iterable(1);

    expectEqualToArray(list, []);
  });

  it('ensure not the same instance returned for the empty input', () => {
    const accessor = Kernel.createEmpty();

    const list1 = accessor.getRepeatedSint32Iterable(1);
    const list2 = accessor.getRepeatedSint32Iterable(1);

    expect(list1).not.toBe(list2);
  });

  it('return size for the empty input', () => {
    const accessor = Kernel.createEmpty();

    const size = accessor.getRepeatedSint32Size(1);

    expect(size).toEqual(0);
  });

  it('return unpacked values from the input', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const list = accessor.getRepeatedSint32Iterable(1);

    expectEqualToArray(list, [value1, value2]);
  });

  it('ensure not the same instance returned for unpacked values', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const list1 = accessor.getRepeatedSint32Iterable(1);
    const list2 = accessor.getRepeatedSint32Iterable(1);

    expect(list1).not.toBe(list2);
  });

  it('add single unpacked value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedSint32Element(1, value1);
    const list1 = accessor.getRepeatedSint32Iterable(1);
    accessor.addUnpackedSint32Element(1, value2);
    const list2 = accessor.getRepeatedSint32Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('add unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedSint32Iterable(1, [value1]);
    const list1 = accessor.getRepeatedSint32Iterable(1);
    accessor.addUnpackedSint32Iterable(1, [value2]);
    const list2 = accessor.getRepeatedSint32Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('set a single unpacked value', () => {
    const accessor = Kernel.fromArrayBuffer(packedValue1Value2);

    accessor.setUnpackedSint32Element(1, 1, value1);
    const list = accessor.getRepeatedSint32Iterable(1);

    expectEqualToArray(list, [value1, value1]);
  });

  it('set unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setUnpackedSint32Iterable(1, [value1]);
    const list = accessor.getRepeatedSint32Iterable(1);

    expectEqualToArray(list, [value1]);
  });

  it('encode for adding single unpacked value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedSint32Element(1, value1);
    accessor.addUnpackedSint32Element(1, value2);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue1Value2);
  });

  it('encode for adding unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedSint32Iterable(1, [value1]);
    accessor.addUnpackedSint32Iterable(1, [value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue1Value2);
  });

  it('encode for setting single unpacked value', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    accessor.setUnpackedSint32Element(1, 0, value2);
    accessor.setUnpackedSint32Element(1, 1, value1);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue2Value1);
  });

  it('encode for setting unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setUnpackedSint32Iterable(1, [value1, value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue1Value2);
  });

  it('return packed values from the input', () => {
    const accessor = Kernel.fromArrayBuffer(packedValue1Value2);

    const list = accessor.getRepeatedSint32Iterable(1);

    expectEqualToArray(list, [value1, value2]);
  });

  it('ensure not the same instance returned for packed values', () => {
    const accessor = Kernel.fromArrayBuffer(packedValue1Value2);

    const list1 = accessor.getRepeatedSint32Iterable(1);
    const list2 = accessor.getRepeatedSint32Iterable(1);

    expect(list1).not.toBe(list2);
  });

  it('add single packed value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedSint32Element(1, value1);
    const list1 = accessor.getRepeatedSint32Iterable(1);
    accessor.addPackedSint32Element(1, value2);
    const list2 = accessor.getRepeatedSint32Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('add packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedSint32Iterable(1, [value1]);
    const list1 = accessor.getRepeatedSint32Iterable(1);
    accessor.addPackedSint32Iterable(1, [value2]);
    const list2 = accessor.getRepeatedSint32Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('set a single packed value', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    accessor.setPackedSint32Element(1, 1, value1);
    const list = accessor.getRepeatedSint32Iterable(1);

    expectEqualToArray(list, [value1, value1]);
  });

  it('set packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setPackedSint32Iterable(1, [value1]);
    const list1 = accessor.getRepeatedSint32Iterable(1);
    accessor.setPackedSint32Iterable(1, [value2]);
    const list2 = accessor.getRepeatedSint32Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value2]);
  });

  it('encode for adding single packed value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedSint32Element(1, value1);
    accessor.addPackedSint32Element(1, value2);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue1Value2);
  });

  it('encode for adding packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedSint32Iterable(1, [value1, value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue1Value2);
  });

  it('encode for setting single packed value', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    accessor.setPackedSint32Element(1, 0, value2);
    accessor.setPackedSint32Element(1, 1, value1);

    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue2Value1);
  });

  it('encode for setting packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setPackedSint32Iterable(1, [value1, value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue1Value2);
  });

  it('return combined values from the input', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(
        0x08,
        0x01,  // unpacked value1
        0x0A,
        0x02,
        0x01,
        0x00,  // packed value1 and value2
        0x08,
        0x00,  // unpacked value2
        ));

    const list = accessor.getRepeatedSint32Iterable(1);

    expectEqualToArray(list, [value1, value1, value2, value2]);
  });

  it('return the repeated field element from the input', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const result1 = accessor.getRepeatedSint32Element(
        /* fieldNumber= */ 1, /* index= */ 0);
    const result2 = accessor.getRepeatedSint32Element(
        /* fieldNumber= */ 1, /* index= */ 1);

    expect(result1).toEqual(value1);
    expect(result2).toEqual(value2);
  });

  it('return the size from the input', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const size = accessor.getRepeatedSint32Size(1);

    expect(size).toEqual(2);
  });

  it('fail when getting unpacked sint32 value with other wire types', () => {
    const accessor =
        Kernel.fromArrayBuffer(createArrayBuffer(0x0D, 0x80, 0x80, 0x80, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => {
        accessor.getRepeatedSint32Iterable(1);
      }).toThrowError('Expected wire type: 0 but found: 5');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      expectQualifiedIterable(
          accessor.getRepeatedSint32Iterable(1),
          (value) => Number.isInteger(value));
    }
  });

  it('fail when adding unpacked sint32 values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeSint32 = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addUnpackedSint32Iterable(1, [fakeSint32]))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addUnpackedSint32Iterable(1, [fakeSint32]);
      expectQualifiedIterable(accessor.getRepeatedSint32Iterable(1));
    }
  });

  it('fail when adding single unpacked sint32 value with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeSint32 = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addUnpackedSint32Element(1, fakeSint32))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addUnpackedSint32Element(1, fakeSint32);
      expectQualifiedIterable(accessor.getRepeatedSint32Iterable(1));
    }
  });

  it('fail when setting unpacked sint32 values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeSint32 = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setUnpackedSint32Iterable(1, [fakeSint32]))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setUnpackedSint32Iterable(1, [fakeSint32]);
      expectQualifiedIterable(accessor.getRepeatedSint32Iterable(1));
    }
  });

  it('fail when setting single unpacked sint32 value with null value', () => {
    const accessor =
        Kernel.fromArrayBuffer(createArrayBuffer(0x0D, 0x80, 0x80, 0x80, 0x00));
    const fakeSint32 = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setUnpackedSint32Element(1, 0, fakeSint32))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setUnpackedSint32Element(1, 0, fakeSint32);
      expectQualifiedIterable(
          accessor.getRepeatedSint32Iterable(1),
      );
    }
  });

  it('fail when adding packed sint32 values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeSint32 = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addPackedSint32Iterable(1, [fakeSint32]))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addPackedSint32Iterable(1, [fakeSint32]);
      expectQualifiedIterable(accessor.getRepeatedSint32Iterable(1));
    }
  });

  it('fail when adding single packed sint32 value with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeSint32 = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addPackedSint32Element(1, fakeSint32))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addPackedSint32Element(1, fakeSint32);
      expectQualifiedIterable(accessor.getRepeatedSint32Iterable(1));
    }
  });

  it('fail when setting packed sint32 values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeSint32 = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setPackedSint32Iterable(1, [fakeSint32]))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setPackedSint32Iterable(1, [fakeSint32]);
      expectQualifiedIterable(accessor.getRepeatedSint32Iterable(1));
    }
  });

  it('fail when setting single packed sint32 value with null value', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(0x08, 0x00));
    const fakeSint32 = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setPackedSint32Element(1, 0, fakeSint32))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setPackedSint32Element(1, 0, fakeSint32);
      expectQualifiedIterable(accessor.getRepeatedSint32Iterable(1));
    }
  });

  it('fail when setting single unpacked with out-of-bound index', () => {
    const accessor =
        Kernel.fromArrayBuffer(createArrayBuffer(0x0A, 0x01, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setUnpackedSint32Element(1, 1, 1))
          .toThrowError('Index out of bounds: index: 1 size: 1');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setUnpackedSint32Element(1, 1, 1);
      expectQualifiedIterable(
          accessor.getRepeatedSint32Iterable(1),
          (value) => Number.isInteger(value));
    }
  });

  it('fail when setting single packed with out-of-bound index', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(0x08, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setPackedSint32Element(1, 1, 1))
          .toThrowError('Index out of bounds: index: 1 size: 1');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setPackedSint32Element(1, 1, 1);
      expectQualifiedIterable(
          accessor.getRepeatedSint32Iterable(1),
          (value) => Number.isInteger(value));
    }
  });

  it('fail when getting element with out-of-range index', () => {
    const accessor = Kernel.createEmpty();
    if (CHECK_CRITICAL_STATE) {
      expect(() => {
        accessor.getRepeatedSint32Element(
            /* fieldNumber= */ 1, /* index= */ 0);
      }).toThrowError('Index out of bounds: index: 0 size: 0');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      expect(accessor.getRepeatedSint32Element(
                 /* fieldNumber= */ 1, /* index= */ 0))
          .toBe(undefined);
    }
  });
});

describe('Kernel for repeated sint64 does', () => {
  const value1 = Int64.fromInt(-1);
  const value2 = Int64.fromInt(0);

  const unpackedValue1Value2 = createArrayBuffer(0x08, 0x01, 0x08, 0x00);
  const unpackedValue2Value1 = createArrayBuffer(0x08, 0x00, 0x08, 0x01);

  const packedValue1Value2 = createArrayBuffer(0x0A, 0x02, 0x01, 0x00);
  const packedValue2Value1 = createArrayBuffer(0x0A, 0x02, 0x00, 0x01);

  it('return empty array for the empty input', () => {
    const accessor = Kernel.createEmpty();

    const list = accessor.getRepeatedSint64Iterable(1);

    expectEqualToArray(list, []);
  });

  it('ensure not the same instance returned for the empty input', () => {
    const accessor = Kernel.createEmpty();

    const list1 = accessor.getRepeatedSint64Iterable(1);
    const list2 = accessor.getRepeatedSint64Iterable(1);

    expect(list1).not.toBe(list2);
  });

  it('return size for the empty input', () => {
    const accessor = Kernel.createEmpty();

    const size = accessor.getRepeatedSint64Size(1);

    expect(size).toEqual(0);
  });

  it('return unpacked values from the input', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const list = accessor.getRepeatedSint64Iterable(1);

    expectEqualToArray(list, [value1, value2]);
  });

  it('ensure not the same instance returned for unpacked values', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const list1 = accessor.getRepeatedSint64Iterable(1);
    const list2 = accessor.getRepeatedSint64Iterable(1);

    expect(list1).not.toBe(list2);
  });

  it('add single unpacked value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedSint64Element(1, value1);
    const list1 = accessor.getRepeatedSint64Iterable(1);
    accessor.addUnpackedSint64Element(1, value2);
    const list2 = accessor.getRepeatedSint64Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('add unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedSint64Iterable(1, [value1]);
    const list1 = accessor.getRepeatedSint64Iterable(1);
    accessor.addUnpackedSint64Iterable(1, [value2]);
    const list2 = accessor.getRepeatedSint64Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('set a single unpacked value', () => {
    const accessor = Kernel.fromArrayBuffer(packedValue1Value2);

    accessor.setUnpackedSint64Element(1, 1, value1);
    const list = accessor.getRepeatedSint64Iterable(1);

    expectEqualToArray(list, [value1, value1]);
  });

  it('set unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setUnpackedSint64Iterable(1, [value1]);
    const list = accessor.getRepeatedSint64Iterable(1);

    expectEqualToArray(list, [value1]);
  });

  it('encode for adding single unpacked value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedSint64Element(1, value1);
    accessor.addUnpackedSint64Element(1, value2);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue1Value2);
  });

  it('encode for adding unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedSint64Iterable(1, [value1]);
    accessor.addUnpackedSint64Iterable(1, [value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue1Value2);
  });

  it('encode for setting single unpacked value', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    accessor.setUnpackedSint64Element(1, 0, value2);
    accessor.setUnpackedSint64Element(1, 1, value1);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue2Value1);
  });

  it('encode for setting unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setUnpackedSint64Iterable(1, [value1, value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue1Value2);
  });

  it('return packed values from the input', () => {
    const accessor = Kernel.fromArrayBuffer(packedValue1Value2);

    const list = accessor.getRepeatedSint64Iterable(1);

    expectEqualToArray(list, [value1, value2]);
  });

  it('ensure not the same instance returned for packed values', () => {
    const accessor = Kernel.fromArrayBuffer(packedValue1Value2);

    const list1 = accessor.getRepeatedSint64Iterable(1);
    const list2 = accessor.getRepeatedSint64Iterable(1);

    expect(list1).not.toBe(list2);
  });

  it('add single packed value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedSint64Element(1, value1);
    const list1 = accessor.getRepeatedSint64Iterable(1);
    accessor.addPackedSint64Element(1, value2);
    const list2 = accessor.getRepeatedSint64Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('add packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedSint64Iterable(1, [value1]);
    const list1 = accessor.getRepeatedSint64Iterable(1);
    accessor.addPackedSint64Iterable(1, [value2]);
    const list2 = accessor.getRepeatedSint64Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('set a single packed value', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    accessor.setPackedSint64Element(1, 1, value1);
    const list = accessor.getRepeatedSint64Iterable(1);

    expectEqualToArray(list, [value1, value1]);
  });

  it('set packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setPackedSint64Iterable(1, [value1]);
    const list1 = accessor.getRepeatedSint64Iterable(1);
    accessor.setPackedSint64Iterable(1, [value2]);
    const list2 = accessor.getRepeatedSint64Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value2]);
  });

  it('encode for adding single packed value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedSint64Element(1, value1);
    accessor.addPackedSint64Element(1, value2);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue1Value2);
  });

  it('encode for adding packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedSint64Iterable(1, [value1, value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue1Value2);
  });

  it('encode for setting single packed value', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    accessor.setPackedSint64Element(1, 0, value2);
    accessor.setPackedSint64Element(1, 1, value1);

    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue2Value1);
  });

  it('encode for setting packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setPackedSint64Iterable(1, [value1, value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue1Value2);
  });

  it('return combined values from the input', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(
        0x08,
        0x01,  // unpacked value1
        0x0A,
        0x02,
        0x01,
        0x00,  // packed value1 and value2
        0x08,
        0x00,  // unpacked value2
        ));

    const list = accessor.getRepeatedSint64Iterable(1);

    expectEqualToArray(list, [value1, value1, value2, value2]);
  });

  it('return the repeated field element from the input', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const result1 = accessor.getRepeatedSint64Element(
        /* fieldNumber= */ 1, /* index= */ 0);
    const result2 = accessor.getRepeatedSint64Element(
        /* fieldNumber= */ 1, /* index= */ 1);

    expect(result1).toEqual(value1);
    expect(result2).toEqual(value2);
  });

  it('return the size from the input', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const size = accessor.getRepeatedSint64Size(1);

    expect(size).toEqual(2);
  });

  it('fail when getting unpacked sint64 value with other wire types', () => {
    const accessor =
        Kernel.fromArrayBuffer(createArrayBuffer(0x0D, 0x80, 0x80, 0x80, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => {
        accessor.getRepeatedSint64Iterable(1);
      }).toThrowError('Expected wire type: 0 but found: 5');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      expectQualifiedIterable(
          accessor.getRepeatedSint64Iterable(1),
          (value) => value instanceof Int64);
    }
  });

  it('fail when adding unpacked sint64 values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeSint64 = /** @type {!Int64} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addUnpackedSint64Iterable(1, [fakeSint64]))
          .toThrowError('Must be Int64 instance, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addUnpackedSint64Iterable(1, [fakeSint64]);
      expectQualifiedIterable(accessor.getRepeatedSint64Iterable(1));
    }
  });

  it('fail when adding single unpacked sint64 value with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeSint64 = /** @type {!Int64} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addUnpackedSint64Element(1, fakeSint64))
          .toThrowError('Must be Int64 instance, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addUnpackedSint64Element(1, fakeSint64);
      expectQualifiedIterable(accessor.getRepeatedSint64Iterable(1));
    }
  });

  it('fail when setting unpacked sint64 values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeSint64 = /** @type {!Int64} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setUnpackedSint64Iterable(1, [fakeSint64]))
          .toThrowError('Must be Int64 instance, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setUnpackedSint64Iterable(1, [fakeSint64]);
      expectQualifiedIterable(accessor.getRepeatedSint64Iterable(1));
    }
  });

  it('fail when setting single unpacked sint64 value with null value', () => {
    const accessor =
        Kernel.fromArrayBuffer(createArrayBuffer(0x0D, 0x80, 0x80, 0x80, 0x00));
    const fakeSint64 = /** @type {!Int64} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setUnpackedSint64Element(1, 0, fakeSint64))
          .toThrowError('Must be Int64 instance, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setUnpackedSint64Element(1, 0, fakeSint64);
      expectQualifiedIterable(accessor.getRepeatedSint64Iterable(1));
    }
  });

  it('fail when adding packed sint64 values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeSint64 = /** @type {!Int64} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addPackedSint64Iterable(1, [fakeSint64]))
          .toThrowError('Must be Int64 instance, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addPackedSint64Iterable(1, [fakeSint64]);
      expectQualifiedIterable(accessor.getRepeatedSint64Iterable(1));
    }
  });

  it('fail when adding single packed sint64 value with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeSint64 = /** @type {!Int64} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addPackedSint64Element(1, fakeSint64))
          .toThrowError('Must be Int64 instance, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addPackedSint64Element(1, fakeSint64);
      expectQualifiedIterable(accessor.getRepeatedSint64Iterable(1));
    }
  });

  it('fail when setting packed sint64 values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeSint64 = /** @type {!Int64} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setPackedSint64Iterable(1, [fakeSint64]))
          .toThrowError('Must be Int64 instance, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setPackedSint64Iterable(1, [fakeSint64]);
      expectQualifiedIterable(accessor.getRepeatedSint64Iterable(1));
    }
  });

  it('fail when setting single packed sint64 value with null value', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(0x08, 0x00));
    const fakeSint64 = /** @type {!Int64} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setPackedSint64Element(1, 0, fakeSint64))
          .toThrowError('Must be Int64 instance, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setPackedSint64Element(1, 0, fakeSint64);
      expectQualifiedIterable(accessor.getRepeatedSint64Iterable(1));
    }
  });

  it('fail when setting single unpacked with out-of-bound index', () => {
    const accessor =
        Kernel.fromArrayBuffer(createArrayBuffer(0x0A, 0x01, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setUnpackedSint64Element(1, 1, Int64.fromInt(1)))
          .toThrowError('Index out of bounds: index: 1 size: 1');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setUnpackedSint64Element(1, 1, Int64.fromInt(1));
      expectQualifiedIterable(
          accessor.getRepeatedSint64Iterable(1),
          (value) => value instanceof Int64);
    }
  });

  it('fail when setting single packed with out-of-bound index', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(0x08, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setPackedSint64Element(1, 1, Int64.fromInt(1)))
          .toThrowError('Index out of bounds: index: 1 size: 1');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setPackedSint64Element(1, 1, Int64.fromInt(1));
      expectQualifiedIterable(
          accessor.getRepeatedSint64Iterable(1),
          (value) => value instanceof Int64);
    }
  });

  it('fail when getting element with out-of-range index', () => {
    const accessor = Kernel.createEmpty();
    if (CHECK_CRITICAL_STATE) {
      expect(() => {
        accessor.getRepeatedSint64Element(
            /* fieldNumber= */ 1, /* index= */ 0);
      }).toThrowError('Index out of bounds: index: 0 size: 0');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      expect(accessor.getRepeatedSint64Element(
                 /* fieldNumber= */ 1, /* index= */ 0))
          .toBe(undefined);
    }
  });
});

describe('Kernel for repeated uint32 does', () => {
  const value1 = 1;
  const value2 = 0;

  const unpackedValue1Value2 = createArrayBuffer(0x08, 0x01, 0x08, 0x00);
  const unpackedValue2Value1 = createArrayBuffer(0x08, 0x00, 0x08, 0x01);

  const packedValue1Value2 = createArrayBuffer(0x0A, 0x02, 0x01, 0x00);
  const packedValue2Value1 = createArrayBuffer(0x0A, 0x02, 0x00, 0x01);

  it('return empty array for the empty input', () => {
    const accessor = Kernel.createEmpty();

    const list = accessor.getRepeatedUint32Iterable(1);

    expectEqualToArray(list, []);
  });

  it('ensure not the same instance returned for the empty input', () => {
    const accessor = Kernel.createEmpty();

    const list1 = accessor.getRepeatedUint32Iterable(1);
    const list2 = accessor.getRepeatedUint32Iterable(1);

    expect(list1).not.toBe(list2);
  });

  it('return size for the empty input', () => {
    const accessor = Kernel.createEmpty();

    const size = accessor.getRepeatedUint32Size(1);

    expect(size).toEqual(0);
  });

  it('return unpacked values from the input', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const list = accessor.getRepeatedUint32Iterable(1);

    expectEqualToArray(list, [value1, value2]);
  });

  it('ensure not the same instance returned for unpacked values', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const list1 = accessor.getRepeatedUint32Iterable(1);
    const list2 = accessor.getRepeatedUint32Iterable(1);

    expect(list1).not.toBe(list2);
  });

  it('add single unpacked value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedUint32Element(1, value1);
    const list1 = accessor.getRepeatedUint32Iterable(1);
    accessor.addUnpackedUint32Element(1, value2);
    const list2 = accessor.getRepeatedUint32Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('add unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedUint32Iterable(1, [value1]);
    const list1 = accessor.getRepeatedUint32Iterable(1);
    accessor.addUnpackedUint32Iterable(1, [value2]);
    const list2 = accessor.getRepeatedUint32Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('set a single unpacked value', () => {
    const accessor = Kernel.fromArrayBuffer(packedValue1Value2);

    accessor.setUnpackedUint32Element(1, 1, value1);
    const list = accessor.getRepeatedUint32Iterable(1);

    expectEqualToArray(list, [value1, value1]);
  });

  it('set unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setUnpackedUint32Iterable(1, [value1]);
    const list = accessor.getRepeatedUint32Iterable(1);

    expectEqualToArray(list, [value1]);
  });

  it('encode for adding single unpacked value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedUint32Element(1, value1);
    accessor.addUnpackedUint32Element(1, value2);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue1Value2);
  });

  it('encode for adding unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedUint32Iterable(1, [value1]);
    accessor.addUnpackedUint32Iterable(1, [value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue1Value2);
  });

  it('encode for setting single unpacked value', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    accessor.setUnpackedUint32Element(1, 0, value2);
    accessor.setUnpackedUint32Element(1, 1, value1);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue2Value1);
  });

  it('encode for setting unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setUnpackedUint32Iterable(1, [value1, value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue1Value2);
  });

  it('return packed values from the input', () => {
    const accessor = Kernel.fromArrayBuffer(packedValue1Value2);

    const list = accessor.getRepeatedUint32Iterable(1);

    expectEqualToArray(list, [value1, value2]);
  });

  it('ensure not the same instance returned for packed values', () => {
    const accessor = Kernel.fromArrayBuffer(packedValue1Value2);

    const list1 = accessor.getRepeatedUint32Iterable(1);
    const list2 = accessor.getRepeatedUint32Iterable(1);

    expect(list1).not.toBe(list2);
  });

  it('add single packed value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedUint32Element(1, value1);
    const list1 = accessor.getRepeatedUint32Iterable(1);
    accessor.addPackedUint32Element(1, value2);
    const list2 = accessor.getRepeatedUint32Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('add packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedUint32Iterable(1, [value1]);
    const list1 = accessor.getRepeatedUint32Iterable(1);
    accessor.addPackedUint32Iterable(1, [value2]);
    const list2 = accessor.getRepeatedUint32Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('set a single packed value', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    accessor.setPackedUint32Element(1, 1, value1);
    const list = accessor.getRepeatedUint32Iterable(1);

    expectEqualToArray(list, [value1, value1]);
  });

  it('set packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setPackedUint32Iterable(1, [value1]);
    const list1 = accessor.getRepeatedUint32Iterable(1);
    accessor.setPackedUint32Iterable(1, [value2]);
    const list2 = accessor.getRepeatedUint32Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value2]);
  });

  it('encode for adding single packed value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedUint32Element(1, value1);
    accessor.addPackedUint32Element(1, value2);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue1Value2);
  });

  it('encode for adding packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedUint32Iterable(1, [value1, value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue1Value2);
  });

  it('encode for setting single packed value', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    accessor.setPackedUint32Element(1, 0, value2);
    accessor.setPackedUint32Element(1, 1, value1);

    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue2Value1);
  });

  it('encode for setting packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setPackedUint32Iterable(1, [value1, value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue1Value2);
  });

  it('return combined values from the input', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(
        0x08,
        0x01,  // unpacked value1
        0x0A,
        0x02,
        0x01,
        0x00,  // packed value1 and value2
        0x08,
        0x00,  // unpacked value2
        ));

    const list = accessor.getRepeatedUint32Iterable(1);

    expectEqualToArray(list, [value1, value1, value2, value2]);
  });

  it('return the repeated field element from the input', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const result1 = accessor.getRepeatedUint32Element(
        /* fieldNumber= */ 1, /* index= */ 0);
    const result2 = accessor.getRepeatedUint32Element(
        /* fieldNumber= */ 1, /* index= */ 1);

    expect(result1).toEqual(value1);
    expect(result2).toEqual(value2);
  });

  it('return the size from the input', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const size = accessor.getRepeatedUint32Size(1);

    expect(size).toEqual(2);
  });

  it('fail when getting unpacked uint32 value with other wire types', () => {
    const accessor =
        Kernel.fromArrayBuffer(createArrayBuffer(0x0D, 0x80, 0x80, 0x80, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => {
        accessor.getRepeatedUint32Iterable(1);
      }).toThrowError('Expected wire type: 0 but found: 5');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      expectQualifiedIterable(
          accessor.getRepeatedUint32Iterable(1),
          (value) => Number.isInteger(value));
    }
  });

  it('fail when adding unpacked uint32 values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeUint32 = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addUnpackedUint32Iterable(1, [fakeUint32]))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addUnpackedUint32Iterable(1, [fakeUint32]);
      expectQualifiedIterable(accessor.getRepeatedUint32Iterable(1));
    }
  });

  it('fail when adding single unpacked uint32 value with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeUint32 = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addUnpackedUint32Element(1, fakeUint32))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addUnpackedUint32Element(1, fakeUint32);
      expectQualifiedIterable(accessor.getRepeatedUint32Iterable(1));
    }
  });

  it('fail when setting unpacked uint32 values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeUint32 = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setUnpackedUint32Iterable(1, [fakeUint32]))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setUnpackedUint32Iterable(1, [fakeUint32]);
      expectQualifiedIterable(accessor.getRepeatedUint32Iterable(1));
    }
  });

  it('fail when setting single unpacked uint32 value with null value', () => {
    const accessor =
        Kernel.fromArrayBuffer(createArrayBuffer(0x0D, 0x80, 0x80, 0x80, 0x00));
    const fakeUint32 = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setUnpackedUint32Element(1, 0, fakeUint32))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setUnpackedUint32Element(1, 0, fakeUint32);
      expectQualifiedIterable(
          accessor.getRepeatedUint32Iterable(1),
      );
    }
  });

  it('fail when adding packed uint32 values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeUint32 = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addPackedUint32Iterable(1, [fakeUint32]))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addPackedUint32Iterable(1, [fakeUint32]);
      expectQualifiedIterable(accessor.getRepeatedUint32Iterable(1));
    }
  });

  it('fail when adding single packed uint32 value with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeUint32 = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addPackedUint32Element(1, fakeUint32))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addPackedUint32Element(1, fakeUint32);
      expectQualifiedIterable(accessor.getRepeatedUint32Iterable(1));
    }
  });

  it('fail when setting packed uint32 values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeUint32 = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setPackedUint32Iterable(1, [fakeUint32]))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setPackedUint32Iterable(1, [fakeUint32]);
      expectQualifiedIterable(accessor.getRepeatedUint32Iterable(1));
    }
  });

  it('fail when setting single packed uint32 value with null value', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(0x08, 0x00));
    const fakeUint32 = /** @type {number} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setPackedUint32Element(1, 0, fakeUint32))
          .toThrowError('Must be a number, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setPackedUint32Element(1, 0, fakeUint32);
      expectQualifiedIterable(accessor.getRepeatedUint32Iterable(1));
    }
  });

  it('fail when setting single unpacked with out-of-bound index', () => {
    const accessor =
        Kernel.fromArrayBuffer(createArrayBuffer(0x0A, 0x01, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setUnpackedUint32Element(1, 1, 1))
          .toThrowError('Index out of bounds: index: 1 size: 1');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setUnpackedUint32Element(1, 1, 1);
      expectQualifiedIterable(
          accessor.getRepeatedUint32Iterable(1),
          (value) => Number.isInteger(value));
    }
  });

  it('fail when setting single packed with out-of-bound index', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(0x08, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setPackedUint32Element(1, 1, 1))
          .toThrowError('Index out of bounds: index: 1 size: 1');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setPackedUint32Element(1, 1, 1);
      expectQualifiedIterable(
          accessor.getRepeatedUint32Iterable(1),
          (value) => Number.isInteger(value));
    }
  });

  it('fail when getting element with out-of-range index', () => {
    const accessor = Kernel.createEmpty();
    if (CHECK_CRITICAL_STATE) {
      expect(() => {
        accessor.getRepeatedUint32Element(
            /* fieldNumber= */ 1, /* index= */ 0);
      }).toThrowError('Index out of bounds: index: 0 size: 0');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      expect(accessor.getRepeatedUint32Element(
                 /* fieldNumber= */ 1, /* index= */ 0))
          .toBe(undefined);
    }
  });
});

describe('Kernel for repeated uint64 does', () => {
  const value1 = Int64.fromInt(1);
  const value2 = Int64.fromInt(0);

  const unpackedValue1Value2 = createArrayBuffer(0x08, 0x01, 0x08, 0x00);
  const unpackedValue2Value1 = createArrayBuffer(0x08, 0x00, 0x08, 0x01);

  const packedValue1Value2 = createArrayBuffer(0x0A, 0x02, 0x01, 0x00);
  const packedValue2Value1 = createArrayBuffer(0x0A, 0x02, 0x00, 0x01);

  it('return empty array for the empty input', () => {
    const accessor = Kernel.createEmpty();

    const list = accessor.getRepeatedUint64Iterable(1);

    expectEqualToArray(list, []);
  });

  it('ensure not the same instance returned for the empty input', () => {
    const accessor = Kernel.createEmpty();

    const list1 = accessor.getRepeatedUint64Iterable(1);
    const list2 = accessor.getRepeatedUint64Iterable(1);

    expect(list1).not.toBe(list2);
  });

  it('return size for the empty input', () => {
    const accessor = Kernel.createEmpty();

    const size = accessor.getRepeatedUint64Size(1);

    expect(size).toEqual(0);
  });

  it('return unpacked values from the input', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const list = accessor.getRepeatedUint64Iterable(1);

    expectEqualToArray(list, [value1, value2]);
  });

  it('ensure not the same instance returned for unpacked values', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const list1 = accessor.getRepeatedUint64Iterable(1);
    const list2 = accessor.getRepeatedUint64Iterable(1);

    expect(list1).not.toBe(list2);
  });

  it('add single unpacked value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedUint64Element(1, value1);
    const list1 = accessor.getRepeatedUint64Iterable(1);
    accessor.addUnpackedUint64Element(1, value2);
    const list2 = accessor.getRepeatedUint64Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('add unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedUint64Iterable(1, [value1]);
    const list1 = accessor.getRepeatedUint64Iterable(1);
    accessor.addUnpackedUint64Iterable(1, [value2]);
    const list2 = accessor.getRepeatedUint64Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('set a single unpacked value', () => {
    const accessor = Kernel.fromArrayBuffer(packedValue1Value2);

    accessor.setUnpackedUint64Element(1, 1, value1);
    const list = accessor.getRepeatedUint64Iterable(1);

    expectEqualToArray(list, [value1, value1]);
  });

  it('set unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setUnpackedUint64Iterable(1, [value1]);
    const list = accessor.getRepeatedUint64Iterable(1);

    expectEqualToArray(list, [value1]);
  });

  it('encode for adding single unpacked value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedUint64Element(1, value1);
    accessor.addUnpackedUint64Element(1, value2);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue1Value2);
  });

  it('encode for adding unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addUnpackedUint64Iterable(1, [value1]);
    accessor.addUnpackedUint64Iterable(1, [value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue1Value2);
  });

  it('encode for setting single unpacked value', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    accessor.setUnpackedUint64Element(1, 0, value2);
    accessor.setUnpackedUint64Element(1, 1, value1);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue2Value1);
  });

  it('encode for setting unpacked values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setUnpackedUint64Iterable(1, [value1, value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(unpackedValue1Value2);
  });

  it('return packed values from the input', () => {
    const accessor = Kernel.fromArrayBuffer(packedValue1Value2);

    const list = accessor.getRepeatedUint64Iterable(1);

    expectEqualToArray(list, [value1, value2]);
  });

  it('ensure not the same instance returned for packed values', () => {
    const accessor = Kernel.fromArrayBuffer(packedValue1Value2);

    const list1 = accessor.getRepeatedUint64Iterable(1);
    const list2 = accessor.getRepeatedUint64Iterable(1);

    expect(list1).not.toBe(list2);
  });

  it('add single packed value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedUint64Element(1, value1);
    const list1 = accessor.getRepeatedUint64Iterable(1);
    accessor.addPackedUint64Element(1, value2);
    const list2 = accessor.getRepeatedUint64Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('add packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedUint64Iterable(1, [value1]);
    const list1 = accessor.getRepeatedUint64Iterable(1);
    accessor.addPackedUint64Iterable(1, [value2]);
    const list2 = accessor.getRepeatedUint64Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('set a single packed value', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    accessor.setPackedUint64Element(1, 1, value1);
    const list = accessor.getRepeatedUint64Iterable(1);

    expectEqualToArray(list, [value1, value1]);
  });

  it('set packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setPackedUint64Iterable(1, [value1]);
    const list1 = accessor.getRepeatedUint64Iterable(1);
    accessor.setPackedUint64Iterable(1, [value2]);
    const list2 = accessor.getRepeatedUint64Iterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value2]);
  });

  it('encode for adding single packed value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedUint64Element(1, value1);
    accessor.addPackedUint64Element(1, value2);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue1Value2);
  });

  it('encode for adding packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addPackedUint64Iterable(1, [value1, value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue1Value2);
  });

  it('encode for setting single packed value', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    accessor.setPackedUint64Element(1, 0, value2);
    accessor.setPackedUint64Element(1, 1, value1);

    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue2Value1);
  });

  it('encode for setting packed values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setPackedUint64Iterable(1, [value1, value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(packedValue1Value2);
  });

  it('return combined values from the input', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(
        0x08,
        0x01,  // unpacked value1
        0x0A,
        0x02,
        0x01,
        0x00,  // packed value1 and value2
        0x08,
        0x00,  // unpacked value2
        ));

    const list = accessor.getRepeatedUint64Iterable(1);

    expectEqualToArray(list, [value1, value1, value2, value2]);
  });

  it('return the repeated field element from the input', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const result1 = accessor.getRepeatedUint64Element(
        /* fieldNumber= */ 1, /* index= */ 0);
    const result2 = accessor.getRepeatedUint64Element(
        /* fieldNumber= */ 1, /* index= */ 1);

    expect(result1).toEqual(value1);
    expect(result2).toEqual(value2);
  });

  it('return the size from the input', () => {
    const accessor = Kernel.fromArrayBuffer(unpackedValue1Value2);

    const size = accessor.getRepeatedUint64Size(1);

    expect(size).toEqual(2);
  });

  it('fail when getting unpacked uint64 value with other wire types', () => {
    const accessor =
        Kernel.fromArrayBuffer(createArrayBuffer(0x0D, 0x80, 0x80, 0x80, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => {
        accessor.getRepeatedUint64Iterable(1);
      }).toThrowError('Expected wire type: 0 but found: 5');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      expectQualifiedIterable(
          accessor.getRepeatedUint64Iterable(1),
          (value) => value instanceof Int64);
    }
  });

  it('fail when adding unpacked uint64 values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeUint64 = /** @type {!Int64} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addUnpackedUint64Iterable(1, [fakeUint64]))
          .toThrowError('Must be Int64 instance, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addUnpackedUint64Iterable(1, [fakeUint64]);
      expectQualifiedIterable(accessor.getRepeatedUint64Iterable(1));
    }
  });

  it('fail when adding single unpacked uint64 value with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeUint64 = /** @type {!Int64} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addUnpackedUint64Element(1, fakeUint64))
          .toThrowError('Must be Int64 instance, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addUnpackedUint64Element(1, fakeUint64);
      expectQualifiedIterable(accessor.getRepeatedUint64Iterable(1));
    }
  });

  it('fail when setting unpacked uint64 values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeUint64 = /** @type {!Int64} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setUnpackedUint64Iterable(1, [fakeUint64]))
          .toThrowError('Must be Int64 instance, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setUnpackedUint64Iterable(1, [fakeUint64]);
      expectQualifiedIterable(accessor.getRepeatedUint64Iterable(1));
    }
  });

  it('fail when setting single unpacked uint64 value with null value', () => {
    const accessor =
        Kernel.fromArrayBuffer(createArrayBuffer(0x0D, 0x80, 0x80, 0x80, 0x00));
    const fakeUint64 = /** @type {!Int64} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setUnpackedUint64Element(1, 0, fakeUint64))
          .toThrowError('Must be Int64 instance, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setUnpackedUint64Element(1, 0, fakeUint64);
      expectQualifiedIterable(accessor.getRepeatedUint64Iterable(1));
    }
  });

  it('fail when adding packed uint64 values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeUint64 = /** @type {!Int64} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addPackedUint64Iterable(1, [fakeUint64]))
          .toThrowError('Must be Int64 instance, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addPackedUint64Iterable(1, [fakeUint64]);
      expectQualifiedIterable(accessor.getRepeatedUint64Iterable(1));
    }
  });

  it('fail when adding single packed uint64 value with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeUint64 = /** @type {!Int64} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addPackedUint64Element(1, fakeUint64))
          .toThrowError('Must be Int64 instance, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addPackedUint64Element(1, fakeUint64);
      expectQualifiedIterable(accessor.getRepeatedUint64Iterable(1));
    }
  });

  it('fail when setting packed uint64 values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeUint64 = /** @type {!Int64} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setPackedUint64Iterable(1, [fakeUint64]))
          .toThrowError('Must be Int64 instance, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setPackedUint64Iterable(1, [fakeUint64]);
      expectQualifiedIterable(accessor.getRepeatedUint64Iterable(1));
    }
  });

  it('fail when setting single packed uint64 value with null value', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(0x08, 0x00));
    const fakeUint64 = /** @type {!Int64} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setPackedUint64Element(1, 0, fakeUint64))
          .toThrowError('Must be Int64 instance, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setPackedUint64Element(1, 0, fakeUint64);
      expectQualifiedIterable(accessor.getRepeatedUint64Iterable(1));
    }
  });

  it('fail when setting single unpacked with out-of-bound index', () => {
    const accessor =
        Kernel.fromArrayBuffer(createArrayBuffer(0x0A, 0x01, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setUnpackedUint64Element(1, 1, Int64.fromInt(1)))
          .toThrowError('Index out of bounds: index: 1 size: 1');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setUnpackedUint64Element(1, 1, Int64.fromInt(1));
      expectQualifiedIterable(
          accessor.getRepeatedUint64Iterable(1),
          (value) => value instanceof Int64);
    }
  });

  it('fail when setting single packed with out-of-bound index', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(0x08, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setPackedUint64Element(1, 1, Int64.fromInt(1)))
          .toThrowError('Index out of bounds: index: 1 size: 1');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setPackedUint64Element(1, 1, Int64.fromInt(1));
      expectQualifiedIterable(
          accessor.getRepeatedUint64Iterable(1),
          (value) => value instanceof Int64);
    }
  });

  it('fail when getting element with out-of-range index', () => {
    const accessor = Kernel.createEmpty();
    if (CHECK_CRITICAL_STATE) {
      expect(() => {
        accessor.getRepeatedUint64Element(
            /* fieldNumber= */ 1, /* index= */ 0);
      }).toThrowError('Index out of bounds: index: 0 size: 0');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      expect(accessor.getRepeatedUint64Element(
                 /* fieldNumber= */ 1, /* index= */ 0))
          .toBe(undefined);
    }
  });
});

describe('Kernel for repeated bytes does', () => {
  const value1 = ByteString.fromArrayBuffer((createArrayBuffer(0x61)));
  const value2 = ByteString.fromArrayBuffer((createArrayBuffer(0x62)));

  const repeatedValue1Value2 = createArrayBuffer(
      0x0A,
      0x01,
      0x61,  // value1
      0x0A,
      0x01,
      0x62,  // value2
  );
  const repeatedValue2Value1 = createArrayBuffer(
      0x0A,
      0x01,
      0x62,  // value2
      0x0A,
      0x01,
      0x61,  // value1
  );

  it('return empty array for the empty input', () => {
    const accessor = Kernel.createEmpty();

    const list = accessor.getRepeatedBytesIterable(1);

    expectEqualToArray(list, []);
  });

  it('ensure not the same instance returned for the empty input', () => {
    const accessor = Kernel.createEmpty();

    const list1 = accessor.getRepeatedBytesIterable(1);
    const list2 = accessor.getRepeatedBytesIterable(1);

    expect(list1).not.toBe(list2);
  });

  it('return size for the empty input', () => {
    const accessor = Kernel.createEmpty();

    const size = accessor.getRepeatedBytesSize(1);

    expect(size).toEqual(0);
  });

  it('return values from the input', () => {
    const accessor = Kernel.fromArrayBuffer(repeatedValue1Value2);

    const list = accessor.getRepeatedBytesIterable(1);

    expectEqualToArray(list, [value1, value2]);
  });

  it('ensure not the same instance returned for values', () => {
    const accessor = Kernel.fromArrayBuffer(repeatedValue1Value2);

    const list1 = accessor.getRepeatedBytesIterable(1);
    const list2 = accessor.getRepeatedBytesIterable(1);

    expect(list1).not.toBe(list2);
  });

  it('add single value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addRepeatedBytesElement(1, value1);
    const list1 = accessor.getRepeatedBytesIterable(1);
    accessor.addRepeatedBytesElement(1, value2);
    const list2 = accessor.getRepeatedBytesIterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('add values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addRepeatedBytesIterable(1, [value1]);
    const list1 = accessor.getRepeatedBytesIterable(1);
    accessor.addRepeatedBytesIterable(1, [value2]);
    const list2 = accessor.getRepeatedBytesIterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('set a single value', () => {
    const accessor = Kernel.fromArrayBuffer(repeatedValue1Value2);

    accessor.setRepeatedBytesElement(1, 1, value1);
    const list = accessor.getRepeatedBytesIterable(1);

    expectEqualToArray(list, [value1, value1]);
  });

  it('set values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setRepeatedBytesIterable(1, [value1]);
    const list = accessor.getRepeatedBytesIterable(1);

    expectEqualToArray(list, [value1]);
  });

  it('encode for adding single value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addRepeatedBytesElement(1, value1);
    accessor.addRepeatedBytesElement(1, value2);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(repeatedValue1Value2);
  });

  it('encode for adding values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addRepeatedBytesIterable(1, [value1]);
    accessor.addRepeatedBytesIterable(1, [value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(repeatedValue1Value2);
  });

  it('encode for setting single value', () => {
    const accessor = Kernel.fromArrayBuffer(repeatedValue1Value2);

    accessor.setRepeatedBytesElement(1, 0, value2);
    accessor.setRepeatedBytesElement(1, 1, value1);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(repeatedValue2Value1);
  });

  it('encode for setting values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setRepeatedBytesIterable(1, [value1, value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(repeatedValue1Value2);
  });

  it('return the repeated field element from the input', () => {
    const accessor = Kernel.fromArrayBuffer(repeatedValue1Value2);

    const result1 = accessor.getRepeatedBytesElement(
        /* fieldNumber= */ 1, /* index= */ 0);
    const result2 = accessor.getRepeatedBytesElement(
        /* fieldNumber= */ 1, /* index= */ 1);

    expect(result1).toEqual(value1);
    expect(result2).toEqual(value2);
  });

  it('return the size from the input', () => {
    const accessor = Kernel.fromArrayBuffer(repeatedValue1Value2);

    const size = accessor.getRepeatedBytesSize(1);

    expect(size).toEqual(2);
  });

  it('fail when getting bytes value with other wire types', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(
        0x08, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => {
        accessor.getRepeatedBytesIterable(1);
      }).toThrowError('Expected wire type: 2 but found: 0');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      expectQualifiedIterable(
          accessor.getRepeatedBytesIterable(1),
          (value) => value instanceof ByteString);
    }
  });

  it('fail when adding bytes values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeBytes = /** @type {!ByteString} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addRepeatedBytesIterable(1, [fakeBytes]))
          .toThrowError('Must be a ByteString, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addRepeatedBytesIterable(1, [fakeBytes]);
      expectQualifiedIterable(accessor.getRepeatedBytesIterable(1));
    }
  });

  it('fail when adding single bytes value with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeBytes = /** @type {!ByteString} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addRepeatedBytesElement(1, fakeBytes))
          .toThrowError('Must be a ByteString, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addRepeatedBytesElement(1, fakeBytes);
      expectQualifiedIterable(accessor.getRepeatedBytesIterable(1));
    }
  });

  it('fail when setting bytes values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeBytes = /** @type {!ByteString} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setRepeatedBytesIterable(1, [fakeBytes]))
          .toThrowError('Must be a ByteString, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setRepeatedBytesIterable(1, [fakeBytes]);
      expectQualifiedIterable(accessor.getRepeatedBytesIterable(1));
    }
  });

  it('fail when setting single bytes value with null value', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(
        0x08, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00));
    const fakeBytes = /** @type {!ByteString} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setRepeatedBytesElement(1, 0, fakeBytes))
          .toThrowError('Must be a ByteString, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setRepeatedBytesElement(1, 0, fakeBytes);
      expectQualifiedIterable(accessor.getRepeatedBytesIterable(1));
    }
  });

  it('fail when setting single with out-of-bound index', () => {
    const accessor =
        Kernel.fromArrayBuffer(createArrayBuffer(0x0A, 0x01, 0x61));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setRepeatedBytesElement(1, 1, value1))
          .toThrowError('Index out of bounds: index: 1 size: 1');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setRepeatedBytesElement(1, 1, value1);
      expectQualifiedIterable(
          accessor.getRepeatedBytesIterable(1),
          (value) => value instanceof ByteString);
    }
  });

  it('fail when getting element with out-of-range index', () => {
    const accessor = Kernel.createEmpty();
    if (CHECK_CRITICAL_STATE) {
      expect(() => {
        accessor.getRepeatedBytesElement(
            /* fieldNumber= */ 1, /* index= */ 0);
      }).toThrowError('Index out of bounds: index: 0 size: 0');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      expect(accessor.getRepeatedBytesElement(
                 /* fieldNumber= */ 1, /* index= */ 0))
          .toBe(undefined);
    }
  });
});

describe('Kernel for repeated string does', () => {
  const value1 = 'a';
  const value2 = 'b';

  const repeatedValue1Value2 = createArrayBuffer(
      0x0A,
      0x01,
      0x61,  // value1
      0x0A,
      0x01,
      0x62,  // value2
  );
  const repeatedValue2Value1 = createArrayBuffer(
      0x0A,
      0x01,
      0x62,  // value2
      0x0A,
      0x01,
      0x61,  // value1
  );

  it('return empty array for the empty input', () => {
    const accessor = Kernel.createEmpty();

    const list = accessor.getRepeatedStringIterable(1);

    expectEqualToArray(list, []);
  });

  it('ensure not the same instance returned for the empty input', () => {
    const accessor = Kernel.createEmpty();

    const list1 = accessor.getRepeatedStringIterable(1);
    const list2 = accessor.getRepeatedStringIterable(1);

    expect(list1).not.toBe(list2);
  });

  it('return size for the empty input', () => {
    const accessor = Kernel.createEmpty();

    const size = accessor.getRepeatedStringSize(1);

    expect(size).toEqual(0);
  });

  it('return values from the input', () => {
    const accessor = Kernel.fromArrayBuffer(repeatedValue1Value2);

    const list = accessor.getRepeatedStringIterable(1);

    expectEqualToArray(list, [value1, value2]);
  });

  it('ensure not the same instance returned for values', () => {
    const accessor = Kernel.fromArrayBuffer(repeatedValue1Value2);

    const list1 = accessor.getRepeatedStringIterable(1);
    const list2 = accessor.getRepeatedStringIterable(1);

    expect(list1).not.toBe(list2);
  });

  it('add single value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addRepeatedStringElement(1, value1);
    const list1 = accessor.getRepeatedStringIterable(1);
    accessor.addRepeatedStringElement(1, value2);
    const list2 = accessor.getRepeatedStringIterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('add values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addRepeatedStringIterable(1, [value1]);
    const list1 = accessor.getRepeatedStringIterable(1);
    accessor.addRepeatedStringIterable(1, [value2]);
    const list2 = accessor.getRepeatedStringIterable(1);

    expectEqualToArray(list1, [value1]);
    expectEqualToArray(list2, [value1, value2]);
  });

  it('set a single value', () => {
    const accessor = Kernel.fromArrayBuffer(repeatedValue1Value2);

    accessor.setRepeatedStringElement(1, 1, value1);
    const list = accessor.getRepeatedStringIterable(1);

    expectEqualToArray(list, [value1, value1]);
  });

  it('set values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setRepeatedStringIterable(1, [value1]);
    const list = accessor.getRepeatedStringIterable(1);

    expectEqualToArray(list, [value1]);
  });

  it('encode for adding single value', () => {
    const accessor = Kernel.createEmpty();

    accessor.addRepeatedStringElement(1, value1);
    accessor.addRepeatedStringElement(1, value2);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(repeatedValue1Value2);
  });

  it('encode for adding values', () => {
    const accessor = Kernel.createEmpty();

    accessor.addRepeatedStringIterable(1, [value1]);
    accessor.addRepeatedStringIterable(1, [value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(repeatedValue1Value2);
  });

  it('encode for setting single value', () => {
    const accessor = Kernel.fromArrayBuffer(repeatedValue1Value2);

    accessor.setRepeatedStringElement(1, 0, value2);
    accessor.setRepeatedStringElement(1, 1, value1);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(repeatedValue2Value1);
  });

  it('encode for setting values', () => {
    const accessor = Kernel.createEmpty();

    accessor.setRepeatedStringIterable(1, [value1, value2]);
    const serialized = accessor.serialize();

    expect(serialized).toEqual(repeatedValue1Value2);
  });

  it('return the repeated field element from the input', () => {
    const accessor = Kernel.fromArrayBuffer(repeatedValue1Value2);

    const result1 = accessor.getRepeatedStringElement(
        /* fieldNumber= */ 1, /* index= */ 0);
    const result2 = accessor.getRepeatedStringElement(
        /* fieldNumber= */ 1, /* index= */ 1);

    expect(result1).toEqual(value1);
    expect(result2).toEqual(value2);
  });

  it('return the size from the input', () => {
    const accessor = Kernel.fromArrayBuffer(repeatedValue1Value2);

    const size = accessor.getRepeatedStringSize(1);

    expect(size).toEqual(2);
  });

  it('fail when getting string value with other wire types', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(
        0x08, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => {
        accessor.getRepeatedStringIterable(1);
      }).toThrowError('Expected wire type: 2 but found: 0');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      expectQualifiedIterable(
          accessor.getRepeatedStringIterable(1),
          (value) => typeof value === 'string');
    }
  });

  it('fail when adding string values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeString = /** @type {string} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addRepeatedStringIterable(1, [fakeString]))
          .toThrowError('Must be string, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addRepeatedStringIterable(1, [fakeString]);
      expectQualifiedIterable(accessor.getRepeatedStringIterable(1));
    }
  });

  it('fail when adding single string value with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeString = /** @type {string} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.addRepeatedStringElement(1, fakeString))
          .toThrowError('Must be string, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addRepeatedStringElement(1, fakeString);
      expectQualifiedIterable(accessor.getRepeatedStringIterable(1));
    }
  });

  it('fail when setting string values with null value', () => {
    const accessor = Kernel.createEmpty();
    const fakeString = /** @type {string} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setRepeatedStringIterable(1, [fakeString]))
          .toThrowError('Must be string, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setRepeatedStringIterable(1, [fakeString]);
      expectQualifiedIterable(accessor.getRepeatedStringIterable(1));
    }
  });

  it('fail when setting single string value with null value', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(
        0x08, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00));
    const fakeString = /** @type {string} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setRepeatedStringElement(1, 0, fakeString))
          .toThrowError('Must be string, but got: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setRepeatedStringElement(1, 0, fakeString);
      expectQualifiedIterable(accessor.getRepeatedStringIterable(1));
    }
  });

  it('fail when setting single with out-of-bound index', () => {
    const accessor =
        Kernel.fromArrayBuffer(createArrayBuffer(0x0A, 0x01, 0x61));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setRepeatedStringElement(1, 1, value1))
          .toThrowError('Index out of bounds: index: 1 size: 1');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setRepeatedStringElement(1, 1, value1);
      expectQualifiedIterable(
          accessor.getRepeatedStringIterable(1),
          (value) => typeof value === 'string');
    }
  });

  it('fail when getting element with out-of-range index', () => {
    const accessor = Kernel.createEmpty();
    if (CHECK_CRITICAL_STATE) {
      expect(() => {
        accessor.getRepeatedStringElement(
            /* fieldNumber= */ 1, /* index= */ 0);
      }).toThrowError('Index out of bounds: index: 0 size: 0');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      expect(accessor.getRepeatedStringElement(
                 /* fieldNumber= */ 1, /* index= */ 0))
          .toBe(undefined);
    }
  });
});

describe('Kernel for repeated message does', () => {
  it('return empty array for the empty input', () => {
    const accessor = Kernel.createEmpty();
    expectEqualToArray(
        accessor.getRepeatedMessageIterable(1, TestMessage.instanceCreator),
        []);
  });

  it('return empty accessor array for the empty input', () => {
    const accessor = Kernel.createEmpty();
    expectEqualToArray(accessor.getRepeatedMessageAccessorIterable(1), []);
  });

  it('ensure not the same instance returned for the empty input', () => {
    const accessor = Kernel.createEmpty();
    const list1 =
        accessor.getRepeatedMessageIterable(1, TestMessage.instanceCreator);
    const list2 =
        accessor.getRepeatedMessageIterable(1, TestMessage.instanceCreator);
    expect(list1).not.toBe(list2);
  });

  it('return size for the empty input', () => {
    const accessor = Kernel.createEmpty();
    expect(accessor.getRepeatedMessageSize(1, TestMessage.instanceCreator))
        .toEqual(0);
  });

  it('return values from the input', () => {
    const bytes1 = createArrayBuffer(0x08, 0x01);
    const bytes2 = createArrayBuffer(0x08, 0x00);
    const msg1 = new TestMessage(Kernel.fromArrayBuffer(bytes1));
    const msg2 = new TestMessage(Kernel.fromArrayBuffer(bytes2));

    const bytes =
        createArrayBuffer(0x0A, 0x02, 0x08, 0x01, 0x0A, 0x02, 0x08, 0x00);
    const accessor = Kernel.fromArrayBuffer(bytes);
    expectEqualToMessageArray(
        accessor.getRepeatedMessageIterable(1, TestMessage.instanceCreator),
        [msg1, msg2]);
  });

  it('ensure not the same array instance returned', () => {
    const bytes =
        createArrayBuffer(0x0A, 0x02, 0x08, 0x01, 0x0A, 0x02, 0x08, 0x00);
    const accessor = Kernel.fromArrayBuffer(bytes);
    const list1 =
        accessor.getRepeatedMessageIterable(1, TestMessage.instanceCreator);
    const list2 =
        accessor.getRepeatedMessageIterable(1, TestMessage.instanceCreator);
    expect(list1).not.toBe(list2);
  });

  it('ensure the same array element returned for get iterable', () => {
    const bytes =
        createArrayBuffer(0x0A, 0x02, 0x08, 0x01, 0x0A, 0x02, 0x08, 0x00);
    const accessor = Kernel.fromArrayBuffer(bytes);
    const list1 =
        accessor.getRepeatedMessageIterable(1, TestMessage.instanceCreator);
    const list2 = accessor.getRepeatedMessageIterable(
        1, TestMessage.instanceCreator, /* pivot= */ 0);
    const array1 = Array.from(list1);
    const array2 = Array.from(list2);
    for (let i = 0; i < array1.length; i++) {
      expect(array1[i]).toBe(array2[i]);
    }
  });

  it('return accessors from the input', () => {
    const bytes =
        createArrayBuffer(0x0A, 0x02, 0x08, 0x01, 0x0A, 0x02, 0x08, 0x00);
    const accessor = Kernel.fromArrayBuffer(bytes);
    const [accessor1, accessor2] =
        [...accessor.getRepeatedMessageAccessorIterable(1)];
    expect(accessor1.getInt32WithDefault(1)).toEqual(1);
    expect(accessor2.getInt32WithDefault(1)).toEqual(0);
  });

  it('return accessors from the input when pivot is set', () => {
    const bytes =
        createArrayBuffer(0x0A, 0x02, 0x08, 0x01, 0x0A, 0x02, 0x08, 0x00);
    const accessor = Kernel.fromArrayBuffer(bytes);
    const [accessor1, accessor2] =
        [...accessor.getRepeatedMessageAccessorIterable(1, /* pivot= */ 0)];
    expect(accessor1.getInt32WithDefault(1)).toEqual(1);
    expect(accessor2.getInt32WithDefault(1)).toEqual(0);
  });

  it('return the repeated field element from the input', () => {
    const bytes =
        createArrayBuffer(0x0A, 0x02, 0x08, 0x01, 0x0A, 0x02, 0x08, 0x00);
    const accessor = Kernel.fromArrayBuffer(bytes);
    const msg1 = accessor.getRepeatedMessageElement(
        /* fieldNumber= */ 1, TestMessage.instanceCreator,
        /* index= */ 0);
    const msg2 = accessor.getRepeatedMessageElement(
        /* fieldNumber= */ 1, TestMessage.instanceCreator,
        /* index= */ 1, /* pivot= */ 0);
    expect(msg1.getBoolWithDefault(
               /* fieldNumber= */ 1, /* default= */ false))
        .toEqual(true);
    expect(msg2.getBoolWithDefault(
               /* fieldNumber= */ 1, /* default= */ false))
        .toEqual(false);
  });

  it('ensure the same array element returned', () => {
    const bytes = createArrayBuffer(0x0A, 0x02, 0x08, 0x01);
    const accessor = Kernel.fromArrayBuffer(bytes);
    const msg1 = accessor.getRepeatedMessageElement(
        /* fieldNumber= */ 1, TestMessage.instanceCreator,
        /* index= */ 0);
    const msg2 = accessor.getRepeatedMessageElement(
        /* fieldNumber= */ 1, TestMessage.instanceCreator,
        /* index= */ 0);
    expect(msg1).toBe(msg2);
  });

  it('return the size from the input', () => {
    const bytes =
        createArrayBuffer(0x0A, 0x02, 0x08, 0x01, 0x0A, 0x02, 0x08, 0x00);
    const accessor = Kernel.fromArrayBuffer(bytes);
    expect(accessor.getRepeatedMessageSize(1, TestMessage.instanceCreator))
        .toEqual(2);
  });

  it('encode repeated message from the input', () => {
    const bytes =
        createArrayBuffer(0x0A, 0x02, 0x08, 0x01, 0x0A, 0x02, 0x08, 0x00);
    const accessor = Kernel.fromArrayBuffer(bytes);
    expect(accessor.serialize()).toEqual(bytes);
  });

  it('add a single value', () => {
    const accessor = Kernel.createEmpty();
    const bytes1 = createArrayBuffer(0x08, 0x01);
    const msg1 = new TestMessage(Kernel.fromArrayBuffer(bytes1));
    const bytes2 = createArrayBuffer(0x08, 0x00);
    const msg2 = new TestMessage(Kernel.fromArrayBuffer(bytes2));

    accessor.addRepeatedMessageElement(1, msg1, TestMessage.instanceCreator);
    accessor.addRepeatedMessageElement(1, msg2, TestMessage.instanceCreator);
    const result =
        accessor.getRepeatedMessageIterable(1, TestMessage.instanceCreator);

    expect(Array.from(result)).toEqual([msg1, msg2]);
  });

  it('add values', () => {
    const accessor = Kernel.createEmpty();
    const bytes1 = createArrayBuffer(0x08, 0x01);
    const msg1 = new TestMessage(Kernel.fromArrayBuffer(bytes1));
    const bytes2 = createArrayBuffer(0x08, 0x00);
    const msg2 = new TestMessage(Kernel.fromArrayBuffer(bytes2));

    accessor.addRepeatedMessageIterable(1, [msg1], TestMessage.instanceCreator);
    accessor.addRepeatedMessageIterable(1, [msg2], TestMessage.instanceCreator);
    const result =
        accessor.getRepeatedMessageIterable(1, TestMessage.instanceCreator);

    expect(Array.from(result)).toEqual([msg1, msg2]);
  });

  it('set a single value', () => {
    const bytes = createArrayBuffer(0x0A, 0x02, 0x08, 0x00);
    const accessor = Kernel.fromArrayBuffer(bytes);
    const subbytes = createArrayBuffer(0x08, 0x01);
    const submsg = new TestMessage(Kernel.fromArrayBuffer(subbytes));

    accessor.setRepeatedMessageElement(
        /* fieldNumber= */ 1, submsg, TestMessage.instanceCreator,
        /* index= */ 0);
    const result =
        accessor.getRepeatedMessageIterable(1, TestMessage.instanceCreator);

    expect(Array.from(result)).toEqual([submsg]);
  });

  it('write submessage changes made via getRepeatedMessagElement', () => {
    const bytes = createArrayBuffer(0x0A, 0x02, 0x08, 0x05);
    const expected = createArrayBuffer(0x0A, 0x02, 0x08, 0x00);
    const accessor = Kernel.fromArrayBuffer(bytes);
    const submsg = accessor.getRepeatedMessageElement(
        /* fieldNumber= */ 1, TestMessage.instanceCreator,
        /* index= */ 0);
    expect(submsg.getInt32WithDefault(1, 0)).toEqual(5);
    submsg.setInt32(1, 0);

    expect(accessor.serialize()).toEqual(expected);
  });

  it('set values', () => {
    const accessor = Kernel.createEmpty();
    const subbytes = createArrayBuffer(0x08, 0x01);
    const submsg = new TestMessage(Kernel.fromArrayBuffer(subbytes));

    accessor.setRepeatedMessageIterable(1, [submsg]);
    const result =
        accessor.getRepeatedMessageIterable(1, TestMessage.instanceCreator);

    expect(Array.from(result)).toEqual([submsg]);
  });

  it('encode for adding single value', () => {
    const accessor = Kernel.createEmpty();
    const bytes1 = createArrayBuffer(0x08, 0x01);
    const msg1 = new TestMessage(Kernel.fromArrayBuffer(bytes1));
    const bytes2 = createArrayBuffer(0x08, 0x00);
    const msg2 = new TestMessage(Kernel.fromArrayBuffer(bytes2));
    const expected =
        createArrayBuffer(0x0A, 0x02, 0x08, 0x01, 0x0A, 0x02, 0x08, 0x00);

    accessor.addRepeatedMessageElement(1, msg1, TestMessage.instanceCreator);
    accessor.addRepeatedMessageElement(1, msg2, TestMessage.instanceCreator);
    const result = accessor.serialize();

    expect(result).toEqual(expected);
  });

  it('encode for adding values', () => {
    const accessor = Kernel.createEmpty();
    const bytes1 = createArrayBuffer(0x08, 0x01);
    const msg1 = new TestMessage(Kernel.fromArrayBuffer(bytes1));
    const bytes2 = createArrayBuffer(0x08, 0x00);
    const msg2 = new TestMessage(Kernel.fromArrayBuffer(bytes2));
    const expected =
        createArrayBuffer(0x0A, 0x02, 0x08, 0x01, 0x0A, 0x02, 0x08, 0x00);

    accessor.addRepeatedMessageIterable(
        1, [msg1, msg2], TestMessage.instanceCreator);
    const result = accessor.serialize();

    expect(result).toEqual(expected);
  });

  it('encode for setting single value', () => {
    const bytes = createArrayBuffer(0x0A, 0x02, 0x08, 0x00);
    const accessor = Kernel.fromArrayBuffer(bytes);
    const subbytes = createArrayBuffer(0x08, 0x01);
    const submsg = new TestMessage(Kernel.fromArrayBuffer(subbytes));
    const expected = createArrayBuffer(0x0A, 0x02, 0x08, 0x01);

    accessor.setRepeatedMessageElement(
        /* fieldNumber= */ 1, submsg, TestMessage.instanceCreator,
        /* index= */ 0);
    const result = accessor.serialize();

    expect(result).toEqual(expected);
  });

  it('encode for setting values', () => {
    const accessor = Kernel.createEmpty();
    const subbytes = createArrayBuffer(0x08, 0x01);
    const submsg = new TestMessage(Kernel.fromArrayBuffer(subbytes));
    const expected = createArrayBuffer(0x0A, 0x02, 0x08, 0x01);

    accessor.setRepeatedMessageIterable(1, [submsg]);
    const result = accessor.serialize();

    expect(result).toEqual(expected);
  });

  it('get accessors from set values.', () => {
    const accessor = Kernel.createEmpty();
    const bytes1 = createArrayBuffer(0x08, 0x01);
    const msg1 = new TestMessage(Kernel.fromArrayBuffer(bytes1));
    const bytes2 = createArrayBuffer(0x08, 0x00);
    const msg2 = new TestMessage(Kernel.fromArrayBuffer(bytes2));

    accessor.addRepeatedMessageIterable(
        1, [msg1, msg2], TestMessage.instanceCreator);

    const [accessor1, accessor2] =
        [...accessor.getRepeatedMessageAccessorIterable(1)];
    expect(accessor1.getInt32WithDefault(1)).toEqual(1);
    expect(accessor2.getInt32WithDefault(1)).toEqual(0);

    // Retrieved accessors are the exact same accessors as the added messages.
    expect(accessor1).toBe(
        (/** @type {!InternalMessage} */ (msg1)).internalGetKernel());
    expect(accessor2).toBe(
        (/** @type {!InternalMessage} */ (msg2)).internalGetKernel());
  });

  it('fail when getting message value with other wire types', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(
        0x09, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00));
    if (CHECK_CRITICAL_STATE) {
      expect(() => {
        accessor.getRepeatedMessageIterable(1, TestMessage.instanceCreator);
      }).toThrow();
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      const [msg1] =
          accessor.getRepeatedMessageIterable(1, TestMessage.instanceCreator);
      expect(msg1.serialize()).toEqual(createArrayBuffer());
    }
  });

  it('fail when adding message values with wrong type value', () => {
    const accessor = Kernel.createEmpty();
    const fakeValue = /** @type {!TestMessage} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(
          () => accessor.addRepeatedMessageIterable(
              1, [fakeValue], TestMessage.instanceCreator))
          .toThrowError('Given value is not a message instance: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addRepeatedMessageIterable(
          1, [fakeValue], TestMessage.instanceCreator);
      const list =
          accessor.getRepeatedMessageIterable(1, TestMessage.instanceCreator);
      expect(Array.from(list)).toEqual([null]);
    }
  });

  it('fail when adding single message value with wrong type value', () => {
    const accessor = Kernel.createEmpty();
    const fakeValue = /** @type {!TestMessage} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(
          () => accessor.addRepeatedMessageElement(
              1, fakeValue, TestMessage.instanceCreator))
          .toThrowError('Given value is not a message instance: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addRepeatedMessageElement(
          1, fakeValue, TestMessage.instanceCreator);
      const list =
          accessor.getRepeatedMessageIterable(1, TestMessage.instanceCreator);
      expect(Array.from(list)).toEqual([null]);
    }
  });

  it('fail when setting message values with wrong type value', () => {
    const accessor = Kernel.createEmpty();
    const fakeValue = /** @type {!TestMessage} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setRepeatedMessageIterable(1, [fakeValue]))
          .toThrowError('Given value is not a message instance: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setRepeatedMessageIterable(1, [fakeValue]);
      const list =
          accessor.getRepeatedMessageIterable(1, TestMessage.instanceCreator);
      expect(Array.from(list)).toEqual([null]);
    }
  });

  it('fail when setting single value with wrong type value', () => {
    const accessor =
        Kernel.fromArrayBuffer(createArrayBuffer(0x0A, 0x02, 0x08, 0x00));
    const fakeValue = /** @type {!TestMessage} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(
          () => accessor.setRepeatedMessageElement(
              /* fieldNumber= */ 1, fakeValue, TestMessage.instanceCreator,
              /* index= */ 0))
          .toThrowError('Given value is not a message instance: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setRepeatedMessageElement(
          /* fieldNumber= */ 1, fakeValue, TestMessage.instanceCreator,
          /* index= */ 0);
      const list =
          accessor.getRepeatedMessageIterable(1, TestMessage.instanceCreator);
      expect(Array.from(list).length).toEqual(1);
    }
  });

  it('fail when setting single value with out-of-bound index', () => {
    const accessor =
        Kernel.fromArrayBuffer(createArrayBuffer(0x0A, 0x02, 0x08, 0x00));
    const msg1 =
        accessor.getRepeatedMessageElement(1, TestMessage.instanceCreator, 0);
    const bytes2 = createArrayBuffer(0x08, 0x01);
    const msg2 = new TestMessage(Kernel.fromArrayBuffer(bytes2));
    if (CHECK_CRITICAL_STATE) {
      expect(
          () => accessor.setRepeatedMessageElement(
              /* fieldNumber= */ 1, msg2, TestMessage.instanceCreator,
              /* index= */ 1))
          .toThrowError('Index out of bounds: index: 1 size: 1');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setRepeatedMessageElement(
          /* fieldNumber= */ 1, msg2, TestMessage.instanceCreator,
          /* index= */ 1);
      expectEqualToArray(
          accessor.getRepeatedMessageIterable(1, TestMessage.instanceCreator),
          [msg1, msg2]);
    }
  });
});

describe('Kernel for repeated groups does', () => {
  it('return empty array for the empty input', () => {
    const accessor = Kernel.createEmpty();
    expectEqualToArray(
        accessor.getRepeatedGroupIterable(1, TestMessage.instanceCreator), []);
  });

  it('ensure not the same instance returned for the empty input', () => {
    const accessor = Kernel.createEmpty();
    const list1 =
        accessor.getRepeatedGroupIterable(1, TestMessage.instanceCreator);
    const list2 =
        accessor.getRepeatedGroupIterable(1, TestMessage.instanceCreator);
    expect(list1).not.toBe(list2);
  });

  it('return size for the empty input', () => {
    const accessor = Kernel.createEmpty();
    expect(accessor.getRepeatedGroupSize(1, TestMessage.instanceCreator))
        .toEqual(0);
  });

  it('return values from the input', () => {
    const bytes1 = createArrayBuffer(0x08, 0x01);
    const bytes2 = createArrayBuffer(0x08, 0x02);
    const msg1 = new TestMessage(Kernel.fromArrayBuffer(bytes1));
    const msg2 = new TestMessage(Kernel.fromArrayBuffer(bytes2));

    const bytes =
        createArrayBuffer(0x0B, 0x08, 0x01, 0x0C, 0x0B, 0x08, 0x02, 0x0C);
    const accessor = Kernel.fromArrayBuffer(bytes);
    expectEqualToMessageArray(
        accessor.getRepeatedGroupIterable(1, TestMessage.instanceCreator),
        [msg1, msg2]);
  });

  it('ensure not the same array instance returned', () => {
    const bytes =
        createArrayBuffer(0x0B, 0x08, 0x01, 0x0C, 0x0B, 0x08, 0x02, 0x0C);
    const accessor = Kernel.fromArrayBuffer(bytes);
    const list1 =
        accessor.getRepeatedGroupIterable(1, TestMessage.instanceCreator);
    const list2 =
        accessor.getRepeatedGroupIterable(1, TestMessage.instanceCreator);
    expect(list1).not.toBe(list2);
  });

  it('ensure the same array element returned for get iterable', () => {
    const bytes =
        createArrayBuffer(0x0B, 0x08, 0x01, 0x0C, 0x0B, 0x08, 0x02, 0x0C);
    const accessor = Kernel.fromArrayBuffer(bytes);
    const list1 =
        accessor.getRepeatedGroupIterable(1, TestMessage.instanceCreator);
    const list2 = accessor.getRepeatedGroupIterable(
        1, TestMessage.instanceCreator, /* pivot= */ 0);
    const array1 = Array.from(list1);
    const array2 = Array.from(list2);
    for (let i = 0; i < array1.length; i++) {
      expect(array1[i]).toBe(array2[i]);
    }
  });

  it('return accessors from the input', () => {
    const bytes =
        createArrayBuffer(0x0B, 0x08, 0x01, 0x0C, 0x0B, 0x08, 0x02, 0x0C);
    const accessor = Kernel.fromArrayBuffer(bytes);
    const [accessor1, accessor2] =
        [...accessor.getRepeatedGroupIterable(1, TestMessage.instanceCreator)];
    expect(accessor1.getInt32WithDefault(1)).toEqual(1);
    expect(accessor2.getInt32WithDefault(1)).toEqual(2);
  });

  it('return accessors from the input when pivot is set', () => {
    const bytes =
        createArrayBuffer(0x0B, 0x08, 0x01, 0x0C, 0x0B, 0x08, 0x02, 0x0C);
    const accessor = Kernel.fromArrayBuffer(bytes);
    const [accessor1, accessor2] = [...accessor.getRepeatedGroupIterable(
        1, TestMessage.instanceCreator, /* pivot= */ 0)];
    expect(accessor1.getInt32WithDefault(1)).toEqual(1);
    expect(accessor2.getInt32WithDefault(1)).toEqual(2);
  });

  it('return the repeated field element from the input', () => {
    const bytes =
        createArrayBuffer(0x0B, 0x08, 0x01, 0x0C, 0x0B, 0x08, 0x02, 0x0C);
    const accessor = Kernel.fromArrayBuffer(bytes);
    const msg1 = accessor.getRepeatedGroupElement(
        /* fieldNumber= */ 1, TestMessage.instanceCreator,
        /* index= */ 0);
    const msg2 = accessor.getRepeatedGroupElement(
        /* fieldNumber= */ 1, TestMessage.instanceCreator,
        /* index= */ 1, /* pivot= */ 0);
    expect(msg1.getInt32WithDefault(
               /* fieldNumber= */ 1, /* default= */ 0))
        .toEqual(1);
    expect(msg2.getInt32WithDefault(
               /* fieldNumber= */ 1, /* default= */ 0))
        .toEqual(2);
  });

  it('ensure the same array element returned', () => {
    const bytes =
        createArrayBuffer(0x0B, 0x08, 0x01, 0x0C, 0x0B, 0x08, 0x02, 0x0C);
    const accessor = Kernel.fromArrayBuffer(bytes);
    const msg1 = accessor.getRepeatedGroupElement(
        /* fieldNumber= */ 1, TestMessage.instanceCreator,
        /* index= */ 0);
    const msg2 = accessor.getRepeatedGroupElement(
        /* fieldNumber= */ 1, TestMessage.instanceCreator,
        /* index= */ 0);
    expect(msg1).toBe(msg2);
  });

  it('return the size from the input', () => {
    const bytes =
        createArrayBuffer(0x0B, 0x08, 0x01, 0x0C, 0x0B, 0x08, 0x02, 0x0C);
    const accessor = Kernel.fromArrayBuffer(bytes);
    expect(accessor.getRepeatedGroupSize(1, TestMessage.instanceCreator))
        .toEqual(2);
  });

  it('encode repeated message from the input', () => {
    const bytes =
        createArrayBuffer(0x0B, 0x08, 0x01, 0x0C, 0x0B, 0x08, 0x02, 0x0C);
    const accessor = Kernel.fromArrayBuffer(bytes);
    expect(accessor.serialize()).toEqual(bytes);
  });

  it('add a single value', () => {
    const accessor = Kernel.createEmpty();
    const bytes1 = createArrayBuffer(0x08, 0x01);
    const msg1 = new TestMessage(Kernel.fromArrayBuffer(bytes1));
    const bytes2 = createArrayBuffer(0x08, 0x02);
    const msg2 = new TestMessage(Kernel.fromArrayBuffer(bytes2));

    accessor.addRepeatedGroupElement(1, msg1, TestMessage.instanceCreator);
    accessor.addRepeatedGroupElement(1, msg2, TestMessage.instanceCreator);
    const result =
        accessor.getRepeatedGroupIterable(1, TestMessage.instanceCreator);

    expect(Array.from(result)).toEqual([msg1, msg2]);
  });

  it('add values', () => {
    const accessor = Kernel.createEmpty();
    const bytes1 = createArrayBuffer(0x08, 0x01);
    const msg1 = new TestMessage(Kernel.fromArrayBuffer(bytes1));
    const bytes2 = createArrayBuffer(0x08, 0x02);
    const msg2 = new TestMessage(Kernel.fromArrayBuffer(bytes2));

    accessor.addRepeatedGroupIterable(1, [msg1], TestMessage.instanceCreator);
    accessor.addRepeatedGroupIterable(1, [msg2], TestMessage.instanceCreator);
    const result =
        accessor.getRepeatedGroupIterable(1, TestMessage.instanceCreator);

    expect(Array.from(result)).toEqual([msg1, msg2]);
  });

  it('set a single value', () => {
    const bytes = createArrayBuffer(0x0B, 0x08, 0x01, 0x0C);
    const accessor = Kernel.fromArrayBuffer(bytes);
    const subbytes = createArrayBuffer(0x08, 0x01);
    const submsg = new TestMessage(Kernel.fromArrayBuffer(subbytes));

    accessor.setRepeatedGroupElement(
        /* fieldNumber= */ 1, submsg, TestMessage.instanceCreator,
        /* index= */ 0);
    const result =
        accessor.getRepeatedGroupIterable(1, TestMessage.instanceCreator);

    expect(Array.from(result)).toEqual([submsg]);
  });

  it('write submessage changes made via getRepeatedGroupElement', () => {
    const bytes = createArrayBuffer(0x0B, 0x08, 0x05, 0x0C);
    const expected = createArrayBuffer(0x0B, 0x08, 0x00, 0x0C);
    const accessor = Kernel.fromArrayBuffer(bytes);
    const submsg = accessor.getRepeatedGroupElement(
        /* fieldNumber= */ 1, TestMessage.instanceCreator,
        /* index= */ 0);
    expect(submsg.getInt32WithDefault(1, 0)).toEqual(5);
    submsg.setInt32(1, 0);

    expect(accessor.serialize()).toEqual(expected);
  });

  it('set values', () => {
    const accessor = Kernel.createEmpty();
    const subbytes = createArrayBuffer(0x08, 0x01);
    const submsg = new TestMessage(Kernel.fromArrayBuffer(subbytes));

    accessor.setRepeatedGroupIterable(1, [submsg]);
    const result =
        accessor.getRepeatedGroupIterable(1, TestMessage.instanceCreator);

    expect(Array.from(result)).toEqual([submsg]);
  });

  it('encode for adding single value', () => {
    const accessor = Kernel.createEmpty();
    const bytes1 = createArrayBuffer(0x08, 0x01);
    const msg1 = new TestMessage(Kernel.fromArrayBuffer(bytes1));
    const bytes2 = createArrayBuffer(0x08, 0x00);
    const msg2 = new TestMessage(Kernel.fromArrayBuffer(bytes2));
    const expected =
        createArrayBuffer(0x0B, 0x08, 0x01, 0x0C, 0x0B, 0x08, 0x00, 0x0C);

    accessor.addRepeatedGroupElement(1, msg1, TestMessage.instanceCreator);
    accessor.addRepeatedGroupElement(1, msg2, TestMessage.instanceCreator);
    const result = accessor.serialize();

    expect(result).toEqual(expected);
  });

  it('encode for adding values', () => {
    const accessor = Kernel.createEmpty();
    const bytes1 = createArrayBuffer(0x08, 0x01);
    const msg1 = new TestMessage(Kernel.fromArrayBuffer(bytes1));
    const bytes2 = createArrayBuffer(0x08, 0x00);
    const msg2 = new TestMessage(Kernel.fromArrayBuffer(bytes2));
    const expected =
        createArrayBuffer(0x0B, 0x08, 0x01, 0x0C, 0x0B, 0x08, 0x00, 0x0C);

    accessor.addRepeatedGroupIterable(
        1, [msg1, msg2], TestMessage.instanceCreator);
    const result = accessor.serialize();

    expect(result).toEqual(expected);
  });

  it('encode for setting single value', () => {
    const bytes = createArrayBuffer(0x0B, 0x08, 0x00, 0x0C);
    const accessor = Kernel.fromArrayBuffer(bytes);
    const subbytes = createArrayBuffer(0x08, 0x01);
    const submsg = new TestMessage(Kernel.fromArrayBuffer(subbytes));
    const expected = createArrayBuffer(0x0B, 0x08, 0x01, 0x0C);

    accessor.setRepeatedGroupElement(
        /* fieldNumber= */ 1, submsg, TestMessage.instanceCreator,
        /* index= */ 0);
    const result = accessor.serialize();

    expect(result).toEqual(expected);
  });

  it('encode for setting values', () => {
    const accessor = Kernel.createEmpty();
    const subbytes = createArrayBuffer(0x08, 0x01);
    const submsg = new TestMessage(Kernel.fromArrayBuffer(subbytes));
    const expected = createArrayBuffer(0x0B, 0x08, 0x01, 0x0C);

    accessor.setRepeatedGroupIterable(1, [submsg]);
    const result = accessor.serialize();

    expect(result).toEqual(expected);
  });

  it('fail when getting groups value with other wire types', () => {
    const accessor = Kernel.fromArrayBuffer(createArrayBuffer(
        0x09, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00));

    if (CHECK_CRITICAL_STATE) {
      expect(() => {
        accessor.getRepeatedGroupIterable(1, TestMessage.instanceCreator);
      }).toThrow();
    }
  });

  it('fail when adding group values with wrong type value', () => {
    const accessor = Kernel.createEmpty();
    const fakeValue = /** @type {!TestMessage} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(
          () => accessor.addRepeatedGroupIterable(
              1, [fakeValue], TestMessage.instanceCreator))
          .toThrowError('Given value is not a message instance: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addRepeatedGroupIterable(
          1, [fakeValue], TestMessage.instanceCreator);
      const list =
          accessor.getRepeatedGroupIterable(1, TestMessage.instanceCreator);
      expect(Array.from(list)).toEqual([null]);
    }
  });

  it('fail when adding single group value with wrong type value', () => {
    const accessor = Kernel.createEmpty();
    const fakeValue = /** @type {!TestMessage} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(
          () => accessor.addRepeatedGroupElement(
              1, fakeValue, TestMessage.instanceCreator))
          .toThrowError('Given value is not a message instance: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.addRepeatedGroupElement(
          1, fakeValue, TestMessage.instanceCreator);
      const list =
          accessor.getRepeatedGroupIterable(1, TestMessage.instanceCreator);
      expect(Array.from(list)).toEqual([null]);
    }
  });

  it('fail when setting message values with wrong type value', () => {
    const accessor = Kernel.createEmpty();
    const fakeValue = /** @type {!TestMessage} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(() => accessor.setRepeatedGroupIterable(1, [fakeValue]))
          .toThrowError('Given value is not a message instance: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setRepeatedGroupIterable(1, [fakeValue]);
      const list =
          accessor.getRepeatedGroupIterable(1, TestMessage.instanceCreator);
      expect(Array.from(list)).toEqual([null]);
    }
  });

  it('fail when setting single value with wrong type value', () => {
    const accessor =
        Kernel.fromArrayBuffer(createArrayBuffer(0x0B, 0x08, 0x00, 0x0C));
    const fakeValue = /** @type {!TestMessage} */ (/** @type {*} */ (null));
    if (CHECK_CRITICAL_STATE) {
      expect(
          () => accessor.setRepeatedGroupElement(
              /* fieldNumber= */ 1, fakeValue, TestMessage.instanceCreator,
              /* index= */ 0))
          .toThrowError('Given value is not a message instance: null');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setRepeatedGroupElement(
          /* fieldNumber= */ 1, fakeValue, TestMessage.instanceCreator,
          /* index= */ 0);
      const list =
          accessor.getRepeatedGroupIterable(1, TestMessage.instanceCreator);
      expect(Array.from(list).length).toEqual(1);
    }
  });

  it('fail when setting single value with out-of-bound index', () => {
    const accessor =
        Kernel.fromArrayBuffer(createArrayBuffer(0x0B, 0x08, 0x00, 0x0C));
    const msg1 =
        accessor.getRepeatedGroupElement(1, TestMessage.instanceCreator, 0);
    const bytes2 = createArrayBuffer(0x08, 0x01);
    const msg2 = new TestMessage(Kernel.fromArrayBuffer(bytes2));
    if (CHECK_CRITICAL_STATE) {
      expect(
          () => accessor.setRepeatedGroupElement(
              /* fieldNumber= */ 1, msg2, TestMessage.instanceCreator,
              /* index= */ 1))
          .toThrowError('Index out of bounds: index: 1 size: 1');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      accessor.setRepeatedGroupElement(
          /* fieldNumber= */ 1, msg2, TestMessage.instanceCreator,
          /* index= */ 1);
      expectEqualToArray(
          accessor.getRepeatedGroupIterable(1, TestMessage.instanceCreator),
          [msg1, msg2]);
    }
  });
});
