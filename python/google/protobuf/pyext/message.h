// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: anuraag@google.com (Anuraag Agrawal)
// Author: tibell@google.com (Johan Tibell)

#ifndef GOOGLE_PROTOBUF_PYTHON_CPP_MESSAGE_H__
#define GOOGLE_PROTOBUF_PYTHON_CPP_MESSAGE_H__

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <cstdint>
#include <optional>
#include <utility>

#include "absl/log/absl_check.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/pyext/container_base.h"
#include "google/protobuf/pyext/free_threading/access_guard.h"
#include "google/protobuf/pyext/free_threading/guarded.h"
#include "google/protobuf/pyext/lazy_unique_ptr.h"
#include "google/protobuf/pyext/message_tree.h"
#include "google/protobuf/pyext/weak_value_map.h"

namespace google {
namespace protobuf {

class Message;
class Reflection;
class FieldDescriptor;
class Descriptor;
class DescriptorPool;
class MessageFactory;

namespace python {

struct ExtensionDict;
struct PyMessageFactory;
struct CMessageClass;

// Most of the complexity of the Message class comes from the "Release"
// behavior:
//
// When a field is cleared, it is only detached from its message. Existing
// references to submessages, to repeated container etc. won't see any change,
// as if the data was effectively managed by these containers.
//
// ExtensionDicts and UnknownFields containers do NOT follow this rule. They
// don't store any data, and always refer to their parent message.

// Defines the mutability and promotion state of a CMessage.
enum MessageMutabilityState : uint8_t {
  // Backed by a fully allocated, mutable C++ Message object whose parent
  // hierarchy is already mutable / marked dirty. Mutations on this message
  // do not require promoting the parent hierarchy.
  MESSAGE_MUTABLE = 0,

  // Backed by a const Message* (e.g., a default instance, a submessage inside
  // an unpromoted parent, a message backed by a LazyField, or a read-only
  // element from a repeated or map field) that is mutable on write (not
  // frozen).
  //
  // Even if the C++ Message* is already allocated (e.g., in a RepeatedPtrField
  // or Map), it must remain in MESSAGE_UNPROMOTED until the first mutation so
  // that AssureWritable promotes the entire parent hierarchy (e.g., allocating
  // default submessages, setting oneof cases, and marking LazyField ancestors
  // as dirty). Transitions to MESSAGE_MUTABLE upon the first mutation via
  // AssureWritable.
  MESSAGE_UNPROMOTED = 1,

