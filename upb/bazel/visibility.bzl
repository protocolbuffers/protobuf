# Copyright (c) 2025, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

visibility("public")
upb_clients = [
    "//...",
    # go/keep-sorted start
    "@com_github_grpc_grpc//...",
    "@com_google_cel_cpp//...",
    "@com_google_googleapis//...",
    "@com_google_protobuf//...",
    "@com_google_s2a_proto//...",
    "@phst_rules_elisp//...",
    # go/keep-sorted end
]
