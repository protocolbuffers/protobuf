// Protocol Buffers - Google's data interchange format
// Copyright 2009 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
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

import com.google.protobuf.ByteString;
import com.google.protobuf.CodedInputStream;
import com.google.protobuf.CodedOutputStream;
import com.google.protobuf.ExtensionRegistry;
import com.google.protobuf.Message;
import com.google.protobuf.benchmarks.Benchmarks.BenchmarkDataset;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.util.ArrayList;
import java.util.List;

public class ProtoBench {

  private static final long MIN_SAMPLE_TIME_MS = 1 * 1000;
  private static final long TARGET_TIME_MS = 5 * 1000;

  private ProtoBench() {
    // Prevent instantiation
  }
  
  public static void main(String[] args) {
    if (args.length < 1) {
      System.err.println("Usage: ./java-benchmark <input data>");
      System.err.println("input data is in the format of \"benchmarks.proto\"");
      System.exit(1);
    }
    boolean success = true;
    for (int i = 0; i < args.length; i++) {
      success &= runTest(args[i]);
    }
    System.exit(success ? 0 : 1);
  }
  
  public static ExtensionRegistry getExtensionsRegistry(BenchmarkDataset benchmarkDataset) {
    ExtensionRegistry extensions = ExtensionRegistry.newInstance();
    if (benchmarkDataset.getMessageName().equals("benchmarks.google_message3.GoogleMessage3")) {
      benchmarks.google_message3.BenchmarkMessage38.registerAllExtensions(extensions);
      benchmarks.google_message3.BenchmarkMessage37.registerAllExtensions(extensions);
      benchmarks.google_message3.BenchmarkMessage36.registerAllExtensions(extensions);
      benchmarks.google_message3.BenchmarkMessage35.registerAllExtensions(extensions);
      benchmarks.google_message3.BenchmarkMessage34.registerAllExtensions(extensions);
      benchmarks.google_message3.BenchmarkMessage33.registerAllExtensions(extensions);
      benchmarks.google_message3.BenchmarkMessage32.registerAllExtensions(extensions);
      benchmarks.google_message3.BenchmarkMessage31.registerAllExtensions(extensions);
      benchmarks.google_message3.BenchmarkMessage3.registerAllExtensions(extensions);
    } else if (benchmarkDataset.getMessageName().equals(
        "benchmarks.google_message4.GoogleMessage4")) {
      benchmarks.google_message4.BenchmarkMessage43.registerAllExtensions(extensions);
      benchmarks.google_message4.BenchmarkMessage42.registerAllExtensions(extensions);
      benchmarks.google_message4.BenchmarkMessage41.registerAllExtensions(extensions);
      benchmarks.google_message4.BenchmarkMessage4.registerAllExtensions(extensions);      
    }
    
    return extensions;
  }

  /**
   * Return an message instance for one specific dataset, and register the extensions for that 
   * message. 
   */
  public static Message registerBenchmarks(BenchmarkDataset benchmarkDataset) {
    if (benchmarkDataset.getMessageName().equals("benchmarks.proto3.GoogleMessage1")) {
      return com.google.protobuf.benchmarks.BenchmarkMessage1Proto3.GoogleMessage1
          .getDefaultInstance();
    } else if (benchmarkDataset.getMessageName().equals("benchmarks.proto2.GoogleMessage1")) {
      return com.google.protobuf.benchmarks.BenchmarkMessage1Proto2.GoogleMessage1
          .getDefaultInstance();
    } else if (benchmarkDataset.getMessageName().equals("benchmarks.proto2.GoogleMessage2")) {
      return com.google.protobuf.benchmarks.BenchmarkMessage2.GoogleMessage2.getDefaultInstance();
    } else if (benchmarkDataset.getMessageName().
        equals("benchmarks.google_message3.GoogleMessage3")) {
      return benchmarks.google_message3.BenchmarkMessage3.GoogleMessage3.getDefaultInstance();
    } else if (benchmarkDataset.getMessageName().
        equals("benchmarks.google_message4.GoogleMessage4")) {
      return benchmarks.google_message4.BenchmarkMessage4.GoogleMessage4.getDefaultInstance();
    } else {
      return null;
    }
  }

