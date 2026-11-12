package main

import (
	"fmt"
	"sort"
	"strings"

	"google.golang.org/protobuf/compiler/protogen"
	"google.golang.org/protobuf/reflect/protoreflect"
)

func (g *genContext) emitWKTEncoders() {
	es := strings.ReplaceAll(wktEncodeTemplate, "{SCHEMA}", quoteIdent(g.schema))
	g.out.P(es)
}

func (g *genContext) emitMessageEncode(m *protogen.Message) {
	full := string(m.Desc.FullName())
	if isWKT(full) {
		return
	}
	if g.generated["enc:"+full] {
		return
	}
	g.generated["enc:"+full] = true

	pkg := string(m.Desc.ParentFile().Package())
	name := strings.TrimPrefix(full, pkg+".")
	name = strings.ReplaceAll(name, ".", "_")
	fn := "encode_" + strings.ToLower(sanitizeIdent(name))

	g.out.P()
	g.out.P("CREATE OR REPLACE FUNCTION ", g.q(), ".", quoteIdent(fn), "(j jsonb) RETURNS bytea")
	g.out.P("LANGUAGE plpgsql IMMUTABLE STRICT AS $$")
	g.out.P("DECLARE")
	g.out.P("  buf bytea := ''::bytea;")
	g.out.P("  elem jsonb;")
	g.out.P("  k text;")
	g.out.P("  v jsonb;")
	g.out.P("  entrybuf bytea;")
	g.out.P("  enumval int;")
	g.out.P("  tmp numeric;")
	g.out.P("BEGIN")
	g.out.P("  IF j IS NULL OR jsonb_typeof(j) <> 'object' THEN")
	g.out.P("    RETURN buf;")
	g.out.P("  END IF;")

	type fentry struct {
		num int
		f   *protogen.Field
	}
	var entries []fentry
	for _, f := range m.Fields {
		if f.Desc.IsExtension() {
			continue
		}
		if int(f.Desc.Kind()) == int(protoreflect.GroupKind) {
			continue
		}
		entries = append(entries, fentry{int(f.Desc.Number()), f})
	}
	sort.Slice(entries, func(i, j int) bool { return entries[i].num < entries[j].num })

	for _, e := range entries {
		g.emitFieldEncode(e.f)
	}

	g.out.P("  RETURN buf;")
	g.out.P("END;")
	g.out.P("$$;")
}

