# `upb_Message_Convert`

This document summarizes the conversion algorithm implemented in
`upb_Message_Convert`.

## Objective

The `upb_Message_Convert` API efficiently converts a upb message from one
`upb_MiniTable` representation to another compatible one. Compatible minitables
typically arise from tree-shaking or subsets of the same underlying protocol
buffer definition.

Conceptually, it achieves the same result as serializing the source message and
decoding it against the destination schema, but it is optimized for speed and
memory usage by leveraging zero-copy aliasing (shallow copying) wherever
possible.

## Design Principles

1.  **Shallow Copying (Aliasing)**: If submessages, strings, or unknown fields
    share the same minitable definition (by pointer identity) between source and
    destination schemas, the algorithm avoids deep copies and instead aliases
    the source memory.
2.  **Destination Unknown Encoding**: Data present in the source message but
    absent from the destination schema (e.g., dropped fields, unknown
    extensions, invalid enum values) is not discarded. Instead, it is encoded
    and appended to the destination message's unknown fields.
3.  **Source Unknown Decoding**: Unknown fields in the source message are parsed
    against the destination schema, allowing previously unknown data to be
    parsed using the destination schema.

## Algorithm Walkthrough

The conversion process is driven by `upb_Message_ConvertInternal`, which
performs a synchronized, descending traversal of the fields in both source and
destination minitables.

### 1. Field Traversal

Minitable fields are assumed to be sorted by field number. The algorithm
iterates through both source and destination field lists from highest field
number to lowest.

At each step, it compares the current source field number (`src_nr`) and
destination field number (`dst_nr`):

*   **`dst_nr == src_nr` (Field Match)**: The fields exist in both schemas. The
    algorithm verifies basic compatibility (type, array/map cardinality) and
    proceeds to **Field Conversion**. Both pointers advance.

*   **`dst_nr > src_nr` (Destination-Only Field)**: The destination schema has a
    field not present in the source. This field is skipped and remains
    unset/default in the destination message. The destination pointer advances.

*   **`dst_nr < src_nr` (Source-Only Field)**: The source message contains a
    field dropped from the destination schema. The algorithm encodes this
    field's data from the source message and appends the resulting wire bytes to
    the destination message's unknown fields. The source pointer advances.

### 2. Field Conversion (`upb_Message_ConvertField`)

For matched fields, conversion depends on the field type and presence:

*   **Basic Check**: If the source field is unset (or contains default zero
    values for proto3 implicit scalars), no action is taken.

*   **Primitive Types**: Data is copied directly. Presence bits are updated in
    the destination if required.

*   **Submessages**:

    *   **Identical Schema (`src_sub_mt == dst_sub_mt`)**: Performs a shallow
        copy by copying the message pointer.
    *   **Differing Schema**: Allocates a new submessage and recursively invokes
        the conversion algorithm.

*   **Enums (Closed)**:

    *   Validates the source enum value against the destination enum table.
    *   If valid, copies the value.
    *   If invalid (e.g., source schema allowed values dropped in destination),
        encodes the enum value and adds it to the destination's unknown fields.

*   **Arrays and Maps**:

    *   If element/entry schemas are identical and do not require validation
        (like closed enums), the entire container pointer is shallow-copied.
    *   Otherwise, performs a deep conversion: allocates a new container,
        iterates elements, applies recursive conversion to submessages, and
        filters invalid enum values (moving them to the parent message's unknown
        fields).

### 3. Extension Handling

If the source message contains extensions, they are processed after regular
fields:

*   The algorithm attempts to map each source extension to a known field or
    extension in the destination schema (consulting the `ExtensionRegistry` if
    necessary).

*   **Mapped**: Converted similarly to regular fields (supporting shallow copies
    for identical submessage types).

*   **Unmapped (Dropped)**: * If the destination message does *not* support
    extensions, the extension is encoded and added to unknown fields. * If the
    destination supports extensions, the extension structure itself is carried
    over (shallow copy).

### 4. Unknown Fields Processing

Finally, the algorithm iterates over the raw unknown fields accumulated in the
source message. It feeds these wire bytes into a decoder configured with the
destination minitable. * If the destination schema recognizes any of these
fields, they are successfully decoded and populated as structured data in the
destination message. * Remaining unparseable bytes are retained as unknown
fields in the destination.

## Notes on Closed Enum Handling

The implementation strictly adheres to the protocol buffer specification for
closed enums
([go/proto-enum-behavior#implications-of-closed-enums](https://goto.google.com/proto-enum-behavior#implications-of-closed-enums)).
When converting into a destination schema where an enum is closed, unknown or
invalid integer values are stripped from structured fields and preserved in the
unknown field set:

*   **Singular Fields**: If an enum value is invalid in the destination schema,
    it is varint-encoded (tag + value) and appended to the destination message's
    unknown fields. The structured field remains unset, causing accessors to
    return the default value.
*   **Repeated Fields**: When deep-converting an array of closed enums
    (`upb_Array_DeepConvert`), valid elements are copied into the destination
    array while invalid elements are filtered out and varint-encoded directly
    into the parent message's unknown fields buffer. This ensures known values
    remain accessible without breaking array indexing, while unknown values are
    deferred to reserialization.
*   **Map Values**: For maps where values are closed enums
    (`upb_Map_DeepConvert`), an invalid enum value invalidates the entire map
    entry. The algorithm constructs a standalone key-value entry submessage
    containing the original key and invalid value, then serializes the entire
    entry as a length-delimited unknown field on the parent message.

## Notes on Non-Canonical Extension Handling

When an extension present in the source message is unrecognized by the
destination schema (and extension registry), standard protocol buffer semantics
dictate that unrecognized data should be stored as raw bytes in the unknown
field set.

However, for performance optimization, `upb_Message_Convert` handles this
scenario differently when the destination message supports extensions:

*   Instead of serializing the unrecognized extension into wire bytes during
    conversion, the algorithm directly copies the internal extension structure
    (`upb_MiniTableExtension*` and value) over to the destination message as a
    shallow copy.

*   This unrecognized extension is maintained internally as a "non-canonical
    extension" on the destination message.

*   When the destination message is eventually serialized to wire format, the
    encoder processes this non-canonical extension, naturally producing the
    exact same serialized wire output as if it had been stored in the raw
    unknown fields buffer.

This zero-copy optimization significantly reduces CPU cycles and memory overhead
during conversion. We are working on [b/510055656](http://b/510055656) to clean
up the `extendee` metadata tracking for non-canonical extensions.

## Error Handling & Fallback

The API returns a new `upb_Message*` on success, or `NULL` on failure (e.g., out
of memory, schema incompatibility, max recursion depth exceeded).

Callers are advised that `NULL` may be returned even for valid inputs if edge
cases are encountered. A robust integration should catch `NULL` returns and
fallback to a standard encoding-then-decoding approach.
