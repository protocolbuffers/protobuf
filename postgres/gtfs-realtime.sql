-- runtime helpers (schema: "transit_realtime")
CREATE SCHEMA IF NOT EXISTS "transit_realtime";

CREATE OR REPLACE FUNCTION "transit_realtime".pb_read_varint(data bytea, INOUT p int, OUT val numeric)
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  b int;
  shift int := 0;
  acc numeric := 0;
  i int := 0;
BEGIN
  LOOP
    IF p >= octet_length(data) THEN
      RAISE EXCEPTION 'unexpected end of varint';
    END IF;
    b := get_byte(data, p);
    p := p + 1;
    acc := acc + ((b & 127)::numeric) * (2::numeric ^ shift);
    shift := shift + 7;
    i := i + 1;
    IF (b & 128) = 0 THEN
      EXIT;
    END IF;
    IF i >= 10 THEN
      RAISE EXCEPTION 'varint too long';
    END IF;
  END LOOP;
  val := trunc(acc);
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime".pb_read_fixed32(data bytea, INOUT p int, OUT val numeric)
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  b0 int; b1 int; b2 int; b3 int;
BEGIN
  IF p + 4 > octet_length(data) THEN
    RAISE EXCEPTION 'unexpected end of fixed32';
  END IF;
  b0 := get_byte(data, p);
  b1 := get_byte(data, p+1);
  b2 := get_byte(data, p+2);
  b3 := get_byte(data, p+3);
  p := p + 4;
  val := trunc((b0 + b1*256 + b2*65536 + b3*16777216)::numeric);
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime".pb_read_fixed64(data bytea, INOUT p int, OUT val numeric)
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  i int;
  acc numeric := 0;
  mult numeric := 1;
BEGIN
  IF p + 8 > octet_length(data) THEN
    RAISE EXCEPTION 'unexpected end of fixed64';
  END IF;
  FOR i IN 0..7 LOOP
    acc := acc + get_byte(data, p+i)::numeric * mult;
    mult := mult * 256::numeric;
  END LOOP;
  p := p + 8;
  val := trunc(acc);
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime".pb_skip_field(data bytea, pos int, wire_type int, field_num int) RETURNS int
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  n int;
  v numeric;
  newpos int := pos;
  depth int := 0;
  t numeric;
  fn int;
  wt int;
BEGIN
  IF wire_type = 0 THEN
    SELECT val, p FROM "transit_realtime".pb_read_varint(data, newpos) INTO v, newpos;
  ELSIF wire_type = 1 THEN
    newpos := newpos + 8;
  ELSIF wire_type = 5 THEN
    newpos := newpos + 4;
  ELSIF wire_type = 2 THEN
    SELECT val, p FROM "transit_realtime".pb_read_varint(data, newpos) INTO v, newpos;
    n := v::int;
    newpos := newpos + n;
  ELSIF wire_type = 3 THEN
    LOOP
      IF newpos >= octet_length(data) THEN
        EXIT;
      END IF;
      SELECT val, p FROM "transit_realtime".pb_read_varint(data, newpos) INTO t, newpos;
      fn := (t::bigint / 8)::int;
      wt := (t::numeric % 8)::int;
      IF wt = 4 AND fn = field_num THEN
        EXIT;
      END IF;
      newpos := "transit_realtime".pb_skip_field(data, newpos, wt, fn);
    END LOOP;
  ELSIF wire_type = 4 THEN
    NULL;
  END IF;
  IF newpos > octet_length(data) THEN
    newpos := octet_length(data);
  END IF;
  RETURN newpos;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime".pb_zigzag(v numeric) RETURNS numeric
LANGUAGE sql IMMUTABLE STRICT AS $$
  SELECT CASE WHEN (v::numeric % 2) = 0 THEN v/2 ELSE -((v+1)/2) END;
$$;


CREATE OR REPLACE FUNCTION "transit_realtime".pb_fixed32_to_float(v numeric) RETURNS numeric
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  bits bigint;
  sign int;
  exp int;
  mant numeric;
  res numeric;
BEGIN
  bits := trunc(v)::bigint;
  sign := (bits >> 31) & 1;
  exp := (bits >> 23) & 255;
  mant := bits & 8388607;
  IF exp = 0 THEN
    IF mant = 0 THEN
      res := 0;
    ELSE
      res := mant * (2::numeric ^ (-149));
    END IF;
  ELSIF exp = 255 THEN
    res := NULL;
  ELSE
    res := (1::numeric + mant / 8388608::numeric) * (2::numeric ^ (exp - 127));
  END IF;
  IF sign = 1 AND res IS NOT NULL THEN
    res := -res;
  END IF;
  RETURN res::double precision::numeric;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime".pb_fixed64_to_double(v numeric) RETURNS numeric
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  sign int;
  v2 numeric;
  exp int;
  mant numeric;
  res numeric;
BEGIN
  v := trunc(v);
  IF v >= (2::numeric ^ 63) THEN
    sign := 1;
    v2 := v - (2::numeric ^ 63);
  ELSE
    sign := 0;
    v2 := v;
  END IF;
  exp := (v2 / (2::numeric ^ 52))::int;
  mant := v2 - exp::numeric * (2::numeric ^ 52);
  IF exp = 0 THEN
    IF mant = 0 THEN
      res := 0;
    ELSE
      res := mant * (2::numeric ^ (-1074));
    END IF;
  ELSIF exp = 2047 THEN
    res := NULL;
  ELSE
    res := (1::numeric + mant / (2::numeric ^ 52)) * (2::numeric ^ (exp - 1023));
  END IF;
  IF sign = 1 AND res IS NOT NULL THEN
    res := -res;
  END IF;
  RETURN res::double precision::numeric;
END;
$$;


CREATE OR REPLACE FUNCTION "transit_realtime".pb_write_varint(buf bytea, val numeric) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  v numeric := val;
  res bytea := buf;
  b int;
BEGIN
  IF v < 0 THEN
    -- two's complement for signed: convert to unsigned 64-bit
    v := v + (2::numeric ^ 64);
  END IF;
  LOOP
    b := (v::numeric % 128)::int;
    v := trunc(v / 128);
    IF v > 0 THEN
      b := b | 128;
    END IF;
    res := res || decode(lpad(to_hex(b), 2, '0'), 'hex');
    EXIT WHEN v = 0;
  END LOOP;
  RETURN res;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime".pb_write_fixed32(buf bytea, val numeric) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  v bigint := (val::numeric)::bigint;
  res bytea := buf;
  i int;
BEGIN
  FOR i IN 0..3 LOOP
    res := res || decode(lpad(to_hex((v >> (i*8)) & 255), 2, '0'), 'hex');
  END LOOP;
  RETURN res;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime".pb_write_fixed64(buf bytea, val numeric) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  hi bigint;
  lo bigint;
  v numeric := val;
  res bytea := buf;
  i int;
BEGIN
  IF v < 0 THEN
    v := v + (2::numeric ^ 64);
  END IF;
  hi := (v / (2::numeric ^ 32))::bigint;
  lo := (v - hi::numeric * (2::numeric ^ 32))::bigint;
  FOR i IN 0..3 LOOP
    res := res || decode(lpad(to_hex((lo >> (i*8)) & 255), 2, '0'), 'hex');
  END LOOP;
  FOR i IN 0..3 LOOP
    res := res || decode(lpad(to_hex((hi >> (i*8)) & 255), 2, '0'), 'hex');
  END LOOP;
  RETURN res;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime".pb_write_tag(buf bytea, field_num int, wire_type int) RETURNS bytea
LANGUAGE sql IMMUTABLE STRICT AS $$
  SELECT "transit_realtime".pb_write_varint(buf, (field_num::numeric * 8 + wire_type));
$$;

CREATE OR REPLACE FUNCTION "transit_realtime".pb_write_string(buf bytea, s text) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  data bytea := convert_to(COALESCE(s,''), 'UTF8');
BEGIN
  RETURN "transit_realtime".pb_write_varint(buf, octet_length(data)::numeric) || data;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime".pb_write_bytes(buf bytea, data bytea) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
BEGIN
  data := COALESCE(data, ''::bytea);
  RETURN "transit_realtime".pb_write_varint(buf, octet_length(data)::numeric) || data;
END;
$$;


CREATE OR REPLACE FUNCTION "transit_realtime".pb_write_float4(buf bytea, val double precision) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  data bytea := float4send(val::float4);
  res bytea := buf;
  i int;
BEGIN
  FOR i IN REVERSE 3..0 LOOP
    res := res || substring(data from i+1 for 1);
  END LOOP;
  RETURN res;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime".pb_write_float8(buf bytea, val double precision) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  data bytea := float8send(val::float8);
  res bytea := buf;
  i int;
BEGIN
  FOR i IN REVERSE 7..0 LOOP
    res := res || substring(data from i+1 for 1);
  END LOOP;
  RETURN res;
END;
$$;


-- Empty
CREATE OR REPLACE FUNCTION "transit_realtime".decode_google_protobuf_Empty(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
BEGIN
  RETURN '{}'::jsonb;
END;
$$;

-- Wrappers
CREATE OR REPLACE FUNCTION "transit_realtime".decode_google_protobuf_BoolValue(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE result jsonb := 'null'::jsonb; pos int := 0; tag numeric; fn int; wt int; v numeric; n int; b bytea;
BEGIN
  IF data IS NULL OR octet_length(data)=0 THEN RETURN result; END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data,pos) INTO tag,pos;
    fn := (tag::bigint / 8)::int; wt := (tag::numeric % 8)::int;
    IF fn=1 AND wt=0 THEN
      SELECT val, p FROM "transit_realtime".pb_read_varint(data,pos) INTO v,pos;
      result := to_jsonb(v <> 0);
    ELSE
      pos := "transit_realtime".pb_skip_field(data,pos,wt,fn);
    END IF;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime".decode_google_protobuf_Int32Value(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE result jsonb := 'null'::jsonb; pos int := 0; tag numeric; fn int; wt int; v numeric; n int; b bytea;
BEGIN
  IF data IS NULL OR octet_length(data)=0 THEN RETURN result; END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data,pos) INTO tag,pos;
    fn := (tag::bigint / 8)::int; wt := (tag::numeric % 8)::int;
    IF fn=1 AND wt=0 THEN
      SELECT val, p FROM "transit_realtime".pb_read_varint(data,pos) INTO v,pos;
      result := to_jsonb(v);
    ELSE
      pos := "transit_realtime".pb_skip_field(data,pos,wt,fn);
    END IF;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime".decode_google_protobuf_Int64Value(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE result jsonb := 'null'::jsonb; pos int := 0; tag numeric; fn int; wt int; v numeric; n int; b bytea;
BEGIN
  IF data IS NULL OR octet_length(data)=0 THEN RETURN result; END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data,pos) INTO tag,pos;
    fn := (tag::bigint / 8)::int; wt := (tag::numeric % 8)::int;
    IF fn=1 AND wt=0 THEN
      SELECT val, p FROM "transit_realtime".pb_read_varint(data,pos) INTO v,pos;
      result := to_jsonb(v);
    ELSE
      pos := "transit_realtime".pb_skip_field(data,pos,wt,fn);
    END IF;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime".decode_google_protobuf_UInt32Value(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE result jsonb := 'null'::jsonb; pos int := 0; tag numeric; fn int; wt int; v numeric; n int; b bytea;
BEGIN
  IF data IS NULL OR octet_length(data)=0 THEN RETURN result; END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data,pos) INTO tag,pos;
    fn := (tag::bigint / 8)::int; wt := (tag::numeric % 8)::int;
    IF fn=1 AND wt=0 THEN
      SELECT val, p FROM "transit_realtime".pb_read_varint(data,pos) INTO v,pos;
      result := to_jsonb(v);
    ELSE
      pos := "transit_realtime".pb_skip_field(data,pos,wt,fn);
    END IF;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime".decode_google_protobuf_UInt64Value(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE result jsonb := 'null'::jsonb; pos int := 0; tag numeric; fn int; wt int; v numeric; n int; b bytea;
BEGIN
  IF data IS NULL OR octet_length(data)=0 THEN RETURN result; END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data,pos) INTO tag,pos;
    fn := (tag::bigint / 8)::int; wt := (tag::numeric % 8)::int;
    IF fn=1 AND wt=0 THEN
      SELECT val, p FROM "transit_realtime".pb_read_varint(data,pos) INTO v,pos;
      result := to_jsonb(v);
    ELSE
      pos := "transit_realtime".pb_skip_field(data,pos,wt,fn);
    END IF;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime".decode_google_protobuf_FloatValue(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE result jsonb := 'null'::jsonb; pos int := 0; tag numeric; fn int; wt int; v numeric; n int; b bytea;
BEGIN
  IF data IS NULL OR octet_length(data)=0 THEN RETURN result; END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data,pos) INTO tag,pos;
    fn := (tag::bigint / 8)::int; wt := (tag::numeric % 8)::int;
    IF fn=1 AND wt=5 THEN
      SELECT val, p FROM "transit_realtime".pb_read_fixed32(data,pos) INTO v,pos;
      v := "transit_realtime".pb_fixed32_to_float(v);
      result := to_jsonb(v);
    ELSE
      pos := "transit_realtime".pb_skip_field(data,pos,wt,fn);
    END IF;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime".decode_google_protobuf_DoubleValue(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE result jsonb := 'null'::jsonb; pos int := 0; tag numeric; fn int; wt int; v numeric; n int; b bytea;
BEGIN
  IF data IS NULL OR octet_length(data)=0 THEN RETURN result; END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data,pos) INTO tag,pos;
    fn := (tag::bigint / 8)::int; wt := (tag::numeric % 8)::int;
    IF fn=1 AND wt=1 THEN
      SELECT val, p FROM "transit_realtime".pb_read_fixed64(data,pos) INTO v,pos;
      v := "transit_realtime".pb_fixed64_to_double(v);
      result := to_jsonb(v);
    ELSE
      pos := "transit_realtime".pb_skip_field(data,pos,wt,fn);
    END IF;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime".decode_google_protobuf_StringValue(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE result jsonb := 'null'::jsonb; pos int := 0; tag numeric; fn int; wt int; v numeric; n int; b bytea; s text;
BEGIN
  IF data IS NULL OR octet_length(data)=0 THEN RETURN result; END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data,pos) INTO tag,pos;
    fn := (tag::bigint / 8)::int; wt := (tag::numeric % 8)::int;
    IF fn=1 AND wt=2 THEN
      SELECT val, p FROM "transit_realtime".pb_read_varint(data,pos) INTO n,pos;
      b := substring(data from pos+1 for n); pos := pos+n;
      s := convert_from(b,'UTF8');
      result := to_jsonb(s);
    ELSE
      pos := "transit_realtime".pb_skip_field(data,pos,wt,fn);
    END IF;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime".decode_google_protobuf_BytesValue(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE result jsonb := 'null'::jsonb; pos int := 0; tag numeric; fn int; wt int; v numeric; n int; b bytea; s text;
BEGIN
  IF data IS NULL OR octet_length(data)=0 THEN RETURN result; END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data,pos) INTO tag,pos;
    fn := (tag::bigint / 8)::int; wt := (tag::numeric % 8)::int;
    IF fn=1 AND wt=2 THEN
      SELECT val, p FROM "transit_realtime".pb_read_varint(data,pos) INTO n,pos;
      b := substring(data from pos+1 for n); pos := pos+n;
      s := encode(b,'base64');
      result := to_jsonb(s);
    ELSE
      pos := "transit_realtime".pb_skip_field(data,pos,wt,fn);
    END IF;
  END LOOP;
  RETURN result;
END;
$$;

-- Timestamp / Duration as objects
CREATE OR REPLACE FUNCTION "transit_realtime".decode_google_protobuf_Timestamp(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE result jsonb := '{}'::jsonb; pos int := 0; tag numeric; fn int; wt int; v numeric; n int; b bytea;
BEGIN
  IF data IS NULL OR octet_length(data)=0 THEN RETURN result; END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data,pos) INTO tag,pos;
    fn := (tag::bigint / 8)::int; wt := (tag::numeric % 8)::int;
    IF fn=1 AND wt=0 THEN
      SELECT val, p FROM "transit_realtime".pb_read_varint(data,pos) INTO v,pos;
      result := jsonb_set(result,'{seconds}',to_jsonb(v),true);
    ELSIF fn=2 AND wt=0 THEN
      SELECT val, p FROM "transit_realtime".pb_read_varint(data,pos) INTO v,pos;
      result := jsonb_set(result,'{nanos}',to_jsonb(v),true);
    ELSE
      pos := "transit_realtime".pb_skip_field(data,pos,wt,fn);
    END IF;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime".decode_google_protobuf_Duration(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE result jsonb := '{}'::jsonb; pos int := 0; tag numeric; fn int; wt int; v numeric; n int; b bytea;
