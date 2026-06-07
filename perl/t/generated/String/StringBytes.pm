package String::StringBytes;

use strict;
use warnings;

our $VERSION = '0.01';

use Protobuf::Message;
use Protobuf::DescriptorPool;
use Protobuf::Internal qw(:all);
use MIME::Base64;

BEGIN {
    my $descriptor_b64 = <<'EOF';
ChJzdHJpbmdfYnl0ZXMucHJvdG8SBlN0cmluZyI7CgVCeXRlcxIZCgh2X3N0cmluZxgBIAEoCVIH
dlN0cmluZxIXCgd2X2J5dGVzGAIgASgMUgZ2Qnl0ZXNCAkgBSskBCgYSBAAABxwKCAoBAhIDAAAP
CgoKAgQAEgQCAAUBCgoKAwQAARIDAggNCgsKBAQAAgASAwMCHwoMCgUEAAIABBIDAwIKCgwKBQQA
AgAFEgMDCxEKDAoFBAACAAESAwMSGgoMCgUEAAIAAxIDAx0eCgsKBAQAAgESAwQCHwoMCgUEAAIB
BBIDBAIKCgwKBQQAAgEFEgMECxAKDAoFBAACAQESAwQSGQoMCgUEAAIBAxIDBB0eCggKAQgSAwcA
HAoJCgIICRIDBwAc
EOF
    Protobuf::DescriptorPool->generated_pool->add_serialized_file(MIME::Base64::decode_base64($descriptor_b64));
}

# Message definitions

# === Message: String::StringBytes::Bytes ===
    # Fields for Bytes
    # Field: v_string Type: 9 ()
    # Field: v_bytes Type: 12 ()



1;
