/**
 * @fileoverview Tests for indexer.js.
 */
goog.module('protobuf.binary.IndexerTest');

goog.setTestOnly();

// Note to the reader:
// Since the index behavior changes with the checking level some of the tests
// in this file have to know which checking level is enabled to make correct
// assertions.
// Test are run in all checking levels.
const BinaryStorage = goog.require('protobuf.runtime.BinaryStorage');
const BufferDecoder = goog.require('protobuf.binary.BufferDecoder');
const WireType = goog.require('protobuf.binary.WireType');
const {CHECK_CRITICAL_STATE} = goog.require('protobuf.internal.checks');
const {Field, IndexEntry} = goog.require('protobuf.binary.field');
const {buildIndex} = goog.require('protobuf.binary.indexer');
const {createBufferDecoder} = goog.require('protobuf.binary.bufferDecoderHelper');

/**
 * Returns the number of fields stored.
 *
 * @param {!BinaryStorage} storage
 * @return {number}
 */
function getStorageSize(storage) {
  let size = 0;
  storage.forEach(() => void size++);
  return size;
}

/**
 * @type {number}
 */
const PIVOT = 1;

/**
 * Asserts a single IndexEntry at a given field number.
 * @param {!BinaryStorage} storage
 * @param {number} fieldNumber
 * @param {...!IndexEntry} expectedEntries
 */
function assertStorageEntries(storage, fieldNumber, ...expectedEntries) {
  expect(getStorageSize(storage)).toBe(1);

  const entryArray = storage.get(fieldNumber).getIndexArray();
  expect(entryArray).not.toBeUndefined();
  expect(entryArray.length).toBe(expectedEntries.length);

  for (let i = 0; i < entryArray.length; i++) {
    const storageEntry = entryArray[i];
    const expectedEntry = expectedEntries[i];

    expect(storageEntry).toBe(expectedEntry);
  }
}

