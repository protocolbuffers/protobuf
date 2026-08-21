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

*   Handle `.proto` files as "code" by default, similarly to the handling of
    `.java` or `.cc` files. Executing untrusted code can potentially be a supply
    chain attack vector.
*   Prefer to use the binary wire format encoding where possible: all
    Google-maintained Protobuf implementations treat binary wire format parsing
    as the primary use case to handle untrusted inputs. Other formats are
    considered secondary.
*   **Apply Defensive limits:** Prefer to have a layer that applies defensive
    limits or custom filtering around the Protobuf parse: for example gRPC
    enforces a 4 MiB payload limit by default. We recommend having sensible
    limits defensively applied on all untrusted inputs.
*   While all Google maintained Protobuf implementations are considered to be
    intended to be used with untrusted inputs, not all runtimes are equally
    hardened:
    *   **JavaProto** (including the Kotlin bindings) is recommended as the
        default implementation for the best security posture, as it benefits
        from the JVM's memory safety guarantees.
    *   **C++Proto** is recommended for applications requiring optimal
        performance, and has been extensively fuzzed and hardened such that
        Google uses it without sandboxing to parse untrusted binary format
        inputs in critical surfaces. When using C++, developers must remain
        aware of native memory management risks.
    *   In languages that have multiple supported implementations, strongly
        prefer to use the default implementation for best security. The
        non-default implementations are generally supported for more exotic
        use-cases and may not have as much hardening attention. The default
        Python and PHP runtimes use a C extension that has been hardened more
        than the fallback behaviors that that do not use any C extension.
*   **Use latest releases:** Ensure that the generated code and the runtime
    library version match exactly and are kept up to date with the latest patch
    release. Certain obscure and low severity issues may only be patched on the
    latest release.

## Threat Model and Security Boundaries

Protobuf is a serialization format designed to parse data efficiently. Depending
on the input format, language runtime, and integration context, different parts
of the Protobuf ecosystem are hardened against adversarial inputs.

Our threat model divides surfaces into three tiers:

### Proactively Hardened

These surfaces are fully supported and hardened against adversarial inputs.

*   Security vulnerabilities identified in these paths are treated with high
    priority, actively fixed, and issued CVEs.

