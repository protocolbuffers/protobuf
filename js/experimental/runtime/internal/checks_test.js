/**
 * @fileoverview Tests for checks.js.
 */
goog.module('protobuf.internal.checksTest');

const {CHECK_TYPE, checkDefAndNotNull, checkFunctionExists} = goog.require('protobuf.internal.checks');

describe('checkDefAndNotNull', () => {
  it('throws if undefined', () => {
    let value;
    if (CHECK_TYPE) {
      expect(() => checkDefAndNotNull(value)).toThrow();
    } else {
      expect(checkDefAndNotNull(value)).toBeUndefined();
    }
  });

  it('throws if null', () => {
    const value = null;
    if (CHECK_TYPE) {
      expect(() => checkDefAndNotNull(value)).toThrow();
    } else {
      expect(checkDefAndNotNull(value)).toBeNull();
    }
  });

  it('does not throw if empty string', () => {
    const value = '';
    expect(checkDefAndNotNull(value)).toEqual('');
  });
});

describe('checkFunctionExists', () => {
  it('throws if the function is undefined', () => {
    let foo = /** @type {function()} */ (/** @type {*} */ (undefined));
    if (CHECK_TYPE) {
      expect(() => checkFunctionExists(foo)).toThrow();
    } else {
      checkFunctionExists(foo);
    }
  });

  it('throws if the property is defined but not a function', () => {
    let foo = /** @type {function()} */ (/** @type {*} */ (1));
    if (CHECK_TYPE) {
      expect(() => checkFunctionExists(foo)).toThrow();
    } else {
      checkFunctionExists(foo);
    }
  });

  it('does not throw if the function is defined', () => {
    function foo(x) {
      return x;
    }
    checkFunctionExists(foo);
  });
});