package main

// End-to-end tests: generated SQL is loaded into PostgreSQL and checked
// against the Go protobuf runtime, which serves as the reference
// implementation of the wire format.
//
//   decode: Go proto.Marshal -> SQL decode_* -> compare with expected jsonb
//   encode: SQL encode_*(expected jsonb) -> Go proto.Unmarshal -> proto.Equal
//   round trip: SQL decode_*(encode_*(jsonb)) == jsonb

import (
	"os"
	"strings"
	"testing"

	"google.golang.org/protobuf/encoding/protowire"
	"google.golang.org/protobuf/proto"
	"google.golang.org/protobuf/testing/protocmp"

	"github.com/google/go-cmp/cmp"
)

const typesProto = `
syntax = "proto3";
package pgt_types;

import "google/protobuf/any.proto";
import "google/protobuf/duration.proto";
import "google/protobuf/empty.proto";
import "google/protobuf/field_mask.proto";
import "google/protobuf/struct.proto";
import "google/protobuf/timestamp.proto";
import "google/protobuf/wrappers.proto";

enum Color {
  COLOR_UNSPECIFIED = 0;
  RED = 1;
  GREEN = 2;
}

message Scalars {
  int32 i32 = 1;
  int64 i64 = 2;
  uint32 u32 = 3;
  uint64 u64 = 4;
  sint32 s32 = 5;
  sint64 s64 = 6;
  fixed32 f32 = 7;
  fixed64 f64 = 8;
  sfixed32 sf32 = 9;
  sfixed64 sf64 = 10;
  float fl = 11;
  double db = 12;
  bool b = 13;
  string str = 14;
  bytes by = 15;
  Color color = 16;
}

message Repeated {
  repeated int32 i32 = 1;
  repeated sint64 s64 = 2;
  repeated fixed32 f32 = 3;
  repeated double db = 4;
  repeated bool b = 5;
  repeated string str = 6;
  repeated bytes by = 7;
  repeated Color color = 8;
  repeated int32 unpacked = 9 [packed = false];
}

message Inner {
  string name = 1;
  int32 n = 2;
}

message Nested {
  Inner one = 1;
  repeated Inner many = 2;
  map<string, int32> counts = 3;
  map<int64, string> labels = 4;
  map<string, Inner> inners = 5;
  map<string, Color> colors = 6;
  oneof choice {
    string text = 7;
    Inner obj = 8;
    int32 num = 9;
  }
}

message WKT {
  google.protobuf.Timestamp ts = 1;
  google.protobuf.Duration dur = 2;
  google.protobuf.Int32Value i32w = 3;
  google.protobuf.Int64Value i64w = 4;
  google.protobuf.StringValue strw = 5;
  google.protobuf.BoolValue boolw = 6;
  google.protobuf.DoubleValue dblw = 7;
  google.protobuf.BytesValue bytesw = 8;
  google.protobuf.Struct st = 9;
  google.protobuf.Value val = 10;
  google.protobuf.ListValue list = 11;
  google.protobuf.Empty empty = 12;
  google.protobuf.FieldMask mask = 13;
  google.protobuf.Any any = 14;
  repeated google.protobuf.Timestamp tss = 15;
}
`

type wireCase struct {
	name      string
	msg       string // message name within the package
	protoJSON string // Go-side value, protojson syntax
	want      string // expected jsonb produced by decode_* / accepted by encode_*
	skip      string // non-empty: known failure, reason for skipping
}

