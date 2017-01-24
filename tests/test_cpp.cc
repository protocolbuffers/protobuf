/*
 *
 * Tests for C++ wrappers.
 */

#include <stdio.h>
#include <string.h>

#include <fstream>
#include <iostream>
#include <set>
#include <sstream>

#include "upb/def.h"
#include "upb/descriptor/reader.h"
#include "upb/handlers.h"
#include "upb/pb/decoder.h"
#include "upb/pb/glue.h"
#include "upb_test.h"
#include "upb/upb.h"

template <class T>
void AssertInsert(T* const container, const typename T::value_type& val) {
  bool inserted = container->insert(val).second;
  ASSERT(inserted);
}

static void TestCastsUpDown() {
  upb::reffed_ptr<const upb::MessageDef> reffed_md(upb::MessageDef::New());
  const upb::MessageDef* md = reffed_md.get();

  // Upcast to reffed_ptr implicitly.
  upb::reffed_ptr<const upb::Def> reffed_def = reffed_md;
  ASSERT(reffed_def.get() == upb::upcast(reffed_md.get()));

  // Upcast to raw pointer must be explicit.
  const upb::Def* def = upb::upcast(md);
  ASSERT(def == reffed_def.get());
  const upb::Def* def2 = upb::upcast(reffed_md.get());
  ASSERT(def2 == reffed_def.get());

  // Downcast/dyncast of raw pointer uses upb::down_cast/upb::dyn_cast.
  const upb::MessageDef* md2 = upb::down_cast<const upb::MessageDef*>(def);
  const upb::MessageDef* md3 = upb::dyn_cast<const upb::MessageDef*>(def);
  ASSERT(md == md2);
  ASSERT(md == md3);

  // Downcast/dyncast of reffed_ptr uses down_cast/dyn_cast members.
  upb::reffed_ptr<const upb::MessageDef> md4(
      reffed_def.down_cast<const upb::MessageDef>());
  upb::reffed_ptr<const upb::MessageDef> md5(
      reffed_def.dyn_cast<const upb::MessageDef>());
  ASSERT(md == md4.get());
  ASSERT(md == md5.get());

  // Failed dyncast returns NULL.
  ASSERT(upb::dyn_cast<const upb::EnumDef*>(def) == NULL);
  ASSERT(reffed_def.dyn_cast<const upb::EnumDef>().get() == NULL);
}

static void TestCastsConst0() {
  // Should clean up properly even if it is not assigned to anything.
  upb::MessageDef::New();
}

static void TestCastsConst1() {
  // Test reffed mutable -> reffed mutable construction/assignment.
  upb::reffed_ptr<upb::MessageDef> md(upb::MessageDef::New());
  upb::MessageDef *md2 = md.get();
  md = upb::MessageDef::New();
  ASSERT(md.get());
  ASSERT(md.get() != md2);
}

static void TestCastsConst2() {
  // Test reffed mutable -> reffed mutable upcast construction/assignment.
  upb::reffed_ptr<upb::MessageDef> md(upb::MessageDef::New());
  upb::reffed_ptr<upb::Def> def = md;
  ASSERT(upb::upcast(md.get()) == def.get());
  def = md;
  ASSERT(upb::upcast(md.get()) == def.get());
}

static void TestCastsConst3() {
  // Test reffed mutable -> reffed mutable downcast.
  upb::reffed_ptr<upb::Def> def(upb::MessageDef::New());
  upb::reffed_ptr<upb::MessageDef> md = def.down_cast<upb::MessageDef>();
  ASSERT(upb::upcast(md.get()) == def.get());
}

static void TestCastsConst4() {
  // Test reffed mutable -> reffed mutable dyncast.
  upb::reffed_ptr<upb::Def> def(upb::MessageDef::New());
  upb::reffed_ptr<upb::MessageDef> md = def.dyn_cast<upb::MessageDef>();
  ASSERT(upb::upcast(md.get()) == def.get());
}

static void TestCastsConst5() {
  // Test reffed mutable -> reffed const construction/assignment.
  upb::reffed_ptr<const upb::MessageDef> md(upb::MessageDef::New());
  const upb::MessageDef *md2 = md.get();
  md = upb::MessageDef::New();
  ASSERT(md.get());
  ASSERT(md.get() != md2);
}

static void TestCastsConst6() {
  // Test reffed mutable -> reffed const upcast construction/assignment.
  upb::reffed_ptr<upb::MessageDef> md(upb::MessageDef::New());
  upb::reffed_ptr<const upb::Def> def = md;
  ASSERT(upb::upcast(md.get()) == def.get());
  def = md;
  ASSERT(upb::upcast(md.get()) == def.get());
}

static void TestCastsConst7() {
  // Test reffed mutable -> reffed const downcast.
  upb::reffed_ptr<upb::Def> def(upb::MessageDef::New());
  upb::reffed_ptr<const upb::MessageDef> md =
      def.down_cast<const upb::MessageDef>();
  ASSERT(upb::upcast(md.get()) == def.get());
}

static void TestCastsConst8() {
  // Test reffed mutable -> reffed const dyncast.
  upb::reffed_ptr<upb::Def> def(upb::MessageDef::New());
  upb::reffed_ptr<const upb::MessageDef> md =
      def.dyn_cast<const upb::MessageDef>();
  ASSERT(upb::upcast(md.get()) == def.get());
}

static void TestCastsConst9() {
  // Test plain mutable -> plain mutable upcast
  upb::reffed_ptr<upb::MessageDef> md(upb::MessageDef::New());
  upb::Def* def = upb::upcast(md.get());
  ASSERT(upb::down_cast<upb::MessageDef*>(def) == md.get());
}

static void TestCastsConst10() {
  // Test plain const -> plain const upcast
  upb::reffed_ptr<const upb::MessageDef> md(upb::MessageDef::New());
  const upb::Def* def = upb::upcast(md.get());
  ASSERT(upb::down_cast<const upb::MessageDef*>(def) == md.get());
}

