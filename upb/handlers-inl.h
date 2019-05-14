/*
** Inline definitions for handlers.h, which are particularly long and a bit
** tricky.
*/

#ifndef UPB_HANDLERS_INL_H_
#define UPB_HANDLERS_INL_H_

#include <limits.h>
#include <stddef.h>
#include "upb/handlers.h"

#include "upb/port_def.inc"

#ifdef __cplusplus

/* Type detection and typedefs for integer types.
 * For platforms where there are multiple 32-bit or 64-bit types, we need to be
 * able to enumerate them so we can properly create overloads for all variants.
 *
 * If any platform existed where there were three integer types with the same
 * size, this would have to become more complicated.  For example, short, int,
 * and long could all be 32-bits.  Even more diabolically, short, int, long,
 * and long long could all be 64 bits and still be standard-compliant.
 * However, few platforms are this strange, and it's unlikely that upb will be
 * used on the strangest ones. */

/* Can't count on stdint.h limits like INT32_MAX, because in C++ these are
 * only defined when __STDC_LIMIT_MACROS are defined before the *first* include
 * of stdint.h.  We can't guarantee that someone else didn't include these first
 * without defining __STDC_LIMIT_MACROS. */
#define UPB_INT32_MAX 0x7fffffffLL
#define UPB_INT32_MIN (-UPB_INT32_MAX - 1)
#define UPB_INT64_MAX 0x7fffffffffffffffLL
#define UPB_INT64_MIN (-UPB_INT64_MAX - 1)

#if INT_MAX == UPB_INT32_MAX && INT_MIN == UPB_INT32_MIN
#define UPB_INT_IS_32BITS 1
#endif

#if LONG_MAX == UPB_INT32_MAX && LONG_MIN == UPB_INT32_MIN
#define UPB_LONG_IS_32BITS 1
#endif

#if LONG_MAX == UPB_INT64_MAX && LONG_MIN == UPB_INT64_MIN
#define UPB_LONG_IS_64BITS 1
#endif

#if LLONG_MAX == UPB_INT64_MAX && LLONG_MIN == UPB_INT64_MIN
#define UPB_LLONG_IS_64BITS 1
#endif

/* We use macros instead of typedefs so we can undefine them later and avoid
 * leaking them outside this header file. */
#if UPB_INT_IS_32BITS
#define UPB_INT32_T int
#define UPB_UINT32_T unsigned int

#if UPB_LONG_IS_32BITS
#define UPB_TWO_32BIT_TYPES 1
#define UPB_INT32ALT_T long
#define UPB_UINT32ALT_T unsigned long
#endif  /* UPB_LONG_IS_32BITS */

#elif UPB_LONG_IS_32BITS  /* && !UPB_INT_IS_32BITS */
#define UPB_INT32_T long
#define UPB_UINT32_T unsigned long
#endif  /* UPB_INT_IS_32BITS */


#if UPB_LONG_IS_64BITS
#define UPB_INT64_T long
#define UPB_UINT64_T unsigned long

#if UPB_LLONG_IS_64BITS
#define UPB_TWO_64BIT_TYPES 1
#define UPB_INT64ALT_T long long
#define UPB_UINT64ALT_T unsigned long long
#endif  /* UPB_LLONG_IS_64BITS */

#elif UPB_LLONG_IS_64BITS  /* && !UPB_LONG_IS_64BITS */
#define UPB_INT64_T long long
#define UPB_UINT64_T unsigned long long
#endif  /* UPB_LONG_IS_64BITS */

#undef UPB_INT32_MAX
#undef UPB_INT32_MIN
#undef UPB_INT64_MAX
#undef UPB_INT64_MIN
#undef UPB_INT_IS_32BITS
#undef UPB_LONG_IS_32BITS
#undef UPB_LONG_IS_64BITS
#undef UPB_LLONG_IS_64BITS


namespace upb {

typedef void CleanupFunc(void *ptr);

/* Template to remove "const" from "const T*" and just return "T*".
 *
 * We define a nonsense default because otherwise it will fail to instantiate as
 * a function parameter type even in cases where we don't expect any caller to
 * actually match the overload. */
class CouldntRemoveConst {};
template <class T> struct remove_constptr { typedef CouldntRemoveConst type; };
template <class T> struct remove_constptr<const T *> { typedef T *type; };

/* Template that we use below to remove a template specialization from
 * consideration if it matches a specific type. */
template <class T, class U> struct disable_if_same { typedef void Type; };
template <class T> struct disable_if_same<T, T> {};

template <class T> void DeletePointer(void *p) { delete static_cast<T>(p); }

template <class T1, class T2>
struct FirstUnlessVoidOrBool {
  typedef T1 value;
};

template <class T2>
struct FirstUnlessVoidOrBool<void, T2> {
  typedef T2 value;
};

template <class T2>
struct FirstUnlessVoidOrBool<bool, T2> {
  typedef T2 value;
};

template<class T, class U>
struct is_same {
  static bool value;
};

template<class T>
struct is_same<T, T> {
  static bool value;
};

template<class T, class U>
bool is_same<T, U>::value = false;

template<class T>
bool is_same<T, T>::value = true;

/* FuncInfo *******************************************************************/

/* Info about the user's original, pre-wrapped function. */
template <class C, class R = void>
struct FuncInfo {
  /* The type of the closure that the function takes (its first param). */
  typedef C Closure;