var typesCases = []wireCase{
	{name: "empty message", msg: "Scalars", protoJSON: `{}`, want: `{}`},
	{
		name:      "positive scalars",
		msg:       "Scalars",
		protoJSON: `{"i32":150,"i64":"1234567890123","u32":4294967295,"u64":"1099511627776","s32":-75,"s64":"-9000000000","db":-2.25,"b":true,"str":"héllo ☃","by":"AAEC/w==","color":"GREEN"}`,
		want:      `{"i32":150,"i64":1234567890123,"u32":4294967295,"u64":1099511627776,"s32":-75,"s64":-9000000000,"db":-2.25,"b":true,"str":"héllo ☃","by":"AAEC/w==","color":"GREEN"}`,
	},
	{name: "uint64 max", msg: "Scalars", protoJSON: `{"u64":"18446744073709551615"}`, want: `{"u64":18446744073709551615}`},
	{name: "negative sfixed64", msg: "Scalars", protoJSON: `{"sf64":"-3"}`, want: `{"sf64":-3}`},
	{name: "sint extremes", msg: "Scalars", protoJSON: `{"s32":-2147483648,"s64":"9223372036854775807"}`, want: `{"s32":-2147483648,"s64":9223372036854775807}`},
	{name: "fixed32 small", msg: "Scalars", protoJSON: `{"f32":123456,"fl":1.5}`, want: `{"f32":123456,"fl":1.5}`},
	{name: "fixed64 max", msg: "Scalars", protoJSON: `{"f64":"18446744073709551615"}`, want: `{"f64":18446744073709551615}`},
	{name: "double full precision", msg: "Scalars", protoJSON: `{"db":0.30000000000000004}`, want: `{"db":0.30000000000000004}`},
	{name: "negative int32", msg: "Scalars", protoJSON: `{"i32":-1}`, want: `{"i32":-1}`},
	{name: "negative int64", msg: "Scalars", protoJSON: `{"i64":"-5"}`, want: `{"i64":-5}`},
	{name: "fixed32 high bit", msg: "Scalars", protoJSON: `{"f32":4294967295}`, want: `{"f32":4294967295}`},
	{name: "negative sfixed32", msg: "Scalars", protoJSON: `{"sf32":-2}`, want: `{"sf32":-2}`},
	{name: "negative float", msg: "Scalars", protoJSON: `{"fl":-1.5}`, want: `{"fl":-1.5}`},
	{
		name:      "long bytes",
		msg:       "Scalars",
		protoJSON: `{"by":"` + strings.Repeat("AAAA", 30) + `"}`,
		want:      `{"by":"` + strings.Repeat("AAAA", 30) + `"}`,
	},
	{
		name:      "repeated packed and unpacked",
		msg:       "Repeated",
		protoJSON: `{"i32":[1,150,3],"s64":["-1","2"],"f32":[7,8],"db":[0.5,-1],"b":[true,false,true],"str":["a","","c"],"by":["AQ==",""],"color":["RED","COLOR_UNSPECIFIED","GREEN"],"unpacked":[5,6]}`,
		want:      `{"i32":[1,150,3],"s64":[-1,2],"f32":[7,8],"db":[0.5,-1],"b":[true,false,true],"str":["a","","c"],"by":["AQ==",""],"color":["RED","COLOR_UNSPECIFIED","GREEN"],"unpacked":[5,6]}`,
	},
	{
		name:      "nested messages and maps",
		msg:       "Nested",
		protoJSON: `{"one":{"name":"x","n":1},"many":[{"name":"a"},{},{"n":3}],"counts":{"a":1,"b":2},"labels":{"10":"ten","0":"zero"},"inners":{"k":{"name":"v"}},"colors":{"c":"RED"}}`,
		want:      `{"one":{"name":"x","n":1},"many":[{"name":"a"},{},{"n":3}],"counts":{"a":1,"b":2},"labels":{"10":"ten","0":"zero"},"inners":{"k":{"name":"v"}},"colors":{"c":"RED"}}`,
	},
	{name: "negative map key", msg: "Nested", protoJSON: `{"labels":{"-1":"neg"}}`, want: `{"labels":{"-1":"neg"}}`},
	{name: "oneof string", msg: "Nested", protoJSON: `{"text":"hi"}`, want: `{"text":"hi"}`},
	{name: "oneof message", msg: "Nested", protoJSON: `{"obj":{"n":4}}`, want: `{"obj":{"n":4}}`},
	{name: "oneof zero scalar", msg: "Nested", protoJSON: `{"num":0}`, want: `{"num":0}`},
	{name: "empty submessage", msg: "Nested", protoJSON: `{"one":{}}`, want: `{"one":{}}`},
	{
		name:      "timestamp duration",
		msg:       "WKT",
		protoJSON: `{"ts":"2023-11-14T22:13:20.000000005Z","dur":"90.5s","tss":["1970-01-01T00:00:01Z","1970-01-01T00:00:02Z"]}`,
		want:      `{"ts":{"seconds":1700000000,"nanos":5},"dur":{"seconds":90,"nanos":500000000},"tss":[{"seconds":1},{"seconds":2}]}`,
	},
	{
		name:      "wrappers",
		msg:       "WKT",
		protoJSON: `{"i32w":7,"i64w":"-8","strw":"s","boolw":true,"dblw":2.5,"bytesw":"AQI="}`,
		want:      `{"i32w":7,"i64w":-8,"strw":"s","boolw":true,"dblw":2.5,"bytesw":"AQI="}`,
	},
	{
		name:      "struct value list",
		msg:       "WKT",
		protoJSON: `{"st":{"a":1,"b":"two","c":true,"d":null,"e":{"f":[1,"x"]}},"val":"hello","list":[1,false,"z",{"k":"v"}]}`,
		want:      `{"st":{"a":1,"b":"two","c":true,"d":null,"e":{"f":[1,"x"]}},"val":"hello","list":[1,false,"z",{"k":"v"}]}`,
	},
	{name: "value number", msg: "WKT", protoJSON: `{"val":1.25}`, want: `{"val":1.25}`},
	{name: "value struct", msg: "WKT", protoJSON: `{"val":{"x":[true]}}`, want: `{"val":{"x":[true]}}`},
	{
		name:      "struct string colliding with Value field name",
		msg:       "WKT",
		protoJSON: `{"st":{"kind":"string_value"}}`,
		want:      `{"st":{"kind":"string_value"}}`,
	},
	{
		name:      "empty fieldmask any",
		msg:       "WKT",
		protoJSON: `{"empty":{},"mask":"a.b,c","any":{"@type":"type.googleapis.com/pgt_types.Inner","name":"n"}}`,
		want:      `{"empty":{},"mask":["a.b","c"],"any":{"type_url":"type.googleapis.com/pgt_types.Inner","value":"CgFu"}}`,
	},
}

