using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Google.Protobuf;

namespace Google.Protobuf.FieldAccess
{
    /// <summary>
    /// Accessor for repeated fields.
    /// </summary>
    /// <typeparam name="T">The type of message containing the field.</typeparam>
    internal sealed class RepeatedFieldAccessor<T> : FieldAccessorBase<T> where T : IMessage<T>
    {
        internal RepeatedFieldAccessor(string name) : base(name)
        {
        }

        public override void Clear(T message)
        {
            IList list = (IList) GetValue(message);
            list.Clear();
        }

        public override bool HasValue(T message)
        {
            throw new NotImplementedException("HasValue is not implemented for repeated fields");
        }

        public override void SetValue(T message, object value)
        {
            throw new NotImplementedException("SetValue is not implemented for repeated fields");
        }

    }
}