BEGIN
  IF data IS NULL OR octet_length(data)=0 THEN RETURN result; END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data,pos) INTO tag,pos;
    fn := (tag::bigint / 8)::int; wt := (tag::numeric % 8)::int;
    IF fn=1 AND wt=0 THEN
      SELECT val, p FROM "transit_realtime".pb_read_varint(data,pos) INTO v,pos;
      result := jsonb_set(result,'{seconds}',to_jsonb(v),true);
    ELSIF fn=2 AND wt=0 THEN
      SELECT val, p FROM "transit_realtime".pb_read_varint(data,pos) INTO v,pos;
      result := jsonb_set(result,'{nanos}',to_jsonb(v),true);
    ELSE
      pos := "transit_realtime".pb_skip_field(data,pos,wt,fn);
    END IF;
  END LOOP;
  RETURN result;
END;
$$;

-- Any
CREATE OR REPLACE FUNCTION "transit_realtime".decode_google_protobuf_Any(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE result jsonb := '{}'::jsonb; pos int := 0; tag numeric; fn int; wt int; v numeric; n int; b bytea; s text;
BEGIN
  IF data IS NULL OR octet_length(data)=0 THEN RETURN result; END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data,pos) INTO tag,pos;
    fn := (tag::bigint / 8)::int; wt := (tag::numeric % 8)::int;
    IF fn=1 AND wt=2 THEN
      SELECT val, p FROM "transit_realtime".pb_read_varint(data,pos) INTO n,pos;
      b := substring(data from pos+1 for n); pos := pos+n;
      s := convert_from(b,'UTF8');
      result := jsonb_set(result,'{type_url}',to_jsonb(s),true);
    ELSIF fn=2 AND wt=2 THEN
      SELECT val, p FROM "transit_realtime".pb_read_varint(data,pos) INTO n,pos;
      b := substring(data from pos+1 for n); pos := pos+n;
      s := encode(b,'base64');
      result := jsonb_set(result,'{value}',to_jsonb(s),true);
    ELSE
      pos := "transit_realtime".pb_skip_field(data,pos,wt,fn);
    END IF;
  END LOOP;
  RETURN result;
END;
$$;

-- FieldMask (paths repeated string)
CREATE OR REPLACE FUNCTION "transit_realtime".decode_google_protobuf_FieldMask(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE result jsonb := '[]'::jsonb; pos int := 0; tag numeric; fn int; wt int; v numeric; n int; b bytea; s text;
BEGIN
  IF data IS NULL OR octet_length(data)=0 THEN RETURN result; END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data,pos) INTO tag,pos;
    fn := (tag::bigint / 8)::int; wt := (tag::numeric % 8)::int;
    IF fn=1 AND wt=2 THEN
      SELECT val, p FROM "transit_realtime".pb_read_varint(data,pos) INTO n,pos;
      b := substring(data from pos+1 for n); pos := pos+n;
      s := convert_from(b,'UTF8');
      result := result || jsonb_build_array(s);
    ELSE
      pos := "transit_realtime".pb_skip_field(data,pos,wt,fn);
    END IF;
  END LOOP;
  RETURN result;
END;
$$;

-- Value native decode
CREATE OR REPLACE FUNCTION "transit_realtime".pb_decode_value(submsg jsonb) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
BEGIN
  -- submsg is already decoded as object with one of the oneof fields
  IF submsg IS NULL THEN RETURN 'null'::jsonb; END IF;
  IF submsg ? 'null_value' THEN RETURN 'null'::jsonb; END IF;
  IF submsg ? 'number_value' THEN RETURN submsg->'number_value'; END IF;
  IF submsg ? 'string_value' THEN RETURN submsg->'string_value'; END IF;
  IF submsg ? 'bool_value' THEN RETURN submsg->'bool_value'; END IF;
  IF submsg ? 'struct_value' THEN RETURN "transit_realtime".pb_decode_struct(submsg->'struct_value'); END IF;
  IF submsg ? 'list_value' THEN RETURN "transit_realtime".pb_decode_listvalue(submsg->'list_value'); END IF;
  RETURN submsg;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime".pb_decode_listvalue(submsg jsonb) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE res jsonb := '[]'::jsonb; elem jsonb;
BEGIN
  -- submsg is decoded ListValue object with 'values' array of Value messages (already decoded to objects)
  IF submsg ? 'values' THEN
    FOR elem IN SELECT jsonb_array_elements(submsg->'values') LOOP
      res := res || jsonb_build_array("transit_realtime".pb_decode_value(elem));
    END LOOP;
  END IF;
  RETURN res;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime".pb_decode_struct(submsg jsonb) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE res jsonb := '{}'::jsonb; k text; v jsonb;
BEGIN
  -- submsg is decoded Struct object with 'fields' map of string->Value (decoded as object with key/value)
  -- Our map decode stores fields as object: {"fields": {"key": <Value decoded as object>}}
  IF submsg ? 'fields' AND jsonb_typeof(submsg->'fields') = 'object' THEN
    FOR k, v IN SELECT key, value FROM jsonb_each(submsg->'fields') LOOP
      res := jsonb_set(res, ARRAY[k], "transit_realtime".pb_decode_value(v), true);
    END LOOP;
  END IF;
  RETURN res;
END;
$$;

-- Value message decoder (produces intermediate object; pb_decode_value converts)
CREATE OR REPLACE FUNCTION "transit_realtime".decode_google_protobuf_Value(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE result jsonb := '{}'::jsonb; pos int := 0; tag numeric; fn int; wt int; v numeric; n int; b bytea; s text; submsg jsonb;
BEGIN
  IF data IS NULL OR octet_length(data)=0 THEN RETURN result; END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data,pos) INTO tag,pos;
    fn := (tag::bigint / 8)::int; wt := (tag::numeric % 8)::int;
    IF fn=1 AND wt=0 THEN
      -- null_value (enum) -> store marker
      SELECT val, p FROM "transit_realtime".pb_read_varint(data,pos) INTO v,pos;
      result := jsonb_set(result,'{null_value}',to_jsonb(v),true);
    ELSIF fn=2 AND wt=1 THEN
      SELECT val, p FROM "transit_realtime".pb_read_fixed64(data,pos) INTO v,pos;
      v := "transit_realtime".pb_fixed64_to_double(v);
      result := jsonb_set(result,'{number_value}',to_jsonb(v),true);
    ELSIF fn=3 AND wt=2 THEN
      SELECT val, p FROM "transit_realtime".pb_read_varint(data,pos) INTO n,pos;
      b := substring(data from pos+1 for n); pos := pos+n;
      s := convert_from(b,'UTF8');
      result := jsonb_set(result,'{string_value}',to_jsonb(s),true);
    ELSIF fn=4 AND wt=0 THEN
      SELECT val, p FROM "transit_realtime".pb_read_varint(data,pos) INTO v,pos;
      result := jsonb_set(result,'{bool_value}',to_jsonb(v <> 0),true);
    ELSIF fn=5 AND wt=2 THEN
      SELECT val, p FROM "transit_realtime".pb_read_varint(data,pos) INTO n,pos;
      b := substring(data from pos+1 for n); pos := pos+n;
      submsg := "transit_realtime".decode_google_protobuf_Struct(b);
      result := jsonb_set(result,'{struct_value}',submsg,true);
    ELSIF fn=6 AND wt=2 THEN
      SELECT val, p FROM "transit_realtime".pb_read_varint(data,pos) INTO n,pos;
      b := substring(data from pos+1 for n); pos := pos+n;
      submsg := "transit_realtime".decode_google_protobuf_ListValue(b);
      result := jsonb_set(result,'{list_value}',submsg,true);
    ELSE
      pos := "transit_realtime".pb_skip_field(data,pos,wt,fn);
    END IF;
  END LOOP;
  RETURN result;
END;
$$;

-- ListValue message decoder
CREATE OR REPLACE FUNCTION "transit_realtime".decode_google_protobuf_ListValue(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE result jsonb := '{}'::jsonb; pos int := 0; tag numeric; fn int; wt int; v numeric; n int; b bytea; submsg jsonb;
BEGIN
  IF data IS NULL OR octet_length(data)=0 THEN
    result := jsonb_set(result,'{values}', '[]'::jsonb, true);
    RETURN result;
  END IF;
  result := jsonb_set(result,'{values}', '[]'::jsonb, true);
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data,pos) INTO tag,pos;
    fn := (tag::bigint / 8)::int; wt := (tag::numeric % 8)::int;
    IF fn=1 AND wt=2 THEN
      SELECT val, p FROM "transit_realtime".pb_read_varint(data,pos) INTO n,pos;
      b := substring(data from pos+1 for n); pos := pos+n;
      submsg := "transit_realtime".decode_google_protobuf_Value(b);
      result := jsonb_set(result,'{values}', (result->'values') || jsonb_build_array(submsg), true);
    ELSE
      pos := "transit_realtime".pb_skip_field(data,pos,wt,fn);
    END IF;
  END LOOP;
  RETURN result;
END;
$$;

-- Struct message decoder
CREATE OR REPLACE FUNCTION "transit_realtime".decode_google_protobuf_Struct(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE result jsonb := '{}'::jsonb; pos int := 0; tag numeric; fn int; wt int; v numeric; n int; b bytea; submsg jsonb; keytext text;
  fields jsonb := '{}'::jsonb;
BEGIN
  IF data IS NULL OR octet_length(data)=0 THEN
    result := jsonb_set(result,'{fields}', fields, true);
    RETURN result;
  END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data,pos) INTO tag,pos;
    fn := (tag::bigint / 8)::int; wt := (tag::numeric % 8)::int;
    IF fn=1 AND wt=2 THEN
      SELECT val, p FROM "transit_realtime".pb_read_varint(data,pos) INTO n,pos;
      b := substring(data from pos+1 for n); pos := pos+n;
      submsg := "transit_realtime".decode_google_protobuf_Struct_FieldsEntry(b);
      keytext := COALESCE(submsg->>'key','');
      IF keytext <> '' THEN
        fields := jsonb_set(fields, ARRAY[keytext], "transit_realtime".pb_decode_value(COALESCE(submsg->'value','{}'::jsonb)), true);
      END IF;
    ELSE
      pos := "transit_realtime".pb_skip_field(data,pos,wt,fn);
    END IF;
  END LOOP;
  result := jsonb_set(result,'{fields}', fields, true);
  RETURN result;
END;
$$;

-- Map entry for Struct fields: fields map<string, Value>
CREATE OR REPLACE FUNCTION "transit_realtime".decode_google_protobuf_Struct_FieldsEntry(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE result jsonb := '{}'::jsonb; pos int := 0; tag numeric; fn int; wt int; v numeric; n int; b bytea; s text; submsg jsonb;
BEGIN
  IF data IS NULL OR octet_length(data)=0 THEN RETURN result; END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data,pos) INTO tag,pos;
    fn := (tag::bigint / 8)::int; wt := (tag::numeric % 8)::int;
    IF fn=1 AND wt=2 THEN
      SELECT val, p FROM "transit_realtime".pb_read_varint(data,pos) INTO n,pos;
      b := substring(data from pos+1 for n); pos := pos+n;
      s := convert_from(b,'UTF8');
      result := jsonb_set(result,'{key}',to_jsonb(s),true);
    ELSIF fn=2 AND wt=2 THEN
      SELECT val, p FROM "transit_realtime".pb_read_varint(data,pos) INTO n,pos;
      b := substring(data from pos+1 for n); pos := pos+n;
      submsg := "transit_realtime".decode_google_protobuf_Value(b);
      result := jsonb_set(result,'{value}',submsg,true);
    ELSE
      pos := "transit_realtime".pb_skip_field(data,pos,wt,fn);
    END IF;
  END LOOP;
  RETURN result;
END;
$$;


