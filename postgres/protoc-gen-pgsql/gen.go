package main

import (
	"fmt"
	"sort"
	"strings"

	"google.golang.org/protobuf/compiler/protogen"
	"google.golang.org/protobuf/reflect/protoreflect"
)

type genContext struct {
	out         *protogen.GeneratedFile
	file        *protogen.File
	schema      string
	exts        map[string][]*protogen.Field // extendee full name -> fields
	messages    []*protogen.Message
	generated   map[string]bool
	encode      bool
}

func (g *genContext) generate() {
	g.schema = schemaName(g.file.Desc.Package())
	g.messages = collectMessages(g.file)
	emitRuntime(g.out, g.schema)

	// Always emit WKT decoders (so user messages can reference them).
	emitWKTDecoders(g)

	// Extensions for messages in this file: add to exts already (global).
	// Generate user messages in post-order (leaves first) — collectMessages already post-order.
	for _, m := range g.messages {
		g.emitMessage(m)
	}

	if g.encode {
		g.emitWKTEncoders()
		for _, m := range g.messages {
			g.emitMessageEncode(m)
		}
	}
}

func (g *genContext) q() string { return quoteIdent(g.schema) }

func (g *genContext) emitMessage(m *protogen.Message) {
	full := string(m.Desc.FullName())
	if isWKT(full) {
		// WKT already emitted globally; skip duplicate
		if g.generated[full] {
			return
		}
		g.generated[full] = true
		// emit handled by emitWKTDecoders; but if a user message has same name? no
		return
	}
	if g.generated[full] {
		return
	}
	g.generated[full] = true

	fn := messageFuncName(m)
	// extension fields for this message
	extFields := g.exts[full]

	g.out.P()
	g.out.P("CREATE OR REPLACE FUNCTION ", g.q(), ".", quoteIdent(fn), "(data bytea) RETURNS jsonb")
	g.out.P("LANGUAGE plpgsql IMMUTABLE STRICT AS $$")
	g.out.P("DECLARE")
	g.out.P("  result jsonb := '{}'::jsonb;")
	g.out.P("  pos int := 0;")
	g.out.P("  tag numeric;")
	g.out.P("  field_num int;")
	g.out.P("  wire_type int;")
	g.out.P("  n int;")
	g.out.P("  v numeric;")
	g.out.P("  b bytea;")
	g.out.P("  s text;")
	g.out.P("  submsg jsonb;")
	g.out.P("  endpos int;")
	g.out.P("  mapobj jsonb;")
	g.out.P("  keytext text;")
	g.out.P("BEGIN")
	g.out.P("  IF data IS NULL OR octet_length(data) = 0 THEN")
	g.out.P("    RETURN result;")
	g.out.P("  END IF;")
	g.out.P("  WHILE pos < octet_length(data) LOOP")
	g.out.P("    SELECT val, p FROM ", g.q(), ".pb_read_varint(data, pos) INTO tag, pos;")
	g.out.P("    field_num := (tag::bigint / 8)::int;")
	g.out.P("    wire_type := (tag::numeric % 8)::int;")
	g.out.P("    CASE field_num")

	// Build field branches. Group real fields by number.
	// Collect (field number -> field) for normal fields + extensions.
	type branch struct {
		num    int
		fields []*protogen.Field // usually one; extensions/oneof may share? oneof fields have distinct numbers
	}
	branches := map[int]*branch{}
	addField := func(f *protogen.Field) {
		n := int(f.Desc.Number())
		if branches[n] == nil {
			branches[n] = &branch{num: n}
		}
		branches[n].fields = append(branches[n].fields, f)
	}
	for _, f := range m.Fields {
		if f.Desc.IsExtension() {
			continue
		}
		if int(f.Desc.Kind()) == int(protoreflect.GroupKind) {
			continue // skip groups
		}
		addField(f)
	}
	for _, ef := range extFields {
		addField(ef)
	}

	// sorted field numbers
	var nums []int
	for n := range branches {
		nums = append(nums, n)
	}
	sort.Ints(nums)

	for _, num := range nums {
		br := branches[num]
		// If multiple fields share a number (shouldn't for normal), handle first.
		f := br.fields[0]
		g.out.P("      WHEN ", num, " THEN")
		g.emitFieldDecode(m, f, extFields)
	}

	g.out.P("      ELSE")
	g.out.P("        pos := ", g.q(), ".pb_skip_field(data, pos, wire_type, field_num);")
	g.out.P("    END CASE;")
	g.out.P("  END LOOP;")
	g.out.P("  RETURN result;")
	g.out.P("END;")
	g.out.P("$$;")
}