describe('Indexer does', () => {
  it('return empty storage for empty array', () => {
    const storage = buildIndex(createBufferDecoder(), PIVOT);
    expect(storage).not.toBeNull();
    expect(getStorageSize(storage)).toBe(0);
  });

  it('throw for null array', () => {
    expect(
        () => buildIndex(
            /** @type {!BufferDecoder} */ (/** @type {*} */ (null)), PIVOT))
        .toThrow();
  });

  it('fail for invalid wire type (6)', () => {
    expect(() => buildIndex(createBufferDecoder(0x0E, 0x01), PIVOT))
        .toThrowError('Unexpected wire type: 6');
  });

  it('fail for invalid wire type (7)', () => {
    expect(() => buildIndex(createBufferDecoder(0x0F, 0x01), PIVOT))
        .toThrowError('Unexpected wire type: 7');
  });

  it('index varint', () => {
    const data = createBufferDecoder(0x08, 0x01, 0x08, 0x01);
    const storage = buildIndex(data, PIVOT);
    assertStorageEntries(
        storage, /* fieldNumber= */ 1,
        Field.encodeIndexEntry(WireType.VARINT, /* startIndex= */ 1),
        Field.encodeIndexEntry(WireType.VARINT, /* startIndex= */ 3));
  });

  it('index varint with two bytes field number', () => {
    const data = createBufferDecoder(0xF8, 0x01, 0x01);
    const storage = buildIndex(data, PIVOT);
    assertStorageEntries(
        storage, /* fieldNumber= */ 31,
        Field.encodeIndexEntry(WireType.VARINT, /* startIndex= */ 2));
  });

  it('fail for varints that are longer than 10 bytes', () => {
    const data = createBufferDecoder(
        0x08, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00);
    if (CHECK_CRITICAL_STATE) {
      expect(() => buildIndex(data, PIVOT))
          .toThrowError('Index out of bounds: index: 12 size: 11');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      const storage = buildIndex(data, PIVOT);
      assertStorageEntries(
          storage, /* fieldNumber= */ 1,
          Field.encodeIndexEntry(WireType.VARINT, /* startIndex= */ 1));
    }
  });

  it('fail for varints with no data', () => {
    const data = createBufferDecoder(0x08);
    expect(() => buildIndex(data, PIVOT)).toThrow();
  });

  it('index fixed64', () => {
    const data = createBufferDecoder(
        /* first= */ 0x09, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        /* second= */ 0x09, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08);
    const storage = buildIndex(data, PIVOT);
    assertStorageEntries(
        storage, /* fieldNumber= */ 1,
        Field.encodeIndexEntry(WireType.FIXED64, /* startIndex= */ 1),
        Field.encodeIndexEntry(WireType.FIXED64, /* startIndex= */ 10));
  });

  it('fail for fixed64 data missing in input', () => {
    const data =
        createBufferDecoder(0x09, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07);
    if (CHECK_CRITICAL_STATE) {
      expect(() => buildIndex(data, PIVOT))
          .toThrowError('Index out of bounds: index: 9 size: 8');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      const storage = buildIndex(data, PIVOT);
      assertStorageEntries(
          storage, /* fieldNumber= */ 1,
          Field.encodeIndexEntry(WireType.FIXED64, /* startIndex= */ 1));
    }
  });

  it('fail for fixed64 tag that has no data after it', () => {
    if (CHECK_CRITICAL_STATE) {
      const data = createBufferDecoder(0x09);
      expect(() => buildIndex(data, PIVOT))
          .toThrowError('Index out of bounds: index: 9 size: 1');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      const data = createBufferDecoder(0x09);
      const storage = buildIndex(data, PIVOT);
      assertStorageEntries(
          storage, /* fieldNumber= */ 1,
          Field.encodeIndexEntry(WireType.FIXED64, /* startIndex= */ 1));
    }
  });

  it('index delimited', () => {
    const data = createBufferDecoder(
        /* first= */ 0x0A, 0x02, 0x00, 0x01, /* second= */ 0x0A, 0x02, 0x00,
        0x01);
    const storage = buildIndex(data, PIVOT);
    assertStorageEntries(
        storage, /* fieldNumber= */ 1,
        Field.encodeIndexEntry(WireType.DELIMITED, /* startIndex= */ 1),
        Field.encodeIndexEntry(WireType.DELIMITED, /* startIndex= */ 5));
  });

  it('fail for length deliimted field data missing in input', () => {
    const data = createBufferDecoder(0x0A, 0x04, 0x00, 0x01);
    if (CHECK_CRITICAL_STATE) {
      expect(() => buildIndex(data, PIVOT))
          .toThrowError('Index out of bounds: index: 6 size: 4');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      const storage = buildIndex(data, PIVOT);
      assertStorageEntries(
          storage, /* fieldNumber= */ 1,
          Field.encodeIndexEntry(WireType.DELIMITED, /* startIndex= */ 1));
    }
  });

  it('fail for delimited tag that has no data after it', () => {
    const data = createBufferDecoder(0x0A);
    expect(() => buildIndex(data, PIVOT)).toThrow();
  });

  it('index fixed32', () => {
    const data = createBufferDecoder(
        /* first= */ 0x0D, 0x01, 0x02, 0x03, 0x04, /* second= */ 0x0D, 0x01,
        0x02, 0x03, 0x04);
    const storage = buildIndex(data, PIVOT);
    assertStorageEntries(
        storage, /* fieldNumber= */ 1,
        Field.encodeIndexEntry(WireType.FIXED32, /* startIndex= */ 1),
        Field.encodeIndexEntry(WireType.FIXED32, /* startIndex= */ 6));
  });

  it('fail for fixed32 data missing in input', () => {
    const data = createBufferDecoder(0x0D, 0x01, 0x02, 0x03);

    if (CHECK_CRITICAL_STATE) {
      expect(() => buildIndex(data, PIVOT))
          .toThrowError('Index out of bounds: index: 5 size: 4');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      const storage = buildIndex(data, PIVOT);
      assertStorageEntries(
          storage, /* fieldNumber= */ 1,
          Field.encodeIndexEntry(WireType.FIXED32, /* startIndex= */ 1));
    }
  });

  it('fail for fixed32 tag that has no data after it', () => {
    if (CHECK_CRITICAL_STATE) {
      const data = createBufferDecoder(0x0D);
      expect(() => buildIndex(data, PIVOT))
          .toThrowError('Index out of bounds: index: 5 size: 1');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      const data = createBufferDecoder(0x0D);
      const storage = buildIndex(data, PIVOT);
      assertStorageEntries(
          storage, /* fieldNumber= */ 1,
          Field.encodeIndexEntry(WireType.FIXED32, /* startIndex= */ 1));
    }
  });

  it('index group', () => {
    const data = createBufferDecoder(
        /* first= */ 0x0B, 0x08, 0x01, 0x0C, /* second= */ 0x0B, 0x08, 0x01,
        0x0C);
    const storage = buildIndex(data, PIVOT);
    assertStorageEntries(
        storage, /* fieldNumber= */ 1,
        Field.encodeIndexEntry(WireType.START_GROUP, /* startIndex= */ 1),
        Field.encodeIndexEntry(WireType.START_GROUP, /* startIndex= */ 5));
  });

  it('index group and skips inner group', () => {
    const data =
        createBufferDecoder(0x0B, 0x0B, 0x08, 0x01, 0x0C, 0x08, 0x01, 0x0C);
    const storage = buildIndex(data, PIVOT);
    assertStorageEntries(
        storage, /* fieldNumber= */ 1,
        Field.encodeIndexEntry(WireType.START_GROUP, /* startIndex= */ 1));
  });

  it('fail on unmatched stop group', () => {
    const data = createBufferDecoder(0x0C, 0x01);
    expect(() => buildIndex(data, PIVOT))
        .toThrowError('Unexpected wire type: 4');
  });

  it('fail for groups without matching stop group', () => {
    const data = createBufferDecoder(0x0B, 0x08, 0x01, 0x1C);
    if (CHECK_CRITICAL_STATE) {
      expect(() => buildIndex(data, PIVOT))
          .toThrowError('Expected stop group for fieldnumber 1 not found.');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      const storage = buildIndex(data, PIVOT);
      assertStorageEntries(
          storage, /* fieldNumber= */ 1,
          Field.encodeIndexEntry(WireType.START_GROUP, /* startIndex= */ 1));
    }
  });

  it('fail for groups without stop group', () => {
    const data = createBufferDecoder(0x0B, 0x08, 0x01);
    if (CHECK_CRITICAL_STATE) {
      expect(() => buildIndex(data, PIVOT)).toThrowError('No end group found.');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      const storage = buildIndex(data, PIVOT);
      assertStorageEntries(
          storage, /* fieldNumber= */ 1,
          Field.encodeIndexEntry(WireType.START_GROUP, /* startIndex= */ 1));
    }
  });

  it('fail for group tag that has no data after it', () => {
    const data = createBufferDecoder(0x0B);
    if (CHECK_CRITICAL_STATE) {
      expect(() => buildIndex(data, PIVOT)).toThrowError('No end group found.');
    } else {
      // Note in unchecked mode we produce invalid output for invalid inputs.
      // This test just documents our behavior in those cases.
      // These values might change at any point and are not considered
      // what the implementation should be doing here.
      const storage = buildIndex(data, PIVOT);
      assertStorageEntries(
          storage, /* fieldNumber= */ 1,
          Field.encodeIndexEntry(WireType.START_GROUP, /* startIndex= */ 1));
    }
  });

  it('index too large tag', () => {
    const data = createBufferDecoder(0xF8, 0xFF, 0xFF, 0xFF, 0xFF);
    expect(() => buildIndex(data, PIVOT)).toThrow();
  });

  it('fail for varint tag that has no data after it', () => {
    const data = createBufferDecoder(0x08);
    expect(() => buildIndex(data, PIVOT)).toThrow();
  });
});
