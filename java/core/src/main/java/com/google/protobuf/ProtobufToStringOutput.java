package com.google.protobuf;

import java.util.concurrent.Callable;

/**
 * ProtobufToStringOutput controls the output format of {@link Message#toString()}. Specifically,
 * for the Runnable object passed to APIs likes `runWithDebugFormat` and `runWithTextFormat`,
 * Message.toString() will always output the specified format unless ProtobufToStringOutput is used
 * again to change the output format. If the output of the computation is not needed, you can use
 * `callWith.*` APIs that takes a Callable object instead of a Runnable object.
 */
public final class ProtobufToStringOutput {
  private enum OutputMode {
    DEBUG_FORMAT,
    TEXT_FORMAT
  }

  private static final ThreadLocal<OutputMode> outputMode =
      new ThreadLocal<OutputMode>() {
        @Override
        protected OutputMode initialValue() {
          return OutputMode.TEXT_FORMAT;
        }
      };

  private ProtobufToStringOutput() {}

  @CanIgnoreReturnValue
  private static OutputMode setOutputMode(OutputMode newMode) {
    OutputMode oldMode = outputMode.get();
    outputMode.set(newMode);
    return oldMode;
  }

  private static void runWithSpecificFormat(Runnable impl, OutputMode mode) {
    OutputMode oldMode = setOutputMode(mode);
    try {
      impl.run();
    } finally {
      OutputMode unused = setOutputMode(oldMode);
    }
  }

  private static <T> T callWithSpecificFormatWithoutException(Callable<T> impl, OutputMode mode) {
    OutputMode oldMode = setOutputMode(mode);
    try {
      return impl.call();
    } catch (Exception e) {
      return null;
    } finally {
      OutputMode unused = setOutputMode(oldMode);
    }
  }

  private static <T> T callWithSpecificFormatWithException(Callable<T> impl, OutputMode mode)
      throws Exception {
    OutputMode oldMode = setOutputMode(mode);
    try {
      return impl.call();
    } finally {
      OutputMode unused = setOutputMode(oldMode);
    }
  }

  public static void runWithDebugFormat(Runnable impl) {
    runWithSpecificFormat(impl, OutputMode.DEBUG_FORMAT);
  }

  public static void runWithTextFormat(Runnable impl) {
    runWithSpecificFormat(impl, OutputMode.TEXT_FORMAT);
  }

  public static <T> T callWithDebugFormatWithException(Callable<T> impl) throws Exception {
    return callWithSpecificFormatWithException(impl, OutputMode.DEBUG_FORMAT);
  }

  public static <T> T callWithTextFormatWithException(Callable<T> impl) throws Exception {
    return callWithSpecificFormatWithException(impl, OutputMode.TEXT_FORMAT);
  }

  public static <T> T callWithDebugFormatWithoutException(Callable<T> impl) {
    return callWithSpecificFormatWithoutException(impl, OutputMode.DEBUG_FORMAT);
  }

  public static <T> T callWithTextFormatWithoutException(Callable<T> impl) {
    return callWithSpecificFormatWithoutException(impl, OutputMode.TEXT_FORMAT);
  }

  public static boolean shouldOutputDebugFormat() {
    return outputMode.get() == OutputMode.DEBUG_FORMAT;
  }
}
