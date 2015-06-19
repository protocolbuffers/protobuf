using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Text;
using Google.Protobuf;
using Google.Protobuf.FieldAccess;

namespace Google.Protobuf.FieldAccess
{
    /// <summary>
    /// Base class for field accessors.
    /// </summary>
    /// <typeparam name="T">Type of message containing the field</typeparam>
    internal abstract class FieldAccessorBase<T> : IFieldAccessor<T> where T : IMessage<T>
    {
        private readonly Func<T, object> getValueDelegate;

        internal FieldAccessorBase(string name)
        {
            PropertyInfo property = typeof(T).GetProperty(name);
            if (property == null || !property.CanRead)
            {
                throw new ArgumentException("Not all required properties/methods available");
            }
            getValueDelegate = ReflectionUtil.CreateUpcastDelegate<T>(property.GetGetMethod());
        }

        public object GetValue(T message)
        {
            return getValueDelegate(message);
        }

        public abstract bool HasValue(T message);
        public abstract void Clear(T message);
        public abstract void SetValue(T message, object value);
    }
}
