# protoc-gen-pgsql

A `protoc` plugin that compiles `.proto` files into PostgreSQL PL/pgSQL
functions capable of decoding protobuf-encoded messages into `jsonb` values
(and encoding `jsonb` back into protobuf bytes), analogous to how `jsonb`
parses JSON.

## Build

```sh
cd postgres/protoc-gen-pgsql
go build -o protoc-gen-pgsql .
```

Requires Go 1.21+.

## Usage

```sh
protoc -I. \
  --plugin=protoc-gen-pgsql=./protoc-gen-pgsql \
  --pgsql_out=./out \
  --pgsql_opt=Mmypackage.proto=example.com/mypackage \
  mypackage.proto
```

Then load the generated SQL into PostgreSQL:

```sh
psql "$DATABASE_URL" -f ./out/mypackage.sql
```

Decode a message:

```sql
SELECT mypackage.decode_mymessage(payload) FROM my_table;
```

Encode a message (reverse direction):

```sql
SELECT mypackage.encode_mymessage('{"field": 42}'::jsonb);
```

The decoded result is `jsonb`, so all JSONB operators (`->`, `->>`, `#>`,
`@>`, ...) and GIN indexing work out of the box.

## Options

- `M<file.proto>=<importpath>` — Go import path for the proto (required by
  protoc if the file has no `go_package` option).
- `encode=true|false` — generate `encode_*` functions (default `true`).

## Supported features

- All scalar types (int32/64, uint32/64, sint32/64, fixed32/64, sfixed32/64,
  float, double, bool, string, bytes)
- Enums (decoded as their name string; encoded from name or number)
- Nested messages, repeated fields (packed and unpacked), maps, oneofs
- proto2, proto3, and editions (treated as proto3 semantics)
- Well-known types: Timestamp, Duration, Any, Struct, Value, ListValue, Empty,
  FieldMask, and the wrapper types (decoded to native JSON where sensible)
- Extensions (top-level `extend` declarations)

## Notes

- Unknown fields are skipped on decode (and not re-emitted on encode).
- Groups are not supported.
- Composite-type (typed) output is not generated; results are `jsonb`.
## Testing

```sh
go test ./...          # generator + PostgreSQL round-trip tests
go test -short ./...   # generator tests only (no database)
go test -run TestGTFSRealtimeGolden -update   # regenerate ../gtfs-realtime.sql
```

Database tests use `psql` with the standard `PG*` environment variables. By
default they create a temporary database (dropped afterwards); set
`PGSQL_TEST_DATABASE=<name>` to use an existing one instead. Tests check the
generated SQL against the Go protobuf runtime in both directions (Go marshal →
SQL `decode_*`, SQL `encode_*` → Go unmarshal).