func (g *genContext) mapEntryFunc(f *protogen.Field) string {
	msg := f.Message
	if msg == nil {
		return g.q() + `."decode_map_entry"`
	}
	return g.q() + `.` + quoteIdent(messageFuncName(msg))
}

func (g *genContext) emitFieldDecode(m *protogen.Message, f *protogen.Field, extFields []*protogen.Field) {
	_ = extFields
	jsonKey := string(f.Desc.Name())
	isRepeated := f.Desc.Cardinality() == protoreflect.Repeated
	isMap := f.Desc.IsMap()
	kind := f.Desc.Kind()
	isEnum := f.Desc.Enum() != nil

	if isMap {
		g.out.P("        IF wire_type = 2 THEN")
		g.out.P("          SELECT val, p FROM ", g.q(), ".pb_read_varint(data, pos) INTO n, pos;")
		g.out.P("          b := substring(data from pos+1 for n);")
		g.out.P("          pos := pos + n;")
		g.out.P("          submsg := ", g.mapEntryFunc(f), "(b);")
		g.out.P("          keytext := COALESCE(submsg->>'key', '');")
		g.out.P("          IF NOT (result ? ", quoteLiteral(jsonKey), ") THEN")
		g.out.P("            result := jsonb_set(result, ", jsonbPath(jsonKey), ", '{}'::jsonb, true);")
		g.out.P("          END IF;")
		g.out.P("          result := jsonb_set(result, ARRAY[", quoteLiteral(jsonKey), ", keytext], COALESCE(submsg->'value', 'null'::jsonb), true);")
		g.out.P("        ELSE")
		g.out.P("          pos := ", g.q(), ".pb_skip_field(data, pos, wire_type, field_num);")
		g.out.P("        END IF;")
		return
	}

	if kind == protoreflect.MessageKind {
		msg := f.Message
		wkt := ""
		if msg != nil {
			wkt = wktKind(string(msg.Desc.FullName()))
		}
		decodeFn := ""
		if msg != nil {
			if isWKT(string(msg.Desc.FullName())) {
				decodeFn = g.q() + "." + quoteIdent(wktFuncName(string(msg.Desc.FullName())))
			} else {
				ts := schemaName(msg.Desc.ParentFile().Package())
				decodeFn = quoteIdent(ts) + "." + quoteIdent(messageFuncName(msg))
			}
		}
		if isRepeated {
			g.out.P("        IF wire_type = 2 THEN")
			g.out.P("          SELECT val, p FROM ", g.q(), ".pb_read_varint(data, pos) INTO n, pos;")
			g.out.P("          b := substring(data from pos+1 for n);")
			g.out.P("          pos := pos + n;")
			g.out.P("          submsg := ", decodeFn, "(b);")
			g.out.P("          IF result ? ", quoteLiteral(jsonKey), " THEN")
			g.out.P("            result := jsonb_set(result, ", jsonbPath(jsonKey), ", (result->", quoteLiteral(jsonKey), ") || jsonb_build_array(submsg));")
			g.out.P("          ELSE")
			g.out.P("            result := jsonb_set(result, ", jsonbPath(jsonKey), ", jsonb_build_array(submsg));")
			g.out.P("          END IF;")
			g.out.P("        ELSE")
			g.out.P("          pos := ", g.q(), ".pb_skip_field(data, pos, wire_type, field_num);")
			g.out.P("        END IF;")
		} else {
			g.out.P("        IF wire_type = 2 THEN")
			g.out.P("          SELECT val, p FROM ", g.q(), ".pb_read_varint(data, pos) INTO n, pos;")
			g.out.P("          b := substring(data from pos+1 for n);")
			g.out.P("          pos := pos + n;")
			g.out.P("          submsg := ", decodeFn, "(b);")
			if wkt != "" {
				g.emitWKTUnwrap(wkt, jsonKey)
			} else {
				g.out.P("          result := jsonb_set(result, ", jsonbPath(jsonKey), ", submsg, true);")
			}
			g.out.P("        ELSE")
			g.out.P("          pos := ", g.q(), ".pb_skip_field(data, pos, wire_type, field_num);")
			g.out.P("        END IF;")
		}
		return
	}

	if kind == protoreflect.StringKind {
		if isRepeated {
			g.out.P("        IF wire_type = 2 THEN")
			g.out.P("          SELECT val, p FROM ", g.q(), ".pb_read_varint(data, pos) INTO n, pos;")
			g.out.P("          b := substring(data from pos+1 for n);")
			g.out.P("          pos := pos + n;")
			g.out.P("          s := convert_from(b, 'UTF8');")
			g.out.P("          IF result ? ", quoteLiteral(jsonKey), " THEN")
			g.out.P("            result := jsonb_set(result, ", jsonbPath(jsonKey), ", (result->", quoteLiteral(jsonKey), ") || jsonb_build_array(to_jsonb(s)));")
			g.out.P("          ELSE")
			g.out.P("            result := jsonb_set(result, ", jsonbPath(jsonKey), ", jsonb_build_array(to_jsonb(s)));")
			g.out.P("          END IF;")
			g.out.P("        ELSE")
			g.out.P("          pos := ", g.q(), ".pb_skip_field(data, pos, wire_type, field_num);")
			g.out.P("        END IF;")
		} else {
			g.out.P("        IF wire_type = 2 THEN")
			g.out.P("          SELECT val, p FROM ", g.q(), ".pb_read_varint(data, pos) INTO n, pos;")
			g.out.P("          b := substring(data from pos+1 for n);")
			g.out.P("          pos := pos + n;")
			g.out.P("          s := convert_from(b, 'UTF8');")
			g.out.P("          result := jsonb_set(result, ", jsonbPath(jsonKey), ", to_jsonb(s), true);")
			g.out.P("        ELSE")
			g.out.P("          pos := ", g.q(), ".pb_skip_field(data, pos, wire_type, field_num);")
			g.out.P("        END IF;")
		}
		return
	}

	if kind == protoreflect.BytesKind {
		if isRepeated {
			g.out.P("        IF wire_type = 2 THEN")
			g.out.P("          SELECT val, p FROM ", g.q(), ".pb_read_varint(data, pos) INTO n, pos;")
			g.out.P("          b := substring(data from pos+1 for n);")
			g.out.P("          pos := pos + n;")
			g.out.P("          s := encode(b, 'base64');")
			g.out.P("          IF result ? ", quoteLiteral(jsonKey), " THEN")
			g.out.P("            result := jsonb_set(result, ", jsonbPath(jsonKey), ", (result->", quoteLiteral(jsonKey), ") || jsonb_build_array(to_jsonb(s)));")
			g.out.P("          ELSE")
			g.out.P("            result := jsonb_set(result, ", jsonbPath(jsonKey), ", jsonb_build_array(to_jsonb(s)));")
			g.out.P("          END IF;")
			g.out.P("        ELSE")
			g.out.P("          pos := ", g.q(), ".pb_skip_field(data, pos, wire_type, field_num);")
			g.out.P("        END IF;")
		} else {
			g.out.P("        IF wire_type = 2 THEN")
			g.out.P("          SELECT val, p FROM ", g.q(), ".pb_read_varint(data, pos) INTO n, pos;")
			g.out.P("          b := substring(data from pos+1 for n);")
			g.out.P("          pos := pos + n;")
			g.out.P("          s := encode(b, 'base64');")
			g.out.P("          result := jsonb_set(result, ", jsonbPath(jsonKey), ", to_jsonb(s), true);")
			g.out.P("        ELSE")
			g.out.P("          pos := ", g.q(), ".pb_skip_field(data, pos, wire_type, field_num);")
			g.out.P("        END IF;")
		}
		return
	}

	if isEnum {
		g.emitEnumField(jsonKey, isRepeated, f.Enum)
		return
	}

	if kind == protoreflect.BoolKind {
		g.emitBoolField(jsonKey, isRepeated)
		return
	}

	if isVarintKind(kind) {
		zigzag := kind == protoreflect.Sint32Kind || kind == protoreflect.Sint64Kind
		g.emitVarintScalarField(jsonKey, isRepeated, zigzag, false, "int")
		return
	}

	if kind == protoreflect.Fixed32Kind || kind == protoreflect.Sfixed32Kind || kind == protoreflect.FloatKind {
		g.emitFixed32Field(jsonKey, isRepeated, kind == protoreflect.Sfixed32Kind, kind == protoreflect.FloatKind)
		return
	}

	if kind == protoreflect.Fixed64Kind || kind == protoreflect.Sfixed64Kind || kind == protoreflect.DoubleKind {
		g.emitFixed64Field(jsonKey, isRepeated, kind == protoreflect.Sfixed64Kind, kind == protoreflect.DoubleKind)
		return
	}

	g.out.P("        pos := ", g.q(), ".pb_skip_field(data, pos, wire_type, field_num);")
}

