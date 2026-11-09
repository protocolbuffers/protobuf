package main

// Shared test harness:
//   - compileProtos: compiles .proto sources in-process (no protoc needed)
//   - runGenerator:  drives the plugin in-process with a CodeGeneratorRequest
//   - testDB:        thin psql(1) wrapper around a throwaway PostgreSQL database
//
// Database tests are skipped with -short, when psql is unavailable, or when a
// database cannot be created. Set PGSQL_TEST_DATABASE to use an existing
// database instead of creating (and dropping) a temporary one; connection
// parameters otherwise come from the standard libpq PG* environment variables.

import (
	"bytes"
	"context"
	"encoding/hex"
	"encoding/json"
	"flag"
	"fmt"
	"math/big"
	"os"
	"os/exec"
	"reflect"
	"sort"
	"strings"
	"testing"

	"github.com/bufbuild/protocompile"
	"google.golang.org/protobuf/encoding/protojson"
	"google.golang.org/protobuf/proto"
	"google.golang.org/protobuf/reflect/protodesc"
	"google.golang.org/protobuf/reflect/protoreflect"
	"google.golang.org/protobuf/reflect/protoregistry"
	"google.golang.org/protobuf/types/descriptorpb"
	"google.golang.org/protobuf/types/dynamicpb"
	"google.golang.org/protobuf/types/pluginpb"
)

var updateGolden = flag.Bool("update", false, "rewrite golden files")

// ---------------------------------------------------------------------------
// Compilation

type compiled struct {
	fdps  []*descriptorpb.FileDescriptorProto // all files, dependency order
	files *protoregistry.Files
	types *dynamicpb.Types
}

// compileProtos compiles the given in-memory sources (path -> content).
// google/protobuf/*.proto imports resolve to the standard WKT sources.
func compileProtos(t testing.TB, sources map[string]string) *compiled {
	t.Helper()
	var names []string
	for name := range sources {
		names = append(names, name)
	}
	sort.Strings(names)
	c := protocompile.Compiler{
		Resolver: protocompile.WithStandardImports(&protocompile.SourceResolver{
			Accessor: protocompile.SourceAccessorFromMap(sources),
		}),
	}
	res, err := c.Compile(context.Background(), names...)
	if err != nil {
		t.Fatalf("compile protos: %v", err)
	}
	var fdps []*descriptorpb.FileDescriptorProto
	seen := map[string]bool{}
	var add func(fd protoreflect.FileDescriptor)
	add = func(fd protoreflect.FileDescriptor) {
		if seen[fd.Path()] {
			return
		}
		seen[fd.Path()] = true
		imps := fd.Imports()
		for i := 0; i < imps.Len(); i++ {
			add(imps.Get(i).FileDescriptor)
		}
		fdps = append(fdps, protodesc.ToFileDescriptorProto(fd))
	}
	for _, f := range res {
		add(f)
	}
	files, err := protodesc.NewFiles(&descriptorpb.FileDescriptorSet{File: fdps})
	if err != nil {
		t.Fatalf("build registry: %v", err)
	}
	return &compiled{fdps: fdps, files: files, types: dynamicpb.NewTypes(files)}
}

// runGenerator runs the plugin on the named files and returns generated
// file name -> content. param is the raw plugin parameter (e.g. "encode=false").
func runGenerator(t testing.TB, c *compiled, param string, generate ...string) map[string]string {
	t.Helper()
	out, err := tryRunGenerator(c, param, generate...)
	if err != nil {
		t.Fatalf("generator: %v", err)
	}
	return out
}

func tryRunGenerator(c *compiled, param string, generate ...string) (map[string]string, error) {
	// protogen requires a Go import path for every file; supply one for
	// files without go_package via M options, as a protoc user would.
	var params []string
	for _, fd := range c.fdps {
		if fd.GetOptions().GetGoPackage() == "" {
			params = append(params, fmt.Sprintf("M%s=example.com/test/%s", fd.GetName(), strings.TrimSuffix(fd.GetName(), ".proto")))
		}
	}
	if param != "" {
		params = append(params, param)
	}
	req := &pluginpb.CodeGeneratorRequest{
		FileToGenerate: generate,
		Parameter:      proto.String(strings.Join(params, ",")),
		ProtoFile:      c.fdps,
	}
	opts, run := pluginOptions()
	gen, err := opts.New(req)
	if err != nil {
		return nil, err
	}
	if err := run(gen); err != nil {
		return nil, err
	}
	resp := gen.Response()
	if resp.Error != nil {
		return nil, fmt.Errorf("%s", resp.GetError())
	}
	out := map[string]string{}
	for _, f := range resp.File {
		out[f.GetName()] = f.GetContent()
	}
	return out, nil
}

