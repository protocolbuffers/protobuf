/**
 * @fileoverview Ensures types are live that would be live in a typical g3
 * JS program.
 *
 * Making certain constructs live ensures that we compare against the same
 * baseline for all code size benchmarks. This increases the size
 * of our benchmarks, but note that this size in a regular app would be
 * attributes to other places.
 */
goog.module('protobuf.benchmark.codeSize.codeSizeBase');


/**
 * Ensures that the array iterator polyfill is live.
 * @return {string}
 */
function useArrayIterator() {
  let a = [];
  let s = '';
  for (let value of a) {
    s += value;
  }
  return s;
}

/**
 * Ensures that the symbol iterator polyfill is live.
 * @return {string}
 */
function useSymbolIterator() {
  /**
   * @implements {Iterable}
   */
  class Foo {
    /** @return {!Iterator} */
    [Symbol.iterator]() {}
  }

  let foo = new Foo();
  let s = '';
  for (let value of foo) {
    s += value;
  }
  return s;
}

/**
 * Ensures certain base libs are live so we can have an apples to apples
 * comparison for code size of different implementations
 */
function ensureCommonBaseLine() {
  goog.global['__hiddenTest'] += useArrayIterator();
  goog.global['__hiddenTest'] += useSymbolIterator();
}


exports = {ensureCommonBaseLine};
