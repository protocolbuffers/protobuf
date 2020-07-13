package main

import (
	"strings"
	"testing"

	"github.com/google/go-cmp/cmp"
	"google.golang.org/protobuf/testing/protocmp"

	pb "github.com/protocolbuffers/protobuf/examples/tutorial"
)

func TestPromptForAddressReturnsAddress(t *testing.T) {
	in := `12345
Example Name
name@example.com
123-456-7890
home
222-222-2222
mobile
111-111-1111
work
777-777-7777
unknown

`
	m, err := promptForAddress(strings.NewReader(in))
	if err != nil {
		t.Fatalf("promptForAddress(%q) error: %v", in, err)
	}
	if got, want := m.GetId(), int32(12345); got != want {
		t.Errorf("promptForAddress(%q).Id = %d, want %d", in, got, want)
	}
	if got, want := m.GetName(), "Example Name"; got != want {
		t.Errorf("promptForAddress(%q).Name = %q, want %q", in, got, want)
	}
	if got, want := m.GetEmail(), "name@example.com"; got != want {
		t.Errorf("promptForAddress(%q).Email = %q, want %q", in, got, want)
	}

	gotPhones := m.GetPhones()
	wantPhones := []*pb.Person_PhoneNumber{
		{Number: "123-456-7890", Type: pb.Person_HOME},
		{Number: "222-222-2222", Type: pb.Person_MOBILE},
		{Number: "111-111-1111", Type: pb.Person_WORK},
		{Number: "777-777-7777", Type: pb.Person_MOBILE},
	}
	if diff := cmp.Diff(wantPhones, gotPhones, protocmp.Transform()); diff != "" {
		t.Errorf("promptForAddress(%q).Phones mismatch (-want +got):\n%s", in, diff)
	}
}
