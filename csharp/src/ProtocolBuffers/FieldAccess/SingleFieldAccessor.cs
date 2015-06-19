using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Text;
using Google.Protobuf;
using Google.Protobuf.Descriptors;
using Google.Protobuf.FieldAccess;

namespace Google.Protobuf.FieldAccess
{
    /// <summary>
    /// Accessor for single fields.
    /// </summary>
    /// <typeparam name="T">The type of message containing the field.</typeparam>
    internal sealed class SingleFieldAccessor<T> : FieldAccessorBase<T> where T : IMessage<T>
    {
        // All the work here is actually done in the constructor - it creates the appropriate delegates.
        // There are various cases to consider, based on the property type (message, string/bytes, or "genuine" primitive)
        // and proto2 vs proto3 for non-message types, as proto3 doesn't support "full" presence detection or default
        // values.

        private readonly Action<T, object> setValueDelegate;
        private readonly Action<T> clearDelegate;
        private readonly Func<T, bool> hasValueDelegate;

        internal SingleFieldAccessor(FieldDescriptor descriptor, string name, bool supportsFieldPresence) : base(name)
        {
            PropertyInfo property = typeof(T).GetProperty(name);
            // We know there *is* such a property, or the base class constructor would have thrown. We should be able to write
            // to it though.
            if (!property.CanWrite)
            {
                throw new ArgumentException("Not all required properties/methods available");
            }
            setValueDelegate = ReflectionUtil.CreateDowncastDelegate<T>(property.GetSetMethod());

            var clrType = property.PropertyType;

            if (typeof(IMessage).IsAssignableFrom(clrType))
            {
                // Message types are simple - we only need to detect nullity.
                clearDelegate = message => SetValue(message, null);
                hasValueDelegate = message => GetValue(message) == null;
            }

            if (supportsFieldPresence)
            {
                // Proto2: we expect a HasFoo property and a ClearFoo method.
                // For strings and byte arrays, setting the property to null would have the equivalent effect,
                // but we generate the method for consistency, which makes this simpler.
                PropertyInfo hasProperty = typeof(T).GetProperty("Has" + name);
                MethodInfo clearMethod = typeof(T).GetMethod("Clear" + name);
                if (hasProperty == null || clearMethod == null || !hasProperty.CanRead)
                {
                    throw new ArgumentException("Not all required properties/methods available");
                }
                hasValueDelegate = ReflectionUtil.CreateDelegateFunc<T, bool>(hasProperty.GetGetMethod());
                clearDelegate = ReflectionUtil.CreateDelegateAction<T>(clearMethod);
            }
            else
            {
                /*
                // TODO(jonskeet): Reimplement. We need a better way of working out default values.
                // Proto3: for field detection, we just need the default value of the field (0, "", byte[0] etc)
                // To clear a field, we set the value to that default.
                object defaultValue = descriptor.DefaultValue;
                hasValueDelegate = message => GetValue(message).Equals(defaultValue);
                clearDelegate = message => SetValue(message, defaultValue);
                */
            }
        }

        public override bool HasValue(T message)
        {
            return hasValueDelegate(message);
        }

        public override void Clear(T message)
        {
            clearDelegate(message);
        }

        public override void SetValue(T message, object value)
        {
            setValueDelegate(message, value);
        }
    }
}