static void TestSymbolTable(const char *descriptor_file) {
  upb::Status status;
  std::ifstream file_in(descriptor_file, std::ios::binary);
  std::string descriptor((std::istreambuf_iterator<char>(file_in)),
                         (std::istreambuf_iterator<char>()));
  std::vector<upb::reffed_ptr<upb::FileDef> > files;
  if (!upb::LoadDescriptor(descriptor, &status, &files)) {
    std::cerr << "Couldn't load descriptor: " << status.error_message();
    exit(1);
  }

  upb::SymbolTable* s = upb::SymbolTable::New();

  for (size_t i = 0; i < files.size(); i++) {
    ASSERT(s->AddFile(files[i].get(), &status));
  }

  upb::reffed_ptr<const upb::MessageDef> md(s->LookupMessage("C"));
  ASSERT(md.get());

  // We want a def that satisfies this to test iteration.
  ASSERT(md->field_count() > 1);

#ifdef UPB_CXX11
  // Test range-based for.
  std::set<const upb::FieldDef*> fielddefs;
  for (const upb::FieldDef* f : md.get()->fields()) {
    AssertInsert(&fielddefs, f);
    ASSERT(f->containing_type() == md.get());
  }
  ASSERT(fielddefs.size() == md->field_count());
#endif

  ASSERT(md.get());
  upb::SymbolTable::Free(s);
}

static void TestCasts1() {
  upb::reffed_ptr<const upb::MessageDef> md(upb::MessageDef::New());
  const upb::Def* def = upb::upcast(md.get());
  const upb::MessageDef* md2 = upb::down_cast<const upb::MessageDef*>(def);
  const upb::MessageDef* md3 = upb::dyn_cast<const upb::MessageDef*>(def);

  ASSERT(md.get() == md2);
  ASSERT(md.get() == md3);

  const upb::EnumDef* ed = upb::dyn_cast<const upb::EnumDef*>(def);
  ASSERT(!ed);
}

static void TestCasts2() {
  // Test mutable -> const cast.
  upb::reffed_ptr<upb::MessageDef> md(upb::MessageDef::New());
  upb::Def* def = upb::upcast(md.get());
  const upb::MessageDef* const_md = upb::down_cast<const upb::MessageDef*>(def);
  ASSERT(const_md == md.get());
}

//
// Tests for registering and calling handlers in all their variants.
// This test code is very repetitive because we have to declare each
// handler function variant separately, and they all have different
// signatures so it does not lend itself well to templates.
//
// We test three handler types:
//   StartMessage (no data params)
//   Int32        (1 data param (int32_t))
//   String Buf   (2 data params (const char*, size_t))
//
// For each handler type we test all 8 handler variants:
//   (handler data?) x  (function/method) x (returns {void, success})
//
// The one notable thing we don't test at the moment is
// StartSequence/StartString handlers: these are different from StartMessage()
// in that they return void* for the sub-closure.  But this is exercised in
// other tests.
//

static const int kExpectedHandlerData = 1232323;

class StringBufTesterBase {
 public:
  static const upb::FieldDef::Type kFieldType = UPB_TYPE_STRING;

  StringBufTesterBase() : seen_(false), handler_data_val_(0) {}

  void CallAndVerify(upb::Sink* sink, const upb::FieldDef* f) {
    upb::Handlers::Selector start;
    ASSERT(upb::Handlers::GetSelector(f, UPB_HANDLER_STARTSTR, &start));
    upb::Handlers::Selector str;
    ASSERT(upb::Handlers::GetSelector(f, UPB_HANDLER_STRING, &str));

    ASSERT(!seen_);
    upb::Sink sub;
    sink->StartMessage();
    sink->StartString(start, 0, &sub);
    size_t ret = sub.PutStringBuffer(str, &buf_, 5, &handle_);
    ASSERT(seen_);
    ASSERT(len_ == 5);
    ASSERT(ret == 5);
    ASSERT(handler_data_val_ == kExpectedHandlerData);
  }

 protected:
  bool seen_;
  int handler_data_val_;
  size_t len_;
  char buf_;
  upb::BufferHandle handle_;
};

// Test 8 combinations of:
//   (handler data?) x (buffer handle?) x (function/method)
//
// Then we add one test each for this variation: to prevent combinatorial
// explosion of these tests we don't test the full 16 combinations, but
// rely on our knowledge that the implementation processes the return wrapping
// in a second separate and independent stage:
//
//   (function/method)

class StringBufTesterVoidMethodNoHandlerDataNoHandle
    : public StringBufTesterBase {
 public:
  typedef StringBufTesterVoidMethodNoHandlerDataNoHandle ME;
  void Register(upb::Handlers* h, const upb::FieldDef* f) {
    UPB_UNUSED(f);
    ASSERT(h->SetStringHandler(f, UpbMakeHandler(&ME::Handler)));
    handler_data_val_ = kExpectedHandlerData;
  }

 private:
  void Handler(const char *buf, size_t len) {
    ASSERT(buf == &buf_);
    seen_ = true;
    len_ = len;
  }
};

class StringBufTesterVoidMethodNoHandlerDataWithHandle
    : public StringBufTesterBase {
 public:
  typedef StringBufTesterVoidMethodNoHandlerDataWithHandle ME;
  void Register(upb::Handlers* h, const upb::FieldDef* f) {
    UPB_UNUSED(f);
    ASSERT(h->SetStringHandler(f, UpbMakeHandler(&ME::Handler)));
    handler_data_val_ = kExpectedHandlerData;
  }

 private:
  void Handler(const char *buf, size_t len, const upb::BufferHandle* handle) {
    ASSERT(buf == &buf_);
    ASSERT(handle == &handle_);
    seen_ = true;
    len_ = len;
  }
};

class StringBufTesterVoidMethodWithHandlerDataNoHandle
    : public StringBufTesterBase {
 public:
  typedef StringBufTesterVoidMethodWithHandlerDataNoHandle ME;
  void Register(upb::Handlers* h, const upb::FieldDef* f) {
    UPB_UNUSED(f);
    ASSERT(h->SetStringHandler(
        f, UpbBind(&ME::Handler, new int(kExpectedHandlerData))));
  }

 private:
  void Handler(const int* hd, const char *buf, size_t len) {
    ASSERT(buf == &buf_);
    handler_data_val_ = *hd;
    seen_ = true;
    len_ = len;
  }
};