func TestTypesRoundTrip(t *testing.T) {
	db := requireDB(t)
	c, sql := generateOne(t, "pgt_types.proto", typesProto, "")
	db.load(t, sql, "pgt_types")
	runWireCases(t, db, c, "pgt_types", typesCases)
}

func runWireCases(t *testing.T, db *testDB, c *compiled, pkg string, cases []wireCase) {
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			if tc.skip != "" {
				t.Skip(tc.skip)
			}
			full := pkg + "." + tc.msg
			fn := strings.ToLower(tc.msg)
			want := c.newMessage(t, full, tc.protoJSON)

			t.Run("decode", func(t *testing.T) {
				got := db.decode(t, pkg, "decode_"+fn, marshal(t, want))
				assertJSONEqual(t, got, tc.want)
			})
			t.Run("encode", func(t *testing.T) {
				data := db.encode(t, pkg, "encode_"+fn, tc.want)
				got := c.emptyMessage(t, full)
				if err := (proto.UnmarshalOptions{Resolver: c.types}).Unmarshal(data, got); err != nil {
					t.Fatalf("Go cannot parse encode_%s output %x: %v", fn, data, err)
				}
				if diff := cmp.Diff(want, got, protocmp.Transform()); diff != "" {
					t.Errorf("encode_%s(%s) mismatch (-want +got):\n%s", fn, tc.want, diff)
				}
			})
			t.Run("sql round trip", func(t *testing.T) {
				got := db.query(t, "SELECT "+quoteIdent(pkg)+"."+quoteIdent("decode_"+fn)+"("+
					quoteIdent(pkg)+"."+quoteIdent("encode_"+fn)+"("+dollarQuote(tc.want)+"::jsonb))::text")
				assertJSONEqual(t, got, tc.want)
			})
		})
	}
}

