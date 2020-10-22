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

#include "tests/test_cpp.upbdefs.h"
#include "tests/upb_test.h"
#include "upb/def.h"
#include "upb/handlers.h"
#include "upb/pb/decoder.h"
#include "upb/pb/textprinter.h"
#include "upb/port_def.inc"
#include "upb/upb.h"

template <class T>
void AssertInsert(T* const container, const typename T::value_type& val) {
  bool inserted = container->insert(val).second;
  ASSERT(inserted);
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
  static constexpr int kFieldNumber = 3;

  StringBufTesterBase() : seen_(false), handler_data_val_(0) {}

  void CallAndVerify(upb::Sink sink, upb::FieldDefPtr f) {
    upb_selector_t start;
    ASSERT(upb_handlers_getselector(f.ptr(), UPB_HANDLER_STARTSTR, &start));
    upb_selector_t str;
    ASSERT(upb_handlers_getselector(f.ptr(), UPB_HANDLER_STRING, &str));

    ASSERT(!seen_);
    upb::Sink sub;
    sink.StartMessage();
    sink.StartString(start, 0, &sub);
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
  upb_bufhandle handle_;
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
  void Register(upb::HandlersPtr h, upb::FieldDefPtr f) {
    UPB_UNUSED(f);
    ASSERT(h.SetStringHandler(f, UpbMakeHandler(&ME::Handler)));
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
  void Register(upb::HandlersPtr h, upb::FieldDefPtr f) {
    UPB_UNUSED(f);
    ASSERT(h.SetStringHandler(f, UpbMakeHandler(&ME::Handler)));
    handler_data_val_ = kExpectedHandlerData;
  }

 private:
  void Handler(const char *buf, size_t len, const upb_bufhandle* handle) {
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
  void Register(upb::HandlersPtr h, upb::FieldDefPtr f) {
    UPB_UNUSED(f);
    ASSERT(h.SetStringHandler(
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
  void Register(upb::HandlersPtr h, upb::FieldDefPtr f) {
    UPB_UNUSED(f);
    ASSERT(h.SetStringHandler(
        f, UpbBind(&ME::Handler, new int(kExpectedHandlerData))));
  }

 private:
  void Handler(const int* hd, const char* buf, size_t len,
               const upb_bufhandle* handle) {
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
  void Register(upb::HandlersPtr h, upb::FieldDefPtr f) {
    UPB_UNUSED(f);
    ASSERT(h.SetStringHandler(f, UpbMakeHandler(&ME::Handler)));
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
  void Register(upb::HandlersPtr h, upb::FieldDefPtr f) {
    UPB_UNUSED(f);
    ASSERT(h.SetStringHandler(f, UpbMakeHandler(&ME::Handler)));
    handler_data_val_ = kExpectedHandlerData;
  }

 private:
  static void Handler(ME* t, const char* buf, size_t len,
                      const upb_bufhandle* handle) {
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
  void Register(upb::HandlersPtr h, upb::FieldDefPtr f) {
    UPB_UNUSED(f);
    ASSERT(h.SetStringHandler(
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
  void Register(upb::HandlersPtr h, upb::FieldDefPtr f) {
    UPB_UNUSED(f);
    ASSERT(h.SetStringHandler(
        f, UpbBind(&ME::Handler, new int(kExpectedHandlerData))));
  }

 private:
  static void Handler(ME* t, const int* hd, const char* buf, size_t len,
                      const upb_bufhandle* handle) {
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
  void Register(upb::HandlersPtr h, upb::FieldDefPtr f) {
    UPB_UNUSED(f);
    ASSERT(h.SetStringHandler(f, UpbMakeHandler(&ME::Handler)));
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
  void Register(upb::HandlersPtr h, upb::FieldDefPtr f) {
    UPB_UNUSED(f);
    ASSERT(h.SetStringHandler(f, UpbMakeHandler(&ME::Handler)));
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
  static constexpr int kFieldNumber = 3;

  StartMsgTesterBase() : seen_(false), handler_data_val_(0) {}

  void CallAndVerify(upb::Sink sink, upb::FieldDefPtr f) {
    UPB_UNUSED(f);
    ASSERT(!seen_);
    sink.StartMessage();
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
  void Register(upb::HandlersPtr h, upb::FieldDefPtr f) {
    UPB_UNUSED(f);
    ASSERT(h.SetStartMessageHandler(UpbMakeHandler(&Handler)));
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
  void Register(upb::HandlersPtr h, upb::FieldDefPtr f) {
    UPB_UNUSED(f);
    ASSERT(h.SetStartMessageHandler(UpbMakeHandler(&Handler)));
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
  void Register(upb::HandlersPtr h, upb::FieldDefPtr f) {
    UPB_UNUSED(f);
    ASSERT(h.SetStartMessageHandler(UpbMakeHandler(&ME::Handler)));
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
  void Register(upb::HandlersPtr h, upb::FieldDefPtr f) {
    UPB_UNUSED(f);
    ASSERT(h.SetStartMessageHandler(UpbMakeHandler(&ME::Handler)));
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
  void Register(upb::HandlersPtr h, upb::FieldDefPtr f) {
    UPB_UNUSED(f);
    ASSERT(h.SetStartMessageHandler(
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
  void Register(upb::HandlersPtr h, upb::FieldDefPtr f) {
    UPB_UNUSED(f);
    ASSERT(h.SetStartMessageHandler(
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
  void Register(upb::HandlersPtr h, upb::FieldDefPtr f) {
    UPB_UNUSED(f);
    ASSERT(h.SetStartMessageHandler(
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
  void Register(upb::HandlersPtr h, upb::FieldDefPtr f) {
    UPB_UNUSED(f);
    ASSERT(h.SetStartMessageHandler(
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
  static constexpr int kFieldNumber = 1;

  Int32ValueTesterBase() : seen_(false), val_(0), handler_data_val_(0) {}

  void CallAndVerify(upb::Sink sink, upb::FieldDefPtr f) {
    upb_selector_t s;
    ASSERT(upb_handlers_getselector(f.ptr(), UPB_HANDLER_INT32, &s));

    ASSERT(!seen_);
    sink.PutInt32(s, 5);
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
  void Register(upb::HandlersPtr h, upb::FieldDefPtr f) {
    ASSERT(h.SetInt32Handler(f, UpbMakeHandler(&Handler)));
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
  void Register(upb::HandlersPtr h, upb::FieldDefPtr f) {
    ASSERT(h.SetInt32Handler(f, UpbMakeHandler(&Handler)));
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
  void Register(upb::HandlersPtr h, upb::FieldDefPtr f) {
    ASSERT(h.SetInt32Handler(f, UpbMakeHandler(&ME::Handler)));
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
  void Register(upb::HandlersPtr h, upb::FieldDefPtr f) {
    ASSERT(h.SetInt32Handler(f, UpbMakeHandler(&ME::Handler)));
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
  void Register(upb::HandlersPtr h, upb::FieldDefPtr f) {
    ASSERT(h.SetInt32Handler(
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
  void Register(upb::HandlersPtr h, upb::FieldDefPtr f) {
    ASSERT(h.SetInt32Handler(
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
  void Register(upb::HandlersPtr h, upb::FieldDefPtr f) {
    ASSERT(h.SetInt32Handler(
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
  void Register(upb::HandlersPtr h, upb::FieldDefPtr f) {
    ASSERT(h.SetInt32Handler(
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
void RegisterHandlers(const void* closure, upb::Handlers* h_ptr) {
  T* tester = const_cast<T*>(static_cast<const T*>(closure));
  upb::HandlersPtr h(h_ptr);
  upb::FieldDefPtr f = h.message_def().FindFieldByNumber(T::kFieldNumber);
  ASSERT(f);
  tester->Register(h, f);
}

template <class T>
void TestHandler() {
  T tester;
  upb::SymbolTable symtab;
  upb::HandlerCache cache(&RegisterHandlers<T>, &tester);
  upb::MessageDefPtr md(upb_test_TestMessage_getmsgdef(symtab.ptr()));
  ASSERT(md);
  upb::FieldDefPtr f = md.FindFieldByNumber(T::kFieldNumber);
  ASSERT(f);

  const upb::Handlers* h = cache.Get(md);

  upb::Sink sink(h, &tester);
  tester.CallAndVerify(sink, f);
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
void DoNothingEndMessageHandler(C* closure, upb_status *status) {
  UPB_UNUSED(closure);
  UPB_UNUSED(status);
}

void RegisterMismatchedTypes(const void* closure, upb::Handlers* h_ptr) {
  upb::HandlersPtr h(h_ptr);
  UPB_UNUSED(closure);

  upb::MessageDefPtr md(h.message_def());
  ASSERT(md);
  upb::FieldDefPtr i32 = md.FindFieldByName("i32");
  upb::FieldDefPtr r_i32 = md.FindFieldByName("r_i32");
  upb::FieldDefPtr str = md.FindFieldByName("str");
  upb::FieldDefPtr r_str = md.FindFieldByName("r_str");
  upb::FieldDefPtr msg = md.FindFieldByName("msg");
  upb::FieldDefPtr r_msg = md.FindFieldByName("r_msg");
  ASSERT(i32);
  ASSERT(r_i32);
  ASSERT(str);
  ASSERT(r_str);
  ASSERT(msg);
  ASSERT(r_msg);

  // Establish T1 as the top-level closure type.
  ASSERT(h.SetInt32Handler(i32, UpbMakeHandler(DoNothingInt32Handler<T1>)));

  // Now any other attempt to set another handler with T2 as the top-level
  // closure should fail.  But setting these same handlers with T1 as the
  // top-level closure will succeed.
  ASSERT(!h.SetStartMessageHandler(UpbMakeHandler(DoNothingHandler<T2>)));
  ASSERT(h.SetStartMessageHandler(UpbMakeHandler(DoNothingHandler<T1>)));

  ASSERT(
      !h.SetEndMessageHandler(UpbMakeHandler(DoNothingEndMessageHandler<T2>)));
  ASSERT(
      h.SetEndMessageHandler(UpbMakeHandler(DoNothingEndMessageHandler<T1>)));

  ASSERT(!h.SetStartStringHandler(
              str, UpbMakeHandler(DoNothingStartHandler<T1>::String<T2>)));
  ASSERT(h.SetStartStringHandler(
              str, UpbMakeHandler(DoNothingStartHandler<T1>::String<T1>)));

  ASSERT(!h.SetEndStringHandler(str, UpbMakeHandler(DoNothingHandler<T2>)));
  ASSERT(h.SetEndStringHandler(str, UpbMakeHandler(DoNothingHandler<T1>)));

  ASSERT(!h.SetStartSubMessageHandler(
              msg, UpbMakeHandler(DoNothingStartHandler<T1>::Handler<T2>)));
  ASSERT(h.SetStartSubMessageHandler(
              msg, UpbMakeHandler(DoNothingStartHandler<T1>::Handler<T1>)));

  ASSERT(
      !h.SetEndSubMessageHandler(msg, UpbMakeHandler(DoNothingHandler<T2>)));
  ASSERT(
      h.SetEndSubMessageHandler(msg, UpbMakeHandler(DoNothingHandler<T1>)));

  ASSERT(!h.SetStartSequenceHandler(
              r_i32, UpbMakeHandler(DoNothingStartHandler<T1>::Handler<T2>)));
  ASSERT(h.SetStartSequenceHandler(
              r_i32, UpbMakeHandler(DoNothingStartHandler<T1>::Handler<T1>)));

  ASSERT(!h.SetEndSequenceHandler(
              r_i32, UpbMakeHandler(DoNothingHandler<T2>)));
  ASSERT(h.SetEndSequenceHandler(
              r_i32, UpbMakeHandler(DoNothingHandler<T1>)));

  ASSERT(!h.SetStartSequenceHandler(
              r_msg, UpbMakeHandler(DoNothingStartHandler<T1>::Handler<T2>)));
  ASSERT(h.SetStartSequenceHandler(
              r_msg, UpbMakeHandler(DoNothingStartHandler<T1>::Handler<T1>)));

  ASSERT(!h.SetEndSequenceHandler(
              r_msg, UpbMakeHandler(DoNothingHandler<T2>)));
  ASSERT(h.SetEndSequenceHandler(
              r_msg, UpbMakeHandler(DoNothingHandler<T1>)));

  ASSERT(!h.SetStartSequenceHandler(
              r_str, UpbMakeHandler(DoNothingStartHandler<T1>::Handler<T2>)));
  ASSERT(h.SetStartSequenceHandler(
              r_str, UpbMakeHandler(DoNothingStartHandler<T1>::Handler<T1>)));

  ASSERT(!h.SetEndSequenceHandler(
              r_str, UpbMakeHandler(DoNothingHandler<T2>)));
  ASSERT(h.SetEndSequenceHandler(
              r_str, UpbMakeHandler(DoNothingHandler<T1>)));

  // By setting T1 as the return type for the Start* handlers we have
  // established T1 as the type of the sequence and string frames.
  // Setting callbacks that use T2 should fail, but T1 should succeed.
  ASSERT(
      !h.SetStringHandler(str, UpbMakeHandler(DoNothingStringBufHandler<T2>)));
  ASSERT(
      h.SetStringHandler(str, UpbMakeHandler(DoNothingStringBufHandler<T1>)));

  ASSERT(!h.SetInt32Handler(r_i32, UpbMakeHandler(DoNothingInt32Handler<T2>)));
  ASSERT(h.SetInt32Handler(r_i32, UpbMakeHandler(DoNothingInt32Handler<T1>)));

  ASSERT(!h.SetStartSubMessageHandler(
              r_msg, UpbMakeHandler(DoNothingStartHandler<T1>::Handler<T2>)));
  ASSERT(h.SetStartSubMessageHandler(
              r_msg, UpbMakeHandler(DoNothingStartHandler<T1>::Handler<T1>)));

  ASSERT(!h.SetEndSubMessageHandler(r_msg,
                                     UpbMakeHandler(DoNothingHandler<T2>)));
  ASSERT(h.SetEndSubMessageHandler(r_msg,
                                    UpbMakeHandler(DoNothingHandler<T1>)));

  ASSERT(!h.SetStartStringHandler(
              r_str, UpbMakeHandler(DoNothingStartHandler<T1>::String<T2>)));
  ASSERT(h.SetStartStringHandler(
              r_str, UpbMakeHandler(DoNothingStartHandler<T1>::String<T1>)));

  ASSERT(
      !h.SetEndStringHandler(r_str, UpbMakeHandler(DoNothingHandler<T2>)));
  ASSERT(h.SetEndStringHandler(r_str, UpbMakeHandler(DoNothingHandler<T1>)));

  ASSERT(!h.SetStringHandler(r_str,
                              UpbMakeHandler(DoNothingStringBufHandler<T2>)));
  ASSERT(h.SetStringHandler(r_str,
                             UpbMakeHandler(DoNothingStringBufHandler<T1>)));
}

void RegisterMismatchedTypes2(const void* closure, upb::Handlers* h_ptr) {
  upb::HandlersPtr h(h_ptr);
  UPB_UNUSED(closure);

  upb::MessageDefPtr md(h.message_def());
  ASSERT(md);
  upb::FieldDefPtr i32 = md.FindFieldByName("i32");
  upb::FieldDefPtr r_i32 = md.FindFieldByName("r_i32");
  upb::FieldDefPtr str = md.FindFieldByName("str");
  upb::FieldDefPtr r_str = md.FindFieldByName("r_str");
  upb::FieldDefPtr msg = md.FindFieldByName("msg");
  upb::FieldDefPtr r_msg = md.FindFieldByName("r_msg");
  ASSERT(i32);
  ASSERT(r_i32);
  ASSERT(str);
  ASSERT(r_str);
  ASSERT(msg);
  ASSERT(r_msg);

  // For our second test we do the same in reverse.  We directly set the type of
  // the frame and then observe failures at registering a Start* handler that
  // returns a different type.

  // First establish the type of a sequence frame directly.
  ASSERT(h.SetInt32Handler(r_i32, UpbMakeHandler(DoNothingInt32Handler<T1>)));

  // Now setting a StartSequence callback that returns a different type should
  // fail.
  ASSERT(!h.SetStartSequenceHandler(
              r_i32, UpbMakeHandler(DoNothingStartHandler<T2>::Handler<T1>)));
  ASSERT(h.SetStartSequenceHandler(
              r_i32, UpbMakeHandler(DoNothingStartHandler<T1>::Handler<T1>)));

  // Establish a string frame directly.
  ASSERT(h.SetStringHandler(r_str,
                             UpbMakeHandler(DoNothingStringBufHandler<T1>)));

  // Fail setting a StartString callback that returns a different type.
  ASSERT(!h.SetStartStringHandler(
              r_str, UpbMakeHandler(DoNothingStartHandler<T2>::String<T1>)));
  ASSERT(h.SetStartStringHandler(
      r_str, UpbMakeHandler(DoNothingStartHandler<T1>::String<T1>)));

  // The previous established T1 as the frame for the r_str sequence.
  ASSERT(!h.SetStartSequenceHandler(
              r_str, UpbMakeHandler(DoNothingStartHandler<T2>::Handler<T1>)));
  ASSERT(h.SetStartSequenceHandler(
      r_str, UpbMakeHandler(DoNothingStartHandler<T1>::Handler<T1>)));
}

void TestMismatchedTypes() {
  // First create a schema for our test.
  upb::SymbolTable symtab;
  upb::HandlerCache handler_cache(&RegisterMismatchedTypes, nullptr);
  upb::HandlerCache handler_cache2(&RegisterMismatchedTypes2, nullptr);
  const upb::MessageDefPtr md(upb_test_TestMessage_getmsgdef(symtab.ptr()));

  // Now test the type-checking in handler registration.
  handler_cache.Get(md);
  handler_cache2.Get(md);
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

void RegisterIncrementor(const void* closure, upb::Handlers* h_ptr) {
  const int* x = static_cast<const int*>(closure);
  upb::HandlersPtr h(h_ptr);
  upb::FieldDefPtr f = h.message_def().FindFieldByName("i32");
  h.SetInt32Handler(f, UpbBind(&IntIncrementer::Handler,
                               new IntIncrementer(const_cast<int*>(x))));
}

void TestHandlerDataDestruction() {
  int x = 0;

  {
    upb::SymbolTable symtab;
    upb::HandlerCache cache(&RegisterIncrementor, &x);
    upb::MessageDefPtr md(upb_test_TestMessage_getmsgdef(symtab.ptr()));
    cache.Get(md);
    ASSERT(x == 1);
  }

  ASSERT(x == 0);
}

void TestIteration() {
  upb::SymbolTable symtab;
  upb::MessageDefPtr md(upb_test_TestMessage_getmsgdef(symtab.ptr()));

  // Test range-based for on both fields and oneofs (with the iterator adaptor).
  int field_count = 0;
  for (auto field : md.fields()) {
    UPB_UNUSED(field);
    field_count++;
  }
  ASSERT(field_count == md.field_count());

  int oneof_count = 0;
  for (auto oneof : md.oneofs()) {
    UPB_UNUSED(oneof);
    oneof_count++;
  }
  ASSERT(oneof_count == md.oneof_count());
}

void TestArena() {
  int n = 100000;

  struct Decrementer {
    Decrementer(int* _p) : p(_p) {}
    ~Decrementer() { (*p)--; }
    int* p;
  };

  {
    upb::Arena arena;
    for (int i = 0; i < n; i++) {
      arena.Own(new Decrementer(&n));

      // Intersperse allocation and ensure we can write to it.
      int* val = static_cast<int*>(upb_arena_malloc(arena.ptr(), sizeof(int)));
      *val = i;
    }

    // Test a large allocation.
    upb_arena_malloc(arena.ptr(), 1000000);
  }
  ASSERT(n == 0);

  {
    // Test fuse.
    upb::Arena arena1;
    upb::Arena arena2;

    arena1.Fuse(arena2);

    upb_arena_malloc(arena1.ptr(), 10000);
    upb_arena_malloc(arena2.ptr(), 10000);
  }
}

void TestInlinedArena() {
  int n = 100000;

  struct Decrementer {
    Decrementer(int* _p) : p(_p) {}
    ~Decrementer() { (*p)--; }
    int* p;
  };

  {
    upb::InlinedArena<1024> arena;
    for (int i = 0; i < n; i++) {
      arena.Own(new Decrementer(&n));

      // Intersperse allocation and ensure we can write to it.
      int* val = static_cast<int*>(upb_arena_malloc(arena.ptr(), sizeof(int)));
      *val = i;
    }

    // Test a large allocation.
    upb_arena_malloc(arena.ptr(), 1000000);
  }
  ASSERT(n == 0);
}

extern "C" {

int run_tests() {
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
  TestIteration();
  TestArena();

  return 0;
}

}
