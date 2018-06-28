package main

import (
	benchmarkWrapper "../tmp"
	googleMessage1Proto2 "../tmp/datasets/google_message1/proto2"
	googleMessage1Proto3 "../tmp/datasets/google_message1/proto3"
	googleMessage2 "../tmp/datasets/google_message2"
	googleMessage3 "../tmp/datasets/google_message3"
	googleMessage4 "../tmp/datasets/google_message4"
	"flag"
	"github.com/golang/protobuf/proto"
	"io/ioutil"
	"testing"
)

// Data is returned by the Load function.
type Dataset struct {
	name        string
	newMessage  func() proto.Message
	marshaled   [][]byte
	unmarshaled []proto.Message
}

var datasets []Dataset

// This is used to getDefaultInstance for a message type.
func generateNewMessageFunction(dataset benchmarkWrapper.BenchmarkDataset) func() proto.Message {
	switch dataset.MessageName {
	case "benchmarks.proto3.GoogleMessage1":
		return func() proto.Message { return new(googleMessage1Proto3.GoogleMessage1) }
	case "benchmarks.proto2.GoogleMessage1":
		return func() proto.Message { return new(googleMessage1Proto2.GoogleMessage1) }
	case "benchmarks.proto2.GoogleMessage2":
		return func() proto.Message { return new(googleMessage2.GoogleMessage2) }
	case "benchmarks.google_message3.GoogleMessage3":
		return func() proto.Message { return new(googleMessage3.GoogleMessage3) }
	case "benchmarks.google_message4.GoogleMessage4":
		return func() proto.Message { return new(googleMessage4.GoogleMessage4) }
	default:
		panic("Unknown message type: " + dataset.MessageName)
	}
}

func init() {
	flag.Parse()
	for _, f := range flag.Args() {
		// Load the benchmark.
		b, err := ioutil.ReadFile(f)
		if err != nil {
			panic(err)
		}

		// Parse the benchmark.
		var dm benchmarkWrapper.BenchmarkDataset
		if err := proto.Unmarshal(b, &dm); err != nil {
			panic(err)
		}

		// Determine the concrete protobuf message type to use.
		var ds Dataset
		ds.newMessage = generateNewMessageFunction(dm)

		// Unmarshal each test message.
		for _, payload := range dm.Payload {
			ds.marshaled = append(ds.marshaled, payload)
			m := ds.newMessage()
			if err := proto.Unmarshal(payload, m); err != nil {
				panic(err)
			}
			ds.unmarshaled = append(ds.unmarshaled, m)
		}
		ds.name = f

		datasets = append(datasets, ds)
	}
}

func Benchmark(b *testing.B) {
	for _, ds := range datasets {
		b.Run(ds.name, func(b *testing.B) {
			b.Run("Unmarshal", func(b *testing.B) {
				for i := 0; i < b.N; i++ {
					for j, payload := range ds.marshaled {
						out := ds.newMessage()
						if err := proto.Unmarshal(payload, out); err != nil {
							b.Fatalf("can't unmarshal message %d %v", j, err)
						}
					}
				}
			})
			b.Run("Marshal", func(b *testing.B) {
				for i := 0; i < b.N; i++ {
					for j, m := range ds.unmarshaled {
						if _, err := proto.Marshal(m); err != nil {
							b.Fatalf("can't marshal message %d %+v: %v", j, m, err)
						}
					}
				}
			})
			b.Run("Size", func(b *testing.B) {
				for i := 0; i < b.N; i++ {
					for _, m := range ds.unmarshaled {
						proto.Size(m)
					}
				}
			})
			b.Run("Clone", func(b *testing.B) {
				for i := 0; i < b.N; i++ {
					for _, m := range ds.unmarshaled {
						proto.Clone(m)
					}
				}
			})
			b.Run("Merge", func(b *testing.B) {
				for i := 0; i < b.N; i++ {
					for _, m := range ds.unmarshaled {
						out := ds.newMessage()
						proto.Merge(out, m)
					}
				}
			})
		})
	}
}