  /* The return type. */
  typedef R Return;
};

/* Func ***********************************************************************/

/* Func1, Func2, Func3: Template classes representing a function and its
 * signature.
 *
 * Since the function is a template parameter, calling the function can be
 * inlined at compile-time and does not require a function pointer at runtime.
 * These functions are not bound to a handler data so have no data or cleanup
 * handler. */
struct UnboundFunc {
  CleanupFunc *GetCleanup() { return nullptr; }
  void *GetData() { return nullptr; }
};

template <class R, class P1, R F(P1), class I>
struct Func1 : public UnboundFunc {
  typedef R Return;
  typedef I FuncInfo;
  static R Call(P1 p1) { return F(p1); }
};

template <class R, class P1, class P2, R F(P1, P2), class I>
struct Func2 : public UnboundFunc {
  typedef R Return;
  typedef I FuncInfo;
  static R Call(P1 p1, P2 p2) { return F(p1, p2); }
};

template <class R, class P1, class P2, class P3, R F(P1, P2, P3), class I>
struct Func3 : public UnboundFunc {
  typedef R Return;
  typedef I FuncInfo;
  static R Call(P1 p1, P2 p2, P3 p3) { return F(p1, p2, p3); }
};

template <class R, class P1, class P2, class P3, class P4, R F(P1, P2, P3, P4),
          class I>
struct Func4 : public UnboundFunc {
  typedef R Return;
  typedef I FuncInfo;
  static R Call(P1 p1, P2 p2, P3 p3, P4 p4) { return F(p1, p2, p3, p4); }
};

template <class R, class P1, class P2, class P3, class P4, class P5,
          R F(P1, P2, P3, P4, P5), class I>
struct Func5 : public UnboundFunc {
  typedef R Return;
  typedef I FuncInfo;
  static R Call(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5) {
    return F(p1, p2, p3, p4, p5);
  }
};

/* BoundFunc ******************************************************************/

/* BoundFunc2, BoundFunc3: Like Func2/Func3 except also contains a value that
 * shall be bound to the function's second parameter.
 * 
 * Note that the second parameter is a const pointer, but our stored bound value
 * is non-const so we can free it when the handlers are destroyed. */
template <class T>
struct BoundFunc {
  typedef typename remove_constptr<T>::type MutableP2;
  explicit BoundFunc(MutableP2 data_) : data(data_) {}
  CleanupFunc *GetCleanup() { return &DeletePointer<MutableP2>; }
  MutableP2 GetData() { return data; }
  MutableP2 data;
};

template <class R, class P1, class P2, R F(P1, P2), class I>
struct BoundFunc2 : public BoundFunc<P2> {
  typedef BoundFunc<P2> Base;
  typedef I FuncInfo;
  explicit BoundFunc2(typename Base::MutableP2 arg) : Base(arg) {}
};

template <class R, class P1, class P2, class P3, R F(P1, P2, P3), class I>
struct BoundFunc3 : public BoundFunc<P2> {
  typedef BoundFunc<P2> Base;
  typedef I FuncInfo;
  explicit BoundFunc3(typename Base::MutableP2 arg) : Base(arg) {}
};

template <class R, class P1, class P2, class P3, class P4, R F(P1, P2, P3, P4),
          class I>
struct BoundFunc4 : public BoundFunc<P2> {
  typedef BoundFunc<P2> Base;
  typedef I FuncInfo;
  explicit BoundFunc4(typename Base::MutableP2 arg) : Base(arg) {}
};

template <class R, class P1, class P2, class P3, class P4, class P5,
          R F(P1, P2, P3, P4, P5), class I>
struct BoundFunc5 : public BoundFunc<P2> {
  typedef BoundFunc<P2> Base;
  typedef I FuncInfo;
  explicit BoundFunc5(typename Base::MutableP2 arg) : Base(arg) {}
};

/* FuncSig ********************************************************************/

/* FuncSig1, FuncSig2, FuncSig3: template classes reflecting a function
 * *signature*, but without a specific function attached.
 *
 * These classes contain member functions that can be invoked with a
 * specific function to return a Func/BoundFunc class. */
template <class R, class P1>
struct FuncSig1 {
  template <R F(P1)>
  Func1<R, P1, F, FuncInfo<P1, R> > GetFunc() {
    return Func1<R, P1, F, FuncInfo<P1, R> >();
  }
};

template <class R, class P1, class P2>
struct FuncSig2 {
  template <R F(P1, P2)>
  Func2<R, P1, P2, F, FuncInfo<P1, R> > GetFunc() {
    return Func2<R, P1, P2, F, FuncInfo<P1, R> >();
  }