func TestEnumEncoding(t *testing.T) {
	db := requireDB(t)
	c, sql := generateOne(t, "pgt_enum.proto", strings.Replace(typesProto, "package pgt_types;", "package pgt_enum;", 1), "")
	db.load(t, sql, "pgt_enum")

	// encode accepts enum numbers as well as names
	data := db.encode(t, "pgt_enum", "encode_scalars", `{"color":2}`)
	got := c.emptyMessage(t, "pgt_enum.Scalars")
	if err := proto.Unmarshal(data, got); err != nil {
		t.Fatal(err)
	}
	if diff := cmp.Diff(c.newMessage(t, "pgt_enum.Scalars", `{"color":"GREEN"}`), got, protocmp.Transform()); diff != "" {
		t.Errorf("numeric enum encode mismatch:\n%s", diff)
	}

	// unknown enum numbers decode to their number as a string
	raw := protowire.AppendTag(nil, 16, protowire.VarintType)
	raw = protowire.AppendVarint(raw, 42)
	assertJSONEqual(t, db.decode(t, "pgt_enum", "decode_scalars", raw), `{"color":"42"}`)

	// unknown enum names are dropped on encode
	if data := db.encode(t, "pgt_enum", "encode_scalars", `{"color":"PURPLE"}`); len(data) != 0 {
		t.Errorf("unknown enum name encoded as %x, want empty", data)
	}
}

func TestDecodeEdgeCases(t *testing.T) {
	db := requireDB(t)
	_, sql := generateOne(t, "pgt_edge.proto", strings.Replace(typesProto, "package pgt_types;", "package pgt_edge;", 1), "")
	db.load(t, sql, "pgt_edge")

	t.Run("NULL input", func(t *testing.T) {
		if got := db.query(t, `SELECT pgt_edge.decode_scalars(NULL) IS NULL`); got != "t" {
			t.Errorf("decode(NULL) IS NULL = %s", got)
		}
	})
	t.Run("empty input", func(t *testing.T) {
		assertJSONEqual(t, db.decode(t, "pgt_edge", "decode_scalars", nil), `{}`)
	})
	t.Run("unknown fields skipped", func(t *testing.T) {
		var b []byte
		b = protowire.AppendTag(b, 1, protowire.VarintType) // i32 = 9
		b = protowire.AppendVarint(b, 9)
		b = protowire.AppendTag(b, 100, protowire.VarintType)
		b = protowire.AppendVarint(b, 1<<40)
		b = protowire.AppendTag(b, 101, protowire.Fixed64Type)
		b = protowire.AppendFixed64(b, 1)
		b = protowire.AppendTag(b, 102, protowire.BytesType)
		b = protowire.AppendString(b, "ignored")
		b = protowire.AppendTag(b, 103, protowire.Fixed32Type)
		b = protowire.AppendFixed32(b, 1)
		b = protowire.AppendTag(b, 104, protowire.StartGroupType)
		b = protowire.AppendTag(b, 1, protowire.VarintType)
		b = protowire.AppendVarint(b, 5)
		b = protowire.AppendTag(b, 104, protowire.EndGroupType)
		b = protowire.AppendTag(b, 14, protowire.BytesType) // str = "end"
		b = protowire.AppendString(b, "end")
		assertJSONEqual(t, db.decode(t, "pgt_edge", "decode_scalars", b), `{"i32":9,"str":"end"}`)
	})
	t.Run("wrong wire type skipped", func(t *testing.T) {
		var b []byte
		b = protowire.AppendTag(b, 1, protowire.BytesType) // i32 sent as bytes
		b = protowire.AppendString(b, "zz")
		b = protowire.AppendTag(b, 13, protowire.VarintType)
		b = protowire.AppendVarint(b, 1)
		assertJSONEqual(t, db.decode(t, "pgt_edge", "decode_scalars", b), `{"b":true}`)
	})
	t.Run("last value wins for singular fields", func(t *testing.T) {
		var b []byte
		for _, v := range []uint64{1, 2, 3} {
			b = protowire.AppendTag(b, 1, protowire.VarintType)
			b = protowire.AppendVarint(b, v)
		}
		assertJSONEqual(t, db.decode(t, "pgt_edge", "decode_scalars", b), `{"i32":3}`)
	})
	t.Run("packed and unpacked mixed", func(t *testing.T) {
		var b []byte
		b = protowire.AppendTag(b, 1, protowire.VarintType)
		b = protowire.AppendVarint(b, 1)
		b = protowire.AppendTag(b, 1, protowire.BytesType)
		b = protowire.AppendBytes(b, protowire.AppendVarint(protowire.AppendVarint(nil, 2), 300))
		b = protowire.AppendTag(b, 1, protowire.VarintType)
		b = protowire.AppendVarint(b, 4)
		assertJSONEqual(t, db.decode(t, "pgt_edge", "decode_repeated", b), `{"i32":[1,2,300,4]}`)
	})
	t.Run("truncated varint raises", func(t *testing.T) {
		if _, err := db.tryDecode("pgt_edge", "decode_scalars", []byte{0x08, 0x96}); err == nil ||
			!strings.Contains(err.Error(), "unexpected end of varint") {
			t.Errorf("want 'unexpected end of varint' error, got %v", err)
		}
	})
	t.Run("truncated fixed64 raises", func(t *testing.T) {
		if _, err := db.tryDecode("pgt_edge", "decode_scalars", []byte{0x41, 1, 2, 3}); err == nil ||
			!strings.Contains(err.Error(), "unexpected end of fixed64") {
			t.Errorf("want 'unexpected end of fixed64' error, got %v", err)
		}
	})
	t.Run("encode non-object returns empty", func(t *testing.T) {
		for _, js := range []string{`[]`, `1`, `"x"`, `null`} {
			if data := db.encode(t, "pgt_edge", "encode_scalars", js); len(data) != 0 {
				t.Errorf("encode_scalars(%s) = %x, want empty", js, data)
			}
		}
	})
	t.Run("encode ignores json null fields", func(t *testing.T) {
		if data := db.encode(t, "pgt_edge", "encode_nested", `{"one":null,"text":null}`); len(data) != 0 {
			t.Errorf("got %x, want empty", data)
		}
	})
}