func (g *genContext) emitBoolField(jsonKey string, isRepeated bool) {
	if isRepeated {
		g.out.P("        IF wire_type = 0 THEN")
		g.out.P("          SELECT val, p FROM ", g.q(), ".pb_read_varint(data, pos) INTO v, pos;")
		g.out.P("          IF result ? ", quoteLiteral(jsonKey), " THEN")
		g.out.P("            result := jsonb_set(result, ", jsonbPath(jsonKey), ", (result->", quoteLiteral(jsonKey), ") || jsonb_build_array(to_jsonb(v <> 0)));")
		g.out.P("          ELSE")
		g.out.P("            result := jsonb_set(result, ", jsonbPath(jsonKey), ", jsonb_build_array(to_jsonb(v <> 0)));")
		g.out.P("          END IF;")
		g.out.P("        ELSIF wire_type = 2 THEN")
		g.out.P("          SELECT val, p FROM ", g.q(), ".pb_read_varint(data, pos) INTO n, pos;")
		g.out.P("          endpos := pos + n;")
		g.out.P("          WHILE pos < endpos LOOP")
		g.out.P("            SELECT val, p FROM ", g.q(), ".pb_read_varint(data, pos) INTO v, pos;")
		g.out.P("            IF result ? ", quoteLiteral(jsonKey), " THEN")
		g.out.P("              result := jsonb_set(result, ", jsonbPath(jsonKey), ", (result->", quoteLiteral(jsonKey), ") || jsonb_build_array(to_jsonb(v <> 0)));")
		g.out.P("            ELSE")
		g.out.P("              result := jsonb_set(result, ", jsonbPath(jsonKey), ", jsonb_build_array(to_jsonb(v <> 0)));")
		g.out.P("            END IF;")
		g.out.P("          END LOOP;")
		g.out.P("        ELSE")
		g.out.P("          pos := ", g.q(), ".pb_skip_field(data, pos, wire_type, field_num);")
		g.out.P("        END IF;")
	} else {
		g.out.P("        IF wire_type = 0 THEN")
		g.out.P("          SELECT val, p FROM ", g.q(), ".pb_read_varint(data, pos) INTO v, pos;")
		g.out.P("          result := jsonb_set(result, ", jsonbPath(jsonKey), ", to_jsonb(v <> 0), true);")
		g.out.P("        ELSE")
		g.out.P("          pos := ", g.q(), ".pb_skip_field(data, pos, wire_type, field_num);")
		g.out.P("        END IF;")
	}
}