  template <R F(P1, P2)>
  BoundFunc2<R, P1, P2, F, FuncInfo<P1, R> > GetFunc(
      typename remove_constptr<P2>::type param2) {
    return BoundFunc2<R, P1, P2, F, FuncInfo<P1, R> >(param2);
  }
};

template <class R, class P1, class P2, class P3>
struct FuncSig3 {
  template <R F(P1, P2, P3)>
  Func3<R, P1, P2, P3, F, FuncInfo<P1, R> > GetFunc() {
    return Func3<R, P1, P2, P3, F, FuncInfo<P1, R> >();
  }

  template <R F(P1, P2, P3)>
  BoundFunc3<R, P1, P2, P3, F, FuncInfo<P1, R> > GetFunc(
      typename remove_constptr<P2>::type param2) {
    return BoundFunc3<R, P1, P2, P3, F, FuncInfo<P1, R> >(param2);
  }
};

template <class R, class P1, class P2, class P3, class P4>
struct FuncSig4 {
  template <R F(P1, P2, P3, P4)>
  Func4<R, P1, P2, P3, P4, F, FuncInfo<P1, R> > GetFunc() {
    return Func4<R, P1, P2, P3, P4, F, FuncInfo<P1, R> >();
  }

  template <R F(P1, P2, P3, P4)>
  BoundFunc4<R, P1, P2, P3, P4, F, FuncInfo<P1, R> > GetFunc(
      typename remove_constptr<P2>::type param2) {
    return BoundFunc4<R, P1, P2, P3, P4, F, FuncInfo<P1, R> >(param2);
  }
};

template <class R, class P1, class P2, class P3, class P4, class P5>
struct FuncSig5 {
  template <R F(P1, P2, P3, P4, P5)>
  Func5<R, P1, P2, P3, P4, P5, F, FuncInfo<P1, R> > GetFunc() {
    return Func5<R, P1, P2, P3, P4, P5, F, FuncInfo<P1, R> >();
  }

  template <R F(P1, P2, P3, P4, P5)>
  BoundFunc5<R, P1, P2, P3, P4, P5, F, FuncInfo<P1, R> > GetFunc(
      typename remove_constptr<P2>::type param2) {
    return BoundFunc5<R, P1, P2, P3, P4, P5, F, FuncInfo<P1, R> >(param2);
  }
};

/* Overloaded template function that can construct the appropriate FuncSig*
 * class given a function pointer by deducing the template parameters. */
template <class R, class P1>
inline FuncSig1<R, P1> MatchFunc(R (*f)(P1)) {
  UPB_UNUSED(f);  /* Only used for template parameter deduction. */
  return FuncSig1<R, P1>();
}

template <class R, class P1, class P2>
inline FuncSig2<R, P1, P2> MatchFunc(R (*f)(P1, P2)) {
  UPB_UNUSED(f);  /* Only used for template parameter deduction. */
  return FuncSig2<R, P1, P2>();
}

template <class R, class P1, class P2, class P3>
inline FuncSig3<R, P1, P2, P3> MatchFunc(R (*f)(P1, P2, P3)) {
  UPB_UNUSED(f);  /* Only used for template parameter deduction. */
  return FuncSig3<R, P1, P2, P3>();
}

template <class R, class P1, class P2, class P3, class P4>
inline FuncSig4<R, P1, P2, P3, P4> MatchFunc(R (*f)(P1, P2, P3, P4)) {
  UPB_UNUSED(f);  /* Only used for template parameter deduction. */
  return FuncSig4<R, P1, P2, P3, P4>();
}

template <class R, class P1, class P2, class P3, class P4, class P5>
inline FuncSig5<R, P1, P2, P3, P4, P5> MatchFunc(R (*f)(P1, P2, P3, P4, P5)) {
  UPB_UNUSED(f);  /* Only used for template parameter deduction. */
  return FuncSig5<R, P1, P2, P3, P4, P5>();
}

/* MethodSig ******************************************************************/

/* CallMethod*: a function template that calls a given method. */
template <class R, class C, R (C::*F)()>
R CallMethod0(C *obj) {
  return ((*obj).*F)();
}

template <class R, class C, class P1, R (C::*F)(P1)>
R CallMethod1(C *obj, P1 arg1) {
  return ((*obj).*F)(arg1);
}

template <class R, class C, class P1, class P2, R (C::*F)(P1, P2)>
R CallMethod2(C *obj, P1 arg1, P2 arg2) {
  return ((*obj).*F)(arg1, arg2);
}

template <class R, class C, class P1, class P2, class P3, R (C::*F)(P1, P2, P3)>
R CallMethod3(C *obj, P1 arg1, P2 arg2, P3 arg3) {
  return ((*obj).*F)(arg1, arg2, arg3);
}

template <class R, class C, class P1, class P2, class P3, class P4,
          R (C::*F)(P1, P2, P3, P4)>
R CallMethod4(C *obj, P1 arg1, P2 arg2, P3 arg3, P4 arg4) {
  return ((*obj).*F)(arg1, arg2, arg3, arg4);
}

/* MethodSig: like FuncSig, but for member functions.
 *
 * GetFunc() returns a normal FuncN object, so after calling GetFunc() no
 * more logic is required to special-case methods. */
template <class R, class C>
struct MethodSig0 {
  template <R (C::*F)()>
  Func1<R, C *, CallMethod0<R, C, F>, FuncInfo<C *, R> > GetFunc() {
    return Func1<R, C *, CallMethod0<R, C, F>, FuncInfo<C *, R> >();
  }
};

template <class R, class C, class P1>
struct MethodSig1 {
  template <R (C::*F)(P1)>
  Func2<R, C *, P1, CallMethod1<R, C, P1, F>, FuncInfo<C *, R> > GetFunc() {
    return Func2<R, C *, P1, CallMethod1<R, C, P1, F>, FuncInfo<C *, R> >();
  }

