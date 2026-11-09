package main

const wktEncodeTemplate = `
CREATE OR REPLACE FUNCTION {SCHEMA}.encode_google_protobuf_empty(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
BEGIN
  RETURN ''::bytea;
END;
$$;

CREATE OR REPLACE FUNCTION {SCHEMA}.encode_google_protobuf_boolvalue(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE buf bytea := ''::bytea;
BEGIN
  IF j IS NULL THEN RETURN buf; END IF;
  buf := {SCHEMA}.pb_write_tag(buf, 1, 0);
  buf := {SCHEMA}.pb_write_varint(buf, CASE WHEN j::boolean THEN 1 ELSE 0 END);
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION {SCHEMA}.encode_google_protobuf_int32value(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE buf bytea := ''::bytea;
BEGIN
  IF j IS NULL THEN RETURN buf; END IF;
  buf := {SCHEMA}.pb_write_tag(buf, 1, 0);
  buf := {SCHEMA}.pb_write_varint(buf, j::numeric);
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION {SCHEMA}.encode_google_protobuf_int64value(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE buf bytea := ''::bytea;
BEGIN
  IF j IS NULL THEN RETURN buf; END IF;
  buf := {SCHEMA}.pb_write_tag(buf, 1, 0);
  buf := {SCHEMA}.pb_write_varint(buf, j::numeric);
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION {SCHEMA}.encode_google_protobuf_uint32value(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE buf bytea := ''::bytea;
BEGIN
  IF j IS NULL THEN RETURN buf; END IF;
  buf := {SCHEMA}.pb_write_tag(buf, 1, 0);
  buf := {SCHEMA}.pb_write_varint(buf, j::numeric);
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION {SCHEMA}.encode_google_protobuf_uint64value(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE buf bytea := ''::bytea;
BEGIN
  IF j IS NULL THEN RETURN buf; END IF;
  buf := {SCHEMA}.pb_write_tag(buf, 1, 0);
  buf := {SCHEMA}.pb_write_varint(buf, j::numeric);
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION {SCHEMA}.encode_google_protobuf_floatvalue(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE buf bytea := ''::bytea;
BEGIN
  IF j IS NULL THEN RETURN buf; END IF;
  buf := {SCHEMA}.pb_write_tag(buf, 1, 5);
  buf := {SCHEMA}.pb_write_float4(buf, j::float8);
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION {SCHEMA}.encode_google_protobuf_doublevalue(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE buf bytea := ''::bytea;
BEGIN
  IF j IS NULL THEN RETURN buf; END IF;
  buf := {SCHEMA}.pb_write_tag(buf, 1, 1);
  buf := {SCHEMA}.pb_write_float8(buf, j::float8);
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION {SCHEMA}.encode_google_protobuf_stringvalue(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE buf bytea := ''::bytea;
BEGIN
  IF j IS NULL THEN RETURN buf; END IF;
  buf := {SCHEMA}.pb_write_tag(buf, 1, 2);
  buf := {SCHEMA}.pb_write_string(buf, j #>> '{}');
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION {SCHEMA}.encode_google_protobuf_bytesvalue(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE buf bytea := ''::bytea;
BEGIN
  IF j IS NULL THEN RETURN buf; END IF;
  buf := {SCHEMA}.pb_write_tag(buf, 1, 2);
  buf := {SCHEMA}.pb_write_bytes(buf, decode(j #>> '{}', 'base64'));
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION {SCHEMA}.encode_google_protobuf_timestamp(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE buf bytea := ''::bytea;
BEGIN
  IF j IS NULL OR jsonb_typeof(j) <> 'object' THEN RETURN buf; END IF;
  IF j ? 'seconds' AND j->'seconds' IS NOT NULL THEN
    buf := {SCHEMA}.pb_write_tag(buf, 1, 0);
    buf := {SCHEMA}.pb_write_varint(buf, (j->>'seconds')::numeric);
  END IF;
  IF j ? 'nanos' AND j->'nanos' IS NOT NULL THEN
    buf := {SCHEMA}.pb_write_tag(buf, 2, 0);
    buf := {SCHEMA}.pb_write_varint(buf, (j->>'nanos')::numeric);
  END IF;
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION {SCHEMA}.encode_google_protobuf_duration(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE buf bytea := ''::bytea;
BEGIN
  IF j IS NULL OR jsonb_typeof(j) <> 'object' THEN RETURN buf; END IF;
  IF j ? 'seconds' AND j->'seconds' IS NOT NULL THEN
    buf := {SCHEMA}.pb_write_tag(buf, 1, 0);
    buf := {SCHEMA}.pb_write_varint(buf, (j->>'seconds')::numeric);
  END IF;
  IF j ? 'nanos' AND j->'nanos' IS NOT NULL THEN
    buf := {SCHEMA}.pb_write_tag(buf, 2, 0);
    buf := {SCHEMA}.pb_write_varint(buf, (j->>'nanos')::numeric);
  END IF;
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION {SCHEMA}.encode_google_protobuf_value(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE buf bytea := ''::bytea;
BEGIN
  IF j IS NULL OR j = 'null'::jsonb THEN
    buf := {SCHEMA}.pb_write_tag(buf, 1, 0);
    buf := {SCHEMA}.pb_write_varint(buf, 0);
    RETURN buf;
  END IF;
  IF jsonb_typeof(j) = 'boolean' THEN
    buf := {SCHEMA}.pb_write_tag(buf, 4, 0);
    buf := {SCHEMA}.pb_write_varint(buf, CASE WHEN j::boolean THEN 1 ELSE 0 END);
  ELSIF jsonb_typeof(j) = 'number' THEN
    buf := {SCHEMA}.pb_write_tag(buf, 2, 1);
    buf := {SCHEMA}.pb_write_float8(buf, j::float8);
  ELSIF jsonb_typeof(j) = 'string' THEN
    buf := {SCHEMA}.pb_write_tag(buf, 3, 2);
    buf := {SCHEMA}.pb_write_string(buf, j #>> '{}');
  ELSIF jsonb_typeof(j) = 'object' THEN
    buf := {SCHEMA}.pb_write_tag(buf, 5, 2);
    buf := {SCHEMA}.pb_write_bytes(buf, {SCHEMA}.encode_google_protobuf_struct(j));
  ELSIF jsonb_typeof(j) = 'array' THEN
    buf := {SCHEMA}.pb_write_tag(buf, 6, 2);
    buf := {SCHEMA}.pb_write_bytes(buf, {SCHEMA}.encode_google_protobuf_listvalue(j));
  END IF;
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION {SCHEMA}.encode_google_protobuf_struct(j jsonb) RETURNS bytea
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
    entrybuf := {SCHEMA}.pb_write_tag(entrybuf, 1, 2);
    entrybuf := {SCHEMA}.pb_write_string(entrybuf, k);
    entrybuf := {SCHEMA}.pb_write_tag(entrybuf, 2, 2);
    entrybuf := {SCHEMA}.pb_write_bytes(entrybuf, {SCHEMA}.encode_google_protobuf_value(v));
    buf := {SCHEMA}.pb_write_tag(buf, 1, 2);
    buf := {SCHEMA}.pb_write_bytes(buf, entrybuf);
  END LOOP;
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION {SCHEMA}.encode_google_protobuf_listvalue(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE buf bytea := ''::bytea; elem jsonb;
BEGIN
  IF j IS NULL OR jsonb_typeof(j) <> 'array' THEN RETURN buf; END IF;
  FOR elem IN SELECT jsonb_array_elements(j) LOOP
    buf := {SCHEMA}.pb_write_tag(buf, 1, 2);
    buf := {SCHEMA}.pb_write_bytes(buf, {SCHEMA}.encode_google_protobuf_value(elem));
  END LOOP;
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION {SCHEMA}.encode_google_protobuf_any(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE buf bytea := ''::bytea;
BEGIN
  IF j IS NULL OR jsonb_typeof(j) <> 'object' THEN RETURN buf; END IF;
  IF j ? 'type_url' THEN
    buf := {SCHEMA}.pb_write_tag(buf, 1, 2);
    buf := {SCHEMA}.pb_write_string(buf, j->>'type_url');
  END IF;
  IF j ? 'value' THEN
    buf := {SCHEMA}.pb_write_tag(buf, 2, 2);
    buf := {SCHEMA}.pb_write_bytes(buf, decode(j->>'value', 'base64'));
  END IF;
  RETURN buf;
END;
$$;

CREATE OR REPLACE FUNCTION {SCHEMA}.encode_google_protobuf_fieldmask(j jsonb) RETURNS bytea
LANGUAGE plpgsql IMMUTABLE STRICT AS $$
DECLARE buf bytea := ''::bytea; elem text;
BEGIN
  IF j IS NULL THEN RETURN buf; END IF;
  IF jsonb_typeof(j) = 'array' THEN
    FOR elem IN SELECT jsonb_array_elements_text(j) LOOP
      buf := {SCHEMA}.pb_write_tag(buf, 1, 2);
      buf := {SCHEMA}.pb_write_string(buf, elem);
    END LOOP;
  ELSIF jsonb_typeof(j) = 'string' THEN
    buf := {SCHEMA}.pb_write_tag(buf, 1, 2);
    buf := {SCHEMA}.pb_write_string(buf, j #>> '{}');
  END IF;
  RETURN buf;
END;
$$;
`