// generateOne compiles a single proto source and returns its generated SQL.
func generateOne(t testing.TB, name, src, param string) (*compiled, string) {
	t.Helper()
	c := compileProtos(t, map[string]string{name: src})
	out := runGenerator(t, c, param, name)
	if len(out) != 1 {
		t.Fatalf("expected 1 generated file, got %d", len(out))
	}
	for _, v := range out {
		return c, v
	}
	panic("unreachable")
}

// ---------------------------------------------------------------------------
// Messages

// newMessage builds a dynamic message of the named type from protojson text.
func (c *compiled) newMessage(t testing.TB, fullName, protoJSON string) *dynamicpb.Message {
	t.Helper()
	m := c.emptyMessage(t, fullName)
	if err := (protojson.UnmarshalOptions{Resolver: c.types}).Unmarshal([]byte(protoJSON), m); err != nil {
		t.Fatalf("protojson unmarshal %s: %v", fullName, err)
	}
	return m
}

func (c *compiled) emptyMessage(t testing.TB, fullName string) *dynamicpb.Message {
	t.Helper()
	d, err := c.files.FindDescriptorByName(protoreflect.FullName(fullName))
	if err != nil {
		t.Fatalf("find %s: %v", fullName, err)
	}
	md, ok := d.(protoreflect.MessageDescriptor)
	if !ok {
		t.Fatalf("%s is not a message", fullName)
	}
	return dynamicpb.NewMessage(md)
}

func marshal(t testing.TB, m proto.Message) []byte {
	t.Helper()
	b, err := proto.Marshal(m)
	if err != nil {
		t.Fatalf("marshal: %v", err)
	}
	return b
}

// ---------------------------------------------------------------------------
// JSON comparison

// normalizeJSON parses s with numbers as exact rationals so that e.g. 1.50
// and 1.5, or 18446744073709551615 written either way, compare equal.
func normalizeJSON(t testing.TB, s string) any {
	t.Helper()
	dec := json.NewDecoder(strings.NewReader(s))
	dec.UseNumber()
	var v any
	if err := dec.Decode(&v); err != nil {
		t.Fatalf("invalid JSON %q: %v", s, err)
	}
	return normNumbers(t, v)
}

func normNumbers(t testing.TB, v any) any {
	switch x := v.(type) {
	case json.Number:
		r, ok := new(big.Rat).SetString(string(x))
		if !ok {
			t.Fatalf("bad number %q", x)
		}
		return "num:" + r.RatString()
	case map[string]any:
		for k, e := range x {
			x[k] = normNumbers(t, e)
		}
	case []any:
		for i, e := range x {
			x[i] = normNumbers(t, e)
		}
	}
	return v
}

func assertJSONEqual(t testing.TB, got, want string) {
	t.Helper()
	if !reflect.DeepEqual(normalizeJSON(t, got), normalizeJSON(t, want)) {
		t.Errorf("JSON mismatch\n got: %s\nwant: %s", got, want)
	}
}

// ---------------------------------------------------------------------------
// PostgreSQL

type testDB struct {
	name string
}

var (
	sharedDB     *testDB
	sharedDBErr  error
	dropDBOnExit string
)

func TestMain(m *testing.M) {
	flag.Parse()
	code := m.Run()
	if dropDBOnExit != "" {
		cmd := exec.Command("psql", "-X", "-q", "-d", "postgres", "-c", "DROP DATABASE IF EXISTS "+quoteIdent(dropDBOnExit)+" WITH (FORCE)")
		if out, err := cmd.CombinedOutput(); err != nil {
			fmt.Fprintf(os.Stderr, "warning: dropping test database %s: %v\n%s", dropDBOnExit, err, out)
		}
	}
	os.Exit(code)
}

