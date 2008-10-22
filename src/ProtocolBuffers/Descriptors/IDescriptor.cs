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

namespace Google.ProtocolBuffers.Descriptors {

  /// <summary>
  /// The non-generic form of the IDescriptor interface. Useful for describing a general
  /// descriptor.
  /// </summary>
  public interface IDescriptor {
    string Name { get; }
    string FullName { get; }
    FileDescriptor File { get; }
    IMessage Proto { get; }
  }

  /// <summary>
  /// Strongly-typed form of the IDescriptor interface.
  /// </summary>
  /// <typeparam name="TProto">Protocol buffer type underlying this descriptor type</typeparam>
  public interface IDescriptor<TProto> : IDescriptor where TProto : IMessage {
    new TProto Proto { get; }
  }
}