func isVarintKind(k protoreflect.Kind) bool {
	switch k {
	case protoreflect.Int32Kind, protoreflect.Int64Kind,
		protoreflect.Uint32Kind, protoreflect.Uint64Kind,
		protoreflect.Sint32Kind, protoreflect.Sint64Kind:
		return true
	}
	return false
}

func isScalarKind(k protoreflect.Kind) bool {
	return isVarintKind(k) || k == protoreflect.BoolKind || k == protoreflect.EnumKind ||
		k == protoreflect.FloatKind || k == protoreflect.DoubleKind ||
		k == protoreflect.Fixed32Kind || k == protoreflect.Fixed64Kind ||
		k == protoreflect.Sfixed32Kind || k == protoreflect.Sfixed64Kind
}

func (g *genContext) emitVarintScalarField(jsonKey string, isRepeated, zigzag, _ bool, kind string) {
	_ = kind
	if isRepeated {
		g.out.P("        IF wire_type = 0 THEN")
		g.out.P("          SELECT val, p FROM ", g.q(), ".pb_read_varint(data, pos) INTO v, pos;")
		if zigzag {
			g.out.P("          v := ", g.q(), ".pb_zigzag(v);")
		}
		g.out.P("          IF result ? ", quoteLiteral(jsonKey), " THEN")
		g.out.P("            result := jsonb_set(result, ", jsonbPath(jsonKey), ", (result->", quoteLiteral(jsonKey), ") || jsonb_build_array(to_jsonb(v)));")
		g.out.P("          ELSE")
		g.out.P("            result := jsonb_set(result, ", jsonbPath(jsonKey), ", jsonb_build_array(to_jsonb(v)));")
		g.out.P("          END IF;")
		g.out.P("        ELSIF wire_type = 2 THEN")
		g.out.P("          SELECT val, p FROM ", g.q(), ".pb_read_varint(data, pos) INTO n, pos;")
		g.out.P("          endpos := pos + n;")
		g.out.P("          WHILE pos < endpos LOOP")
		g.out.P("            SELECT val, p FROM ", g.q(), ".pb_read_varint(data, pos) INTO v, pos;")
		if zigzag {
			g.out.P("            v := ", g.q(), ".pb_zigzag(v);")
		}
		g.out.P("            IF result ? ", quoteLiteral(jsonKey), " THEN")
		g.out.P("              result := jsonb_set(result, ", jsonbPath(jsonKey), ", (result->", quoteLiteral(jsonKey), ") || jsonb_build_array(to_jsonb(v)));")
		g.out.P("            ELSE")
		g.out.P("              result := jsonb_set(result, ", jsonbPath(jsonKey), ", jsonb_build_array(to_jsonb(v)));")
		g.out.P("            END IF;")
		g.out.P("          END LOOP;")
		g.out.P("        ELSE")
		g.out.P("          pos := ", g.q(), ".pb_skip_field(data, pos, wire_type, field_num);")
		g.out.P("        END IF;")
	} else {
		g.out.P("        IF wire_type = 0 THEN")
		g.out.P("          SELECT val, p FROM ", g.q(), ".pb_read_varint(data, pos) INTO v, pos;")
		if zigzag {
			g.out.P("          v := ", g.q(), ".pb_zigzag(v);")
		}
		g.out.P("          result := jsonb_set(result, ", jsonbPath(jsonKey), ", to_jsonb(v), true);")
		g.out.P("        ELSE")
		g.out.P("          pos := ", g.q(), ".pb_skip_field(data, pos, wire_type, field_num);")
		g.out.P("        END IF;")
	}
}