class StringBufTesterVoidMethodWithHandlerDataWithHandle
    : public StringBufTesterBase {
 public:
  typedef StringBufTesterVoidMethodWithHandlerDataWithHandle ME;
  void Register(upb::Handlers* h, const upb::FieldDef* f) {
    UPB_UNUSED(f);
    ASSERT(h->SetStringHandler(
        f, UpbBind(&ME::Handler, new int(kExpectedHandlerData))));
  }

 private:
  void Handler(const int* hd, const char* buf, size_t len,
               const upb::BufferHandle* handle) {
    ASSERT(buf == &buf_);
    ASSERT(handle == &handle_);
    handler_data_val_ = *hd;
    seen_ = true;
    len_ = len;
  }
};

class StringBufTesterVoidFunctionNoHandlerDataNoHandle
    : public StringBufTesterBase {
 public:
  typedef StringBufTesterVoidFunctionNoHandlerDataNoHandle ME;
  void Register(upb::Handlers* h, const upb::FieldDef* f) {
    UPB_UNUSED(f);
    ASSERT(h->SetStringHandler(f, UpbMakeHandler(&ME::Handler)));
    handler_data_val_ = kExpectedHandlerData;
  }

 private:
  static void Handler(ME* t, const char *buf, size_t len) {
    ASSERT(buf == &t->buf_);
    t->seen_ = true;
    t->len_ = len;
  }
};

class StringBufTesterVoidFunctionNoHandlerDataWithHandle
    : public StringBufTesterBase {
 public:
  typedef StringBufTesterVoidFunctionNoHandlerDataWithHandle ME;
  void Register(upb::Handlers* h, const upb::FieldDef* f) {
    UPB_UNUSED(f);
    ASSERT(h->SetStringHandler(f, UpbMakeHandler(&ME::Handler)));
    handler_data_val_ = kExpectedHandlerData;
  }

 private:
  static void Handler(ME* t, const char* buf, size_t len,
                      const upb::BufferHandle* handle) {
    ASSERT(buf == &t->buf_);
    ASSERT(handle == &t->handle_);
    t->seen_ = true;
    t->len_ = len;
  }
};

class StringBufTesterVoidFunctionWithHandlerDataNoHandle
    : public StringBufTesterBase {
 public:
  typedef StringBufTesterVoidFunctionWithHandlerDataNoHandle ME;
  void Register(upb::Handlers* h, const upb::FieldDef* f) {
    UPB_UNUSED(f);
    ASSERT(h->SetStringHandler(
        f, UpbBind(&ME::Handler, new int(kExpectedHandlerData))));
  }

 private:
  static void Handler(ME* t, const int* hd, const char *buf, size_t len) {
    ASSERT(buf == &t->buf_);
    t->handler_data_val_ = *hd;
    t->seen_ = true;
    t->len_ = len;
  }
};

class StringBufTesterVoidFunctionWithHandlerDataWithHandle
    : public StringBufTesterBase {
 public:
  typedef StringBufTesterVoidFunctionWithHandlerDataWithHandle ME;
  void Register(upb::Handlers* h, const upb::FieldDef* f) {
    UPB_UNUSED(f);
    ASSERT(h->SetStringHandler(
        f, UpbBind(&ME::Handler, new int(kExpectedHandlerData))));
  }

 private:
  static void Handler(ME* t, const int* hd, const char* buf, size_t len,
                      const upb::BufferHandle* handle) {
    ASSERT(buf == &t->buf_);
    ASSERT(handle == &t->handle_);
    t->handler_data_val_ = *hd;
    t->seen_ = true;
    t->len_ = len;
  }
};

class StringBufTesterSizeTMethodNoHandlerDataNoHandle
    : public StringBufTesterBase {
 public:
  typedef StringBufTesterSizeTMethodNoHandlerDataNoHandle ME;
  void Register(upb::Handlers* h, const upb::FieldDef* f) {
    UPB_UNUSED(f);
    ASSERT(h->SetStringHandler(f, UpbMakeHandler(&ME::Handler)));
    handler_data_val_ = kExpectedHandlerData;
  }

 private:
  size_t Handler(const char *buf, size_t len) {
    ASSERT(buf == &buf_);
    seen_ = true;
    len_ = len;
    return len;
  }
};

class StringBufTesterBoolMethodNoHandlerDataNoHandle
    : public StringBufTesterBase {
 public:
  typedef StringBufTesterBoolMethodNoHandlerDataNoHandle ME;
  void Register(upb::Handlers* h, const upb::FieldDef* f) {
    UPB_UNUSED(f);
    ASSERT(h->SetStringHandler(f, UpbMakeHandler(&ME::Handler)));
    handler_data_val_ = kExpectedHandlerData;
  }

 private:
  bool Handler(const char *buf, size_t len) {
    ASSERT(buf == &buf_);
    seen_ = true;
    len_ = len;
    return true;
  }
};

class StartMsgTesterBase {
 public:
  // We don't need the FieldDef it will create, but the test harness still
  // requires that we provide one.
  static const upb::FieldDef::Type kFieldType = UPB_TYPE_STRING;

  StartMsgTesterBase() : seen_(false), handler_data_val_(0) {}

  void CallAndVerify(upb::Sink* sink, const upb::FieldDef* f) {
    UPB_UNUSED(f);
    ASSERT(!seen_);
    sink->StartMessage();
    ASSERT(seen_);
    ASSERT(handler_data_val_ == kExpectedHandlerData);
  }

 protected:
  bool seen_;
  int handler_data_val_;
};

// Test all 8 combinations of:
//   (handler data?) x  (function/method) x (returns {void, bool})

class StartMsgTesterVoidFunctionNoHandlerData : public StartMsgTesterBase {
 public:
  typedef StartMsgTesterVoidFunctionNoHandlerData ME;
  void Register(upb::Handlers* h, const upb::FieldDef* f) {
    UPB_UNUSED(f);
    ASSERT(h->SetStartMessageHandler(UpbMakeHandler(&Handler)));
    handler_data_val_ = kExpectedHandlerData;
  }

 private:
  //static void Handler(ME* t) {
  static void Handler(ME* t) {
    t->seen_ = true;
  }
};

class StartMsgTesterBoolFunctionNoHandlerData : public StartMsgTesterBase {
 public:
  typedef StartMsgTesterBoolFunctionNoHandlerData ME;
  void Register(upb::Handlers* h, const upb::FieldDef* f) {
    UPB_UNUSED(f);
    ASSERT(h->SetStartMessageHandler(UpbMakeHandler(&Handler)));
    handler_data_val_ = kExpectedHandlerData;
  }

