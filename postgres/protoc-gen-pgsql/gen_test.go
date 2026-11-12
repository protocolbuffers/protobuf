package main

// Generator-level tests. These inspect generated SQL text and do not need a
// database.

import (
	"os"
	"regexp"
	"strings"
	"testing"

	"google.golang.org/protobuf/reflect/protoreflect"
)

func TestSanitizeIdent(t *testing.T) {
	for in, want := range map[string]string{
		"":           "x",
		"_":          "x",
		"foo":        "foo",
		"Foo_Bar9":   "Foo_Bar9",
		"a.b-c d":    "a_b_c_d",
		"9lives":     "_9lives",
		"héllo":      "h_llo", // multi-byte rune -> one underscore (regexp is rune based)
		"semi;colon": "semi_colon",
	} {
		if got := sanitizeIdent(in); got != want {
			t.Errorf("sanitizeIdent(%q) = %q, want %q", in, got, want)
		}
	}
}

func TestSchemaName(t *testing.T) {
	for in, want := range map[string]string{
		"":                 "public",
		"transit_realtime": "transit_realtime",
		"foo.bar.v1":       "foo_bar_v1",
		"2fa":              "_2fa",
	} {
		if got := schemaName(protoreflect.FullName(in)); got != want {
			t.Errorf("schemaName(%q) = %q, want %q", in, got, want)
		}
	}
}

func TestQuoting(t *testing.T) {
	if got, want := quoteIdent(`we"ird`), `"we""ird"`; got != want {
		t.Errorf("quoteIdent = %s, want %s", got, want)
	}
	if got, want := quoteLiteral(`it's`), `'it''s'`; got != want {
		t.Errorf("quoteLiteral = %s, want %s", got, want)
	}
	if got, want := jsonbPath("field"), `'{field}'`; got != want {
		t.Errorf("jsonbPath = %s, want %s", got, want)
	}
}

func TestWKTNames(t *testing.T) {
	if got := wktKind("google.protobuf.Timestamp"); got != "timestamp" {
		t.Errorf("wktKind(Timestamp) = %q", got)
	}
	if isWKT("google.protobuf.FileDescriptorProto") {
		t.Error("descriptor.proto messages must not be treated as WKTs")
	}
	if got, want := wktFuncName("google.protobuf.StringValue"), "decode_google_protobuf_stringvalue"; got != want {
		t.Errorf("wktFuncName = %q, want %q", got, want)
	}
}

const namesProto = `
syntax = "proto3";
package acme.inventory.v1;

message Item {
  message Variant {
    message Price { int64 cents = 1; }
    Price price = 1;
  }
  repeated Variant variants = 1;
  map<string, int32> stock = 2;
}
`

func TestGeneratedNames(t *testing.T) {
	c := compileProtos(t, map[string]string{"acme/inventory.proto": namesProto})
	out := runGenerator(t, c, "", "acme/inventory.proto")

	sql, ok := out["inventory.sql"]
	if !ok {
		t.Fatalf("expected output file inventory.sql (directory stripped), got %v", keys(out))
	}
	if !strings.Contains(sql, `CREATE SCHEMA IF NOT EXISTS "acme_inventory_v1";`) {
		t.Error("missing schema creation for package acme.inventory.v1")
	}
	for _, fn := range []string{
		"decode_item", "decode_item_variant", "decode_item_variant_price", "decode_item_stockentry",
		"encode_item", "encode_item_variant", "encode_item_variant_price", "encode_item_stockentry",
	} {
		if !hasFunction(sql, "acme_inventory_v1", fn) {
			t.Errorf("missing function %s", fn)
		}
	}
	// Leaves must be defined before parents (post-order), so that a reader
	// of the file sees dependencies first.
	if strings.Index(sql, `"decode_item_variant_price"(`) > strings.Index(sql, `"decode_item_variant"(`) {
		t.Error("nested message decoder emitted after its parent")
	}
}