CREATE OR REPLACE FUNCTION "transit_realtime"."decode_feedheader"(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  result jsonb := '{}'::jsonb;
  pos int := 0;
  tag numeric;
  field_num int;
  wire_type int;
  n int;
  v numeric;
  b bytea;
  s text;
  submsg jsonb;
  endpos int;
  mapobj jsonb;
  keytext text;
BEGIN
  IF data IS NULL OR octet_length(data) = 0 THEN
    RETURN result;
  END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO tag, pos;
    field_num := (tag::bigint / 8)::int;
    wire_type := (tag::numeric % 8)::int;
    CASE field_num
      WHEN 1 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{gtfs_realtime_version}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 2 THEN
        IF wire_type = 0 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO v, pos;
          s := CASE v::int WHEN 0 THEN 'FULL_DATASET' WHEN 1 THEN 'DIFFERENTIAL' ELSE v::text END;
          result := jsonb_set(result, '{incrementality}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 3 THEN
        IF wire_type = 0 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO v, pos;
          result := jsonb_set(result, '{timestamp}', to_jsonb(v), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 4 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{feed_version}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      ELSE
        pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
    END CASE;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."decode_tripupdate_stoptimeevent"(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  result jsonb := '{}'::jsonb;
  pos int := 0;
  tag numeric;
  field_num int;
  wire_type int;
  n int;
  v numeric;
  b bytea;
  s text;
  submsg jsonb;
  endpos int;
  mapobj jsonb;
  keytext text;
BEGIN
  IF data IS NULL OR octet_length(data) = 0 THEN
    RETURN result;
  END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO tag, pos;
    field_num := (tag::bigint / 8)::int;
    wire_type := (tag::numeric % 8)::int;
    CASE field_num
      WHEN 1 THEN
        IF wire_type = 0 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO v, pos;
          result := jsonb_set(result, '{delay}', to_jsonb(v), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 2 THEN
        IF wire_type = 0 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO v, pos;
          result := jsonb_set(result, '{time}', to_jsonb(v), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 3 THEN
        IF wire_type = 0 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO v, pos;
          result := jsonb_set(result, '{uncertainty}', to_jsonb(v), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 4 THEN
        IF wire_type = 0 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO v, pos;
          result := jsonb_set(result, '{scheduled_time}', to_jsonb(v), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      ELSE
        pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
    END CASE;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."decode_tripupdate_stoptimeupdate_stoptimeproperties"(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  result jsonb := '{}'::jsonb;
  pos int := 0;
  tag numeric;
  field_num int;
  wire_type int;
  n int;
  v numeric;
  b bytea;
  s text;
  submsg jsonb;
  endpos int;
  mapobj jsonb;
  keytext text;
BEGIN
  IF data IS NULL OR octet_length(data) = 0 THEN
    RETURN result;
  END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO tag, pos;
    field_num := (tag::bigint / 8)::int;
    wire_type := (tag::numeric % 8)::int;
    CASE field_num
      WHEN 1 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{assigned_stop_id}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 2 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{stop_headsign}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 3 THEN
        IF wire_type = 0 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO v, pos;
          s := CASE v::int WHEN 0 THEN 'REGULAR' WHEN 1 THEN 'NONE' WHEN 2 THEN 'PHONE_AGENCY' WHEN 3 THEN 'COORDINATE_WITH_DRIVER' ELSE v::text END;
          result := jsonb_set(result, '{pickup_type}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 4 THEN
        IF wire_type = 0 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO v, pos;
          s := CASE v::int WHEN 0 THEN 'REGULAR' WHEN 1 THEN 'NONE' WHEN 2 THEN 'PHONE_AGENCY' WHEN 3 THEN 'COORDINATE_WITH_DRIVER' ELSE v::text END;
          result := jsonb_set(result, '{drop_off_type}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      ELSE
        pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
    END CASE;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."decode_tripupdate_stoptimeupdate"(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  result jsonb := '{}'::jsonb;
  pos int := 0;
  tag numeric;
  field_num int;
  wire_type int;
  n int;
  v numeric;
  b bytea;
  s text;
  submsg jsonb;
  endpos int;
  mapobj jsonb;
  keytext text;
BEGIN
  IF data IS NULL OR octet_length(data) = 0 THEN
    RETURN result;
  END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO tag, pos;
    field_num := (tag::bigint / 8)::int;
    wire_type := (tag::numeric % 8)::int;
    CASE field_num
      WHEN 1 THEN
        IF wire_type = 0 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO v, pos;
          result := jsonb_set(result, '{stop_sequence}', to_jsonb(v), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 2 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_tripupdate_stoptimeevent"(b);
          result := jsonb_set(result, '{arrival}', submsg, true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 3 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_tripupdate_stoptimeevent"(b);
          result := jsonb_set(result, '{departure}', submsg, true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 4 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{stop_id}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 5 THEN
        IF wire_type = 0 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO v, pos;
          s := CASE v::int WHEN 0 THEN 'SCHEDULED' WHEN 1 THEN 'SKIPPED' WHEN 2 THEN 'NO_DATA' WHEN 3 THEN 'UNSCHEDULED' ELSE v::text END;
          result := jsonb_set(result, '{schedule_relationship}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 6 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_tripupdate_stoptimeupdate_stoptimeproperties"(b);
          result := jsonb_set(result, '{stop_time_properties}', submsg, true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 7 THEN
        IF wire_type = 0 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO v, pos;
          s := CASE v::int WHEN 0 THEN 'EMPTY' WHEN 1 THEN 'MANY_SEATS_AVAILABLE' WHEN 2 THEN 'FEW_SEATS_AVAILABLE' WHEN 3 THEN 'STANDING_ROOM_ONLY' WHEN 4 THEN 'CRUSHED_STANDING_ROOM_ONLY' WHEN 5 THEN 'FULL' WHEN 6 THEN 'NOT_ACCEPTING_PASSENGERS' WHEN 7 THEN 'NO_DATA_AVAILABLE' WHEN 8 THEN 'NOT_BOARDABLE' ELSE v::text END;
          result := jsonb_set(result, '{departure_occupancy_status}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      ELSE
        pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
    END CASE;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."decode_tripupdate_tripproperties"(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  result jsonb := '{}'::jsonb;
  pos int := 0;
  tag numeric;
  field_num int;
  wire_type int;
  n int;
  v numeric;
  b bytea;
  s text;
  submsg jsonb;
  endpos int;
  mapobj jsonb;
  keytext text;
BEGIN
  IF data IS NULL OR octet_length(data) = 0 THEN
    RETURN result;
  END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO tag, pos;
    field_num := (tag::bigint / 8)::int;
    wire_type := (tag::numeric % 8)::int;
    CASE field_num
      WHEN 1 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{trip_id}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 2 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{start_date}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 3 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{start_time}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 4 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{shape_id}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 5 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{trip_headsign}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 6 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{trip_short_name}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      ELSE
        pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
    END CASE;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."decode_tripdescriptor_modifiedtripselector"(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  result jsonb := '{}'::jsonb;
  pos int := 0;
  tag numeric;
  field_num int;
  wire_type int;
  n int;
  v numeric;
  b bytea;
  s text;
  submsg jsonb;
  endpos int;
  mapobj jsonb;
  keytext text;
BEGIN
  IF data IS NULL OR octet_length(data) = 0 THEN
    RETURN result;
  END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO tag, pos;
    field_num := (tag::bigint / 8)::int;
    wire_type := (tag::numeric % 8)::int;
    CASE field_num
      WHEN 1 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{modifications_id}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 2 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{affected_trip_id}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 3 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{start_time}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 4 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{start_date}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      ELSE
        pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
    END CASE;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."decode_tripdescriptor"(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  result jsonb := '{}'::jsonb;
  pos int := 0;
  tag numeric;
  field_num int;
  wire_type int;
  n int;
  v numeric;
  b bytea;
  s text;
  submsg jsonb;
  endpos int;
  mapobj jsonb;
  keytext text;
BEGIN
  IF data IS NULL OR octet_length(data) = 0 THEN
    RETURN result;
  END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO tag, pos;
    field_num := (tag::bigint / 8)::int;
    wire_type := (tag::numeric % 8)::int;
    CASE field_num
      WHEN 1 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{trip_id}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 2 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{start_time}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 3 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{start_date}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 4 THEN
        IF wire_type = 0 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO v, pos;
          s := CASE v::int WHEN 0 THEN 'SCHEDULED' WHEN 1 THEN 'ADDED' WHEN 2 THEN 'UNSCHEDULED' WHEN 3 THEN 'CANCELED' WHEN 5 THEN 'REPLACEMENT' WHEN 6 THEN 'DUPLICATED' WHEN 7 THEN 'DELETED' WHEN 8 THEN 'NEW' ELSE v::text END;
          result := jsonb_set(result, '{schedule_relationship}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 5 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{route_id}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 6 THEN
        IF wire_type = 0 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO v, pos;
          result := jsonb_set(result, '{direction_id}', to_jsonb(v), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 7 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_tripdescriptor_modifiedtripselector"(b);
          result := jsonb_set(result, '{modified_trip}', submsg, true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      ELSE
        pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
    END CASE;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."decode_vehicledescriptor"(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  result jsonb := '{}'::jsonb;
  pos int := 0;
  tag numeric;
  field_num int;
  wire_type int;
  n int;
  v numeric;
  b bytea;
  s text;
  submsg jsonb;
  endpos int;
  mapobj jsonb;
  keytext text;
BEGIN
  IF data IS NULL OR octet_length(data) = 0 THEN
    RETURN result;
  END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO tag, pos;
    field_num := (tag::bigint / 8)::int;
    wire_type := (tag::numeric % 8)::int;
    CASE field_num
      WHEN 1 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{id}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 2 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{label}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 3 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{license_plate}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 4 THEN
        IF wire_type = 0 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO v, pos;
          s := CASE v::int WHEN 0 THEN 'NO_VALUE' WHEN 1 THEN 'UNKNOWN' WHEN 2 THEN 'WHEELCHAIR_ACCESSIBLE' WHEN 3 THEN 'WHEELCHAIR_INACCESSIBLE' ELSE v::text END;
          result := jsonb_set(result, '{wheelchair_accessible}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      ELSE
        pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
    END CASE;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."decode_tripupdate"(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  result jsonb := '{}'::jsonb;
  pos int := 0;
  tag numeric;
  field_num int;
  wire_type int;
  n int;
  v numeric;
  b bytea;
  s text;
  submsg jsonb;
  endpos int;
  mapobj jsonb;
  keytext text;
BEGIN
  IF data IS NULL OR octet_length(data) = 0 THEN
    RETURN result;
  END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO tag, pos;
    field_num := (tag::bigint / 8)::int;
    wire_type := (tag::numeric % 8)::int;
    CASE field_num
      WHEN 1 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_tripdescriptor"(b);
          result := jsonb_set(result, '{trip}', submsg, true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 2 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_tripupdate_stoptimeupdate"(b);
          IF result ? 'stop_time_update' THEN
            result := jsonb_set(result, '{stop_time_update}', (result->'stop_time_update') || jsonb_build_array(submsg));
          ELSE
            result := jsonb_set(result, '{stop_time_update}', jsonb_build_array(submsg));
          END IF;
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 3 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_vehicledescriptor"(b);
          result := jsonb_set(result, '{vehicle}', submsg, true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 4 THEN
        IF wire_type = 0 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO v, pos;
          result := jsonb_set(result, '{timestamp}', to_jsonb(v), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 5 THEN
        IF wire_type = 0 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO v, pos;
          result := jsonb_set(result, '{delay}', to_jsonb(v), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 6 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_tripupdate_tripproperties"(b);
          result := jsonb_set(result, '{trip_properties}', submsg, true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      ELSE
        pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
    END CASE;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."decode_vehicleposition_carriagedetails"(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  result jsonb := '{}'::jsonb;
  pos int := 0;
  tag numeric;
  field_num int;
  wire_type int;
  n int;
  v numeric;
  b bytea;
  s text;
  submsg jsonb;
  endpos int;
  mapobj jsonb;
  keytext text;
BEGIN
  IF data IS NULL OR octet_length(data) = 0 THEN
    RETURN result;
  END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO tag, pos;
    field_num := (tag::bigint / 8)::int;
    wire_type := (tag::numeric % 8)::int;
    CASE field_num
      WHEN 1 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{id}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 2 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{label}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 3 THEN
        IF wire_type = 0 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO v, pos;
          s := CASE v::int WHEN 0 THEN 'EMPTY' WHEN 1 THEN 'MANY_SEATS_AVAILABLE' WHEN 2 THEN 'FEW_SEATS_AVAILABLE' WHEN 3 THEN 'STANDING_ROOM_ONLY' WHEN 4 THEN 'CRUSHED_STANDING_ROOM_ONLY' WHEN 5 THEN 'FULL' WHEN 6 THEN 'NOT_ACCEPTING_PASSENGERS' WHEN 7 THEN 'NO_DATA_AVAILABLE' WHEN 8 THEN 'NOT_BOARDABLE' ELSE v::text END;
          result := jsonb_set(result, '{occupancy_status}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 4 THEN
        IF wire_type = 0 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO v, pos;
          result := jsonb_set(result, '{occupancy_percentage}', to_jsonb(v), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 5 THEN
        IF wire_type = 0 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO v, pos;
          result := jsonb_set(result, '{carriage_sequence}', to_jsonb(v), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      ELSE
        pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
    END CASE;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."decode_position"(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  result jsonb := '{}'::jsonb;
  pos int := 0;
  tag numeric;
  field_num int;
  wire_type int;
  n int;
  v numeric;
  b bytea;
  s text;
  submsg jsonb;
  endpos int;
  mapobj jsonb;
  keytext text;
BEGIN
  IF data IS NULL OR octet_length(data) = 0 THEN
    RETURN result;
  END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO tag, pos;
    field_num := (tag::bigint / 8)::int;
    wire_type := (tag::numeric % 8)::int;
    CASE field_num
      WHEN 1 THEN
        IF wire_type = 5 THEN
          SELECT val, p FROM "transit_realtime".pb_read_fixed32(data, pos) INTO v, pos;
          v := "transit_realtime".pb_fixed32_to_float(v);
          result := jsonb_set(result, '{latitude}', to_jsonb(v), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 2 THEN
        IF wire_type = 5 THEN
          SELECT val, p FROM "transit_realtime".pb_read_fixed32(data, pos) INTO v, pos;
          v := "transit_realtime".pb_fixed32_to_float(v);
          result := jsonb_set(result, '{longitude}', to_jsonb(v), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 3 THEN
        IF wire_type = 5 THEN
          SELECT val, p FROM "transit_realtime".pb_read_fixed32(data, pos) INTO v, pos;
          v := "transit_realtime".pb_fixed32_to_float(v);
          result := jsonb_set(result, '{bearing}', to_jsonb(v), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 4 THEN
        IF wire_type = 1 THEN
          SELECT val, p FROM "transit_realtime".pb_read_fixed64(data, pos) INTO v, pos;
          v := "transit_realtime".pb_fixed64_to_double(v);
          result := jsonb_set(result, '{odometer}', to_jsonb(v), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 5 THEN
        IF wire_type = 5 THEN
          SELECT val, p FROM "transit_realtime".pb_read_fixed32(data, pos) INTO v, pos;
          v := "transit_realtime".pb_fixed32_to_float(v);
          result := jsonb_set(result, '{speed}', to_jsonb(v), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      ELSE
        pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
    END CASE;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."decode_vehicleposition"(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  result jsonb := '{}'::jsonb;
  pos int := 0;
  tag numeric;
  field_num int;
  wire_type int;
  n int;
  v numeric;
  b bytea;
  s text;
  submsg jsonb;
  endpos int;
  mapobj jsonb;
  keytext text;
BEGIN
  IF data IS NULL OR octet_length(data) = 0 THEN
    RETURN result;
  END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO tag, pos;
    field_num := (tag::bigint / 8)::int;
    wire_type := (tag::numeric % 8)::int;
    CASE field_num
      WHEN 1 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_tripdescriptor"(b);
          result := jsonb_set(result, '{trip}', submsg, true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 2 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_position"(b);
          result := jsonb_set(result, '{position}', submsg, true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 3 THEN
        IF wire_type = 0 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO v, pos;
          result := jsonb_set(result, '{current_stop_sequence}', to_jsonb(v), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 4 THEN
        IF wire_type = 0 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO v, pos;
          s := CASE v::int WHEN 0 THEN 'INCOMING_AT' WHEN 1 THEN 'STOPPED_AT' WHEN 2 THEN 'IN_TRANSIT_TO' ELSE v::text END;
          result := jsonb_set(result, '{current_status}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 5 THEN
        IF wire_type = 0 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO v, pos;
          result := jsonb_set(result, '{timestamp}', to_jsonb(v), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 6 THEN
        IF wire_type = 0 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO v, pos;
          s := CASE v::int WHEN 0 THEN 'UNKNOWN_CONGESTION_LEVEL' WHEN 1 THEN 'RUNNING_SMOOTHLY' WHEN 2 THEN 'STOP_AND_GO' WHEN 3 THEN 'CONGESTION' WHEN 4 THEN 'SEVERE_CONGESTION' ELSE v::text END;
          result := jsonb_set(result, '{congestion_level}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 7 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{stop_id}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 8 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_vehicledescriptor"(b);
          result := jsonb_set(result, '{vehicle}', submsg, true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 9 THEN
        IF wire_type = 0 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO v, pos;
          s := CASE v::int WHEN 0 THEN 'EMPTY' WHEN 1 THEN 'MANY_SEATS_AVAILABLE' WHEN 2 THEN 'FEW_SEATS_AVAILABLE' WHEN 3 THEN 'STANDING_ROOM_ONLY' WHEN 4 THEN 'CRUSHED_STANDING_ROOM_ONLY' WHEN 5 THEN 'FULL' WHEN 6 THEN 'NOT_ACCEPTING_PASSENGERS' WHEN 7 THEN 'NO_DATA_AVAILABLE' WHEN 8 THEN 'NOT_BOARDABLE' ELSE v::text END;
          result := jsonb_set(result, '{occupancy_status}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 10 THEN
        IF wire_type = 0 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO v, pos;
          result := jsonb_set(result, '{occupancy_percentage}', to_jsonb(v), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 11 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_vehicleposition_carriagedetails"(b);
          IF result ? 'multi_carriage_details' THEN
            result := jsonb_set(result, '{multi_carriage_details}', (result->'multi_carriage_details') || jsonb_build_array(submsg));
          ELSE
            result := jsonb_set(result, '{multi_carriage_details}', jsonb_build_array(submsg));
          END IF;
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      ELSE
        pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
    END CASE;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."decode_timerange"(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  result jsonb := '{}'::jsonb;
  pos int := 0;
  tag numeric;
  field_num int;
  wire_type int;
  n int;
  v numeric;
  b bytea;
  s text;
  submsg jsonb;
  endpos int;
  mapobj jsonb;
  keytext text;
BEGIN
  IF data IS NULL OR octet_length(data) = 0 THEN
    RETURN result;
  END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO tag, pos;
    field_num := (tag::bigint / 8)::int;
    wire_type := (tag::numeric % 8)::int;
    CASE field_num
      WHEN 1 THEN
        IF wire_type = 0 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO v, pos;
          result := jsonb_set(result, '{start}', to_jsonb(v), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 2 THEN
        IF wire_type = 0 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO v, pos;
          result := jsonb_set(result, '{end}', to_jsonb(v), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      ELSE
        pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
    END CASE;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."decode_entityselector"(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  result jsonb := '{}'::jsonb;
  pos int := 0;
  tag numeric;
  field_num int;
  wire_type int;
  n int;
  v numeric;
  b bytea;
  s text;
  submsg jsonb;
  endpos int;
  mapobj jsonb;
  keytext text;
BEGIN
  IF data IS NULL OR octet_length(data) = 0 THEN
    RETURN result;
  END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO tag, pos;
    field_num := (tag::bigint / 8)::int;
    wire_type := (tag::numeric % 8)::int;
    CASE field_num
      WHEN 1 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{agency_id}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 2 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{route_id}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 3 THEN
        IF wire_type = 0 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO v, pos;
          result := jsonb_set(result, '{route_type}', to_jsonb(v), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 4 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_tripdescriptor"(b);
          result := jsonb_set(result, '{trip}', submsg, true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 5 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{stop_id}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 6 THEN
        IF wire_type = 0 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO v, pos;
          result := jsonb_set(result, '{direction_id}', to_jsonb(v), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      ELSE
        pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
    END CASE;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."decode_translatedstring_translation"(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  result jsonb := '{}'::jsonb;
  pos int := 0;
  tag numeric;
  field_num int;
  wire_type int;
  n int;
  v numeric;
  b bytea;
  s text;
  submsg jsonb;
  endpos int;
  mapobj jsonb;
  keytext text;
BEGIN
  IF data IS NULL OR octet_length(data) = 0 THEN
    RETURN result;
  END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO tag, pos;
    field_num := (tag::bigint / 8)::int;
    wire_type := (tag::numeric % 8)::int;
    CASE field_num
      WHEN 1 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{text}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 2 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{language}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      ELSE
        pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
    END CASE;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."decode_translatedstring"(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  result jsonb := '{}'::jsonb;
  pos int := 0;
  tag numeric;
  field_num int;
  wire_type int;
  n int;
  v numeric;
  b bytea;
  s text;
  submsg jsonb;
  endpos int;
  mapobj jsonb;
  keytext text;
BEGIN
  IF data IS NULL OR octet_length(data) = 0 THEN
    RETURN result;
  END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO tag, pos;
    field_num := (tag::bigint / 8)::int;
    wire_type := (tag::numeric % 8)::int;
    CASE field_num
      WHEN 1 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_translatedstring_translation"(b);
          IF result ? 'translation' THEN
            result := jsonb_set(result, '{translation}', (result->'translation') || jsonb_build_array(submsg));
          ELSE
            result := jsonb_set(result, '{translation}', jsonb_build_array(submsg));
          END IF;
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      ELSE
        pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
    END CASE;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."decode_alert"(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  result jsonb := '{}'::jsonb;
  pos int := 0;
  tag numeric;
  field_num int;
  wire_type int;
  n int;
  v numeric;
  b bytea;
  s text;
  submsg jsonb;
  endpos int;
  mapobj jsonb;
  keytext text;
BEGIN
  IF data IS NULL OR octet_length(data) = 0 THEN
    RETURN result;
  END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO tag, pos;
    field_num := (tag::bigint / 8)::int;
    wire_type := (tag::numeric % 8)::int;
    CASE field_num
      WHEN 1 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_timerange"(b);
          IF result ? 'active_period' THEN
            result := jsonb_set(result, '{active_period}', (result->'active_period') || jsonb_build_array(submsg));
          ELSE
            result := jsonb_set(result, '{active_period}', jsonb_build_array(submsg));
          END IF;
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 2 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_timerange"(b);
          IF result ? 'communication_period' THEN
            result := jsonb_set(result, '{communication_period}', (result->'communication_period') || jsonb_build_array(submsg));
          ELSE
            result := jsonb_set(result, '{communication_period}', jsonb_build_array(submsg));
          END IF;
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 3 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_timerange"(b);
          IF result ? 'impact_period' THEN
            result := jsonb_set(result, '{impact_period}', (result->'impact_period') || jsonb_build_array(submsg));
          ELSE
            result := jsonb_set(result, '{impact_period}', jsonb_build_array(submsg));
          END IF;
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 5 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_entityselector"(b);
          IF result ? 'informed_entity' THEN
            result := jsonb_set(result, '{informed_entity}', (result->'informed_entity') || jsonb_build_array(submsg));
          ELSE
            result := jsonb_set(result, '{informed_entity}', jsonb_build_array(submsg));
          END IF;
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 6 THEN
        IF wire_type = 0 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO v, pos;
          s := CASE v::int WHEN 1 THEN 'UNKNOWN_CAUSE' WHEN 2 THEN 'OTHER_CAUSE' WHEN 3 THEN 'TECHNICAL_PROBLEM' WHEN 4 THEN 'STRIKE' WHEN 5 THEN 'DEMONSTRATION' WHEN 6 THEN 'ACCIDENT' WHEN 7 THEN 'HOLIDAY' WHEN 8 THEN 'WEATHER' WHEN 9 THEN 'MAINTENANCE' WHEN 10 THEN 'CONSTRUCTION' WHEN 11 THEN 'POLICE_ACTIVITY' WHEN 12 THEN 'MEDICAL_EMERGENCY' WHEN 13 THEN 'SPECIAL_EVENT' ELSE v::text END;
          result := jsonb_set(result, '{cause}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 7 THEN
        IF wire_type = 0 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO v, pos;
          s := CASE v::int WHEN 1 THEN 'NO_SERVICE' WHEN 2 THEN 'REDUCED_SERVICE' WHEN 3 THEN 'SIGNIFICANT_DELAYS' WHEN 4 THEN 'DETOUR' WHEN 5 THEN 'ADDITIONAL_SERVICE' WHEN 6 THEN 'MODIFIED_SERVICE' WHEN 7 THEN 'OTHER_EFFECT' WHEN 8 THEN 'UNKNOWN_EFFECT' WHEN 9 THEN 'STOP_MOVED' WHEN 10 THEN 'NO_EFFECT' WHEN 11 THEN 'ACCESSIBILITY_ISSUE' ELSE v::text END;
          result := jsonb_set(result, '{effect}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 8 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_translatedstring"(b);
          result := jsonb_set(result, '{url}', submsg, true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 10 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_translatedstring"(b);
          result := jsonb_set(result, '{header_text}', submsg, true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 11 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_translatedstring"(b);
          result := jsonb_set(result, '{description_text}', submsg, true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 12 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_translatedstring"(b);
          result := jsonb_set(result, '{tts_header_text}', submsg, true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 13 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_translatedstring"(b);
          result := jsonb_set(result, '{tts_description_text}', submsg, true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 14 THEN
        IF wire_type = 0 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO v, pos;
          s := CASE v::int WHEN 1 THEN 'UNKNOWN_SEVERITY' WHEN 2 THEN 'INFO' WHEN 3 THEN 'WARNING' WHEN 4 THEN 'SEVERE' ELSE v::text END;
          result := jsonb_set(result, '{severity_level}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 17 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_translatedstring"(b);
          result := jsonb_set(result, '{cause_detail}', submsg, true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 18 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_translatedstring"(b);
          result := jsonb_set(result, '{effect_detail}', submsg, true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      ELSE
        pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
    END CASE;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."decode_shape"(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  result jsonb := '{}'::jsonb;
  pos int := 0;
  tag numeric;
  field_num int;
  wire_type int;
  n int;
  v numeric;
  b bytea;
  s text;
  submsg jsonb;
  endpos int;
  mapobj jsonb;
  keytext text;
BEGIN
  IF data IS NULL OR octet_length(data) = 0 THEN
    RETURN result;
  END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO tag, pos;
    field_num := (tag::bigint / 8)::int;
    wire_type := (tag::numeric % 8)::int;
    CASE field_num
      WHEN 1 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{shape_id}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 2 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{encoded_polyline}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      ELSE
        pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
    END CASE;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."decode_stop"(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  result jsonb := '{}'::jsonb;
  pos int := 0;
  tag numeric;
  field_num int;
  wire_type int;
  n int;
  v numeric;
  b bytea;
  s text;
  submsg jsonb;
  endpos int;
  mapobj jsonb;
  keytext text;
BEGIN
  IF data IS NULL OR octet_length(data) = 0 THEN
    RETURN result;
  END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO tag, pos;
    field_num := (tag::bigint / 8)::int;
    wire_type := (tag::numeric % 8)::int;
    CASE field_num
      WHEN 1 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{stop_id}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 2 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_translatedstring"(b);
          result := jsonb_set(result, '{stop_code}', submsg, true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 3 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_translatedstring"(b);
          result := jsonb_set(result, '{stop_name}', submsg, true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 4 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_translatedstring"(b);
          result := jsonb_set(result, '{tts_stop_name}', submsg, true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 5 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_translatedstring"(b);
          result := jsonb_set(result, '{stop_desc}', submsg, true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 6 THEN
        IF wire_type = 5 THEN
          SELECT val, p FROM "transit_realtime".pb_read_fixed32(data, pos) INTO v, pos;
          v := "transit_realtime".pb_fixed32_to_float(v);
          result := jsonb_set(result, '{stop_lat}', to_jsonb(v), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 7 THEN
        IF wire_type = 5 THEN
          SELECT val, p FROM "transit_realtime".pb_read_fixed32(data, pos) INTO v, pos;
          v := "transit_realtime".pb_fixed32_to_float(v);
          result := jsonb_set(result, '{stop_lon}', to_jsonb(v), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 8 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{zone_id}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 9 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_translatedstring"(b);
          result := jsonb_set(result, '{stop_url}', submsg, true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 11 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{parent_station}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 12 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{stop_timezone}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 13 THEN
        IF wire_type = 0 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO v, pos;
          s := CASE v::int WHEN 0 THEN 'UNKNOWN' WHEN 1 THEN 'AVAILABLE' WHEN 2 THEN 'NOT_AVAILABLE' ELSE v::text END;
          result := jsonb_set(result, '{wheelchair_boarding}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 14 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{level_id}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 15 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_translatedstring"(b);
          result := jsonb_set(result, '{platform_code}', submsg, true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      ELSE
        pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
    END CASE;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."decode_stopselector"(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  result jsonb := '{}'::jsonb;
  pos int := 0;
  tag numeric;
  field_num int;
  wire_type int;
  n int;
  v numeric;
  b bytea;
  s text;
  submsg jsonb;
  endpos int;
  mapobj jsonb;
  keytext text;
BEGIN
  IF data IS NULL OR octet_length(data) = 0 THEN
    RETURN result;
  END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO tag, pos;
    field_num := (tag::bigint / 8)::int;
    wire_type := (tag::numeric % 8)::int;
    CASE field_num
      WHEN 1 THEN
        IF wire_type = 0 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO v, pos;
          result := jsonb_set(result, '{stop_sequence}', to_jsonb(v), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 2 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{stop_id}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      ELSE
        pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
    END CASE;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."decode_replacementstop"(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  result jsonb := '{}'::jsonb;
  pos int := 0;
  tag numeric;
  field_num int;
  wire_type int;
  n int;
  v numeric;
  b bytea;
  s text;
  submsg jsonb;
  endpos int;
  mapobj jsonb;
  keytext text;
BEGIN
  IF data IS NULL OR octet_length(data) = 0 THEN
    RETURN result;
  END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO tag, pos;
    field_num := (tag::bigint / 8)::int;
    wire_type := (tag::numeric % 8)::int;
    CASE field_num
      WHEN 1 THEN
        IF wire_type = 0 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO v, pos;
          result := jsonb_set(result, '{travel_time_to_stop}', to_jsonb(v), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 2 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{stop_id}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      ELSE
        pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
    END CASE;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."decode_tripmodifications_modification"(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  result jsonb := '{}'::jsonb;
  pos int := 0;
  tag numeric;
  field_num int;
  wire_type int;
  n int;
  v numeric;
  b bytea;
  s text;
  submsg jsonb;
  endpos int;
  mapobj jsonb;
  keytext text;
BEGIN
  IF data IS NULL OR octet_length(data) = 0 THEN
    RETURN result;
  END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO tag, pos;
    field_num := (tag::bigint / 8)::int;
    wire_type := (tag::numeric % 8)::int;
    CASE field_num
      WHEN 1 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_stopselector"(b);
          result := jsonb_set(result, '{start_stop_selector}', submsg, true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 2 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_stopselector"(b);
          result := jsonb_set(result, '{end_stop_selector}', submsg, true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 3 THEN
        IF wire_type = 0 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO v, pos;
          result := jsonb_set(result, '{propagated_modification_delay}', to_jsonb(v), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 4 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_replacementstop"(b);
          IF result ? 'replacement_stops' THEN
            result := jsonb_set(result, '{replacement_stops}', (result->'replacement_stops') || jsonb_build_array(submsg));
          ELSE
            result := jsonb_set(result, '{replacement_stops}', jsonb_build_array(submsg));
          END IF;
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 5 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{service_alert_id}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 6 THEN
        IF wire_type = 0 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO v, pos;
          result := jsonb_set(result, '{last_modified_time}', to_jsonb(v), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      ELSE
        pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
    END CASE;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."decode_tripmodifications_selectedtrips"(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  result jsonb := '{}'::jsonb;
  pos int := 0;
  tag numeric;
  field_num int;
  wire_type int;
  n int;
  v numeric;
  b bytea;
  s text;
  submsg jsonb;
  endpos int;
  mapobj jsonb;
  keytext text;
BEGIN
  IF data IS NULL OR octet_length(data) = 0 THEN
    RETURN result;
  END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO tag, pos;
    field_num := (tag::bigint / 8)::int;
    wire_type := (tag::numeric % 8)::int;
    CASE field_num
      WHEN 1 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          IF result ? 'trip_ids' THEN
            result := jsonb_set(result, '{trip_ids}', (result->'trip_ids') || jsonb_build_array(to_jsonb(s)));
          ELSE
            result := jsonb_set(result, '{trip_ids}', jsonb_build_array(to_jsonb(s)));
          END IF;
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 2 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{shape_id}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      ELSE
        pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
    END CASE;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."decode_tripmodifications"(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  result jsonb := '{}'::jsonb;
  pos int := 0;
  tag numeric;
  field_num int;
  wire_type int;
  n int;
  v numeric;
  b bytea;
  s text;
  submsg jsonb;
  endpos int;
  mapobj jsonb;
  keytext text;
BEGIN
  IF data IS NULL OR octet_length(data) = 0 THEN
    RETURN result;
  END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO tag, pos;
    field_num := (tag::bigint / 8)::int;
    wire_type := (tag::numeric % 8)::int;
    CASE field_num
      WHEN 1 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_tripmodifications_selectedtrips"(b);
          IF result ? 'selected_trips' THEN
            result := jsonb_set(result, '{selected_trips}', (result->'selected_trips') || jsonb_build_array(submsg));
          ELSE
            result := jsonb_set(result, '{selected_trips}', jsonb_build_array(submsg));
          END IF;
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 2 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          IF result ? 'start_times' THEN
            result := jsonb_set(result, '{start_times}', (result->'start_times') || jsonb_build_array(to_jsonb(s)));
          ELSE
            result := jsonb_set(result, '{start_times}', jsonb_build_array(to_jsonb(s)));
          END IF;
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 3 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          IF result ? 'service_dates' THEN
            result := jsonb_set(result, '{service_dates}', (result->'service_dates') || jsonb_build_array(to_jsonb(s)));
          ELSE
            result := jsonb_set(result, '{service_dates}', jsonb_build_array(to_jsonb(s)));
          END IF;
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 4 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_tripmodifications_modification"(b);
          IF result ? 'modifications' THEN
            result := jsonb_set(result, '{modifications}', (result->'modifications') || jsonb_build_array(submsg));
          ELSE
            result := jsonb_set(result, '{modifications}', jsonb_build_array(submsg));
          END IF;
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      ELSE
        pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
    END CASE;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."decode_feedentity"(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  result jsonb := '{}'::jsonb;
  pos int := 0;
  tag numeric;
  field_num int;
  wire_type int;
  n int;
  v numeric;
  b bytea;
  s text;
  submsg jsonb;
  endpos int;
  mapobj jsonb;
  keytext text;
BEGIN
  IF data IS NULL OR octet_length(data) = 0 THEN
    RETURN result;
  END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO tag, pos;
    field_num := (tag::bigint / 8)::int;
    wire_type := (tag::numeric % 8)::int;
    CASE field_num
      WHEN 1 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{id}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 2 THEN
        IF wire_type = 0 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO v, pos;
          result := jsonb_set(result, '{is_deleted}', to_jsonb(v <> 0), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 3 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_tripupdate"(b);
          result := jsonb_set(result, '{trip_update}', submsg, true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 4 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_vehicleposition"(b);
          result := jsonb_set(result, '{vehicle}', submsg, true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 5 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_alert"(b);
          result := jsonb_set(result, '{alert}', submsg, true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 6 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_shape"(b);
          result := jsonb_set(result, '{shape}', submsg, true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 7 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_stop"(b);
          result := jsonb_set(result, '{stop}', submsg, true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 8 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_tripmodifications"(b);
          result := jsonb_set(result, '{trip_modifications}', submsg, true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      ELSE
        pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
    END CASE;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."decode_feedmessage"(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  result jsonb := '{}'::jsonb;
  pos int := 0;
  tag numeric;
  field_num int;
  wire_type int;
  n int;
  v numeric;
  b bytea;
  s text;
  submsg jsonb;
  endpos int;
  mapobj jsonb;
  keytext text;
BEGIN
  IF data IS NULL OR octet_length(data) = 0 THEN
    RETURN result;
  END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO tag, pos;
    field_num := (tag::bigint / 8)::int;
    wire_type := (tag::numeric % 8)::int;
    CASE field_num
      WHEN 1 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_feedheader"(b);
          result := jsonb_set(result, '{header}', submsg, true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 2 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_feedentity"(b);
          IF result ? 'entity' THEN
            result := jsonb_set(result, '{entity}', (result->'entity') || jsonb_build_array(submsg));
          ELSE
            result := jsonb_set(result, '{entity}', jsonb_build_array(submsg));
          END IF;
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      ELSE
        pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
    END CASE;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."decode_translatedimage_localizedimage"(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  result jsonb := '{}'::jsonb;
  pos int := 0;
  tag numeric;
  field_num int;
  wire_type int;
  n int;
  v numeric;
  b bytea;
  s text;
  submsg jsonb;
  endpos int;
  mapobj jsonb;
  keytext text;
BEGIN
  IF data IS NULL OR octet_length(data) = 0 THEN
    RETURN result;
  END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO tag, pos;
    field_num := (tag::bigint / 8)::int;
    wire_type := (tag::numeric % 8)::int;
    CASE field_num
      WHEN 1 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{url}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 2 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{media_type}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      WHEN 3 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          s := convert_from(b, 'UTF8');
          result := jsonb_set(result, '{language}', to_jsonb(s), true);
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      ELSE
        pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
    END CASE;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."decode_translatedimage"(data bytea) RETURNS jsonb
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  result jsonb := '{}'::jsonb;
  pos int := 0;
  tag numeric;
  field_num int;
  wire_type int;
  n int;
  v numeric;
  b bytea;
  s text;
  submsg jsonb;
  endpos int;
  mapobj jsonb;
  keytext text;
BEGIN
  IF data IS NULL OR octet_length(data) = 0 THEN
    RETURN result;
  END IF;
  WHILE pos < octet_length(data) LOOP
    SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO tag, pos;
    field_num := (tag::bigint / 8)::int;
    wire_type := (tag::numeric % 8)::int;
    CASE field_num
      WHEN 1 THEN
        IF wire_type = 2 THEN
          SELECT val, p FROM "transit_realtime".pb_read_varint(data, pos) INTO n, pos;
          b := substring(data from pos+1 for n);
          pos := pos + n;
          submsg := "transit_realtime"."decode_translatedimage_localizedimage"(b);
          IF result ? 'localized_image' THEN
            result := jsonb_set(result, '{localized_image}', (result->'localized_image') || jsonb_build_array(submsg));
          ELSE
            result := jsonb_set(result, '{localized_image}', jsonb_build_array(submsg));
          END IF;
        ELSE
          pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
        END IF;
      ELSE
        pos := "transit_realtime".pb_skip_field(data, pos, wire_type, field_num);
    END CASE;
  END LOOP;
  RETURN result;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime".encode_google_protobuf_empty(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
BEGIN
  RETURN ''::bytea;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime".encode_google_protobuf_boolvalue(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE buf bytea := ''::bytea;
BEGIN
  IF j IS NULL THEN RETURN buf; END IF;
  buf := "transit_realtime".pb_write_tag(buf, 1, 0);
  buf := "transit_realtime".pb_write_varint(buf, CASE WHEN j::boolean THEN 1 ELSE 0 END);
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime".encode_google_protobuf_int32value(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE buf bytea := ''::bytea;
BEGIN
  IF j IS NULL THEN RETURN buf; END IF;
  buf := "transit_realtime".pb_write_tag(buf, 1, 0);
  buf := "transit_realtime".pb_write_varint(buf, j::numeric);
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime".encode_google_protobuf_int64value(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE buf bytea := ''::bytea;
BEGIN
  IF j IS NULL THEN RETURN buf; END IF;
  buf := "transit_realtime".pb_write_tag(buf, 1, 0);
  buf := "transit_realtime".pb_write_varint(buf, j::numeric);
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime".encode_google_protobuf_uint32value(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE buf bytea := ''::bytea;
BEGIN
  IF j IS NULL THEN RETURN buf; END IF;
  buf := "transit_realtime".pb_write_tag(buf, 1, 0);
  buf := "transit_realtime".pb_write_varint(buf, j::numeric);
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime".encode_google_protobuf_uint64value(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE buf bytea := ''::bytea;
BEGIN
  IF j IS NULL THEN RETURN buf; END IF;
  buf := "transit_realtime".pb_write_tag(buf, 1, 0);
  buf := "transit_realtime".pb_write_varint(buf, j::numeric);
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime".encode_google_protobuf_floatvalue(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE buf bytea := ''::bytea;
BEGIN
  IF j IS NULL THEN RETURN buf; END IF;
  buf := "transit_realtime".pb_write_tag(buf, 1, 5);
  buf := "transit_realtime".pb_write_float4(buf, j::float8);
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime".encode_google_protobuf_doublevalue(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE buf bytea := ''::bytea;
BEGIN
  IF j IS NULL THEN RETURN buf; END IF;
  buf := "transit_realtime".pb_write_tag(buf, 1, 1);
  buf := "transit_realtime".pb_write_float8(buf, j::float8);
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime".encode_google_protobuf_stringvalue(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE buf bytea := ''::bytea;
BEGIN
  IF j IS NULL THEN RETURN buf; END IF;
  buf := "transit_realtime".pb_write_tag(buf, 1, 2);
  buf := "transit_realtime".pb_write_string(buf, j #>> '{}');
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime".encode_google_protobuf_bytesvalue(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE buf bytea := ''::bytea;
BEGIN
  IF j IS NULL THEN RETURN buf; END IF;
  buf := "transit_realtime".pb_write_tag(buf, 1, 2);
  buf := "transit_realtime".pb_write_bytes(buf, decode(j #>> '{}', 'base64'));
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime".encode_google_protobuf_timestamp(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE buf bytea := ''::bytea;
BEGIN
  IF j IS NULL OR jsonb_typeof(j) <> 'object' THEN RETURN buf; END IF;
  IF j ? 'seconds' AND j->'seconds' IS NOT NULL THEN
    buf := "transit_realtime".pb_write_tag(buf, 1, 0);
    buf := "transit_realtime".pb_write_varint(buf, (j->>'seconds')::numeric);
  END IF;
  IF j ? 'nanos' AND j->'nanos' IS NOT NULL THEN
    buf := "transit_realtime".pb_write_tag(buf, 2, 0);
    buf := "transit_realtime".pb_write_varint(buf, (j->>'nanos')::numeric);
  END IF;
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime".encode_google_protobuf_duration(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE buf bytea := ''::bytea;
BEGIN
  IF j IS NULL OR jsonb_typeof(j) <> 'object' THEN RETURN buf; END IF;
  IF j ? 'seconds' AND j->'seconds' IS NOT NULL THEN
    buf := "transit_realtime".pb_write_tag(buf, 1, 0);
    buf := "transit_realtime".pb_write_varint(buf, (j->>'seconds')::numeric);
  END IF;
  IF j ? 'nanos' AND j->'nanos' IS NOT NULL THEN
    buf := "transit_realtime".pb_write_tag(buf, 2, 0);
    buf := "transit_realtime".pb_write_varint(buf, (j->>'nanos')::numeric);
  END IF;
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime".encode_google_protobuf_value(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE buf bytea := ''::bytea;
BEGIN
  IF j IS NULL OR j = 'null'::jsonb THEN
    buf := "transit_realtime".pb_write_tag(buf, 1, 0);
    buf := "transit_realtime".pb_write_varint(buf, 0);
    RETURN buf;
  END IF;
  IF jsonb_typeof(j) = 'boolean' THEN
    buf := "transit_realtime".pb_write_tag(buf, 4, 0);
    buf := "transit_realtime".pb_write_varint(buf, CASE WHEN j::boolean THEN 1 ELSE 0 END);
  ELSIF jsonb_typeof(j) = 'number' THEN
    buf := "transit_realtime".pb_write_tag(buf, 2, 1);
    buf := "transit_realtime".pb_write_float8(buf, j::float8);
  ELSIF jsonb_typeof(j) = 'string' THEN
    buf := "transit_realtime".pb_write_tag(buf, 3, 2);
    buf := "transit_realtime".pb_write_string(buf, j #>> '{}');
  ELSIF jsonb_typeof(j) = 'object' THEN
    buf := "transit_realtime".pb_write_tag(buf, 5, 2);
    buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime".encode_google_protobuf_struct(j));
  ELSIF jsonb_typeof(j) = 'array' THEN
    buf := "transit_realtime".pb_write_tag(buf, 6, 2);
    buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime".encode_google_protobuf_listvalue(j));
  END IF;
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime".encode_google_protobuf_struct(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE buf bytea := ''::bytea; entrybuf bytea; k text; v jsonb; fields jsonb;
BEGIN
  IF j IS NULL OR jsonb_typeof(j) <> 'object' THEN RETURN buf; END IF;
  IF j ? 'fields' AND jsonb_typeof(j->'fields') = 'object' THEN
    fields := j->'fields';
  ELSE
    fields := j;
  END IF;
  FOR k, v IN SELECT key, value FROM jsonb_each(fields) LOOP
    entrybuf := ''::bytea;
    entrybuf := "transit_realtime".pb_write_tag(entrybuf, 1, 2);
    entrybuf := "transit_realtime".pb_write_string(entrybuf, k);
    entrybuf := "transit_realtime".pb_write_tag(entrybuf, 2, 2);
    entrybuf := "transit_realtime".pb_write_bytes(entrybuf, "transit_realtime".encode_google_protobuf_value(v));
    buf := "transit_realtime".pb_write_tag(buf, 1, 2);
    buf := "transit_realtime".pb_write_bytes(buf, entrybuf);
  END LOOP;
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime".encode_google_protobuf_listvalue(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE buf bytea := ''::bytea; elem jsonb;
BEGIN
  IF j IS NULL OR jsonb_typeof(j) <> 'array' THEN RETURN buf; END IF;
  FOR elem IN SELECT jsonb_array_elements(j) LOOP
    buf := "transit_realtime".pb_write_tag(buf, 1, 2);
    buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime".encode_google_protobuf_value(elem));
  END LOOP;
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime".encode_google_protobuf_any(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE buf bytea := ''::bytea;
BEGIN
  IF j IS NULL OR jsonb_typeof(j) <> 'object' THEN RETURN buf; END IF;
  IF j ? 'type_url' THEN
    buf := "transit_realtime".pb_write_tag(buf, 1, 2);
    buf := "transit_realtime".pb_write_string(buf, j->>'type_url');
  END IF;
  IF j ? 'value' THEN
    buf := "transit_realtime".pb_write_tag(buf, 2, 2);
    buf := "transit_realtime".pb_write_bytes(buf, decode(j->>'value', 'base64'));
  END IF;
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime".encode_google_protobuf_fieldmask(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE buf bytea := ''::bytea; elem text;
BEGIN
  IF j IS NULL THEN RETURN buf; END IF;
  IF jsonb_typeof(j) = 'array' THEN
    FOR elem IN SELECT jsonb_array_elements_text(j) LOOP
      buf := "transit_realtime".pb_write_tag(buf, 1, 2);
      buf := "transit_realtime".pb_write_string(buf, elem);
    END LOOP;
  ELSIF jsonb_typeof(j) = 'string' THEN
    buf := "transit_realtime".pb_write_tag(buf, 1, 2);
    buf := "transit_realtime".pb_write_string(buf, j #>> '{}');
  END IF;
  RETURN buf;
END;
$$;


CREATE OR REPLACE FUNCTION "transit_realtime"."encode_feedheader"(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  buf bytea := ''::bytea;
  elem jsonb;
  k text;
  v jsonb;
  entrybuf bytea;
  enumval int;
  tmp numeric;
BEGIN
  IF j IS NULL OR jsonb_typeof(j) <> 'object' THEN
    RETURN buf;
  END IF;
  IF j ? 'gtfs_realtime_version' AND j->'gtfs_realtime_version' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 1, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'gtfs_realtime_version'->>0);
  END IF;
  IF j ? 'incrementality' AND j->'incrementality' IS NOT NULL THEN
      enumval := NULL;
      IF j->'incrementality'->>0 IS NOT NULL AND j->'incrementality'#>>'{}' = 'FULL_DATASET' THEN enumval := 0; END IF;
      IF j->'incrementality'->>0 IS NOT NULL AND j->'incrementality'#>>'{}' = 'DIFFERENTIAL' THEN enumval := 1; END IF;
      IF enumval IS NULL AND jsonb_typeof(j->'incrementality') = 'number' THEN enumval := (j->'incrementality'->>0)::int; END IF;
      IF enumval IS NOT NULL THEN
        buf := "transit_realtime".pb_write_tag(buf, 2, 0);
        buf := "transit_realtime".pb_write_varint(buf, enumval);
      END IF;
  END IF;
  IF j ? 'timestamp' AND j->'timestamp' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 3, 0);
      buf := "transit_realtime".pb_write_varint(buf, (j->'timestamp')::numeric);
  END IF;
  IF j ? 'feed_version' AND j->'feed_version' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 4, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'feed_version'->>0);
  END IF;
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."encode_tripupdate_stoptimeevent"(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  buf bytea := ''::bytea;
  elem jsonb;
  k text;
  v jsonb;
  entrybuf bytea;
  enumval int;
  tmp numeric;
BEGIN
  IF j IS NULL OR jsonb_typeof(j) <> 'object' THEN
    RETURN buf;
  END IF;
  IF j ? 'delay' AND j->'delay' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 1, 0);
      buf := "transit_realtime".pb_write_varint(buf, (j->'delay')::numeric);
  END IF;
  IF j ? 'time' AND j->'time' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 2, 0);
      buf := "transit_realtime".pb_write_varint(buf, (j->'time')::numeric);
  END IF;
  IF j ? 'uncertainty' AND j->'uncertainty' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 3, 0);
      buf := "transit_realtime".pb_write_varint(buf, (j->'uncertainty')::numeric);
  END IF;
  IF j ? 'scheduled_time' AND j->'scheduled_time' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 4, 0);
      buf := "transit_realtime".pb_write_varint(buf, (j->'scheduled_time')::numeric);
  END IF;
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."encode_tripupdate_stoptimeupdate_stoptimeproperties"(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  buf bytea := ''::bytea;
  elem jsonb;
  k text;
  v jsonb;
  entrybuf bytea;
  enumval int;
  tmp numeric;
BEGIN
  IF j IS NULL OR jsonb_typeof(j) <> 'object' THEN
    RETURN buf;
  END IF;
  IF j ? 'assigned_stop_id' AND j->'assigned_stop_id' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 1, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'assigned_stop_id'->>0);
  END IF;
  IF j ? 'stop_headsign' AND j->'stop_headsign' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 2, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'stop_headsign'->>0);
  END IF;
  IF j ? 'pickup_type' AND j->'pickup_type' IS NOT NULL THEN
      enumval := NULL;
      IF j->'pickup_type'->>0 IS NOT NULL AND j->'pickup_type'#>>'{}' = 'REGULAR' THEN enumval := 0; END IF;
      IF j->'pickup_type'->>0 IS NOT NULL AND j->'pickup_type'#>>'{}' = 'NONE' THEN enumval := 1; END IF;
      IF j->'pickup_type'->>0 IS NOT NULL AND j->'pickup_type'#>>'{}' = 'PHONE_AGENCY' THEN enumval := 2; END IF;
      IF j->'pickup_type'->>0 IS NOT NULL AND j->'pickup_type'#>>'{}' = 'COORDINATE_WITH_DRIVER' THEN enumval := 3; END IF;
      IF enumval IS NULL AND jsonb_typeof(j->'pickup_type') = 'number' THEN enumval := (j->'pickup_type'->>0)::int; END IF;
      IF enumval IS NOT NULL THEN
        buf := "transit_realtime".pb_write_tag(buf, 3, 0);
        buf := "transit_realtime".pb_write_varint(buf, enumval);
      END IF;
  END IF;
  IF j ? 'drop_off_type' AND j->'drop_off_type' IS NOT NULL THEN
      enumval := NULL;
      IF j->'drop_off_type'->>0 IS NOT NULL AND j->'drop_off_type'#>>'{}' = 'REGULAR' THEN enumval := 0; END IF;
      IF j->'drop_off_type'->>0 IS NOT NULL AND j->'drop_off_type'#>>'{}' = 'NONE' THEN enumval := 1; END IF;
      IF j->'drop_off_type'->>0 IS NOT NULL AND j->'drop_off_type'#>>'{}' = 'PHONE_AGENCY' THEN enumval := 2; END IF;
      IF j->'drop_off_type'->>0 IS NOT NULL AND j->'drop_off_type'#>>'{}' = 'COORDINATE_WITH_DRIVER' THEN enumval := 3; END IF;
      IF enumval IS NULL AND jsonb_typeof(j->'drop_off_type') = 'number' THEN enumval := (j->'drop_off_type'->>0)::int; END IF;
      IF enumval IS NOT NULL THEN
        buf := "transit_realtime".pb_write_tag(buf, 4, 0);
        buf := "transit_realtime".pb_write_varint(buf, enumval);
      END IF;
  END IF;
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."encode_tripupdate_stoptimeupdate"(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  buf bytea := ''::bytea;
  elem jsonb;
  k text;
  v jsonb;
  entrybuf bytea;
  enumval int;
  tmp numeric;
BEGIN
  IF j IS NULL OR jsonb_typeof(j) <> 'object' THEN
    RETURN buf;
  END IF;
  IF j ? 'stop_sequence' AND j->'stop_sequence' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 1, 0);
      buf := "transit_realtime".pb_write_varint(buf, (j->'stop_sequence')::numeric);
  END IF;
  IF j ? 'arrival' AND j->'arrival' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 2, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_tripupdate_stoptimeevent"(j->'arrival'));
  END IF;
  IF j ? 'departure' AND j->'departure' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 3, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_tripupdate_stoptimeevent"(j->'departure'));
  END IF;
  IF j ? 'stop_id' AND j->'stop_id' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 4, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'stop_id'->>0);
  END IF;
  IF j ? 'schedule_relationship' AND j->'schedule_relationship' IS NOT NULL THEN
      enumval := NULL;
      IF j->'schedule_relationship'->>0 IS NOT NULL AND j->'schedule_relationship'#>>'{}' = 'SCHEDULED' THEN enumval := 0; END IF;
      IF j->'schedule_relationship'->>0 IS NOT NULL AND j->'schedule_relationship'#>>'{}' = 'SKIPPED' THEN enumval := 1; END IF;
      IF j->'schedule_relationship'->>0 IS NOT NULL AND j->'schedule_relationship'#>>'{}' = 'NO_DATA' THEN enumval := 2; END IF;
      IF j->'schedule_relationship'->>0 IS NOT NULL AND j->'schedule_relationship'#>>'{}' = 'UNSCHEDULED' THEN enumval := 3; END IF;
      IF enumval IS NULL AND jsonb_typeof(j->'schedule_relationship') = 'number' THEN enumval := (j->'schedule_relationship'->>0)::int; END IF;
      IF enumval IS NOT NULL THEN
        buf := "transit_realtime".pb_write_tag(buf, 5, 0);
        buf := "transit_realtime".pb_write_varint(buf, enumval);
      END IF;
  END IF;
  IF j ? 'stop_time_properties' AND j->'stop_time_properties' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 6, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_tripupdate_stoptimeupdate_stoptimeproperties"(j->'stop_time_properties'));
  END IF;
  IF j ? 'departure_occupancy_status' AND j->'departure_occupancy_status' IS NOT NULL THEN
      enumval := NULL;
      IF j->'departure_occupancy_status'->>0 IS NOT NULL AND j->'departure_occupancy_status'#>>'{}' = 'EMPTY' THEN enumval := 0; END IF;
      IF j->'departure_occupancy_status'->>0 IS NOT NULL AND j->'departure_occupancy_status'#>>'{}' = 'MANY_SEATS_AVAILABLE' THEN enumval := 1; END IF;
      IF j->'departure_occupancy_status'->>0 IS NOT NULL AND j->'departure_occupancy_status'#>>'{}' = 'FEW_SEATS_AVAILABLE' THEN enumval := 2; END IF;
      IF j->'departure_occupancy_status'->>0 IS NOT NULL AND j->'departure_occupancy_status'#>>'{}' = 'STANDING_ROOM_ONLY' THEN enumval := 3; END IF;
      IF j->'departure_occupancy_status'->>0 IS NOT NULL AND j->'departure_occupancy_status'#>>'{}' = 'CRUSHED_STANDING_ROOM_ONLY' THEN enumval := 4; END IF;
      IF j->'departure_occupancy_status'->>0 IS NOT NULL AND j->'departure_occupancy_status'#>>'{}' = 'FULL' THEN enumval := 5; END IF;
      IF j->'departure_occupancy_status'->>0 IS NOT NULL AND j->'departure_occupancy_status'#>>'{}' = 'NOT_ACCEPTING_PASSENGERS' THEN enumval := 6; END IF;
      IF j->'departure_occupancy_status'->>0 IS NOT NULL AND j->'departure_occupancy_status'#>>'{}' = 'NO_DATA_AVAILABLE' THEN enumval := 7; END IF;
      IF j->'departure_occupancy_status'->>0 IS NOT NULL AND j->'departure_occupancy_status'#>>'{}' = 'NOT_BOARDABLE' THEN enumval := 8; END IF;
      IF enumval IS NULL AND jsonb_typeof(j->'departure_occupancy_status') = 'number' THEN enumval := (j->'departure_occupancy_status'->>0)::int; END IF;
      IF enumval IS NOT NULL THEN
        buf := "transit_realtime".pb_write_tag(buf, 7, 0);
        buf := "transit_realtime".pb_write_varint(buf, enumval);
      END IF;
  END IF;
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."encode_tripupdate_tripproperties"(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  buf bytea := ''::bytea;
  elem jsonb;
  k text;
  v jsonb;
  entrybuf bytea;
  enumval int;
  tmp numeric;
BEGIN
  IF j IS NULL OR jsonb_typeof(j) <> 'object' THEN
    RETURN buf;
  END IF;
  IF j ? 'trip_id' AND j->'trip_id' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 1, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'trip_id'->>0);
  END IF;
  IF j ? 'start_date' AND j->'start_date' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 2, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'start_date'->>0);
  END IF;
  IF j ? 'start_time' AND j->'start_time' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 3, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'start_time'->>0);
  END IF;
  IF j ? 'shape_id' AND j->'shape_id' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 4, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'shape_id'->>0);
  END IF;
  IF j ? 'trip_headsign' AND j->'trip_headsign' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 5, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'trip_headsign'->>0);
  END IF;
  IF j ? 'trip_short_name' AND j->'trip_short_name' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 6, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'trip_short_name'->>0);
  END IF;
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."encode_tripdescriptor_modifiedtripselector"(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  buf bytea := ''::bytea;
  elem jsonb;
  k text;
  v jsonb;
  entrybuf bytea;
  enumval int;
  tmp numeric;
BEGIN
  IF j IS NULL OR jsonb_typeof(j) <> 'object' THEN
    RETURN buf;
  END IF;
  IF j ? 'modifications_id' AND j->'modifications_id' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 1, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'modifications_id'->>0);
  END IF;
  IF j ? 'affected_trip_id' AND j->'affected_trip_id' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 2, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'affected_trip_id'->>0);
  END IF;
  IF j ? 'start_time' AND j->'start_time' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 3, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'start_time'->>0);
  END IF;
  IF j ? 'start_date' AND j->'start_date' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 4, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'start_date'->>0);
  END IF;
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."encode_tripdescriptor"(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  buf bytea := ''::bytea;
  elem jsonb;
  k text;
  v jsonb;
  entrybuf bytea;
  enumval int;
  tmp numeric;
BEGIN
  IF j IS NULL OR jsonb_typeof(j) <> 'object' THEN
    RETURN buf;
  END IF;
  IF j ? 'trip_id' AND j->'trip_id' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 1, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'trip_id'->>0);
  END IF;
  IF j ? 'start_time' AND j->'start_time' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 2, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'start_time'->>0);
  END IF;
  IF j ? 'start_date' AND j->'start_date' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 3, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'start_date'->>0);
  END IF;
  IF j ? 'schedule_relationship' AND j->'schedule_relationship' IS NOT NULL THEN
      enumval := NULL;
      IF j->'schedule_relationship'->>0 IS NOT NULL AND j->'schedule_relationship'#>>'{}' = 'SCHEDULED' THEN enumval := 0; END IF;
      IF j->'schedule_relationship'->>0 IS NOT NULL AND j->'schedule_relationship'#>>'{}' = 'ADDED' THEN enumval := 1; END IF;
      IF j->'schedule_relationship'->>0 IS NOT NULL AND j->'schedule_relationship'#>>'{}' = 'UNSCHEDULED' THEN enumval := 2; END IF;
      IF j->'schedule_relationship'->>0 IS NOT NULL AND j->'schedule_relationship'#>>'{}' = 'CANCELED' THEN enumval := 3; END IF;
      IF j->'schedule_relationship'->>0 IS NOT NULL AND j->'schedule_relationship'#>>'{}' = 'REPLACEMENT' THEN enumval := 5; END IF;
      IF j->'schedule_relationship'->>0 IS NOT NULL AND j->'schedule_relationship'#>>'{}' = 'DUPLICATED' THEN enumval := 6; END IF;
      IF j->'schedule_relationship'->>0 IS NOT NULL AND j->'schedule_relationship'#>>'{}' = 'DELETED' THEN enumval := 7; END IF;
      IF j->'schedule_relationship'->>0 IS NOT NULL AND j->'schedule_relationship'#>>'{}' = 'NEW' THEN enumval := 8; END IF;
      IF enumval IS NULL AND jsonb_typeof(j->'schedule_relationship') = 'number' THEN enumval := (j->'schedule_relationship'->>0)::int; END IF;
      IF enumval IS NOT NULL THEN
        buf := "transit_realtime".pb_write_tag(buf, 4, 0);
        buf := "transit_realtime".pb_write_varint(buf, enumval);
      END IF;
  END IF;
  IF j ? 'route_id' AND j->'route_id' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 5, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'route_id'->>0);
  END IF;
  IF j ? 'direction_id' AND j->'direction_id' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 6, 0);
      buf := "transit_realtime".pb_write_varint(buf, (j->'direction_id')::numeric);
  END IF;
  IF j ? 'modified_trip' AND j->'modified_trip' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 7, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_tripdescriptor_modifiedtripselector"(j->'modified_trip'));
  END IF;
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."encode_vehicledescriptor"(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  buf bytea := ''::bytea;
  elem jsonb;
  k text;
  v jsonb;
  entrybuf bytea;
  enumval int;
  tmp numeric;
BEGIN
  IF j IS NULL OR jsonb_typeof(j) <> 'object' THEN
    RETURN buf;
  END IF;
  IF j ? 'id' AND j->'id' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 1, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'id'->>0);
  END IF;
  IF j ? 'label' AND j->'label' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 2, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'label'->>0);
  END IF;
  IF j ? 'license_plate' AND j->'license_plate' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 3, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'license_plate'->>0);
  END IF;
  IF j ? 'wheelchair_accessible' AND j->'wheelchair_accessible' IS NOT NULL THEN
      enumval := NULL;
      IF j->'wheelchair_accessible'->>0 IS NOT NULL AND j->'wheelchair_accessible'#>>'{}' = 'NO_VALUE' THEN enumval := 0; END IF;
      IF j->'wheelchair_accessible'->>0 IS NOT NULL AND j->'wheelchair_accessible'#>>'{}' = 'UNKNOWN' THEN enumval := 1; END IF;
      IF j->'wheelchair_accessible'->>0 IS NOT NULL AND j->'wheelchair_accessible'#>>'{}' = 'WHEELCHAIR_ACCESSIBLE' THEN enumval := 2; END IF;
      IF j->'wheelchair_accessible'->>0 IS NOT NULL AND j->'wheelchair_accessible'#>>'{}' = 'WHEELCHAIR_INACCESSIBLE' THEN enumval := 3; END IF;
      IF enumval IS NULL AND jsonb_typeof(j->'wheelchair_accessible') = 'number' THEN enumval := (j->'wheelchair_accessible'->>0)::int; END IF;
      IF enumval IS NOT NULL THEN
        buf := "transit_realtime".pb_write_tag(buf, 4, 0);
        buf := "transit_realtime".pb_write_varint(buf, enumval);
      END IF;
  END IF;
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."encode_tripupdate"(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  buf bytea := ''::bytea;
  elem jsonb;
  k text;
  v jsonb;
  entrybuf bytea;
  enumval int;
  tmp numeric;
BEGIN
  IF j IS NULL OR jsonb_typeof(j) <> 'object' THEN
    RETURN buf;
  END IF;
  IF j ? 'trip' AND j->'trip' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 1, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_tripdescriptor"(j->'trip'));
  END IF;
  IF j ? 'stop_time_update' AND j->'stop_time_update' IS NOT NULL AND jsonb_typeof(j->'stop_time_update') = 'array' THEN
    FOR elem IN SELECT jsonb_array_elements(j->'stop_time_update') LOOP
      buf := "transit_realtime".pb_write_tag(buf, 2, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_tripupdate_stoptimeupdate"(elem));
    END LOOP;
  END IF;
  IF j ? 'vehicle' AND j->'vehicle' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 3, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_vehicledescriptor"(j->'vehicle'));
  END IF;
  IF j ? 'timestamp' AND j->'timestamp' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 4, 0);
      buf := "transit_realtime".pb_write_varint(buf, (j->'timestamp')::numeric);
  END IF;
  IF j ? 'delay' AND j->'delay' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 5, 0);
      buf := "transit_realtime".pb_write_varint(buf, (j->'delay')::numeric);
  END IF;
  IF j ? 'trip_properties' AND j->'trip_properties' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 6, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_tripupdate_tripproperties"(j->'trip_properties'));
  END IF;
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."encode_vehicleposition_carriagedetails"(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  buf bytea := ''::bytea;
  elem jsonb;
  k text;
  v jsonb;
  entrybuf bytea;
  enumval int;
  tmp numeric;
BEGIN
  IF j IS NULL OR jsonb_typeof(j) <> 'object' THEN
    RETURN buf;
  END IF;
  IF j ? 'id' AND j->'id' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 1, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'id'->>0);
  END IF;
  IF j ? 'label' AND j->'label' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 2, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'label'->>0);
  END IF;
  IF j ? 'occupancy_status' AND j->'occupancy_status' IS NOT NULL THEN
      enumval := NULL;
      IF j->'occupancy_status'->>0 IS NOT NULL AND j->'occupancy_status'#>>'{}' = 'EMPTY' THEN enumval := 0; END IF;
      IF j->'occupancy_status'->>0 IS NOT NULL AND j->'occupancy_status'#>>'{}' = 'MANY_SEATS_AVAILABLE' THEN enumval := 1; END IF;
      IF j->'occupancy_status'->>0 IS NOT NULL AND j->'occupancy_status'#>>'{}' = 'FEW_SEATS_AVAILABLE' THEN enumval := 2; END IF;
      IF j->'occupancy_status'->>0 IS NOT NULL AND j->'occupancy_status'#>>'{}' = 'STANDING_ROOM_ONLY' THEN enumval := 3; END IF;
      IF j->'occupancy_status'->>0 IS NOT NULL AND j->'occupancy_status'#>>'{}' = 'CRUSHED_STANDING_ROOM_ONLY' THEN enumval := 4; END IF;
      IF j->'occupancy_status'->>0 IS NOT NULL AND j->'occupancy_status'#>>'{}' = 'FULL' THEN enumval := 5; END IF;
      IF j->'occupancy_status'->>0 IS NOT NULL AND j->'occupancy_status'#>>'{}' = 'NOT_ACCEPTING_PASSENGERS' THEN enumval := 6; END IF;
      IF j->'occupancy_status'->>0 IS NOT NULL AND j->'occupancy_status'#>>'{}' = 'NO_DATA_AVAILABLE' THEN enumval := 7; END IF;
      IF j->'occupancy_status'->>0 IS NOT NULL AND j->'occupancy_status'#>>'{}' = 'NOT_BOARDABLE' THEN enumval := 8; END IF;
      IF enumval IS NULL AND jsonb_typeof(j->'occupancy_status') = 'number' THEN enumval := (j->'occupancy_status'->>0)::int; END IF;
      IF enumval IS NOT NULL THEN
        buf := "transit_realtime".pb_write_tag(buf, 3, 0);
        buf := "transit_realtime".pb_write_varint(buf, enumval);
      END IF;
  END IF;
  IF j ? 'occupancy_percentage' AND j->'occupancy_percentage' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 4, 0);
      buf := "transit_realtime".pb_write_varint(buf, (j->'occupancy_percentage')::numeric);
  END IF;
  IF j ? 'carriage_sequence' AND j->'carriage_sequence' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 5, 0);
      buf := "transit_realtime".pb_write_varint(buf, (j->'carriage_sequence')::numeric);
  END IF;
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."encode_position"(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  buf bytea := ''::bytea;
  elem jsonb;
  k text;
  v jsonb;
  entrybuf bytea;
  enumval int;
  tmp numeric;
BEGIN
  IF j IS NULL OR jsonb_typeof(j) <> 'object' THEN
    RETURN buf;
  END IF;
  IF j ? 'latitude' AND j->'latitude' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 1, 5);
      buf := "transit_realtime".pb_write_float4(buf, (j->'latitude')::float8);
  END IF;
  IF j ? 'longitude' AND j->'longitude' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 2, 5);
      buf := "transit_realtime".pb_write_float4(buf, (j->'longitude')::float8);
  END IF;
  IF j ? 'bearing' AND j->'bearing' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 3, 5);
      buf := "transit_realtime".pb_write_float4(buf, (j->'bearing')::float8);
  END IF;
  IF j ? 'odometer' AND j->'odometer' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 4, 1);
      buf := "transit_realtime".pb_write_float8(buf, (j->'odometer')::float8);
  END IF;
  IF j ? 'speed' AND j->'speed' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 5, 5);
      buf := "transit_realtime".pb_write_float4(buf, (j->'speed')::float8);
  END IF;
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."encode_vehicleposition"(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  buf bytea := ''::bytea;
  elem jsonb;
  k text;
  v jsonb;
  entrybuf bytea;
  enumval int;
  tmp numeric;
BEGIN
  IF j IS NULL OR jsonb_typeof(j) <> 'object' THEN
    RETURN buf;
  END IF;
  IF j ? 'trip' AND j->'trip' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 1, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_tripdescriptor"(j->'trip'));
  END IF;
  IF j ? 'position' AND j->'position' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 2, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_position"(j->'position'));
  END IF;
  IF j ? 'current_stop_sequence' AND j->'current_stop_sequence' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 3, 0);
      buf := "transit_realtime".pb_write_varint(buf, (j->'current_stop_sequence')::numeric);
  END IF;
  IF j ? 'current_status' AND j->'current_status' IS NOT NULL THEN
      enumval := NULL;
      IF j->'current_status'->>0 IS NOT NULL AND j->'current_status'#>>'{}' = 'INCOMING_AT' THEN enumval := 0; END IF;
      IF j->'current_status'->>0 IS NOT NULL AND j->'current_status'#>>'{}' = 'STOPPED_AT' THEN enumval := 1; END IF;
      IF j->'current_status'->>0 IS NOT NULL AND j->'current_status'#>>'{}' = 'IN_TRANSIT_TO' THEN enumval := 2; END IF;
      IF enumval IS NULL AND jsonb_typeof(j->'current_status') = 'number' THEN enumval := (j->'current_status'->>0)::int; END IF;
      IF enumval IS NOT NULL THEN
        buf := "transit_realtime".pb_write_tag(buf, 4, 0);
        buf := "transit_realtime".pb_write_varint(buf, enumval);
      END IF;
  END IF;
  IF j ? 'timestamp' AND j->'timestamp' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 5, 0);
      buf := "transit_realtime".pb_write_varint(buf, (j->'timestamp')::numeric);
  END IF;
  IF j ? 'congestion_level' AND j->'congestion_level' IS NOT NULL THEN
      enumval := NULL;
      IF j->'congestion_level'->>0 IS NOT NULL AND j->'congestion_level'#>>'{}' = 'UNKNOWN_CONGESTION_LEVEL' THEN enumval := 0; END IF;
      IF j->'congestion_level'->>0 IS NOT NULL AND j->'congestion_level'#>>'{}' = 'RUNNING_SMOOTHLY' THEN enumval := 1; END IF;
      IF j->'congestion_level'->>0 IS NOT NULL AND j->'congestion_level'#>>'{}' = 'STOP_AND_GO' THEN enumval := 2; END IF;
      IF j->'congestion_level'->>0 IS NOT NULL AND j->'congestion_level'#>>'{}' = 'CONGESTION' THEN enumval := 3; END IF;
      IF j->'congestion_level'->>0 IS NOT NULL AND j->'congestion_level'#>>'{}' = 'SEVERE_CONGESTION' THEN enumval := 4; END IF;
      IF enumval IS NULL AND jsonb_typeof(j->'congestion_level') = 'number' THEN enumval := (j->'congestion_level'->>0)::int; END IF;
      IF enumval IS NOT NULL THEN
        buf := "transit_realtime".pb_write_tag(buf, 6, 0);
        buf := "transit_realtime".pb_write_varint(buf, enumval);
      END IF;
  END IF;
  IF j ? 'stop_id' AND j->'stop_id' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 7, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'stop_id'->>0);
  END IF;
  IF j ? 'vehicle' AND j->'vehicle' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 8, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_vehicledescriptor"(j->'vehicle'));
  END IF;
  IF j ? 'occupancy_status' AND j->'occupancy_status' IS NOT NULL THEN
      enumval := NULL;
      IF j->'occupancy_status'->>0 IS NOT NULL AND j->'occupancy_status'#>>'{}' = 'EMPTY' THEN enumval := 0; END IF;
      IF j->'occupancy_status'->>0 IS NOT NULL AND j->'occupancy_status'#>>'{}' = 'MANY_SEATS_AVAILABLE' THEN enumval := 1; END IF;
      IF j->'occupancy_status'->>0 IS NOT NULL AND j->'occupancy_status'#>>'{}' = 'FEW_SEATS_AVAILABLE' THEN enumval := 2; END IF;
      IF j->'occupancy_status'->>0 IS NOT NULL AND j->'occupancy_status'#>>'{}' = 'STANDING_ROOM_ONLY' THEN enumval := 3; END IF;
      IF j->'occupancy_status'->>0 IS NOT NULL AND j->'occupancy_status'#>>'{}' = 'CRUSHED_STANDING_ROOM_ONLY' THEN enumval := 4; END IF;
      IF j->'occupancy_status'->>0 IS NOT NULL AND j->'occupancy_status'#>>'{}' = 'FULL' THEN enumval := 5; END IF;
      IF j->'occupancy_status'->>0 IS NOT NULL AND j->'occupancy_status'#>>'{}' = 'NOT_ACCEPTING_PASSENGERS' THEN enumval := 6; END IF;
      IF j->'occupancy_status'->>0 IS NOT NULL AND j->'occupancy_status'#>>'{}' = 'NO_DATA_AVAILABLE' THEN enumval := 7; END IF;
      IF j->'occupancy_status'->>0 IS NOT NULL AND j->'occupancy_status'#>>'{}' = 'NOT_BOARDABLE' THEN enumval := 8; END IF;
      IF enumval IS NULL AND jsonb_typeof(j->'occupancy_status') = 'number' THEN enumval := (j->'occupancy_status'->>0)::int; END IF;
      IF enumval IS NOT NULL THEN
        buf := "transit_realtime".pb_write_tag(buf, 9, 0);
        buf := "transit_realtime".pb_write_varint(buf, enumval);
      END IF;
  END IF;
  IF j ? 'occupancy_percentage' AND j->'occupancy_percentage' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 10, 0);
      buf := "transit_realtime".pb_write_varint(buf, (j->'occupancy_percentage')::numeric);
  END IF;
  IF j ? 'multi_carriage_details' AND j->'multi_carriage_details' IS NOT NULL AND jsonb_typeof(j->'multi_carriage_details') = 'array' THEN
    FOR elem IN SELECT jsonb_array_elements(j->'multi_carriage_details') LOOP
      buf := "transit_realtime".pb_write_tag(buf, 11, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_vehicleposition_carriagedetails"(elem));
    END LOOP;
  END IF;
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."encode_timerange"(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  buf bytea := ''::bytea;
  elem jsonb;
  k text;
  v jsonb;
  entrybuf bytea;
  enumval int;
  tmp numeric;
BEGIN
  IF j IS NULL OR jsonb_typeof(j) <> 'object' THEN
    RETURN buf;
  END IF;
  IF j ? 'start' AND j->'start' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 1, 0);
      buf := "transit_realtime".pb_write_varint(buf, (j->'start')::numeric);
  END IF;
  IF j ? 'end' AND j->'end' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 2, 0);
      buf := "transit_realtime".pb_write_varint(buf, (j->'end')::numeric);
  END IF;
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."encode_entityselector"(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  buf bytea := ''::bytea;
  elem jsonb;
  k text;
  v jsonb;
  entrybuf bytea;
  enumval int;
  tmp numeric;
BEGIN
  IF j IS NULL OR jsonb_typeof(j) <> 'object' THEN
    RETURN buf;
  END IF;
  IF j ? 'agency_id' AND j->'agency_id' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 1, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'agency_id'->>0);
  END IF;
  IF j ? 'route_id' AND j->'route_id' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 2, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'route_id'->>0);
  END IF;
  IF j ? 'route_type' AND j->'route_type' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 3, 0);
      buf := "transit_realtime".pb_write_varint(buf, (j->'route_type')::numeric);
  END IF;
  IF j ? 'trip' AND j->'trip' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 4, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_tripdescriptor"(j->'trip'));
  END IF;
  IF j ? 'stop_id' AND j->'stop_id' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 5, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'stop_id'->>0);
  END IF;
  IF j ? 'direction_id' AND j->'direction_id' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 6, 0);
      buf := "transit_realtime".pb_write_varint(buf, (j->'direction_id')::numeric);
  END IF;
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."encode_translatedstring_translation"(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  buf bytea := ''::bytea;
  elem jsonb;
  k text;
  v jsonb;
  entrybuf bytea;
  enumval int;
  tmp numeric;
BEGIN
  IF j IS NULL OR jsonb_typeof(j) <> 'object' THEN
    RETURN buf;
  END IF;
  IF j ? 'text' AND j->'text' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 1, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'text'->>0);
  END IF;
  IF j ? 'language' AND j->'language' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 2, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'language'->>0);
  END IF;
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."encode_translatedstring"(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  buf bytea := ''::bytea;
  elem jsonb;
  k text;
  v jsonb;
  entrybuf bytea;
  enumval int;
  tmp numeric;
BEGIN
  IF j IS NULL OR jsonb_typeof(j) <> 'object' THEN
    RETURN buf;
  END IF;
  IF j ? 'translation' AND j->'translation' IS NOT NULL AND jsonb_typeof(j->'translation') = 'array' THEN
    FOR elem IN SELECT jsonb_array_elements(j->'translation') LOOP
      buf := "transit_realtime".pb_write_tag(buf, 1, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_translatedstring_translation"(elem));
    END LOOP;
  END IF;
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."encode_alert"(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  buf bytea := ''::bytea;
  elem jsonb;
  k text;
  v jsonb;
  entrybuf bytea;
  enumval int;
  tmp numeric;
BEGIN
  IF j IS NULL OR jsonb_typeof(j) <> 'object' THEN
    RETURN buf;
  END IF;
  IF j ? 'active_period' AND j->'active_period' IS NOT NULL AND jsonb_typeof(j->'active_period') = 'array' THEN
    FOR elem IN SELECT jsonb_array_elements(j->'active_period') LOOP
      buf := "transit_realtime".pb_write_tag(buf, 1, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_timerange"(elem));
    END LOOP;
  END IF;
  IF j ? 'communication_period' AND j->'communication_period' IS NOT NULL AND jsonb_typeof(j->'communication_period') = 'array' THEN
    FOR elem IN SELECT jsonb_array_elements(j->'communication_period') LOOP
      buf := "transit_realtime".pb_write_tag(buf, 2, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_timerange"(elem));
    END LOOP;
  END IF;
  IF j ? 'impact_period' AND j->'impact_period' IS NOT NULL AND jsonb_typeof(j->'impact_period') = 'array' THEN
    FOR elem IN SELECT jsonb_array_elements(j->'impact_period') LOOP
      buf := "transit_realtime".pb_write_tag(buf, 3, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_timerange"(elem));
    END LOOP;
  END IF;
  IF j ? 'informed_entity' AND j->'informed_entity' IS NOT NULL AND jsonb_typeof(j->'informed_entity') = 'array' THEN
    FOR elem IN SELECT jsonb_array_elements(j->'informed_entity') LOOP
      buf := "transit_realtime".pb_write_tag(buf, 5, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_entityselector"(elem));
    END LOOP;
  END IF;
  IF j ? 'cause' AND j->'cause' IS NOT NULL THEN
      enumval := NULL;
      IF j->'cause'->>0 IS NOT NULL AND j->'cause'#>>'{}' = 'UNKNOWN_CAUSE' THEN enumval := 1; END IF;
      IF j->'cause'->>0 IS NOT NULL AND j->'cause'#>>'{}' = 'OTHER_CAUSE' THEN enumval := 2; END IF;
      IF j->'cause'->>0 IS NOT NULL AND j->'cause'#>>'{}' = 'TECHNICAL_PROBLEM' THEN enumval := 3; END IF;
      IF j->'cause'->>0 IS NOT NULL AND j->'cause'#>>'{}' = 'STRIKE' THEN enumval := 4; END IF;
      IF j->'cause'->>0 IS NOT NULL AND j->'cause'#>>'{}' = 'DEMONSTRATION' THEN enumval := 5; END IF;
      IF j->'cause'->>0 IS NOT NULL AND j->'cause'#>>'{}' = 'ACCIDENT' THEN enumval := 6; END IF;
      IF j->'cause'->>0 IS NOT NULL AND j->'cause'#>>'{}' = 'HOLIDAY' THEN enumval := 7; END IF;
      IF j->'cause'->>0 IS NOT NULL AND j->'cause'#>>'{}' = 'WEATHER' THEN enumval := 8; END IF;
      IF j->'cause'->>0 IS NOT NULL AND j->'cause'#>>'{}' = 'MAINTENANCE' THEN enumval := 9; END IF;
      IF j->'cause'->>0 IS NOT NULL AND j->'cause'#>>'{}' = 'CONSTRUCTION' THEN enumval := 10; END IF;
      IF j->'cause'->>0 IS NOT NULL AND j->'cause'#>>'{}' = 'POLICE_ACTIVITY' THEN enumval := 11; END IF;
      IF j->'cause'->>0 IS NOT NULL AND j->'cause'#>>'{}' = 'MEDICAL_EMERGENCY' THEN enumval := 12; END IF;
      IF j->'cause'->>0 IS NOT NULL AND j->'cause'#>>'{}' = 'SPECIAL_EVENT' THEN enumval := 13; END IF;
      IF enumval IS NULL AND jsonb_typeof(j->'cause') = 'number' THEN enumval := (j->'cause'->>0)::int; END IF;
      IF enumval IS NOT NULL THEN
        buf := "transit_realtime".pb_write_tag(buf, 6, 0);
        buf := "transit_realtime".pb_write_varint(buf, enumval);
      END IF;
  END IF;
  IF j ? 'effect' AND j->'effect' IS NOT NULL THEN
      enumval := NULL;
      IF j->'effect'->>0 IS NOT NULL AND j->'effect'#>>'{}' = 'NO_SERVICE' THEN enumval := 1; END IF;
      IF j->'effect'->>0 IS NOT NULL AND j->'effect'#>>'{}' = 'REDUCED_SERVICE' THEN enumval := 2; END IF;
      IF j->'effect'->>0 IS NOT NULL AND j->'effect'#>>'{}' = 'SIGNIFICANT_DELAYS' THEN enumval := 3; END IF;
      IF j->'effect'->>0 IS NOT NULL AND j->'effect'#>>'{}' = 'DETOUR' THEN enumval := 4; END IF;
      IF j->'effect'->>0 IS NOT NULL AND j->'effect'#>>'{}' = 'ADDITIONAL_SERVICE' THEN enumval := 5; END IF;
      IF j->'effect'->>0 IS NOT NULL AND j->'effect'#>>'{}' = 'MODIFIED_SERVICE' THEN enumval := 6; END IF;
      IF j->'effect'->>0 IS NOT NULL AND j->'effect'#>>'{}' = 'OTHER_EFFECT' THEN enumval := 7; END IF;
      IF j->'effect'->>0 IS NOT NULL AND j->'effect'#>>'{}' = 'UNKNOWN_EFFECT' THEN enumval := 8; END IF;
      IF j->'effect'->>0 IS NOT NULL AND j->'effect'#>>'{}' = 'STOP_MOVED' THEN enumval := 9; END IF;
      IF j->'effect'->>0 IS NOT NULL AND j->'effect'#>>'{}' = 'NO_EFFECT' THEN enumval := 10; END IF;
      IF j->'effect'->>0 IS NOT NULL AND j->'effect'#>>'{}' = 'ACCESSIBILITY_ISSUE' THEN enumval := 11; END IF;
      IF enumval IS NULL AND jsonb_typeof(j->'effect') = 'number' THEN enumval := (j->'effect'->>0)::int; END IF;
      IF enumval IS NOT NULL THEN
        buf := "transit_realtime".pb_write_tag(buf, 7, 0);
        buf := "transit_realtime".pb_write_varint(buf, enumval);
      END IF;
  END IF;
  IF j ? 'url' AND j->'url' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 8, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_translatedstring"(j->'url'));
  END IF;
  IF j ? 'header_text' AND j->'header_text' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 10, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_translatedstring"(j->'header_text'));
  END IF;
  IF j ? 'description_text' AND j->'description_text' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 11, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_translatedstring"(j->'description_text'));
  END IF;
  IF j ? 'tts_header_text' AND j->'tts_header_text' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 12, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_translatedstring"(j->'tts_header_text'));
  END IF;
  IF j ? 'tts_description_text' AND j->'tts_description_text' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 13, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_translatedstring"(j->'tts_description_text'));
  END IF;
  IF j ? 'severity_level' AND j->'severity_level' IS NOT NULL THEN
      enumval := NULL;
      IF j->'severity_level'->>0 IS NOT NULL AND j->'severity_level'#>>'{}' = 'UNKNOWN_SEVERITY' THEN enumval := 1; END IF;
      IF j->'severity_level'->>0 IS NOT NULL AND j->'severity_level'#>>'{}' = 'INFO' THEN enumval := 2; END IF;
      IF j->'severity_level'->>0 IS NOT NULL AND j->'severity_level'#>>'{}' = 'WARNING' THEN enumval := 3; END IF;
      IF j->'severity_level'->>0 IS NOT NULL AND j->'severity_level'#>>'{}' = 'SEVERE' THEN enumval := 4; END IF;
      IF enumval IS NULL AND jsonb_typeof(j->'severity_level') = 'number' THEN enumval := (j->'severity_level'->>0)::int; END IF;
      IF enumval IS NOT NULL THEN
        buf := "transit_realtime".pb_write_tag(buf, 14, 0);
        buf := "transit_realtime".pb_write_varint(buf, enumval);
      END IF;
  END IF;
  IF j ? 'cause_detail' AND j->'cause_detail' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 17, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_translatedstring"(j->'cause_detail'));
  END IF;
  IF j ? 'effect_detail' AND j->'effect_detail' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 18, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_translatedstring"(j->'effect_detail'));
  END IF;
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."encode_shape"(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  buf bytea := ''::bytea;
  elem jsonb;
  k text;
  v jsonb;
  entrybuf bytea;
  enumval int;
  tmp numeric;
BEGIN
  IF j IS NULL OR jsonb_typeof(j) <> 'object' THEN
    RETURN buf;
  END IF;
  IF j ? 'shape_id' AND j->'shape_id' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 1, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'shape_id'->>0);
  END IF;
  IF j ? 'encoded_polyline' AND j->'encoded_polyline' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 2, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'encoded_polyline'->>0);
  END IF;
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."encode_stop"(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  buf bytea := ''::bytea;
  elem jsonb;
  k text;
  v jsonb;
  entrybuf bytea;
  enumval int;
  tmp numeric;
BEGIN
  IF j IS NULL OR jsonb_typeof(j) <> 'object' THEN
    RETURN buf;
  END IF;
  IF j ? 'stop_id' AND j->'stop_id' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 1, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'stop_id'->>0);
  END IF;
  IF j ? 'stop_code' AND j->'stop_code' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 2, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_translatedstring"(j->'stop_code'));
  END IF;
  IF j ? 'stop_name' AND j->'stop_name' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 3, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_translatedstring"(j->'stop_name'));
  END IF;
  IF j ? 'tts_stop_name' AND j->'tts_stop_name' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 4, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_translatedstring"(j->'tts_stop_name'));
  END IF;
  IF j ? 'stop_desc' AND j->'stop_desc' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 5, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_translatedstring"(j->'stop_desc'));
  END IF;
  IF j ? 'stop_lat' AND j->'stop_lat' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 6, 5);
      buf := "transit_realtime".pb_write_float4(buf, (j->'stop_lat')::float8);
  END IF;
  IF j ? 'stop_lon' AND j->'stop_lon' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 7, 5);
      buf := "transit_realtime".pb_write_float4(buf, (j->'stop_lon')::float8);
  END IF;
  IF j ? 'zone_id' AND j->'zone_id' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 8, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'zone_id'->>0);
  END IF;
  IF j ? 'stop_url' AND j->'stop_url' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 9, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_translatedstring"(j->'stop_url'));
  END IF;
  IF j ? 'parent_station' AND j->'parent_station' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 11, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'parent_station'->>0);
  END IF;
  IF j ? 'stop_timezone' AND j->'stop_timezone' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 12, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'stop_timezone'->>0);
  END IF;
  IF j ? 'wheelchair_boarding' AND j->'wheelchair_boarding' IS NOT NULL THEN
      enumval := NULL;
      IF j->'wheelchair_boarding'->>0 IS NOT NULL AND j->'wheelchair_boarding'#>>'{}' = 'UNKNOWN' THEN enumval := 0; END IF;
      IF j->'wheelchair_boarding'->>0 IS NOT NULL AND j->'wheelchair_boarding'#>>'{}' = 'AVAILABLE' THEN enumval := 1; END IF;
      IF j->'wheelchair_boarding'->>0 IS NOT NULL AND j->'wheelchair_boarding'#>>'{}' = 'NOT_AVAILABLE' THEN enumval := 2; END IF;
      IF enumval IS NULL AND jsonb_typeof(j->'wheelchair_boarding') = 'number' THEN enumval := (j->'wheelchair_boarding'->>0)::int; END IF;
      IF enumval IS NOT NULL THEN
        buf := "transit_realtime".pb_write_tag(buf, 13, 0);
        buf := "transit_realtime".pb_write_varint(buf, enumval);
      END IF;
  END IF;
  IF j ? 'level_id' AND j->'level_id' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 14, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'level_id'->>0);
  END IF;
  IF j ? 'platform_code' AND j->'platform_code' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 15, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_translatedstring"(j->'platform_code'));
  END IF;
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."encode_stopselector"(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  buf bytea := ''::bytea;
  elem jsonb;
  k text;
  v jsonb;
  entrybuf bytea;
  enumval int;
  tmp numeric;
BEGIN
  IF j IS NULL OR jsonb_typeof(j) <> 'object' THEN
    RETURN buf;
  END IF;
  IF j ? 'stop_sequence' AND j->'stop_sequence' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 1, 0);
      buf := "transit_realtime".pb_write_varint(buf, (j->'stop_sequence')::numeric);
  END IF;
  IF j ? 'stop_id' AND j->'stop_id' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 2, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'stop_id'->>0);
  END IF;
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."encode_replacementstop"(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  buf bytea := ''::bytea;
  elem jsonb;
  k text;
  v jsonb;
  entrybuf bytea;
  enumval int;
  tmp numeric;
BEGIN
  IF j IS NULL OR jsonb_typeof(j) <> 'object' THEN
    RETURN buf;
  END IF;
  IF j ? 'travel_time_to_stop' AND j->'travel_time_to_stop' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 1, 0);
      buf := "transit_realtime".pb_write_varint(buf, (j->'travel_time_to_stop')::numeric);
  END IF;
  IF j ? 'stop_id' AND j->'stop_id' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 2, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'stop_id'->>0);
  END IF;
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."encode_tripmodifications_modification"(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  buf bytea := ''::bytea;
  elem jsonb;
  k text;
  v jsonb;
  entrybuf bytea;
  enumval int;
  tmp numeric;
BEGIN
  IF j IS NULL OR jsonb_typeof(j) <> 'object' THEN
    RETURN buf;
  END IF;
  IF j ? 'start_stop_selector' AND j->'start_stop_selector' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 1, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_stopselector"(j->'start_stop_selector'));
  END IF;
  IF j ? 'end_stop_selector' AND j->'end_stop_selector' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 2, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_stopselector"(j->'end_stop_selector'));
  END IF;
  IF j ? 'propagated_modification_delay' AND j->'propagated_modification_delay' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 3, 0);
      buf := "transit_realtime".pb_write_varint(buf, (j->'propagated_modification_delay')::numeric);
  END IF;
  IF j ? 'replacement_stops' AND j->'replacement_stops' IS NOT NULL AND jsonb_typeof(j->'replacement_stops') = 'array' THEN
    FOR elem IN SELECT jsonb_array_elements(j->'replacement_stops') LOOP
      buf := "transit_realtime".pb_write_tag(buf, 4, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_replacementstop"(elem));
    END LOOP;
  END IF;
  IF j ? 'service_alert_id' AND j->'service_alert_id' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 5, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'service_alert_id'->>0);
  END IF;
  IF j ? 'last_modified_time' AND j->'last_modified_time' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 6, 0);
      buf := "transit_realtime".pb_write_varint(buf, (j->'last_modified_time')::numeric);
  END IF;
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."encode_tripmodifications_selectedtrips"(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  buf bytea := ''::bytea;
  elem jsonb;
  k text;
  v jsonb;
  entrybuf bytea;
  enumval int;
  tmp numeric;
BEGIN
  IF j IS NULL OR jsonb_typeof(j) <> 'object' THEN
    RETURN buf;
  END IF;
  IF j ? 'trip_ids' AND j->'trip_ids' IS NOT NULL AND jsonb_typeof(j->'trip_ids') = 'array' THEN
    FOR elem IN SELECT jsonb_array_elements(j->'trip_ids') LOOP
      buf := "transit_realtime".pb_write_tag(buf, 1, 2);
      buf := "transit_realtime".pb_write_string(buf, elem->>0);
    END LOOP;
  END IF;
  IF j ? 'shape_id' AND j->'shape_id' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 2, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'shape_id'->>0);
  END IF;
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."encode_tripmodifications"(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  buf bytea := ''::bytea;
  elem jsonb;
  k text;
  v jsonb;
  entrybuf bytea;
  enumval int;
  tmp numeric;
BEGIN
  IF j IS NULL OR jsonb_typeof(j) <> 'object' THEN
    RETURN buf;
  END IF;
  IF j ? 'selected_trips' AND j->'selected_trips' IS NOT NULL AND jsonb_typeof(j->'selected_trips') = 'array' THEN
    FOR elem IN SELECT jsonb_array_elements(j->'selected_trips') LOOP
      buf := "transit_realtime".pb_write_tag(buf, 1, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_tripmodifications_selectedtrips"(elem));
    END LOOP;
  END IF;
  IF j ? 'start_times' AND j->'start_times' IS NOT NULL AND jsonb_typeof(j->'start_times') = 'array' THEN
    FOR elem IN SELECT jsonb_array_elements(j->'start_times') LOOP
      buf := "transit_realtime".pb_write_tag(buf, 2, 2);
      buf := "transit_realtime".pb_write_string(buf, elem->>0);
    END LOOP;
  END IF;
  IF j ? 'service_dates' AND j->'service_dates' IS NOT NULL AND jsonb_typeof(j->'service_dates') = 'array' THEN
    FOR elem IN SELECT jsonb_array_elements(j->'service_dates') LOOP
      buf := "transit_realtime".pb_write_tag(buf, 3, 2);
      buf := "transit_realtime".pb_write_string(buf, elem->>0);
    END LOOP;
  END IF;
  IF j ? 'modifications' AND j->'modifications' IS NOT NULL AND jsonb_typeof(j->'modifications') = 'array' THEN
    FOR elem IN SELECT jsonb_array_elements(j->'modifications') LOOP
      buf := "transit_realtime".pb_write_tag(buf, 4, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_tripmodifications_modification"(elem));
    END LOOP;
  END IF;
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."encode_feedentity"(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  buf bytea := ''::bytea;
  elem jsonb;
  k text;
  v jsonb;
  entrybuf bytea;
  enumval int;
  tmp numeric;
BEGIN
  IF j IS NULL OR jsonb_typeof(j) <> 'object' THEN
    RETURN buf;
  END IF;
  IF j ? 'id' AND j->'id' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 1, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'id'->>0);
  END IF;
  IF j ? 'is_deleted' AND j->'is_deleted' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 2, 0);
      buf := "transit_realtime".pb_write_varint(buf, CASE WHEN (j->'is_deleted')::boolean THEN 1 ELSE 0 END);
  END IF;
  IF j ? 'trip_update' AND j->'trip_update' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 3, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_tripupdate"(j->'trip_update'));
  END IF;
  IF j ? 'vehicle' AND j->'vehicle' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 4, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_vehicleposition"(j->'vehicle'));
  END IF;
  IF j ? 'alert' AND j->'alert' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 5, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_alert"(j->'alert'));
  END IF;
  IF j ? 'shape' AND j->'shape' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 6, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_shape"(j->'shape'));
  END IF;
  IF j ? 'stop' AND j->'stop' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 7, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_stop"(j->'stop'));
  END IF;
  IF j ? 'trip_modifications' AND j->'trip_modifications' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 8, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_tripmodifications"(j->'trip_modifications'));
  END IF;
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."encode_feedmessage"(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  buf bytea := ''::bytea;
  elem jsonb;
  k text;
  v jsonb;
  entrybuf bytea;
  enumval int;
  tmp numeric;
BEGIN
  IF j IS NULL OR jsonb_typeof(j) <> 'object' THEN
    RETURN buf;
  END IF;
  IF j ? 'header' AND j->'header' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 1, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_feedheader"(j->'header'));
  END IF;
  IF j ? 'entity' AND j->'entity' IS NOT NULL AND jsonb_typeof(j->'entity') = 'array' THEN
    FOR elem IN SELECT jsonb_array_elements(j->'entity') LOOP
      buf := "transit_realtime".pb_write_tag(buf, 2, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_feedentity"(elem));
    END LOOP;
  END IF;
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."encode_translatedimage_localizedimage"(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  buf bytea := ''::bytea;
  elem jsonb;
  k text;
  v jsonb;
  entrybuf bytea;
  enumval int;
  tmp numeric;
BEGIN
  IF j IS NULL OR jsonb_typeof(j) <> 'object' THEN
    RETURN buf;
  END IF;
  IF j ? 'url' AND j->'url' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 1, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'url'->>0);
  END IF;
  IF j ? 'media_type' AND j->'media_type' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 2, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'media_type'->>0);
  END IF;
  IF j ? 'language' AND j->'language' IS NOT NULL THEN
      buf := "transit_realtime".pb_write_tag(buf, 3, 2);
      buf := "transit_realtime".pb_write_string(buf, j->'language'->>0);
  END IF;
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION "transit_realtime"."encode_translatedimage"(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  buf bytea := ''::bytea;
  elem jsonb;
  k text;
  v jsonb;
  entrybuf bytea;
  enumval int;
  tmp numeric;
BEGIN
  IF j IS NULL OR jsonb_typeof(j) <> 'object' THEN
    RETURN buf;
  END IF;
  IF j ? 'localized_image' AND j->'localized_image' IS NOT NULL AND jsonb_typeof(j->'localized_image') = 'array' THEN
    FOR elem IN SELECT jsonb_array_elements(j->'localized_image') LOOP
      buf := "transit_realtime".pb_write_tag(buf, 1, 2);
      buf := "transit_realtime".pb_write_bytes(buf, "transit_realtime"."encode_translatedimage_localizedimage"(elem));
    END LOOP;
  END IF;
  RETURN buf;
END;
$$;
