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

  // TODO(jonskeet): Convert these to Func/Action family
  delegate bool HasDelegate<T>(T message);
  delegate T ClearDelegate<T>(T builder);
  delegate int RepeatedCountDelegate<T>(T message);
  delegate object GetValueDelegate<T>(T message);
  delegate void SingleValueDelegate<TSource>(TSource source, object value);
  delegate IBuilder CreateBuilderDelegate();
  delegate object GetIndexedValueDelegate<T>(T message, int index);
  delegate object SetIndexedValueDelegate<T>(T message, int index, object value);
}