func (g *genContext) emitFixed32Field(jsonKey string, isRepeated, signed, isFloat bool) {
	if isRepeated {
		g.out.P("        IF wire_type = 5 THEN")
		g.out.P("          SELECT val, p FROM ", g.q(), ".pb_read_fixed32(data, pos) INTO v, pos;")
		g.out.P("          ", g.convFixed32(signed, isFloat))
		g.out.P("          IF result ? ", quoteLiteral(jsonKey), " THEN")
		g.out.P("            result := jsonb_set(result, ", jsonbPath(jsonKey), ", (result->", quoteLiteral(jsonKey), ") || jsonb_build_array(to_jsonb(v)));")
		g.out.P("          ELSE")
		g.out.P("            result := jsonb_set(result, ", jsonbPath(jsonKey), ", jsonb_build_array(to_jsonb(v)));")
		g.out.P("          END IF;")
		g.out.P("        ELSIF wire_type = 2 THEN")
		g.out.P("          SELECT val, p FROM ", g.q(), ".pb_read_varint(data, pos) INTO n, pos;")
		g.out.P("          endpos := pos + n;")
		g.out.P("          WHILE pos < endpos LOOP")
		g.out.P("            SELECT val, p FROM ", g.q(), ".pb_read_fixed32(data, pos) INTO v, pos;")
		g.out.P("            ", g.convFixed32(signed, isFloat))
		g.out.P("            IF result ? ", quoteLiteral(jsonKey), " THEN")
		g.out.P("              result := jsonb_set(result, ", jsonbPath(jsonKey), ", (result->", quoteLiteral(jsonKey), ") || jsonb_build_array(to_jsonb(v)));")
		g.out.P("            ELSE")
		g.out.P("              result := jsonb_set(result, ", jsonbPath(jsonKey), ", jsonb_build_array(to_jsonb(v)));")
		g.out.P("            END IF;")
		g.out.P("          END LOOP;")
		g.out.P("        ELSE")
		g.out.P("          pos := ", g.q(), ".pb_skip_field(data, pos, wire_type, field_num);")
		g.out.P("        END IF;")
	} else {
		g.out.P("        IF wire_type = 5 THEN")
		g.out.P("          SELECT val, p FROM ", g.q(), ".pb_read_fixed32(data, pos) INTO v, pos;")
		g.out.P("          ", g.convFixed32(signed, isFloat))
		g.out.P("          result := jsonb_set(result, ", jsonbPath(jsonKey), ", to_jsonb(v), true);")
		g.out.P("        ELSE")
		g.out.P("          pos := ", g.q(), ".pb_skip_field(data, pos, wire_type, field_num);")
		g.out.P("        END IF;")
	}
	_ = signed
	_ = isFloat
}