func (g *genContext) emitFieldEncode(f *protogen.Field) {
	jsonKey := string(f.Desc.Name())
	num := int(f.Desc.Number())
	isRepeated := f.Desc.Cardinality() == protoreflect.Repeated
	isMap := f.Desc.IsMap()
	kind := f.Desc.Kind()

	presence := fmt.Sprintf("j ? %s AND j->%s IS NOT NULL", quoteLiteral(jsonKey), quoteLiteral(jsonKey))

	if isMap {
		g.out.P("  IF ", presence, " AND jsonb_typeof(j->", quoteLiteral(jsonKey), ") = 'object' THEN")
		g.out.P("    FOR k, v IN SELECT key, value FROM jsonb_each(j->", quoteLiteral(jsonKey), ") LOOP")
		entry := f.Message
		// key field
		g.out.P("      entrybuf := ''::bytea;")
		keyField := entry.Fields[0]
		valField := entry.Fields[1]
		_ = keyField
		_ = valField
		// key
		if keyField.Desc.Kind() == protoreflect.StringKind {
			g.out.P("      entrybuf := ", g.q(), ".pb_write_tag(entrybuf, 1, 2);")
			g.out.P("      entrybuf := ", g.q(), ".pb_write_string(entrybuf, k);")
		} else if keyField.Desc.Kind() == protoreflect.BoolKind {
			g.out.P("      entrybuf := ", g.q(), ".pb_write_tag(entrybuf, 1, 0);")
			g.out.P("      entrybuf := ", g.q(), ".pb_write_varint(entrybuf, CASE WHEN k::boolean THEN 1 ELSE 0 END);")
		} else {
			g.out.P("      entrybuf := ", g.q(), ".pb_write_tag(entrybuf, 1, 0);")
			g.out.P("      entrybuf := ", g.q(), ".pb_write_varint(entrybuf, k::numeric);")
		}
		// value (field 2): encode based on valField kind
		g.emitEncodeValueInto("entrybuf", 2, valField, "v")
		g.out.P("      buf := ", g.q(), ".pb_write_tag(buf, ", num, ", 2);")
		g.out.P("      buf := ", g.q(), ".pb_write_bytes(buf, entrybuf);")
		g.out.P("    END LOOP;")
		g.out.P("  END IF;")
		return
	}

	if isRepeated {
		// repeated message
		if kind == protoreflect.MessageKind {
			msg := f.Message
			g.out.P("  IF ", presence, " AND jsonb_typeof(j->", quoteLiteral(jsonKey), ") = 'array' THEN")
			g.out.P("    FOR elem IN SELECT jsonb_array_elements(j->", quoteLiteral(jsonKey), ") LOOP")
			if isWKT(string(msg.Desc.FullName())) {
				g.out.P("      buf := ", g.q(), ".pb_write_tag(buf, ", num, ", 2);")
				encName := "encode_" + strings.ToLower(sanitizeIdent(wktFuncNameBase(string(msg.Desc.FullName()))))
				g.out.P("      buf := ", g.q(), ".pb_write_bytes(buf, ", g.q(), ".", quoteIdent(encName), "(elem));")
			} else {
				ts := schemaName(msg.Desc.ParentFile().Package())
				encName := "encode_" + strings.ToLower(sanitizeIdent(messageBaseName(msg)))
				g.out.P("      buf := ", g.q(), ".pb_write_tag(buf, ", num, ", 2);")
				g.out.P("      buf := ", g.q(), ".pb_write_bytes(buf, ", quoteIdent(ts), ".", quoteIdent(encName), "(elem));")
			}
			g.out.P("    END LOOP;")
			g.out.P("  END IF;")
			return
		}
		// repeated scalar/enum/string/bytes
		g.out.P("  IF ", presence, " AND jsonb_typeof(j->", quoteLiteral(jsonKey), ") = 'array' THEN")
		g.out.P("    FOR elem IN SELECT jsonb_array_elements(j->", quoteLiteral(jsonKey), ") LOOP")
		g.emitEncodeValueInto("buf", num, f, "elem")
		g.out.P("    END LOOP;")
		g.out.P("  END IF;")
		return
	}

	// single field
	g.out.P("  IF ", presence, " THEN")
	g.emitEncodeValueInto("buf", num, f, "j->"+quoteLiteral(jsonKey))
	g.out.P("  END IF;")
}