const proto2Proto = `
syntax = "proto2";
package pgt_p2;

message Base {
  optional int32 a = 1;
  optional string s = 2 [default = "dflt"];
  optional group G = 3 {
    optional int32 x = 4;
  }
  optional int32 after = 5;
  extensions 100 to 199;
}

extend Base {
  optional string ext_s = 100;
  repeated int32 ext_r = 101;
}
`

func TestProto2(t *testing.T) {
	db := requireDB(t)
	c, sql := generateOne(t, "pgt_p2.proto", proto2Proto, "")
	db.load(t, sql, "pgt_p2")

	runWireCases(t, db, c, "pgt_p2", []wireCase{
		{name: "explicit zero is present", msg: "Base", protoJSON: `{"a":0,"s":""}`, want: `{"a":0,"s":""}`},
		{name: "unset fields absent (no defaults)", msg: "Base", protoJSON: `{}`, want: `{}`},
	})

	t.Run("group skipped on decode", func(t *testing.T) {
		m := c.newMessage(t, "pgt_p2.Base", `{"a":1,"g":{"x":2},"after":3}`)
		assertJSONEqual(t, db.decode(t, "pgt_p2", "decode_base", marshal(t, m)), `{"a":1,"after":3}`)
	})
	t.Run("extensions decoded", func(t *testing.T) {
		m := c.newMessage(t, "pgt_p2.Base", `{"a":1,"[pgt_p2.ext_s]":"e","[pgt_p2.ext_r]":[7,8]}`)
		assertJSONEqual(t, db.decode(t, "pgt_p2", "decode_base", marshal(t, m)), `{"a":1,"ext_s":"e","ext_r":[7,8]}`)
	})
	t.Run("extensions encoded", func(t *testing.T) {
		data := db.encode(t, "pgt_p2", "encode_base", `{"a":1,"ext_s":"e","ext_r":[7,8]}`)
		got := c.emptyMessage(t, "pgt_p2.Base")
		if err := (proto.UnmarshalOptions{Resolver: c.types}).Unmarshal(data, got); err != nil {
			t.Fatal(err)
		}
		want := c.newMessage(t, "pgt_p2.Base", `{"a":1,"[pgt_p2.ext_s]":"e","[pgt_p2.ext_r]":[7,8]}`)
		if diff := cmp.Diff(want, got, protocmp.Transform()); diff != "" {
			t.Errorf("mismatch (-want +got):\n%s", diff)
		}
	})
}

