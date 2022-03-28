using System;

namespace Google.Protobuf
{
    /// <summary>
    /// Struct used to hold the keys for the fieldByNumber table in DescriptorPool and the keys for the 
    /// extensionByNumber table in ExtensionRegistry.
    /// </summary>
    internal struct ObjectIntPair<T> : IEquatable<ObjectIntPair<T>> where T : class
    {
        private readonly int number;
        private readonly T obj;

        internal ObjectIntPair(T obj, int number)
        {
            this.number = number;
            this.obj = obj;
        }

        public bool Equals(ObjectIntPair<T> other)
        {
            return obj == other.obj
                   && number == other.number;
        }

        public override bool Equals(object obj)
        {
            if (obj is ObjectIntPair<T>)
            {
                return Equals((ObjectIntPair<T>)obj);
            }
            return false;
        }

        public override int GetHashCode()
        {
            return obj.GetHashCode() * ((1 << 16) - 1) + number;
        }
    }
}
