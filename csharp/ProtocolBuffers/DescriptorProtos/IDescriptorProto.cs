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
namespace Google.ProtocolBuffers.DescriptorProtos {

  /// <summary>
  /// Interface implemented by all DescriptorProtos. The generator doesn't
  /// emit the interface implementation claim, so PartialClasses.cs contains
  /// partial class declarations for each of them.
  /// </summary>
  /// <typeparam name="TOptions">The associated options protocol buffer type</typeparam>
  public interface IDescriptorProto<TOptions> {

    /// <summary>
    /// The brief name of the descriptor's target.
    /// </summary>
    string Name { get; }

    /// <summary>
    /// The options for this descriptor.
    /// </summary>
    TOptions Options { get; }
  }
}
