
package com.google.protobuf;

import com.google.caliper.BeforeExperiment;
import com.google.caliper.Benchmark;
import com.google.caliper.Param;
import com.google.protobuf.ByteString;
import com.google.protobuf.CodedOutputStream;
import com.google.protobuf.ExtensionRegistry;
import com.google.protobuf.Message;
import com.google.protobuf.benchmarks.Benchmarks.BenchmarkDataset;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

public class ProtoBenchCaliper {
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
        benchmarks.google_message3.BenchmarkMessage38.registerAllExtensions(extensions);
        benchmarks.google_message3.BenchmarkMessage37.registerAllExtensions(extensions);
        benchmarks.google_message3.BenchmarkMessage36.registerAllExtensions(extensions);
        benchmarks.google_message3.BenchmarkMessage35.registerAllExtensions(extensions);
        benchmarks.google_message3.BenchmarkMessage34.registerAllExtensions(extensions);
        benchmarks.google_message3.BenchmarkMessage33.registerAllExtensions(extensions);
        benchmarks.google_message3.BenchmarkMessage32.registerAllExtensions(extensions);
        benchmarks.google_message3.BenchmarkMessage31.registerAllExtensions(extensions);
        benchmarks.google_message3.BenchmarkMessage3.registerAllExtensions(extensions);
        return extensions;
      }
      @Override
      Message getDefaultInstance() {
        return benchmarks.google_message3.BenchmarkMessage3.GoogleMessage3.getDefaultInstance();
      }
    },
    GOOGLE_MESSAGE4 {
      @Override
      ExtensionRegistry getExtensionRegistry() {
        ExtensionRegistry extensions = ExtensionRegistry.newInstance();
        benchmarks.google_message4.BenchmarkMessage43.registerAllExtensions(extensions);
        benchmarks.google_message4.BenchmarkMessage42.registerAllExtensions(extensions);
        benchmarks.google_message4.BenchmarkMessage41.registerAllExtensions(extensions);
        benchmarks.google_message4.BenchmarkMessage4.registerAllExtensions(extensions);
        return extensions;
      }
      @Override
      Message getDefaultInstance() {
        return benchmarks.google_message4.BenchmarkMessage4.GoogleMessage4.getDefaultInstance();
      }
    };
    
    abstract ExtensionRegistry getExtensionRegistry();
    abstract Message getDefaultInstance();
  }
  
  @Param 
  private BenchmarkMessageType benchmarkMessageType;
  @Param
  private String dataFile;
  
  private byte[] inputData;
  private BenchmarkDataset benchmarkDataset;
  private Message defaultMessage;
  private ExtensionRegistry extensions;
  private List<byte[]> inputDataList;
  private List<ByteArrayInputStream> inputStreamList;
  private List<ByteString> inputStringList;
  private List<Message> sampleMessageList;
  private int counter;
  
  @BeforeExperiment
  void setUp() throws IOException {
    inputData = ProtoBench.readAllBytes(dataFile);
    benchmarkDataset = BenchmarkDataset.parseFrom(inputData);
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
    
    counter = 0;
  }
  
  
  @Benchmark
  void serializeToByteString(int reps) throws IOException {
    for (int i = 0; i < reps; i++) {
      sampleMessageList.get(counter % sampleMessageList.size()).toByteString();
      counter++;
    }
  }
  
  @Benchmark
  void serializeToByteArray(int reps) throws IOException {
    for (int i = 0; i < reps; i++) {
      sampleMessageList.get(counter % sampleMessageList.size()).toByteArray();
      counter++;
    }
  }
  
  @Benchmark
  void serializeToMemoryStream(int reps) throws IOException {
    for (int i = 0; i < reps; i++) {
      ByteArrayOutputStream output = new ByteArrayOutputStream();
      sampleMessageList.get(counter % sampleMessageList.size()).writeTo(output);
      counter++;
    }
  }
  
  @Benchmark
  void deserializeFromByteString(int reps) throws IOException {
    for (int i = 0; i < reps; i++) {
      defaultMessage
        .newBuilderForType()
        .mergeFrom(inputStringList.get(counter % inputStringList.size()), extensions)
        .build();
      counter++;
    }
  }
  
  @Benchmark
  void deserializeFromByteArray(int reps) throws IOException {
    for (int i = 0; i < reps; i++) {
      defaultMessage
        .newBuilderForType()
        .mergeFrom(inputDataList.get(counter % inputDataList.size()), extensions)
        .build();
      counter++;
    }
  }
  
  @Benchmark
  void deserializeFromMemoryStream(int reps) throws IOException {
    for (int i = 0; i < reps; i++) {
      defaultMessage
        .newBuilderForType()
        .mergeFrom(inputStreamList.get(counter % inputStreamList.size()), extensions)
        .build();
      inputStreamList.get(counter % inputStreamList.size()).reset();
      counter++;
    }
  }
}
