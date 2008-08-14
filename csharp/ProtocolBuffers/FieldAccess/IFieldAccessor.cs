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

  /// <summary>
  /// Allows fields to be reflectively accessed in a smart manner.
  /// The property descriptors for each field are created once and then cached.
  /// In addition, this interface holds knowledge of repeated fields, builders etc.
  /// </summary>
  internal interface IFieldAccessor {

    /// <summary>
    /// Indicates whether the specified message contains the field.
    /// </summary>
    bool Has(IMessage message);

    /// <summary>
    /// Gets the count of the repeated field in the specified message.
    /// </summary>
    int GetRepeatedCount(IMessage message);

    /// <summary>
    /// Clears the field in the specified builder.
    /// </summary>
    /// <param name="builder"></param>
    void Clear(IBuilder builder);

    /// <summary>
    /// Creates a builder for the type of this field (which must be a message field).
    /// </summary>
    IBuilder CreateBuilder();

    /// <summary>
    /// Accessor for single fields
    /// </summary>
    object GetValue(IMessage message);
    /// <summary>
    /// Mutator for single fields
    /// </summary>
    void SetValue(IBuilder builder, object value);

    /// <summary>
    /// Accessor for repeated fields
    /// </summary>
    object GetRepeatedValue(IMessage message, int index);
    /// <summary>
    /// Mutator for repeated fields
    /// </summary>
    void SetRepeated(IBuilder builder, int index, object value);
    /// <summary>
    /// Adds the specified value to the field in the given builder.
    /// </summary>
    void AddRepeated(IBuilder builder, object value);
    /// <summary>
    /// Returns a read-only wrapper around the value of a repeated field.
    /// </summary>
    object GetRepeatedWrapper(IBuilder builder);
  }
}
