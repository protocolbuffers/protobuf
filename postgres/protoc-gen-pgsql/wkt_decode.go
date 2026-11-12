package main

import (
	"strings"
)

const wktDecodersTemplate = `
-- Empty
CREATE OR REPLACE FUNCTION {SCHEMA}.decode_google_protobuf_Empty(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
BEGIN
  RETURN '{}'::jsonb;
END;
$$;

-- Wrappers
CREATE OR REPLACE FUNCTION {SCHEMA}.decode_google_protobuf_BoolValue(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE result jsonb := 'null'::jsonb; pos int := 0; tag numeric; fn int; wt int; v numeric; n int; b bytea;
BEGIN
  IF data IS NULL OR octet_length(data)=0 THEN RETURN result; END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM {SCHEMA}.pb_read_varint(data,pos) INTO tag,pos;
    fn := (tag::bigint / 8)::int; wt := (tag::numeric % 8)::int;
    IF fn=1 AND wt=0 THEN
      SELECT val, p FROM {SCHEMA}.pb_read_varint(data,pos) INTO v,pos;
      result := to_jsonb(v <> 0);
    ELSE
      pos := {SCHEMA}.pb_skip_field(data,pos,wt,fn);
    END IF;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION {SCHEMA}.decode_google_protobuf_Int32Value(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE result jsonb := 'null'::jsonb; pos int := 0; tag numeric; fn int; wt int; v numeric; n int; b bytea;
BEGIN
  IF data IS NULL OR octet_length(data)=0 THEN RETURN result; END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM {SCHEMA}.pb_read_varint(data,pos) INTO tag,pos;
    fn := (tag::bigint / 8)::int; wt := (tag::numeric % 8)::int;
    IF fn=1 AND wt=0 THEN
      SELECT val, p FROM {SCHEMA}.pb_read_varint(data,pos) INTO v,pos;
      result := to_jsonb(v);
    ELSE
      pos := {SCHEMA}.pb_skip_field(data,pos,wt,fn);
    END IF;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION {SCHEMA}.decode_google_protobuf_Int64Value(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE result jsonb := 'null'::jsonb; pos int := 0; tag numeric; fn int; wt int; v numeric; n int; b bytea;
BEGIN
  IF data IS NULL OR octet_length(data)=0 THEN RETURN result; END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM {SCHEMA}.pb_read_varint(data,pos) INTO tag,pos;
    fn := (tag::bigint / 8)::int; wt := (tag::numeric % 8)::int;
    IF fn=1 AND wt=0 THEN
      SELECT val, p FROM {SCHEMA}.pb_read_varint(data,pos) INTO v,pos;
      result := to_jsonb(v);
    ELSE
      pos := {SCHEMA}.pb_skip_field(data,pos,wt,fn);
    END IF;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION {SCHEMA}.decode_google_protobuf_UInt32Value(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE result jsonb := 'null'::jsonb; pos int := 0; tag numeric; fn int; wt int; v numeric; n int; b bytea;
BEGIN
  IF data IS NULL OR octet_length(data)=0 THEN RETURN result; END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM {SCHEMA}.pb_read_varint(data,pos) INTO tag,pos;
    fn := (tag::bigint / 8)::int; wt := (tag::numeric % 8)::int;
    IF fn=1 AND wt=0 THEN
      SELECT val, p FROM {SCHEMA}.pb_read_varint(data,pos) INTO v,pos;
      result := to_jsonb(v);
    ELSE
      pos := {SCHEMA}.pb_skip_field(data,pos,wt,fn);
    END IF;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION {SCHEMA}.decode_google_protobuf_UInt64Value(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE result jsonb := 'null'::jsonb; pos int := 0; tag numeric; fn int; wt int; v numeric; n int; b bytea;
BEGIN
  IF data IS NULL OR octet_length(data)=0 THEN RETURN result; END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM {SCHEMA}.pb_read_varint(data,pos) INTO tag,pos;
    fn := (tag::bigint / 8)::int; wt := (tag::numeric % 8)::int;
    IF fn=1 AND wt=0 THEN
      SELECT val, p FROM {SCHEMA}.pb_read_varint(data,pos) INTO v,pos;
      result := to_jsonb(v);
    ELSE
      pos := {SCHEMA}.pb_skip_field(data,pos,wt,fn);
    END IF;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION {SCHEMA}.decode_google_protobuf_FloatValue(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE result jsonb := 'null'::jsonb; pos int := 0; tag numeric; fn int; wt int; v numeric; n int; b bytea;
BEGIN
  IF data IS NULL OR octet_length(data)=0 THEN RETURN result; END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM {SCHEMA}.pb_read_varint(data,pos) INTO tag,pos;
    fn := (tag::bigint / 8)::int; wt := (tag::numeric % 8)::int;
    IF fn=1 AND wt=5 THEN
      SELECT val, p FROM {SCHEMA}.pb_read_fixed32(data,pos) INTO v,pos;
      v := {SCHEMA}.pb_fixed32_to_float(v);
      result := to_jsonb(v);
    ELSE
      pos := {SCHEMA}.pb_skip_field(data,pos,wt,fn);
    END IF;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION {SCHEMA}.decode_google_protobuf_DoubleValue(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE result jsonb := 'null'::jsonb; pos int := 0; tag numeric; fn int; wt int; v numeric; n int; b bytea;
BEGIN
  IF data IS NULL OR octet_length(data)=0 THEN RETURN result; END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM {SCHEMA}.pb_read_varint(data,pos) INTO tag,pos;
    fn := (tag::bigint / 8)::int; wt := (tag::numeric % 8)::int;
    IF fn=1 AND wt=1 THEN
      SELECT val, p FROM {SCHEMA}.pb_read_fixed64(data,pos) INTO v,pos;
      v := {SCHEMA}.pb_fixed64_to_double(v);
      result := to_jsonb(v);
    ELSE
      pos := {SCHEMA}.pb_skip_field(data,pos,wt,fn);
    END IF;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION {SCHEMA}.decode_google_protobuf_StringValue(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE result jsonb := 'null'::jsonb; pos int := 0; tag numeric; fn int; wt int; v numeric; n int; b bytea; s text;
BEGIN
  IF data IS NULL OR octet_length(data)=0 THEN RETURN result; END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM {SCHEMA}.pb_read_varint(data,pos) INTO tag,pos;
    fn := (tag::bigint / 8)::int; wt := (tag::numeric % 8)::int;
    IF fn=1 AND wt=2 THEN
      SELECT val, p FROM {SCHEMA}.pb_read_varint(data,pos) INTO n,pos;
      b := substring(data from pos+1 for n); pos := pos+n;
      s := convert_from(b,'UTF8');
      result := to_jsonb(s);
    ELSE
      pos := {SCHEMA}.pb_skip_field(data,pos,wt,fn);
    END IF;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION {SCHEMA}.decode_google_protobuf_BytesValue(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE result jsonb := 'null'::jsonb; pos int := 0; tag numeric; fn int; wt int; v numeric; n int; b bytea; s text;
BEGIN
  IF data IS NULL OR octet_length(data)=0 THEN RETURN result; END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM {SCHEMA}.pb_read_varint(data,pos) INTO tag,pos;
    fn := (tag::bigint / 8)::int; wt := (tag::numeric % 8)::int;
    IF fn=1 AND wt=2 THEN
      SELECT val, p FROM {SCHEMA}.pb_read_varint(data,pos) INTO n,pos;
      b := substring(data from pos+1 for n); pos := pos+n;
      s := encode(b,'base64');
      result := to_jsonb(s);
    ELSE
      pos := {SCHEMA}.pb_skip_field(data,pos,wt,fn);
    END IF;
  END LOOP;
  RETURN result;
END;
$$;

-- Timestamp / Duration as objects
CREATE OR REPLACE FUNCTION {SCHEMA}.decode_google_protobuf_Timestamp(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE result jsonb := '{}'::jsonb; pos int := 0; tag numeric; fn int; wt int; v numeric; n int; b bytea;
BEGIN
  IF data IS NULL OR octet_length(data)=0 THEN RETURN result; END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM {SCHEMA}.pb_read_varint(data,pos) INTO tag,pos;
    fn := (tag::bigint / 8)::int; wt := (tag::numeric % 8)::int;
    IF fn=1 AND wt=0 THEN
      SELECT val, p FROM {SCHEMA}.pb_read_varint(data,pos) INTO v,pos;
      result := jsonb_set(result,'{seconds}',to_jsonb(v),true);
    ELSIF fn=2 AND wt=0 THEN
      SELECT val, p FROM {SCHEMA}.pb_read_varint(data,pos) INTO v,pos;
      result := jsonb_set(result,'{nanos}',to_jsonb(v),true);
    ELSE
      pos := {SCHEMA}.pb_skip_field(data,pos,wt,fn);
    END IF;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION {SCHEMA}.decode_google_protobuf_Duration(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE result jsonb := '{}'::jsonb; pos int := 0; tag numeric; fn int; wt int; v numeric; n int; b bytea;
BEGIN
  IF data IS NULL OR octet_length(data)=0 THEN RETURN result; END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM {SCHEMA}.pb_read_varint(data,pos) INTO tag,pos;
    fn := (tag::bigint / 8)::int; wt := (tag::numeric % 8)::int;
    IF fn=1 AND wt=0 THEN
      SELECT val, p FROM {SCHEMA}.pb_read_varint(data,pos) INTO v,pos;
      result := jsonb_set(result,'{seconds}',to_jsonb(v),true);
    ELSIF fn=2 AND wt=0 THEN
      SELECT val, p FROM {SCHEMA}.pb_read_varint(data,pos) INTO v,pos;
      result := jsonb_set(result,'{nanos}',to_jsonb(v),true);
    ELSE
      pos := {SCHEMA}.pb_skip_field(data,pos,wt,fn);
    END IF;
  END LOOP;
  RETURN result;
END;
$$;

-- Any
CREATE OR REPLACE FUNCTION {SCHEMA}.decode_google_protobuf_Any(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE result jsonb := '{}'::jsonb; pos int := 0; tag numeric; fn int; wt int; v numeric; n int; b bytea; s text;
BEGIN
  IF data IS NULL OR octet_length(data)=0 THEN RETURN result; END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM {SCHEMA}.pb_read_varint(data,pos) INTO tag,pos;
    fn := (tag::bigint / 8)::int; wt := (tag::numeric % 8)::int;
    IF fn=1 AND wt=2 THEN
      SELECT val, p FROM {SCHEMA}.pb_read_varint(data,pos) INTO n,pos;
      b := substring(data from pos+1 for n); pos := pos+n;
      s := convert_from(b,'UTF8');
      result := jsonb_set(result,'{type_url}',to_jsonb(s),true);
    ELSIF fn=2 AND wt=2 THEN
      SELECT val, p FROM {SCHEMA}.pb_read_varint(data,pos) INTO n,pos;
      b := substring(data from pos+1 for n); pos := pos+n;
      s := encode(b,'base64');
      result := jsonb_set(result,'{value}',to_jsonb(s),true);
    ELSE
      pos := {SCHEMA}.pb_skip_field(data,pos,wt,fn);
    END IF;
  END LOOP;
  RETURN result;
END;
$$;

-- FieldMask (paths repeated string)
CREATE OR REPLACE FUNCTION {SCHEMA}.decode_google_protobuf_FieldMask(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE result jsonb := '[]'::jsonb; pos int := 0; tag numeric; fn int; wt int; v numeric; n int; b bytea; s text;
BEGIN
  IF data IS NULL OR octet_length(data)=0 THEN RETURN result; END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM {SCHEMA}.pb_read_varint(data,pos) INTO tag,pos;
    fn := (tag::bigint / 8)::int; wt := (tag::numeric % 8)::int;
    IF fn=1 AND wt=2 THEN
      SELECT val, p FROM {SCHEMA}.pb_read_varint(data,pos) INTO n,pos;
      b := substring(data from pos+1 for n); pos := pos+n;
      s := convert_from(b,'UTF8');
      result := result || jsonb_build_array(s);
    ELSE
      pos := {SCHEMA}.pb_skip_field(data,pos,wt,fn);
    END IF;
  END LOOP;
  RETURN result;
END;
$$;

-- Value native decode
CREATE OR REPLACE FUNCTION {SCHEMA}.pb_decode_value(submsg jsonb) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
BEGIN
  -- submsg is already decoded as object with one of the oneof fields
  IF submsg IS NULL THEN RETURN 'null'::jsonb; END IF;
  IF submsg ? 'null_value' THEN RETURN 'null'::jsonb; END IF;
  IF submsg ? 'number_value' THEN RETURN submsg->'number_value'; END IF;
  IF submsg ? 'string_value' THEN RETURN submsg->'string_value'; END IF;
  IF submsg ? 'bool_value' THEN RETURN submsg->'bool_value'; END IF;
  IF submsg ? 'struct_value' THEN RETURN {SCHEMA}.pb_decode_struct(submsg->'struct_value'); END IF;
  IF submsg ? 'list_value' THEN RETURN {SCHEMA}.pb_decode_listvalue(submsg->'list_value'); END IF;
  RETURN submsg;
END;
$$;

CREATE OR REPLACE FUNCTION {SCHEMA}.pb_decode_listvalue(submsg jsonb) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE res jsonb := '[]'::jsonb; elem jsonb;
BEGIN
  -- submsg is decoded ListValue object with 'values' array of Value messages (already decoded to objects)
  IF submsg ? 'values' THEN
    FOR elem IN SELECT jsonb_array_elements(submsg->'values') LOOP
      res := res || jsonb_build_array({SCHEMA}.pb_decode_value(elem));
    END LOOP;
  END IF;
  RETURN res;
END;
$$;

CREATE OR REPLACE FUNCTION {SCHEMA}.pb_decode_struct(submsg jsonb) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE res jsonb := '{}'::jsonb; k text; v jsonb;
BEGIN
  -- submsg is decoded Struct object with 'fields' map of string->Value (decoded as object with key/value)
  -- Our map decode stores fields as object: {"fields": {"key": <Value decoded as object>}}
  IF submsg ? 'fields' AND jsonb_typeof(submsg->'fields') = 'object' THEN
    FOR k, v IN SELECT key, value FROM jsonb_each(submsg->'fields') LOOP
      res := jsonb_set(res, ARRAY[k], {SCHEMA}.pb_decode_value(v), true);
    END LOOP;
  END IF;
  RETURN res;
END;
$$;

-- Value message decoder (produces intermediate object; pb_decode_value converts)
CREATE OR REPLACE FUNCTION {SCHEMA}.decode_google_protobuf_Value(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE result jsonb := '{}'::jsonb; pos int := 0; tag numeric; fn int; wt int; v numeric; n int; b bytea; s text; submsg jsonb;
BEGIN
  IF data IS NULL OR octet_length(data)=0 THEN RETURN result; END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM {SCHEMA}.pb_read_varint(data,pos) INTO tag,pos;
    fn := (tag::bigint / 8)::int; wt := (tag::numeric % 8)::int;
    IF fn=1 AND wt=0 THEN
      -- null_value (enum) -> store marker
      SELECT val, p FROM {SCHEMA}.pb_read_varint(data,pos) INTO v,pos;
      result := jsonb_set(result,'{null_value}',to_jsonb(v),true);
    ELSIF fn=2 AND wt=1 THEN
      SELECT val, p FROM {SCHEMA}.pb_read_fixed64(data,pos) INTO v,pos;
      v := {SCHEMA}.pb_fixed64_to_double(v);
      result := jsonb_set(result,'{number_value}',to_jsonb(v),true);
    ELSIF fn=3 AND wt=2 THEN
      SELECT val, p FROM {SCHEMA}.pb_read_varint(data,pos) INTO n,pos;
      b := substring(data from pos+1 for n); pos := pos+n;
      s := convert_from(b,'UTF8');
      result := jsonb_set(result,'{string_value}',to_jsonb(s),true);
    ELSIF fn=4 AND wt=0 THEN
      SELECT val, p FROM {SCHEMA}.pb_read_varint(data,pos) INTO v,pos;
      result := jsonb_set(result,'{bool_value}',to_jsonb(v <> 0),true);
    ELSIF fn=5 AND wt=2 THEN
      SELECT val, p FROM {SCHEMA}.pb_read_varint(data,pos) INTO n,pos;
      b := substring(data from pos+1 for n); pos := pos+n;
      submsg := {SCHEMA}.decode_google_protobuf_Struct(b);
      result := jsonb_set(result,'{struct_value}',submsg,true);
    ELSIF fn=6 AND wt=2 THEN
      SELECT val, p FROM {SCHEMA}.pb_read_varint(data,pos) INTO n,pos;
      b := substring(data from pos+1 for n); pos := pos+n;
      submsg := {SCHEMA}.decode_google_protobuf_ListValue(b);
      result := jsonb_set(result,'{list_value}',submsg,true);
    ELSE
      pos := {SCHEMA}.pb_skip_field(data,pos,wt,fn);
    END IF;
  END LOOP;
  RETURN result;
END;
$$;

-- ListValue message decoder
CREATE OR REPLACE FUNCTION {SCHEMA}.decode_google_protobuf_ListValue(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE result jsonb := '{}'::jsonb; pos int := 0; tag numeric; fn int; wt int; v numeric; n int; b bytea; submsg jsonb;
BEGIN
  IF data IS NULL OR octet_length(data)=0 THEN
    result := jsonb_set(result,'{values}', '[]'::jsonb, true);
    RETURN result;
  END IF;
  result := jsonb_set(result,'{values}', '[]'::jsonb, true);
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM {SCHEMA}.pb_read_varint(data,pos) INTO tag,pos;
    fn := (tag::bigint / 8)::int; wt := (tag::numeric % 8)::int;
    IF fn=1 AND wt=2 THEN
      SELECT val, p FROM {SCHEMA}.pb_read_varint(data,pos) INTO n,pos;
      b := substring(data from pos+1 for n); pos := pos+n;
      submsg := {SCHEMA}.decode_google_protobuf_Value(b);
      result := jsonb_set(result,'{values}', (result->'values') || jsonb_build_array(submsg), true);
    ELSE
      pos := {SCHEMA}.pb_skip_field(data,pos,wt,fn);
    END IF;
  END LOOP;
  RETURN result;
END;
$$;

-- Struct message decoder
CREATE OR REPLACE FUNCTION {SCHEMA}.decode_google_protobuf_Struct(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE result jsonb := '{}'::jsonb; pos int := 0; tag numeric; fn int; wt int; v numeric; n int; b bytea; submsg jsonb; keytext text;
  fields jsonb := '{}'::jsonb;
BEGIN
  IF data IS NULL OR octet_length(data)=0 THEN
    result := jsonb_set(result,'{fields}', fields, true);
    RETURN result;
  END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM {SCHEMA}.pb_read_varint(data,pos) INTO tag,pos;
    fn := (tag::bigint / 8)::int; wt := (tag::numeric % 8)::int;
    IF fn=1 AND wt=2 THEN
      SELECT val, p FROM {SCHEMA}.pb_read_varint(data,pos) INTO n,pos;
      b := substring(data from pos+1 for n); pos := pos+n;
      submsg := {SCHEMA}.decode_google_protobuf_Struct_FieldsEntry(b);
      keytext := COALESCE(submsg->>'key','');
      IF keytext <> '' THEN
        fields := jsonb_set(fields, ARRAY[keytext], {SCHEMA}.pb_decode_value(COALESCE(submsg->'value','{}'::jsonb)), true);
      END IF;
    ELSE
      pos := {SCHEMA}.pb_skip_field(data,pos,wt,fn);
    END IF;
  END LOOP;
  result := jsonb_set(result,'{fields}', fields, true);
  RETURN result;
END;
$$;

-- Map entry for Struct fields: fields map<string, Value>
CREATE OR REPLACE FUNCTION {SCHEMA}.decode_google_protobuf_Struct_FieldsEntry(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE result jsonb := '{}'::jsonb; pos int := 0; tag numeric; fn int; wt int; v numeric; n int; b bytea; s text; submsg jsonb;
BEGIN
  IF data IS NULL OR octet_length(data)=0 THEN RETURN result; END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM {SCHEMA}.pb_read_varint(data,pos) INTO tag,pos;
    fn := (tag::bigint / 8)::int; wt := (tag::numeric % 8)::int;
    IF fn=1 AND wt=2 THEN
      SELECT val, p FROM {SCHEMA}.pb_read_varint(data,pos) INTO n,pos;
      b := substring(data from pos+1 for n); pos := pos+n;
      s := convert_from(b,'UTF8');
      result := jsonb_set(result,'{key}',to_jsonb(s),true);
    ELSIF fn=2 AND wt=2 THEN
      SELECT val, p FROM {SCHEMA}.pb_read_varint(data,pos) INTO n,pos;
      b := substring(data from pos+1 for n); pos := pos+n;
      submsg := {SCHEMA}.decode_google_protobuf_Value(b);
      result := jsonb_set(result,'{value}',submsg,true);
    ELSE
      pos := {SCHEMA}.pb_skip_field(data,pos,wt,fn);
    END IF;
  END LOOP;
  RETURN result;
END;
$$;
`

func emitWKTDecoders(g *genContext) {
	s := strings.ReplaceAll(wktDecodersTemplate, "{SCHEMA}", quoteIdent(g.schema))
	g.out.P(s)
}
