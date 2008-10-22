// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.
// http://code.google.com/p/protobuf/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
using System;
using System.Reflection;

namespace Google.ProtocolBuffers.FieldAccess {

  /// <summary>
  /// The methods in this class are somewhat evil, and should not be tampered with lightly.
  /// Basically they allow the creation of relatively weakly typed delegates from MethodInfos
  /// which are more strongly typed. They do this by creating an appropriate strongly typed
  /// delegate from the MethodInfo, and then calling that within an anonymous method.
  /// Mind-bending stuff (at least to your humble narrator) but the resulting delegates are
  /// very fast compared with calling Invoke later on.
  /// </summary>
  internal static class ReflectionUtil {

    /// <summary>
    /// Creates a delegate which will execute the given method and then return
    /// the result as an object.
    /// </summary>
    public static Func<T, object> CreateUpcastDelegate<T>(MethodInfo method) {

      // The tricky bit is invoking CreateCreateUpcastDelegateImpl with the right type parameters
      MethodInfo openImpl = typeof(ReflectionUtil).GetMethod("CreateUpcastDelegateImpl");
      MethodInfo closedImpl = openImpl.MakeGenericMethod(typeof(T), method.ReturnType);
      return (Func<T, object>) closedImpl.Invoke(null, new object[] { method });
    }

    /// <summary>
    /// Method used solely for implementing CreateUpcastDelegate. Public to avoid trust issues
    /// in low-trust scenarios, e.g. Silverlight.
    /// TODO(jonskeet): Check any of this actually works in Silverlight...
    /// </summary>
    public static Func<TSource, object> CreateUpcastDelegateImpl<TSource, TResult>(MethodInfo method) {
      // Convert the reflection call into an open delegate, i.e. instead of calling x.Method()
      // we'll call getter(x).
      Func<TSource, TResult> getter = (Func<TSource, TResult>)Delegate.CreateDelegate(typeof(Func<TSource, TResult>), method);

      // Implicit upcast to object (within the delegate)
      return delegate(TSource source) { return getter(source); };
    }


    /// <summary>
    /// Creates a delegate which will execute the given method after casting the parameter
    /// down from object to the required parameter type.
    /// </summary>
    public static Action<T, object> CreateDowncastDelegate<T>(MethodInfo method) {
      MethodInfo openImpl = typeof(ReflectionUtil).GetMethod("CreateDowncastDelegateImpl");
      MethodInfo closedImpl = openImpl.MakeGenericMethod(typeof(T), method.GetParameters()[0].ParameterType);
      return (Action<T, object>) closedImpl.Invoke(null, new object[] { method });
    }

    public static Action<TSource, object> CreateDowncastDelegateImpl<TSource, TParam>(MethodInfo method) {
      // Convert the reflection call into an open delegate, i.e. instead of calling x.Method(y) we'll
      // call Method(x, y)
      Action<TSource, TParam> call = (Action<TSource, TParam>) Delegate.CreateDelegate(typeof(Action<TSource, TParam>), method);

      return delegate(TSource source, object parameter) { call(source, (TParam)parameter); };
    }

    /// <summary>
    /// Creates a delegate which will execute the given method after casting the parameter
    /// down from object to the required parameter type.
    /// </summary>
    public static Action<T, object> CreateDowncastDelegateIgnoringReturn<T>(MethodInfo method) {
      MethodInfo openImpl = typeof(ReflectionUtil).GetMethod("CreateDowncastDelegateIgnoringReturnImpl");
      MethodInfo closedImpl = openImpl.MakeGenericMethod(typeof(T), method.GetParameters()[0].ParameterType, method.ReturnType);
      return (Action<T, object>)closedImpl.Invoke(null, new object[] { method });
    }

    public static Action<TSource, object> CreateDowncastDelegateIgnoringReturnImpl<TSource, TParam, TReturn>(MethodInfo method) {
      // Convert the reflection call into an open delegate, i.e. instead of calling x.Method(y) we'll
      // call Method(x, y)
      Func<TSource, TParam, TReturn> call = (Func<TSource, TParam, TReturn>)
          Delegate.CreateDelegate(typeof(Func<TSource, TParam, TReturn>), method);

      return delegate(TSource source, object parameter) { call(source, (TParam)parameter); };
    }

    /// <summary>
    /// Creates a delegate which will execute the given static method and cast the result up to IBuilder.
    /// </summary>
    public static Func<IBuilder> CreateStaticUpcastDelegate(MethodInfo method) {
      MethodInfo openImpl = typeof(ReflectionUtil).GetMethod("CreateStaticUpcastDelegateImpl");
      MethodInfo closedImpl = openImpl.MakeGenericMethod(method.ReturnType);
      return (Func<IBuilder>)closedImpl.Invoke(null, new object[] { method });
    }

    public static Func<IBuilder> CreateStaticUpcastDelegateImpl<T>(MethodInfo method) {
      Func<T> call = (Func<T>)Delegate.CreateDelegate(typeof(Func<T>), method);
      return delegate { return (IBuilder)call(); };
    }
  }
}
