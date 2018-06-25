
package com.google.protobuf;

import com.google.caliper.BeforeExperiment;
import com.google.caliper.AfterExperiment;
import com.google.caliper.Benchmark;
import com.google.caliper.Param;
import com.google.caliper.api.VmOptions;
import com.google.protobuf.ByteString;
import com.google.protobuf.CodedOutputStream;
import com.google.protobuf.ExtensionRegistry;
import com.google.protobuf.Message;
import com.google.protobuf.benchmarks.Benchmarks.BenchmarkDataset;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.util.ArrayList;
import java.util.List;

// Caliper set CICompilerCount to 1 for making sure compilation doesn't run in parallel with itself,
// This makes TieredCompilation not working. We just disable TieredCompilation by default. In master 
// branch this has been disabled by default in caliper: 
// https://github.com/google/caliper/blob/master/caliper-runner/src/main/java/com/google/caliper/runner/target/Jvm.java#L38:14
// But this haven't been added into most recent release.
@VmOptions("-XX:-TieredCompilation")
public class ProtoCaliperBenchmark {
  public enum BenchmarkMessageType {
    GOOGLE_MESSAGE1_PROTO3 {
      @Override ExtensionRegistry getExtensionRegistry() { return ExtensionRegistry.newInstance(); }
      @Override
      Message getDefaultInstance() {
        return com.google.protobuf.benchmarks.BenchmarkMessage1Proto3.GoogleMessage1
            .getDefaultInstance();
      }
    },
    GOOGLE_MESSAGE1_PROTO2 {
      @Override ExtensionRegistry getExtensionRegistry() { return ExtensionRegistry.newInstance(); }
      @Override
      Message getDefaultInstance() {
        return com.google.protobuf.benchmarks.BenchmarkMessage1Proto2.GoogleMessage1
            .getDefaultInstance();
      }
    },
    GOOGLE_MESSAGE2 {
      @Override ExtensionRegistry getExtensionRegistry() { return ExtensionRegistry.newInstance(); }
      @Override
      Message getDefaultInstance() {
        return com.google.protobuf.benchmarks.BenchmarkMessage2.GoogleMessage2.getDefaultInstance();
      }
    },
    GOOGLE_MESSAGE3 {
      @Override
      ExtensionRegistry getExtensionRegistry() {
        ExtensionRegistry extensions = ExtensionRegistry.newInstance();
        com.google.protobuf.benchmarks.BenchmarkMessage38.registerAllExtensions(extensions);
        com.google.protobuf.benchmarks.BenchmarkMessage37.registerAllExtensions(extensions);
        com.google.protobuf.benchmarks.BenchmarkMessage36.registerAllExtensions(extensions);
        com.google.protobuf.benchmarks.BenchmarkMessage35.registerAllExtensions(extensions);
        com.google.protobuf.benchmarks.BenchmarkMessage34.registerAllExtensions(extensions);
        com.google.protobuf.benchmarks.BenchmarkMessage33.registerAllExtensions(extensions);
        com.google.protobuf.benchmarks.BenchmarkMessage32.registerAllExtensions(extensions);
        com.google.protobuf.benchmarks.BenchmarkMessage31.registerAllExtensions(extensions);
        com.google.protobuf.benchmarks.BenchmarkMessage3.registerAllExtensions(extensions);
        return extensions;
      }
      @Override
      Message getDefaultInstance() {
        return com.google.protobuf.benchmarks.BenchmarkMessage3.GoogleMessage3.getDefaultInstance();
      }
    },
    GOOGLE_MESSAGE4 {
      @Override
      ExtensionRegistry getExtensionRegistry() {
        ExtensionRegistry extensions = ExtensionRegistry.newInstance();
        com.google.protobuf.benchmarks.BenchmarkMessage43.registerAllExtensions(extensions);
        com.google.protobuf.benchmarks.BenchmarkMessage42.registerAllExtensions(extensions);
        com.google.protobuf.benchmarks.BenchmarkMessage41.registerAllExtensions(extensions);
        com.google.protobuf.benchmarks.BenchmarkMessage4.registerAllExtensions(extensions);
        return extensions;
      }
      @Override
      Message getDefaultInstance() {
        return com.google.protobuf.benchmarks.BenchmarkMessage4.GoogleMessage4.getDefaultInstance();
      }
    };
    
    abstract ExtensionRegistry getExtensionRegistry();
    abstract Message getDefaultInstance();
  }

  private BenchmarkMessageType benchmarkMessageType;
  @Param("")
  private String dataFile;
  