func setupDB() (*testDB, error) {
	if _, err := exec.LookPath("psql"); err != nil {
		return nil, fmt.Errorf("psql not found in PATH")
	}
	if name := os.Getenv("PGSQL_TEST_DATABASE"); name != "" {
		db := &testDB{name: name}
		if _, err := db.tryQuery("SELECT 1"); err != nil {
			return nil, fmt.Errorf("connect to %s: %v", name, err)
		}
		return db, nil
	}
	name := fmt.Sprintf("protoc_gen_pgsql_test_%d", os.Getpid())
	cmd := exec.Command("psql", "-X", "-q", "-v", "ON_ERROR_STOP=1", "-d", "postgres", "-c", "CREATE DATABASE "+quoteIdent(name))
	if out, err := cmd.CombinedOutput(); err != nil {
		return nil, fmt.Errorf("create database: %v: %s", err, bytes.TrimSpace(out))
	}
	dropDBOnExit = name
	return &testDB{name: name}, nil
}

// requireDB returns the shared test database or skips the test.
func requireDB(t testing.TB) *testDB {
	t.Helper()
	if testing.Short() {
		t.Skip("skipping PostgreSQL test in -short mode")
	}
	if sharedDB == nil && sharedDBErr == nil {
		sharedDB, sharedDBErr = setupDB()
	}
	if sharedDBErr != nil {
		t.Skipf("PostgreSQL unavailable: %v", sharedDBErr)
	}
	return sharedDB
}

func (db *testDB) psql(stdin string, args ...string) (string, error) {
	base := []string{"-X", "-q", "-A", "-t", "-v", "ON_ERROR_STOP=1", "-d", db.name}
	cmd := exec.Command("psql", append(base, args...)...)
	cmd.Stdin = strings.NewReader(stdin)
	var stdout, stderr bytes.Buffer
	cmd.Stdout, cmd.Stderr = &stdout, &stderr
	if err := cmd.Run(); err != nil {
		return "", fmt.Errorf("%v: %s", err, strings.TrimSpace(stderr.String()))
	}
	return strings.TrimRight(stdout.String(), "\n"), nil
}

func (db *testDB) tryQuery(sql string) (string, error) {
	return db.psql(sql, "-f", "-")
}

func (db *testDB) query(t testing.TB, sql string) string {
	t.Helper()
	out, err := db.tryQuery(sql)
	if err != nil {
		t.Fatalf("query failed: %v\nSQL: %s", err, sql)
	}
	return out
}

// load executes generated SQL and drops the given schema when the test ends.
func (db *testDB) load(t testing.TB, sql, schema string) {
	t.Helper()
	t.Cleanup(func() {
		db.tryQuery("DROP SCHEMA IF EXISTS " + quoteIdent(schema) + " CASCADE")
	})
	if _, err := db.psql(sql, "-f", "-"); err != nil {
		t.Fatalf("loading generated SQL: %v", err)
	}
}

// decode calls schema.fn(bytes) and returns the jsonb result as text
// ("" for SQL NULL).
func (db *testDB) decode(t testing.TB, schema, fn string, data []byte) string {
	t.Helper()
	out, err := db.tryDecode(schema, fn, data)
	if err != nil {
		t.Fatalf("%s.%s: %v", schema, fn, err)
	}
	return out
}

func (db *testDB) tryDecode(schema, fn string, data []byte) (string, error) {
	return db.tryQuery(fmt.Sprintf("SELECT %s.%s(decode('%s', 'hex'))::text;",
		quoteIdent(schema), quoteIdent(fn), hex.EncodeToString(data)))
}

// encode calls schema.fn(jsonb) and returns the produced bytes.
func (db *testDB) encode(t testing.TB, schema, fn, js string) []byte {
	t.Helper()
	out := db.query(t, fmt.Sprintf("SELECT encode(%s.%s(%s::jsonb), 'hex');",
		quoteIdent(schema), quoteIdent(fn), dollarQuote(js)))
	b, err := hex.DecodeString(out)
	if err != nil {
		t.Fatalf("bad hex from %s: %q", fn, out)
	}
	return b
}

func dollarQuote(s string) string {
	if strings.Contains(s, "$pgt$") {
		panic("literal contains delimiter")
	}
	return "$pgt$" + s + "$pgt$"
}
