# Handle Protobuf - hbp

go/hpb

<!--*
# Document freshness: For more information, see go/fresh-source.
freshness: { owner: 'hongshin' reviewed: '2025-05-14' }
*-->

## What is hpb?

hpb is a C++ Protobuf API that provides a familiar protobuf experience while
hiding internal implementation details. This encapsulation allows for swapping
out the backend without breaking user code. hpb wraps other protobuf
implementations -- currently only upb is supported, but a proto2 C++ backend is
in the early stages.

hpb is designed to be a "free" wrapper, such that it adds no runtime overhead
compared with using the underlying backend directly. The wrapping is all
performed in header files that optimize away at compile time.

hpb is the preferred solution for users who want to use upb protos in a C++
application. Unlike the upb C generated code, hpb provides a stable API and
properly handles naming conflicts. hpb is also far more ergonomic than the upb
generated code.

## Why hpb?

-   As mentioned in go/upb, the upb C API is not stable, and not intended for
    use in applications. hpb fills this gap.
    -   upbc gencode can change and break clients at any time. hpb is designed
        for user applications inside of g3 and has stronger guarantees of api
        stability
    -   LSCs/cleanups are handled by the Protobuf team - less maintenance burden
-   Code will be more readable. Regular C++ method calls vs field accessors. No
    need to provide fully qualified names in terra hpb.
    -   `my_project_MyMessage_set_x` vs `msg.set_x()`
-   [planned] Gives a transition path if your team wishes to move from proto2::cpp to upb
    via hpb(cpp) -> hpb(upb). All toggled statically with a BUILD flag
-   [planned] interop between hpb(upb) and hpb(cpp)

## How can I get access?

hpb is currently in special availability. If you think this would be a good
fit for your team, please reach out to the Protobuf team.