 private:
  static bool Handler(ME* t) {
    t->seen_ = true;
    return true;
  }
};

class StartMsgTesterVoidMethodNoHandlerData : public StartMsgTesterBase {
 public:
  typedef StartMsgTesterVoidMethodNoHandlerData ME;
  void Register(upb::Handlers* h, const upb::FieldDef* f) {
    UPB_UNUSED(f);
    ASSERT(h->SetStartMessageHandler(UpbMakeHandler(&ME::Handler)));
    handler_data_val_ = kExpectedHandlerData;
  }

 private:
  void Handler() {
    seen_ = true;
  }
};

class StartMsgTesterBoolMethodNoHandlerData : public StartMsgTesterBase {
 public:
  typedef StartMsgTesterBoolMethodNoHandlerData ME;
  void Register(upb::Handlers* h, const upb::FieldDef* f) {
    UPB_UNUSED(f);
    ASSERT(h->SetStartMessageHandler(UpbMakeHandler(&ME::Handler)));
    handler_data_val_ = kExpectedHandlerData;
  }

 private:
  bool Handler() {
    seen_ = true;
    return true;
  }
};

class StartMsgTesterVoidFunctionWithHandlerData : public StartMsgTesterBase {
 public:
  typedef StartMsgTesterVoidFunctionWithHandlerData ME;
  void Register(upb::Handlers* h, const upb::FieldDef* f) {
    UPB_UNUSED(f);
    ASSERT(h->SetStartMessageHandler(
        UpbBind(&Handler, new int(kExpectedHandlerData))));
  }

 private:
  static void Handler(ME* t, const int* hd) {
    t->handler_data_val_ = *hd;
    t->seen_ = true;
  }
};

class StartMsgTesterBoolFunctionWithHandlerData : public StartMsgTesterBase {
 public:
  typedef StartMsgTesterBoolFunctionWithHandlerData ME;
  void Register(upb::Handlers* h, const upb::FieldDef* f) {
    UPB_UNUSED(f);
    ASSERT(h->SetStartMessageHandler(
        UpbBind(&Handler, new int(kExpectedHandlerData))));
  }

 private:
  static bool Handler(ME* t, const int* hd) {
    t->handler_data_val_ = *hd;
    t->seen_ = true;
    return true;
  }
};

class StartMsgTesterVoidMethodWithHandlerData : public StartMsgTesterBase {
 public:
  typedef StartMsgTesterVoidMethodWithHandlerData ME;
  void Register(upb::Handlers* h, const upb::FieldDef* f) {
    UPB_UNUSED(f);
    ASSERT(h->SetStartMessageHandler(
        UpbBind(&ME::Handler, new int(kExpectedHandlerData))));
  }

 private:
  void Handler(const int* hd) {
    handler_data_val_ = *hd;
    seen_ = true;
  }
};

class StartMsgTesterBoolMethodWithHandlerData : public StartMsgTesterBase {
 public:
  typedef StartMsgTesterBoolMethodWithHandlerData ME;
  void Register(upb::Handlers* h, const upb::FieldDef* f) {
    UPB_UNUSED(f);
    ASSERT(h->SetStartMessageHandler(
        UpbBind(&ME::Handler, new int(kExpectedHandlerData))));
  }

 private:
  bool Handler(const int* hd) {
    handler_data_val_ = *hd;
    seen_ = true;
    return true;
  }
};

class Int32ValueTesterBase {
 public:
  static const upb::FieldDef::Type kFieldType = UPB_TYPE_INT32;

  Int32ValueTesterBase() : seen_(false), val_(0), handler_data_val_(0) {}

  void CallAndVerify(upb::Sink* sink, const upb::FieldDef* f) {
    upb::Handlers::Selector s;
    ASSERT(upb::Handlers::GetSelector(f, UPB_HANDLER_INT32, &s));

    ASSERT(!seen_);
    sink->PutInt32(s, 5);
    ASSERT(seen_);
    ASSERT(handler_data_val_ == kExpectedHandlerData);
    ASSERT(val_ == 5);
  }

 protected:
  bool seen_;
  int32_t val_;
  int handler_data_val_;
};

// Test all 8 combinations of:
//   (handler data?) x  (function/method) x (returns {void, bool})

class ValueTesterInt32VoidFunctionNoHandlerData
    : public Int32ValueTesterBase {
 public:
  typedef ValueTesterInt32VoidFunctionNoHandlerData ME;
  void Register(upb::Handlers* h, const upb::FieldDef* f) {
    ASSERT(h->SetInt32Handler(f, UpbMakeHandler(&Handler)));
    handler_data_val_ = kExpectedHandlerData;
  }

 private:
  static void Handler(ME* t, int32_t val) {
    t->val_ = val;
    t->seen_ = true;
  }
};

class ValueTesterInt32BoolFunctionNoHandlerData
    : public Int32ValueTesterBase {
 public:
  typedef ValueTesterInt32BoolFunctionNoHandlerData ME;
  void Register(upb::Handlers* h, const upb::FieldDef* f) {
    ASSERT(h->SetInt32Handler(f, UpbMakeHandler(&Handler)));
    handler_data_val_ = kExpectedHandlerData;
  }

 private:
  static bool Handler(ME* t, int32_t val) {
    t->val_ = val;
    t->seen_ = true;
    return true;
  }
};

class ValueTesterInt32VoidMethodNoHandlerData : public Int32ValueTesterBase {
 public:
  typedef ValueTesterInt32VoidMethodNoHandlerData ME;
  void Register(upb::Handlers* h, const upb::FieldDef* f) {
    ASSERT(h->SetInt32Handler(f, UpbMakeHandler(&ME::Handler)));
    handler_data_val_ = kExpectedHandlerData;
  }

 private:
  void Handler(int32_t val) {
    val_ = val;
    seen_ = true;
  }
};

class ValueTesterInt32BoolMethodNoHandlerData : public Int32ValueTesterBase {
 public:
  typedef ValueTesterInt32BoolMethodNoHandlerData ME;
  void Register(upb::Handlers* h, const upb::FieldDef* f) {
    ASSERT(h->SetInt32Handler(f, UpbMakeHandler(&ME::Handler)));
    handler_data_val_ = kExpectedHandlerData;
  }

