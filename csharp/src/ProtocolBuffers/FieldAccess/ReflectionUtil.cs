#region Copyright notice and license
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
#endregion

using System;
using System.Reflection;

namespace Google.Protobuf.FieldAccess
{
    /// <summary>
    /// The methods in this class are somewhat evil, and should not be tampered with lightly.
    /// Basically they allow the creation of relatively weakly typed delegates from MethodInfos
    /// which are more strongly typed. They do this by creating an appropriate strongly typed
    /// delegate from the MethodInfo, and then calling that within an anonymous method.
    /// Mind-bending stuff (at least to your humble narrator) but the resulting delegates are
    /// very fast compared with calling Invoke later on.
    /// </summary>
    internal static class ReflectionUtil
    {
        /// <summary>
        /// Empty Type[] used when calling GetProperty to force property instead of indexer fetching.
        /// </summary>
        internal static readonly Type[] EmptyTypes = new Type[0];

        /// <summary>
        /// Creates a delegate which will execute the given method and then return
        /// the result as an object.
        /// </summary>
        public static Func<T, object> CreateUpcastDelegate<T>(MethodInfo method)
        {
            // The tricky bit is invoking CreateCreateUpcastDelegateImpl with the right type parameters
            MethodInfo openImpl = typeof(ReflectionUtil).GetMethod("CreateUpcastDelegateImpl");
            MethodInfo closedImpl = openImpl.MakeGenericMethod(typeof(T), method.ReturnType);
            return (Func<T, object>) closedImpl.Invoke(null, new object[] {method});
        }

        /// <summary>
        /// Method used solely for implementing CreateUpcastDelegate. Public to avoid trust issues
        /// in low-trust scenarios.
        /// </summary>
        public static Func<TSource, object> CreateUpcastDelegateImpl<TSource, TResult>(MethodInfo method)
        {
            // Convert the reflection call into an open delegate, i.e. instead of calling x.Method()
            // we'll call getter(x).
            Func<TSource, TResult> getter = ReflectionUtil.CreateDelegateFunc<TSource, TResult>(method);

            // Implicit upcast to object (within the delegate)
            return source => getter(source);
        }

        /// <summary>
        /// Creates a delegate which will execute the given method after casting the parameter
        /// down from object to the required parameter type.
        /// </summary>
        public static Action<T, object> CreateDowncastDelegate<T>(MethodInfo method)
        {
            MethodInfo openImpl = typeof(ReflectionUtil).GetMethod("CreateDowncastDelegateImpl");
            MethodInfo closedImpl = openImpl.MakeGenericMethod(typeof(T), method.GetParameters()[0].ParameterType);
            return (Action<T, object>) closedImpl.Invoke(null, new object[] {method});
        }

        public static Action<TSource, object> CreateDowncastDelegateImpl<TSource, TParam>(MethodInfo method)
        {
            // Convert the reflection call into an open delegate, i.e. instead of calling x.Method(y) we'll
            // call Method(x, y)
            Action<TSource, TParam> call = ReflectionUtil.CreateDelegateAction<TSource, TParam>(method);

            return (source, parameter) => call(source, (TParam) parameter);
        }

        /// <summary>
        /// Creates a delegate which will execute the given method after casting the parameter
        /// down from object to the required parameter type.
        /// </summary>
        public static Action<T, object> CreateDowncastDelegateIgnoringReturn<T>(MethodInfo method)
        {
            MethodInfo openImpl = typeof(ReflectionUtil).GetMethod("CreateDowncastDelegateIgnoringReturnImpl");
            MethodInfo closedImpl = openImpl.MakeGenericMethod(typeof(T), method.GetParameters()[0].ParameterType,
                                                               method.ReturnType);
            return (Action<T, object>) closedImpl.Invoke(null, new object[] {method});
        }

        public static Action<TSource, object> CreateDowncastDelegateIgnoringReturnImpl<TSource, TParam, TReturn>(
            MethodInfo method)
        {
            // Convert the reflection call into an open delegate, i.e. instead of calling x.Method(y) we'll
            // call Method(x, y)
            Func<TSource, TParam, TReturn> call = ReflectionUtil.CreateDelegateFunc<TSource, TParam, TReturn>(method);

            return delegate(TSource source, object parameter) { call(source, (TParam) parameter); };
        }

        internal static Func<TResult> CreateDelegateFunc<TResult>(MethodInfo method)
        {
            object tdelegate = Delegate.CreateDelegate(typeof(Func<TResult>), null, method);
            return (Func<TResult>)tdelegate;
        }

        internal static Func<T, TResult> CreateDelegateFunc<T, TResult>(MethodInfo method)
        {
            object tdelegate = Delegate.CreateDelegate(typeof(Func<T, TResult>), null, method);
            return (Func<T, TResult>)tdelegate;
        }

        internal static Func<T1, T2, TResult> CreateDelegateFunc<T1, T2, TResult>(MethodInfo method)
        {
            object tdelegate = Delegate.CreateDelegate(typeof(Func<T1, T2, TResult>), null, method);
            return (Func<T1, T2, TResult>)tdelegate;
        }

        internal static Action<T> CreateDelegateAction<T>(MethodInfo method)
        {
            object tdelegate = Delegate.CreateDelegate(typeof(Action<T>), null, method);
            return (Action<T>)tdelegate;
        }

        internal static Action<T1, T2> CreateDelegateAction<T1, T2>(MethodInfo method)
        {
            object tdelegate = Delegate.CreateDelegate(typeof(Action<T1, T2>), null, method);
            return (Action<T1, T2>)tdelegate;
        }
    }
}