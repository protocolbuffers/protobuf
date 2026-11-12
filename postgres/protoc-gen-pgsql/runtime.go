package main

import (
	"fmt"
	"strings"

	"google.golang.org/protobuf/compiler/protogen"
)

const runtimeTemplate = `-- runtime helpers (schema: {SCHEMA})
CREATE SCHEMA IF NOT EXISTS {SCHEMA};

CREATE OR REPLACE FUNCTION {SCHEMA}.pb_read_varint(data bytea, INOUT p int, OUT val numeric)
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

CREATE OR REPLACE FUNCTION {SCHEMA}.pb_read_fixed32(data bytea, INOUT p int, OUT val numeric)
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

CREATE OR REPLACE FUNCTION {SCHEMA}.pb_read_fixed64(data bytea, INOUT p int, OUT val numeric)
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

CREATE OR REPLACE FUNCTION {SCHEMA}.pb_skip_field(data bytea, pos int, wire_type int, field_num int) RETURNS int
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
    SELECT val, p FROM {SCHEMA}.pb_read_varint(data, newpos) INTO v, newpos;
  ELSIF wire_type = 1 THEN
    newpos := newpos + 8;
  ELSIF wire_type = 5 THEN
    newpos := newpos + 4;
  ELSIF wire_type = 2 THEN
    SELECT val, p FROM {SCHEMA}.pb_read_varint(data, newpos) INTO v, newpos;
    n := v::int;
    newpos := newpos + n;
  ELSIF wire_type = 3 THEN
    LOOP
      IF newpos >= octet_length(data) THEN
        EXIT;
      END IF;
      SELECT val, p FROM {SCHEMA}.pb_read_varint(data, newpos) INTO t, newpos;
      fn := (t::bigint / 8)::int;
      wt := (t::numeric % 8)::int;
      IF wt = 4 AND fn = field_num THEN
        EXIT;
      END IF;
      newpos := {SCHEMA}.pb_skip_field(data, newpos, wt, fn);
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

CREATE OR REPLACE FUNCTION {SCHEMA}.pb_zigzag(v numeric) RETURNS numeric
LANGUAGE sql IMMUTABLE STRICT AS $$
  SELECT CASE WHEN (v::numeric % 2) = 0 THEN v/2 ELSE -((v+1)/2) END;
$$;
`

const floatRuntimeTemplate = `
CREATE OR REPLACE FUNCTION {SCHEMA}.pb_fixed32_to_float(v numeric) RETURNS numeric
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

CREATE OR REPLACE FUNCTION {SCHEMA}.pb_fixed64_to_double(v numeric) RETURNS numeric
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
`

func emitRuntime(out *protogen.GeneratedFile, schema string) {
	s := strings.ReplaceAll(runtimeTemplate, "{SCHEMA}", quoteIdent(schema))
	out.P(s)
	fs := strings.ReplaceAll(floatRuntimeTemplate, "{SCHEMA}", quoteIdent(schema))
	out.P(fs)
	es := strings.ReplaceAll(encodeRuntimeTemplate, "{SCHEMA}", quoteIdent(schema))
	out.P(es)
	fes := strings.ReplaceAll(floatEncodeRuntimeTemplate, "{SCHEMA}", quoteIdent(schema))
	out.P(fes)
}

var _ = fmt.Sprintf

const encodeRuntimeTemplate = `
CREATE OR REPLACE FUNCTION {SCHEMA}.pb_write_varint(buf bytea, val numeric) RETURNS bytea
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

CREATE OR REPLACE FUNCTION {SCHEMA}.pb_write_fixed32(buf bytea, val numeric) RETURNS bytea
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

CREATE OR REPLACE FUNCTION {SCHEMA}.pb_write_fixed64(buf bytea, val numeric) RETURNS bytea
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

CREATE OR REPLACE FUNCTION {SCHEMA}.pb_write_tag(buf bytea, field_num int, wire_type int) RETURNS bytea
LANGUAGE sql IMMUTABLE STRICT AS $$
  SELECT {SCHEMA}.pb_write_varint(buf, (field_num::numeric * 8 + wire_type));
$$;

CREATE OR REPLACE FUNCTION {SCHEMA}.pb_write_string(buf bytea, s text) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE
  data bytea := convert_to(COALESCE(s,''), 'UTF8');
BEGIN
  RETURN {SCHEMA}.pb_write_varint(buf, octet_length(data)::numeric) || data;
END;
$$;

CREATE OR REPLACE FUNCTION {SCHEMA}.pb_write_bytes(buf bytea, data bytea) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
BEGIN
  data := COALESCE(data, ''::bytea);
  RETURN {SCHEMA}.pb_write_varint(buf, octet_length(data)::numeric) || data;
END;
$$;
`

const floatEncodeRuntimeTemplate = `
CREATE OR REPLACE FUNCTION {SCHEMA}.pb_write_float4(buf bytea, val double precision) RETURNS bytea
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

CREATE OR REPLACE FUNCTION {SCHEMA}.pb_write_float8(buf bytea, val double precision) RETURNS bytea
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
`