 private:
  bool Handler(int32_t val) {
    val_ = val;
    seen_ = true;
    return true;
  }
};

class ValueTesterInt32VoidFunctionWithHandlerData
    : public Int32ValueTesterBase {
 public:
  typedef ValueTesterInt32VoidFunctionWithHandlerData ME;
  void Register(upb::Handlers* h, const upb::FieldDef* f) {
    ASSERT(h->SetInt32Handler(
        f, UpbBind(&Handler, new int(kExpectedHandlerData))));
  }

 private:
  static void Handler(ME* t, const int* hd, int32_t val) {
    t->val_ = val;
    t->handler_data_val_ = *hd;
    t->seen_ = true;
  }
};

class ValueTesterInt32BoolFunctionWithHandlerData
    : public Int32ValueTesterBase {
 public:
  typedef ValueTesterInt32BoolFunctionWithHandlerData ME;
  void Register(upb::Handlers* h, const upb::FieldDef* f) {
    ASSERT(h->SetInt32Handler(
        f, UpbBind(&Handler, new int(kExpectedHandlerData))));
  }

 private:
  static bool Handler(ME* t, const int* hd, int32_t val) {
    t->val_ = val;
    t->handler_data_val_ = *hd;
    t->seen_ = true;
    return true;
  }
};

class ValueTesterInt32VoidMethodWithHandlerData : public Int32ValueTesterBase {
 public:
  typedef ValueTesterInt32VoidMethodWithHandlerData ME;
  void Register(upb::Handlers* h, const upb::FieldDef* f) {
    ASSERT(h->SetInt32Handler(
        f, UpbBind(&ME::Handler, new int(kExpectedHandlerData))));
  }

 private:
  void Handler(const int* hd, int32_t val) {
    val_ = val;
    handler_data_val_ = *hd;
    seen_ = true;
  }
};

class ValueTesterInt32BoolMethodWithHandlerData : public Int32ValueTesterBase {
 public:
  typedef ValueTesterInt32BoolMethodWithHandlerData ME;
  void Register(upb::Handlers* h, const upb::FieldDef* f) {
    ASSERT(h->SetInt32Handler(
        f, UpbBind(&ME::Handler, new int(kExpectedHandlerData))));
  }

 private:
  bool Handler(const int* hd, int32_t val) {
    val_ = val;
    handler_data_val_ = *hd;
    seen_ = true;
    return true;
  }
};

template <class T>
void TestHandler() {
  upb::reffed_ptr<upb::MessageDef> md(upb::MessageDef::New());
  upb::reffed_ptr<upb::FieldDef> f(upb::FieldDef::New());
  f->set_type(T::kFieldType);
  ASSERT(f->set_name("test", NULL));
  ASSERT(f->set_number(1, NULL));
  ASSERT(md->AddField(f, NULL));
  ASSERT(md->Freeze(NULL));

  upb::reffed_ptr<upb::Handlers> h(upb::Handlers::New(md.get()));
  T tester;
  tester.Register(h.get(), f.get());
  ASSERT(h->Freeze(NULL));

  upb::Sink sink(h.get(), &tester);
  tester.CallAndVerify(&sink, f.get());
}

class T1 {};
class T2 {};

template <class C>
void DoNothingHandler(C* closure) {
  UPB_UNUSED(closure);
}

template <class C>
void DoNothingInt32Handler(C* closure, int32_t val) {
  UPB_UNUSED(closure);
  UPB_UNUSED(val);
}

template <class R>
class DoNothingStartHandler {
 public:
  // We wrap these functions inside of a class for a somewhat annoying reason.
  // UpbMakeHandler() is a macro, so we can't say
  //    UpbMakeHandler(DoNothingStartHandler<T1, T2>)
  //
  // because otherwise the preprocessor gets confused at the comma and tries to
  // make it two macro arguments.  The usual solution doesn't work either:
  //    UpbMakeHandler((DoNothingStartHandler<T1, T2>))
  //
  // If we do that the macro expands correctly, but then it tries to pass that
  // parenthesized expression as a template parameter, ie. Type<(F)>, which
  // isn't legal C++ (Clang will compile it but complains with
  //    warning: address non-type template argument cannot be surrounded by
  //    parentheses
  //
  // This two-level thing allows us to effectively pass two template parameters,
  // but without any commas:
  //    UpbMakeHandler(DoNothingStartHandler<T1>::Handler<T2>)
  template <class C>
  static R* Handler(C* closure) {
    UPB_UNUSED(closure);
    return NULL;
  }

  template <class C>
  static R* String(C* closure, size_t size_len) {
    UPB_UNUSED(closure);
    UPB_UNUSED(size_len);
    return NULL;
  }
};

template <class C>
void DoNothingStringBufHandler(C* closure, const char *buf, size_t len) {
  UPB_UNUSED(closure);
  UPB_UNUSED(buf);
  UPB_UNUSED(len);
}

template <class C>
void DoNothingEndMessageHandler(C* closure, upb::Status *status) {
  UPB_UNUSED(closure);
  UPB_UNUSED(status);
}

