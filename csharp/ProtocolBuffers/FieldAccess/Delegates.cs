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

namespace Google.ProtocolBuffers.FieldAccess {

  // These delegate declarations mirror the ones in .NET 3.5 for the sake of familiarity.
  internal delegate TResult Func<TResult>();
  internal delegate TResult Func<T, TResult>(T arg);
  internal delegate TResult Func<T1, T2, TResult>(T1 arg1, T2 arg2);
  internal delegate TResult Func<T1, T2, T3, TResult>(T1 arg1, T2 arg2, T3 arg3);
  internal delegate TResult Func<T1, T2, T3, T4, TResult>(T1 arg1, T2 arg2, T3 arg3, T4 arg4); 
  internal delegate void Action<T1, T2>(T1 arg1, T2 arg2);
}
