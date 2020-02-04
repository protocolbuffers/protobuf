/**
 * @fileoverview Internal interface for messages implemented with the binary
 * kernel.
 */
goog.module('protobuf.binary.InternalMessage');

const LazyAccessor = goog.requireType('protobuf.binary.LazyAccessor');

/**
 * Interface that needs to be implemented by messages implemented with the
 * binary kernel. This is an internal only interface and should be used only by
 * the classes in binary kernel.
 *
 * @interface
 */
class InternalMessage {
  /**
   * @package
   * @return {!LazyAccessor}
   */
  internalGetKernel() {}
}

exports = InternalMessage;