void TestMismatchedTypes() {
  // First create a schema for our test.
  upb::reffed_ptr<upb::MessageDef> md(upb::MessageDef::New());

  upb::reffed_ptr<upb::FieldDef> f(upb::FieldDef::New());
  f->set_type(UPB_TYPE_INT32);
  ASSERT(f->set_name("i32", NULL));
  ASSERT(f->set_number(1, NULL));
  ASSERT(md->AddField(f, NULL));
  const upb::FieldDef* i32 = f.get();

  f = upb::FieldDef::New();
  f->set_type(UPB_TYPE_INT32);
  ASSERT(f->set_name("r_i32", NULL));
  ASSERT(f->set_number(2, NULL));
  f->set_label(UPB_LABEL_REPEATED);
  ASSERT(md->AddField(f, NULL));
  const upb::FieldDef* r_i32 = f.get();

  f = upb::FieldDef::New();
  f->set_type(UPB_TYPE_STRING);
  ASSERT(f->set_name("str", NULL));
  ASSERT(f->set_number(3, NULL));
  ASSERT(md->AddField(f, NULL));
  const upb::FieldDef* str = f.get();

  f = upb::FieldDef::New();
  f->set_type(UPB_TYPE_STRING);
  ASSERT(f->set_name("r_str", NULL));
  ASSERT(f->set_number(4, NULL));
  f->set_label(UPB_LABEL_REPEATED);
  ASSERT(md->AddField(f, NULL));
  const upb::FieldDef* r_str = f.get();

  f = upb::FieldDef::New();
  f->set_type(UPB_TYPE_MESSAGE);
  ASSERT(f->set_name("msg", NULL));
  ASSERT(f->set_number(5, NULL));
  ASSERT(f->set_message_subdef(md.get(), NULL));
  ASSERT(md->AddField(f, NULL));
  const upb::FieldDef* msg = f.get();

  f = upb::FieldDef::New();
  f->set_type(UPB_TYPE_MESSAGE);
  ASSERT(f->set_name("r_msg", NULL));
  ASSERT(f->set_number(6, NULL));
  ASSERT(f->set_message_subdef(md.get(), NULL));
  f->set_label(UPB_LABEL_REPEATED);
  ASSERT(md->AddField(f, NULL));
  const upb::FieldDef* r_msg = f.get();

  ASSERT(md->Freeze(NULL));

  // Now test the type-checking in handler registration.
  upb::reffed_ptr<upb::Handlers> h(upb::Handlers::New(md.get()));

  // Establish T1 as the top-level closure type.
  ASSERT(h->SetInt32Handler(i32, UpbMakeHandler(DoNothingInt32Handler<T1>)));

  // Now any other attempt to set another handler with T2 as the top-level
  // closure should fail.  But setting these same handlers with T1 as the
  // top-level closure will succeed.
  ASSERT(!h->SetStartMessageHandler(UpbMakeHandler(DoNothingHandler<T2>)));
  ASSERT(h->SetStartMessageHandler(UpbMakeHandler(DoNothingHandler<T1>)));

  ASSERT(
      !h->SetEndMessageHandler(UpbMakeHandler(DoNothingEndMessageHandler<T2>)));
  ASSERT(
      h->SetEndMessageHandler(UpbMakeHandler(DoNothingEndMessageHandler<T1>)));

  ASSERT(!h->SetStartStringHandler(
              str, UpbMakeHandler(DoNothingStartHandler<T1>::String<T2>)));
  ASSERT(h->SetStartStringHandler(
              str, UpbMakeHandler(DoNothingStartHandler<T1>::String<T1>)));

  ASSERT(!h->SetEndStringHandler(str, UpbMakeHandler(DoNothingHandler<T2>)));
  ASSERT(h->SetEndStringHandler(str, UpbMakeHandler(DoNothingHandler<T1>)));

  ASSERT(!h->SetStartSubMessageHandler(
              msg, UpbMakeHandler(DoNothingStartHandler<T1>::Handler<T2>)));
  ASSERT(h->SetStartSubMessageHandler(
              msg, UpbMakeHandler(DoNothingStartHandler<T1>::Handler<T1>)));

  ASSERT(
      !h->SetEndSubMessageHandler(msg, UpbMakeHandler(DoNothingHandler<T2>)));
  ASSERT(
      h->SetEndSubMessageHandler(msg, UpbMakeHandler(DoNothingHandler<T1>)));

  ASSERT(!h->SetStartSequenceHandler(
              r_i32, UpbMakeHandler(DoNothingStartHandler<T1>::Handler<T2>)));
  ASSERT(h->SetStartSequenceHandler(
              r_i32, UpbMakeHandler(DoNothingStartHandler<T1>::Handler<T1>)));

  ASSERT(!h->SetEndSequenceHandler(
              r_i32, UpbMakeHandler(DoNothingHandler<T2>)));
  ASSERT(h->SetEndSequenceHandler(
              r_i32, UpbMakeHandler(DoNothingHandler<T1>)));

  ASSERT(!h->SetStartSequenceHandler(
              r_msg, UpbMakeHandler(DoNothingStartHandler<T1>::Handler<T2>)));
  ASSERT(h->SetStartSequenceHandler(
              r_msg, UpbMakeHandler(DoNothingStartHandler<T1>::Handler<T1>)));

  ASSERT(!h->SetEndSequenceHandler(
              r_msg, UpbMakeHandler(DoNothingHandler<T2>)));
  ASSERT(h->SetEndSequenceHandler(
              r_msg, UpbMakeHandler(DoNothingHandler<T1>)));

  ASSERT(!h->SetStartSequenceHandler(
              r_str, UpbMakeHandler(DoNothingStartHandler<T1>::Handler<T2>)));
  ASSERT(h->SetStartSequenceHandler(
              r_str, UpbMakeHandler(DoNothingStartHandler<T1>::Handler<T1>)));

  ASSERT(!h->SetEndSequenceHandler(
              r_str, UpbMakeHandler(DoNothingHandler<T2>)));
  ASSERT(h->SetEndSequenceHandler(
              r_str, UpbMakeHandler(DoNothingHandler<T1>)));

  // By setting T1 as the return type for the Start* handlers we have
  // established T1 as the type of the sequence and string frames.
  // Setting callbacks that use T2 should fail, but T1 should succeed.
  ASSERT(
      !h->SetStringHandler(str, UpbMakeHandler(DoNothingStringBufHandler<T2>)));
  ASSERT(
      h->SetStringHandler(str, UpbMakeHandler(DoNothingStringBufHandler<T1>)));

  ASSERT(!h->SetInt32Handler(r_i32, UpbMakeHandler(DoNothingInt32Handler<T2>)));
  ASSERT(h->SetInt32Handler(r_i32, UpbMakeHandler(DoNothingInt32Handler<T1>)));

  ASSERT(!h->SetStartSubMessageHandler(
              r_msg, UpbMakeHandler(DoNothingStartHandler<T1>::Handler<T2>)));
  ASSERT(h->SetStartSubMessageHandler(
              r_msg, UpbMakeHandler(DoNothingStartHandler<T1>::Handler<T1>)));

  ASSERT(!h->SetEndSubMessageHandler(r_msg,
                                     UpbMakeHandler(DoNothingHandler<T2>)));
  ASSERT(h->SetEndSubMessageHandler(r_msg,
                                    UpbMakeHandler(DoNothingHandler<T1>)));

  ASSERT(!h->SetStartStringHandler(
              r_str, UpbMakeHandler(DoNothingStartHandler<T1>::String<T2>)));
  ASSERT(h->SetStartStringHandler(
              r_str, UpbMakeHandler(DoNothingStartHandler<T1>::String<T1>)));

  ASSERT(
      !h->SetEndStringHandler(r_str, UpbMakeHandler(DoNothingHandler<T2>)));
  ASSERT(h->SetEndStringHandler(r_str, UpbMakeHandler(DoNothingHandler<T1>)));

  ASSERT(!h->SetStringHandler(r_str,
                              UpbMakeHandler(DoNothingStringBufHandler<T2>)));
  ASSERT(h->SetStringHandler(r_str,
                             UpbMakeHandler(DoNothingStringBufHandler<T1>)));

  h->ClearError();
  ASSERT(h->Freeze(NULL));

  // For our second test we do the same in reverse.  We directly set the type of
  // the frame and then observe failures at registering a Start* handler that
  // returns a different type.
  h = upb::Handlers::New(md.get());

  // First establish the type of a sequence frame directly.
  ASSERT(h->SetInt32Handler(r_i32, UpbMakeHandler(DoNothingInt32Handler<T1>)));

  // Now setting a StartSequence callback that returns a different type should
  // fail.
  ASSERT(!h->SetStartSequenceHandler(
              r_i32, UpbMakeHandler(DoNothingStartHandler<T2>::Handler<T1>)));
  ASSERT(h->SetStartSequenceHandler(
              r_i32, UpbMakeHandler(DoNothingStartHandler<T1>::Handler<T1>)));

  // Establish a string frame directly.
  ASSERT(h->SetStringHandler(r_str,
                             UpbMakeHandler(DoNothingStringBufHandler<T1>)));

  // Fail setting a StartString callback that returns a different type.
  ASSERT(!h->SetStartStringHandler(
              r_str, UpbMakeHandler(DoNothingStartHandler<T2>::String<T1>)));
  ASSERT(h->SetStartStringHandler(
      r_str, UpbMakeHandler(DoNothingStartHandler<T1>::String<T1>)));

  // The previous established T1 as the frame for the r_str sequence.
  ASSERT(!h->SetStartSequenceHandler(
              r_str, UpbMakeHandler(DoNothingStartHandler<T2>::Handler<T1>)));
  ASSERT(h->SetStartSequenceHandler(
      r_str, UpbMakeHandler(DoNothingStartHandler<T1>::Handler<T1>)));

  // Now test for this error that is not caught until freeze time:
  // Change-of-closure-type implies that a StartSequence or StartString handler
  // should exist to return the closure type of the inner frame but no
  // StartSequence/StartString handler is registered.

  h = upb::Handlers::New(md.get());

  // Establish T1 as top-level closure type.
  ASSERT(h->SetInt32Handler(i32, UpbMakeHandler(DoNothingInt32Handler<T1>)));

  // Establish T2 as closure type of sequence frame.
  ASSERT(
      h->SetInt32Handler(r_i32, UpbMakeHandler(DoNothingInt32Handler<T2>)));

  // Now attempt to freeze; this should fail because a StartSequence handler
  // needs to be registered that takes a T1 and returns a T2.
  ASSERT(!h->Freeze(NULL));

  // Now if we register the necessary StartSequence handler, the freezing should
  // work.
  ASSERT(h->SetStartSequenceHandler(
      r_i32, UpbMakeHandler(DoNothingStartHandler<T2>::Handler<T1>)));
  h->ClearError();
  ASSERT(h->Freeze(NULL));

  // Test for a broken chain that is two deep.
  h = upb::Handlers::New(md.get());

  // Establish T1 as top-level closure type.
  ASSERT(h->SetInt32Handler(i32, UpbMakeHandler(DoNothingInt32Handler<T1>)));

  // Establish T2 as the closure type of the string frame inside a sequence
  // frame.
  ASSERT(h->SetStringHandler(r_str,
                             UpbMakeHandler(DoNothingStringBufHandler<T2>)));

  // Now attempt to freeze; this should fail because a StartSequence or
  // StartString handler needs to be registered that takes a T1 and returns a
  // T2.
  ASSERT(!h->Freeze(NULL));

  // Now if we register a StartSequence handler it succeeds.
  ASSERT(h->SetStartSequenceHandler(
      r_str, UpbMakeHandler(DoNothingStartHandler<T2>::Handler<T1>)));
  h->ClearError();
  ASSERT(h->Freeze(NULL));

  // TODO(haberman): test that closure returned by StartSubMessage does not
  // match top-level closure of sub-handlers.
}

