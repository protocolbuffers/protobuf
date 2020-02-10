/**
 * @fileoverview Tests for storage.js.
 */
goog.module('protobuf.binary.StorageTest');

goog.setTestOnly();

const Storage = goog.require('protobuf.binary.Storage');
const {Field} = goog.require('protobuf.binary.field');

/**
 * @type {number}
 */
const DEFAULT_PIVOT = 24;

const /** !Field */ field1 =
    Field.fromDecodedValue(/* decodedValue= */ 1, /* encoder= */ () => {});
const /** !Field */ field2 =
    Field.fromDecodedValue(/* decodedValue= */ 2, /* encoder= */ () => {});
const /** !Field */ field3 =
    Field.fromDecodedValue(/* decodedValue= */ 3, /* encoder= */ () => {});
const /** !Field */ field4 =
    Field.fromDecodedValue(/* decodedValue= */ 4, /* encoder= */ () => {});

/**
 * Returns the number of fields stored.
 *
 * @param {!Storage} storage
 * @return {number}
 */
function getStorageSize(storage) {
  let size = 0;
  storage.forEach(() => void size++);
  return size;
}

describe('Storage', () => {
  it('sets and gets a field not greater than the pivot', () => {
    const storage = new Storage(DEFAULT_PIVOT);

    storage.set(1, field1);
    storage.set(DEFAULT_PIVOT, field2);

    expect(storage.getPivot()).toBe(DEFAULT_PIVOT);
    expect(storage.get(1)).toBe(field1);
    expect(storage.get(DEFAULT_PIVOT)).toBe(field2);
  });

  it('sets and gets a field greater than the pivot', () => {
    const storage = new Storage(DEFAULT_PIVOT);

    storage.set(DEFAULT_PIVOT + 1, field1);
    storage.set(100000, field2);

    expect(storage.get(DEFAULT_PIVOT + 1)).toBe(field1);
    expect(storage.get(100000)).toBe(field2);
  });

  it('sets and gets a field when pivot is zero', () => {
    const storage = new Storage(0);

    storage.set(0, field1);
    storage.set(100000, field2);

    expect(storage.getPivot()).toBe(0);
    expect(storage.get(0)).toBe(field1);
    expect(storage.get(100000)).toBe(field2);
  });

  it('sets and gets a field when pivot is undefined', () => {
    const storage = new Storage();

    storage.set(0, field1);
    storage.set(DEFAULT_PIVOT, field2);
    storage.set(DEFAULT_PIVOT + 1, field3);

    expect(storage.getPivot()).toBe(DEFAULT_PIVOT);
    expect(storage.get(0)).toBe(field1);
    expect(storage.get(DEFAULT_PIVOT)).toBe(field2);
    expect(storage.get(DEFAULT_PIVOT + 1)).toBe(field3);
  });

  it('returns undefined for nonexistent fields', () => {
    const storage = new Storage(DEFAULT_PIVOT);

    expect(storage.get(1)).toBeUndefined();
    expect(storage.get(DEFAULT_PIVOT)).toBeUndefined();
    expect(storage.get(DEFAULT_PIVOT + 1)).toBeUndefined();
    expect(storage.get(100000)).toBeUndefined();
  });

  it('returns undefined for nonexistent fields after map initialization',
     () => {
       const storage = new Storage(DEFAULT_PIVOT);
       storage.set(100001, field1);

       expect(storage.get(1)).toBeUndefined();
       expect(storage.get(DEFAULT_PIVOT)).toBeUndefined();
       expect(storage.get(DEFAULT_PIVOT + 1)).toBeUndefined();
       expect(storage.get(100000)).toBeUndefined();
     });

  it('deletes a field in delete() when values are only in array', () => {
    const storage = new Storage(DEFAULT_PIVOT);
    storage.set(1, field1);

    storage.delete(1);

    expect(storage.get(1)).toBeUndefined();
  });

  it('deletes a field in delete() when values are both in array and map',
     () => {
       const storage = new Storage(DEFAULT_PIVOT);
       storage.set(DEFAULT_PIVOT, field2);
       storage.set(DEFAULT_PIVOT + 1, field3);

       storage.delete(DEFAULT_PIVOT);
       storage.delete(DEFAULT_PIVOT + 1);

       expect(storage.get(DEFAULT_PIVOT)).toBeUndefined();
       expect(storage.get(DEFAULT_PIVOT + 1)).toBeUndefined();
     });

  it('deletes a field in delete() when values are only in map', () => {
    const storage = new Storage(DEFAULT_PIVOT);
    storage.set(100000, field4);

    storage.delete(100000);

    expect(storage.get(100000)).toBeUndefined();
  });

  it('loops over all the elements in forEach()', () => {
    const storage = new Storage(DEFAULT_PIVOT);
    storage.set(1, field1);
    storage.set(DEFAULT_PIVOT, field2);
    storage.set(DEFAULT_PIVOT + 1, field3);
    storage.set(100000, field4);

    const fields = new Map();
    storage.forEach(
        (field, fieldNumber) => void fields.set(fieldNumber, field));

    expect(fields.size).toEqual(4);
    expect(fields.get(1)).toBe(field1);
    expect(storage.get(DEFAULT_PIVOT)).toBe(field2);
    expect(storage.get(DEFAULT_PIVOT + 1)).toBe(field3);
    expect(fields.get(100000)).toBe(field4);
  });

  it('creates a shallow copy of the storage in shallowCopy()', () => {
    const storage = new Storage(DEFAULT_PIVOT);
    storage.set(1, field1);
    storage.set(100000, field2);

    const copy = storage.shallowCopy();

    expect(getStorageSize(copy)).toEqual(2);
    expect(copy.get(1)).not.toBe(field1);
    expect(copy.get(1).getDecodedValue()).toEqual(1);
    expect(copy.get(100000)).not.toBe(field1);
    expect(copy.get(100000).getDecodedValue()).toEqual(2);
  });
});