  /**
   * Runs a single test. Error messages are displayed to stderr, and the return value indicates
   * general success/failure.
   */
  public static boolean runTest(String file) {
    final Message defaultMessage;
    BenchmarkDataset benchmarkDataset;
    ExtensionRegistry extensions;
    final byte[] inputData;

    try {
      inputData = readAllBytes(file);
      benchmarkDataset = BenchmarkDataset.parseFrom(inputData);
    } catch (IOException e) {
      System.err.println("Unable to get input data");
      return false;
    }
    defaultMessage = registerBenchmarks(benchmarkDataset);
    extensions = getExtensionsRegistry(benchmarkDataset);
    if (defaultMessage == null) {
      System.err.println("Unable to get default message " + benchmarkDataset.getMessageName());
      return false;
    }
    System.out.println("Benchmarking " + benchmarkDataset.getMessageName() + " with file " + file);

    try {
      List<byte[]> inputDataList = new ArrayList<byte[]>();
      List<ByteArrayInputStream> inputStreamList = new ArrayList<ByteArrayInputStream>();
      List<ByteString> inputStringList = new ArrayList<ByteString>();
      List<Message> sampleMessageList = new ArrayList<Message>();

      for (int i = 0; i < benchmarkDataset.getPayloadCount(); i++) {
        byte[] singleInputData = benchmarkDataset.getPayload(i).toByteArray();
        inputDataList.add(benchmarkDataset.getPayload(i).toByteArray());
        inputStreamList.add(new ByteArrayInputStream(benchmarkDataset.getPayload(i).toByteArray()));
        inputStringList.add(benchmarkDataset.getPayload(i));
        sampleMessageList.add(
            defaultMessage.newBuilderForType().mergeFrom(singleInputData, extensions).build());
      }

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
      benchmark(
          "Serialize to byte string",
          inputData.length,
          new Action() {
            public void execute() {
              for (int i = 0; i < sampleMessageList.size(); i++) {
                sampleMessageList.get(i).toByteString();
              }
            }
          });
      benchmark(
          "Serialize to byte array",
          inputData.length,
          new Action() {
            public void execute() {
              for (int i = 0; i < sampleMessageList.size(); i++) {
                sampleMessageList.get(i).toByteString();
              }
            }
          });
      benchmark(
          "Serialize to memory stream",
          inputData.length,
          new Action() {
            public void execute() throws IOException {
              ByteArrayOutputStream output = new ByteArrayOutputStream();
              for (int i = 0; i < sampleMessageList.size(); i++) {
                sampleMessageList.get(i).writeTo(output);
              }
            }
          });
      if (devNull != null) {
        benchmark(
            "Serialize to /dev/null with FileOutputStream",
            inputData.length,
            new Action() {
              public void execute() throws IOException {
                for (int i = 0; i < sampleMessageList.size(); i++) {
                  sampleMessageList.get(i).writeTo(devNull);
                }
              }
            });
        benchmark(
            "Serialize to /dev/null reusing FileOutputStream",
            inputData.length,
            new Action() {
              public void execute() throws IOException {
                for (int i = 0; i < sampleMessageList.size(); i++) {
                  sampleMessageList.get(i).writeTo(reuseDevNull);
                  reuseDevNull.flush(); // force the write to the OutputStream
                }
              }
            });
      }
      benchmark(
          "Deserialize from byte string",
          inputData.length,
          new Action() {
            public void execute() throws IOException {
              for (int i = 0; i < inputStringList.size(); i++) {
                defaultMessage
                    .newBuilderForType()
                    .mergeFrom(inputStringList.get(i), extensions)
                    .build();
              }
            }
          });
      benchmark(
          "Deserialize from byte array",
          inputData.length,
          new Action() {
            public void execute() throws IOException {
              for (int i = 0; i < inputDataList.size(); i++) {
                defaultMessage
                    .newBuilderForType()
                    .mergeFrom(CodedInputStream.newInstance(inputDataList.get(i)), extensions)
                    .build();
              }
            }
          });
      benchmark(
          "Deserialize from memory stream",
          inputData.length,
          new Action() {
            public void execute() throws IOException {
              for (int i = 0; i < inputStreamList.size(); i++) {
                defaultMessage
                    .newBuilderForType()
                    .mergeFrom(CodedInputStream.newInstance(inputStreamList.get(i)), extensions)
                    .build();
                inputStreamList.get(i).reset();
              }
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
