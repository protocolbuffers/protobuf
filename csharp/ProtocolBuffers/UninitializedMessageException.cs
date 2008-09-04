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
using System.Collections;
using System.Collections.Generic;
using System.Text;
using Google.ProtocolBuffers.Collections;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers {
  /// <summary>
  /// TODO(jonskeet): Write summary text.
  /// </summary>
  public sealed class UninitializedMessageException : Exception {

    private readonly IList<string> missingFields;

    public UninitializedMessageException(IMessage message)
        : this(FindMissingFields(message)) {
    }

    private UninitializedMessageException(IList<string> missingFields)
        : base(BuildDescription(missingFields)) {
      this.missingFields = Lists.AsReadOnly(missingFields);
    }


    /// <summary>
    /// Converts this exception into an InvalidProtocolBufferException.
    /// When a parsed message is missing required fields, this should be thrown
    /// instead of UninitializedMessageException.
    /// </summary>
    public InvalidProtocolBufferException AsInvalidProtocolBufferException() {
      return new InvalidProtocolBufferException(Message);
    }

    /// <summary>
    /// Constructs the description string for a given list of missing fields.
    /// </summary>
    private static string BuildDescription(IEnumerable<string> missingFields) {
      StringBuilder description = new StringBuilder("Message missing required fields: ");
      bool first = true;
      foreach(string field in missingFields) {
        if (first) {
          first = false;
        } else {
          description.Append(", ");
        }
        description.Append(field);
      }
      return description.ToString();
    }

    /// <summary>
    /// Returns a list of the full "paths" of missing required
    /// fields in the specified message.
    /// </summary>
    private static IList<String> FindMissingFields(IMessage message) {
      List<String> results = new List<String>();
      FindMissingFields(message, "", results);
      return results;
    }

    /// <summary>
    /// Recursive helper implementing FindMissingFields.
    /// </summary>
    private static void FindMissingFields(IMessage message, String prefix, List<String> results) {
      foreach (FieldDescriptor field in message.DescriptorForType.Fields) {
        if (field.IsRequired && !message.HasField(field)) {
          results.Add(prefix + field.Name);
        }
      }

      foreach (KeyValuePair<FieldDescriptor, object> entry in message.AllFields) {
        FieldDescriptor field = entry.Key;
        object value = entry.Value;

        if (field.MappedType == MappedType.Message) {
          if (field.IsRepeated) {
            int i = 0;
            foreach (object element in (IEnumerable) value) {
              FindMissingFields((IMessage) element, SubMessagePrefix(prefix, field, i++), results);
            }
          } else {
            if (message.HasField(field)) {
              FindMissingFields((IMessage) value, SubMessagePrefix(prefix, field, -1), results);
            }
          }
        }
      }
    }

    private static String SubMessagePrefix(String prefix, FieldDescriptor field, int index) {
      StringBuilder result = new StringBuilder(prefix);
      if (field.IsExtension) {
        result.Append('(')
              .Append(field.FullName)
              .Append(')');
      } else {
        result.Append(field.Name);
      }
      if (index != -1) {
        result.Append('[')
              .Append(index)
              .Append(']');
      }
      result.Append('.');
      return result.ToString();
    }
  }
}

  

