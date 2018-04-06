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
    
    System.exit(runTest(args) ? 0 : 1);
  }
  
  public static boolean runTest(String args[]) {
    List<String> argsList = getCaliperOption(args);
    argsList.add("com.google.protobuf.ProtoCaliperBenchmark");
    
    try {
      String newArgs[] = new String[argsList.size()];
      argsList.toArray(newArgs);
      CaliperMain.exitlessMain(newArgs,
          new PrintWriter(System.out, true), new PrintWriter(System.err, true));
    } catch (Exception e) {
      System.err.println("Error: " + e.getMessage());
      System.err.println("Detailed exception information:");
      e.printStackTrace(System.err);
      return false;
    }
    return true;
  }
  
  private static List<String> getCaliperOption(String args[]) {
    List<String> temp = new ArrayList<String>();
    temp.add("-i");
    temp.add("runtime");
    String files = "";
    for (int i = 0; i < args.length; i++) {
      if (args[i].charAt(0) == '-') {
        temp.add(args[i]);
      } else {
        files += (files.equals("") ? "" : ",") + args[i];
      }
    }
    temp.add("-DdataFile=" + files);
    temp.add("-b");
    String benchmarkNames = "serializeToByteString,serializeToByteArray,serializeToMemoryStream"
       + ",deserializeFromByteString,deserializeFromByteArray,deserializeFromMemoryStream";
    temp.add(benchmarkNames);
    
    return temp;
  }
}
