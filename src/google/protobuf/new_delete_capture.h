// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// This file exists for testing allocation behavior when using arenas by hooking
// new/delete. It is a copy of //experimental/mvels/util/new_delete_capture.h.
//
// Copyright 2014 Google Inc.
//
// Author: Martijn Vels
//
// A simple class that captures memory allocations and deletes.
//
// This class is private to //strings and only intended to be used inside
// unit tests. It uses the MallocHook functionality to capture memory
// allocation and delete operations performed by the thread that activated
// a hook on a specific instance.
//
// The class  captures the following information:
// - Total allocation count (new, malloc(), etc).
// - Total delete count (delete, free(), etc).
// - The size and returned pointer for the first memory allocation.
// - The pointer for the first delete operation.
//
// The latter 2 infos (size and pointer of first new/delete) are usefull in
// cases where you can closely scope a Hook() / Unhook sequence around a
// specific piece of code where you expect no more than 1 pair of new / delete
// operations.
//
// Sample usage where we expect a single unique alloc / free:
//
//   NewDeleteCapture capture_alloc;
//   const void *ptr;
//   {
//     capture_alloc.Hook();
//     MyAllocationClass my_instance(size);
//     capture_alloc.Unhook();
//
//     ptr = my_instance.ptr();
//     GOOGLE_CHECK_EQ(1, capture_alloc.alloc_count());
//     GOOGLE_CHECK_EQ(0, capture_alloc.free_count());
//     GOOGLE_CHECK_EQ(size, capture_alloc.alloc_size());
//     GOOGLE_CHECK_EQ(ptr, capture_alloc.alloc_ptr());
//
//     capture_alloc.Hook();
//   }
//   capture_alloc.Unhook();
//   GOOGLE_CHECK_EQ(1, capture_alloc.alloc_count());
//   GOOGLE_CHECK_EQ(1, capture_alloc.free_count());
//   GOOGLE_CHECK_EQ(ptr, capture_alloc.free_ptr());
//
// You can only have one NewDeleteCapture instance active at the time. It is
// total valid to have many instances in different threads, but only one
// instance can have a hook active.
//
// Legal:
//
//   NewDeleteCapture capture_alloc1;
//   NewDeleteCapture capture_alloc2;
//   const void *ptr;
//   {
//     capture_alloc1.Hook();
//     MyAllocationClass my_instance(size);
//     capture_alloc1.Unhook();
//
//     capture_alloc2.Hook();
//     my_instance.reset(size);
//     capture_alloc2.Unhook();
//   }
//
// Illegal:
//
//   NewDeleteCapture capture_alloc1;
//   NewDeleteCapture capture_alloc2;
//   const void *ptr;
//   {
//     capture_alloc1.Hook();
//     MyAllocationClass my_instance(size);
//
//     capture_alloc2.Hook();
//     my_instance.reset(size);
//
//     capture_alloc1.Unhook();
//     capture_alloc2.Unhook();
//   }
//
#ifndef GOOGLE_PROTOBUF_NEW_DELETE_CAPTURE_H__
#define GOOGLE_PROTOBUF_NEW_DELETE_CAPTURE_H__

#include <stddef.h>

namespace google {
namespace protobuf_unittest {

class NewDeleteCapture {
 public:
  // Creates a new inactive capture instance
  NewDeleteCapture();

  // Destroys this capture instance. Active hooks are automatically removed.
  ~NewDeleteCapture();

  // Activates a hook on this instance. If reset is true (the default), all
  // internal counters will be reset to 0.
  // Returns true if the hook was activated, false if this instance already
  // owned the hook.
  // Requires no other instance owning the hook (check fails)
  bool Hook(bool reset = true);

  // De-activate the hook on this instance.
  // Returns true if the hook was removed, false if this instance did not own
  // the hook.
  bool Unhook();

  // Resets all counters to 0
  void Reset();

  // Returns the total number of allocations (new, malloc(), etc)
  size_t alloc_count() const { return alloc_count_; }

  // Returns the total number of deletes (delete, free(), etc)
  size_t free_count() const { return free_count_; }

  // Returns the size of the first observed allocation
  size_t alloc_size() const { return alloc_size_; }

  // Returns the allocated ptr of the first observed allocation
  const void *alloc_ptr() const { return alloc_ptr_; }

  // Returns the ptr of the first observed delete
  const void* free_ptr() const { return free_ptr_; }

 private:
  static void NewHook(const void *ptr, size_t size);
  static void DeleteHook(const void *ptr);

 private:
  size_t alloc_count_;
  size_t alloc_size_;
  const void *alloc_ptr_;

  size_t free_count_;
  const void *free_ptr_;
};

}  // namespace protobuf_unittest

}  // namespace google
#endif  // GOOGLE_PROTOBUF_NEW_DELETE_CAPTURE_H__
