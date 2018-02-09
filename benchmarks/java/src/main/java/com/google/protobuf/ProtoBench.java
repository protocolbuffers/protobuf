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


package com.google.protobuf;

import com.google.caliper.BeforeExperiment;
import com.google.caliper.Benchmark;
import com.google.caliper.Param;
import com.google.caliper.runner.CaliperMain;
import com.google.protobuf.ByteString;
import com.google.protobuf.CodedInputStream;
import com.google.protobuf.CodedOutputStream;
import com.google.protobuf.ExtensionRegistry;
import com.google.protobuf.Message;
import com.google.protobuf.benchmarks.Benchmarks.BenchmarkDataset;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.EOFException;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.PrintWriter;
import java.io.RandomAccessFile;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;
import java.util.Scanner;


public class ProtoBench {

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
  
 
  /**
   * Runs a single test with specific test data. Error messages are displayed to stderr, 
   * and the return value indicates general success/failure.
   */
  public static boolean runTest(String file) {
    byte[] inputData;
    BenchmarkDataset benchmarkDataset;
    try {
      inputData = readAllBytes(file);
      benchmarkDataset = BenchmarkDataset.parseFrom(inputData);
    } catch (IOException e) {
      System.err.println("Unable to get input data");
      return false;
    }
    List<String> argsList = getCaliperOption(benchmarkDataset);
    if (argsList == null) {
      System.err.println("Unable to get default message " + benchmarkDataset.getMessageName());
      return false;
    }
    argsList.add("-DdataFile=" + file);
    argsList.add("com.google.protobuf.ProtoCaliperBenchmark");
    
    try {
      String args[] = new String[argsList.size()];
      argsList.toArray(args);
      CaliperMain.exitlessMain(args,
          new PrintWriter(System.out, true), new PrintWriter(System.err, true));
    } catch (Exception e) {
      System.err.println("Error: " + e.getMessage());
      System.err.println("Detailed exception information:");
      e.printStackTrace(System.err);
      return false;
    }
    try {
      double mininumScale = 0;
      // If the file not exist, this will throw IOException, which won't print the warning 
      // information below.
      Scanner scanner = new Scanner(new String(readAllBytes("JavaBenchmarkWarning.txt")));
      while (scanner.hasNext()) {
        mininumScale = Math.max(mininumScale, scanner.nextDouble());
      }
      scanner.close();
      
      System.out.println(
         "WARNING: This benchmark's whole iterations are not enough, consider to config caliper to "
        + "run for more time to make the result more convincing. You may change the configure "
        + "code in com.google.protobuf.ProtoBench.getCaliperOption() of benchmark " 
        + benchmarkDataset.getMessageName()
        + " to run for more time. e.g. Change the value of "
        + "instrument.runtime.options.timingInterval or value of "
        + "instrument.runtime.options.measurements to be at least "
        + Math.round(mininumScale * 10 + 1) / 10.0 
        + " times of before, then build and run the benchmark again\n");
      Files.deleteIfExists(Paths.get("JavaBenchmarkWarning.txt"));
    } catch (IOException e) {
      // The IOException here should be file not found, which means there's no warning generated by
      // The benchmark, so this IOException should be discarded.
    }
    return true;
  }
  
  
  private static List<String> getCaliperOption(final BenchmarkDataset benchmarkDataset) {
    List<String> temp = new ArrayList<String>();
    if (benchmarkDataset.getMessageName().equals("benchmarks.proto3.GoogleMessage1")) {
    } else if (benchmarkDataset.getMessageName().equals("benchmarks.proto2.GoogleMessage1")) {
    } else if (benchmarkDataset.getMessageName().equals("benchmarks.proto2.GoogleMessage2")) {
    } else if (benchmarkDataset.getMessageName().
        equals("benchmarks.google_message3.GoogleMessage3")) {
      temp.add("-Cinstrument.runtime.options.timingInterval=3000ms");
      temp.add("-Cinstrument.runtime.options.measurements=20");
    } else if (benchmarkDataset.getMessageName().
        equals("benchmarks.google_message4.GoogleMessage4")) {
      temp.add("-Cinstrument.runtime.options.timingInterval=1500ms");
      temp.add("-Cinstrument.runtime.options.measurements=20");
    } else {
      return null;
    }
    
    temp.add("-i");
    temp.add("runtime");
    temp.add("-b");
    String benchmarkNames = "serializeToByteString,serializeToByteArray,serializeToMemoryStream"
       + ",deserializeFromByteString,deserializeFromByteArray,deserializeFromMemoryStream";
    temp.add(benchmarkNames);
    
    return temp;
  }
  
  public static byte[] readAllBytes(String filename) throws IOException {
    if (filename.equals("")) {
      return new byte[0];
    }
    RandomAccessFile file = new RandomAccessFile(new File(filename), "r");
    byte[] content = new byte[(int) file.length()];
    file.readFully(content);
    return content;
  }
}
