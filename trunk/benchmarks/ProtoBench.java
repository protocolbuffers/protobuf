package com.google.protocolbuffers;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.lang.reflect.Method;

import com.google.protobuf.ByteString;
import com.google.protobuf.CodedInputStream;
import com.google.protobuf.Message;

public class ProtoBench {
  
  private static long MIN_SAMPLE_TIME_MS = 2 * 1000;
  private static long TARGET_TIME_MS = 30 * 1000;

  private ProtoBench() {
    // Prevent instantiation
  }
  
  public static void main(String[] args) {
    if (args.length < 2 || (args.length % 2) != 0) {
      System.err.println("Usage: ProtoBench <descriptor type name> <input data>");
      System.err.println("The descriptor type name is the fully-qualified message name,");
      System.err.println("e.g. com.google.protocolbuffers.benchmark.Message1");
      System.err.println("(You can specify multiple pairs of descriptor type name and input data.)");
      System.exit(1);
    }
    boolean success = true;
    for (int i = 0; i < args.length; i += 2) {
      success &= runTest(args[i], args[i + 1]);
    }
    System.exit(success ? 0 : 1);
  }

  /**
   * Runs a single test. Error messages are displayed to stderr, and the return value
   * indicates general success/failure.
   */
  public static boolean runTest(String type, String file) {
    System.out.println("Benchmarking " + type + " with file " + file);
    final Message defaultMessage;
    try {
      Class<?> clazz = Class.forName(type);
      Method method = clazz.getDeclaredMethod("getDefaultInstance");
      defaultMessage = (Message) method.invoke(null);
    } catch (Exception e) {
      // We want to do the same thing with all exceptions. Not generally nice,
      // but this is slightly different.
      System.err.println("Unable to get default message for " + type);
      return false;
    }
    
    try {
      final byte[] inputData = readAllBytes(file);
      final ByteArrayInputStream inputStream = new ByteArrayInputStream(inputData);
      final ByteString inputString = ByteString.copyFrom(inputData);
      final Message sampleMessage = defaultMessage.newBuilderForType().mergeFrom(inputString).build();
      benchmark("Serialize to byte string", inputData.length, new Action() {
        public void execute() { sampleMessage.toByteString(); }
      });      
      benchmark("Serialize to byte array", inputData.length, new Action() {
        public void execute() { sampleMessage.toByteArray(); }
      });
      benchmark("Serialize to memory stream", inputData.length, new Action() {
        public void execute() throws IOException { 
          sampleMessage.writeTo(new ByteArrayOutputStream()); 
        }
      });
      benchmark("Deserialize from byte string", inputData.length, new Action() {
        public void execute() throws IOException { 
          defaultMessage.newBuilderForType().mergeFrom(inputString).build();
        }
      });
      benchmark("Deserialize from byte array", inputData.length, new Action() {
        public void execute() throws IOException { 
          defaultMessage.newBuilderForType()
            .mergeFrom(CodedInputStream.newInstance(inputData)).build();
        }
      });
      benchmark("Deserialize from memory stream", inputData.length, new Action() {
        public void execute() throws IOException { 
          defaultMessage.newBuilderForType()
            .mergeFrom(CodedInputStream.newInstance(inputStream)).build();
          inputStream.reset();
        }
      });
      System.out.println();
      return true;
    } catch (Exception e) {
      System.err.println("Error: " + e.getMessage());
      System.err.println("Detailed exception information:");
      e.printStackTrace(System.err);
      return false;
    }
  }
  
  private static void benchmark(String name, long dataSize, Action action) throws IOException {
    // Make sure it's JITted "reasonably" hard before running the first progress test
    for (int i=0; i < 100; i++) {
      action.execute();
    }
    
    // Run it progressively more times until we've got a reasonable sample
    int iterations = 1;
    long elapsed = timeAction(action, iterations);
    while (elapsed < MIN_SAMPLE_TIME_MS) {
      iterations *= 2;
      elapsed = timeAction(action, iterations);
    }
    // Upscale the sample to the target time. Do this in floating point arithmetic
    // to avoid overflow issues.
    iterations = (int) ((TARGET_TIME_MS / (double) elapsed) * iterations);
    elapsed = timeAction(action, iterations);
    System.out.println(name + ": " + iterations + " iterations in "
         + (elapsed/1000f) + "s; " 
         + (iterations * dataSize) / (elapsed * 1024 * 1024 / 1000f) 
         + "MB/s");
  }
  
  private static long timeAction(Action action, int iterations) throws IOException {
    long start = System.currentTimeMillis();
    for (int i = 0; i < iterations; i++) {
      action.execute();
    }
    long end = System.currentTimeMillis();
    return end - start;
  }
  
  private static byte[] readAllBytes(String filename) throws IOException {
    RandomAccessFile file = new RandomAccessFile(new File(filename), "r");
    byte[] content = new byte[(int) file.length()];
    file.readFully(content);
    return content;
  }

  /**
   * Interface used to capture a single action to benchmark.
   */
  interface Action {
    void execute() throws IOException;
  }
}