  template <R (C::*F)(P1)>
  BoundFunc2<R, C *, P1, CallMethod1<R, C, P1, F>, FuncInfo<C *, R> > GetFunc(
      typename remove_constptr<P1>::type param1) {
    return BoundFunc2<R, C *, P1, CallMethod1<R, C, P1, F>, FuncInfo<C *, R> >(
        param1);
  }
};

template <class R, class C, class P1, class P2>
struct MethodSig2 {
  template <R (C::*F)(P1, P2)>
  Func3<R, C *, P1, P2, CallMethod2<R, C, P1, P2, F>, FuncInfo<C *, R> >
  GetFunc() {
    return Func3<R, C *, P1, P2, CallMethod2<R, C, P1, P2, F>,
                 FuncInfo<C *, R> >();
  }

  template <R (C::*F)(P1, P2)>
  BoundFunc3<R, C *, P1, P2, CallMethod2<R, C, P1, P2, F>, FuncInfo<C *, R> >
  GetFunc(typename remove_constptr<P1>::type param1) {
    return BoundFunc3<R, C *, P1, P2, CallMethod2<R, C, P1, P2, F>,
                      FuncInfo<C *, R> >(param1);
  }
};

template <class R, class C, class P1, class P2, class P3>
struct MethodSig3 {
  template <R (C::*F)(P1, P2, P3)>
  Func4<R, C *, P1, P2, P3, CallMethod3<R, C, P1, P2, P3, F>, FuncInfo<C *, R> >
  GetFunc() {
    return Func4<R, C *, P1, P2, P3, CallMethod3<R, C, P1, P2, P3, F>,
                 FuncInfo<C *, R> >();
  }

  template <R (C::*F)(P1, P2, P3)>
  BoundFunc4<R, C *, P1, P2, P3, CallMethod3<R, C, P1, P2, P3, F>,
             FuncInfo<C *, R> >
  GetFunc(typename remove_constptr<P1>::type param1) {
    return BoundFunc4<R, C *, P1, P2, P3, CallMethod3<R, C, P1, P2, P3, F>,
                      FuncInfo<C *, R> >(param1);
  }
};

template <class R, class C, class P1, class P2, class P3, class P4>
struct MethodSig4 {
  template <R (C::*F)(P1, P2, P3, P4)>
  Func5<R, C *, P1, P2, P3, P4, CallMethod4<R, C, P1, P2, P3, P4, F>,
        FuncInfo<C *, R> >
  GetFunc() {
    return Func5<R, C *, P1, P2, P3, P4, CallMethod4<R, C, P1, P2, P3, P4, F>,
                 FuncInfo<C *, R> >();
  }