  // Permanently read-only (e.g., Descriptor Options).
  // Any attempt to mutate will raise a Python FrozenInstanceError.
  MESSAGE_FROZEN = 2,
};

typedef struct CMessage : public TreeLockable<CMessage> {
  // Allocates an incomplete Python Message: the caller must fill self->tree,
  // self->message, and eventually self->parent.
  static CMessage* NewEmpty(CMessageClass* type);

  // Pointer to the C++ Message object for this CMessage.
  // - If this object has no parent, we own this pointer.
  // - If this object has a parent message, the parent owns this pointer.
  const Message* message;

  // A mapping indexed by field, containing weak references to contained objects
  // which need to implement the "Release" mechanism:
  // direct submessages, RepeatedCompositeContainer, RepeatedScalarContainer
  // and MapContainer.
  //   Maps: const FieldDescriptor* -> ContainerBase*
  typedef PyWeakValueMap CompositeFieldsMap;
  LazyUniquePtr<CompositeFieldsMap> composite_fields;

  // A mapping containing weak references to indirect child messages, accessed
  // through containers: repeated messages, and values of message maps.
  // This avoid the creation of similar maps in each of those containers.
  //   Maps: const Message* -> CMessage*
  typedef PyWeakValueMap SubMessagesMap;
  LazyUniquePtr<SubMessagesMap> child_submessages;

  // Implements the "weakref" protocol for this object.
  PyObject* weakreflist;

  // Return a *borrowed* reference to the message class.
  CMessageClass* GetMessageClass() const {
    return reinterpret_cast<CMessageClass*>(Py_TYPE(this));
  }

  // Return the C++ Descriptor for this message from its Python type.
  const Descriptor* GetDescriptor() const;

  // A message is a view of its own contents (see TreeLockable).
  const CMessage* Owner(const ReadAccess&) const { return this; }
  CMessage* Owner(const WriteAccess&) { return this; }

  bool IsFrozen(const ReadAccess& access) const {
    return state.Get(access) == MESSAGE_FROZEN;
  }
  void Freeze(const WriteAccess& access) { state.Set(MESSAGE_FROZEN, access); }

  // Returns true if this CMessage is quiescent: either being deallocated
  // (ob_refcnt == 0), or uniquely referenced by the current thread with no
  // parent.
  bool IsQuiescent() const {
    if (Py_REFCNT(this) == 0) return true;
#ifdef Py_GIL_DISABLED
    if (!PyUnstable_Object_IsUniquelyReferenced(
            reinterpret_cast<PyObject*>(const_cast<CMessage*>(this)))) {
      return false;
    }
#else
    if (Py_REFCNT(this) != 1) return false;
#endif
    return parent == nullptr;
  }

  // Returns the underlying mutable Message* without locking. Must ONLY be used
  // when this CMessage is quiescent.
  Message* GetQuiescent() const {
    ABSL_DCHECK(IsQuiescent());
    ABSL_DCHECK_EQ(state.GetQuiescent(), MESSAGE_MUTABLE);
    // Cast away constness is safe because of the checks above.
    return const_cast<Message*>(message);
  }

  // Attaches a newly constructed quiescent `child` message to `this` message's
  // repeated field `field_descriptor`, adopting `this` message's tree and
  // mutability state and registering `child` in `child_submessages`.
  void AttachQuiescentChild(const FieldDescriptor* field_descriptor,
                            CMessage* child, const WriteAccess& lock);
  bool AttachQuiescentChild(const FieldDescriptor* field_descriptor,
                            CMessage* child);

  // For container containing messages, return a Python object for the given
  // pointer to a message. Requires (at least) read access to this message's
  // tree. This is a const operation on the message tree: it only updates this
  // wrapper's internally synchronized child cache and Python refcount.
  CMessage* BuildSubMessageFromPointer(const FieldDescriptor* field_descriptor,
                                       const Message* sub_message,
                                       CMessageClass* message_class,
                                       MessageMutabilityState state,
                                       const ReadAccess& access) const;
  CMessage* BuildSubMessageFromPointer(const FieldDescriptor* field_descriptor,
                                       const Message* sub_message,
                                       CMessageClass* message_class,
                                       MessageMutabilityState state);

  // If a Python wrapper exists for `sub_message`, detaches it into a new root
  // CMessage with its own MessageTree and removes it from `child_submessages`.
  // When `swap_to_new` is true, `sub_message`'s contents are swapped into a
  // fresh C++ Message owned by the detached wrapper.
  bool MaybeReleaseSubMessage(Message* sub_message, const WriteAccess& lock,
                              bool swap_to_new = false);

  // Detaches the live Python wrapper (if any) for `sub_message`, a message
  // value in one of this message's map fields that is about to be overwritten
  // or deleted: the wrapper becomes a standalone message that owns the value's
  // current contents, which are moved into a new message.
  //
  // Note that ClearField() cannot call this, because it also must handle
  // repeated and map fields, and their wrappers always need a parent message.
  void DetachSubMessage(Message* sub_message, const WriteAccess& lock);
  void DetachSubMessage(Message* sub_message);

  // Initializes a new CMessage instance for a submessage. Only called once per
  // submessage as the result is cached in composite_fields.
  CMessage* GetSubMessage(const FieldDescriptor* field_descriptor,
                          CMessageClass* message_class, const ReadAccess& r);
  CMessage* GetSubMessage(const FieldDescriptor* field_descriptor);

  // After a Merge, visit every sub-message that was read-only, and
  // eventually update their pointer if the Merge operation modified them.
  void FixupAfterMerge(const WriteAccess& lock);
  void FixupAfterMerge();

 private:
  template <typename>
  friend struct TreeLockable;
  friend Message* cmessage::AssureWritable(CMessage* self,
                                           const WriteAccess& lock);

  void BindDerivedFieldsQuiescent(const AccessGuard& guard) {
    state.BindQuiescent(guard);
    has_mutable_map_ancestor.BindQuiescent(guard);
  }
  void UpdateDerivedFieldsGuard(const WriteAccess& old_access,
                                const WriteAccess& new_access) {
    state.UpdateGuard(old_access, new_access);
    has_mutable_map_ancestor.UpdateGuard(old_access, new_access);
  }
  void UpdateSubtreeChildrenTree(MessageTree* new_tree,
                                 const WriteAccess& old_lock,
                                 const WriteAccess& new_lock);

  // Indicates the mutability state of this CMessage wrapper.
  Guarded<MessageMutabilityState> state;

  // Whether there is a map ancestor anywhere in the hierarchy.
  Guarded<bool> has_mutable_map_ancestor;
} CMessage;

// The (meta) type of all Messages classes.
// It allows us to cache some C++ pointers in the class object itself, they are
// faster to extract than from the type's dictionary.

struct CMessageClass {
  // This is how CPython subclasses C structures: the base structure must be
  // the first member of the object.
  PyHeapTypeObject super;