func TestEncodeOption(t *testing.T) {
	_, withEnc := generateOne(t, "e.proto", `syntax="proto3"; package enc_opt; message M { int32 a = 1; }`, "")
	_, noEnc := generateOne(t, "e.proto", `syntax="proto3"; package enc_opt; message M { int32 a = 1; }`, "encode=false")

	if !hasFunction(withEnc, "enc_opt", "encode_m") {
		t.Error("encode_m missing with default options")
	}
	if !hasFunction(noEnc, "enc_opt", "decode_m") {
		t.Error("decode_m missing with encode=false")
	}
	if regexp.MustCompile(`FUNCTION "enc_opt"\."encode_`).MatchString(noEnc) {
		t.Error("encode=false still generated encode_* message functions")
	}
	if regexp.MustCompile(`encode_google_protobuf_`).MatchString(noEnc) {
		t.Error("encode=false still generated WKT encoders")
	}
}

func TestInvalidParameter(t *testing.T) {
	c := compileProtos(t, map[string]string{"e.proto": `syntax="proto3"; package bad_param; message M {}`})
	if _, err := tryRunGenerator(c, "encode=maybe", "e.proto"); err == nil {
		t.Error("expected error for encode=maybe")
	}
	if _, err := tryRunGenerator(c, "nosuchopt=1", "e.proto"); err == nil {
		t.Error("expected error for unknown option")
	}
}

func TestEmptyPackageUsesPublicSchema(t *testing.T) {
	_, sql := generateOne(t, "nopkg.proto", `syntax="proto3"; message Thing { int32 a = 1; }`, "")
	if !hasFunction(sql, "public", "decode_thing") {
		t.Error("message without package should be generated into the public schema")
	}
}

func TestOnlyRequestedFilesGenerated(t *testing.T) {
	c := compileProtos(t, map[string]string{
		"dep.proto":  `syntax="proto3"; package dep_pkg; message D { int32 x = 1; }`,
		"main.proto": `syntax="proto3"; package main_pkg; import "dep.proto"; message M { dep_pkg.D d = 1; }`,
	})
	out := runGenerator(t, c, "", "main.proto")
	if _, ok := out["dep.sql"]; ok || len(out) != 1 {
		t.Fatalf("expected only main.sql, got %v", keys(out))
	}
	// Cross-package references must point at the dependency's schema.
	if !strings.Contains(out["main.sql"], `"dep_pkg"."decode_d"(b)`) {
		t.Error("decoder for M.d does not call dep_pkg.decode_d")
	}
	if !strings.Contains(out["main.sql"], `"dep_pkg"."encode_d"(`) {
		t.Error("encoder for M.d does not call dep_pkg.encode_d")
	}
}

// The checked-in gtfs-realtime.sql must stay in sync with the generator.
// Regenerate with: go test -run TestGTFSRealtimeGolden -update
func TestGTFSRealtimeGolden(t *testing.T) {
	src, err := os.ReadFile("../gtfs-realtime.proto")
	if err != nil {
		t.Skipf("gtfs-realtime.proto not available: %v", err)
	}
	c := compileProtos(t, map[string]string{"gtfs-realtime.proto": string(src)})
	out := runGenerator(t, c, "", "gtfs-realtime.proto")
	got := out["gtfs-realtime.sql"]
	const golden = "../gtfs-realtime.sql"
	if *updateGolden {
		if err := os.WriteFile(golden, []byte(got), 0o644); err != nil {
			t.Fatal(err)
		}
		return
	}
	want, err := os.ReadFile(golden)
	if err != nil {
		t.Fatal(err)
	}
	if got != string(want) {
		t.Errorf("%s is out of date; rerun with -update (first difference at line %d)", golden, firstDiffLine(got, string(want)))
	}
}

// ---------------------------------------------------------------------------

func hasFunction(sql, schema, fn string) bool {
	return strings.Contains(sql, "FUNCTION "+quoteIdent(schema)+"."+quoteIdent(fn)+"(")
}

func keys(m map[string]string) []string {
	var ks []string
	for k := range m {
		ks = append(ks, k)
	}
	return ks
}

func firstDiffLine(a, b string) int {
	al, bl := strings.Split(a, "\n"), strings.Split(b, "\n")
	for i := 0; i < len(al) && i < len(bl); i++ {
		if al[i] != bl[i] {
			return i + 1
		}
	}
	return min(len(al), len(bl)) + 1
}