  template <R (C::*F)(P1, P2, P3, P4)>
  BoundFunc5<R, C *, P1, P2, P3, P4, CallMethod4<R, C, P1, P2, P3, P4, F>,
             FuncInfo<C *, R> >
  GetFunc(typename remove_constptr<P1>::type param1) {
    return BoundFunc5<R, C *, P1, P2, P3, P4,
                      CallMethod4<R, C, P1, P2, P3, P4, F>, FuncInfo<C *, R> >(
        param1);
  }
};

template <class R, class C>
inline MethodSig0<R, C> MatchFunc(R (C::*f)()) {
  UPB_UNUSED(f);  /* Only used for template parameter deduction. */
  return MethodSig0<R, C>();
}

template <class R, class C, class P1>
inline MethodSig1<R, C, P1> MatchFunc(R (C::*f)(P1)) {
  UPB_UNUSED(f);  /* Only used for template parameter deduction. */
  return MethodSig1<R, C, P1>();
}

template <class R, class C, class P1, class P2>
inline MethodSig2<R, C, P1, P2> MatchFunc(R (C::*f)(P1, P2)) {
  UPB_UNUSED(f);  /* Only used for template parameter deduction. */
  return MethodSig2<R, C, P1, P2>();
}

template <class R, class C, class P1, class P2, class P3>
inline MethodSig3<R, C, P1, P2, P3> MatchFunc(R (C::*f)(P1, P2, P3)) {
  UPB_UNUSED(f);  /* Only used for template parameter deduction. */
  return MethodSig3<R, C, P1, P2, P3>();
}

template <class R, class C, class P1, class P2, class P3, class P4>
inline MethodSig4<R, C, P1, P2, P3, P4> MatchFunc(R (C::*f)(P1, P2, P3, P4)) {
  UPB_UNUSED(f);  /* Only used for template parameter deduction. */
  return MethodSig4<R, C, P1, P2, P3, P4>();
}

/* MaybeWrapReturn ************************************************************/

/* Template class that attempts to wrap the return value of the function so it
 * matches the expected type.  There are two main adjustments it may make:
 *
 *   1. If the function returns void, make it return the expected type and with
 *      a value that always indicates success.
 *   2. If the function returns bool, make it return the expected type with a
 *      value that indicates success or failure.
 *
 * The "expected type" for return is:
 *   1. void* for start handlers.  If the closure parameter has a different type
 *      we will cast it to void* for the return in the success case.
 *   2. size_t for string buffer handlers.
 *   3. bool for everything else. */

/* Template parameters are FuncN type and desired return type. */
template <class F, class R, class Enable = void>
struct MaybeWrapReturn;

/* If the return type matches, return the given function unwrapped. */
template <class F>
struct MaybeWrapReturn<F, typename F::Return> {
  typedef F Func;
};

/* Function wrapper that munges the return value from void to (bool)true. */
template <class P1, class P2, void F(P1, P2)>
bool ReturnTrue2(P1 p1, P2 p2) {
  F(p1, p2);
  return true;
}

template <class P1, class P2, class P3, void F(P1, P2, P3)>
bool ReturnTrue3(P1 p1, P2 p2, P3 p3) {
  F(p1, p2, p3);
  return true;
}

/* Function wrapper that munges the return value from void to (void*)arg1  */
template <class P1, class P2, void F(P1, P2)>
void *ReturnClosure2(P1 p1, P2 p2) {
  F(p1, p2);
  return p1;
}

template <class P1, class P2, class P3, void F(P1, P2, P3)>
void *ReturnClosure3(P1 p1, P2 p2, P3 p3) {
  F(p1, p2, p3);
  return p1;
}

/* Function wrapper that munges the return value from R to void*. */
template <class R, class P1, class P2, R F(P1, P2)>
void *CastReturnToVoidPtr2(P1 p1, P2 p2) {
  return F(p1, p2);
}

template <class R, class P1, class P2, class P3, R F(P1, P2, P3)>
void *CastReturnToVoidPtr3(P1 p1, P2 p2, P3 p3) {
  return F(p1, p2, p3);
}

/* Function wrapper that munges the return value from bool to void*. */
template <class P1, class P2, bool F(P1, P2)>
void *ReturnClosureOrBreak2(P1 p1, P2 p2) {
  return F(p1, p2) ? p1 : UPB_BREAK;
}

template <class P1, class P2, class P3, bool F(P1, P2, P3)>
void *ReturnClosureOrBreak3(P1 p1, P2 p2, P3 p3) {
  return F(p1, p2, p3) ? p1 : UPB_BREAK;
}

/* For the string callback, which takes five params, returns the size param. */
template <class P1, class P2,
          void F(P1, P2, const char *, size_t, const upb_bufhandle *)>
size_t ReturnStringLen(P1 p1, P2 p2, const char *p3, size_t p4,
                       const upb_bufhandle *p5) {
  F(p1, p2, p3, p4, p5);
  return p4;
}

/* For the string callback, which takes five params, returns the size param or
 * zero. */
template <class P1, class P2,
          bool F(P1, P2, const char *, size_t, const upb_bufhandle *)>
size_t ReturnNOr0(P1 p1, P2 p2, const char *p3, size_t p4,
                  const upb_bufhandle *p5) {
  return F(p1, p2, p3, p4, p5) ? p4 : 0;
}

/* If we have a function returning void but want a function returning bool, wrap
 * it in a function that returns true. */
template <class P1, class P2, void F(P1, P2), class I>
struct MaybeWrapReturn<Func2<void, P1, P2, F, I>, bool> {
  typedef Func2<bool, P1, P2, ReturnTrue2<P1, P2, F>, I> Func;
};

template <class P1, class P2, class P3, void F(P1, P2, P3), class I>
struct MaybeWrapReturn<Func3<void, P1, P2, P3, F, I>, bool> {
  typedef Func3<bool, P1, P2, P3, ReturnTrue3<P1, P2, P3, F>, I> Func;
};

/* If our function returns void but we want one returning void*, wrap it in a
 * function that returns the first argument. */
template <class P1, class P2, void F(P1, P2), class I>
struct MaybeWrapReturn<Func2<void, P1, P2, F, I>, void *> {
  typedef Func2<void *, P1, P2, ReturnClosure2<P1, P2, F>, I> Func;
};

template <class P1, class P2, class P3, void F(P1, P2, P3), class I>
struct MaybeWrapReturn<Func3<void, P1, P2, P3, F, I>, void *> {
  typedef Func3<void *, P1, P2, P3, ReturnClosure3<P1, P2, P3, F>, I> Func;
};

/* If our function returns R* but we want one returning void*, wrap it in a
 * function that casts to void*. */
template <class R, class P1, class P2, R *F(P1, P2), class I>
struct MaybeWrapReturn<Func2<R *, P1, P2, F, I>, void *,
                       typename disable_if_same<R *, void *>::Type> {
  typedef Func2<void *, P1, P2, CastReturnToVoidPtr2<R *, P1, P2, F>, I> Func;
};

template <class R, class P1, class P2, class P3, R *F(P1, P2, P3), class I>
struct MaybeWrapReturn<Func3<R *, P1, P2, P3, F, I>, void *,
                       typename disable_if_same<R *, void *>::Type> {
  typedef Func3<void *, P1, P2, P3, CastReturnToVoidPtr3<R *, P1, P2, P3, F>, I>
      Func;
};

/* If our function returns bool but we want one returning void*, wrap it in a
 * function that returns either the first param or UPB_BREAK. */
template <class P1, class P2, bool F(P1, P2), class I>
struct MaybeWrapReturn<Func2<bool, P1, P2, F, I>, void *> {
  typedef Func2<void *, P1, P2, ReturnClosureOrBreak2<P1, P2, F>, I> Func;
};

template <class P1, class P2, class P3, bool F(P1, P2, P3), class I>
struct MaybeWrapReturn<Func3<bool, P1, P2, P3, F, I>, void *> {
  typedef Func3<void *, P1, P2, P3, ReturnClosureOrBreak3<P1, P2, P3, F>, I>
      Func;
};

/* If our function returns void but we want one returning size_t, wrap it in a
 * function that returns the size argument. */
template <class P1, class P2,
          void F(P1, P2, const char *, size_t, const upb_bufhandle *), class I>
struct MaybeWrapReturn<
    Func5<void, P1, P2, const char *, size_t, const upb_bufhandle *, F, I>,
          size_t> {
  typedef Func5<size_t, P1, P2, const char *, size_t, const upb_bufhandle *,
                ReturnStringLen<P1, P2, F>, I> Func;
};

/* If our function returns bool but we want one returning size_t, wrap it in a
 * function that returns either 0 or the buf size. */
template <class P1, class P2,
          bool F(P1, P2, const char *, size_t, const upb_bufhandle *), class I>
struct MaybeWrapReturn<
    Func5<bool, P1, P2, const char *, size_t, const upb_bufhandle *, F, I>,
    size_t> {
  typedef Func5<size_t, P1, P2, const char *, size_t, const upb_bufhandle *,
                ReturnNOr0<P1, P2, F>, I> Func;
};

/* ConvertParams **************************************************************/

/* Template class that converts the function parameters if necessary, and
 * ignores the HandlerData parameter if appropriate.
 *
 * Template parameter is the are FuncN function type. */
template <class F, class T>
struct ConvertParams;

/* Function that discards the handler data parameter. */
template <class R, class P1, R F(P1)>
R IgnoreHandlerData2(void *p1, const void *hd) {
  UPB_UNUSED(hd);
  return F(static_cast<P1>(p1));
}

template <class R, class P1, class P2Wrapper, class P2Wrapped,
          R F(P1, P2Wrapped)>
R IgnoreHandlerData3(void *p1, const void *hd, P2Wrapper p2) {
  UPB_UNUSED(hd);
  return F(static_cast<P1>(p1), p2);
}

template <class R, class P1, class P2, class P3, R F(P1, P2, P3)>
R IgnoreHandlerData4(void *p1, const void *hd, P2 p2, P3 p3) {
  UPB_UNUSED(hd);
  return F(static_cast<P1>(p1), p2, p3);
}

template <class R, class P1, class P2, class P3, class P4, R F(P1, P2, P3, P4)>
R IgnoreHandlerData5(void *p1, const void *hd, P2 p2, P3 p3, P4 p4) {
  UPB_UNUSED(hd);
  return F(static_cast<P1>(p1), p2, p3, p4);
}

template <class R, class P1, R F(P1, const char*, size_t)>
R IgnoreHandlerDataIgnoreHandle(void *p1, const void *hd, const char *p2,
                                size_t p3, const upb_bufhandle *handle) {
  UPB_UNUSED(hd);
  UPB_UNUSED(handle);
  return F(static_cast<P1>(p1), p2, p3);
}

/* Function that casts the handler data parameter. */
template <class R, class P1, class P2, R F(P1, P2)>
R CastHandlerData2(void *c, const void *hd) {
  return F(static_cast<P1>(c), static_cast<P2>(hd));
}

template <class R, class P1, class P2, class P3Wrapper, class P3Wrapped,
          R F(P1, P2, P3Wrapped)>
R CastHandlerData3(void *c, const void *hd, P3Wrapper p3) {
  return F(static_cast<P1>(c), static_cast<P2>(hd), p3);
}

template <class R, class P1, class P2, class P3, class P4, class P5,
          R F(P1, P2, P3, P4, P5)>
R CastHandlerData5(void *c, const void *hd, P3 p3, P4 p4, P5 p5) {
  return F(static_cast<P1>(c), static_cast<P2>(hd), p3, p4, p5);
}

template <class R, class P1, class P2, R F(P1, P2, const char *, size_t)>
R CastHandlerDataIgnoreHandle(void *c, const void *hd, const char *p3,
                              size_t p4, const upb_bufhandle *handle) {
  UPB_UNUSED(handle);
  return F(static_cast<P1>(c), static_cast<P2>(hd), p3, p4);
}

/* For unbound functions, ignore the handler data. */
template <class R, class P1, R F(P1), class I, class T>
struct ConvertParams<Func1<R, P1, F, I>, T> {
  typedef Func2<R, void *, const void *, IgnoreHandlerData2<R, P1, F>, I> Func;
};

template <class R, class P1, class P2, R F(P1, P2), class I,
          class R2, class P1_2, class P2_2, class P3_2>
struct ConvertParams<Func2<R, P1, P2, F, I>,
                     R2 (*)(P1_2, P2_2, P3_2)> {
  typedef Func3<R, void *, const void *, P3_2,
                IgnoreHandlerData3<R, P1, P3_2, P2, F>, I> Func;
};

/* For StringBuffer only; this ignores both the handler data and the
 * upb_bufhandle. */
template <class R, class P1, R F(P1, const char *, size_t), class I, class T>
struct ConvertParams<Func3<R, P1, const char *, size_t, F, I>, T> {
  typedef Func5<R, void *, const void *, const char *, size_t,
                const upb_bufhandle *, IgnoreHandlerDataIgnoreHandle<R, P1, F>,
                I> Func;
};

template <class R, class P1, class P2, class P3, class P4, R F(P1, P2, P3, P4),
          class I, class T>
struct ConvertParams<Func4<R, P1, P2, P3, P4, F, I>, T> {
  typedef Func5<R, void *, const void *, P2, P3, P4,
                IgnoreHandlerData5<R, P1, P2, P3, P4, F>, I> Func;
};

/* For bound functions, cast the handler data. */
template <class R, class P1, class P2, R F(P1, P2), class I, class T>
struct ConvertParams<BoundFunc2<R, P1, P2, F, I>, T> {
  typedef Func2<R, void *, const void *, CastHandlerData2<R, P1, P2, F>, I>
      Func;
};

template <class R, class P1, class P2, class P3, R F(P1, P2, P3), class I,
          class R2, class P1_2, class P2_2, class P3_2>
struct ConvertParams<BoundFunc3<R, P1, P2, P3, F, I>,
                     R2 (*)(P1_2, P2_2, P3_2)> {
  typedef Func3<R, void *, const void *, P3_2,
                CastHandlerData3<R, P1, P2, P3_2, P3, F>, I> Func;
};

/* For StringBuffer only; this ignores the upb_bufhandle. */
template <class R, class P1, class P2, R F(P1, P2, const char *, size_t),
          class I, class T>
struct ConvertParams<BoundFunc4<R, P1, P2, const char *, size_t, F, I>, T> {
  typedef Func5<R, void *, const void *, const char *, size_t,
                const upb_bufhandle *,
                CastHandlerDataIgnoreHandle<R, P1, P2, F>, I>
      Func;
};

template <class R, class P1, class P2, class P3, class P4, class P5,
          R F(P1, P2, P3, P4, P5), class I, class T>
struct ConvertParams<BoundFunc5<R, P1, P2, P3, P4, P5, F, I>, T> {
  typedef Func5<R, void *, const void *, P3, P4, P5,
                CastHandlerData5<R, P1, P2, P3, P4, P5, F>, I> Func;
};

/* utype/ltype are upper/lower-case, ctype is canonical C type, vtype is
 * variant C type. */
#define TYPE_METHODS(utype, ltype, ctype, vtype)                      \
  template <>                                                         \
  struct CanonicalType<vtype> {                                       \
    typedef ctype Type;                                               \
  };                                                                  \
  template <>                                                         \
  inline bool HandlersPtr::SetValueHandler<vtype>(                    \
      FieldDefPtr f, const HandlersPtr::utype##Handler &handler) {    \
    handler.AddCleanup(ptr());                                        \
    return upb_handlers_set##ltype(ptr(), f.ptr(), handler.handler(), \
                                   &handler.attr());                  \
  }

