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

#include <google/protobuf/map_field.h>

#include <vector>

namespace google {
namespace protobuf {
namespace internal {

ProtobufOnceType map_entry_default_instances_once_;
Mutex* map_entry_default_instances_mutex_;
vector<MessageLite*>* map_entry_default_instances_;

void DeleteMapEntryDefaultInstances() {
  for (int i = 0; i < map_entry_default_instances_->size(); ++i) {
    delete map_entry_default_instances_->at(i);
  }
  delete map_entry_default_instances_mutex_;
  delete map_entry_default_instances_;
}

void InitMapEntryDefaultInstances() {
  map_entry_default_instances_mutex_ = new Mutex();
  map_entry_default_instances_ = new vector<MessageLite*>();
  OnShutdown(&DeleteMapEntryDefaultInstances);
}

void RegisterMapEntryDefaultInstance(MessageLite* default_instance) {
  GoogleOnceInit(&map_entry_default_instances_once_,
                 &InitMapEntryDefaultInstances);
  MutexLock lock(map_entry_default_instances_mutex_);
  map_entry_default_instances_->push_back(default_instance);
}

MapFieldBase::~MapFieldBase() {
  if (repeated_field_ != NULL) delete repeated_field_;
}

const RepeatedPtrFieldBase& MapFieldBase::GetRepeatedField() const {
  SyncRepeatedFieldWithMap();
  return *repeated_field_;
}

RepeatedPtrFieldBase* MapFieldBase::MutableRepeatedField() {
  SyncRepeatedFieldWithMap();
  SetRepeatedDirty();
  return repeated_field_;
}

int MapFieldBase::SpaceUsedExcludingSelf() const {
  mutex_.Lock();
  int size = SpaceUsedExcludingSelfNoLock();
  mutex_.Unlock();
  return size;
}

int MapFieldBase::SpaceUsedExcludingSelfNoLock() const {
  if (repeated_field_ != NULL) {
    return repeated_field_->SpaceUsedExcludingSelf();
  } else {
    return 0;
  }
}

void MapFieldBase::InitMetadataOnce() const {
  GOOGLE_CHECK(entry_descriptor_ != NULL);
  GOOGLE_CHECK(assign_descriptor_callback_ != NULL);
  (*assign_descriptor_callback_)();
}

void MapFieldBase::SetMapDirty() { state_ = STATE_MODIFIED_MAP; }

void MapFieldBase::SetRepeatedDirty() { state_ = STATE_MODIFIED_REPEATED; }

void* MapFieldBase::MutableRepeatedPtrField() const { return repeated_field_; }

void MapFieldBase::SyncRepeatedFieldWithMap() const {
  Atomic32 state = google::protobuf::internal::NoBarrier_Load(&state_);
  if (state == STATE_MODIFIED_MAP) {
    mutex_.Lock();
    // Double check state, because another thread may have seen the same state
    // and done the synchronization before the current thread.
    if (state_ == STATE_MODIFIED_MAP) {
      SyncRepeatedFieldWithMapNoLock();
      google::protobuf::internal::NoBarrier_Store(&state_, CLEAN);
    }
    mutex_.Unlock();
  }
}

void MapFieldBase::SyncRepeatedFieldWithMapNoLock() const {
  if (repeated_field_ == NULL) repeated_field_ = new RepeatedPtrField<Message>;
}

void MapFieldBase::SyncMapWithRepeatedField() const {
  Atomic32 state = google::protobuf::internal::NoBarrier_Load(&state_);
  if (state == STATE_MODIFIED_REPEATED) {
    mutex_.Lock();
    // Double check state, because another thread may have seen the same state
    // and done the synchronization before the current thread.
    if (state_ == STATE_MODIFIED_REPEATED) {
      SyncMapWithRepeatedFieldNoLock();
      google::protobuf::internal::NoBarrier_Store(&state_, CLEAN);
    }
    mutex_.Unlock();
  }
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google