*   Where necessary, certain patches that are technically breaking may be done
    if it is inherently necessary to close a security issue. This also includes
    that gencode-runtime version compatibility guarantees may be broken if it is
    strictly necessary to close a security issue (see documentation
    [here](https://protobuf.dev/support/cross-version-runtime-guarantee/#exception)).
    These cases are rare and we will avoid this wherever it is possible to close
    the security concern without a breaking change.

*   Google trusts these surfaces enough to use in sensitive, publicly exposed
    endpoints without sandboxing.

### Reactively hardened

The Protobuf team welcomes bug reports and pull requests to improve the
hardening of these surfaces. However, we typically will not break backwards
compatibility guarantees to address security issues in these areas (especially
for lower severity risks).

Defensive hardening is applied to these surfaces, but with weaker guarantees
compared to our hardened surfaces.

*   Serious security issues are still highly prioritized for fixing on these
    surfaces.

*   We do not break backwards compatibility guarantees to address lower severity
    issues on these surfaces outside of major version bumps. In some cases this
    means low-impact known issues may even be left open if they inherently
    cannot be closed without a breaking change. Serious issues (like RCE) would
    still be urgently addressed.

*   When using these surfaces on potentially malicious inputs, especially
    security sensitive usages are recommended to consider application-level
    isolation (such as sandboxing) or other defensive handling.

### Outside of CVE Threat Model (Best-Effort Hardened)

These are surfaces where Protobuf libraries are not as hardened against
adversarial inputs.

Outside of threat model does not mean we do not care at all about
security-relevant behaviors on these surfaces: we still apply defensive
hardening where feasible, and welcome reports on these surfaces. However, as
these surfaces are expected to be used with trusted inputs, the higher priority
for those cases is other topics including stability, performance, and developer
ergonomics.

## Proactively Hardened

### Parsing of Binary Wire Format Encoded Data **(Primary Use Case)**

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

--------------------------------------------------------------------------------

## Reactively Hardened

These areas are defensive against malicious inputs, but application-level
defense (such as sandboxing) is recommended.

### Parsing Text Format

Text Format is designed for local debugging, testing, and managing trusted
configurations by developers. It is not intended to be used as an interchange
format and is not recommended to expose public services which consume Text
Format as an encoding.

Note: Text Format parsing currently does not enforce any depth limit in several
supported runtimes (they support opt-in depth limits). We cannot enforce a depth
limit by default without breaking backwards compatibility, but may begin to
enforce depth limits by default as part of a future breaking change release.

### Lite Runtimes (C++ Lite, Java Lite) Denial of Service Risks

Lite runtimes target mobile and web usage: they are optimized for those
constrained envirnoments, and prioritize small binary size at the expense of
other properties.

Lite runtimes are still intended to be used to parse untrusted inputs, but DoS
issues are considered to be much less severe in mobile contexts, as the
opportunity and impact of an attacker successfully freezing one app is low
compared to reducing the availability of a public service.

We still intend to mitigate such risks, but Lite gencode/runtimes are not
recommended for servers exposing public endpoints.

### 'Wrong Kind of Exception Thrown'

In memory-safe runtimes (Java, Go, Python, C#), reaching a catchable exception
or runtime error (such as `IndexOutOfBoundsException` or `StackOverflowError`)
instead of the declared exception (such as `InvalidProtocolBufferException`) is
treated as an important bug to fix, but is considered to be a minor security
concern relative to serious issues like native heap corruption, data leakage,
remote code execution, or DoS vectors from unbounded computation or memory use.

It is recommended that RPC handler code be defensive against unexpected
exceptions if they are exceptionally sensitive to such behavior.

## Outside of CVE Threat Model (Best-effort Hardened)

### `protoc` CLI

The `protoc` CLI is an offline developer tool. `.proto` files are considered
source code (equivalent to `.java` or `.cpp` source files).

A supported modality of the CLI is to parse untrusted `proto` files to emit
`FileDescriptors` which can enable further machine processing of the schemas.

While `protoc` is hardened on a best-effort basis for this use case, we
recommend using defensive validation and sandboxing whenever running `protoc`
against potentially malicious inputs.

Untrusted flags being passed to `protoc` is fully outside of our threat model:
CLI flags are never be adversarial and arbitrary behavior driven by CLI flags
may be working as intended.

Caution: Compiling and executing generated code from untrusted schemas is
functionally equivalent to compiling and running arbitrary third-party `.java`
or `.cpp` source code and executing it: there may be intentional language
features which act as intentional code injection into the generated code. You
must treat untrusted `.proto` files the same as any other programming language
code in terms of supply-chain risk in this way, and not blindly execute the
gencode which was generated off of untrusted `.proto` files.

### DynamicMessage on Untrusted Descriptors

Protobuf supports encoding schemas into a Protobuf message format (e.g.
`FileDescriptorSet` or `DescriptorProto`). These messages can be handled as any
other Protobuf type. Parsing untrusted binary-encoded DescriptorProto falls
within the "primary use-case" described above.

In addition to simply processing DescriptorProto, it is additionally possible in
most runtimes to use a type named `DynamicMessage` which allows for using
runtime-loaded descriptors instead of using generated code and to use that type
with the reflection APIs.

For use-cases sensetive to DoS risks, it is recommended to use `DynamicMessage`
only with trusted descriptors (via trusted side channel source / config pushes).
When using `DynamicMessage` with a descriptor sourced from an untrusted source,
you may need to validate and sanitize them as you would user provided SQL.

Caution: Usage of `DynamicMessage` with malicious descriptors reaching an RCE or
information leak would still be treated as a high priority issue. However, there
are inherently reachable cases of where malicious descriptors used with
`DynamicMessage` can reach behavior which may otherwise be considered a Denial
of Service risk under our primary threat model. For example, it will be
reachable to hit memory use which is O(N*M) where N is "# of messages observed
on the wire" and M is "size of the message definition". Since untrusted
descriptors gives an affordance for arbitrarily large message definitions, using
DynamicMessage with untrusted descriptors and untrusted binary format inherently
can have memory amplification risks.

### Adversarial Application Code

Violating runtime API constraints or passing invalid arguments directly to a C++
API is considered an application integration error rather than a library
vulnerability.

Protobuf libraries do harden against the impact of certain classes of mistakes
being worse; for example, we often will panic if we can detect that an out of
bounds memory reads will occur in some cases. This is considered
defense-in-depth and misuse is not considered a vulnerability.

Excepting the surfaces enumerated above as hardened, Protobuf APIs in
memory-safe languages reaching memory safety problems on 'bad' parameters is
considered an high priority bug, but typically not within scope for CVE
disclosure.

In languages like PHP, this means that use of PHP Protobuf in an unconstrained
multi-tenant system where malicious application code may try to attack other
jobs concurrently running is not within our threat model.

Examples:

*   Passing negative or invalid buffer size parameters directly to
    `ParseFromArray` in C++ is wrong application code. It may be hardened to
    panic instead of risk out of bounds memory reads, but is not intended to be
    gracefully handled as a malformed-wire-bytes input would be (following C++
    idioms).
*   In a memory language Python, if code like `msg.repeatedField[-2147483649]`
    can reach a segfault, that is considered an important bug to fix, but it is
    not considered to be within CVE scope.

### Differential Parsing (Gateway propagation of original payload)

Differential parsing is a risk stemming from by two different libraries parsing
the same data with different interpretations.

In some contexts and for some formats differential parsing is considered a
security-sensitive topic. The primary risk is around flows that would validate
in a gateway, forwarding the data unmodified, and then a backend handles the
original payload and interprets the data differently, bypassing the intended
checks.

The Protobuf binary format is explicitly designed for propagation of unknown
fields, where the gateway may not be aware of the content at all and forwarding
will result in the next server who has an updated version of the schema will
corresponding parse to a different interpretation because it is aware of those
fields.

Additionally, there may be certain edge-case byte sequences where a few
different interpretations which may be considered acceptable within spec. Google
maintained runtimes will never encode these sequences, but they may successfully
parse them.

When using ProtoJSON format, the underlying JSON format itself contains
significant inherent ambiguities (as noted in ECMA-404 and RFC-8259, including
that there is no spec around the handling of duplicate keys and numeric
precision). As ProtoJSON is built on top of that foundation, ProtoJSON inherits
these ambiguities where certain sequences may have multiple different spec
permissible implementations. Spec Protobuf implementations strongly attempt to
avoid ever encoding such sequences (including that they always quote large
int64s, and don't emit duplicate fields), but the parsing behavior may differ in
such sequences and ProtoJSON cannot spec behavior which is unimplementable when
using ecosystem of JSON parsers.

For both formats, an architecture where a gateway performs validation and
forwards the original user request to a second server which reparses the user's
request but presumes validation has already occurred is outside of our threat
model.

For best security practice, it is recommended to:

*   Use a different set of messages schema for your public API and internal
    messages. Besides the security benefits, this decoupling also allows for
    easier evolution of your system, where public APIs often need to change
    slowly but internal ones can evolve faster.
*   Where you do propagate the same message type, always prefer for the gateway
    to parse and reserialize instead of forwarding the original payload
    verbatim, as this will commonly normalize edge case byte sequences.
*   Wherever possible to have each microservice verify any relevant ACLs for
    actions it is taking based on its interpretation of the request rather than
    rely on gateway validation.

### Specific depth cap exceeded but without uncontrolled recursion

Protobuf parsers apply depth limits (which are configurable): the purpose of
these depth limits is to prevent resource exhaustion issues stemming from
uncontrolled recursion.

To generally maintain consistent and interoperable behavior, we intend these
depth limits to be consistent in behavior in what payloads will be accepted or
rejected for a given integer depth.

Issues where an edge case is successfully parsed which is deeper than the exact
intended limit, this is viewed as a simple bug as long as it does not expose
meaningful resource exhaustion risks.

We welcome reports and patches for issues of that nature, but do not view it as
a security concern and so these issues can be filed via our public GitHub Issues
flow.

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

### Risks if wire bytes are modified in-transit

In terms of transport security, Protobuf is functionally equivalent to a
plaintext format: the encoding has no built-in signing or other integrity
features.

Best practice is to transport Protobuf encoded data over https. If signing or
other integrity features are needed, it is expected to be done in the layers
built top of the Protobuf libraries.

### API surfaces which not intended for direct public use

Protobuf has APIs which are not advertised or intended for direct public use.
These APIs may have non-obvious invariants for how they must be used.

Most notably, `upb` is a library which is used as an implementation detail API
of our other Protobuf libraries to use. `upb` itself is a highly optimized C
library which requires callers maintain invariants to be sound.

Security issues may arise if our language-specific runtimes which use `upb` do
not maintain those necessary invariants, or if `upb` has reachable bad behavior
when all intended invariants are maintained. However, it is not considered not a
security topic if arbitrary bad behavior may be reachable if `upb` APIs are
directly misused (including that `upb's` APIs accept MiniDescriptors/MiniTables
which are considered trusted types, and so will have arbitrary behavior if those
types do not meet the intended invariants).