TYPE_METHODS(Double, double, double,   double)
TYPE_METHODS(Float,  float,  float,    float)
TYPE_METHODS(UInt64, uint64, uint64_t, UPB_UINT64_T)
TYPE_METHODS(UInt32, uint32, uint32_t, UPB_UINT32_T)
TYPE_METHODS(Int64,  int64,  int64_t,  UPB_INT64_T)
TYPE_METHODS(Int32,  int32,  int32_t,  UPB_INT32_T)
TYPE_METHODS(Bool,   bool,   bool,     bool)

#ifdef UPB_TWO_32BIT_TYPES
TYPE_METHODS(Int32,  int32,  int32_t,  UPB_INT32ALT_T)
TYPE_METHODS(UInt32, uint32, uint32_t, UPB_UINT32ALT_T)
#endif

#ifdef UPB_TWO_64BIT_TYPES
TYPE_METHODS(Int64,  int64,  int64_t,  UPB_INT64ALT_T)
TYPE_METHODS(UInt64, uint64, uint64_t, UPB_UINT64ALT_T)
#endif
#undef TYPE_METHODS

template <> struct CanonicalType<Status*> {
  typedef Status* Type;
};

template <class F> struct ReturnOf;

template <class R, class P1, class P2>
struct ReturnOf<R (*)(P1, P2)> {
  typedef R Return;
};

