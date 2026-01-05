"""Version transition that parses the single user flag into internal components."""

load("//bazel/private:prebuilt_tool_integrity.bzl", "RELEASE_MAJOR", "RELEASE_MINOR", "RELEASE_PATCH")

def _parse_version(version_str):
    """Parse '33' or '33.0' or '33.0.0' into (major, minor, patch).

    Args:
        version_str: The version string to parse. Can be:
            - Empty string: use RELEASE_VERSION defaults
            - "33": major version only
            - "33.0": major.minor
            - "33.0.0": exact version

    Returns:
        A tuple of (major, minor, patch) strings.
    """
    if not version_str:
        # Empty means use the release version
        return (RELEASE_MAJOR, RELEASE_MINOR, RELEASE_PATCH)

    parts = version_str.split(".")
    major = parts[0] if len(parts) > 0 else RELEASE_MAJOR

    # If user only specifies major (e.g., "33"), minor and patch are empty
    # This allows match_major to succeed while match_major_minor fails
    minor = parts[1] if len(parts) > 1 else ""
    patch = parts[2] if len(parts) > 2 else ""
    return (major, minor, patch)

def _protoc_version_transition_impl(settings, attr):
    user_version = settings["@com_google_protobuf//bazel/toolchains:protoc_version"]
    major, minor, patch = _parse_version(user_version)

    return {
        "@com_google_protobuf//bazel/private/toolchains/version:protoc_major": major,
        "@com_google_protobuf//bazel/private/toolchains/version:protoc_minor": minor,
        "@com_google_protobuf//bazel/private/toolchains/version:protoc_patch": patch,
    }

protoc_version_transition = transition(
    implementation = _protoc_version_transition_impl,
    inputs = ["@com_google_protobuf//bazel/toolchains:protoc_version"],
    outputs = [
        "@com_google_protobuf//bazel/private/toolchains/version:protoc_major",
        "@com_google_protobuf//bazel/private/toolchains/version:protoc_minor",
        "@com_google_protobuf//bazel/private/toolchains/version:protoc_patch",
    ],
)
