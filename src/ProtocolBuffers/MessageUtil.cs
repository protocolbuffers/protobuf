using System;
using System.Collections.Generic;
using System.Reflection;
using System.Text;

namespace Google.ProtocolBuffers {
  /// <summary>
  /// Utilities for arbitrary messages of an unknown type. This class does not use
  /// generics precisely because it is designed for dynamically discovered types.
  /// </summary>
  public static class MessageUtil {

    /// <summary>
    /// Returns the default message for the given type. If an exception is thrown
    /// (directly from this code), the message will be suitable to be displayed to a user.
    /// </summary>
    /// <param name="type"></param>
    /// <exception cref="ArgumentNullException">The type parameter is null.</exception>
    /// <exception cref="ArgumentException">The type doesn't implement IMessage, or doesn't
    /// have a static DefaultMessage property of the same type, or is generic or abstract.</exception>
    /// <returns></returns>
    public static IMessage GetDefaultMessage(Type type) {
      if (type == null) {
        throw new ArgumentNullException("type", "No type specified.");
      }
      if (type.IsAbstract || type.IsGenericTypeDefinition) {
        throw new ArgumentException("Unable to get a default message for an abstract or generic type (" + type.FullName + ")");
      }
      if (!typeof(IMessage).IsAssignableFrom(type)) {
        throw new ArgumentException("Unable to get a default message for non-message type (" + type.FullName + ")");
      }
      PropertyInfo property = type.GetProperty("DefaultInstance", BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic);
      if (property == null) {
        throw new ArgumentException(type.FullName + " doesn't have a static DefaultInstance property");
      }
      if (property.PropertyType != type) {
        throw new ArgumentException(type.FullName + ".DefaultInstance property is of the wrong type");
      }
      return (IMessage) property.GetValue(null, null);
    }

    /// <summary>
    /// Returns the default message for the type with the given name. This is just
    /// a convenience wrapper around calling Type.GetType and then GetDefaultMessage.
    /// If an exception is thrown, the message will be suitable to be displayed to a user.
    /// </summary>
    /// <param name="typeName"></param>
    /// <exception cref="ArgumentNullException">The typeName parameter is null.</exception>
    /// <exception cref="ArgumentException">The type doesn't implement IMessage, or doesn't
    /// have a static DefaultMessage property of the same type, or can't be found.</exception>
    public static IMessage GetDefaultMessage(string typeName) {
      if (typeName == null) {
        throw new ArgumentNullException("typeName", "No type name specified.");
      }
      Type type = Type.GetType(typeName);
      if (type == null) {
        throw new ArgumentException("Unable to load type " + typeName);
      }
      return GetDefaultMessage(type);
    }
  }
}
