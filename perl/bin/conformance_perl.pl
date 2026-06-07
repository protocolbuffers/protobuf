#!/usr/bin/env perl

use strict;
use warnings;

use Conformance::Conformance;
BEGIN {
    no strict 'refs';
    *Conformance::ConformanceRequest:: = *Conformance::Conformance::ConformanceRequest::;
    *Conformance::ConformanceResponse:: = *Conformance::Conformance::ConformanceResponse::;
}
use Protobuf_test_messages::Proto3::TestMessagesProto3;
use Protobuf_test_messages::Proto2::TestMessagesProto2;

# Disable buffering on stdout and stdin to ensure immediate communication with the runner
select((select(STDOUT), $| = 1)[0]);
select((select(STDIN), $| = 1)[0]);

my $test_count = 0;

while (1) {
    # Read 4-byte length prefix
    my $len_bytes;
    my $bytes_read = read(STDIN, $len_bytes, 4);
    if (!$bytes_read) {
        # EOF
        last;
    }
    if ($bytes_read != 4) {
        die "Failed to read 4-byte length prefix (read $bytes_read bytes)";
    }

    my $len = unpack('V', $len_bytes); # Little-endian 32-bit unsigned

    # Read the payload
    my $payload_bytes;
    $bytes_read = read(STDIN, $payload_bytes, $len);
    if ($bytes_read != $len) {
        die "Failed to read payload of length $len (read $bytes_read bytes)";
    }

    $test_count++;

    # Decode ConformanceRequest
    my $request;
    eval {
        $request = Conformance::ConformanceRequest->parse($payload_bytes);
    };
    if ($@) {
        warn "Failed to parse ConformanceRequest: $@";
        send_runtime_error("Failed to parse ConformanceRequest: $@");
        next;
    }

    my $response = handle_request($request);

    # Serialize ConformanceResponse
    my $response_bytes;
    eval {
        $response_bytes = $response->serialize;
    };
    if ($@) {
        warn "Failed to serialize ConformanceResponse: $@";
        send_runtime_error("Failed to serialize ConformanceResponse: $@");
        next;
    }

    # Write 4-byte length prefix and payload to STDOUT
    my $response_len = length($response_bytes);
    print STDOUT pack('V', $response_len);
    print STDOUT $response_bytes;
}

sub send_runtime_error {
    my ($msg) = @_;
    my $response = Conformance::ConformanceResponse->new(
        runtime_error => $msg
    );
    my $response_bytes = $response->serialize;
    my $response_len = length($response_bytes);
    print STDOUT pack('V', $response_len);
    print STDOUT $response_bytes;
}

sub handle_request {
    my ($request) = @_;

    my $message_type = $request->message_type;
    my $payload_format = $request->payload;

    # Map proto message type to Perl class
    my $class;
    if ($message_type eq 'protobuf_test_messages.proto3.TestAllTypesProto3') {
        $class = 'Protobuf_test_messages::Proto3::TestMessagesProto3::TestAllTypesProto3';
    } elsif ($message_type eq 'protobuf_test_messages.proto2.TestAllTypesProto2') {
        $class = 'Protobuf_test_messages::Proto2::TestMessagesProto2::TestAllTypesProto2';
    } else {
        return Conformance::ConformanceResponse->new(
            skipped => "Unsupported message type: $message_type"
        );
    }

    # 1. Decode the input payload
    my $msg;

    if ($payload_format eq 'protobuf_payload') {
        eval {
            $msg = $class->parse($request->protobuf_payload);
        };
        if ($@) {
            return Conformance::ConformanceResponse->new(
                parse_error => "Protobuf parse error: $@"
            );
        }
    } elsif ($payload_format eq 'json_payload') {
        my %options;
        if ($request->test_category == 3) { # JSON_IGNORE_UNKNOWN_PARSING_TEST = 3
            $options{ignore_unknown} = 1;
        }
        eval {
            $msg = $class->from_json($request->json_payload, \%options);
        };
        if ($@) {
            if ($@ =~ /not yet implemented/i) {
                return Conformance::ConformanceResponse->new(
                    skipped => "JSON decoding not implemented in this engine"
                );
            }
            return Conformance::ConformanceResponse->new(
                parse_error => "JSON parse error: $@"
            );
        }
    } elsif ($payload_format eq 'text_payload') {
        return Conformance::ConformanceResponse->new(
            skipped => "Text format decoding not supported"
        );
    } else {
        return Conformance::ConformanceResponse->new(
            runtime_error => "Unsupported input payload format: $payload_format"
        );
    }

    # 2. Serialize to the requested output format
    my $requested_format = $request->requested_output_format;

    # WireFormat enum values: PROTOBUF = 1, JSON = 2, TEXT_FORMAT = 4
    if ($requested_format == 1) {
        my $output_bytes;
        eval {
            $output_bytes = $msg->serialize;
        };
        if ($@) {
            return Conformance::ConformanceResponse->new(
                serialize_error => "Protobuf serialize error: $@"
            );
        }
        return Conformance::ConformanceResponse->new(
            protobuf_payload => $output_bytes
        );
    } elsif ($requested_format == 2) {
        my $output_json;
        eval {
            $output_json = $msg->to_json;
        };
        if ($@) {
            if ($@ =~ /not yet implemented/i) {
                return Conformance::ConformanceResponse->new(
                    skipped => "JSON encoding not implemented in this engine"
                );
            }
            return Conformance::ConformanceResponse->new(
                serialize_error => "JSON serialize error: $@"
            );
        }
        return Conformance::ConformanceResponse->new(
            json_payload => $output_json
        );
    } elsif ($requested_format == 4) {
        my $output_text;
        eval {
            $output_text = $msg->to_text;
        };
        if ($@) {
            return Conformance::ConformanceResponse->new(
                serialize_error => "Text format serialize error: $@"
            );
        }
        return Conformance::ConformanceResponse->new(
            text_payload => $output_text
        );
    } else {
        return Conformance::ConformanceResponse->new(
            skipped => "Unsupported requested output format: $requested_format"
        );
    }
}