func (g *genContext) emitFixed64Field(jsonKey string, isRepeated, signed, isDouble bool) {
	if isRepeated {
		g.out.P("        IF wire_type = 1 THEN")
		g.out.P("          SELECT val, p FROM ", g.q(), ".pb_read_fixed64(data, pos) INTO v, pos;")
		g.out.P("          ", g.convFixed64(signed, isDouble))
		g.out.P("          IF result ? ", quoteLiteral(jsonKey), " THEN")
		g.out.P("            result := jsonb_set(result, ", jsonbPath(jsonKey), ", (result->", quoteLiteral(jsonKey), ") || jsonb_build_array(to_jsonb(v)));")
		g.out.P("          ELSE")
		g.out.P("            result := jsonb_set(result, ", jsonbPath(jsonKey), ", jsonb_build_array(to_jsonb(v)));")
		g.out.P("          END IF;")
		g.out.P("        ELSIF wire_type = 2 THEN")
		g.out.P("          SELECT val, p FROM ", g.q(), ".pb_read_varint(data, pos) INTO n, pos;")
		g.out.P("          endpos := pos + n;")
		g.out.P("          WHILE pos < endpos LOOP")
		g.out.P("            SELECT val, p FROM ", g.q(), ".pb_read_fixed64(data, pos) INTO v, pos;")
		g.out.P("            ", g.convFixed64(signed, isDouble))
		g.out.P("            IF result ? ", quoteLiteral(jsonKey), " THEN")
		g.out.P("              result := jsonb_set(result, ", jsonbPath(jsonKey), ", (result->", quoteLiteral(jsonKey), ") || jsonb_build_array(to_jsonb(v)));")
		g.out.P("            ELSE")
		g.out.P("              result := jsonb_set(result, ", jsonbPath(jsonKey), ", jsonb_build_array(to_jsonb(v)));")
		g.out.P("            END IF;")
		g.out.P("          END LOOP;")
		g.out.P("        ELSE")
		g.out.P("          pos := ", g.q(), ".pb_skip_field(data, pos, wire_type, field_num);")
		g.out.P("        END IF;")
	} else {
		g.out.P("        IF wire_type = 1 THEN")
		g.out.P("          SELECT val, p FROM ", g.q(), ".pb_read_fixed64(data, pos) INTO v, pos;")
		g.out.P("          ", g.convFixed64(signed, isDouble))
		g.out.P("          result := jsonb_set(result, ", jsonbPath(jsonKey), ", to_jsonb(v), true);")
		g.out.P("        ELSE")
		g.out.P("          pos := ", g.q(), ".pb_skip_field(data, pos, wire_type, field_num);")
		g.out.P("        END IF;")
	}
	_ = signed
	_ = isDouble
}