func TestCrossPackageReference(t *testing.T) {
	db := requireDB(t)
	c := compileProtos(t, map[string]string{
		"dep.proto":  `syntax="proto3"; package pgt_dep; message D { int32 x = 1; }`,
		"main.proto": `syntax="proto3"; package pgt_main; import "dep.proto"; message M { pgt_dep.D d = 1; repeated pgt_dep.D ds = 2; }`,
	})
	out := runGenerator(t, c, "", "dep.proto", "main.proto")
	db.load(t, out["dep.sql"], "pgt_dep")
	db.load(t, out["main.sql"], "pgt_main")
	runWireCases(t, db, c, "pgt_main", []wireCase{
		{name: "cross package", msg: "M", protoJSON: `{"d":{"x":1},"ds":[{"x":2},{}]}`, want: `{"d":{"x":1},"ds":[{"x":2},{}]}`},
	})
}

// Decodes a real GTFS-realtime TripUpdates feed and checks that re-encoding
// it in SQL yields a message equal to the original.
func TestGTFSRealtimeFeed(t *testing.T) {
	db := requireDB(t)
	src, err := os.ReadFile("../gtfs-realtime.proto")
	if err != nil {
		t.Skipf("gtfs-realtime.proto not available: %v", err)
	}
	c := compileProtos(t, map[string]string{"gtfs-realtime.proto": string(src)})
	db.load(t, runGenerator(t, c, "", "gtfs-realtime.proto")["gtfs-realtime.sql"], "transit_realtime")

	// Build a feed in Go rather than depending on an untracked sample file.
	feed := c.newMessage(t, "transit_realtime.FeedMessage", `{
	  "header": {"gtfs_realtime_version": "2.0", "incrementality": "FULL_DATASET", "timestamp": "1762838000"},
	  "entity": [{
	    "id": "48886-1196",
	    "trip_update": {
	      "trip": {"trip_id": "48886-1196", "start_time": "16:28:00", "start_date": "20261111", "schedule_relationship": "SCHEDULED", "route_id": "IWLR-191"},
	      "vehicle": {"id": "v1", "label": "Bus 1"},
	      "stop_time_update": [
	        {"stop_sequence": 1, "stop_id": "S1", "arrival": {"delay": -30, "time": "1762838100"}, "departure": {"delay": 0, "time": "1762838160"}},
	        {"stop_sequence": 2, "stop_id": "S2", "schedule_relationship": "SKIPPED"}
	      ],
	      "timestamp": "1762838001",
	      "delay": 15
	    }
	  }, {
	    "id": "veh-1",
	    "vehicle": {"position": {"latitude": -33.865143, "longitude": 151.2099, "bearing": 90, "speed": 12.5}, "current_status": "IN_TRANSIT_TO"}
	  }]
	}`)

	decoded := db.decode(t, "transit_realtime", "decode_feedmessage", marshal(t, feed))
	if got := db.query(t, "SELECT "+dollarQuote(decoded)+"::jsonb #>> '{entity,0,trip_update,stop_time_update,0,arrival,delay}'"); got != "-30" {
		t.Errorf("arrival delay = %q, want -30", got)
	}

	data := db.encode(t, "transit_realtime", "encode_feedmessage", decoded)
	got := c.emptyMessage(t, "transit_realtime.FeedMessage")
	if err := proto.Unmarshal(data, got); err != nil {
		t.Fatal(err)
	}
	// float32 fields go through numeric conversion; compare with float tolerance.
	if diff := cmp.Diff(feed, got, protocmp.Transform(), cmpFloatApprox()); diff != "" {
		t.Errorf("GTFS feed round trip mismatch (-want +got):\n%s", diff)
	}
}

func cmpFloatApprox() cmp.Option {
	return cmp.Comparer(func(a, b float32) bool {
		d := a - b
		if d < 0 {
			d = -d
		}
		return d <= 1e-6*max(abs32(a), abs32(b))
	})
}

func abs32(f float32) float32 {
	if f < 0 {
		return -f
	}
	return f
}