// emitEncodeValueInto encodes a value into bufVar for field num.
// valueExpr is a SQL expression yielding jsonb.
func (g *genContext) emitEncodeValueInto(bufVar string, num int, f *protogen.Field, valueExpr string) {
	kind := f.Desc.Kind()
	isEnum := f.Desc.Enum() != nil

	if isEnum {
		enum := f.Enum
		g.out.P("      enumval := NULL;")
		for _, v := range enum.Values {
			g.out.P("      IF ", valueExpr, "->>0 IS NOT NULL AND ", valueExpr, "#>>'{}' = ", quoteLiteral(string(v.Desc.Name())), " THEN enumval := ", v.Desc.Number(), "; END IF;")
		}
		g.out.P("      IF enumval IS NULL AND jsonb_typeof(", valueExpr, ") = 'number' THEN enumval := (", valueExpr, "->>0)::int; END IF;")
		g.out.P("      IF enumval IS NOT NULL THEN")
		g.out.P("        ", bufVar, " := ", g.q(), ".pb_write_tag(", bufVar, ", ", num, ", 0);")
		g.out.P("        ", bufVar, " := ", g.q(), ".pb_write_varint(", bufVar, ", enumval);")
		g.out.P("      END IF;")
		return
	}

	switch kind {
	case protoreflect.BoolKind:
		g.out.P("      ", bufVar, " := ", g.q(), ".pb_write_tag(", bufVar, ", ", num, ", 0);")
		g.out.P("      ", bufVar, " := ", g.q(), ".pb_write_varint(", bufVar, ", CASE WHEN (", valueExpr, ")::boolean THEN 1 ELSE 0 END);")

	case protoreflect.Int32Kind, protoreflect.Int64Kind, protoreflect.Uint32Kind, protoreflect.Uint64Kind:
		g.out.P("      ", bufVar, " := ", g.q(), ".pb_write_tag(", bufVar, ", ", num, ", 0);")
		g.out.P("      ", bufVar, " := ", g.q(), ".pb_write_varint(", bufVar, ", (", valueExpr, ")::numeric);")

	case protoreflect.Sint32Kind, protoreflect.Sint64Kind:
		g.out.P("      tmp := (", valueExpr, ")::numeric;")
		g.out.P("      tmp := CASE WHEN tmp >= 0 THEN 2*tmp ELSE 2*(-tmp)-1 END;")
		g.out.P("      ", bufVar, " := ", g.q(), ".pb_write_tag(", bufVar, ", ", num, ", 0);")
		g.out.P("      ", bufVar, " := ", g.q(), ".pb_write_varint(", bufVar, ", tmp);")

	case protoreflect.Fixed32Kind, protoreflect.Sfixed32Kind:
		// uint32/fixed32: use fixed32
		if kind == protoreflect.Fixed32Kind || kind == protoreflect.Sfixed32Kind || kind == protoreflect.Uint32Kind {
			g.out.P("      ", bufVar, " := ", g.q(), ".pb_write_tag(", bufVar, ", ", num, ", 5);")
			g.out.P("      ", bufVar, " := ", g.q(), ".pb_write_fixed32(", bufVar, ", (", valueExpr, ")::numeric);")
		}

	case protoreflect.Fixed64Kind, protoreflect.Sfixed64Kind:
		g.out.P("      ", bufVar, " := ", g.q(), ".pb_write_tag(", bufVar, ", ", num, ", 1);")
		g.out.P("      ", bufVar, " := ", g.q(), ".pb_write_fixed64(", bufVar, ", (", valueExpr, ")::numeric);")

	case protoreflect.FloatKind:
		g.out.P("      ", bufVar, " := ", g.q(), ".pb_write_tag(", bufVar, ", ", num, ", 5);")
		g.out.P("      ", bufVar, " := ", g.q(), ".pb_write_float4(", bufVar, ", (", valueExpr, ")::float8);")

	case protoreflect.DoubleKind:
		g.out.P("      ", bufVar, " := ", g.q(), ".pb_write_tag(", bufVar, ", ", num, ", 1);")
		g.out.P("      ", bufVar, " := ", g.q(), ".pb_write_float8(", bufVar, ", (", valueExpr, ")::float8);")

	case protoreflect.StringKind:
		g.out.P("      ", bufVar, " := ", g.q(), ".pb_write_tag(", bufVar, ", ", num, ", 2);")
		g.out.P("      ", bufVar, " := ", g.q(), ".pb_write_string(", bufVar, ", ", valueExpr, "->>0);")

	case protoreflect.BytesKind:
		g.out.P("      ", bufVar, " := ", g.q(), ".pb_write_tag(", bufVar, ", ", num, ", 2);")
		g.out.P("      ", bufVar, " := ", g.q(), ".pb_write_bytes(", bufVar, ", decode(", valueExpr, "->>0, 'base64'));")

	case protoreflect.MessageKind:
		msg := f.Message
		if isWKT(string(msg.Desc.FullName())) {
			encName := "encode_" + strings.ToLower(sanitizeIdent(wktFuncNameBase(string(msg.Desc.FullName()))))
			g.out.P("      ", bufVar, " := ", g.q(), ".pb_write_tag(", bufVar, ", ", num, ", 2);")
			g.out.P("      ", bufVar, " := ", g.q(), ".pb_write_bytes(", bufVar, ", ", g.q(), ".", quoteIdent(encName), "(", valueExpr, "));")
		} else {
			ts := schemaName(msg.Desc.ParentFile().Package())
			encName := "encode_" + strings.ToLower(sanitizeIdent(messageBaseName(msg)))
			g.out.P("      ", bufVar, " := ", g.q(), ".pb_write_tag(", bufVar, ", ", num, ", 2);")
			g.out.P("      ", bufVar, " := ", g.q(), ".pb_write_bytes(", bufVar, ", ", quoteIdent(ts), ".", quoteIdent(encName), "(", valueExpr, "));")
		}
	}
}

func wktFuncNameBase(fullName string) string {
	name := strings.ReplaceAll(fullName, ".", "_")
	return name
}

func messageBaseName(m *protogen.Message) string {
	full := string(m.Desc.FullName())
	pkg := string(m.Desc.ParentFile().Package())
	name := strings.TrimPrefix(full, pkg+".")
	name = strings.ReplaceAll(name, ".", "_")
	return name
}
