"""Starlark helpers for Objective-C protos."""

# State constants for objc_proto_camel_case_name.
_last_was_other = 0
_last_was_lowercase = 1
_last_was_uppercase = 2
_last_was_number = 3

def objc_proto_camel_case_name(name):
    """A Starlark version of the ObjC generator's CamelCase name transform.

    This needs to track
    src/google/protobuf/compiler/objectivec/names.cc's UnderscoresToCamelCase()

    NOTE: This code is written to model the C++ in protoc's ObjC generator so it
    is easier to confirm that the algorithms between the implementations match.
    The customizations for starlark performance are:
    - The cascade within the loop is reordered and special cases "_" to
      optimize for google3 inputs.
    - The "last was" state is handled via integers instead of three booleans.

    The `first_capitalized` argument in the C++ code is left off this code and
    it acts as if the value were `True`.

    Args:
      name: The proto file name to convert to camel case. The extension should
        already be removed.

    Returns:
      The converted proto name to camel case.
    """
    segments = []
    current = ""
    last_was = _last_was_other
    for c in name.elems():
        if c.islower():
            # lowercase letter can follow a lowercase or uppercase letter.
            if last_was != _last_was_lowercase and last_was != _last_was_uppercase:
                segments.append(current)
                current = c.upper()
            else:
                current += c
            last_was = _last_was_lowercase
        elif c == "_":  # more common than rest, special case it.
            last_was = _last_was_other
        elif c.isdigit():
            if last_was != _last_was_number:
                segments.append(current)
                current = ""
            current += c
            last_was = _last_was_number
        elif c.isupper():
            if last_was != _last_was_uppercase:
                segments.append(current)
                current = c
            else:
                current += c.lower()
            last_was = _last_was_uppercase
        else:
            last_was = _last_was_other
    segments.append(current)
    result = ""
    for x in segments:
        if x in ("Url", "Http", "Https"):
            result += x.upper()
        else:
            result += x
    return result
