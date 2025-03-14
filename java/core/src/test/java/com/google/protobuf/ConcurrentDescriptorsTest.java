package com.google.protobuf;

import com.google.protobuf.Descriptors.FieldDescriptor;
import proto2_unittest.UnittestProto.TestAllTypes;
import java.util.concurrent.CountDownLatch;
import java.util.function.Supplier;
import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public final class ConcurrentDescriptorsTest {
  public static final int N = 100;

  static class Worker implements Runnable {
    private final CountDownLatch startSignal;
    private final CountDownLatch doneSignal;
    private final Supplier<Object> trigger;

    Worker(CountDownLatch startSignal, CountDownLatch doneSignal, Supplier<Object> trigger) {
      this.startSignal = startSignal;
      this.doneSignal = doneSignal;
      this.trigger = trigger;
    }

    @Override
    public void run() {
      try {
        startSignal.await();
        var unused = trigger.get();
        doneSignal.countDown();
      } catch (InterruptedException e) {
        Assert.fail("InterruptedException in worker thread: " + e);
      }
    }
  }

  @Test
  public void testSimultaneousStaticInit() {
    try {
      CountDownLatch startSignal = new CountDownLatch(1);
      CountDownLatch doneSignal = new CountDownLatch(N);

      for (int i = 0; i < N; ++i) {
        // getDescriptor() is a static method and should trigger static initialization.
        new Thread(new Worker(startSignal, doneSignal, TestAllTypes::getDescriptor)).start();
      }
      startSignal.countDown();
      doneSignal.await();
      System.out.println("Done with all threads...");
    } catch (InterruptedException e) {
      Assert.fail("InterruptedException in test: " + e);
    } catch (RuntimeException e) {
      Assert.fail("Exception in test: " + e);
    }
  }

  @Test
  public void testSimultaneousFeatureAccess() {
    try {
      CountDownLatch startSignal = new CountDownLatch(1);
      CountDownLatch doneSignal = new CountDownLatch(N);

      for (int i = 0; i < N; ++i) {
        FieldDescriptor fieldDescriptor =
            TestAllTypes.getDescriptor().findFieldByName("optional_int32");
        new Thread(
                new Worker(
                    // legacyEnumFieldTreatedAsClosed is a feature accessor, which may
                    // trigger lazy feature resolution for old OSS gencode.
                    startSignal, doneSignal, fieldDescriptor::legacyEnumFieldTreatedAsClosed))
            .start();
      }
      startSignal.countDown();
      doneSignal.await();
      System.out.println("Done with all threads...");
    } catch (InterruptedException e) {
      Assert.fail("InterruptedException in test: " + e);
    } catch (RuntimeException e) {
      Assert.fail("Exception in test: " + e);
    }
  }
}
