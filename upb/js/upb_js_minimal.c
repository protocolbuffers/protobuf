// Copyright 2023 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// This disables inlining and forces all public functions to be exported.
#define UPB_BUILD_API

#include "upb/mem/arena.h"  // IWYU pragma: keep
#include "upb/mini_descriptor/decode.h"  // IWYU pragma: keep
#include "upb/wire/decode.h"  // IWYU pragma: keep

// This is unused when we compile for WASM, but Builder doesn't like it
// if this file cannot compile as a regular cc_binary().
int main(void) {}
