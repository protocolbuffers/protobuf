package main

import (
	"fmt"
	"regexp"
	"strings"

	"google.golang.org/protobuf/compiler/protogen"
	"google.golang.org/protobuf/reflect/protoreflect"
)

// sanitizeIdent makes a safe PostgreSQL identifier (unquoted). We quote anyway,
// but keep it alphanumeric/underscore and not starting with a digit.
func sanitizeIdent(s string) string {
	if s == "" {
		return "x"
	}
	s = regexp.MustCompile(`[^A-Za-z0-9_]`).ReplaceAllString(s, "_")
	if s[0] >= '0' && s[0] <= '9' {
		s = "_" + s
	}
	// avoid empty
	if s == "" || s == "_" {
		s = "x"
	}
	return s
}

func quoteIdent(s string) string {
	return `"` + strings.ReplaceAll(s, `"`, `""`) + `"`
}

// schemaName derives a schema name from a proto package name.
func schemaName(pkg protoreflect.FullName) string {
	p := string(pkg)
	if p == "" {
		return "public"
	}
	// flatten nested packages with underscore
	p = strings.ReplaceAll(p, ".", "_")
	return sanitizeIdent(p)
}

// messageFuncName builds a unique function name for a message's decoder.
// Uses the full name flattened: transit_realtime.FeedMessage -> decode_transit_realtime_FeedMessage?
// We keep it shorter: decode_<Msg> inside its schema, but nested messages include parents.
func messageFuncName(msg *protogen.Message) string {
	// flatten full name (without package) parents with underscore
	full := string(msg.Desc.FullName()) // includes package
	// strip package prefix
	pkg := string(msg.Desc.ParentFile().Package())
	name := full
	if pkg != "" {
		name = strings.TrimPrefix(full, pkg+".")
	}
	name = strings.ReplaceAll(name, ".", "_")
	return "decode_" + strings.ToLower(sanitizeIdent(name))
}

// enumValueName returns the proto enum value name for a numeric value.
// (handled inline; this is a helper for codegen)
func intStr(n int) string { return fmt.Sprintf("%d", n) }