template <class R, class P1, class P2, class P3>
struct ReturnOf<R (*)(P1, P2, P3)> {
  typedef R Return;
};

template <class R, class P1, class P2, class P3, class P4>
struct ReturnOf<R (*)(P1, P2, P3, P4)> {
  typedef R Return;
};

template <class R, class P1, class P2, class P3, class P4, class P5>
struct ReturnOf<R (*)(P1, P2, P3, P4, P5)> {
  typedef R Return;
};


template <class T>
template <class F>
inline Handler<T>::Handler(F func)
    : registered_(false),
      cleanup_data_(func.GetData()),
      cleanup_func_(func.GetCleanup()) {
  attr_.handler_data = func.GetData();
  typedef typename ReturnOf<T>::Return Return;
  typedef typename ConvertParams<F, T>::Func ConvertedParamsFunc;
  typedef typename MaybeWrapReturn<ConvertedParamsFunc, Return>::Func
      ReturnWrappedFunc;
  handler_ = ReturnWrappedFunc().Call;

  /* Set attributes based on what templates can statically tell us about the
   * user's function. */

  /* If the original function returns void, then we know that we wrapped it to
   * always return ok. */
  bool always_ok = is_same<typename F::FuncInfo::Return, void>::value;
  attr_.alwaysok = always_ok;

  /* Closure parameter and return type. */
  attr_.closure_type = UniquePtrForType<typename F::FuncInfo::Closure>();

  /* We use the closure type (from the first parameter) if the return type is
   * void or bool, since these are the two cases we wrap to return the closure's
   * type anyway.
   *
   * This is all nonsense for non START* handlers, but it doesn't matter because
   * in that case the value will be ignored. */
  typedef typename FirstUnlessVoidOrBool<typename F::FuncInfo::Return,
                                         typename F::FuncInfo::Closure>::value
      EffectiveReturn;
  attr_.return_closure_type = UniquePtrForType<EffectiveReturn>();
}

template <class T>
inline void Handler<T>::AddCleanup(upb_handlers* h) const {
  UPB_ASSERT(!registered_);
  registered_ = true;
  if (cleanup_func_) {
    bool ok = upb_handlers_addcleanup(h, cleanup_data_, cleanup_func_);
    UPB_ASSERT(ok);
  }
}

}  /* namespace upb */

#endif  /* __cplusplus */


#undef UPB_TWO_32BIT_TYPES
#undef UPB_TWO_64BIT_TYPES
#undef UPB_INT32_T
#undef UPB_UINT32_T
#undef UPB_INT32ALT_T
#undef UPB_UINT32ALT_T
#undef UPB_INT64_T
#undef UPB_UINT64_T
#undef UPB_INT64ALT_T
#undef UPB_UINT64ALT_T

#include "upb/port_undef.inc"

#endif  /* UPB_HANDLERS_INL_H_ */
