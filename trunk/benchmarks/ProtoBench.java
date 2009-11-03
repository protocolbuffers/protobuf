// Protocol Buffers - Google's data interchange format
// Copyright 2009 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

package com.google.protocolbuffers;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.lang.reflect.Method;

import com.google.protobuf.ByteString;
import com.google.protobuf.CodedInputStream;
import com.google.protobuf.CodedOutputStream;
import com.google.protobuf.Message;

public class ProtoBench {
  
  private static final long MIN_SAMPLE_TIME_MS = 2 * 1000;
  private static final long TARGET_TIME_MS = 30 * 1000;

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
      FileOutputStream devNullTemp = null;
      CodedOutputStream reuseDevNullTemp = null;
      try {
        devNullTemp = new FileOutputStream("/dev/null");
        reuseDevNullTemp = CodedOutputStream.newInstance(devNullTemp);
      } catch (FileNotFoundException e) {
        // ignore: this is probably Windows, where /dev/null does not exist
      }
      final FileOutputStream devNull = devNullTemp;
      final CodedOutputStream reuseDevNull = reuseDevNullTemp;
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
      if (devNull != null) {
        benchmark("Serialize to /dev/null with FileOutputStream", inputData.length, new Action() {
          public void execute() throws IOException {
            sampleMessage.writeTo(devNull);
          }
        });
        benchmark("Serialize to /dev/null reusing FileOutputStream", inputData.length, new Action() {
          public void execute() throws IOException {
            sampleMessage.writeTo(reuseDevNull);
            reuseDevNull.flush();  // force the write to the OutputStream
          }
        });
      }
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
    System.gc();    
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