class IntIncrementer {
 public:
  explicit IntIncrementer(int* x) : x_(x) { (*x_)++; }
  ~IntIncrementer() { (*x_)--; }

  static void Handler(void* closure, const IntIncrementer* incrementer,
                      int32_t x) {
    UPB_UNUSED(closure);
    UPB_UNUSED(incrementer);
    UPB_UNUSED(x);
  }

 private:
  int* x_;
};


void TestHandlerDataDestruction() {
  upb::reffed_ptr<upb::MessageDef> md(upb::MessageDef::New());
  upb::reffed_ptr<upb::FieldDef> f(upb::FieldDef::New());
  f->set_type(UPB_TYPE_INT32);
  ASSERT(f->set_name("test", NULL));
  ASSERT(f->set_number(1, NULL));
  ASSERT(md->AddField(f, NULL));
  ASSERT(md->Freeze(NULL));

  int x = 0;
  {
    upb::reffed_ptr<upb::Handlers> h(upb::Handlers::New(md.get()));
    h->SetInt32Handler(
        f.get(), UpbBind(&IntIncrementer::Handler, new IntIncrementer(&x)));
    ASSERT(x == 1);
  }

  ASSERT(x == 0);
}

void TestOneofs() {
  upb::Status status;
  upb::reffed_ptr<upb::MessageDef> md(upb::MessageDef::New());
  upb::reffed_ptr<upb::OneofDef> o(upb::OneofDef::New());

  o->set_name("test_oneof", &status);
  ASSERT(status.ok());

  for (int i = 0; i < 5; i++) {
    std::ostringstream fieldname;
    fieldname << "field_" << i;
    upb::reffed_ptr<upb::FieldDef> f(upb::FieldDef::New());
    f->set_name(fieldname.str(), &status);
    ASSERT(status.ok());
    f->set_type(UPB_TYPE_INT32);
    f->set_number(i + 1, &status);
    ASSERT(status.ok());
    f->set_label(UPB_LABEL_OPTIONAL);

    o->AddField(f.get(), &status);
    ASSERT(status.ok());
  }

  md->AddOneof(o.get(), &status);
  ASSERT(status.ok());

  int field_count = 0;
  for (upb::OneofDef::iterator it = o->begin(); it != o->end(); ++it) {
    upb::FieldDef* f = *it;
    ASSERT(f->type() == UPB_TYPE_INT32);
    field_count++;
  }
  ASSERT(field_count == 5);

  upb::MessageDef::oneof_iterator msg_it = md->oneof_begin();
  ASSERT(msg_it != md->oneof_end());
  ASSERT((*msg_it) == o.get());

#ifdef UPB_CXX11
  // Test range-based for on both fields and oneofs (with the iterator adaptor).
  field_count = 0;
  for (auto* field : md->fields()) {
    UPB_UNUSED(field);
    field_count++;
  }
  ASSERT(field_count == 5);

  int oneof_count = 0;
  for (auto* oneof : md->oneofs()) {
    UPB_UNUSED(oneof);
    oneof_count++;
  }
  ASSERT(oneof_count == 1);
#endif  //  UPB_CXX11

  // Test that we can add a new field to the oneof and that it becomes a member
  // of the msgdef as well.
  upb::reffed_ptr<upb::FieldDef> newf(upb::FieldDef::New());
  newf->set_name("new_field_10", &status);
  ASSERT(status.ok());
  newf->set_number(10, &status);
  ASSERT(status.ok());
  newf->set_label(UPB_LABEL_OPTIONAL);
  newf->set_type(UPB_TYPE_INT32);
  o->AddField(newf.get(), &status);
  ASSERT(status.ok());
  ASSERT(newf->containing_type() == md.get());

  // Test that we can add a new field to the msgdef first and then to the oneof.
  upb::reffed_ptr<upb::FieldDef> newf2(upb::FieldDef::New());
  newf2->set_name("new_field_11", &status);
  ASSERT(status.ok());
  newf2->set_number(11, &status);
  ASSERT(status.ok());
  newf2->set_label(UPB_LABEL_OPTIONAL);
  newf2->set_type(UPB_TYPE_INT32);
  md->AddField(newf2.get(), &status);
  ASSERT(status.ok());
  o->AddField(newf2.get(), &status);
  ASSERT(status.ok());
  ASSERT(newf2->containing_oneof() == o.get());

  // Test that we cannot add REQUIRED or REPEATED fields to the oneof.
  upb::reffed_ptr<upb::FieldDef> newf3(upb::FieldDef::New());
  newf3->set_name("new_field_12", &status);
  ASSERT(status.ok());
  newf3->set_number(12, &status);
  ASSERT(status.ok());
  newf3->set_label(UPB_LABEL_REQUIRED);
  newf3->set_type(UPB_TYPE_INT32);
  o->AddField(newf3.get(), &status);
  ASSERT(!status.ok());
  newf->set_label(UPB_LABEL_REPEATED);
  o->AddField(newf3.get(), &status);
  ASSERT(!status.ok());
}

