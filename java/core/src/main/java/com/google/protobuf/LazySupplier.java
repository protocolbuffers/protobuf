package com.google.protobuf;

import java.util.function.Supplier;

final class LazySupplier<T> implements Supplier<T> {

  @SuppressWarnings("unchecked")
  static <T> Supplier<T> of(Supplier<? extends T> supplier) {
    if (supplier instanceof LazySupplier) {
      return (LazySupplier<T>) supplier;
    }
    return new LazySupplier<T>(supplier);
  }

  private Supplier<? extends T> delegate;
  private volatile T value = null;
  private volatile boolean done = false;

  @Override
  public T get() {
    if (done) {
      return value;
    }
    synchronized (this) {
      if (done) {
        return value;
      }
      T value = delegate.get();
      delegate = null;
      this.value = value;
      done = true;
      return value;
    }
  }

  private LazySupplier(Supplier<? extends T> delegate) {
    this.delegate = delegate;
  }
}
