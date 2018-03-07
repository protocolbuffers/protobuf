package main

import (
	"errors"
	"io/ioutil"
	"flag"
	"testing"
	"os"

	benchmarkWrapper "./tmp"
	proto "github.com/golang/protobuf/proto"
	googleMessage1Proto3 "./tmp/datasets/google_message1/proto3"
	googleMessage1Proto2 "./tmp/datasets/google_message1/proto2"
	googleMessage2 "./tmp/datasets/google_message2"
	googleMessage3 "./tmp/datasets/google_message3"
	googleMessage4 "./tmp/datasets/google_message4"

)

// Data is returned by the Load function.
type Data struct {
	// Marshalled is a slice of marshalled protocol
	// buffers. 1:1 with Unmarshalled.
	Marshalled [][]byte

	// Unmarshalled is a slice of unmarshalled protocol
	// buffers. 1:1 with Marshalled.
	Unmarshalled []proto.Message

	count int
}

var data *Data
var counter int

type GetDefaultInstanceFunction func() proto.Message
var getDefaultInstance GetDefaultInstanceFunction

// This is used to getDefaultInstance for a message type.
func generateGetDefaltInstanceFunction(dataset benchmarkWrapper.BenchmarkDataset) error {
	switch dataset.MessageName {
	case "benchmarks.proto3.GoogleMessage1":
		getDefaultInstance = func() proto.Message { return &googleMessage1Proto3.GoogleMessage1{} }
		return nil
	case "benchmarks.proto2.GoogleMessage1":
		getDefaultInstance = func() proto.Message { return &googleMessage1Proto2.GoogleMessage1{} }
		return nil
	case "benchmarks.proto2.GoogleMessage2":
		getDefaultInstance = func() proto.Message { return &googleMessage2.GoogleMessage2{} }
		return nil
	case "benchmarks.google_message3.GoogleMessage3":
		getDefaultInstance = func() proto.Message { return &googleMessage3.GoogleMessage3{} }
		return nil
	case "benchmarks.google_message4.GoogleMessage4":
		getDefaultInstance = func() proto.Message { return &googleMessage4.GoogleMessage4{} }
		return nil
	default:
		return errors.New("Unknown message type: " + dataset.MessageName)
	}
}

func TestMain(m *testing.M) {
	flag.Parse()
	data = new(Data)
	rawData, error := ioutil.ReadFile(flag.Arg(0))
	if error != nil {
		panic("Couldn't find file" + flag.Arg(0))
	}
	var dataset benchmarkWrapper.BenchmarkDataset

	if err1 := proto.Unmarshal(rawData, &dataset); err1 != nil {
		panic("The raw input data can't be parse into BenchmarkDataset message.")
	}

	generateGetDefaltInstanceFunction(dataset)

	for _, payload := range dataset.Payload {
		data.Marshalled = append(data.Marshalled, payload)
		m := getDefaultInstance()
		proto.Unmarshal(payload, m)
		data.Unmarshalled = append(data.Unmarshalled, m)
	}
	data.count = len(data.Unmarshalled)

	os.Exit(m.Run())
}

func BenchmarkUnmarshal(b *testing.B) {
	b.ReportAllocs()
	for i := 0; i < b.N; i++ {
		payload := data.Marshalled[counter % data.count]
		out := getDefaultInstance()
		if err := proto.Unmarshal(payload, out); err != nil {
			b.Fatalf("can't unmarshal message %d %v", counter % data.count, err)
		}
		counter++
	}
}

func BenchmarkMarshal(b *testing.B) {
	b.ReportAllocs()
	for i := 0; i < b.N; i++ {
		m := data.Unmarshalled[counter % data.count]
		if _, err := proto.Marshal(m); err != nil {
			b.Fatalf("can't marshal message %d %+v: %v", counter % data.count, m, err)
		}
		counter++
	}
}

func BenchmarkSize(b *testing.B) {
	b.ReportAllocs()
	for i := 0; i < b.N; i++ {
		proto.Size(data.Unmarshalled[counter % data.count])
		counter++
	}
}

func BenchmarkClone(b *testing.B) {
	b.ReportAllocs()
	for i := 0; i < b.N; i++ {
		proto.Clone(data.Unmarshalled[counter % data.count])
		counter++
	}
}

func BenchmarkMerge(b *testing.B) {
	b.ReportAllocs()
	for i := 0; i < b.N; i++ {
		out := getDefaultInstance()
		proto.Merge(out, data.Unmarshalled[counter % data.count])
		counter++
	}
}