extern "C" {

int run_tests(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: test_cpp <descriptor file>\n");
    return 1;
  }
  TestSymbolTable(argv[1]);
  TestCastsUpDown();
  TestCasts1();
  TestCasts2();
  TestCastsConst0();
  TestCastsConst1();
  TestCastsConst2();
  TestCastsConst3();
  TestCastsConst4();
  TestCastsConst5();
  TestCastsConst6();
  TestCastsConst7();
  TestCastsConst8();
  TestCastsConst9();
  TestCastsConst10();

  TestHandler<ValueTesterInt32VoidFunctionNoHandlerData>();
  TestHandler<ValueTesterInt32BoolFunctionNoHandlerData>();
  TestHandler<ValueTesterInt32VoidMethodNoHandlerData>();
  TestHandler<ValueTesterInt32BoolMethodNoHandlerData>();
  TestHandler<ValueTesterInt32VoidFunctionWithHandlerData>();
  TestHandler<ValueTesterInt32BoolFunctionWithHandlerData>();
  TestHandler<ValueTesterInt32VoidMethodWithHandlerData>();
  TestHandler<ValueTesterInt32BoolMethodWithHandlerData>();

  TestHandler<StartMsgTesterVoidFunctionNoHandlerData>();
  TestHandler<StartMsgTesterBoolFunctionNoHandlerData>();
  TestHandler<StartMsgTesterVoidMethodNoHandlerData>();
  TestHandler<StartMsgTesterBoolMethodNoHandlerData>();
  TestHandler<StartMsgTesterVoidFunctionWithHandlerData>();
  TestHandler<StartMsgTesterBoolFunctionWithHandlerData>();
  TestHandler<StartMsgTesterVoidMethodWithHandlerData>();
  TestHandler<StartMsgTesterBoolMethodWithHandlerData>();

  TestHandler<StringBufTesterVoidMethodNoHandlerDataNoHandle>();
  TestHandler<StringBufTesterVoidMethodNoHandlerDataWithHandle>();
  TestHandler<StringBufTesterVoidMethodWithHandlerDataNoHandle>();
  TestHandler<StringBufTesterVoidMethodWithHandlerDataWithHandle>();
  TestHandler<StringBufTesterVoidFunctionNoHandlerDataNoHandle>();
  TestHandler<StringBufTesterVoidFunctionNoHandlerDataWithHandle>();
  TestHandler<StringBufTesterVoidFunctionWithHandlerDataNoHandle>();
  TestHandler<StringBufTesterVoidFunctionWithHandlerDataWithHandle>();
  TestHandler<StringBufTesterSizeTMethodNoHandlerDataNoHandle>();
  TestHandler<StringBufTesterBoolMethodNoHandlerDataNoHandle>();

  TestMismatchedTypes();

  TestHandlerDataDestruction();

  TestOneofs();

  return 0;
}

}
