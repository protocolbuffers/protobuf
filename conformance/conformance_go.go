// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
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

package main

import (
	"bufio"
	"encoding/binary"
	"errors"
	"fmt"
	"github.com/golang/protobuf/jsonpb"
	"github.com/golang/protobuf/proto"
	"io"
	"log"
	"os"

	pb "github.com/google/protobuf/conformance/conformance"
)

// doTest returns a ConformanceResponse with the results of a test.
// Tests parse a Protobuf or JSON payload and then output a serialized Protobuf
// or JSON payload.
func doTest(req *pb.ConformanceRequest) (*pb.ConformanceResponse, error) {
	resp := &pb.ConformanceResponse{}
	test := &pb.TestAllTypes{}

	switch p := req.Payload.(type) {
	case *pb.ConformanceRequest_ProtobufPayload:
		if err := proto.Unmarshal(p.ProtobufPayload, test); err != nil {
			resp.Result = &pb.ConformanceResponse_ParseError{ParseError: err.Error()}
			return resp, nil
		}
	case *pb.ConformanceRequest_JsonPayload:
		if err := jsonpb.UnmarshalString(p.JsonPayload, test); err != nil {
			resp.Result = &pb.ConformanceResponse_ParseError{ParseError: err.Error()}
			return resp, nil
		}
	case nil:
		return resp, errors.New("request didn't have payload")
	default:
		return resp, errors.New("request had unknown payload type")
	}

	switch req.RequestedOutputFormat {
	case pb.WireFormat_UNSPECIFIED:
		return resp, errors.New("unspecified output format")
	case pb.WireFormat_PROTOBUF:
		b, err := proto.Marshal(test)
		if err != nil {
			return resp, err
		}
		resp.Result = &pb.ConformanceResponse_ProtobufPayload{ProtobufPayload: b}
	case pb.WireFormat_JSON:
		m := &jsonpb.Marshaler{}
		s, err := m.MarshalToString(test)
		if err != nil {
			return resp, err
		}
		resp.Result = &pb.ConformanceResponse_JsonPayload{JsonPayload: s}
	default:
		return resp, errors.New("request had unknown output format")
	}

	return resp, nil
}

// doTestIO reads a length-prefixed protobuf from Stdio and writes test results to Stdout.
// If EOF is encountered in an unexpected place, returns an error.
func doTestIO() error {
	r := bufio.NewReader(os.Stdin)

	// Get the length of the next message, which is encoded in the first 4 bytes
	// as an unsigned integer. If there aren't 4 bytes here, the test is
	// finished.
	b, err := r.Peek(4)
	if err == io.EOF {
		if len(b) == 0 {
			return io.EOF
		}
		return errors.New("unexpected EOF")
	}
	if err != nil {
		return fmt.Errorf("failed to read length: %v", err)
	}

	var n uint32
	if err := binary.Read(r, binary.LittleEndian, &n); err != nil {
		return fmt.Errorf("failed to decode length: %v", err)
	}

	// Read the next message, now that the size is in the variable n.
	b = make([]byte, n, n)
	nb, err := r.Read(b)
	if nb != int(n) {
		return fmt.Errorf("got %v bytes; expected %v", nb, n)
	}
	req := &pb.ConformanceRequest{}
	if err := proto.Unmarshal(b, req); err != nil {
		return fmt.Errorf("failed to unmarshal conformance req: %v", err)
	}

	// Run the test.
	resp, err := doTest(req)
	if err != nil {
		return err
	}

	// Write the test response to stdout, prepended with the length of the
	// response.
	b, err = proto.Marshal(resp)
	if err != nil {
		return fmt.Errorf("failed to marshal conformance resp: %v", err)
	}

	n = uint32(len(b))
	if err := binary.Write(os.Stdout, binary.LittleEndian, &n); err != nil {
		return fmt.Errorf("failed to write conformance resp length: %v", err)
	}
	if _, err := os.Stdout.Write(b); err != nil {
		return fmt.Errorf("failed to write conformance resp: %v", err)
	}

	return nil
}

func main() {
	for n := 1; ; n++ {
		err := doTestIO()
		if err == io.EOF {
			fmt.Fprintf(os.Stderr, "conformance-go: received EOF from test runner after %v tests, exiting\n", n)
			return
		}
		if err != nil {
			log.Fatalf("Error after %v tests: %v", n, err)
		}
	}
}