func (g *genContext) emitEnumField(jsonKey string, isRepeated bool, enum *protogen.Enum) {
	caseExpr := "CASE v::int "
	for _, v := range enum.Values {
		caseExpr += fmt.Sprintf("WHEN %d THEN %s ", int(v.Desc.Number()), quoteLiteral(string(v.Desc.Name())))
	}
	caseExpr += "ELSE v::text END"

	if isRepeated {
		g.out.P("        IF wire_type = 0 THEN")
		g.out.P("          SELECT val, p FROM ", g.q(), ".pb_read_varint(data, pos) INTO v, pos;")
		g.out.P("          s := ", caseExpr, ";")
		g.out.P("          IF result ? ", quoteLiteral(jsonKey), " THEN")
		g.out.P("            result := jsonb_set(result, ", jsonbPath(jsonKey), ", (result->", quoteLiteral(jsonKey), ") || jsonb_build_array(to_jsonb(s)));")
		g.out.P("          ELSE")
		g.out.P("            result := jsonb_set(result, ", jsonbPath(jsonKey), ", jsonb_build_array(to_jsonb(s)));")
		g.out.P("          END IF;")
		g.out.P("        ELSIF wire_type = 2 THEN")
		g.out.P("          SELECT val, p FROM ", g.q(), ".pb_read_varint(data, pos) INTO n, pos;")
		g.out.P("          endpos := pos + n;")
		g.out.P("          WHILE pos < endpos LOOP")
		g.out.P("            SELECT val, p FROM ", g.q(), ".pb_read_varint(data, pos) INTO v, pos;")
		g.out.P("            s := ", caseExpr, ";")
		g.out.P("            IF result ? ", quoteLiteral(jsonKey), " THEN")
		g.out.P("              result := jsonb_set(result, ", jsonbPath(jsonKey), ", (result->", quoteLiteral(jsonKey), ") || jsonb_build_array(to_jsonb(s)));")
		g.out.P("            ELSE")
		g.out.P("              result := jsonb_set(result, ", jsonbPath(jsonKey), ", jsonb_build_array(to_jsonb(s)));")
		g.out.P("            END IF;")
		g.out.P("          END LOOP;")
		g.out.P("        ELSE")
		g.out.P("          pos := ", g.q(), ".pb_skip_field(data, pos, wire_type, field_num);")
		g.out.P("        END IF;")
	} else {
		g.out.P("        IF wire_type = 0 THEN")
		g.out.P("          SELECT val, p FROM ", g.q(), ".pb_read_varint(data, pos) INTO v, pos;")
		g.out.P("          s := ", caseExpr, ";")
		g.out.P("          result := jsonb_set(result, ", jsonbPath(jsonKey), ", to_jsonb(s), true);")
		g.out.P("        ELSE")
		g.out.P("          pos := ", g.q(), ".pb_skip_field(data, pos, wire_type, field_num);")
		g.out.P("        END IF;")
	}
}

func (g *genContext) emitWKTUnwrap(wkt, jsonKey string) {
	switch wkt {
	case "wrapper_bool", "wrapper_int32", "wrapper_int64", "wrapper_uint32", "wrapper_uint64",
		"wrapper_float", "wrapper_double", "wrapper_string", "wrapper_bytes":
		g.out.P("          IF submsg ? 'value' THEN")
		g.out.P("            result := jsonb_set(result, ", jsonbPath(jsonKey), ", submsg->'value', true);")
		g.out.P("          ELSE")
		g.out.P("            result := jsonb_set(result, ", jsonbPath(jsonKey), ", submsg, true);")
		g.out.P("          END IF;")
	case "value":
		g.out.P("          result := jsonb_set(result, ", jsonbPath(jsonKey), ", ", g.q(), ".pb_decode_value(submsg), true);")
	case "listvalue":
		g.out.P("          result := jsonb_set(result, ", jsonbPath(jsonKey), ", ", g.q(), ".pb_decode_listvalue(submsg), true);")
	case "struct":
		g.out.P("          result := jsonb_set(result, ", jsonbPath(jsonKey), ", ", g.q(), ".pb_decode_struct(submsg), true);")
	case "empty":
		g.out.P("          result := jsonb_set(result, ", jsonbPath(jsonKey), ", '{}'::jsonb, true);")
	default:
		g.out.P("          result := jsonb_set(result, ", jsonbPath(jsonKey), ", submsg, true);")
	}
}

func jsonbPath(key string) string {
	return fmt.Sprintf("'{%s}'", key)
}

func quoteLiteral(s string) string {
	return "'" + strings.ReplaceAll(s, "'", "''") + "'"
}

func boolStr(b bool) string {
	if b {
		return "TRUE"
	}
	return "FALSE"
}


func (g *genContext) convFixed32(signed, isFloat bool) string {
	if isFloat {
		return "v := " + g.q() + ".pb_fixed32_to_float(v);"
	}
	if signed {
		return "IF v >= 2147483648 THEN v := v - 4294967296; END IF;"
	}
	return ""
}

func (g *genContext) convFixed64(signed, isDouble bool) string {
	if isDouble {
		return "v := " + g.q() + ".pb_fixed64_to_double(v);"
	}
	if signed {
		return "IF v >= 9223372036854775808 THEN v := v - 18446744073709551616; END IF;"
	}
	return ""
}
