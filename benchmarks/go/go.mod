module github.com/protocolbuffers/protobuf/benchmarks/go

go 1.14

require (
	github.com/protocolbuffers/protobuf/benchmarks/go/benchmarks v0.0.0-00010101000000-000000000000
	google.golang.org/protobuf v1.25.0
)

replace github.com/protocolbuffers/protobuf/benchmarks/go/benchmarks => ../tmp/go
