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
import java.nio.file.*;
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
    int endIndex = args.length;
    for (int i = 0; i < args.length; i++) {
      if (args[i].equals("--")) {
        endIndex = i;
        break;
      }
    }
    for (int i = 0; i < endIndex; i++) {
      success &= runTest(args, i, endIndex);
    }
    System.exit(success ? 0 : 1);
  }
  
 
  /**
   * Runs a single test with specific test data. Error messages are displayed to stderr, 
   * and the return value indicates general success/failure.
   */
  public static boolean runTest(String[] args, int currentIndex, int endIndex) {
    String file = args[currentIndex];
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
    String[] caliperArgs = new String[argsList.size() + Math.max(args.length - endIndex - 1, 0)];
    argsList.toArray(caliperArgs);
    if (endIndex != args.length) {
      System.arraycopy(args, endIndex + 1, 
          caliperArgs, argsList.size(), args.length - endIndex - 1);
    }
    
    try {
      CaliperMain.exitlessMain(caliperArgs, 
          new PrintWriter(System.out, true), new PrintWriter(System.err, true));
    } catch (Exception e) {
      System.err.println("Error: " + e.getMessage());
      System.err.println("Detailed exception information:");
      e.printStackTrace(System.err);
      return false;
    }
    try {
      double maxTimeIntervalScale = 0;
      Scanner scanner = new Scanner(new String(readAllBytes("JavaBenchmarkWarning.txt")));
      while (scanner.hasNext()) {
        maxTimeIntervalScale = Math.max(maxTimeIntervalScale, scanner.nextDouble());
      }
      scanner.close();

      String timingIntervalWithUnit = getFromArgs(caliperArgs, 
          "instrument.runtime.options.timingInterval");
      if (timingIntervalWithUnit.equals("")) {
        timingIntervalWithUnit = "500ms";
      }
      int doubleValueEndIndex = timingIntervalWithUnit.length();
      double timingIntervalValue = 0;
      for (int i = timingIntervalWithUnit.length(); i >= 0 ; i--) {
        try {
          timingIntervalValue = Double.parseDouble(timingIntervalWithUnit.substring(0, i));
          doubleValueEndIndex = i;
          break;
        } catch (NumberFormatException e) {
          
        } catch (NullPointerException e) {
          
        }
      } 
      timingIntervalValue *= maxTimeIntervalScale * 1.2;
      String measurementsString = getFromArgs(caliperArgs, 
          "instrument.runtime.options.measurements");
      if (measurementsString.equals("")) {
        measurementsString = "9";
      }
      int measurements = (int) (Integer.parseInt(measurementsString) * 1.2 * maxTimeIntervalScale);
      
      System.out.println(
         "WARNING: This benchmark's whole iterations are not enough, consider to config caliper to "
        + "run for more time to make the result more convincing. e.g. try to run \"./java-benchmark"
        + " " + file + " -- -Cinstrument.runtime.options.timingInterval=" 
        + (int) Math.ceil(timingIntervalValue) 
        + timingIntervalWithUnit.substring(doubleValueEndIndex) 
        + "\" or try to run \"./java-benchmark " + file + " -- -Cinstrument.runtime.options."
        + "measurements=" + measurements + "\" again.");
      Files.deleteIfExists(Paths.get("JavaBenchmarkWarning.txt"));
    } catch (IOException e) {
      
    }
    return true;
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
      temp.add("-Cinstrument.runtime.options.timingInterval=3000ms");
      temp.add("-Cinstrument.runtime.options.measurements=20");
    } else if (benchmarkDataset.getMessageName().
        equals("benchmarks.google_message4.GoogleMessage4")) {
      temp.add("-DbenchmarkMessageType=GOOGLE_MESSAGE4");
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
    temp.add("--print-config");
    
    return temp;
  }
  
  public static byte[] readAllBytes(String filename) throws IOException {
    RandomAccessFile file = new RandomAccessFile(new File(filename), "r");
    byte[] content = new byte[(int) file.length()];
    file.readFully(content);
    return content;
  }
  
  private static String getFromArgs(String[] args, String argName) {
    String ans = "";
    for (String arg : args) {
      int startIndex = arg.lastIndexOf(argName + "="); 
      if (startIndex != -1) {
        ans = arg.substring(startIndex + argName.length() + 1);
      }
    }
    return ans;
  }
}
