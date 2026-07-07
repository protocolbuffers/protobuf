# Security Policy

This document describes the security model, threat boundaries, and security
recommendations for Protobuf. It is intended to help developers understand what
security guarantees Protobuf provides, and how to safely parse and serialize
messages.

--------------------------------------------------------------------------------

## Reporting a Vulnerability

If you believe you have discovered a security vulnerability in Protobuf, please
report it via
[Google's official vulnerability disclosure channel](https://bughunters.google.com/report).

For reports that do not meet the criteria or bar for the Google Vulnerability
Reward Program (VRP) but are still security-sensitive, you can privately report
them by opening a
[draft GitHub Security Advisory](https://github.com/protocolbuffers/protobuf/security/advisories/new).

If an issue relates to something which is listed as Best Effort or Out of Threat
Model below, please open a
[public issue](https://github.com/protocolbuffers/protobuf/issues) instead.

--------------------------------------------------------------------------------

## Best Practices

*   Handle `.proto` files as 'code' by default (similarly to how you would
    handle `.java` or `.cc` files).
*   Prefer to use the binary wire format where possible (all Google-maintained
    Protobuf implementations treat binary wire format parsing as the primary use
    case to handle untrusted inputs).
*   **JavaProto** (including the Kotlin bindings) is recommended as the default
    implementation for the best security posture, as it benefits from the JVM's
    memory safety guarantees. **C++Proto** is recommended for applications
    requiring optimal performance, but developers should remain aware of native
    memory management risks.
*   **Transport Configuration:** Prefer to use gRPC-over-HTTPS with default gRPC
    configurations, including payload size limits.
*   **Version Matching:** Ensure that the generated code and the runtime library
    version match exactly and are kept up to date with the latest patch
    releases. Certain obscure and low severity issues may only be patched on the
    latest release.

## Threat Model and Security Boundaries

Protobuf is a serialization format designed to parse data efficiently. Depending
on the input format, language runtime, and integration context, different parts
of the Protobuf ecosystem are hardened against adversarial inputs.

Our threat model divides surfaces into three tiers:

### Hardened

These surfaces are fully supported and hardened against adversarial inputs.

*   Security vulnerabilities identified in these paths are treated with high
    priority, actively fixed, and issued CVEs.

*   If necessary, Protobuf may make out-of-schedule breaking changes without
    bumping the major version to close security issues on this surface area.

*   Google trusts these surfaces enough to use in sensitive, publicly exposed
    endpoints without sandboxing.

### Best-Effort

Defensive hardening is applied to these surfaces, but with weaker guarantees
compared to our hardened surfaces.

*   Serious security issues are still highly prioritized for fixing on these
    surfaces.
*   We will not break our backwards compatibility guarantees to address more
    minor issues on these surfaces. In some cases this means low-impact known
    issues may even be left open if they cannot be closed without a breaking
    change.

*   When using these surfaces on untrusted inputs, application-level isolation
    (such as sandboxing) or other defensive handling is recommended.

### Outside of Threat Model

These are surfaces where Protobuf libraries are not intended to be used in
untrusted contexts at all.

In some cases we still apply defensive hardening even to components outside of
our threat model, but the higher priority for those cases is in topics like
stability, performance, and developer ergonomics.

## Hardened Use Cases

### Binary Wire Format over Public Networks **(Primary Use Case)**

This is considered the primary surface of security concern in Protobuf
libraries.

In this use case, the `.proto` schema files, the compiler (`protoc`), and the
generated code are fully trusted. The incoming binary wire format bytes are
untrusted and may be adversarial.

The parsing library will safely process or reject any arbitrary byte stream
without exposing the server to memory corruption, out-of-bounds reads/writes, or
remote code execution (RCE).

Note that the intended surface here is only parsing: once a message is in-memory
it is treated as a trusted object in our threat model. For that reason,
serialization or any other in-memory handling of parsed objects is not
considered a surface within the threat model. For example, parsing an untrusted
wire format payload should not be able to reach uncontrolled recursion, but
serializing an arbitrarily in-memory object may. This is similar in nature to
how modern browsers do not throw `RangeError` on `JSON.parse()`, but do on
`JSON.stringify()`.

### Parsing of ProtoJSON Format

[ProtoJSON](https://protobuf.dev/programming-guides/json/) allows using `.proto`
schemas with standard JSON encoding. This is considered an ancillary supported
encoding, and the binary format should be preferred where feasible.

Under the same threat model as publicly exposed binary wire format services,
ProtoJSON parsing is intended to be used with untrusted inputs. ProtoJSON
serialization is similarly not considered within the threat model risks.

### Setting Primitive Fields

Applications may receive raw, untrusted primitives (strings, bytes, or integers)
and programmatically assign them to fields using generated setters. Subsequent
handling of that message by a Protobuf library (including serialization) is
within threat model.

For example, if a ProtoJSON serializer failed to properly escape a string field,
that is a special case that is within threat model.

--------------------------------------------------------------------------------

## Best-Effort Hardened Surfaces

The Protobuf team welcomes bug reports and pull requests to improve the
hardening of these surfaces. However, we typically will not break backwards
compatibility guarantees to address security issues in these areas (especially
for lower severity risks).

These areas are defensive against malicious inputs, but application-level
defense (such as sandboxing) may be if exposed to untrusted environments.

### Parsing Text Format

Text Format is explicitly designed for local debugging, testing, and managing
trusted configurations by developers. It is not intended to be used as an
interchange format. It is not recommended to expose public services that use
Text Format as an encoding.

Note: Text Format parsing currently does not enforce any depth limit in several
supported runtimes. Those runtimes support opt-in depth limits. We cannot
enforce a depth limit by default without breaking backwards compatibility, but
may begin to enforce depth limits by default as part of a future breaking change
release.

### Using protoc to parse untrusted .proto files into FileDescriptors

The `protoc` CLI is an offline developer tool. `.proto` files are considered
source code (equivalent to `.java` or `.cpp` source files).

A supported modality of the CLI is to parse untrusted `proto` files to emit
`FileDescriptors` which can enable further machine processing of the schemas.

While `protoc` is hardened on a best-effort basis for this use case, we
recommend using defensive validation and sandboxing whenever running `protoc`
against potentially malicious inputs.

Any additional flags passed to `protoc` must not contain untrusted content as
CLI flags are never considered adversarial.

### Lite Runtimes (C++ Lite, Java Lite) DoS Risks

Lite runtimes target mobile and web usage, and omit features like descriptors
and reflection.

Unlike with servers, the nature of mobile parsing makes them generally are not
sensitive to Denial of Service (DoS) risks. We still intend to address any such
risks, but Lite gencode/runtimes are not recommended for servers exposing public
endpoints.

### 'Wrong Exceptions Thrown'

In memory-safe runtimes (Java, Go, Python, C#), reaching a catchable exception
or runtime error (such as `IndexOutOfBoundsException` or `StackOverflowError`)
instead of the declared exception (such as `InvalidProtocolBufferException`) is
treated as an important bug to fix, but is considered to be a minor security
concern relative to serious issues like native heap corruption, data leakage,
remote code execution, or DoS vectors from unbounded computation or memory use.

It is recommended that RPC handler code be defensive against unexpected
exceptions if they are exceptionally sensitive to such behavior.

### DynamicMessage and Reflection on Untrusted Descriptors

In reflection-based services where schema details (`FileDescriptorSet` or
`DescriptorProto`) are provided by an untrusted source, parsing untrusted bytes
reflectively into a `DynamicMessage` is best-effort supported.

## Outside of Threat Model (Not Hardened)

### Executing Language Gencode Generated from Untrusted Schemas

Protobuf includes features (such as custom options and code injection
mechanisms) that may intentionally allow `.proto` files to specify code to be
injected into the generated source code.

> [!CAUTION] Compiling and executing generated code from untrusted schemas is
> functionally equivalent to compiling and running arbitrary third-party `.java`
> or `.cpp` source code and executing it. Treat untrusted `.proto` files the
> same as any other programming language code in terms of supply-chain risk.

### Adversarial Application Code

Violating runtime API constraints or passing invalid arguments directly to the
C++ API is considered an application integration error rather than a library
vulnerability.

Protobuf libraries are not hardened against insider threats: we cannot mitigate
malicious code being committed by a trusted developer.

*   *Example:* Passing negative or invalid buffer size parameters directly to
    `ParseFromArray` in C++ is simply wrong application code and is not
    considered a surface area relevant for security.

### Differential Parsing

Differential parsing is a risk caused by two different libraries parsing the
same data differently. For some formats this is considered a security-sensitive
topic, because of risks of validating in a gateway, forwarding the data
unmodified, and then a backend handles the same payload and interprets the data
differently, bypassing the intended checks.

Differential parsing risks are not considered to be within Protobuf's security
threat model: there are certain edge-case byte sequences that are known to have
different interpretations in different parsers while still being within spec.

Note that there should be no such sequences that a spec-compliant parser would
emit: differential confusion in such a case would be considered a very high
priority misparse bug.

For JSON parsing, the underlying JSON format itself contains inherent
ambiguities (including situations of duplicate keys and numeric precision). As
ProtoJSON is built on top of that foundation, different ProtoJSON parsers will
interpret certain edge-case legal sequences differently while being within spec.

### Specific depth cap exceeded without uncontrolled recursion

Protobuf's parsers have depth limits (which are often configurable).

The purpose of these depth limits is to prevent uncontrolled recursion. To
generally maintain consistent and interoperable behavior, we intend these depth
limits to be consistent in behavior.

If an edge case allows content to be successfully parsed beyond the intended
depth limit without causing uncontrolled recursion, this is viewed as a simple
bug. We welcome reports and patches for issues of that nature, but do not view
it as a security concern.

### Canonical Representation and Signature Verification

There is **no canonical representation** of Protobuf messages.

*   **Deterministic Serialization:** Many runtimes support deterministic
    serialization, which guarantees that a given build of a binary will
    serialize the same message to the same sequence of bytes. However,
    deterministic does not mean canonical: rebuilding the binary, changing the
    compiler version, or minor schema modifications can legally result in an
    alternate serialized byte representation that would have the same
    interpretation when parsed. For more details, see
    [Protobuf Serialization is Not Canonical](https://protobuf.dev/programming-guides/serialization-not-canonical/).
*   **Recommendation:** Do not use the serialized byte output of Protobuf
    messages to compute stable cryptographic signatures. You may still sign a
    given encoded byte sequence, knowing that there are other byte sequences
    that would be equally valid representations of the same message. If you need
    a stable signature of a given message, you must define and implement your
    own canonicalization specification over the parsed message fields and not
    over the encoded messages.
