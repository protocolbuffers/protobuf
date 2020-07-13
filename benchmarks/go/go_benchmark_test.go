package main

import (
	"io/ioutil"
	"os"
	"testing"

	"google.golang.org/protobuf/proto"

	pb "github.com/protocolbuffers/protobuf/benchmarks/go/benchmarks"
	pb1a "github.com/protocolbuffers/protobuf/benchmarks/go/benchmarks/datasets/google_message1/proto2"
	pb1b "github.com/protocolbuffers/protobuf/benchmarks/go/benchmarks/datasets/google_message1/proto3"
	pb2 "github.com/protocolbuffers/protobuf/benchmarks/go/benchmarks/datasets/google_message2"
	pb3 "github.com/protocolbuffers/protobuf/benchmarks/go/benchmarks/datasets/google_message3"
	pb4 "github.com/protocolbuffers/protobuf/benchmarks/go/benchmarks/datasets/google_message4"
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
func generateNewMessageFunction(dataset pb.BenchmarkDataset) func() proto.Message {
	switch dataset.MessageName {
	case "benchmarks.proto3.GoogleMessage1":
		return func() proto.Message { return new(pb1b.GoogleMessage1) }
	case "benchmarks.proto2.GoogleMessage1":
		return func() proto.Message { return new(pb1a.GoogleMessage1) }
	case "benchmarks.proto2.GoogleMessage2":
		return func() proto.Message { return new(pb2.GoogleMessage2) }
	case "benchmarks.google_message3.GoogleMessage3":
		return func() proto.Message { return new(pb3.GoogleMessage3) }
	case "benchmarks.google_message4.GoogleMessage4":
		return func() proto.Message { return new(pb4.GoogleMessage4) }
	default:
		panic("Unknown message type: " + dataset.MessageName)
	}
}

func init() {
	var foundDashDash bool
	for _, arg := range os.Args {
		switch {
		case arg == "--":
			foundDashDash = true
			continue
		case !foundDashDash:
			continue
		}

		// Load the benchmark.
		b, err := ioutil.ReadFile(arg)
		if err != nil {
			panic(err)
		}

		// Parse the benchmark.
		var dm pb.BenchmarkDataset
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
		ds.name = arg

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
