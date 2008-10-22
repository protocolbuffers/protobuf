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

namespace Google.ProtocolBuffers {
  /// <summary>
  /// Grab-bag of utility functions useful when dealing with RPCs.
  /// </summary>
  public static class RpcUtil {

    /// <summary>
    /// Converts an Action[IMessage] to an Action[T].
    /// </summary>
    public static Action<T> SpecializeCallback<T>(Action<IMessage> action) 
        where T : IMessage<T> {
      return message => action(message);
    }

    /// <summary>
    /// Converts an Action[T] to an Action[IMessage].
    /// The generalized action will accept any message object which has
    /// the same descriptor, and will convert it to the correct class
    /// before calling the original action. However, if the generalized
    /// callback is given a message with a different descriptor, an
    /// exception will be thrown.
    /// </summary>
    public static Action<IMessage> GeneralizeCallback<TMessage, TBuilder>(Action<TMessage> action, TMessage defaultInstance)
        where TMessage : class, IMessage<TMessage, TBuilder> 
        where TBuilder : IBuilder<TMessage, TBuilder> {
      return message => {
        TMessage castMessage = message as TMessage;
        if (castMessage == null) {  
          castMessage = defaultInstance.CreateBuilderForType().MergeFrom(message).Build();
        }
        action(castMessage);
      };
    }
  }
}
