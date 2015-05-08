using System;
using System.Reflection;
using Google.ProtocolBuffers.Descriptors;

namespace Google.ProtocolBuffers.FieldAccess
{
    /// <summary>
    /// Access for an oneof
    /// </summary>
    internal class OneofAccessor<TMessage, TBuilder>
        where TMessage : IMessage<TMessage, TBuilder>
        where TBuilder : IBuilder<TMessage, TBuilder>
    {
        private readonly Func<TMessage, int> caseDelegate;
        private readonly Func<TBuilder, IBuilder> clearDelegate;
        private MessageDescriptor descriptor;

        internal OneofAccessor(MessageDescriptor descriptor, string name) 
        {
            this.descriptor = descriptor;
            MethodInfo clearMethod = typeof(TBuilder).GetMethod("Clear" + name);
            PropertyInfo caseProperty = typeof(TMessage).GetProperty(name + "Case");
            if (clearMethod == null || caseProperty == null)
            {
                throw new ArgumentException("Not all required properties/methods available for oneof");
            }
            

            clearDelegate = ReflectionUtil.CreateDelegateFunc<TBuilder, IBuilder>(clearMethod);
            caseDelegate = ReflectionUtil.CreateDelegateFunc<TMessage, int>(caseProperty.GetGetMethod());
        }

        /// <summary>
        /// Indicates whether the specified message has set any field in the oneof.
        /// </summary>
        public bool Has(TMessage message)
        {
            return (caseDelegate(message) != 0);
        }

        /// <summary>
        /// Clears the oneof in the specified builder.
        /// </summary>
        public void Clear(TBuilder builder)
        {
            clearDelegate(builder);
        }

        /// <summary>
        /// Indicates which field in the oneof is set for specified message
        /// </summary>
        public virtual FieldDescriptor GetOneofFieldDescriptor(TMessage message)
        {
            int fieldNumber = caseDelegate(message);
            if (fieldNumber > 0)
            {
                return descriptor.FindFieldByNumber(fieldNumber);
            }
            return null;
        }
    }
}