  // C++ descriptor of this message.
  const Descriptor* message_descriptor;

  // Owned reference, used to keep the pointer above alive.
  // This reference must stay alive until all message pointers are destructed.
  PyObject* py_message_descriptor;

  // The Python MessageFactory used to create the class. It is needed to resolve
  // fields descriptors, including extensions fields; its C++ MessageFactory is
  // used to instantiate submessages.
  // This reference must stay alive until all message pointers are destructed.
  PyMessageFactory* py_message_factory;

  PyObject* AsPyObject() { return reinterpret_cast<PyObject*>(this); }
};

inline const Descriptor* CMessage::GetDescriptor() const {
  return GetMessageClass()->message_descriptor;
}

extern PyTypeObject* CMessageClass_Type;
extern PyTypeObject* CMessage_Type;

namespace cmessage {

// Creates a new CMessage Python object and allocates its C++ Message.
CMessage* NewCMessage(CMessageClass* type);

// Retrieves the C++ descriptor of a Python Extension descriptor.
// On error, return NULL with an exception set.
const FieldDescriptor* GetExtensionDescriptor(PyObject* extension);

// Delete the last n items in a repeated field.
void DeleteLastRepeatedWithSize(CMessage* self,
                                const FieldDescriptor* field_descriptor,
                                Py_ssize_t n);

// Deletes a range of items in a repeated field (following a
// removal in a RepeatedCompositeContainer).
//
// Corresponds to reflection api method RemoveLast.
int DeleteRepeatedField(CMessage* self, const FieldDescriptor* field_descriptor,
                        PyObject* slice);

// Check if a deletion operation on a repeated field is a no-op, valid or error.
// Returns:
//  1 if the deletion is a no-op (empty slice deletion).
//  0 if the deletion is valid and requires mutating the container.
// -1 if an error occurred.
int CheckRepeatedFieldDeletion(CMessage* parent,
                               const FieldDescriptor* field_descriptor,
                               PyObject* slice);

// Sets the specified scalar value to the message.
int InternalSetScalar(CMessage* self, const FieldDescriptor* field_descriptor,
                      PyObject* value);

// Sets the specified scalar value to the message.  Requires it is not a Oneof.
int InternalSetNonOneofScalar(Message* message,
                              const FieldDescriptor* field_descriptor,
                              PyObject* arg);

// Retrieves the specified scalar value from the message.
//
// Returns a new python reference.
PyObject* InternalGetScalar(const Message* message,
                            const FieldDescriptor* field_descriptor);

bool SetCompositeField(CMessage* self, const FieldDescriptor* field,
                       ContainerBase* value);

bool SetSubmessage(CMessage* self, CMessage*& submessage);

// Clears the message, removing all contained data. Extension dictionary and
// submessages are released first if there are remaining external references.
//
// Corresponds to message api method Clear.
PyObject* Clear(CMessage* self);

// Clears the data described by the given descriptor.
// Returns -1 on error.
//
// Corresponds to reflection api method ClearField.
int ClearFieldByDescriptor(CMessage* self, const FieldDescriptor* descriptor);

// Checks if the message has the field described by the descriptor. Used for
// extensions (which have no name).
// Returns 1 if true, 0 if false, and -1 on error.
//
// Corresponds to reflection api method HasField
int HasFieldByDescriptor(CMessage* self,
                         const FieldDescriptor* field_descriptor);

// Checks if the message has the named field.
//
// Corresponds to reflection api method HasField.
PyObject* HasField(CMessage* self, PyObject* arg);

// Initializes values of fields on a newly constructed message.
// Note that positional arguments are disallowed: 'args' must be NULL or the
// empty tuple.
int InitAttributes(CMessage* self, PyObject* args, PyObject* kwargs);

PyObject* MergeFrom(CMessage* self, PyObject* arg);

// Get a field from a message.
PyObject* GetFieldValue(CMessage* self,
                        const FieldDescriptor* field_descriptor);
// Sets the value of a scalar field in a message.
// On error, return -1 with an extension set.
int SetFieldValue(CMessage* self, const FieldDescriptor* field_descriptor,
                  PyObject* value);

PyObject* FindInitializationErrors(CMessage* self);

Message* AssureWritable(CMessage* self);

// Returns the message factory for the given message.
// This is equivalent to message.MESSAGE_FACTORY
//
// The returned factory is suitable for finding fields and building submessages,
// even in the case of extensions.
// Returns a *borrowed* reference, and never fails because we pass a CMessage.
PyMessageFactory* GetFactoryForMessage(CMessage* message);

PyObject* SetAllowOversizeProtos(PyObject* m, PyObject* arg);

}  // namespace cmessage

/* Is 64bit */
#define IS_64BIT (SIZEOF_LONG == 8)

#define FIELD_IS_REPEATED(field_descriptor) \
  ((field_descriptor)->label() == FieldDescriptor::LABEL_REPEATED)

#define PROTOBUF_CHECK_GET_INT32(arg, value, err) \
  int32_t value;                                  \
  if (!CheckAndGetInteger(arg, &value)) {         \
    return err;                                   \
  }

#define PROTOBUF_CHECK_GET_INT64(arg, value, err) \
  int64_t value;                                  \
  if (!CheckAndGetInteger(arg, &value)) {         \
    return err;                                   \
  }

#define PROTOBUF_CHECK_GET_UINT32(arg, value, err) \
  uint32_t value;                                  \
  if (!CheckAndGetInteger(arg, &value)) {          \
    return err;                                    \
  }

#define PROTOBUF_CHECK_GET_UINT64(arg, value, err) \
  uint64_t value;                                  \
  if (!CheckAndGetInteger(arg, &value)) {          \
    return err;                                    \
  }

#define PROTOBUF_CHECK_GET_FLOAT(arg, value, err) \
  float value;                                    \
  if (!CheckAndGetFloat(arg, &value)) {           \
    return err;                                   \
  }

#define PROTOBUF_CHECK_GET_DOUBLE(arg, value, err) \
  double value;                                    \
  if (!CheckAndGetDouble(arg, &value)) {           \
    return err;                                    \
  }

#define PROTOBUF_CHECK_GET_BOOL(arg, value, err) \
  bool value;                                    \
  if (!CheckAndGetBool(arg, &value)) {           \
    return err;                                  \
  }

#define FULL_MODULE_NAME "google.protobuf.pyext._message"

void FormatTypeError(PyObject* arg, const char* expected_types);
template <class T>
bool CheckAndGetInteger(PyObject* arg, T* value);
bool CheckAndGetDouble(PyObject* arg, double* value);
bool CheckAndGetFloat(PyObject* arg, float* value);
bool CheckAndGetBool(PyObject* arg, bool* value);

// Validates arg for a string or bytes field, and returns the string view if
// valid. Returns std::nullopt and sets a Python exception on failure.
std::optional<absl::string_view> CheckString(PyObject* arg,
                                             const FieldDescriptor* descriptor);
bool CheckAndSetString(PyObject* arg, Message* message,
                       const FieldDescriptor* descriptor,
                       const Reflection* reflection, bool append, int index);
PyObject* ToStringObject(const FieldDescriptor* descriptor,
                         absl::string_view value);

// Check if the passed field descriptor belongs to the given message.
// If not, return false and set a Python exception (a KeyError)
bool CheckFieldBelongsToMessage(const FieldDescriptor* field_descriptor,
                                const Message* message);
bool CheckFieldBelongsToMessage(const FieldDescriptor* field_descriptor,
                                const CMessage* message);

extern PyObject* PickleError_class;

extern PyObject* FrozenInstanceError_class;
// Sets a Python FrozenInstanceError with the given message and returns nullptr.
PyObject* SetFrozenError(const char* msg);

// Sets a Python FrozenInstanceError with the default error message for messages
// type and returns nullptr.
PyObject* SetMessageFrozenError();
// Returns 0 if `parent` is writable.
// Returns -1 if `parent` is frozen (sets a Python FrozenInstanceError with
// `error_msg`).
int CheckFrozen(const CMessage* message, const ReadAccess& access,
                const char* error_msg);
int CheckFrozen(CMessage* parent, const char* error_msg);

PyObject* PyMessage_New(const Descriptor* descriptor,
                        PyObject* py_message_factory);
const Message* PyMessage_GetMessagePointer(PyObject* msg);
Message* PyMessage_GetMutableMessagePointer(PyObject* msg);
PyObject* PyMessage_NewMessageOwnedExternally(Message* message,
                                              PyObject* py_message_factory);

bool InitProto2MessageModule(PyObject* m);

// These are referenced by repeated_scalar_container, and must
// be explicitly instantiated.
extern template bool CheckAndGetInteger<int32_t>(PyObject*, int32_t*);
extern template bool CheckAndGetInteger<int64_t>(PyObject*, int64_t*);
extern template bool CheckAndGetInteger<uint32_t>(PyObject*, uint32_t*);
extern template bool CheckAndGetInteger<uint64_t>(PyObject*, uint64_t*);

}  // namespace python
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_PYTHON_CPP_MESSAGE_H__