  private byte[] inputData;
  private BenchmarkDataset benchmarkDataset;
  private Message defaultMessage;
  private ExtensionRegistry extensions;
  private List<byte[]> inputDataList;
  private List<ByteArrayInputStream> inputStreamList;
  private List<ByteString> inputStringList;
  private List<Message> sampleMessageList;

  private BenchmarkMessageType getMessageType() throws IOException {
    if (benchmarkDataset.getMessageName().equals("benchmarks.proto3.GoogleMessage1")) {
      return BenchmarkMessageType.GOOGLE_MESSAGE1_PROTO3;
    } else if (benchmarkDataset.getMessageName().equals("benchmarks.proto2.GoogleMessage1")) {
      return BenchmarkMessageType.GOOGLE_MESSAGE1_PROTO2;
    } else if (benchmarkDataset.getMessageName().equals("benchmarks.proto2.GoogleMessage2")) {
      return BenchmarkMessageType.GOOGLE_MESSAGE2;
    } else if (benchmarkDataset.getMessageName().
        equals("benchmarks.google_message3.GoogleMessage3")) {
      return BenchmarkMessageType.GOOGLE_MESSAGE3;
    } else if (benchmarkDataset.getMessageName().
        equals("benchmarks.google_message4.GoogleMessage4")) {
      return BenchmarkMessageType.GOOGLE_MESSAGE4;
    } else {
      throw new IllegalStateException("Invalid DataFile! There's no testing message named "
          + benchmarkDataset.getMessageName());
    }
  }
  
  @BeforeExperiment
  void setUp() throws IOException {
    if (!dataFile.equals("")) {
      RandomAccessFile file = new RandomAccessFile(new File(dataFile), "r");
      inputData = new byte[(int) file.length()];
      file.readFully(inputData);
      benchmarkDataset = BenchmarkDataset.parseFrom(inputData);
      benchmarkMessageType = getMessageType();
    } else {
      inputData = new byte[0];
      benchmarkDataset = BenchmarkDataset.parseFrom(inputData);
      benchmarkMessageType = BenchmarkMessageType.GOOGLE_MESSAGE2;
    }
    defaultMessage = benchmarkMessageType.getDefaultInstance();
    extensions = benchmarkMessageType.getExtensionRegistry();
    inputDataList = new ArrayList<byte[]>();
    inputStreamList = new ArrayList<ByteArrayInputStream>();
    inputStringList = new ArrayList<ByteString>();
    sampleMessageList = new ArrayList<Message>();
    
    for (int i = 0; i < benchmarkDataset.getPayloadCount(); i++) {
      byte[] singleInputData = benchmarkDataset.getPayload(i).toByteArray();
      inputDataList.add(benchmarkDataset.getPayload(i).toByteArray());
      inputStreamList.add(new ByteArrayInputStream(
          benchmarkDataset.getPayload(i).toByteArray()));
      inputStringList.add(benchmarkDataset.getPayload(i));
      sampleMessageList.add(
          defaultMessage.newBuilderForType().mergeFrom(singleInputData, extensions).build());
    }
  }
  
  
  @Benchmark
  void serializeToByteArray(int reps) throws IOException {
    if (sampleMessageList.size() == 0) {
      return;
    }
    for (int i = 0; i < reps; i++) {
      for (int j = 0; j < sampleMessageList.size(); j++) {
        sampleMessageList.get(j).toByteArray();  
      }
    }
  }
  
  @Benchmark
  void serializeToMemoryStream(int reps) throws IOException {
    if (sampleMessageList.size() == 0) {
      return;
    }
    for (int i = 0; i < reps; i++) {
      for (int j = 0; j < sampleMessageList.size(); j++) {
        ByteArrayOutputStream output = new ByteArrayOutputStream();
        sampleMessageList.get(j).writeTo(output); 
      }
    }
  }
  
  @Benchmark
  void deserializeFromByteArray(int reps) throws IOException {
    if (inputDataList.size() == 0) {
      return;
    }
    for (int i = 0; i < reps; i++) {
      for (int j = 0; j < inputDataList.size(); j++) {
        benchmarkMessageType.getDefaultInstance().getParserForType().parseFrom(
          inputDataList.get(j), extensions);
      }
    }
  }
  
  @Benchmark
  void deserializeFromMemoryStream(int reps) throws IOException {
    if (inputStreamList.size() == 0) {
      return;
    }
    for (int i = 0; i < reps; i++) {
      for (int j = 0; j < inputStreamList.size(); j++) {
        benchmarkMessageType.getDefaultInstance().getParserForType().parseFrom(
            inputStreamList.get(j), extensions);
        inputStreamList.get(j).reset();
      }
    }
  }
}


