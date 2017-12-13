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
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.PrintWriter;
import java.io.RandomAccessFile;
import java.util.ArrayList;
import java.util.List;


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
    argsList.add("com.google.protobuf.ProtoBenchCaliper");
    
    try {
      String args[] = new String[argsList.size()];
      argsList.toArray(args);
      CaliperMain.exitlessMain(args, 
          new PrintWriter(System.out, true), new PrintWriter(System.err, true));
      return true;
    } catch (Exception e) {
      System.err.println("Error: " + e.getMessage());
      System.err.println("Detailed exception information:");
      e.printStackTrace(System.err);
      return false;
    }
  }
  
  
  private static List<String> getCaliperOption(final BenchmarkDataset benchmarkDataset) {
    List<String> temp = new ArrayList<String>();
    if (benchmarkDataset.getMessageName().equals("benchmarks.proto3.GoogleMessage1")) {
      temp.add("-DbenchmarkMessageType=GOOGLE_MESSAGE1_PROTO3");
    } else if (benchmarkDataset.getMessageName().equals("benchmarks.proto2.GoogleMessage1")) {
      temp.add("-DbenchmarkMessageType=GOOGLE_MESSAGE1_PROTO2");
    } else if (benchmarkDataset.getMessageName().equals("benchmarks.proto2.GoogleMessage2")) {
      temp.add("-DbenchmarkMessageType=GOOGLE_MESSAGE2");
    } else if (benchmarkDataset.getMessageName().
        equals("benchmarks.google_message3.GoogleMessage3")) {
      temp.add("-DbenchmarkMessageType=GOOGLE_MESSAGE3");
    } else if (benchmarkDataset.getMessageName().
        equals("benchmarks.google_message4.GoogleMessage4")) {
      temp.add("-DbenchmarkMessageType=GOOGLE_MESSAGE4");
    } else {
      return null;
    }
    
    temp.add("-i");
    temp.add("runtime");
    temp.add("-b");
    String benchmarkNames = "serializeToByteString,serializeToByteArray,serializeToMemoryStream"
       + ",deserializeFromByteString,deserializeFromByteArray,deserializeFromMemoryStream";
    temp.add(benchmarkNames);
    temp.add("-Cinstrument.runtime.options.timingInterval=3000ms");
    
    return temp;
  }
  
  public static byte[] readAllBytes(String filename) throws IOException {
    RandomAccessFile file = new RandomAccessFile(new File(filename), "r");
    byte[] content = new byte[(int) file.length()];
    file.readFully(content);
    return content;
  }
}
